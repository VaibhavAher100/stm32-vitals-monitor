#include "stm32l476xx.h"
#include "max30102.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"

#define REG_FIFO_WR  0x04U
#define REG_FIFO_RD  0x06U
#define REG_FIFO_DA  0x07U
#define REG_FIFO_CF  0x08U
#define REG_MODE     0x09U
#define REG_SPO2     0x0AU
#define REG_LED1     0x0CU
#define REG_PARTID   0xFFU

/* Busy-wait used only in max30102_init() before the scheduler starts.
   At 4 MHz MSI: ~4000 cycles per ms. */
static void busy_delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    while (ms-- > 0U) {
        for (i = 0U; i < 4000U; i++) {}
    }
}

/* Read 3-byte FIFO entry directly over I2C — MAX30102 packs 18-bit IR
   sample MSB-first across 3 bytes. Lower 18 bits are the sample. */
static uint32_t read_fifo_3bytes(void)
{
    volatile uint32_t timeout;
    uint32_t result = 0U;

    /* Write phase: set FIFO_DATA register pointer */
    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && timeout--) {}
    I2C1->CR2 = ((uint32_t)MAX30102_ADDR << 1U) | (1U << 16U);
    I2C1->CR2 |= I2C_CR2_START;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TXIS) && timeout--) {}
    I2C1->TXDR = REG_FIFO_DA;
    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_TC) && timeout--) {}
    I2C1->CR2 |= I2C_CR2_STOP;
    vTaskDelay(pdMS_TO_TICKS(1U));

    /* Read phase: receive 3 bytes */
    I2C1->ICR = 0x3F38U;
    timeout = 50000U;
    while ((I2C1->ISR & I2C_ISR_BUSY) && timeout--) {}
    I2C1->CR2 = ((uint32_t)MAX30102_ADDR << 1U) | I2C_CR2_RD_WRN | (3U << 16U) | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && timeout--) {}
    result = ((uint32_t)I2C1->RXDR << 16U);

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && timeout--) {}
    result |= ((uint32_t)I2C1->RXDR << 8U);

    timeout = 50000U;
    while (!(I2C1->ISR & I2C_ISR_RXNE) && timeout--) {}
    result |= (uint32_t)I2C1->RXDR;

    result &= 0x3FFFFU;   /* 18-bit sample mask */
    vTaskDelay(pdMS_TO_TICKS(1U));
    return result;
}

uint8_t max30102_init(void)
{
    uint8_t id = i2c_read_reg(MAX30102_ADDR, REG_PARTID);
    if (id != MAX30102_ID_EXPECTED) { return 0U; }

    (void)i2c_write_reg(MAX30102_ADDR, REG_MODE,    0x40U); /* reset */
    busy_delay_ms(150U);
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_CF, 0x4FU); /* 4 avg, rollover on */
    (void)i2c_write_reg(MAX30102_ADDR, REG_MODE,    0x02U); /* HR mode */
    (void)i2c_write_reg(MAX30102_ADDR, REG_SPO2,    0x27U); /* 100 sps, 411 us */
    (void)i2c_write_reg(MAX30102_ADDR, REG_LED1,    0x1FU); /* 6.4 mA IR LED */
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_WR, 0x00U);
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_RD, 0x00U);
    return 1U;
}

uint32_t max30102_read_ir(void)
{
    return read_fifo_3bytes();
}
