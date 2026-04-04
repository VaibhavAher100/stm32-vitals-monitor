#include "filter.h"

void filter_init(Filter *f)
{
    uint8_t i;
    for(i = 0; i < (uint8_t)FILTER_WINDOW; i++) { /* Rule 10.4: matching types */
        f->buf[i] = 0U;                            /* Rule 15.6: braces on for body */
    }
    f->idx   = 0U;
    f->count = 0U;
    f->sum   = 0U;
}

uint32_t filter_update(Filter *f, uint32_t val)
{
    f->sum -= f->buf[f->idx];
    f->buf[f->idx] = val;
    f->sum += val;
    f->idx = (uint8_t)((f->idx + 1U) % (uint8_t)FILTER_WINDOW); /* Rule 10.4 */
    if(f->count < (uint8_t)FILTER_WINDOW) {       /* Rule 10.4, Rule 15.6 */
        f->count++;
    }
    return f->sum / f->count;
}
