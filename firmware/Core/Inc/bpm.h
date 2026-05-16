#ifndef BPM_H
#define BPM_H

#include <stdint.h>

/* Number of samples used to track rolling min/max (~1 s at 25 effective SPS) */
#define BPM_HISTORY        25U

/* Minimum interval between beats: 600 ms → 100 BPM maximum.
   Must exceed dicrotic notch timing (~535 ms at 56 BPM) to prevent
   the notch from being counted as a second beat. */
#define BPM_REFRACTORY_MS  600U

/* Returned when fewer than two crossings have been seen */
#define BPM_INVALID        0U

/* Physiological BPM range — max capped at 100 by refractory period */
#define BPM_MIN            30U
#define BPM_MAX            100U

typedef struct {
    uint32_t history[BPM_HISTORY]; /* rolling window of filtered AC PPG values  */
    uint8_t  hist_idx;             /* next write position in history[]           */
    uint8_t  hist_count;           /* samples accumulated (saturates at BPM_HISTORY) */
    uint32_t prev_val;             /* filtered AC PPG from previous call         */
    uint32_t last_cross_ticks;     /* xTaskGetTickCount() at last threshold crossing */
    uint32_t interval_ticks;       /* ticks between two most recent crossings — bpm_get converts via portTICK_PERIOD_MS */
    uint8_t  cross_count;          /* number of valid crossings seen so far      */
    uint8_t  below_lower;          /* hysteresis flag: signal was below lower band since last crossing */
} BpmDetector;

void     bpm_init(BpmDetector *b);
void     bpm_reset(BpmDetector *b);   /* clear crossing state — call when signal lost */
/* tick: FreeRTOS tick at which this sample was captured.
   Caller computes per-sample tick from xTaskGetTickCount() and sample position in FIFO batch. */
void     bpm_update(BpmDetector *b, uint32_t ir_filt, uint32_t tick);
uint32_t bpm_get(const BpmDetector *b);

#endif /* BPM_H */
