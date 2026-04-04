#ifndef TASKS_VITALS_H
#define TASKS_VITALS_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

typedef struct {
    int32_t  temp_x10;  /* Celsius × 10, e.g. 229 = 22.9 °C */
    uint32_t ir_raw;
    uint32_t ir_filt;
    uint32_t bpm;
} VitalsMsg;

extern QueueHandle_t vitals_queue;

void task_sensor(void *arg);
void task_uart(void *arg);

#endif /* TASKS_VITALS_H */
