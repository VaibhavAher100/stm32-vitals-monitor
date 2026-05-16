/* Implements: REQ-TMP-01, REQ-TMP-02, REQ-TMP-03, REQ-TMP-04, REQ-TMP-05 */
#include "tmp117.h"
#include "i2c.h"
#include <stdint.h>

uint8_t tmp117_init(void)
{
    uint16_t id = i2c_read_2bytes(TMP117_ADDR, TMP117_REG_ID);
    return (id == TMP117_ID_EXPECTED) ? 1U : 0U;
}

int32_t tmp117_read_celsius_x10(void)
{
    uint16_t raw = i2c_read_2bytes(TMP117_ADDR, TMP117_REG_TEMP);
    /* raw == 0 on I2C error — indistinguishable from 0°C, so validate with a second read */
    if (raw == 0U) {
        raw = i2c_read_2bytes(TMP117_ADDR, TMP117_REG_TEMP);
        if (raw == 0U) { return INT32_MIN; }
    }
    /* 7.8125 m°C per LSB exactly: raw * 625 / 8000 = raw * 0.078125 gives °C×10 */
    return ((int32_t)(int16_t)raw * 625) / 8000;
}
