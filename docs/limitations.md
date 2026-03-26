# Limitations — STM32 Vitals Monitor

This document records all known limitations, constraints, and design boundaries of this project.
Acknowledging limitations is a mark of engineering rigour, not weakness.
Every limitation listed here was a deliberate scope decision, not an oversight.

---

## Medical and Clinical Limitations

### No Clinical SpO2 Accuracy

**This project does not produce clinically valid SpO2 (blood oxygen saturation) readings.**

The MAX30102 sensor reads raw photoplethysmographic (PPG) data — the change in light absorption
caused by pulsatile blood flow. Converting raw PPG data to a calibrated SpO2 percentage requires:

- Empirical calibration coefficients derived from measurements on human subjects
- Validated algorithms (typically proprietary, e.g., Maxim's algorithm)
- Regulatory approval (FDA clearance or CE marking) for medical use
- Controlled placement on the body (fingertip, earlobe, or wrist)

None of these are implemented here. The IR raw values printed by this firmware indicate
relative changes in light absorption, not an absolute SpO2 percentage.

**Do not use the output of this firmware for any medical decision.**

### No Clinical Heart Rate Accuracy

The firmware reads raw IR FIFO data from the MAX30102. A validated heart rate requires:

- Peak detection algorithm on the PPG waveform
- Motion artefact rejection
- Stabilisation period before reporting

Raw IR values are a prerequisite for heart rate calculation, not the result.

---

## Hardware Limitations

### No Real-Time Operating System (RTOS)

This project runs on bare-metal firmware with no RTOS scheduler.
Tasks execute sequentially in a single main loop.

Implications:
- Sensor reads block the loop — if a sensor hangs, the entire system halts
- No guaranteed timing between sensor reads
- No concurrent task execution

A FreeRTOS extension is planned as a future phase of this project to address these constraints.

### Delay Function is Not Calibrated

The `delay()` function uses a simple count-down loop:
```c
void delay(volatile uint32_t count) { while (count--); }
```

This is not a calibrated timer. The actual delay duration depends on:
- Compiler optimisation level
- Instruction execution time at the current clock frequency
- Other instructions executing in the same loop

For precise timing, the STM32 SysTick or TIM peripheral should be used.
This is marked as a future improvement.

### No Power Management

The STM32L476 supports multiple low-power sleep modes (Sleep, Stop, Standby).
This project runs in full active mode at all times (4 MHz MSI clock).

Power consumption in active mode is approximately 1 mA at 4 MHz.
A production medical device would implement sleep modes between sensor reads
to reduce average consumption to microamps.

### No Watchdog Timer

A watchdog timer (IWDG or WWDG) is not implemented.
If the firmware hangs — for example, due to an I2C bus lock-up — the system will not
automatically recover. A hard reset (RESET button) is required.

Watchdog implementation is planned for a future phase.

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
Accurate timestamps require either a Real-Time Clock (RTC) peripheral
or a calibrated SysTick counter. Neither is implemented in the current phase.

---

## Repository Limitations

### Datasheets Not Included for Redistribution

The STM32 Reference Manual (RM0351) and Nucleo User Manual (UM1724) are referenced
in this project but not redistributed in this repository. They are freely available
from STMicroelectronics at st.com.

The TMP117 and MAX30102 datasheets are referenced but not redistributed.
They are available from Texas Instruments and Analog Devices respectively.

---

## Out of Scope (Deliberate)

The following are explicitly out of scope for this project and are not treated as limitations:

| Item | Reason Out of Scope |
|---|---|
| BLE or WiFi transmission | Would require additional hardware and significantly increase project scope |
| Display output (OLED/LCD) | UART terminal is sufficient for demonstrating sensor functionality |
| PCB design | Breadboard prototype is appropriate for a portfolio project at this stage |
| Production enclosure | Physical packaging is not relevant to the firmware demonstration objective |
| Battery management | Mains-connected via USB throughout development |
| IEC 62304 compliance | This is a learning and portfolio project, not a regulated medical device |

---

*Last updated: March 2026*
*Project: STM32 Vitals Monitor — Bare-Metal Firmware*
*Author: Vaibhav Aher — M.Sc. ICT, FAU Erlangen-Nürnberg*
