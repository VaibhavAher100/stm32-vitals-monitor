/**
 * STM32 Vitals Monitor - Application Layer
 * Bare-metal, no HAL. All hardware access via driver layer.
 *
 * Architecture:
 *   main.c          - application loop (this file)
 *   filter.c/h      - processing layer
 *   uart/i2c/tmp117/max30102 - driver layer
 */

#include "systick.h"
#include "iwdg.h"
#include "uart.h"
#include "i2c.h"
#include "tmp117.h"
#include "max30102.h"
#include "filter.h"

int main(void)
{
    systick_init();
    iwdg_init();
    uart_init();
    i2c_init();

    uart_str("========================\r\n");
    uart_str("STM32 Vitals Monitor\r\n");
    uart_str("========================\r\n");

    /* Initialise and verify sensors */
    if(tmp117_init()) {                         /* Rule 15.6: braces on all branches */
        uart_str("TMP117  OK\r\n");
    } else {
        uart_str("TMP117  FAIL\r\n");
    }

    if(max30102_init()) {
        uart_str("MAX30102 OK\r\n");
    } else {
        uart_str("MAX30102 FAIL\r\n");
    }

    uart_str("========================\r\n");
    uart_str("Temp(C) | IR raw  | IR filt\r\n");
    uart_str("--------+---------+--------\r\n");

    Filter ir_filter;
    filter_init(&ir_filter);

    for(;;)
    {
        /* Processing layer - read and filter */
        int32_t  temp     = tmp117_read_celsius_x10();
        uint32_t ir_raw   = max30102_read_ir();
        uint32_t ir_filt  = filter_update(&ir_filter, ir_raw);

        /* Application layer - format and print */
        uart_int(temp / 10);
        uart_char('.');
        uart_int(temp % 10);
        uart_str("       | ");
        uart_int((int32_t)ir_raw);
        uart_str("  | ");
        uart_int((int32_t)ir_filt);
        uart_str("\r\n");

        iwdg_kick();
        delay_ms(500U);
    }
}
