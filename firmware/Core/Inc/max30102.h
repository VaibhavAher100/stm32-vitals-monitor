#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>

#define MAX30102_ADDR       0x57U
#define MAX30102_ID_EXPECTED 0x15U

uint8_t  max30102_init(void);                                   /* returns 1 if ID verified, 0 if not */
uint32_t max30102_read_red(void);                               /* returns raw 18-bit RED PPG value (HR mode uses red LED only) */
uint8_t  max30102_drain_fifo(uint32_t *buf, uint8_t max_count); /* reads all available FIFO samples, returns count */

#endif /* MAX30102_H */
