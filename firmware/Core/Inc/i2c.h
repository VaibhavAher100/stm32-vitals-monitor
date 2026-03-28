#ifndef I2C_H
#define I2C_H

#include <stdint.h>

void     i2c_init(void);
uint8_t  i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
uint8_t  i2c_read_reg(uint8_t addr, uint8_t reg);
uint16_t i2c_read_2bytes(uint8_t addr, uint8_t reg);

#endif /* I2C_H */
