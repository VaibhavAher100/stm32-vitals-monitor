# Limitations - STM32 Vitals Monitor

Known limitations.

---

## Medical and Clinical Limitations

### No Clinical SpO2 Accuracy

**This project does not produce clinically valid SpO2 (blood oxygen saturation) readings.**

The MAX30102 sensor reads raw photoplethysmographic (PPG) data - the change in light absorption
caused by pulsatile blood flow. Converting raw PPG data to a calibrated SpO2 percentage requires:

- Empirical calibration coefficients derived from measurements on human subjects
- Validated algorithms (typically proprietary, e.g., Maxim's algorithm)
- Regulatory approval (FDA clearance or CE marking) for medical use
- Controlled placement on the body (fingertip, earlobe, or wrist)

None of these are implemented here. The raw PPG values printed by this firmware indicate
relative changes in light absorption, not an absolute SpO2 percentage.

**Do not use the output of this firmware for any medical decision.**

### BPM Not Clinically Validated

The BPM detector uses a Schmitt trigger on the IIR-DC-removed, moving-average-filtered AC PPG signal.
A 600 ms refractory period suppresses dicrotic notch double-detection.
BPM is reported once two valid threshold crossings are recorded with a consensus filter
requiring two consecutive readings within ±2 BPM.

Hardware-verified (May 2026, Nucleo-L476RG, MAX30102 at 12.6 mA):
- DC estimator requires ~20 seconds to converge after finger placement before BPM appears.
- BPM reads in the 64-97 BPM range for a resting adult (physiologically plausible).
- Readings vary ±15 BPM due to finger position sensitivity and Schmitt trigger noise.
- Algorithm cannot detect rates above 100 BPM (limited by 600 ms refractory period).

This is not a clinically validated heart rate measurement. Known gaps:

- No motion artefact rejection - movement corrupts the PPG waveform
- No stabilisation window before reporting (first valid pair triggers output)
- Threshold derived from a rolling 25-sample min/max window, not an absolute calibrated reference
- Not validated against a reference pulse oximeter
- Schmitt trigger approach is unsuitable for clinical use without adaptive filtering

**Do not use the BPM output of this firmware for any medical decision.**

---

## Hardware Limitations

### FreeRTOS Task Scheduling - No Preemption Tuning

Phase 4 splits the main loop into two FreeRTOS tasks communicating via a depth-1 queue:
- `task_sensor` (priority 2): reads sensors, filters, detects BPM, kicks IWDG every 500 ms
- `task_uart` (priority 1): blocks on queue, formats and transmits UART output

Known gaps at this priority/configuration:
- Stack sizes are conservatively fixed; no runtime high-water mark monitoring
- No deadlock detection or task watchdog per task (only system-level IWDG)
- IWDG is kicked inside task_sensor only - a task_uart hang would not be caught
- FreeRTOS heap_4 allocator; heap exhaustion causes configASSERT halt, not graceful recovery

### No Power Management

The STM32L476 supports multiple low-power sleep modes (Sleep, Stop, Standby).
This project runs in full active mode at all times (4 MHz MSI clock).

Power consumption in active mode is approximately 1 mA at 4 MHz.
A production medical device would implement sleep modes between sensor reads
to reduce average consumption to microamps.

### Single I2C Bus, No Error Recovery

Both sensors share I2C1. If one sensor holds the SDA line LOW (a known failure mode),
the entire I2C bus is blocked and neither sensor can communicate.

No bus recovery procedure (9 clock pulses to unstick SDA) is implemented.
The current I2C driver uses bounded polling timeouts, so many runtime I2C
failures return an error-like zero value instead of blocking long enough for
the watchdog to fire. A hard SDA-low fault can still require a power cycle.

---

## Software Limitations

### No Input Validation

The firmware validates TMP117 and MAX30102 device IDs at startup. If either
sensor fails initialisation, the firmware stops before the measurement loop and
allows the IWDG to reset the MCU. Runtime sensor read failures are less
explicit: I2C read helpers return zero, which can be indistinguishable from a
valid zero-valued register unless the caller adds a separate health flag.

### Temperature Display Resolution

Temperature is printed with one decimal place in degrees Celsius.
The TMP117 has 0.0078°C resolution (16-bit), but this precision is truncated in the
UART output to keep the display simple.


### No Non-Volatile Storage

Sensor readings are transmitted over UART in real time. No data is stored on the device.
If the UART connection is interrupted, readings are lost.
An SD card or external EEPROM would be needed for data logging.

### No Timestamp on Readings

Each sensor reading is printed without a timestamp.
FreeRTOS owns SysTick at 1 kHz, but the timestamp is not included in the UART
output line format.
An RTC peripheral would be needed for wall-clock timestamps.

---

## Repository Limitations

### Datasheets

The following reference documents are tracked in `datasheets/` for local reference:

- `RM0351_reference_manual.pdf` — STM32L4 Reference Manual (STMicroelectronics)
- `STM32L476RG_datasheet.pdf` — STM32L476RG datasheet (STMicroelectronics)
- `UM1724_nucleo_user_manual.pdf` — Nucleo-64 User Manual (STMicroelectronics)
- `TMP117_datasheet.pdf` — TMP117 datasheet (Texas Instruments)
- `MAX30102_datasheet.pdf` — MAX30102 datasheet (Analog Devices / Maxim)

These documents are publicly available from their respective manufacturers and are
included for convenience. They remain the intellectual property of their publishers.

---

---

*Last updated: May 2026*
*Project: STM32 Vitals Monitor - Bare-Metal Firmware*
*Author: Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg*
