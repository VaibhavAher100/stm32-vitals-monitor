# Test Record - STM32 Vitals Monitor

This document records how each requirement in `docs/requirements.md` was verified.
Each test case has a unique TC-ID, lists the requirements it covers, describes
the procedure, states the expected and actual result, and cites the evidence.

Two verification methods are used:
- **Hardware test** - firmware flashed to the board, output observed
- **Static inspection** - source files examined by grep or code review

All hardware tests were performed on the target hardware during development.
TC-01 to TC-09 formalise tests already run - evidence is the existing results/ screenshots.
Hardware: STM32 Nucleo-L476RG · TMP117 (0x49) · MAX30102 (0x57) · COM7 · 9600 baud

---

## Test Cases

---

### TC-01 - UART Initialisation and Transmission

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

### TC-02 - I2C Bus Configuration and Sensor Address Detection

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

### TC-03 - TMP117 Initialisation and Temperature Reading

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

### TC-04 - MAX30102 Initialisation and PPG FIFO Read

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-03, REQ-SYS-04, REQ-MAX-01, REQ-MAX-02, REQ-MAX-03, REQ-MAX-04, REQ-MAX-05, REQ-MAX-06, REQ-MAX-07, REQ-MAX-08, REQ-MAX-09, REQ-MAX-10 |
| **Evidence** | `results/max30102_ir_raw_verified.png` |

**Procedure:**
1. Flash full firmware.
2. Observe UART output: check `MAX30102 OK` in startup block.
3. Observe raw PPG column with no finger on sensor.
4. Place fingertip firmly on the MAX30102 sensor window.
5. Observe whether raw PPG values increase substantially.
6. Remove finger and observe values return to ambient baseline.

**Expected result:**
- `MAX30102 OK` at startup (Part ID 0x15 verified).
- Ambient raw PPG values: ~700 (no finger).
- Finger-contact raw PPG values: 80000-92000 (18-bit range, light absorption from blood).
- Values return to baseline after finger removal.
- No value exceeds 0x3FFFF (262143) - 18-bit mask applied.

**Actual result:** `MAX30102 OK` confirmed. Ambient ~700. Finger contact
produced readings in the 80000–92000 range. Values returned to baseline on
removal. All values within 18-bit range.

**Status: PASS**

---

### TC-05 - Both Sensors Combined Operation and Output Format

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-SYS-02, REQ-OUT-01, REQ-OUT-02, REQ-OUT-03, REQ-OUT-04 |
| **Evidence** | `results/both_sensors_combined_output.png` |

**Procedure:**
1. Flash full firmware with both sensors connected.
2. Observe UART output for the complete startup block and multiple measurement rows.
3. Verify each row contains all four fields.
4. Verify row delimiters and line endings.

**Expected result:**
- Startup block: both `TMP117  OK` and `MAX30102 OK` present.
- Each row contains four columns: temperature, raw PPG, DC/contact estimate, BPM.
- Columns separated by ` | ` consistent with the header.
- Each row ends with `\r\n` (CoolTerm shows clean line breaks).

**Actual result:** Both sensors reported OK. All measurement rows contain
four fields with consistent delimiters. No missing or extra columns observed.

**Status: PASS**

---

### TC-06 - Moving Average Filter Behaviour

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-FIL-01, REQ-FIL-02, REQ-FIL-03, REQ-FIL-04, REQ-FIL-05, REQ-FIL-06 |
| **Evidence** | `results/refactored_3layer_output.png` |

**Procedure:**
1. Flash full firmware.
2. Observe raw PPG and DC/contact estimate columns with no finger on sensor.
3. Place finger on MAX30102 and observe raw PPG and DC/contact estimate columns.
4. Remove finger and observe contact estimate return toward ambient.

**Expected result:**
- With no finger: raw PPG is low (~700) and BPM remains `---`.
- On finger contact: raw PPG increases substantially and the contact estimate follows the DC level.
- BPM detection uses the internally filtered AC PPG signal, not the displayed DC/contact estimate.
- Filtered value is always ≤ raw peak (average cannot exceed any single sample).

**Actual result:** Raw PPG responded immediately on contact/removal. Displayed
contact estimate tracked the DC level. Host-side unit tests TC-15 verify the
4-sample moving average implementation used by BPM processing.

**Status: PASS**

---

### TC-07 - Three-Layer Architecture: No Cross-Layer Register Access

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

### TC-08 - No Dynamic Memory Allocation and No Recursion

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

# REQ-NF-02/03: no recursion or VLAs - manual review of all functions
```

**Expected result:**
- Zero matches for any dynamic allocation function.
- No function calls itself directly or indirectly.
- No variable-length array declarations.

**Actual result:**
- No `malloc`, `calloc`, `free`, or `realloc` found in any source file.
- All arrays are fixed-size: `Filter.buf[FILTER_WINDOW]` (constant 4),
  `char buf[12]` in `uart_int()`. Both compile-time constants.
- All functions are iterative. No recursive call patterns identified.

**Status: PASS**

---

### TC-09 - CMSIS Register Access: No Hardcoded Peripheral Base Addresses

| Field | Detail |
|---|---|
| **Method** | Static inspection |
| **Evidence** | Source file inspection (commands below) |

**Procedure:**
Confirm all driver files use CMSIS peripheral struct access (`PERIPHERAL->REGISTER`)
and contain zero hardcoded peripheral base address macros (0x4xxxxxxx range).

Note: bitmask constants (e.g., `ICR = 0x3F38U`) and protocol-defined values
(e.g., IWDG keys `0xCCCCU`, `0x5555U`, `0xAAAAU`) are not peripheral addresses
and are not checked by this grep.

```
grep -n "0x4[0-9A-Fa-f]\{7\}" firmware/Core/Src/uart.c firmware/Core/Src/i2c.c \
  firmware/Core/Src/iwdg.c firmware/Core/Src/max30102.c
```

**Expected result:**
Zero matches. All register access via CMSIS structs (`USART2->BRR`, `I2C1->CR1`,
`IWDG->KR`, etc.) from `stm32l476xx.h`.

**Actual result:**
Zero raw address matches in any driver file. All peripheral access uses CMSIS
struct notation. `stm32l476xx.h` provides the volatile struct definitions;
`USART2->BRR` and `*((volatile uint32_t*)0x4000440C)` compile to identical
instructions.

**Status: PASS**

---

### TC-10 - SysTick 1 ms Tick: UART Timestamp Advances

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-STK-01 |
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

### TC-11 - IWDG: Board Resets if Main Loop Blocked

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-WDG-01, REQ-WDG-02 |
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

### TC-12 - BPM Detection: Heart Rate Output at Rest

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-BPM-01, REQ-BPM-02, REQ-BPM-03, REQ-BPM-04, REQ-BPM-05, REQ-OUT-01, REQ-OUT-04 |
| **Hardware** | Nucleo-L476RG, MAX30102 (LED 12.6 mA, HR mode), CoolTerm serial monitor |
| **Evidence** | UART output captured May 2026, hardware on desk |

**Procedure:**
1. Flash current firmware to Nucleo-L476RG.
2. Open CoolTerm (9600 baud). Press RESET. Confirm `TMP117 OK`, `MAX30102 OK`.
3. Observe ambient readings for 10 seconds: raw ~1730-1850, BPM `---`.
4. Rest fingertip lightly on MAX30102 sensor glass - do not press.
5. Hold for 60 seconds without moving.
6. Remove finger. Confirm BPM returns to `---`.

**Expected result:**
- No finger: raw 1730-1850 (ambient at 12.6 mA LED), filt 0, BPM `---`.
- Finger contact: raw jumps to ~180,000-200,000 range.
- DC estimator (filt column) converges toward raw over ~20 seconds.
- BPM appears after convergence in physiological range (30-100 BPM).
- Removing finger: raw returns to ambient, BPM returns to `---`.
- No IWDG reset events during the run.

**Actual result (hardware run May 2026):**
- Ambient: raw 1738-1753, BPM `---`. Temp 25.6°C. ✓
- Finger contact: raw 164,136 rising to 196,000-197,000 range. ✓
- DC convergence visible over ~20 rows (10 seconds). ✓
- BPM locked in range 64-97 BPM (resting adult). ✓
- Finger removed: raw returned to 1530-1937 range, BPM `---`. ✓
- No IWDG resets, no stack overflow events. ✓
- Temperature stable 25.6-26.2°C across 140+ rows. ✓

**Known limitation:** BPM readings vary ±15 BPM due to finger position sensitivity
and PPG signal amplitude variation. Schmitt trigger crossing detection without dedicated
PPG algorithms is sufficient to demonstrate the signal processing chain but not for
clinical measurement.

**Status: PASS**

---

### TC-13 - FreeRTOS: Scheduler Starts, Sensor Task Runs, UART Output Continues

| Field | Detail |
|---|---|
| **Method** | Hardware test |
| **Requirements** | REQ-RTOS-01, REQ-RTOS-02, REQ-RTOS-03, REQ-RTOS-04, REQ-RTOS-05 |
| **Evidence** | `results/freertos_bpm_output_verified.png` |

**Procedure:**
1. Flash Phase 4 firmware to Nucleo-L476RG.
2. Connect CoolTerm to COM7, 9600 baud, 8N1.
3. Observe startup banner - printed by `main()` before `vTaskStartScheduler()`.
4. Observe that sensor init messages (`TMP117 OK`, `MAX30102 OK`) appear after the scheduler starts and `task_vitals` runs.
5. Observe that measurement rows continue to appear at ~500 ms intervals (`vTaskDelay(pdMS_TO_TICKS(500U))` inside the task loop).
6. Confirm BPM column shows `---` initially then valid BPM with finger on sensor (same behaviour as Phase 3).
7. Confirm firmware does not hang or reset - rows continue indefinitely (scheduler running, IWDG kicked every 500 ms by `task_vitals`).

**Expected result:**
- Startup banner followed by sensor OK messages once scheduler is running.
- Measurement rows appear continuously at ~500 ms interval.
- BPM column behaves identically to Phase 3.
- No hang or unexpected reset (IWDG kicked inside task loop before `vTaskDelay`).

**Actual result (April 2026 Phase 4 initial run, LED 0x10 / 3.2 mA):** Startup banner
appeared immediately after flash. `TMP117 OK` and `MAX30102 OK` printed once scheduler
started and `task_sensor` entered its init block. Measurement rows appeared at ~500 ms
intervals. BPM column showed `---` for the first rows while the history window filled,
then settled to 119 BPM with finger on sensor. Ambient IR ~1000, finger IR ~86000. No
hangs or unexpected resets observed during the run. IWDG kicked every 500 ms by
`task_sensor` as expected.

**Note:** LED current was subsequently increased to 0x3F (12.6 mA). May 2026 re-run
results (IR ~196,000, BPM 64-97) are documented in TC-12.

**Status: PASS**

---

### TC-14 - CMSIS Build: Zero Errors on Full Recompile

| Field | Detail |
|---|---|
| **Method** | Hardware test + build verification |
| **Evidence** | STM32CubeIDE build console output |

**Procedure:**
1. Open workspace `firmware/` in STM32CubeIDE 2.1.0.
2. Project → Clean, then Ctrl+B (Debug configuration).
3. Observe build console for errors or warnings.
4. Flash `firmware/Core/Debug/Core.bin` to Nucleo-L476RG.
5. Observe UART output: confirm header, sensor OK messages, and BPM readings.

**Expected result:**
- Build console ends with `Build Finished. 0 errors, 0 warnings`.
- All 12 project source files compiled with `-DSTM32L476xx` and CMSIS include paths.
- FreeRTOS source (tasks.c, queue.c, list.c, port.c, heap_4.c) compiled from `Middlewares/`.
- Hardware output matches verified numbers: TMP117 ~25 C, IR ~86000 finger, BPM ~119.

**Actual result:** Build completed with 0 errors, 0 warnings. All source files compiled
including FreeRTOS. Firmware flashed and ran correctly on hardware: TMP117 25.3 C,
MAX30102 raw PPG ~86000 with finger contact, BPM 119 stable after window fill.
No IWDG resets observed.

**Status: PASS**

---

### TC-15 - Unity Unit Tests: filter.c (Host)

| Field | Detail |
|---|---|
| **Method** | Automated unit test (host machine, no hardware required) |
| **Requirements** | REQ-FIL-01, REQ-FIL-02, REQ-FIL-03, REQ-FIL-04, REQ-FIL-05, REQ-FIL-06 |
| **Evidence** | `tests/test_filter.c` - 8 test cases |

**Procedure:**
```
cd tests
mingw32-make test_filter && ./test_filter
```

**Test cases:**
- `test_filter_init_zeros_all_fields` - all struct members zero after init
- `test_filter_single_value_returns_that_value` - first update divides by 1
- `test_filter_partial_fill_three_values` - [100,200,300] -> average 200
- `test_filter_full_window_averages_all_eight` - fills `FILTER_WINDOW` samples and verifies the average
- `test_filter_rollover_drops_oldest` - 9th sample replaces oldest, not all 9
- `test_filter_all_identical_values` - identical values across the window -> average equals input
- `test_filter_count_saturates_at_window_size` - 16 updates, count stays 8
- `test_filter_reinit_resets_used_filter` - init after use restores clean state

**Actual result:** 8/8 PASS. Output:
```
8 Tests 0 Failures 0 Ignored
OK
```

**Status: PASS**

---

### TC-16 - Unity Unit Tests: bpm.c (Host)

| Field | Detail |
|---|---|
| **Method** | Automated unit test (host machine, no hardware required) |
| **Requirements** | REQ-BPM-01, REQ-BPM-02, REQ-BPM-03, REQ-BPM-04, REQ-BPM-05 |
| **Evidence** | `tests/test_bpm.c` - 7 test cases. `xTaskGetTickCount()` stubbed in `tests/stubs/`. |

**Procedure:**
```
cd tests
mingw32-make test_bpm && ./test_bpm
```

**Test cases:**
- `test_bpm_invalid_before_two_crossings` - BPM_INVALID on fresh detector
- `test_bpm_no_crossing_during_history_fill` - no crossing fires until BPM_HISTORY window full
- `test_bpm_invalid_after_one_crossing` - BPM_INVALID after exactly one crossing
- `test_bpm_valid_after_two_crossings` - 600ms interval -> 100 BPM
- `test_bpm_refractory_rejects_fast_crossing` - 200ms crossing rejected, stays invalid
- `test_bpm_known_interval_500ms_gives_120bpm` - 500ms interval -> 120 BPM
- `test_bpm_flat_signal_never_crosses` - constant value, threshold == signal, no crossing

**Actual result:** 7/7 PASS. Output:
```
7 Tests 0 Failures 0 Ignored
OK
```

**Status: PASS**

---

## Test Summary

| TC-ID | Description | Method | Status |
|---|---|---|---|
| TC-01 | UART init and transmission | Hardware | PASS |
| TC-02 | I2C bus + sensor address detection | Hardware | PASS |
| TC-03 | TMP117 temperature reading | Hardware | PASS |
| TC-04 | MAX30102 PPG FIFO read | Hardware | PASS |
| TC-05 | Both sensors combined output format | Hardware | PASS |
| TC-06 | Moving average filter behaviour | Hardware | PASS |
| TC-07 | 3-layer architecture: no cross-layer register access | Static | PASS |
| TC-08 | No dynamic allocation, no recursion | Static | PASS |
| TC-09 | CMSIS register access: no hardcoded peripheral base addresses | Static | PASS |
| TC-10 | SysTick 1 ms tick advances correctly | Hardware | PASS |
| TC-11 | IWDG resets board if task hangs | Hardware | PASS |
| TC-12 | BPM detection at rest | Hardware | PASS |
| TC-13 | FreeRTOS scheduler, two-task operation, IWDG kick | Hardware | PASS |
| TC-14 | CMSIS build: zero errors, hardware verified | Hardware | PASS |
| TC-15 | Unity unit tests: filter.c - 8 test cases | Automated | PASS |
| TC-16 | Unity unit tests: bpm.c - 7 test cases | Automated | PASS |

**16 / 16 documented test cases PASS. TC-15 and TC-16 are host-runnable automated suites with 15 unit tests total.**

---

*Document version: 3.0*
*Updated: May 2026 - Unity unit tests added (TC-15, TC-16)*
*Author: Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg*
*Hardware: STM32 Nucleo-L476RG - TMP117 - MAX30102*
