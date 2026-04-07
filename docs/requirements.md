# Software Requirements Specification - STM32 Vitals Monitor

This document defines the requirements that the current firmware implements.
Each requirement has a unique ID so that test cases (TESTING.md) and design
decisions can reference them by ID rather than by description.

All requirements are derived from the implemented firmware. Nothing here is
aspirational. Items that are planned but not yet implemented are listed in
Section 5 (Out of Scope) and marked `[FUTURE]`.

---

## 1. Scope

Bare-metal firmware on an STM32L476RG (Cortex-M4, 4 MHz MSI clock). Reads
temperature from a TMP117 sensor and raw infrared (IR) data from a MAX30102
sensor over I2C. Applies a moving average filter to the IR data. Transmits
results over UART. All peripheral configuration is register-level - no HAL,
no CubeMX.

**Hardware:** STM32 Nucleo-L476RG · TMP117 breakout (I2C 0x49) · MAX30102
breakout (I2C 0x57) · shared I2C1 bus on PB8/PB9.

---

## 2. Functional Requirements

### 2.1 System Startup

| ID | Requirement |
|---|---|
| REQ-SYS-01 | The firmware shall initialise UART before any other peripheral. |
| REQ-SYS-02 | The firmware shall initialise I2C before attempting sensor communication. |
| REQ-SYS-03 | The firmware shall attempt to initialise TMP117 and MAX30102 after I2C is ready. |
| REQ-SYS-04 | The firmware shall report the result of each sensor initialisation over UART (`OK` or `FAIL`) before entering the measurement loop. |
| REQ-SYS-05 | The firmware shall enter the measurement loop regardless of sensor initialisation result. A failed sensor shall not halt the system. |
| REQ-SYS-06 | The firmware shall configure SysTick to generate an interrupt every 1 ms using the 4 MHz processor clock (reload value = 3999). The SysTick handler shall increment a `volatile uint32_t` tick counter on each interrupt. `systick_init()` shall be called before any other peripheral initialisation. |
| REQ-SYS-07 | The firmware shall enable the Independent Watchdog (IWDG) with a 4-second timeout. The IWDG shall be initialised after SysTick and before any other peripheral. The main loop shall reload the watchdog on every iteration. |

### 2.2 UART

| ID | Requirement |
|---|---|
| REQ-UART-01 | The firmware shall configure USART2 on pin PA2 as alternate function AF7 (USART2_TX). |
| REQ-UART-02 | USART2 shall operate at 9600 baud, 8 data bits, no parity, 1 stop bit (8N1), derived from a 4 MHz system clock (BRR = 417). |
| REQ-UART-03 | The transmitter (TE) shall be enabled before the USART peripheral (UE) is enabled. |
| REQ-UART-04 | The firmware shall provide functions to transmit: a single character, a null-terminated string, a signed 32-bit integer as decimal digits, and a single byte as two hex digits. |
| REQ-UART-05 | Each byte transmission shall poll the TXE flag (ISR bit 7) and wait until the transmit register is confirmed empty before writing. |

### 2.3 I2C

| ID | Requirement |
|---|---|
| REQ-I2C-01 | The firmware shall configure I2C1 on PB8 (SCL) and PB9 (SDA). |
| REQ-I2C-02 | PB8 and PB9 shall be configured as alternate function AF4, open-drain output type, very high speed. Open-drain is mandatory for I2C shared-bus operation. |
| REQ-I2C-03 | I2C1 shall operate in standard mode at 100 kHz. The timing register (TIMINGR) shall be set to `0x00100D14`, valid for a 4 MHz system clock. |
| REQ-I2C-04 | TIMINGR shall be written only while the I2C peripheral is disabled (CR1 PE bit = 0). |
| REQ-I2C-05 | The firmware shall provide three I2C transfer functions: single-byte register write, single-byte register read, two-byte register read. |
| REQ-I2C-06 | All I2C polling loops shall use a timeout counter of 50000 cycles. A loop that times out shall exit without hanging. |
| REQ-I2C-07 | All I2C sticky status flags shall be cleared by writing `0x3F38` to ICR before each transaction. |

### 2.4 TMP117 Temperature Sensor

| ID | Requirement |
|---|---|
| REQ-TMP-01 | On initialisation, the firmware shall read the TMP117 device ID register (0x0F) and compare it against the expected value `0x0117`. |
| REQ-TMP-02 | `tmp117_init()` shall return 1 if the device ID matches, 0 otherwise. |
| REQ-TMP-03 | The firmware shall read temperature by reading the TMP117 TEMP register (0x00) as a raw 16-bit signed value. |
| REQ-TMP-04 | Raw temperature shall be converted to Celsius × 10 using the approximation `(raw × 78) / 1000`, which implements the 0.0078125 °C/LSB conversion factor from the TMP117 datasheet. |
| REQ-TMP-05 | Temperature shall be formatted for output as integer degrees and one decimal digit separated by `.` (e.g., `23.4`). |

### 2.5 MAX30102 IR Sensor

| ID | Requirement |
|---|---|
| REQ-MAX-01 | On initialisation, the firmware shall read the MAX30102 Part ID register (0xFF) and compare it against the expected value `0x15`. |
| REQ-MAX-02 | `max30102_init()` shall return 1 if the Part ID matches, 0 otherwise. |
| REQ-MAX-03 | The firmware shall reset the MAX30102 by writing `0x40` to the MODE register (0x09) and waiting before applying further configuration. |
| REQ-MAX-04 | The firmware shall configure the MAX30102 FIFO with 4-sample averaging and rollover enabled: FIFO_CONFIG (0x08) = `0x4F`. |
| REQ-MAX-05 | The firmware shall configure MAX30102 in heart rate mode (IR LED only): MODE (0x09) = `0x02`. |
| REQ-MAX-06 | The firmware shall configure MAX30102 at 100 samples/second with 411 μs pulse width: SpO2 config (0x0A) = `0x27`. |
| REQ-MAX-07 | The IR LED current shall be set to 6.4 mA: LED1_PA (0x0C) = `0x1F`. |
| REQ-MAX-08 | The FIFO write pointer (0x04) and read pointer (0x06) shall be reset to 0 after configuration. |
| REQ-MAX-09 | On each measurement cycle, the firmware shall read 3 bytes from the FIFO data register (0x07) and reconstruct a 32-bit value (MSB first: byte0 << 16 | byte1 << 8 | byte2). |
| REQ-MAX-10 | Raw IR values shall be masked to 18-bit resolution: `result & 0x3FFFF`, per the MAX30102 datasheet. |

### 2.6 Moving Average Filter

| ID | Requirement |
|---|---|
| REQ-FIL-01 | The firmware shall apply a moving average filter to raw IR values before output. |
| REQ-FIL-02 | The filter window shall be 8 samples (`FILTER_WINDOW = 8` in filter.h). |
| REQ-FIL-03 | The filter shall use a circular buffer with a running sum, updating in O(1) per sample - no full recomputation of the sum on each call. |
| REQ-FIL-04 | On each update, the value being replaced shall be subtracted from the running sum before the new value is added. |
| REQ-FIL-05 | The returned average shall divide the running sum by the number of valid samples collected so far, not by `FILTER_WINDOW`, until the buffer is full. |
| REQ-FIL-06 | `filter_init()` shall zero all buffer slots, the write index, the count, and the running sum. |

### 2.7 BPM Detection

| ID | Requirement |
|---|---|
| REQ-SYS-08 | The firmware shall compute heart rate (BPM) from the filtered IR signal using a dynamic-threshold rising-edge detector. The threshold shall be the midpoint of the rolling min/max of the last 8 filtered IR samples. A crossing shall only be accepted if at least 333 ms have elapsed since the previous crossing (refractory period, enforcing a maximum of 180 BPM). BPM shall be calculated as `60000 / interval_ms` where `interval_ms` is the time between the two most recent valid crossings. The output shall display `---` until at least two valid crossings have been recorded. |

### 2.8 Measurement Output

| ID | Requirement |
|---|---|
| REQ-OUT-01 | Each measurement cycle shall output one row containing four fields: temperature (°C), raw IR value, filtered IR value, and BPM. BPM shall be shown as a decimal integer or `---` if not yet valid. |
| REQ-OUT-02 | Output columns shall be separated by ` \| ` delimiters, consistent with the header row transmitted at startup. |
| REQ-OUT-03 | Each output row shall be terminated with `\r\n`. |

### 2.9 RTOS Task Architecture

| ID | Requirement |
|---|---|
| REQ-RTOS-01 | The firmware shall run FreeRTOS V10.5.1 (vendored kernel source, MIT license). The kernel shall be compiled from source; no pre-built library shall be used. |
| REQ-RTOS-02 | The application shall be split into two FreeRTOS tasks: `task_sensor` (priority 2) and `task_uart` (priority 1). No bare `for(;;)` loop shall exist in `main()`. |
| REQ-RTOS-03 | `task_sensor` shall read sensors, apply the moving average filter, run the BPM detector, and send a `VitalsMsg` to a FreeRTOS queue every 500 ms using `vTaskDelay(pdMS_TO_TICKS(500U))`. |
| REQ-RTOS-04 | `task_uart` shall block on the queue using `xQueueReceive(..., portMAX_DELAY)` and transmit one UART row per message received. |
| REQ-RTOS-05 | The IWDG shall be kicked inside `task_sensor` after each sensor read. A hung sensor task shall cause a watchdog reset within the IWDG timeout window. |

---

## 3. Non-Functional Requirements

### 3.1 Memory Safety

| ID | Requirement |
|---|---|
| REQ-NF-01 | The firmware shall not use dynamic memory allocation. `malloc`, `calloc`, `free`, and equivalent functions shall not be called. All buffers shall be statically allocated at compile time. |
| REQ-NF-02 | The firmware shall not use recursive function calls. All repeated execution shall use iterative loops. |
| REQ-NF-03 | No function shall declare variable-length arrays (VLAs). All array sizes shall be compile-time constants. |

### 3.2 Hardware Access

| ID | Requirement |
|---|---|
| REQ-NF-04 | All hardware register accesses shall use `volatile uint32_t *` pointer casts. The `volatile` qualifier shall not be omitted on any register definition. |
| REQ-NF-05 | No STM32 HAL library or CubeMX-generated peripheral initialisation code shall be used. All peripheral configuration shall be performed by direct register writes derived from RM0351. |

### 3.3 Architecture

| ID | Requirement |
|---|---|
| REQ-NF-06 | The driver layer (uart, i2c, tmp117, max30102) shall not call any function defined in main.c. |
| REQ-NF-07 | main.c shall not directly read or write any hardware register. All hardware access shall be through driver-layer functions. |
| REQ-NF-08 | The processing layer (filter.c / filter.h) shall have no hardware dependencies. It shall contain no peripheral register addresses or STM32-specific includes. |

### 3.4 Clock

| ID | Requirement |
|---|---|
| REQ-NF-09 | The system clock shall use the MSI oscillator at the default power-on frequency of 4 MHz. No PLL configuration or external crystal is required. |

---

## 4. Constraints

| ID | Constraint | Source |
|---|---|---|
| CON-01 | Target MCU: STM32L476RGT6 · Cortex-M4 · 1 MB Flash · 128 KB SRAM | Hardware |
| CON-02 | Toolchain: STM32CubeIDE (GCC ARM), Debug build | Build environment |
| CON-03 | Serial terminal: CoolTerm · COM7 · 9600 baud · 8N1 · no flow control | Deployment |
| CON-04 | Both sensors share I2C1. No multi-master configuration. If one sensor holds SDA low, both are blocked. | Hardware |

---

## 5. Out of Scope

Items explicitly outside the scope of the current firmware. Not unmet requirements.

| Item | Status | Note |
|---|---|---|
| SpO2 (blood oxygen) calculation | Out of scope | Requires calibrated coefficients, validated algorithm, regulatory approval |
| Clinical heart rate (BPM) | Implemented | REQ-SYS-08 - dynamic threshold peak detector in bpm.c |
| FreeRTOS or any RTOS | Implemented | Phase 4 - see REQ-RTOS-01 through REQ-RTOS-05 |
| Low-power sleep modes (STOP2) | `[FUTURE]` | Planned extension phase |
| I2C bus recovery (9-clock unstick) | `[FUTURE]` | Planned extension phase |
| Non-volatile storage | Out of scope | No SD card or EEPROM on current hardware |
| Timestamps on readings | `[FUTURE]` | Requires RTC or UART timestamp from get_tick() |
| MISRA-C compliance assessment | `[FUTURE]` | See docs/misra.md when complete |

---

*Document version: 1.0*
*Firmware: post-refactor, 3-layer architecture*
*Author: Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg*
*Date: April 2026*
*References: RM0351 · TMP117 datasheet (Texas Instruments) · MAX30102 datasheet (Analog Devices)*
