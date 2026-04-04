# Session Log — STM32 Vitals Monitor Phase 2

**Date:** April 4, 2026
**Branch:** dev copy at `C:/Users/vaibh/Projects/dev/phase2-stm32-vitals-monitor`
**Cloned from:** `C:/Users/vaibh/Projects/dev/stm32-vitals-monitor` (Phase 1 complete)
**Original repo (untouched):** `C:/Users/vaibh/Projects/stm32-vitals-monitor`

---

## What Phase 1 Delivered (already in this clone)

- `docs/requirements.md` — 49 traceable requirements (IEC 62304 §5.2 style)
- `docs/testing.md` — 9 test cases, 49/49 coverage, all PASS
- `docs/misra.md` — MISRA C:2012 analysis: 38 fixed, 98 justified deviations
- Firmware fixes: Rules 10.4, 15.6, 17.7, 17.8, 5.9, 8.7 all resolved

---

## Full Roadmap

- Phase 1 — Code quality: REQUIREMENTS.md, TESTING.md, MISRA-C analysis — DONE
- Phase 2 — Hardware quality: SysTick delay, IWDG watchdog — DONE
- **Phase 3 — Signal processing: heart rate (BPM) from PPG** — NEXT
- Phase 4 — RTOS: FreeRTOS task separation
- Phase 5 — Power: STOP2 sleep mode

---

## Session Goal

Implement EXT-04 (SysTick calibrated delay) and EXT-05 (IWDG watchdog).

---

## EXT-04 — SysTick Calibrated Delay

**Goal:** Replace the three spin-loop delay functions (`main_delay`, `i2c_delay`,
`max30102_delay`) with a proper SysTick-based 1ms tick counter.

**Current state (in this clone):**
The three delay functions are spin loops with no time basis:
```c
/* main.c */
static void main_delay(volatile uint32_t n)
{
    volatile uint32_t count = n;
    while(count-- != 0U) {}
}
```
Same pattern duplicated in `i2c.c` and `max30102.c`.

**What needs to be done:**

1. **SysTick configuration** — configure SysTick in `systick_init()` to fire every 1ms.
   Clock is 4MHz (MSI default, set in `main.c`).
   - Reload value: `4000 - 1` (4MHz / 1000Hz − 1)
   - STK_LOAD = 3999U
   - STK_VAL  = 0U  (clear current value)
   - STK_CTRL bits: bit2 = processor clock, bit1 = enable exception, bit0 = enable counter

2. **Tick counter** — `volatile uint32_t systick_ms` incremented in `SysTick_Handler`.

3. **`delay_ms(uint32_t ms)`** — spins until `systick_ms` has advanced by `ms`.

4. **`get_tick()`** — returns current `systick_ms` value (useful for timeouts later).

5. **Replace all three delay functions** — remove `main_delay`, `i2c_delay`,
   `max30102_delay`. All call sites use `delay_ms()`.

**SysTick register addresses (STM32L4, RM0351 §4.4):**
```c
#define STK_CTRL  (*((volatile uint32_t*)0xE000E010))
#define STK_LOAD  (*((volatile uint32_t*)0xE000E014))
#define STK_VAL   (*((volatile uint32_t*)0xE000E018))
```

**SysTick_Handler** must be named exactly `SysTick_Handler` — CMSIS weak symbol
in the startup file.

**New files:**
- `firmware/Core/Inc/systick.h` — declares `systick_init`, `delay_ms`, `get_tick`
- `firmware/Core/Src/systick.c` — implements `SysTick_Handler` + the above

**Files to modify:**
- `main.c` — call `systick_init()` first in startup; replace `main_delay()` with `delay_ms()`; remove `main_delay` definition
- `i2c.c` — remove `i2c_delay`; `#include "systick.h"`; replace call sites with `delay_ms()`
- `max30102.c` — remove `max30102_delay`; `#include "systick.h"`; replace call sites with `delay_ms()`

---

## EXT-05 — IWDG Watchdog

**Goal:** Add Independent Watchdog so a firmware hang triggers an MCU reset.

**What needs to be done:**

1. **IWDG register addresses (STM32L4, RM0351 §38):**
   ```c
   #define IWDG_KR   (*((volatile uint32_t*)0x40003000))
   #define IWDG_PR   (*((volatile uint32_t*)0x40003004))
   #define IWDG_RLR  (*((volatile uint32_t*)0x40003008))
   #define IWDG_SR   (*((volatile uint32_t*)0x4000300C))
   ```

2. **Initialisation sequence:**
   ```c
   IWDG_KR  = 0x5555U;          /* unlock PR and RLR for writing */
   IWDG_PR  = 0x03U;            /* prescaler /32 → 1ms per tick at 32kHz LSI */
   IWDG_RLR = 0xFA0U;           /* reload = 4000 → timeout = 4000ms = 4s */
   while(IWDG_SR != 0U) {}      /* wait for registers to update */
   IWDG_KR  = 0xCCCCU;         /* start watchdog */
   ```

3. **Kick in main loop:**
   ```c
   IWDG_KR = 0xAAAAU;           /* reload counter — must happen within 4s */
   ```

4. **Timeout calculation:**
   - LSI clock: ~32kHz nominal
   - Prescaler /32: tick period = 32/32000 = 1ms
   - Reload 4000: timeout = 4000 × 1ms = 4s
   - Main loop period: ~500ms (two sensor reads + UART output)
   - Safety margin: 4s / 500ms = 8×

5. **New files:**
   - `firmware/Core/Inc/iwdg.h` — declares `iwdg_init`, `iwdg_kick`
   - `firmware/Core/Src/iwdg.c` — implements both

6. **Files to modify:**
   - `main.c` — call `iwdg_init()` after `systick_init()`; call `iwdg_kick()` inside `for(;;)`

---

## Docs to Update

- `docs/requirements.md` — add REQ-SYS-06 (SysTick 1ms tick) and REQ-SYS-07 (IWDG 4s timeout)
- `docs/testing.md` — add TC-10 (SysTick: UART timestamp advances 1ms per tick) and TC-11 (IWDG: board resets if main loop blocked)

---

## What Was Done This Session

**EXT-04 — SysTick calibrated delay: DONE**
- Created `firmware/Core/Inc/systick.h` — declares `systick_init`, `delay_ms`, `get_tick`
- Created `firmware/Core/Src/systick.c` — `SysTick_Handler` increments `volatile uint32_t systick_ms`; reload = 3999 (4 MHz / 1000 Hz − 1); `delay_ms` uses unsigned subtraction for rollover-safe wait
- Removed `main_delay`, `i2c_delay`, `max30102_delay` spin loops from all three files
- All 10 delay call sites replaced:
  - `main_delay(500000U)` → `delay_ms(500U)`
  - `i2c_delay(100U)` × 3 → `delay_ms(1U)`
  - `i2c_delay(10U)` × 4 → `delay_ms(1U)`
  - `max30102_delay(200000U)` → `delay_ms(150U)` (post-reset, ~150 ms at 4 MHz)
  - `max30102_delay(10U)` × 2 → `delay_ms(1U)`
- Added `#include "systick.h"` to `i2c.c` and `max30102.c`

**EXT-05 — IWDG watchdog: DONE**
- Created `firmware/Core/Inc/iwdg.h` — declares `iwdg_init`, `iwdg_kick`
- Created `firmware/Core/Src/iwdg.c` — prescaler /32, reload 4000 → 4 s timeout at 32 kHz LSI; 8× safety margin over ~500 ms loop period
- `main.c` calls `systick_init()` → `iwdg_init()` → `uart_init()` → `i2c_init()` at startup
- `iwdg_kick()` called at the top of the main loop before `delay_ms(500U)`

**Docs: DONE**
- `docs/requirements.md` — added REQ-SYS-06 (SysTick 1 ms tick) and REQ-SYS-07 (IWDG 4 s timeout); removed both from Out of Scope
- `docs/testing.md` — added TC-10 (SysTick timestamp) and TC-11 (IWDG forced hang); coverage updated to 51/51

**cppcheck / MISRA:** TODO — run in STM32CubeIDE, then commit locally

---

## Resume Checklist

When starting the next session (Phase 3 — BPM):
1. Read this SESSION_LOG.md
2. If cppcheck not yet run: run it on the Phase 2 files and resolve any new MISRA flags before proceeding
3. If not yet committed: commit Phase 2 locally
4. Begin Phase 3: peak detection on the filtered PPG waveform to compute heart rate (BPM)

---

*Log written: April 4, 2026*
*Dev path: C:/Users/vaibh/Projects/dev/phase2-stm32-vitals-monitor*
