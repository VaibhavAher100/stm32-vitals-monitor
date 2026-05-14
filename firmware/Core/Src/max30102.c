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

/* Busy-wait used only in max30102_init() before the scheduler starts.
   At 4 MHz MSI: ~4000 cycles per ms. */
static void busy_delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    while (ms-- > 0U) {
        for (i = 0U; i < 4000U; i++) {}
    }
}

static uint32_t read_fifo_3bytes(void)
{
    return i2c_read_3bytes(MAX30102_ADDR, REG_FIFO_DA) & 0x3FFFFU;
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
