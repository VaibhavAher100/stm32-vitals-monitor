#ifndef TMP117_H
#define TMP117_H

#include <stdint.h>

#define TMP117_ADDR     0x49U
#define TMP117_REG_TEMP 0x00U
#define TMP117_REG_ID   0x0FU
#define TMP117_ID_EXPECTED 0x0117U

uint8_t tmp117_init(void);   /* returns 1 if ID verified, 0 if not */
int32_t tmp117_read_celsius_x10(void); /* returns temp * 10, e.g. 229 = 22.9 C */

#endif /* TMP117_H */
