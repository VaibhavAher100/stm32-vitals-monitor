#include "systick.h"

#define STK_CTRL  (*((volatile uint32_t*)0xE000E010U))
#define STK_LOAD  (*((volatile uint32_t*)0xE000E014U))
#define STK_VAL   (*((volatile uint32_t*)0xE000E018U))

/* 4 MHz system clock / 1000 Hz tick rate - 1 = 3999 */
#define SYSTICK_RELOAD 3999U

static volatile uint32_t systick_ms = 0U;

void systick_init(void)
{
    STK_LOAD = SYSTICK_RELOAD;
    STK_VAL  = 0U;
    /* bit2: use processor clock; bit1: enable SysTick exception; bit0: enable counter */
    STK_CTRL = 0x00000007U;
}

/* Rule 8.4 deviation: SysTick_Handler overrides the CMSIS weak symbol defined in the
   startup file. No explicit prototype exists in a header; the weak declaration in
   startup_stm32l476rgtx.s acts as the forward declaration at link time. */
void SysTick_Handler(void)
{
    systick_ms++;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while((systick_ms - start) < ms) {}
}

uint32_t get_tick(void)
{
    return systick_ms;
}
