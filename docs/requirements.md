# Requirements - STM32 Vitals Monitor

Numbered requirements derived from the implemented firmware. IDs are used in
testing.md (verification) and in source file traceability comments.

Nothing here is aspirational - every requirement reflects code that exists and
has been verified. Planned items are in Section 5 marked `[FUTURE]`.

---

## 1. Scope

Bare-metal firmware on an STM32L476RG (Cortex-M4, 4 MHz MSI clock). Reads
temperature from a TMP117 sensor and raw infrared (IR) data from a MAX30102
sensor over I2C. Applies a moving average filter to the IR data. Detects
heart rate (BPM) from filtered PPG signal. Runs two FreeRTOS tasks communicating
via a queue. IWDG watchdog provides automatic reset on task hang. Transmits
results over UART every 500 ms. All peripheral configuration is register-level
via CMSIS device headers - no HAL, no CubeMX.

**Hardware:** STM32 Nucleo-L476RG - TMP117 breakout (I2C 0x49) - MAX30102
breakout (I2C 0x57) - shared I2C1 bus on PB8/PB9.

---

## 2. Functional Requirements

### 2.1 System Startup

| ID | Requirement |
|---|---|
| REQ-SYS-01 | The firmware shall initialise IWDG, UART, and I2C before starting the FreeRTOS scheduler. |
| REQ-SYS-02 | The firmware shall initialise I2C before any sensor communication attempt. |
| REQ-SYS-03 | Sensor initialisation (TMP117, MAX30102) shall run inside task_sensor after the scheduler starts, so vTaskDelay is available during init. |
| REQ-SYS-04 | The firmware shall report the result of each sensor initialisation over UART (`OK` or `FAIL`) before entering the measurement loop. |
| REQ-SYS-05 | The firmware shall enter the measurement loop regardless of sensor initialisation result. A failed sensor shall not halt the system. |

### 2.2 UART

| ID | Requirement |
|---|---|
| REQ-UART-01 | The firmware shall configure USART2 on pin PA2 as alternate function AF7 (USART2_TX). |
| REQ-UART-02 | USART2 shall operate at 9600 baud, 8 data bits, no parity, 1 stop bit (8N1), derived from a 4 MHz system clock (BRR = 417). |
| REQ-UART-03 | The transmitter (TE) shall be enabled before the USART peripheral (UE) is enabled. |
| REQ-UART-04 | The firmware shall provide functions to transmit: a single character, a null-terminated string, and a signed 32-bit integer as decimal digits. |
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
| REQ-TMP-04 | Raw temperature shall be converted to Celsius x 10 using the approximation `(raw * 78) / 1000`, which implements the 0.0078125 C/LSB conversion factor from the TMP117 datasheet. |
| REQ-TMP-05 | Temperature shall be formatted for output as integer degrees and one decimal digit separated by `.` (e.g., `23.4`). |

### 2.5 MAX30102 IR Sensor

| ID | Requirement |
|---|---|
| REQ-MAX-01 | On initialisation, the firmware shall read the MAX30102 Part ID register (0xFF) and compare it against the expected value `0x15`. |
| REQ-MAX-02 | `max30102_init()` shall return 1 if the Part ID matches, 0 otherwise. |
| REQ-MAX-03 | The firmware shall reset the MAX30102 by writing `0x40` to the MODE register (0x09) and waiting before applying further configuration. |
| REQ-MAX-04 | The firmware shall configure the MAX30102 FIFO with 4-sample averaging and rollover enabled: FIFO_CONFIG (0x08) = `0x4F`. |
| REQ-MAX-05 | The firmware shall configure MAX30102 in heart rate mode (IR LED only): MODE (0x09) = `0x02`. |
| REQ-MAX-06 | The firmware shall configure MAX30102 at 100 samples/second with 411 us pulse width: SpO2 config (0x0A) = `0x27`. |
| REQ-MAX-07 | The IR LED current shall be set to 6.4 mA: LED1_PA (0x0C) = `0x1F`. |
| REQ-MAX-08 | The FIFO write pointer (0x04) and read pointer (0x06) shall be reset to 0 after configuration. |
| REQ-MAX-09 | On each measurement cycle, the firmware shall read 3 bytes from the FIFO data register (0x07) and reconstruct a 32-bit value (MSB first: byte0 << 16 | byte1 << 8 | byte2). |
| REQ-MAX-10 | Raw IR values shall be masked to 18-bit resolution: `result & 0x3FFFF`, per the MAX30102 datasheet. |

### 2.6 Moving Average Filter

| ID | Requirement |
|---|---|
| REQ-FIL-01 | The firmware shall apply a moving average filter to raw IR values before BPM detection and output. |
| REQ-FIL-02 | The filter window shall be 8 samples (`FILTER_WINDOW = 8` in filter.h). |
| REQ-FIL-03 | The filter shall use a circular buffer with a running sum, updating in O(1) per sample - no full recomputation of the sum on each call. |
| REQ-FIL-04 | On each update, the value being replaced shall be subtracted from the running sum before the new value is added. |
| REQ-FIL-05 | The returned average shall divide the running sum by the number of valid samples collected so far, not by `FILTER_WINDOW`, until the buffer is full. |
| REQ-FIL-06 | `filter_init()` shall zero all buffer slots, the write index, the count, and the running sum. |

### 2.7 BPM Detection

| ID | Requirement |
|---|---|
| REQ-BPM-01 | BPM detection shall operate on filtered IR values from filter.c, not raw IR. |
| REQ-BPM-02 | The dynamic threshold shall be computed as the midpoint of a rolling min/max window over the last BPM_HISTORY (8) filtered samples. |
| REQ-BPM-03 | A valid beat shall be a rising-edge threshold crossing: previous filtered sample below threshold AND current sample at or above threshold. |
| REQ-BPM-04 | Consecutive crossings within BPM_REFRACTORY_MS (333 ms) shall be rejected. |
| REQ-BPM-05 | BPM shall be computed as 60000 / interval_ms from the most recent valid crossing pair. `bpm_get()` shall return BPM_INVALID until at least two valid crossings are recorded. |

### 2.8 FreeRTOS Task Architecture

| ID | Requirement |
|---|---|
| REQ-RTOS-01 | The firmware shall use FreeRTOS V10.5.1 to implement a two-task architecture. |
| REQ-RTOS-02 | task_sensor (priority 2) shall read TMP117 and MAX30102, apply the IR filter, detect BPM, kick the IWDG, and post a VitalsMsg to vitals_queue every 500 ms using `vTaskDelay(pdMS_TO_TICKS(500U))`. |
| REQ-RTOS-03 | task_uart (priority 1) shall block on vitals_queue with `portMAX_DELAY` and format + transmit one UART output row per received VitalsMsg. |
| REQ-RTOS-04 | Inter-task data shall pass exclusively through vitals_queue (depth 1, xQueueOverwrite). No shared globals shall carry measurement data between tasks. |
| REQ-RTOS-05 | vitals_queue creation, IWDG init, UART init, and I2C init shall complete before `vTaskStartScheduler()`. Sensor init runs inside task_sensor after the scheduler starts. |

### 2.9 IWDG Watchdog

| ID | Requirement |
|---|---|
| REQ-WDG-01 | The IWDG shall be configured with LSI at approximately 32 kHz, prescaler /32, reload 4000, giving a 4-second timeout. LSI must be confirmed ready (RCC_CSR_LSIRDY) before proceeding. |
| REQ-WDG-02 | task_sensor shall call `iwdg_kick()` after each successful measurement cycle. A hang in task_sensor longer than 4 seconds shall trigger an automatic MCU reset. |

### 2.10 Measurement Output

| ID | Requirement |
|---|---|
| REQ-OUT-01 | Each measurement cycle shall output one row containing four fields: temperature (C), raw IR value, filtered IR value, BPM. |
| REQ-OUT-02 | Output columns shall be separated by ` \| ` delimiters, consistent with the header row transmitted at startup. |
| REQ-OUT-03 | Each output row shall be terminated with `\r\n`. |
| REQ-OUT-04 | When BPM has not yet reached a valid reading (fewer than two crossings), the BPM field shall output `---`. |

### 2.11 Timing

| ID | Requirement |
|---|---|
| REQ-STK-01 | SysTick shall be managed by FreeRTOS (configTICK_RATE_HZ = 1000, giving 1 ms/tick). Project code shall not configure SysTick directly. |
| REQ-STK-02 | BPM inter-beat interval timing shall use `xTaskGetTickCount()`, which provides 1 ms resolution. Unsigned subtraction handles tick counter rollover. |

---

## 3. Non-Functional Requirements

### 3.1 Memory Safety

| ID | Requirement |
|---|---|
| REQ-NF-01 | Project code shall not use dynamic memory allocation. `malloc`, `calloc`, `free`, and equivalent functions shall not be called in project source files. FreeRTOS heap_4 is present for kernel use only. |
| REQ-NF-02 | The firmware shall not use recursive function calls. All repeated execution shall use iterative loops. |
| REQ-NF-03 | No function shall declare variable-length arrays (VLAs). All array sizes shall be compile-time constants. |

### 3.2 Hardware Access

| ID | Requirement |
|---|---|
| REQ-NF-04 | All hardware register accesses shall use CMSIS peripheral struct pointers from `stm32l476xx.h` (e.g., `RCC->APB1ENR1`, `IWDG->KR`). Direct volatile pointer casts to fixed addresses shall not be used. |
| REQ-NF-05 | No STM32 HAL library or CubeMX-generated peripheral initialisation code shall be used. All peripheral configuration shall be performed by direct register writes derived from RM0351. |

### 3.3 Architecture

| ID | Requirement |
|---|---|
| REQ-NF-06 | The driver layer (uart, i2c, tmp117, max30102, iwdg) shall not call any function defined in main.c or tasks_vitals.c. |
| REQ-NF-07 | main.c and tasks_vitals.c shall not directly read or write any hardware register. All hardware access shall be through driver-layer functions. |
| REQ-NF-08 | The processing layer (filter.c, bpm.c) shall have no hardware dependencies. It shall contain no peripheral register addresses or STM32-specific includes, except where FreeRTOS tick access is required (bpm.c uses xTaskGetTickCount). |

### 3.4 Clock

| ID | Requirement |
|---|---|
| REQ-NF-09 | The system clock shall use the MSI oscillator at the default power-on frequency of 4 MHz. No PLL configuration or external crystal is required. |

---

## 4. Constraints

| ID | Constraint | Source |
|---|---|---|
| CON-01 | Target MCU: STM32L476RGT6 - Cortex-M4 - 1 MB Flash - 128 KB SRAM | Hardware |
| CON-02 | Toolchain: STM32CubeIDE (GCC ARM 14.3.1), Debug build, -Wall | Build environment |
| CON-03 | Serial terminal: CoolTerm - COM7 - 9600 baud - 8N1 - no flow control | Deployment |
| CON-04 | Both sensors share I2C1. No multi-master configuration. If one sensor holds SDA low, both are blocked. | Hardware |
| CON-05 | FreeRTOS V10.5.1, ARM_CM4F port, heap_4, configTICK_RATE_HZ = 1000 | RTOS |

---

## 5. Out of Scope

Items explicitly outside the scope of the current firmware. Not unmet requirements.

| Item | Status | Note |
|---|---|---|
| SpO2 (blood oxygen) calculation | Out of scope | Requires calibrated coefficients, validated algorithm, regulatory approval |
| Clinical heart rate validation | Out of scope | BPM output not tested against certified reference; not for medical use |
| STOP2 low-power sleep modes | `[FUTURE]` | Planned extension phase |
| BLE wireless streaming | `[FUTURE]` | Planned via ESP32 co-processor |
| I2C bus recovery (9-clock unstick) | `[FUTURE]` | Current recovery: IWDG reset after 4 s |
| Unity unit tests for filter.c / bpm.c | Done | `tests/` - 15 automated tests, host-runnable |
| Non-volatile storage | Out of scope | No SD card or EEPROM on current hardware |
| Wall-clock timestamps on readings | Out of scope | Would require RTC peripheral |

---

*v2.0 - May 2026*
*Author: Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg*
