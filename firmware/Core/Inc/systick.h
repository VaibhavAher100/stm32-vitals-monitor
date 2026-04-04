#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void     systick_init(void);
void     delay_ms(uint32_t ms);
uint32_t get_tick(void);

#endif /* SYSTICK_H */
