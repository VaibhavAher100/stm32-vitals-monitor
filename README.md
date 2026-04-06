# STM32 Vitals Monitor

Bare-metal firmware on STM32L476RG. No HAL. No CubeMX.

Registers written directly. Addresses from RM0351.
TMP117 temperature and MAX30102 IR on the same I2C bus.
Moving average filter on raw IR. 3-layer architecture.

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
├── main.c          application - loop, UART output, no register access
├── filter.c/h      processing  - moving average, circular buffer
├── uart.c/h        driver      - USART2
├── i2c.c/h         driver      - I2C1
├── tmp117.c/h      driver      - TMP117 read
└── max30102.c/h    driver      - MAX30102 init + FIFO read
```

Rules: application never touches registers. driver never calls application. nothing uses malloc.

The Git history shows the prototype phase (monolithic main.c) followed by the refactor commit. Both are intentional.

---

## Output

```
========================
STM32 Vitals Monitor
========================
TMP117  OK
MAX30102 OK
========================
Temp(C) | IR raw  | IR filt
--------+---------+--------
23.4       | 719    | 719
23.4       | 712    | 715
23.5       | 88991  | 11752   <- finger contact
23.5       | 92885  | 55903
23.5       | 92344  | 89113
23.5       | 730    | 78074   <- finger removed, filter decays
23.5       | 715    | 719
```

IR values: ~700 ambient, 80000-92000 with finger on sensor.

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

- No clinical accuracy - raw sensor values only, not validated readings
- No power management - runs in full active mode
- No non-volatile storage - readings lost if UART disconnects

See `docs/limitations.md` for the full list.

---

## Build

Prerequisites: STM32CubeIDE 2.7 or later

1. Open `firmware/Core/` as an existing project in STM32CubeIDE
2. Ctrl+B to build
3. Drag `firmware/Core/Debug/Core.bin` onto the NODE_L476RG drive
4. Open a serial terminal (CoolTerm, PuTTY, etc.) on the port assigned to
   your Nucleo board - 9600 baud, 8N1, no flow control
5. Press RESET on the board

---

*Vaibhav Aher - M.Sc. ICT, FAU Erlangen - March 2026*
