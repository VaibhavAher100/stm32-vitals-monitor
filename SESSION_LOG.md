# Session Log — STM32 Vitals Monitor Phase 3

**Date:** April 4, 2026
**Branch:** dev copy at `C:/Users/vaibh/Projects/dev/phase3-stm32-vitals-monitor`
**Cloned from:** `C:/Users/vaibh/Projects/dev/phase2-stm32-vitals-monitor` (Phase 2 complete)
**Original repo (untouched):** `C:/Users/vaibh/Projects/stm32-vitals-monitor`

---

## What Phase 2 Delivered (already in this clone)

- `firmware/Core/Inc/systick.h` + `Src/systick.c` — SysTick 1 ms tick counter, `delay_ms()`, `get_tick()`
- `firmware/Core/Inc/iwdg.h` + `Src/iwdg.c` — IWDG watchdog, 4 s timeout, 8× safety margin
- Spin-loop delays removed from `main.c`, `i2c.c`, `max30102.c`; all replaced with `delay_ms()`
- `docs/requirements.md` — REQ-SYS-06 (SysTick) and REQ-SYS-07 (IWDG) added; coverage 51/51
- `docs/testing.md` — TC-10 and TC-11 added; all tests PASS

---

## Full Roadmap

- Phase 1 — Code quality: REQUIREMENTS.md, TESTING.md, MISRA-C analysis — DONE
- Phase 2 — Hardware quality: SysTick delay, IWDG watchdog — DONE
- **Phase 3 — Signal processing: heart rate (BPM) from PPG** — NEXT
- Phase 4 — RTOS: FreeRTOS task separation
- Phase 5 — Power: STOP2 sleep mode

---

## Session Goal

Implement EXT-06 (BPM detection from filtered PPG/IR signal).

---

## EXT-06 — BPM Detection from PPG

**Goal:** Compute heart rate in BPM from the filtered IR signal already produced by `filter.c`.

**Algorithm: dynamic-threshold rising-edge detector (integer-only, no floats)**

1. **Rolling history window** — keep the last `BPM_HISTORY = 8` filtered IR samples (~4 s at 500 ms/sample).
2. **Dynamic threshold** — midpoint of rolling min/max: `min + (max - min) / 2`.
   Adapts to sensor proximity and ambient light drift without calibration.
3. **Rising-edge crossing** — detected when `prev_val < threshold` and `cur >= threshold`.
4. **Refractory period** — crossings within `BPM_REFRACTORY_MS = 333 ms` of the previous
   crossing are ignored (enforces 180 BPM maximum, rejects noise).
5. **BPM calculation** — `60000 / interval_ms` where `interval_ms` is the time between
   the two most recent valid crossings (uses `get_tick()` timestamps; unsigned subtraction handles rollover).
6. **Output** — display `---` until at least two valid crossings have been recorded.

**New files:**
- `firmware/Core/Inc/bpm.h` — `BpmDetector` struct + `BPM_HISTORY`, `BPM_REFRACTORY_MS`, `BPM_INVALID` constants;
  declares `bpm_init`, `bpm_update`, `bpm_get`
- `firmware/Core/Src/bpm.c` — full implementation; `compute_threshold()` static helper

**Files to modify:**
- `main.c` — `#include "bpm.h"`; declare `BpmDetector bpm`; call `bpm_init(&bpm)` at startup;
  call `bpm_update` + `bpm_get` in loop; add BPM column to UART header and output rows

**Docs to update:**
- `docs/requirements.md` — add REQ-SYS-08 (BPM detection); update REQ-OUT-01 (4 fields now);
  move "Clinical heart rate" out of Out of Scope
- `docs/testing.md` — add TC-12 (BPM at rest 50–110 BPM); update coverage to 52/52

---

## What Was Done This Session

*(fill in after implementation)*

---

## Resume Checklist

When starting the next session (Phase 4 — FreeRTOS):
1. Read this SESSION_LOG.md
2. Verify Phase 3 committed locally (`git log --oneline -3`)
3. Clone `phase3-stm32-vitals-monitor` → `phase4-stm32-vitals-monitor`, remove origin
4. Begin Phase 4: FreeRTOS task separation

---

*Log written: April 4, 2026*
*Dev path: C:/Users/vaibh/Projects/dev/phase3-stm32-vitals-monitor*
