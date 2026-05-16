/* Implements: REQ-MAX-01, REQ-MAX-02, REQ-MAX-03, REQ-MAX-04, REQ-MAX-05,
               REQ-MAX-06, REQ-MAX-07, REQ-MAX-08, REQ-MAX-09, REQ-MAX-10 */
#include "max30102.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "task.h"

#define REG_FIFO_WR  0x04U
#define REG_FIFO_OVF 0x05U  /* overflow counter — must be cleared during FIFO flush */
#define REG_FIFO_RD  0x06U
#define REG_FIFO_DA  0x07U
#define REG_FIFO_CF  0x08U
#define REG_MODE     0x09U
#define REG_SPO2     0x0AU
#define REG_LED1     0x0CU
#define REG_PARTID   0xFFU

static uint32_t read_fifo_3bytes(void)
{
    return i2c_read_3bytes(MAX30102_ADDR, REG_FIFO_DA) & 0x3FFFFU;
}

uint8_t max30102_init(void)
{
    uint8_t id = i2c_read_reg(MAX30102_ADDR, REG_PARTID);
    if (id != MAX30102_ID_EXPECTED) { return 0U; }

    (void)i2c_write_reg(MAX30102_ADDR, REG_MODE,     0x40U); /* reset */
    vTaskDelay(pdMS_TO_TICKS(150U));                          /* reset settle — scheduler running */
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_CF,  0x4FU); /* 4 avg, rollover on */
    (void)i2c_write_reg(MAX30102_ADDR, REG_MODE,     0x02U); /* HR mode */
    (void)i2c_write_reg(MAX30102_ADDR, REG_SPO2,     0x27U); /* 100 sps, 411 us */
    (void)i2c_write_reg(MAX30102_ADDR, REG_LED1,     0x1FU); /* 6.4 mA IR LED */
    /* Datasheet §6.3: flush FIFO by zeroing all three pointer registers */
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_WR,  0x00U);
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_OVF, 0x00U);
    (void)i2c_write_reg(MAX30102_ADDR, REG_FIFO_RD,  0x00U);
    return 1U;
}

uint32_t max30102_read_ir(void)
{
    return read_fifo_3bytes();
}
