/* Implements: REQ-UART-01, REQ-UART-02, REQ-UART-03, REQ-UART-04, REQ-UART-05 */
#include "stm32l476xx.h"
#include "uart.h"

void uart_init(void)
{
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    GPIOA->MODER  &= ~(3U << 4U);
    GPIOA->MODER  |=  (2U << 4U);          /* PA2 alternate function */
    GPIOA->AFR[0] &= ~(0xFU << 8U);
    GPIOA->AFR[0] |=  (7U  << 8U);         /* AF7 = USART2_TX */

    USART2->BRR    = 417U;                  /* 9600 baud at 4 MHz MSI */
    USART2->CR1   |= USART_CR1_TE;
    USART2->CR1   |= USART_CR1_UE;
}

void uart_char(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE)) {}
    USART2->TDR = (uint8_t)c;
}

void uart_str(const char *s)
{
    const char *p = s;
    while (*p != '\0') { uart_char(*p); p++; }
}

void uart_int(int32_t v)
{
    char buf[12];
    uint8_t i = 0;
    int32_t val = v;
    if (val < 0) { uart_char('-'); val = -val; }
    if (val == 0) { uart_char('0'); return; }
    while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
    while (i > 0U) { uart_char(buf[--i]); }
}
