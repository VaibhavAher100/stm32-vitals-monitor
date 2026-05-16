# Implementation Roadmap - STM32 Vitals Monitor

Status date: May 16, 2026

This file is the canonical project timeline for the STM32 repository. It
separates completed work, partially complete quality gates, and future hardware
extensions so the README, requirements, and CV claims stay truthful.

---

## Current Status

| Phase | Item | Status | Evidence |
|---|---|---|---|
| Original build | M1-M7 - blink, UART, I2C, TMP117, MAX30102, filter, 3-layer refactor | DONE | Git history, README, hardware result screenshots |
| EXT-01 | MISRA-C / cppcheck analysis | DONE, maintain | `docs/misra.md` |
| EXT-02 | Strict warnings | DONE for syntax-only verification, CubeIDE config to keep reviewed | `verify.sh`, `.cproject` |
| EXT-03 | Requirements traceability | DONE, maintain | `docs/requirements.md`, `/* Implements */` comments |
| EXT-04 | Unity host tests | DONE | `tests/test_filter.c`, `tests/test_bpm.c` |
| EXT-05 | SysTick deterministic timing | SUPERSEDED | FreeRTOS owns SysTick at 1 kHz |
| EXT-06 | FreeRTOS task scheduler | DONE | `tasks_vitals.c`, `FreeRTOSConfig.h` |
| EXT-07 | IWDG watchdog | DONE, limited | `iwdg.c`, `docs/limitations.md` |
| EXT-08 | STOP2 low-power mode | NOT STARTED | Future hardware phase |
| EXT-09 | BPM from PPG | DONE | `bpm.c`, unit tests, UART hardware evidence |
| EXT-10 | BLE via ESP32 | NOT STARTED | Future hardware phase |

---

## Cleanup Rules

- Requirements describe current firmware only. Future work stays in this roadmap
  or in the Out of Scope table.
- Hardware PASS claims stay only when they match the current code.
- If code changes a behavior, update `docs/requirements.md`, `docs/testing.md`,
  and source traceability in the same commit.
- Do not claim clinical validity. BPM is a signal-processing demonstration, not
  a medical measurement.
- Do not claim full MISRA-C or IEC 62304 compliance. The honest claim is:
  MISRA-C awareness with documented cppcheck analysis, and IEC 62304-style
  requirements traceability.

---

## Next Hardware Phases

### Phase A - Re-verify Current Firmware

Use the hardware currently available:

1. Clean build in STM32CubeIDE.
2. Flash `firmware/Core/Debug/Core.bin` to the Nucleo-L476RG.
3. Confirm UART at 9600 baud.
4. Confirm `TMP117 OK` and `MAX30102 OK`.
5. Record 60 seconds of output with:
   - no finger
   - stable finger contact
   - finger removed
6. Confirm BPM starts as `---`, becomes plausible with contact, and returns to
   `---` after signal loss or stale detection.
7. Save one screenshot in `results/` and update `docs/testing.md` only with what
   was actually observed.

### Phase B - STOP2 Low-Power Mode

Status: NOT STARTED.

Deliverables:

- RTC wakeup or equivalent low-power wake source.
- STOP2 entry between measurement windows.
- Clock/peripheral restoration after wake.
- Measured active and sleep current.
- Battery-life calculation in `docs/testing.md`.

### Phase C - BLE Streaming via ESP32

Status: NOT STARTED.

Deliverables:

- `ble_comms.c/h` driver for STM32 UART link to ESP32.
- ESP32 BLE GATT characteristic that publishes vitals rows.
- Phone verification with nRF Connect.
- Architecture and requirements updates.

---

## Profile Claims Allowed Now

- Bare-metal STM32 C using CMSIS register access.
- I2C sensor integration with TMP117 and MAX30102.
- FreeRTOS tasks and queue-based IPC.
- IWDG watchdog for startup/task-hang recovery.
- Host-side Unity tests for hardware-independent modules.
- Requirements traceability and cppcheck/MISRA-C awareness.
- PPG-based BPM signal processing demonstration.

Do not claim STOP2, BLE, clinical heart-rate accuracy, SpO2, or full standards
compliance until those items are implemented and verified.
