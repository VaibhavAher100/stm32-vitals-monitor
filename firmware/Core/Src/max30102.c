#include "max30102.h"
#include "i2c.h"

#define REG_FIFO_WR  0x04U
#define REG_FIFO_RD  0x06U
#define REG_FIFO_DA  0x07U
#define REG_FIFO_CF  0x08U
#define REG_MODE     0x09U
#define REG_SPO2     0x0AU
#define REG_LED1     0x0CU
#define REG_PARTID   0xFFU

static void delay(volatile uint32_t n) { while(n--) {} }

static uint32_t read_fifo_3bytes(void)
{
    volatile uint32_t timeout;
    uint32_t result = 0;

    /* Set FIFO data register pointer */
    i2c_write_reg(MAX30102_ADDR, REG_FIFO_DA, 0x00U);

    /* Manually read 3 bytes - reuse i2c internals via read_reg calls */
    /* Read MSB, mid, LSB from FIFO */
    #define I2C1_CR2  (*((volatile uint32_t*)0x40005404))
    #define I2C1_ISR  (*((volatile uint32_t*)0x40005418))
    #define I2C1_ICR  (*((volatile uint32_t*)0x4000541C))
    #define I2C1_TXDR (*((volatile uint32_t*)0x40005428))
    #define I2C1_RXDR (*((volatile uint32_t*)0x40005424))

    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}

    /* Write register pointer */
    I2C1_CR2 = ((uint32_t)MAX30102_ADDR << 1) | (1U << 16);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    I2C1_TXDR = REG_FIFO_DA;
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 6)) && timeout--) {}
    I2C1_CR2 |= (1U << 14);
    delay(10);

    /* Read 3 bytes */
    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)MAX30102_ADDR << 1) | (1U << 10) | (3U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);

    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result = ((uint32_t)I2C1_RXDR << 16);

    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= ((uint32_t)I2C1_RXDR << 8);

    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= (uint32_t)I2C1_RXDR;

    result &= 0x3FFFFU;
    delay(10);
    return result;
}

uint8_t max30102_init(void)
{
    uint8_t id = i2c_read_reg(MAX30102_ADDR, REG_PARTID);
    if(id != MAX30102_ID_EXPECTED) return 0U;

    i2c_write_reg(MAX30102_ADDR, REG_MODE,    0x40U); /* reset */
    delay(200000);
    i2c_write_reg(MAX30102_ADDR, REG_FIFO_CF, 0x4FU); /* 4 avg, rollover on */
    i2c_write_reg(MAX30102_ADDR, REG_MODE,    0x02U); /* HR mode */
    i2c_write_reg(MAX30102_ADDR, REG_SPO2,    0x27U); /* 100sps, 411us */
    i2c_write_reg(MAX30102_ADDR, REG_LED1,    0x1FU); /* 6.4mA IR LED */
    i2c_write_reg(MAX30102_ADDR, REG_FIFO_WR, 0x00U);
    i2c_write_reg(MAX30102_ADDR, REG_FIFO_RD, 0x00U);
    return 1U;
}

uint32_t max30102_read_ir(void)
{
    return read_fifo_3bytes();
}
