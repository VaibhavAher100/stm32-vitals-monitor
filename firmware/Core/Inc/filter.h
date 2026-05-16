#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

#define FILTER_WINDOW 4

typedef struct {
    uint32_t buf[FILTER_WINDOW];
    uint8_t  idx;
    uint8_t  count;
    uint32_t sum;
} Filter;

void     filter_init(Filter *f);
uint32_t filter_update(Filter *f, uint32_t val);

#endif /* FILTER_H */
