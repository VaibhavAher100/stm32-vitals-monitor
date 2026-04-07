#ifndef BPM_H
#define BPM_H

#include <stdint.h>

/* Number of samples used to track rolling min/max (~4 s at 500 ms/sample) */
#define BPM_HISTORY        8U

/* Minimum interval between beats: 333 ms → 180 BPM maximum */
#define BPM_REFRACTORY_MS  333U

/* Returned when fewer than two crossings have been seen */
#define BPM_INVALID        0U

typedef struct {
    uint32_t history[BPM_HISTORY]; /* rolling window of filtered IR values      */
    uint8_t  hist_idx;             /* next write position in history[]           */
    uint8_t  hist_count;           /* samples accumulated (saturates at BPM_HISTORY) */
    uint32_t prev_val;             /* filtered IR from previous call             */
    uint32_t last_cross_ms;        /* get_tick() at last threshold crossing      */
    uint32_t interval_ms;          /* ms between the two most recent crossings   */
    uint8_t  cross_count;          /* number of valid crossings seen so far      */
} BpmDetector;

void     bpm_init(BpmDetector *b);
void     bpm_update(BpmDetector *b, uint32_t ir_filt);
uint32_t bpm_get(const BpmDetector *b);

#endif /* BPM_H */
