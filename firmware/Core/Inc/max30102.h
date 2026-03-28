#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>

#define MAX30102_ADDR       0x57U
#define MAX30102_ID_EXPECTED 0x15U

uint8_t  max30102_init(void);      /* returns 1 if ID verified, 0 if not */
uint32_t max30102_read_ir(void);   /* returns raw 18-bit IR value */

#endif /* MAX30102_H */
