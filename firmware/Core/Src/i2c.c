/* Implements: REQ-I2C-01, REQ-I2C-02, REQ-I2C-03, REQ-I2C-04, REQ-I2C-05, REQ-I2C-06, REQ-I2C-07 */
#include "stm32l476xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "i2c.h"

/* Busy-wait used only during i2c_init() before the scheduler starts.
   At 4 MHz MSI: ~4000 cycles per ms. Used nowhere else. */
static void i2c_busy_delay_ms(uint32_t ms)
{
    volatile uint32_t count = ms;
    volatile uint32_t i;
    while (count != 0U) {
        for (i = 0U; i < 4000U; i++) {}
        count--;
    }
}

/* Clear all I2C error flags and return 0 — used on any bus error. */
static uint8_t i2c_clear_err(void)
{
    I2C1->ICR = 0x3F38U;
    return 0U;
}

/* Check for NACK/bus error/arbitration loss after a wait. Returns 1 if error. */
static uint8_t i2c_has_err(void)
{
    return ((I2C1->ISR & (I2C_ISR_NACKF | I2C_ISR_BERR | I2C_ISR_ARLO)) != 0U) ? 1U : 0U;
}

void i2c_init(void)
{
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
    i2c_busy_delay_ms(1U);

    GPIOB->MODER  &= ~((3U << 16U) | (3U << 18U));
    GPIOB->MODER  |=   (2U << 16U) | (2U << 18U);   /* PB8, PB9 alternate function */
    GPIOB->OTYPER |=  (1U << 8U) | (1U << 9U);       /* open-drain — required for I2C */
    GPIOB->OSPEEDR|=  (3U << 16U) | (3U << 18U);     /* very high speed */
    GPIOB->AFR[1] &= ~(0xFFU);
    GPIOB->AFR[1] |=  (4U) | (4U << 4U);             /* AF4 = I2C1 */

    I2C1->CR1     &= ~I2C_CR1_PE;
    i2c_busy_delay_ms(1U);
    I2C1->TIMINGR  = 0x00100D14U;                    /* 100 kHz at 4 MHz MSI — RM0351 §37.4.9 */
    I2C1->CR1     |= I2C_CR1_PE;
    i2c_busy_delay_ms(1U);
}

uint8_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    volatile uint32_t timeout;

    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return 0U; }

    /* Single write: set addr, nbytes, AUTOEND, and START atomically — RM0351 §37.4.5 */
    I2C1->CR2 = ((uint32_t)addr << 1U) | (2U << 16U) | I2C_CR2_AUTOEND | I2C_CR2_START;

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    I2C1->TXDR = reg;

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    I2C1->TXDR = val;

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_STOPF) && (timeout > 0U)) { timeout--; }
    vTaskDelay(pdMS_TO_TICKS(1U));
    return 1U;
}

uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
{
    volatile uint32_t timeout;
    uint8_t val;

    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return 0U; }

    /* Write phase: send register address */
    I2C1->CR2 = ((uint32_t)addr << 1U) | (1U << 16U) | I2C_CR2_START;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    I2C1->TXDR = reg;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TC) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    I2C1->CR2 |= I2C_CR2_STOP;
    vTaskDelay(pdMS_TO_TICKS(1U));

    /* Read phase: receive 1 byte */
    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return 0U; }
    I2C1->CR2 = ((uint32_t)addr << 1U) | I2C_CR2_RD_WRN | (1U << 16U) | I2C_CR2_AUTOEND | I2C_CR2_START;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    val = (uint8_t)I2C1->RXDR;
    vTaskDelay(pdMS_TO_TICKS(1U));
    return val;
}

uint16_t i2c_read_2bytes(uint8_t addr, uint8_t reg)
{
    volatile uint32_t timeout;
    uint16_t result = 0U;

    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return 0U; }

    /* Write phase: send register address */
    I2C1->CR2 = ((uint32_t)addr << 1U) | (1U << 16U) | I2C_CR2_START;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    I2C1->TXDR = reg;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TC) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    I2C1->CR2 |= I2C_CR2_STOP;
    vTaskDelay(pdMS_TO_TICKS(1U));

    /* Read phase: receive 2 bytes MSB-first */
    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return 0U; }
    I2C1->CR2 = ((uint32_t)addr << 1U) | I2C_CR2_RD_WRN | (2U << 16U) | I2C_CR2_AUTOEND | I2C_CR2_START;

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    result = (uint16_t)((uint16_t)(I2C1->RXDR & 0xFFU) << 8U);

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    result |= (uint16_t)I2C1->RXDR;
    vTaskDelay(pdMS_TO_TICKS(1U));
    return result;
}

uint32_t i2c_read_3bytes(uint8_t addr, uint8_t reg)
{
    volatile uint32_t timeout;
    uint32_t result = 0U;

    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return 0U; }

    /* Write phase: send register address */
    I2C1->CR2 = ((uint32_t)addr << 1U) | (1U << 16U) | I2C_CR2_START;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    I2C1->TXDR = reg;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TC) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    I2C1->CR2 |= I2C_CR2_STOP;
    vTaskDelay(pdMS_TO_TICKS(1U));

    /* Read phase: receive 3 bytes MSB-first */
    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return 0U; }
    I2C1->CR2 = ((uint32_t)addr << 1U) | I2C_CR2_RD_WRN | (3U << 16U) | I2C_CR2_AUTOEND | I2C_CR2_START;

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    result = ((uint32_t)I2C1->RXDR << 16U);

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    result |= ((uint32_t)I2C1->RXDR << 8U);

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && (timeout > 0U)) { timeout--; }
    if (timeout == 0U) { return i2c_clear_err(); }
    if (i2c_has_err()) { return i2c_clear_err(); }
    result |= (uint32_t)I2C1->RXDR;
    vTaskDelay(pdMS_TO_TICKS(1U));
    return result;
}
