#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Target: STM32L476RG, Cortex-M4, 4 MHz MSI clock, no HAL */

#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      4000000UL
#define configTICK_RATE_HZ                      1000U
#define configMAX_PRIORITIES                    5U
#define configMINIMAL_STACK_SIZE                128U
#define configTOTAL_HEAP_SIZE                   4096U
#define configMAX_TASK_NAME_LEN                 8U
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configUSE_TIMERS                        0
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* Cortex-M interrupt priority settings (4-bit priority, 16 levels) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5U
#define configKERNEL_INTERRUPT_PRIORITY         ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << 4U )
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << 4U )

/* Map FreeRTOS handlers to CMSIS names used in startup assembly */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/* Stack overflow detection — mode 2 checks stack canary on every context switch */
#define configCHECK_FOR_STACK_OVERFLOW      2

/* Trap on assertion failure — IWDG fires after 4 s */
#define configASSERT(x) \
    do { if ((x) == 0) { taskDISABLE_INTERRUPTS(); for(;;){} } } while(0)

/* Optional API includes */
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_vTaskDelete                 0
#define INCLUDE_vTaskSuspend                0
#define INCLUDE_xTaskGetCurrentTaskHandle   1
#define INCLUDE_uxTaskGetStackHighWaterMark 1

#endif /* FREERTOS_CONFIG_H */
