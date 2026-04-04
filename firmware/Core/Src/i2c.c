#include "i2c.h"

#define RCC_AHB2ENR   (*((volatile uint32_t*)0x4002104C))
#define RCC_APB1ENR1  (*((volatile uint32_t*)0x40021058))
#define GPIOB_MODER   (*((volatile uint32_t*)0x48000400))
#define GPIOB_OTYPER  (*((volatile uint32_t*)0x48000404))
#define GPIOB_OSPEEDR (*((volatile uint32_t*)0x48000408))
#define GPIOB_AFRH    (*((volatile uint32_t*)0x48000424))
#define I2C1_CR1      (*((volatile uint32_t*)0x40005400))
#define I2C1_CR2      (*((volatile uint32_t*)0x40005404))
#define I2C1_TIMINGR  (*((volatile uint32_t*)0x40005410))
#define I2C1_ISR      (*((volatile uint32_t*)0x40005418))
#define I2C1_ICR      (*((volatile uint32_t*)0x4000541C))
#define I2C1_TXDR     (*((volatile uint32_t*)0x40005428))
#define I2C1_RXDR     (*((volatile uint32_t*)0x40005424))

/* Rule 5.9: unique name; Rule 17.8: local copy avoids modifying parameter */
static void i2c_delay(volatile uint32_t n)
{
    volatile uint32_t count = n;
    while(count-- != 0U) {}
}

void i2c_init(void)
{
    RCC_AHB2ENR  |= (1U << 1);
    RCC_APB1ENR1 |= (1U << 21);
    i2c_delay(100U);
    GPIOB_MODER  &= ~(3U << 16); GPIOB_MODER |= (2U << 16);
    GPIOB_MODER  &= ~(3U << 18); GPIOB_MODER |= (2U << 18);
    GPIOB_OTYPER  |= (1U << 8) | (1U << 9);
    GPIOB_OSPEEDR |= (3U << 16) | (3U << 18);
    GPIOB_AFRH   &= ~(0xFFU);
    GPIOB_AFRH   |= (4U) | (4U << 4);
    I2C1_CR1     &= ~(1U << 0);
    i2c_delay(100U);
    I2C1_TIMINGR  = 0x00100D14U;
    I2C1_CR1     |= (1U << 0);
    i2c_delay(100U);
}

uint8_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    volatile uint32_t timeout;
    I2C1_ICR = 0x3F38U;
    timeout = 50000U;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (2U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    if(!timeout) { return 0U; }                    /* Rule 15.6: braces on if body */
    I2C1_TXDR = reg;
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    if(!timeout) { return 0U; }                    /* Rule 15.6 */
    I2C1_TXDR = val;
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 5)) && timeout--) {}
    i2c_delay(10U);
    return 1U;
}

uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
{
    volatile uint32_t timeout;
    uint8_t val;
    I2C1_ICR = 0x3F38U;
    timeout = 50000U;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 16);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    I2C1_TXDR = reg;
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 6)) && timeout--) {}
    I2C1_CR2 |= (1U << 14);
    i2c_delay(10U);
    I2C1_ICR = 0x3F38U;
    timeout = 50000U;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 10) | (1U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    val = (uint8_t)I2C1_RXDR;
    i2c_delay(10U);
    return val;
}

uint16_t i2c_read_2bytes(uint8_t addr, uint8_t reg)
{
    volatile uint32_t timeout;
    uint16_t result = 0U;
    I2C1_ICR = 0x3F38U;
    timeout = 50000U;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 16);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    I2C1_TXDR = reg;
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 6)) && timeout--) {}
    I2C1_CR2 |= (1U << 14);
    i2c_delay(10U);
    I2C1_ICR = 0x3F38U;
    timeout = 50000U;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 10) | (2U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result = (uint16_t)(I2C1_RXDR << 8);
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= (uint16_t)I2C1_RXDR;
    i2c_delay(10U);
    return result;
}
