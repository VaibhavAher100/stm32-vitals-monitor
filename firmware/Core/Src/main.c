/**
 * STM32 Vitals Monitor - Application Layer (Phase 4: FreeRTOS)
 * Bare-metal, no HAL. All hardware access via driver layer.
 *
 * Architecture:
 *   main.c            - RTOS startup; creates queue and tasks
 *   tasks_vitals.c    - task_sensor + task_uart implementations
 *   filter.c/h        - processing layer
 *   uart/i2c/tmp117/max30102 - driver layer
 *
 * FreeRTOS owns SysTick — systick.c is removed.
 * Sensor init (tmp117_init, max30102_init) runs inside task_sensor
 * after the scheduler starts, so vTaskDelay() is safe to call.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "tasks_vitals.h"
#include "iwdg.h"
#include "uart.h"
#include "i2c.h"

int main(void)
{
    /* Hardware init that must happen before the scheduler starts.
       These functions must NOT call vTaskDelay(). */
    iwdg_init();
    uart_init();
    i2c_init();

    uart_str("========================\r\n");
    uart_str("STM32 Vitals Monitor\r\n");
    uart_str("========================\r\n");
    uart_str("Temp(C) | IR raw  | IR filt | BPM\r\n");
    uart_str("--------+---------+---------+----\r\n");

    /* Depth-1 queue: sensor overwrites stale reading if UART is slow */
    vitals_queue = xQueueCreate(1U, sizeof(VitalsMsg));

    xTaskCreate(task_sensor, "Sensor", 256U, NULL, 2U, NULL);
    xTaskCreate(task_uart,   "UART",   256U, NULL, 1U, NULL);

    vTaskStartScheduler();

    /* Never reached */
    for(;;) {}
}
