#include <stdint.h>
#include "iwdg.h"

#define RCC_CSR   (*((volatile uint32_t*)0x40021094U))
#define IWDG_KR   (*((volatile uint32_t*)0x40003000U))
#define IWDG_PR   (*((volatile uint32_t*)0x40003004U))
#define IWDG_RLR  (*((volatile uint32_t*)0x40003008U))
#define IWDG_SR   (*((volatile uint32_t*)0x4000300CU))

/* Timeout: LSI ~32 kHz, prescaler /32 → 1 ms/tick, reload 4000 → 4 s timeout.
   Main loop period ~500 ms gives 8× safety margin. */
void iwdg_init(void)
{
    /* LSI must be running before IWDG_SR update flags can clear.
       Without this, while(IWDG_SR != 0) loops forever. */
    RCC_CSR  |= (1U << 0);             /* LSION = 1 */
    while(!(RCC_CSR & (1U << 1))) {}   /* wait for LSIRDY */

    IWDG_KR  = 0xCCCCU;   /* start watchdog — must come first so PVU/RVU flags can clear */
    IWDG_KR  = 0x5555U;   /* unlock PR and RLR for writing */
    IWDG_PR  = 0x03U;     /* prescaler /32 → 1 ms per tick at 32 kHz LSI */
    IWDG_RLR = 0xFA0U;    /* reload = 4000 → 4000 ms timeout */
    while(IWDG_SR != 0U) {}
    IWDG_KR  = 0xAAAAU;   /* reload counter with new values */
}

void iwdg_kick(void)
{
    IWDG_KR = 0xAAAAU;     /* reload counter — must happen within 4 s */
}
