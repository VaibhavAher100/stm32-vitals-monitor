/* Implements: REQ-BPM-01, REQ-BPM-02, REQ-BPM-03, REQ-BPM-04, REQ-BPM-05, REQ-STK-02 */
#include "bpm.h"
#include "FreeRTOS.h"
#include "task.h"

/* Minimum peak-to-peak range over the history window to accept crossings.
   Hardware measures ~500-count AC swing at 12.6 mA LED on Nucleo-L476RG. */
#define PPG_MIN_AMPLITUDE 8U

/* Hysteresis band around the midpoint threshold.
   Dead zone = 2 * HYST. Must be < half the expected AC amplitude.
   16-count dead zone is well below the measured ~500-count signal swing. */
#define PPG_HYSTERESIS    8U

void bpm_init(BpmDetector *b)
{
    uint8_t i;
    for (i = 0U; i < (uint8_t)BPM_HISTORY; i++) {
        b->history[i] = 0U;
    }
    b->hist_idx         = 0U;
    b->hist_count       = 0U;
    b->prev_val         = 0U;
    b->last_cross_ticks = 0U;
    b->interval_ticks   = 0U;
    b->cross_count      = 0U;
    b->below_lower      = 1U; /* assume we start in the lower band */
}

void bpm_reset(BpmDetector *b)
{
    b->cross_count    = 0U;
    b->interval_ticks = 0U;
    b->hist_count     = 0U;
    b->hist_idx       = 0U;
    b->prev_val       = 0U;
    b->below_lower    = 1U;
}

/* Dynamic threshold: midpoint of rolling min/max over the history window.
   Returns 0 (sentinel) if signal amplitude is too small to be a real PPG pulse. */
static uint32_t compute_threshold(const BpmDetector *b)
{
    uint8_t  i;
    uint32_t mn;
    uint32_t mx;

    if (b->hist_count == 0U) { return 0U; }

    mn = b->history[0U];
    mx = b->history[0U];

    for (i = 1U; i < b->hist_count; i++) {
        if (b->history[i] < mn) { mn = b->history[i]; }
        if (b->history[i] > mx) { mx = b->history[i]; }
    }

    if ((mx - mn) < PPG_MIN_AMPLITUDE) { return 0U; }

    return mn + ((mx - mn) / 2U);
}

void bpm_update(BpmDetector *b, uint32_t ir_filt, uint32_t tick)
{
    uint32_t threshold;

    b->history[b->hist_idx] = ir_filt;
    b->hist_idx = (uint8_t)((b->hist_idx + 1U) % (uint8_t)BPM_HISTORY);
    if (b->hist_count < (uint8_t)BPM_HISTORY) {
        b->hist_count++;
    }

    /* Need at least a full window before threshold is meaningful */
    if (b->hist_count < (uint8_t)BPM_HISTORY) {
        b->prev_val = ir_filt;
        return;
    }

    threshold = compute_threshold(b);

    /* threshold == 0: signal amplitude too small — skip crossing detection */
    if (threshold == 0U) { b->prev_val = ir_filt; return; }

    /* Schmitt trigger: lower band = threshold - hyst, upper band = threshold + hyst.
       Signal must fall below lower band before rising-edge at upper band counts.
       This blocks noise-induced double-crossings without limiting physiological range. */
    {
        uint32_t lower = (threshold > PPG_HYSTERESIS) ? (threshold - PPG_HYSTERESIS) : 0U;
        uint32_t upper = threshold + PPG_HYSTERESIS;

        if (ir_filt <= lower) {
            b->below_lower = 1U;
        }

        if (b->below_lower && (ir_filt >= upper)) {
            b->below_lower = 0U;

            if (b->cross_count == 0U) {
                b->last_cross_ticks = tick;
                b->cross_count      = 1U;
            } else {
                uint32_t elapsed = tick - b->last_cross_ticks;
                if (elapsed >= pdMS_TO_TICKS(BPM_REFRACTORY_MS)) {
                    b->interval_ticks   = elapsed;
                    b->last_cross_ticks = tick;
                    if (b->cross_count < 255U) {
                        b->cross_count++;
                    }
                }
            }
        }
    }

    b->prev_val = ir_filt;
}

uint32_t bpm_get(const BpmDetector *b)
{
    uint32_t bpm;
    uint32_t interval_ms;

    if (b->cross_count < 2U) { return BPM_INVALID; }
    if (b->interval_ticks == 0U) { return BPM_INVALID; }

    /* Stale check: no crossing for longer than one BPM_MIN interval */
    if (((uint32_t)xTaskGetTickCount() - b->last_cross_ticks) > pdMS_TO_TICKS(60000U / BPM_MIN)) {
        return BPM_INVALID;
    }

    interval_ms = b->interval_ticks * (uint32_t)portTICK_PERIOD_MS;

    bpm = 60000U / interval_ms;
    if ((bpm < BPM_MIN) || (bpm > BPM_MAX)) { return BPM_INVALID; }
    return bpm;
}
