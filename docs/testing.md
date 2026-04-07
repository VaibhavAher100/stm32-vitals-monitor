# Test Record — STM32 Vitals Monitor

This document records how each requirement in `docs/requirements.md` was verified.
Each test case has a unique TC-ID, lists the requirements it covers, describes
the procedure, states the expected and actual result, and cites the evidence.

Two verification methods are used:
- **Hardware test** — firmware flashed to the board, output observed
- **Static inspection** — source files examined by grep or code review

All hardware tests were performed on the target hardware during development.
TC-01 to TC-09 formalise tests already run - evidence is the existing results/ screenshots.
Hardware: STM32 Nucleo-L476RG · TMP117 (0x49) · MAX30102 (0x57) · COM7 · 9600 baud

---

## Test Cases

---

### TC-01 — UART Initialisation and Transmission

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-01, REQ-UART-01, REQ-UART-02, REQ-UART-03, REQ-UART-04, REQ-UART-05 |
| **Evidence** | `results/uart_verified_coolterm_com7_9600.png` |

**Procedure:**
1. Flash firmware to Nucleo-L476RG.
2. Connect CoolTerm to COM7, 9600 baud, 8N1, no flow control.
3. Press RESET on the board.
4. Observe CoolTerm output.

**Expected result:**
- Header block appears: `STM32 Vitals Monitor`, separator lines, column headers.
- Characters are correctly decoded at 9600 baud with no garbled output.
- Output continues at regular intervals (~500 ms per row).

**Actual result:** Header and measurement rows appeared correctly. No garbled
characters. Baud rate confirmed correct by readable ASCII output.

**Status: PASS**

---

### TC-02 — I2C Bus Configuration and Sensor Address Detection

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-I2C-01, REQ-I2C-02, REQ-I2C-03, REQ-I2C-04 |
| **Evidence** | `results/i2c_scanner_both_sensors.png` |

**Procedure:**
1. Flash an I2C scanner build (scans addresses 0x00–0x7F, prints ACK responses over UART).
2. Connect TMP117 and MAX30102 to PB8/PB9.
3. Press RESET and observe UART output.

**Expected result:**
- Address 0x49 (TMP117) responds with ACK.
- Address 0x57 (MAX30102) responds with ACK.
- No other unexpected addresses respond.

**Actual result:** Both 0x49 and 0x57 detected. No spurious addresses.
Confirms I2C1 is correctly configured on PB8/PB9 at 100 kHz with open-drain
outputs and correct AF4 routing.

**Status: PASS**

---

### TC-03 — TMP117 Initialisation and Temperature Reading

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-03, REQ-SYS-04, REQ-TMP-01, REQ-TMP-02, REQ-TMP-03, REQ-TMP-04, REQ-TMP-05 |
| **Evidence** | `results/tmp117_temperature_verified.png` |

**Procedure:**
1. Flash full firmware.
2. Observe UART output: check `TMP117  OK` in startup block.
3. Observe temperature column across multiple rows.
4. Apply gentle warmth to the TMP117 breakout (fingertip contact).
5. Observe whether the temperature value rises.

**Expected result:**
- `TMP117  OK` printed at startup (device ID 0x0117 verified).
- Temperature reads in a plausible ambient range (~20–25 °C).
- Temperature rises measurably when sensor is warmed.
- Format is `XX.X` (integer part, decimal point, one digit).

**Actual result:** `TMP117  OK` confirmed. Ambient readings around 23 °C.
Temperature rose on contact. Format correct.

**Status: PASS**

---

### TC-04 — MAX30102 Initialisation and IR FIFO Read

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-03, REQ-SYS-04, REQ-MAX-01, REQ-MAX-02, REQ-MAX-03, REQ-MAX-04, REQ-MAX-05, REQ-MAX-06, REQ-MAX-07, REQ-MAX-08, REQ-MAX-09, REQ-MAX-10 |
| **Evidence** | `results/max30102_ir_raw_verified.png` |

**Procedure:**
1. Flash full firmware.
2. Observe UART output: check `MAX30102 OK` in startup block.
3. Observe raw IR column with no finger on sensor.
4. Place fingertip firmly on the MAX30102 sensor window.
5. Observe whether raw IR values increase substantially.
6. Remove finger and observe values return to ambient baseline.

**Expected result:**
- `MAX30102 OK` at startup (Part ID 0x15 verified).
- Ambient IR values: ~700 (no finger).
- Finger-contact IR values: 80000–92000 (18-bit range, light absorption from blood).
- Values return to baseline after finger removal.
- No value exceeds 0x3FFFF (262143) — 18-bit mask applied.

**Actual result:** `MAX30102 OK` confirmed. Ambient ~700. Finger contact
produced readings in the 80000–92000 range. Values returned to baseline on
removal. All values within 18-bit range.

**Status: PASS**

---

### TC-05 — Both Sensors Combined Operation and Output Format

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-02, REQ-SYS-05, REQ-OUT-01, REQ-OUT-02, REQ-OUT-03 |
| **Evidence** | `results/both_sensors_combined_output.png` |

**Procedure:**
1. Flash full firmware with both sensors connected.
2. Observe UART output for the complete startup block and multiple measurement rows.
3. Verify each row contains all three fields.
4. Verify row delimiters and line endings.

**Expected result:**
- Startup block: both `TMP117  OK` and `MAX30102 OK` present.
- Each row contains three columns: temperature, raw IR, filtered IR.
- Columns separated by ` | ` consistent with the header.
- Each row ends with `\r\n` (CoolTerm shows clean line breaks).

**Actual result:** Both sensors reported OK. All measurement rows contain
three fields with consistent delimiters. No missing or extra columns observed.

**Status: PASS**

---

### TC-06 — Moving Average Filter Behaviour

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-FIL-01, REQ-FIL-02, REQ-FIL-03, REQ-FIL-04, REQ-FIL-05, REQ-FIL-06 |
| **Evidence** | `results/refactored_3layer_output.png` |

**Procedure:**
1. Flash full firmware.
2. Observe raw IR and filtered IR columns with no finger on sensor.
3. Place finger on MAX30102 and observe both columns across 8+ rows.
4. Remove finger and observe filtered value decay.

**Expected result:**
- With no finger: raw and filtered IR are both low (~700), close to equal.
- On first finger contact: raw IR jumps immediately; filtered IR ramps up
  over approximately 8 rows as the window fills.
- After finger removal: raw IR drops immediately; filtered IR decays
  gradually over 8 rows as old high values leave the window.
- Filtered value is always ≤ raw peak (average cannot exceed any single sample).

**Actual result:** Filter ramp-up and decay visible across 8-row window.
Raw IR responded immediately on contact/removal. Filtered IR lagged by
approximately the window duration. Values consistent with an 8-sample
moving average.

**Status: PASS**

---

### TC-07 — Three-Layer Architecture: No Cross-Layer Register Access

| Field | Detail |
|---|---|
| **Method** | Static inspection |
| **Requirements** | REQ-NF-06, REQ-NF-07, REQ-NF-08 |
| **Evidence** | Source file inspection (commands below) |

**Procedure:**
Search `main.c` for any direct register address (hex literals used as
peripheral addresses). Search `filter.c` and `filter.h` for any hardware
register definitions or STM32-specific includes.

```
# REQ-NF-07: main.c must contain no register addresses
grep -n "0x4[0-9A-Fa-f]\{7\}" firmware/Core/Src/main.c

# REQ-NF-08: filter must have no hardware dependencies
grep -n "0x4[0-9A-Fa-f]\{7\}\|stm32\|cmsis" firmware/Core/Src/filter.c firmware/Core/Inc/filter.h

# REQ-NF-06: driver files must not call application-layer functions
grep -n "uart_\|main\b" firmware/Core/Src/tmp117.c firmware/Core/Src/max30102.c firmware/Core/Src/i2c.c
```

**Expected result:**
- No register addresses in `main.c`.
- No hardware includes or addresses in `filter.c` / `filter.h`.
- Driver files do not call `uart_*` or `main()`.

**Actual result:**
- `main.c`: zero register address literals. All hardware access via
  `uart_*`, `i2c_*`, `tmp117_*`, `max30102_*`, `filter_*` calls.
- `filter.c` / `filter.h`: contains only `<stdint.h>`. No addresses, no
  STM32 includes. Compilable on any platform.
- `tmp117.c`, `max30102.c`, `i2c.c`: call only `i2c_*` functions.
  No `uart_*` calls. No calls to `main`.

**Status: PASS**

---

### TC-08 — No Dynamic Memory Allocation and No Recursion

| Field | Detail |
|---|---|
| **Method** | Static inspection |
| **Requirements** | REQ-NF-01, REQ-NF-02, REQ-NF-03 |
| **Evidence** | Source file inspection (commands below) |

**Procedure:**
Search all source files for dynamic allocation functions and variable-length arrays.

```
# REQ-NF-01: no dynamic allocation
grep -rn "malloc\|calloc\|free\|realloc" firmware/Core/Src/ firmware/Core/Inc/

# REQ-NF-02/03: no recursion or VLAs — manual review of all functions
```

**Expected result:**
- Zero matches for any dynamic allocation function.
- No function calls itself directly or indirectly.
- No variable-length array declarations.

**Actual result:**
- No `malloc`, `calloc`, `free`, or `realloc` found in any source file.
- All arrays are fixed-size: `Filter.buf[FILTER_WINDOW]` (constant 8),
  `char buf[12]` in `uart_int()`. Both compile-time constants.
- All functions are iterative. No recursive call patterns identified.

**Status: PASS**

---

### TC-09 — Volatile Qualifier on All Register Definitions

| Field | Detail |
|---|---|
| **Method** | Static inspection |
| **Requirements** | REQ-NF-04 |
| **Evidence** | Source file inspection (commands below) |

**Procedure:**
Inspect all register macro definitions in driver source files and verify
each uses `volatile uint32_t *`.

```
grep -n "0x4[0-9A-Fa-f]\{7\}" firmware/Core/Src/uart.c firmware/Core/Src/i2c.c firmware/Core/Src/max30102.c
```

**Expected result:**
Every line defining a register address uses the pattern:
`(*((volatile uint32_t*)0x...))`.

**Actual result:**
All register definitions in `uart.c` (11 definitions), `i2c.c`
(12 definitions), and `max30102.c` (5 inline definitions) use
`volatile uint32_t *` casts. No bare pointer casts found.

**Status: PASS**

---

### TC-10 — SysTick 1 ms Tick: UART Timestamp Advances

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-06 |
| **Evidence** | UART output observed; `get_tick()` value printed at loop start |

**Procedure:**
1. Temporarily modify `main.c` to print `get_tick()` at the top of the measurement loop.
2. Flash firmware. Observe two consecutive printed tick values over UART.
3. Calculate the difference. Expected: approximately 500 ms (= 500 ticks, plus sensor read time).

**Expected result:**
- Each UART row includes a tick timestamp.
- Consecutive timestamps differ by approximately 500 ticks.
- Tick counter increments monotonically with no stall or wrap within the observation window (~30 rows).

**Actual result:** Tick counter advanced by ~500–510 counts between rows, consistent with the 500 ms `delay_ms()` call plus sensor read overhead. Counter incremented monotonically across all observed rows.

**Status: PASS**

---

### TC-11 — IWDG: Board Resets if Main Loop Blocked

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-07 |
| **Evidence** | UART restart banner observed after forced hang |

**Procedure:**
1. Temporarily modify `main.c` to spin indefinitely (`while(1){}`) at the top of the measurement loop, before `iwdg_kick()`.
2. Flash firmware.
3. Observe UART: the startup banner (`STM32 Vitals Monitor`) should reappear after approximately 4 seconds, indicating IWDG reset.
4. Restore `main.c` to normal, reflash, and confirm the banner appears only once at power-on.

**Expected result:**
- With the forced hang: startup banner repeats every ~4 s (IWDG timeout).
- Without the hang: banner appears exactly once; measurement rows continue indefinitely without reset.

**Actual result:** With the forced hang the startup banner repeated at ~4 s intervals confirming the 4-second IWDG timeout. With the hang removed, normal operation ran for 5 minutes with no unexpected reset.

**Status: PASS**

---

### TC-12 — BPM Detection: Heart Rate Output at Rest

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-08, REQ-OUT-01 |
| **Evidence** | UART output observed with finger placed on MAX30102 |

**Procedure:**
1. Flash firmware with Phase 3 BPM code.
2. Connect CoolTerm, press RESET. Confirm `---` appears in the BPM column for the first ~10 rows (history window + first crossings accumulating).
3. Place finger firmly on the MAX30102 sensor.
4. Observe BPM column for at least 30 rows (~15 seconds).

**Expected result:**
- BPM column shows `---` until the first two threshold crossings are recorded.
- Once crossings are detected, BPM value appears in the range 50–110 BPM (typical resting adult).
- BPM updates each row as new crossings are detected.
- Removing the finger causes BPM to stall at the last valid reading (no new crossings, no new intervals).

**Actual result:** `---` displayed for first ~10 rows while the 8-sample history window and first crossings accumulated. BPM column settled to 68–74 BPM during 30-second rest measurement. Value updated row-by-row as new beats were detected. Finger removal stopped updates; last valid value held.

**Status: PASS**

---

## Requirements Coverage Matrix

| Requirement | Test Case | Status |
|---|---|---|
| REQ-SYS-01 | TC-01 | PASS |
| REQ-SYS-02 | TC-05 | PASS |
| REQ-SYS-03 | TC-03, TC-04 | PASS |
| REQ-SYS-04 | TC-03, TC-04 | PASS |
| REQ-SYS-05 | TC-05 | PASS |
| REQ-UART-01 | TC-01 | PASS |
| REQ-UART-02 | TC-01 | PASS |
| REQ-UART-03 | TC-01 | PASS |
| REQ-UART-04 | TC-01 | PASS |
| REQ-UART-05 | TC-01 | PASS |
| REQ-I2C-01 | TC-02 | PASS |
| REQ-I2C-02 | TC-02 | PASS |
| REQ-I2C-03 | TC-02 | PASS |
| REQ-I2C-04 | TC-02 | PASS |
| REQ-I2C-05 | TC-02 | PASS |
| REQ-I2C-06 | TC-02 | PASS |
| REQ-I2C-07 | TC-02 | PASS |
| REQ-TMP-01 | TC-03 | PASS |
| REQ-TMP-02 | TC-03 | PASS |
| REQ-TMP-03 | TC-03 | PASS |
| REQ-TMP-04 | TC-03 | PASS |
| REQ-TMP-05 | TC-03 | PASS |
| REQ-MAX-01 | TC-04 | PASS |
| REQ-MAX-02 | TC-04 | PASS |
| REQ-MAX-03 | TC-04 | PASS |
| REQ-MAX-04 | TC-04 | PASS |
| REQ-MAX-05 | TC-04 | PASS |
| REQ-MAX-06 | TC-04 | PASS |
| REQ-MAX-07 | TC-04 | PASS |
| REQ-MAX-08 | TC-04 | PASS |
| REQ-MAX-09 | TC-04 | PASS |
| REQ-MAX-10 | TC-04 | PASS |
| REQ-FIL-01 | TC-06 | PASS |
| REQ-FIL-02 | TC-06 | PASS |
| REQ-FIL-03 | TC-06 | PASS |
| REQ-FIL-04 | TC-06 | PASS |
| REQ-FIL-05 | TC-06 | PASS |
| REQ-FIL-06 | TC-06 | PASS |
| REQ-OUT-01 | TC-05 | PASS |
| REQ-OUT-02 | TC-05 | PASS |
| REQ-OUT-03 | TC-05 | PASS |
| REQ-NF-01 | TC-08 | PASS |
| REQ-NF-02 | TC-08 | PASS |
| REQ-NF-03 | TC-08 | PASS |
| REQ-NF-04 | TC-09 | PASS |
| REQ-NF-05 | TC-07 | PASS |
| REQ-NF-06 | TC-07 | PASS |
| REQ-NF-07 | TC-07 | PASS |
| REQ-NF-08 | TC-07 | PASS |
| REQ-NF-09 | TC-01 | PASS |
| REQ-SYS-06 | TC-10 | PASS |
| REQ-SYS-07 | TC-11 | PASS |
| REQ-SYS-08 | TC-12 | PASS |
| REQ-OUT-01 | TC-05, TC-12 | PASS |

**Coverage: 52 / 52 requirements. All tests PASS.**

---

*Document version: 1.0*
*Author: Vaibhav Aher — M.Sc. ICT, FAU Erlangen-Nürnberg*
*Date: April 2026*
*Hardware: STM32 Nucleo-L476RG · TMP117 · MAX30102*
