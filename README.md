# STM32 Vitals Monitor

Bare-metal firmware on STM32L476RG. No HAL. No CubeMX.

Registers written directly. Addresses from RM0351.
TMP117 temperature and MAX30102 IR on the same I2C bus.
Moving average filter on raw IR. BPM detection from PPG signal.
IWDG watchdog. FreeRTOS V10.5.1 single-task scheduler. 3-layer architecture.

---

## Hardware

- STM32 Nucleo-L476RG (STM32L476RGT6, Cortex-M4)
- TMP117 breakout - Soldered 333175 - I2C 0x49
- AEDIKO MAX30102 breakout - I2C 0x57

Both sensors share I2C1 on PB8 (SCL) / PB9 (SDA).

---

## Wiring

| Signal | Nucleo | Header |
|---|---|---|
| 3.3V | +3V3 | CN6 pin 4 |
| GND | GND | CN6 pin 6 |
| SCL | PB8 | CN5 pin 10 |
| SDA | PB9 | CN5 pin 9 |

Both sensors connect to the same four pins via breadboard rails.

---

## Architecture

```
Src/
├── main.c          application - FreeRTOS task_vitals, UART output, no register access
├── filter.c/h      processing  - moving average, circular buffer
├── bpm.c/h         processing  - BPM detection, threshold crossings on filtered PPG
├── uart.c/h        driver      - USART2
├── i2c.c/h         driver      - I2C1
├── iwdg.c/h        driver      - IWDG watchdog, LSI oscillator
├── tmp117.c/h      driver      - TMP117 read
└── max30102.c/h    driver      - MAX30102 init + FIFO read
```

Rules: application never touches registers. driver never calls application. nothing uses malloc.

The Git history shows the full evolution: prototype (monolithic main.c), 3-layer refactor, BPM signal processing, FreeRTOS integration.

---

## Output

```
========================
STM32 Vitals Monitor
========================
TMP117  OK
MAX30102 OK
========================
Temp(C) | IR raw  | IR filt | BPM
--------+---------+---------+----
24.8       | 860    | 860   | ---
24.8       | 857    | 858   | ---    <- BPM shows --- until 2 crossings
24.8       | 95865  | 27233 | ---    <- finger contact
24.8       | 96977  | 51250 | 111
24.8       | 97259  | 75353 | 111
24.8       | 97398  | 93215 | 111
```

IR values: ~857 ambient, ~97000 with finger on sensor. BPM verified at 111.

---

## The bug that cost the most time

USART2_BRR is at offset `0x0C` - address `0x4000440C`.
`0x40004408` is CR3. Writing baud rate to CR3 does nothing and throws no error.
RM0351 section 40.8.4.

---

## Register map

| Register | Address | Used for |
|---|---|---|
| RCC_AHB2ENR | 0x4002104C | GPIOA clock (bit 0), GPIOB clock (bit 1) |
| RCC_APB1ENR1 | 0x40021058 | USART2 (bit 17), I2C1 (bit 21) |
| GPIOA_MODER | 0x48000000 | PA2 as AF (bits 5:4), PA5 as output (bits 11:10) |
| GPIOA_AFRL | 0x48000020 | PA2 - AF7 = USART2_TX |
| USART2_BRR | 0x4000440C | 417 = 9600 baud at 4MHz |
| GPIOB_OTYPER | 0x48000404 | PB8, PB9 open-drain - I2C requires this |
| GPIOB_AFRH | 0x48000424 | PB8, PB9 - AF4 = I2C1 |
| I2C1_TIMINGR | 0x40005410 | 0x00100D14 - 100kHz at 4MHz |

Full detail: `docs/registers.md`

---

## Limitations

- BPM is computed from PPG signal crossings only - not a medically validated reading
- I2C transactions use timeout busy-loops - a bus lockup still needs a hard reset (IWDG covers this)
- Single task architecture - no concurrent sensor reads

See `docs/limitations.md` for the full list.

---

## Build

1. Open `firmware/` in STM32CubeIDE
2. Ctrl+B
3. Drag `firmware/Debug/firmware.bin` onto NODE_L476RG
4. CoolTerm - COM7, 9600, 8N1, Flow Control: None
5. Press RESET

---

*Vaibhav Aher - M.Sc. ICT, FAU Erlangen - April 2026*
