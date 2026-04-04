/**
 * tasks_vitals.c — FreeRTOS task definitions for the vitals monitor.
 *
 * task_sensor  (priority 2): reads sensors, filters, detects BPM, sends to queue.
 * task_uart    (priority 1): blocks on queue, formats and transmits UART row.
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

/* Queue handle — declared extern in tasks_vitals.h, defined here */
QueueHandle_t vitals_queue;

/* ------------------------------------------------------------------
 * task_sensor — priority 2
 * Reads TMP117 + MAX30102 every 500 ms, applies filter and BPM
 * detector, packs results into VitalsMsg and sends to vitals_queue.
 * Also kicks the IWDG so a hung sensor task triggers a reset.
 * ------------------------------------------------------------------ */
void task_sensor(void *arg)
{
    Filter      ir_filter;
    BpmDetector bpm;

    (void)arg;

    /* Sensor init runs here — after scheduler start, so vTaskDelay() is safe */
    if(tmp117_init()) {
        uart_str("TMP117   OK\r\n");
    } else {
        uart_str("TMP117   FAIL\r\n");
    }

    if(max30102_init()) {
        uart_str("MAX30102 OK\r\n");
    } else {
        uart_str("MAX30102 FAIL\r\n");
    }

    uart_str("========================\r\n");

    filter_init(&ir_filter);
    bpm_init(&bpm);

    for(;;)
    {
        VitalsMsg msg;

        msg.temp_x10 = tmp117_read_celsius_x10();
        msg.ir_raw   = max30102_read_ir();
        msg.ir_filt  = filter_update(&ir_filter, msg.ir_raw);

        bpm_update(&bpm, msg.ir_filt);
        msg.bpm = bpm_get(&bpm);

        /* Overwrite old item if not yet consumed — depth-1 queue */
        xQueueOverwrite(vitals_queue, &msg);

        iwdg_kick();
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}

/* ------------------------------------------------------------------
 * task_uart — priority 1
 * Blocks waiting for a VitalsMsg from vitals_queue, then formats
 * and transmits a single UART row.
 * ------------------------------------------------------------------ */
void task_uart(void *arg)
{
    VitalsMsg msg;

    (void)arg;

    for(;;)
    {
        xQueueReceive(vitals_queue, &msg, portMAX_DELAY);

        uart_int(msg.temp_x10 / 10);
        uart_char('.');
        uart_int(msg.temp_x10 % 10);
        uart_str("       | ");
        uart_int((int32_t)msg.ir_raw);
        uart_str("  | ");
        uart_int((int32_t)msg.ir_filt);
        uart_str("  | ");
        if(msg.bpm == BPM_INVALID) {
            uart_str("---");
        } else {
            uart_int((int32_t)msg.bpm);
        }
        uart_str("\r\n");
    }
}
