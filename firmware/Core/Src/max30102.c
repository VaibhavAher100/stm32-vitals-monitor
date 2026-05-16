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
#define REG_LED1     0x0CU  /* RED LED — active in HR mode (mode 0x02) */
#define REG_PARTID   0xFFU

static uint32_t read_fifo_3bytes(void)
{
    return i2c_read_3bytes(MAX30102_ADDR, REG_FIFO_DA) & 0x3FFFFU;
}

uint8_t max30102_init(void)
{
    uint8_t id = i2c_read_reg(MAX30102_ADDR, REG_PARTID);
    if (id != MAX30102_ID_EXPECTED) { return 0U; }

    if (i2c_write_reg(MAX30102_ADDR, REG_MODE,     0x40U) == 0U) { return 0U; } /* reset */
    vTaskDelay(pdMS_TO_TICKS(150U));                                              /* reset settle — scheduler running */
    /* 0x5F: SMP_AVE=010 (4 avg), FIFO_ROLLOVER_EN=1, FIFO_A_FULL=1111 */
    if (i2c_write_reg(MAX30102_ADDR, REG_FIFO_CF,  0x5FU) == 0U) { return 0U; }
    if (i2c_write_reg(MAX30102_ADDR, REG_MODE,     0x02U) == 0U) { return 0U; } /* HR mode — RED LED only */
    if (i2c_write_reg(MAX30102_ADDR, REG_SPO2,     0x27U) == 0U) { return 0U; } /* 100 sps, 411 us */
    if (i2c_write_reg(MAX30102_ADDR, REG_LED1,     0x3FU) == 0U) { return 0U; } /* RED LED 12.6 mA */
    /* Datasheet §6.3: flush FIFO by zeroing all three pointer registers */
    if (i2c_write_reg(MAX30102_ADDR, REG_FIFO_WR,  0x00U) == 0U) { return 0U; }
    if (i2c_write_reg(MAX30102_ADDR, REG_FIFO_OVF, 0x00U) == 0U) { return 0U; }
    if (i2c_write_reg(MAX30102_ADDR, REG_FIFO_RD,  0x00U) == 0U) { return 0U; }
    return 1U;
}

uint32_t max30102_read_red(void)
{
    return read_fifo_3bytes();
}

uint8_t max30102_drain_fifo(uint32_t *buf, uint8_t max_count)
{
    uint8_t wr    = i2c_read_reg(MAX30102_ADDR, REG_FIFO_WR);
    uint8_t rd    = i2c_read_reg(MAX30102_ADDR, REG_FIFO_RD);
    uint8_t avail = (wr >= rd) ? (uint8_t)(wr - rd) : (uint8_t)(32U - rd + wr);
    if (avail > max_count) { avail = max_count; }
    for (uint8_t i = 0U; i < avail; i++) {
        buf[i] = read_fifo_3bytes();
    }
    return avail;
}
