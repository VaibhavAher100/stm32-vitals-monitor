#ifndef TASK_H
#define TASK_H

#include "FreeRTOS.h"

TickType_t xTaskGetTickCount(void);
void       stub_set_tick(uint32_t ms);

#endif /* TASK_H */
