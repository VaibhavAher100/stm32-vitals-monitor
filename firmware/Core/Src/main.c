/**
 * STM32 Vitals Monitor - Phase 4: FreeRTOS
 *
 * Single task wrapping the Phase 3 polling loop.
 * FreeRTOS replaces systick.c — vTaskDelay owns SysTick.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "iwdg.h"
#include "uart.h"
#include "i2c.h"
#include "tmp117.h"
#include "max30102.h"
#include "filter.h"
#include "bpm.h"

void SystemInit(void) {}

static void task_vitals(void *arg)
{
    Filter      ir_filter;
    BpmDetector bpm;
    (void)arg;

    if(tmp117_init())   { uart_str("TMP117   OK\r\n");   }
    else                { uart_str("TMP117   FAIL\r\n"); }

    if(max30102_init()) { uart_str("MAX30102 OK\r\n");   }
    else                { uart_str("MAX30102 FAIL\r\n"); }

    uart_str("========================\r\n");
    uart_str("Temp(C) | IR raw  | IR filt | BPM\r\n");
    uart_str("--------+---------+---------+----\r\n");

    filter_init(&ir_filter);
    bpm_init(&bpm);

    for(;;)
    {
        int32_t  temp    = tmp117_read_celsius_x10();
        uint32_t ir_raw  = max30102_read_ir();
        uint32_t ir_filt = filter_update(&ir_filter, ir_raw);
        uint32_t bpm_val;

        bpm_update(&bpm, ir_filt);
        bpm_val = bpm_get(&bpm);

        uart_int(temp / 10);
        uart_char('.');
        uart_int(temp % 10);
        uart_str("       | ");
        uart_int((int32_t)ir_raw);
        uart_str("  | ");
        uart_int((int32_t)ir_filt);
        uart_str("  | ");
        if(bpm_val == BPM_INVALID) { uart_str("---"); }
        else                       { uart_int((int32_t)bpm_val); }
        uart_str("\r\n");

        iwdg_kick();
        vTaskDelay(pdMS_TO_TICKS(500U));
    }
}

int main(void)
{
    /* Enable FPU — required for ARM_CM4F port */
    *((volatile uint32_t*)0xE000ED88U) |= 0x00F00000U;

    iwdg_init();
    uart_init();
    i2c_init();

    uart_str("========================\r\n");
    uart_str("STM32 Vitals Monitor\r\n");
    uart_str("========================\r\n");

    xTaskCreate(task_vitals, "vitals", 512U, NULL, 2U, NULL);
    vTaskStartScheduler();

    for(;;) {}
}
