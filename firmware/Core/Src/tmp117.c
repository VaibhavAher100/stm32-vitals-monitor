/* Implements: REQ-TMP-01, REQ-TMP-02, REQ-TMP-03, REQ-TMP-04, REQ-TMP-05 */
#include "tmp117.h"
#include "i2c.h"

uint8_t tmp117_init(void)
{
    uint16_t id = i2c_read_2bytes(TMP117_ADDR, TMP117_REG_ID);
    return (id == TMP117_ID_EXPECTED) ? 1U : 0U;
}

int32_t tmp117_read_celsius_x10(void)
{
    uint16_t raw = i2c_read_2bytes(TMP117_ADDR, TMP117_REG_TEMP);
    /* 0.0078125 C per LSB - multiply by 78 divide by 1000 gives x10 result */
    return ((int16_t)raw * 78) / 1000;
}
