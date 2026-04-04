# Session Log — STM32 Vitals Monitor Dev Branch

**Date:** April 4, 2026
**Branch:** dev copy at `C:/Users/vaibh/Projects/dev/stm32-vitals-monitor`
**Original repo (untouched):** `C:/Users/vaibh/Projects/stm32-vitals-monitor`

---

## Session Goal

Work through the gap and extension roadmap for the STM32 Vitals Monitor
portfolio project. Phase 1 targets code quality documentation (EXT-01 to EXT-03)
before touching firmware functionality.

Full roadmap:
- Phase 1 — Code quality: REQUIREMENTS.md, TESTING.md, MISRA-C analysis
- Phase 2 — Hardware quality: SysTick delay, IWDG watchdog
- Phase 3 — Signal processing: heart rate (BPM) from PPG
- Phase 4 — RTOS: FreeRTOS task separation
- Phase 5 — Power: STOP2 sleep mode

---

## What Was Done This Session

---

### Step 0 — Setup: cloned to isolated dev folder

**Why:** Preserve the original repo unchanged. All work goes into the dev copy.
If a change is wrong, the original is unaffected.

```
C:/Users/vaibh/Projects/dev/stm32-vitals-monitor   ← working copy (this session)
C:/Users/vaibh/Projects/stm32-vitals-monitor        ← original (untouched)
```

Command run:
```bash
mkdir C:/Users/vaibh/Projects/dev
git clone https://github.com/VaibhavAher100/stm32-vitals-monitor \
          C:/Users/vaibh/Projects/dev/stm32-vitals-monitor
```

---

### EXT-01 — docs/requirements.md (COMPLETE)

**File created:** `docs/requirements.md`

**What it is:** A formal Software Requirements Specification (SRS). Every
requirement in the document is derived from the actual firmware source code —
nothing was invented. The goal was to make implicit requirements explicit and
give each one a traceable ID.

**Structure:**
- Section 2: Functional requirements (40 requirements)
  - REQ-SYS-01 to 05: System startup sequence
  - REQ-UART-01 to 05: UART configuration and transmission
  - REQ-I2C-01 to 07: I2C bus configuration and transactions
  - REQ-TMP-01 to 05: TMP117 sensor initialisation and temperature reading
  - REQ-MAX-01 to 10: MAX30102 sensor initialisation and IR FIFO read
  - REQ-FIL-01 to 06: Moving average filter behaviour
  - REQ-OUT-01 to 03: Measurement output format
- Section 3: Non-functional requirements (9 requirements)
  - REQ-NF-01 to 03: Memory safety (no malloc, no recursion, no VLAs)
  - REQ-NF-04 to 05: Hardware access (volatile, no HAL)
  - REQ-NF-06 to 08: Architecture layer rules
  - REQ-NF-09: Clock configuration
- Section 4: Constraints (hardware, toolchain, deployment)
- Section 5: Out of scope (items not implemented + [FUTURE] items)

**Key requirement examples grounded in source:**
- REQ-MAX-09 ← `result &= 0x3FFFFU` in max30102.c:65
- REQ-FIL-05 ← `f->sum / f->count` in filter.c:19
- REQ-NF-07 ← zero register addresses in main.c (verified by TC-07)

**Why it matters:**
- EXT-02 (TESTING.md) references these IDs — test cases prove requirements
- EXT-03 (MISRA deviations) reference these IDs — e.g. "deviation justified by REQ-NF-05"
- Mirrors IEC 62304 §5.2 software requirements specification
- Turns "I built a thing" into "here are 49 verifiable claims about what it does"

---

### EXT-02 — docs/testing.md (COMPLETE)

**File created:** `docs/testing.md`

**What it is:** A structured test record. One test case per verifiable area.
Each TC-ID maps to requirement IDs and cites actual evidence.

**9 test cases:**

| TC | Area | Method | Evidence |
|---|---|---|---|
| TC-01 | UART init and transmission | Hardware | `results/uart_verified_coolterm_com7_9600.png` |
| TC-02 | I2C bus and address detection | Hardware | `results/i2c_scanner_both_sensors.png` |
| TC-03 | TMP117 init and temperature | Hardware | `results/tmp117_temperature_verified.png` |
| TC-04 | MAX30102 init and IR FIFO | Hardware | `results/max30102_ir_raw_verified.png` |
| TC-05 | Both sensors combined + output format | Hardware | `results/both_sensors_combined_output.png` |
| TC-06 | Moving average filter behaviour | Hardware | `results/refactored_3layer_output.png` |
| TC-07 | 3-layer architecture - no cross-layer access | Static inspection | grep commands documented |
| TC-08 | No dynamic allocation, no recursion | Static inspection | grep commands documented |
| TC-09 | Volatile on all register definitions | Static inspection | grep commands documented |

**Coverage matrix:** 49/49 requirements covered. All PASS.

**Why it matters:**
- 6 hardware tests are backed by existing screenshots — evidence already exists
- 3 static inspection tests include exact grep commands so anyone can reproduce
- This is the difference between "it works" and "here is the proof it works"

---

### EXT-03 — docs/misra.md + code fixes (IN PROGRESS)

#### Part A — MISRA analysis run (COMPLETE)

**Tool:** Cppcheck 2.20.0 with misra.py addon
(addon downloaded from github.com/danmar/cppcheck during session)

**Command:**
```bash
cppcheck --addon=misra.py --enable=style,warning,performance,portability
         --std=c99 --platform=arm32-wchar_t2
         --suppress=missingInclude --suppress=missingIncludeSystem
         -I firmware/Core/Inc
         firmware/Core/Src/main.c firmware/Core/Src/filter.c
         firmware/Core/Src/uart.c firmware/Core/Src/i2c.c
         firmware/Core/Src/tmp117.c firmware/Core/Src/max30102.c
```

**Results — 136 violations across 12 rules:**

| Rule | Count | Description |
|---|---|---|
| 12.2 | 43 | Shift operands in register bit operations |
| 11.4 | 26 | Pointer-to-integer cast for register macros |
| 13.5 | 22 | Side effect (`timeout--`) in `&&` polling loops |
| 15.6 | 11 | Single-statement bodies without braces |
| 17.7 | 8 | Return value of `i2c_write_reg()` discarded |
| 17.8 | 6 | Function parameter modified (`delay(n){ while(n--) }`) |
| 14.4 | 6 | Non-Boolean condition (`while(n--)`, `if(init())`) |
| 10.4 | 5 | Type mismatch `uint8_t` vs int constant |
| 15.5 | 4 | Early return on timeout (multiple exit points) |
| 5.9  | 3 | Duplicate `delay` identifier across 3 files |
| 8.7  | 1 | `uart_hex` not used outside its translation unit |
| 13.3 | 1 | Compound expression in `uart_int` |

#### Part B — docs/misra.md written (COMPLETE)

**File created:** `docs/misra.md`

**Disposition of all 136 findings:**
- **102 justified deviations** (Rules 11.4, 12.2, 13.3, 13.5, 14.4, 15.5) — documented with rationale
- **34 fixed** (Rules 10.4, 15.6, 17.7, 17.8, 5.9, 8.7) — code changes applied

Key justified deviations:
- Rule 11.4 (pointer casts): inherent to bare-metal register access — no HAL means no alternative. Justified by REQ-NF-05.
- Rule 12.2 (shift operations): cppcheck over-fires; all shifts are `(1U << N)` with N < 32, bounded and safe.
- Rule 13.5 (`timeout--` in `&&`): standard embedded polling-with-timeout pattern. Splitting reduces clarity.

#### Part C — Code fixes applied (COMPLETE, verification pending)

Five fix categories applied across 5 source files:

**Rule 15.6 — Added braces to all single-statement bodies**
- `main.c`: `if(tmp117_init())` / `else` / `if(max30102_init())` / `else`
- `uart.c`: `while(*s)`, `while(i > 0)`
- `filter.c`: `for(...)` body, `if(f->count < ...)` body
- `i2c.c`: `if(!timeout)` early returns (×2)
- `max30102.c`: `if(id != MAX30102_ID_EXPECTED)`

**Rule 17.7 — (void) cast on discarded return values**
- `max30102.c`: all 8 calls to `i2c_write_reg()` in `read_fifo_3bytes()` and `max30102_init()`

**Rule 17.8 — Local copy of function parameter before modification**
- `main.c`: `main_delay(n)` → `volatile uint32_t count = n; while(count-- != 0U) {}`
- `i2c.c`: `i2c_delay(n)` → same pattern
- `max30102.c`: `max30102_delay(n)` → same pattern

**Rule 5.9 — Unique identifiers for static functions**
- `main.c`: `delay` renamed to `main_delay`
- `i2c.c`: `delay` renamed to `i2c_delay`
- `max30102.c`: `delay` renamed to `max30102_delay`

**Rule 8.7 — External linkage for single-TU functions**
- `uart.c`: `uart_hex` made `static`

**Rule 10.4 — Type consistency in comparisons**
- `filter.c`: `(uint8_t)FILTER_WINDOW` casts added to comparisons and modulo

#### Part D — Verification re-run (COMPLETE)

Post-fix cppcheck run confirmed. Additional uart.c violations found and fixed
before committing (Rule 17.8 in uart_str/uart_int, Rule 10.4 in uart_int,
Rule 8.7 — uart_hex removed entirely as dead code).

**Final result: 98 violations, all in justified-deviation categories.**

| Rule | Remaining | Status |
|---|---|---|
| 12.2 | 43 | Justified deviation |
| 11.4 | 26 | Justified deviation |
| 13.5 | 22 | Justified deviation |
| 15.5 | 4 | Justified deviation |
| 14.4 | 2 | Justified deviation |
| 13.3 | 1 | Justified deviation |
| 10.4, 15.6, 17.7, 17.8, 5.9, 8.7 | 0 | Fixed |

misra.md updated to reflect actual numbers (38 fixed, 98 remaining).

---

## Files Created / Modified This Session

| File | Status | Notes |
|---|---|---|
| `docs/requirements.md` | Created | 49 requirements, all derived from source |
| `docs/testing.md` | Created | 9 test cases, 49/49 coverage, all PASS |
| `docs/misra.md` | Created | 136 findings, 34 fixed, 102 justified |
| `firmware/Core/Src/main.c` | Modified | Rules 5.9, 15.6, 17.8 |
| `firmware/Core/Src/uart.c` | Modified | Rules 8.7, 15.6 |
| `firmware/Core/Src/filter.c` | Modified | Rules 10.4, 15.6 |
| `firmware/Core/Src/i2c.c` | Modified | Rules 5.9, 15.6, 17.8 |
| `firmware/Core/Src/max30102.c` | Modified | Rules 5.9, 15.6, 17.7, 17.8 |

---

## What Was NOT Done (Deferred to Next Session)

1. **MISRA post-fix verification re-run** — confirm 34 fixes reduced the count
2. **Commit EXT-01 to EXT-03** — stage and commit all docs + code fixes
3. **EXT-04: SysTick calibrated delay** — replace `main_delay/i2c_delay/max30102_delay` with SysTick-based 1ms tick
4. **EXT-05: IWDG watchdog** — add watchdog kick in main loop, document timeout calculation
5. **EXT-06: Heart rate (BPM)** — peak detection on filtered IR → BPM output
6. **EXT-07: FreeRTOS** — sensor task + UART task separation
7. **EXT-08: STOP2 sleep** — low-power mode between reads

---

## Resume Checklist

When starting the next session:
1. ~~Run the MISRA verification re-run~~ — DONE (98 justified deviations)
2. ~~Commit Phase 1~~ — DONE
3. Begin EXT-04 (SysTick calibrated delay)

---

*Log written: April 4, 2026*
*Dev path: C:/Users/vaibh/Projects/dev/stm32-vitals-monitor*
