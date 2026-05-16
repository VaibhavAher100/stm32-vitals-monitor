/* Implements: REQ-SYS-03, REQ-SYS-04, REQ-SYS-05, REQ-RTOS-02, REQ-RTOS-03,
               REQ-RTOS-04, REQ-OUT-01, REQ-OUT-02, REQ-OUT-03, REQ-OUT-04 */

/**
 * tasks_vitals.c - FreeRTOS task definitions for the vitals monitor.
 *
 * task_sensor  (priority 2): reads sensors, filters, detects BPM, sends to queue.
 * task_uart    (priority 1): blocks on queue, formats and transmits UART row.
 *
 * Signal path (per FIFO sample at 25 Hz):
 *   raw → IIR DC estimator → AC = raw - DC + 32768
 *       → FILTER_WINDOW moving average → bpm_update(tick)
 *
 * Each FIFO sample gets its own timestamp: the last sample in a batch is at
 * xTaskGetTickCount(), earlier samples are 40 ms apart going back. This gives
 * the crossing detector accurate inter-beat intervals instead of multiples of
 * the 500 ms drain period, which caused 37/55/111 BPM aliasing artifacts.
 *
 * Finger detection gates on raw DC level, not the AC signal.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tasks_vitals.h"
#include "iwdg.h"
#include "tmp117.h"
#include "max30102.h"
#include "filter.h"
#include "bpm.h"
#include "uart.h"
#include "i2c.h"

/* Minimum raw DC level to consider a finger present.
   Ambient (no finger): ~200-400. Partial contact: 2000-8000. Full contact: >15000.
   Threshold set above partial-contact range to avoid spurious DC re-initialization. */
#define PPG_MIN_DC_RAW  10000U

/* IIR DC estimator shift: tau = 2^DC_SHIFT samples × 40 ms = 2560 ms at 25 Hz.
   Must be >> heartbeat period (~1s at 60 BPM) to avoid tracking the AC signal.
   DC_SHIFT=3 (320ms) was too fast and suppressed heartbeat; 6 (2.56s) is correct.
   Stored scaled by 2^DC_SHIFT to preserve precision without floating point. */
#define DC_SHIFT        6U

/* Center offset added to AC signal to keep it in uint32_t positive range.
   AC = raw - DC_estimate + AC_CENTER. Heartbeat oscillates around AC_CENTER. */
#define AC_CENTER       32768U

/* Queue handle — declared extern in tasks_vitals.h, defined here */
QueueHandle_t vitals_queue;

/* ------------------------------------------------------------------
 * task_sensor — priority 2
 * ------------------------------------------------------------------ */
void task_sensor(void *arg)
{
    Filter      ac_filter;
    BpmDetector bpm;
    uint32_t    dc_est    = 0U;        /* IIR DC estimate, stored scaled by 2^DC_SHIFT */
    uint32_t    prev_bpm  = BPM_INVALID;
    uint32_t    stable_bpm = BPM_INVALID; /* only updates when two consecutive readings agree */

    (void)arg;

    /* Sensor init runs here — after scheduler start, so vTaskDelay() is safe */
    {
        uint8_t tmp_ok = tmp117_init();
        uint8_t max_ok = max30102_init();

        uart_str(tmp_ok != 0U ? "TMP117   OK\r\n" : "TMP117   FAIL\r\n");
        uart_str(max_ok != 0U ? "MAX30102 OK\r\n" : "MAX30102 FAIL\r\n");

        /* Both sensors required — starve IWDG on failure to force a reset */
        if ((tmp_ok == 0U) || (max_ok == 0U)) {
            for(;;) {}
        }
    }

    uart_str("========================\r\n");

    filter_init(&ac_filter);
    bpm_init(&bpm);

    for(;;)
    {
        VitalsMsg msg;
        uint32_t  fifo_buf[16];
        uint8_t   n;

        msg.temp_x10 = tmp117_read_celsius_x10();

        /* Drain all accumulated FIFO samples (~12 per 500 ms at 25 effective SPS).
           Capture tick AFTER drain so the last sample (i = n-1) maps to now_ticks,
           and earlier samples are at now_ticks - (n-1-i)*40 ms. */
        n = max30102_drain_fifo(fifo_buf, 16U);
        if (n == 0U) {
            fifo_buf[0] = max30102_read_red();
            n = 1U;
        }
        {
            uint32_t now_ticks = (uint32_t)xTaskGetTickCount();
            uint8_t  i;

            for (i = 0U; i < n; i++) {
                uint32_t raw         = fifo_buf[i];
                /* Per-sample timestamp: 40 ms per sample at 25 effective SPS */
                uint32_t sample_tick = now_ticks - ((uint32_t)(n - 1U - i) * 40U);

                if (raw < PPG_MIN_DC_RAW) {
                    /* No finger — reset BPM state and DC estimator */
                    bpm_reset(&bpm);
                    filter_init(&ac_filter);
                    dc_est     = 0U;
                    prev_bpm   = BPM_INVALID;
                    stable_bpm = BPM_INVALID;
                    msg.red_raw  = raw;
                    msg.red_filt = 0U;
                } else {
                    uint32_t dc;
                    uint32_t ac;
                    uint32_t ac_filt;
                    /* One-pole IIR DC estimator: y = y*(1 - 1/64) + x*(1/64)
                       Stored scaled by 64 to avoid fractional loss. */
                    if (dc_est == 0U) { dc_est = raw << DC_SHIFT; }
                    dc_est = dc_est - (dc_est >> DC_SHIFT) + raw;
                    dc = dc_est >> DC_SHIFT;

                    /* AC component centered at AC_CENTER */
                    ac = (raw >= dc) ? (raw - dc + AC_CENTER) : (AC_CENTER - (dc - raw));

                    ac_filt = filter_update(&ac_filter, ac);
                    bpm_update(&bpm, ac_filt, sample_tick);

                    msg.red_raw  = raw;
                    msg.red_filt = dc; /* show DC estimate — useful for finger contact quality */
                }
            }
        }

        /* Consensus filter: only update stable_bpm when two consecutive readings agree.
           BPM_INVALID propagates immediately (finger removed).
           Plausibility gate: reject readings < 60% of current stable — these are
           half-rate (missed-beat) artifacts from the Schmitt trigger, not real BPM drops. */
        {
            uint32_t raw_bpm = bpm_get(&bpm);
            if (raw_bpm == BPM_INVALID) {
                prev_bpm   = BPM_INVALID;
                stable_bpm = BPM_INVALID;
            } else {
                uint8_t plausible = 1U;
                if ((stable_bpm != BPM_INVALID) && (raw_bpm < ((stable_bpm * 3U) / 5U))) {
                    plausible = 0U; /* <60% of stable: likely half/third-rate artifact */
                }
                if (plausible != 0U) {
                    uint32_t diff = (raw_bpm > prev_bpm) ? (raw_bpm - prev_bpm) : (prev_bpm - raw_bpm);
                    if (diff <= 2U) {
                        stable_bpm = raw_bpm;
                    }
                }
            }
            prev_bpm = raw_bpm;
        }
        msg.bpm = stable_bpm;

        (void)xQueueOverwrite(vitals_queue, &msg);

        iwdg_kick();
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}

/* ------------------------------------------------------------------
 * task_uart — priority 1
 * ------------------------------------------------------------------ */
void task_uart(void *arg)
{
    VitalsMsg msg;

    (void)arg;

    for(;;)
    {
        (void)xQueueReceive(vitals_queue, &msg, portMAX_DELAY);

        if (msg.temp_x10 < 0) {
            uart_char('-');
            uart_int(-(msg.temp_x10 / 10));
            uart_char('.');
            uart_int(-(msg.temp_x10 % 10));
        } else {
            uart_int(msg.temp_x10 / 10);
            uart_char('.');
            uart_int(msg.temp_x10 % 10);
        }
        uart_str("       | ");
        uart_int((int32_t)msg.red_raw);
        uart_str("  | ");
        uart_int((int32_t)msg.red_filt);
        uart_str("  | ");
        if (msg.bpm == BPM_INVALID) {
            uart_str("---");
        } else {
            uart_int((int32_t)msg.bpm);
        }
        uart_str("\r\n");
    }
}

/* Called by FreeRTOS when configCHECK_FOR_STACK_OVERFLOW detects overflow.
   Spin here — IWDG fires after 4 s and forces a reset. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    uart_str("STACK OVERFLOW\r\n");
    for(;;) {}
}
