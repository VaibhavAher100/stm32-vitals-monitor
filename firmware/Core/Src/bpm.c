#include "bpm.h"
#include "FreeRTOS.h"
#include "task.h"

void bpm_init(BpmDetector *b)
{
    uint8_t i;
    for(i = 0U; i < (uint8_t)BPM_HISTORY; i++) { /* Rule 15.6 */
        b->history[i] = 0U;
    }
    b->hist_idx      = 0U;
    b->hist_count    = 0U;
    b->prev_val      = 0U;
    b->last_cross_ms = 0U;
    b->interval_ms   = 0U;
    b->cross_count   = 0U;
}

/* Compute midpoint threshold from rolling min/max of recent IR samples.
   Dynamic threshold adapts to sensor proximity and ambient light drift. */
static uint32_t compute_threshold(const BpmDetector *b)
{
    uint8_t  i;
    uint32_t mn;
    uint32_t mx;

    if(b->hist_count == 0U) {
        return 0U;
    }

    mn = b->history[0U];
    mx = b->history[0U];

    for(i = 1U; i < b->hist_count; i++) { /* Rule 15.6 */
        if(b->history[i] < mn) {
            mn = b->history[i];
        }
        if(b->history[i] > mx) {
            mx = b->history[i];
        }
    }

    /* Midpoint: min + (max - min) / 2 — integer division, no overflow risk
       since both are uint32_t PPG counts well below UINT32_MAX/2 */
    return mn + ((mx - mn) / 2U);
}

void bpm_update(BpmDetector *b, uint32_t ir_filt)
{
    uint32_t threshold;
    uint32_t now;
    uint32_t elapsed;

    /* Update rolling history window */
    b->history[b->hist_idx] = ir_filt;
    b->hist_idx = (uint8_t)((b->hist_idx + 1U) % (uint8_t)BPM_HISTORY); /* Rule 10.4 */
    if(b->hist_count < (uint8_t)BPM_HISTORY) { /* Rule 10.4, Rule 15.6 */
        b->hist_count++;
    }

    /* Need at least a full window before threshold is meaningful */
    if(b->hist_count < (uint8_t)BPM_HISTORY) {
        b->prev_val = ir_filt;
        return;
    }

    threshold = compute_threshold(b);

    /* Detect rising-edge threshold crossing: prev below, current at or above */
    if((b->prev_val < threshold) && (ir_filt >= threshold)) { /* Rule 14.4 */
        now = (uint32_t)xTaskGetTickCount();

        if(b->cross_count == 0U) {
            /* First crossing — record timestamp, nothing to measure yet */
            b->last_cross_ms = now;
            b->cross_count   = 1U;
        } else {
            /* Subsequent crossings — enforce refractory period */
            elapsed = now - b->last_cross_ms; /* unsigned subtraction handles rollover */
            if(elapsed >= BPM_REFRACTORY_MS) {
                b->interval_ms   = elapsed;
                b->last_cross_ms = now;
                if(b->cross_count < 255U) { /* Rule 10.4 */
                    b->cross_count++;
                }
            }
        }
    }

    b->prev_val = ir_filt;
}

uint32_t bpm_get(const BpmDetector *b)
{
    /* Need at least two crossings to have a valid interval */
    if(b->cross_count < 2U) {
        return BPM_INVALID;
    }
    /* BPM = 60000 ms/min / interval_ms — guard against zero interval */
    if(b->interval_ms == 0U) {
        return BPM_INVALID;
    }
    return 60000U / b->interval_ms;
}
