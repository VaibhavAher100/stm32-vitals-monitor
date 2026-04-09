#include "stm32l476xx.h"
#include "iwdg.h"

/* LSI ~32 kHz, prescaler /32 → 1 ms/tick, reload 4000 → 4 s timeout.
   task_sensor kicks every 500 ms — 8x safety margin. */
void iwdg_init(void)
{
    /* LSI must be running before IWDG_SR update flags can clear.
       Without this, while(IWDG->SR != 0) loops forever. */
    RCC->CSR |= RCC_CSR_LSION;
    while (!(RCC->CSR & RCC_CSR_LSIRDY)) {}

    IWDG->KR  = 0xCCCCU;   /* start watchdog — must come first so PVU/RVU flags can clear */
    IWDG->KR  = 0x5555U;   /* unlock PR and RLR for writing */
    IWDG->PR  = 0x03U;     /* prescaler /32 */
    IWDG->RLR = 0xFA0U;    /* reload = 4000 -> 4 s */
    while (IWDG->SR != 0U) {}
    IWDG->KR  = 0xAAAAU;   /* reload counter with new values */
}

void iwdg_kick(void)
{
    IWDG->KR = 0xAAAAU;   /* reload counter */
}
