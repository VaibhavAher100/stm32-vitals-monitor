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
    while(ms-- > 0U) {
        for(i = 0U; i < 4000U; i++) {}
    }
}

static uint32_t read_fifo_3bytes(void)
{
    volatile uint32_t timeout;
    uint32_t result = 0U;

    /* Set FIFO data register pointer */
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_DA, 0x00U); /* Rule 17.7 */

    /* Manually read 3 bytes - reuse i2c internals via read_reg calls */
    /* Read MSB, mid, LSB from FIFO */
    #define I2C1_CR2  (*((volatile uint32_t*)0x40005404))
    #define I2C1_ISR  (*((volatile uint32_t*)0x40005418))
    #define I2C1_ICR  (*((volatile uint32_t*)0x4000541C))
    #define I2C1_TXDR (*((volatile uint32_t*)0x40005428))
    #define I2C1_RXDR (*((volatile uint32_t*)0x40005424))

    I2C1_ICR = 0x3F38U;
    timeout = 50000U;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}

    /* Write register pointer */
    I2C1_CR2 = ((uint32_t)MAX30102_ADDR << 1) | (1U << 16);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    I2C1_TXDR = REG_FIFO_DA;
    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 6)) && timeout--) {}
    I2C1_CR2 |= (1U << 14);
    vTaskDelay(pdMS_TO_TICKS(1U));

    /* Read 3 bytes */
    I2C1_ICR = 0x3F38U;
    timeout = 50000U;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)MAX30102_ADDR << 1) | (1U << 10) | (3U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);

    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result = ((uint32_t)I2C1_RXDR << 16);

    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= ((uint32_t)I2C1_RXDR << 8);

    timeout = 50000U;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= (uint32_t)I2C1_RXDR;

    result &= 0x3FFFFU;
    vTaskDelay(pdMS_TO_TICKS(1U));
    return result;
}

uint8_t max30102_init(void)
{
    uint8_t id = i2c_read_reg(MAX30102_ADDR, REG_PARTID);
    if(id != MAX30102_ID_EXPECTED) { return 0U; } /* Rule 15.6: braces on if body */

    (void)i2c_write_reg(MAX30102_ADDR, REG_MODE,    0x40U); /* reset        Rule 17.7 */
    busy_delay_ms(150U);
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_CF, 0x4FU); /* 4 avg, rollover on */
    (void)i2c_write_reg(MAX30102_ADDR, REG_MODE,    0x02U); /* HR mode */
    (void)i2c_write_reg(MAX30102_ADDR, REG_SPO2,    0x27U); /* 100sps, 411us */
    (void)i2c_write_reg(MAX30102_ADDR, REG_LED1,    0x1FU); /* 6.4mA IR LED */
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_WR, 0x00U);
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_RD, 0x00U);
    return 1U;
}

uint32_t max30102_read_ir(void)
{
    return read_fifo_3bytes();
}
