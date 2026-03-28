#include "filter.h"

void filter_init(Filter *f)
{
    uint8_t i;
    for(i = 0; i < FILTER_WINDOW; i++) f->buf[i] = 0;
    f->idx   = 0;
    f->count = 0;
    f->sum   = 0;
}

uint32_t filter_update(Filter *f, uint32_t val)
{
    f->sum -= f->buf[f->idx];
    f->buf[f->idx] = val;
    f->sum += val;
    f->idx = (f->idx + 1) % FILTER_WINDOW;
    if(f->count < FILTER_WINDOW) f->count++;
    return f->sum / f->count;
}
