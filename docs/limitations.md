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

None of these are implemented here. The IR raw values printed by this firmware indicate
relative changes in light absorption, not an absolute SpO2 percentage.

**Do not use the output of this firmware for any medical decision.**

### BPM Not Clinically Validated

Phase 3 adds a dynamic-threshold rising-edge BPM detector operating on the filtered IR PPG signal.
The algorithm reports beats per minute once two valid threshold crossings are recorded,
with a 333 ms refractory period to suppress noise.

This is not a clinically validated heart rate measurement. Known gaps:

- No motion artefact rejection - movement corrupts the PPG waveform
- No stabilisation window before reporting (first valid pair triggers output)
- Threshold derived from a rolling 8-sample min/max window, not an absolute calibrated reference
- Not tested against a reference pulse oximeter

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
Recovery requires a power cycle.

---

## Software Limitations

### No Input Validation

The firmware does not validate sensor register values before using them.
An invalid device ID from TMP117 or MAX30102 will be printed but processing will continue
as if the value were valid. In a production system, invalid readings would trigger an error
handler and halt measurement.

### Integer Temperature Resolution

Temperature is printed as an integer in degrees Celsius.
The TMP117 has 0.0078°C resolution (16-bit), but this precision is truncated in the
current firmware to reduce output complexity.

Full resolution display is planned when the signal processing layer is added.

### No Non-Volatile Storage

Sensor readings are transmitted over UART in real time. No data is stored on the device.
If the UART connection is interrupted, readings are lost.
An SD card or external EEPROM would be needed for data logging.

### No Timestamp on Readings

Each sensor reading is printed without a timestamp.
Phase 2 added a calibrated SysTick 1 ms counter (get_tick()), but the timestamp
is not yet included in the UART output line format.
An RTC peripheral would be needed for wall-clock timestamps.

---

## Repository Limitations

### Datasheets Not Included for Redistribution

The STM32 Reference Manual (RM0351) and Nucleo User Manual (UM1724) are referenced
in this project but not redistributed in this repository. They are freely available
from STMicroelectronics at st.com.

The TMP117 and MAX30102 datasheets are referenced but not redistributed.
They are available from Texas Instruments and Analog Devices respectively.

---

---

*Last updated: May 2026*
*Project: STM32 Vitals Monitor - Bare-Metal Firmware*
*Author: Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg*
