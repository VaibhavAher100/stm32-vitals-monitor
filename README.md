# STM32 Vitals Monitor

Bare-metal firmware on STM32L476RG. No HAL. No CubeMX. Registers written directly from RM0351.

TMP117 temperature + MAX30102 PPG/BPM on a shared I2C bus. Moving average filter on raw IR signal. BPM detection from threshold crossings. IWDG watchdog with LSI oscillator. FreeRTOS V10.5.1 two-task scheduler. 3-layer architecture (application / processing / driver). MISRA-C analysed with Cppcheck.

Hardware-verified: TMP117 24.8 C, MAX30102 IR 97000+ with finger contact, BPM 111, 8-minute run with zero watchdog resets.

---

## How it was built

This project was built in four phases. Each phase is a separate branch and PR in the Git history - the progression is the point.

**Phase 1 - Foundation**
UART driver (USART2, 9600 baud), GPIO output, SysTick 1 ms counter. First characters on a terminal from a clean register state. No libraries, no scaffolding.

**Phase 2 - Sensor drivers**
I2C1 driver (PB8/PB9 open-drain, AF4, 100 kHz). TMP117 register read. MAX30102 FIFO init and read. Both sensors on the same bus. 3-layer refactor: application never touches registers, drivers never call application, no malloc.

**Phase 3 - Signal processing**
8-sample circular buffer moving average on raw IR. Dynamic-threshold rising-edge BPM detector with 333 ms refractory period. BPM column added to UART output. MISRA-C static analysis pass with Cppcheck.

**Phase 4 - FreeRTOS + IWDG**
FreeRTOS V10.5.1 (ARM_CM4F port). Two tasks: `task_sensor` (priority 2) reads sensors, filters, detects BPM, kicks IWDG every 500 ms. `task_uart` (priority 1) blocks on a depth-1 queue and formats output. IWDG watchdog with 4 s timeout - covers bus lockup and task hangs. Hardware verified on Nucleo-L476RG.

---

## Architecture

```
firmware/Core/Src/
├── main.c          application  - FreeRTOS task_sensor + task_uart, queue, no register access
├── filter.c/h      processing   - moving average, 8-sample circular buffer
├── bpm.c/h         processing   - BPM detection, dynamic threshold, 333 ms refractory
├── uart.c/h        driver       - USART2
├── i2c.c/h         driver       - I2C1
├── iwdg.c/h        driver       - IWDG watchdog, LSI oscillator
├── tmp117.c/h      driver       - TMP117 register read
└── max30102.c/h    driver       - MAX30102 FIFO init + read
```

Rules: application never touches registers. Driver never calls application. Nothing uses malloc.

---

## Hardware

- STM32 Nucleo-L476RG (STM32L476RGT6, Cortex-M4)
- TMP117 breakout (Soldered 333175) - I2C address 0x49
- AEDIKO MAX30102 breakout - I2C address 0x57

Both sensors share I2C1 on PB8 (SCL) / PB9 (SDA) via breadboard rails.

---

## Wiring

| Signal | Nucleo pin | Header |
|--------|------------|--------|
| 3.3V   | +3V3       | CN6 pin 4 |
| GND    | GND        | CN6 pin 6 |
| SCL    | PB8        | CN5 pin 10 |
| SDA    | PB9        | CN5 pin 9 |

---

## Output

```
========================
STM32 Vitals Monitor
========================
TMP117   OK
MAX30102 OK
========================
Temp(C) | IR raw  | IR filt | BPM
--------+---------+---------+----
  24.8  |     860 |     860 | ---
  24.8  |     857 |     858 | ---    <- BPM shows --- until 2 crossings detected
  24.8  |   95865 |   27233 | ---    <- finger contact, filter climbing
  24.8  |   96977 |   51250 | 111
  24.8  |   97259 |   75353 | 111
  24.8  |   97398 |   93215 | 111
```

IR raw: ~857 ambient, ~97000 with finger on sensor. BPM 111 verified on hardware. Filter decay visible across rows 1-2 before finger contact.

---

## Bugs worth noting

**USART2_BRR offset**

`USART2_BRR` is at offset `0x0C` - address `0x4000440C`.
`0x40004408` is `CR3`. Writing baud rate to `CR3` does nothing and produces no error.
Took an afternoon to find. RM0351 section 40.8.4.

**IWDG start sequence**

LSI oscillator must be enabled before writing any IWDG registers. Then `0xCCCC` (start) must be written to `IWDG_KR` before `0x5555` (unlock for prescaler/reload access). If the sequence is wrong, `PVU` and `RVU` flags in `IWDG_SR` never clear and the watchdog never arms. Datasheet says write `0xCCCC` first - most example code skips LSI and gets away with it at reset, but not after a soft reboot.

---

## Register map

| Register       | Address      | Used for |
|----------------|--------------|----------|
| RCC_AHB2ENR    | 0x4002104C   | GPIOA clock (bit 0), GPIOB clock (bit 1) |
| RCC_APB1ENR1   | 0x40021058   | USART2 (bit 17), I2C1 (bit 21) |
| RCC_CSR        | 0x40021098   | LSI oscillator enable (bit 0), ready flag (bit 1) |
| GPIOA_MODER    | 0x48000000   | PA2 as AF (bits 5:4), PA5 as output (bits 11:10) |
| GPIOA_AFRL     | 0x48000020   | PA2 - AF7 = USART2_TX |
| USART2_BRR     | 0x4000440C   | 417 = 9600 baud at 4 MHz |
| GPIOB_OTYPER   | 0x48000404   | PB8, PB9 open-drain - I2C requires this |
| GPIOB_AFRH     | 0x48000424   | PB8, PB9 - AF4 = I2C1 |
| I2C1_TIMINGR   | 0x40005410   | 0x00100D14 - 100 kHz at 4 MHz |
| IWDG_KR        | 0x40003000   | 0xCCCC start, 0x5555 unlock, 0xAAAA kick |
| IWDG_PR        | 0x40003004   | Prescaler /256 (bits 2:0 = 0x6) |
| IWDG_RLR       | 0x40003008   | Reload value 624 = ~4 s timeout at 40 kHz LSI / 256 |

Full detail: `docs/registers.md`

---

## Limitations

- BPM uses PPG threshold crossings only - not a clinically validated reading
- I2C uses timeout busy-loops - a bus lockup triggers IWDG reset (4 s)
- No motion artefact rejection on PPG signal
- FreeRTOS heap_4 allocator - heap exhaustion halts via configASSERT, no graceful recovery
- IWDG kicked only inside task_sensor - a task_uart hang alone would not be caught before 4 s timeout

See `docs/limitations.md` for the full list.

---

## Build

1. Open `firmware/` in STM32CubeIDE
2. Ctrl+B
3. Drag `firmware/Debug/firmware.bin` onto `NODE_L476RG`
4. CoolTerm - COM7, 9600 baud, 8N1, Flow Control: None
5. Press RESET

---

*Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg - April 2026*
