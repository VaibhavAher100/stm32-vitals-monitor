#include "uart.h"

#define RCC_AHB2ENR  (*((volatile uint32_t*)0x4002104C))
#define RCC_APB1ENR1 (*((volatile uint32_t*)0x40021058))
#define GPIOA_MODER  (*((volatile uint32_t*)0x48000000))
#define GPIOA_AFRL   (*((volatile uint32_t*)0x48000020))
#define USART2_CR1   (*((volatile uint32_t*)0x40004400))
#define USART2_BRR   (*((volatile uint32_t*)0x4000440C))
#define USART2_ISR   (*((volatile uint32_t*)0x4000441C))
#define USART2_TDR   (*((volatile uint32_t*)0x40004428))

void uart_init(void)
{
    RCC_AHB2ENR  |= (1U << 0);
    RCC_APB1ENR1 |= (1U << 17);
    GPIOA_MODER  &= ~(3U << 4); GPIOA_MODER |= (2U << 4);
    GPIOA_AFRL   &= ~(0xFU << 8); GPIOA_AFRL |= (7U << 8);
    USART2_BRR    = 417U;
    USART2_CR1   |= (1U << 3);
    USART2_CR1   |= (1U << 0);
}

void uart_char(char c)
{
    while(!(USART2_ISR & (1U << 7))) {}
    USART2_TDR = (uint8_t)c;
}

void uart_str(const char *s)
{
    const char *p = s;                         /* Rule 17.8: local copy, do not modify parameter */
    while(*p != '\0') { uart_char(*p); p++; }
}

void uart_int(int32_t v)
{
    char buf[12]; uint8_t i = 0;
    int32_t val = v;                           /* Rule 17.8: local copy, do not modify parameter */
    if(val < 0) { uart_char('-'); val = -val; }
    if(val == 0) { uart_char('0'); return; }
    while(val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
    while(i > 0U) { uart_char(buf[--i]); }    /* Rule 10.4: 0U matches unsigned i */
}

/* Rule 8.7: uart_hex removed — unused function; declaration removed from uart.h */
