#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>

typedef uint32_t TickType_t;
typedef uint32_t UBaseType_t;
typedef int32_t  BaseType_t;

#define pdTRUE  ((BaseType_t)1)
#define pdFALSE ((BaseType_t)0)

#define portMAX_DELAY       ((TickType_t)0xffffffffUL)

/* At 1 kHz tick rate: 1 tick = 1 ms */
#define portTICK_PERIOD_MS  ((TickType_t)1U)
#define pdMS_TO_TICKS(ms)   ((TickType_t)(ms))

#endif /* FREERTOS_H */
