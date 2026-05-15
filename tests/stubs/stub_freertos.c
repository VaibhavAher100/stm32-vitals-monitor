#include "task.h"

static uint32_t s_tick_ms = 0U;

void stub_set_tick(uint32_t ms)
{
    s_tick_ms = ms;
}

TickType_t xTaskGetTickCount(void)
{
    return (TickType_t)s_tick_ms;
}
