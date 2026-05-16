/* Implements: REQ-FIL-01, REQ-FIL-02, REQ-FIL-03, REQ-FIL-04, REQ-FIL-05, REQ-FIL-06 */
#include "filter.h"

void filter_init(Filter *f)
{
    uint8_t i;
    for (i = 0; i < (uint8_t)FILTER_WINDOW; i++) {
        f->buf[i] = 0U;
    }
    f->idx   = 0U;
    f->count = 0U;
    f->sum   = 0U;
}

/* O(1) moving average using circular buffer and running sum. */
uint32_t filter_update(Filter *f, uint32_t val)
{
    f->sum -= f->buf[f->idx];
    f->buf[f->idx] = val;
    f->sum += val;
    f->idx = (uint8_t)((f->idx + 1U) % (uint8_t)FILTER_WINDOW);
    if (f->count < (uint8_t)FILTER_WINDOW) {
        f->count++;
    }
    return (f->sum + (uint32_t)(f->count / 2U)) / (uint32_t)f->count;
}
