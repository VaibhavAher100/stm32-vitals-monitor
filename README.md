# STM32 Vitals Monitor

Bare-metal firmware on STM32L476RG. No HAL. No CubeMX-generated peripheral init. All peripheral drivers written from RM0351 using CMSIS device headers (`stm32l476xx.h`). `syscalls.c` and `sysmem.c` are IDE-generated newlib stubs, retained unchanged.

TMP117 temperature + MAX30102 PPG/BPM on a shared I2C bus. Moving average filter on raw IR signal. BPM detection from threshold crossings. IWDG watchdog with LSI oscillator. FreeRTOS V10.5.1 two-task scheduler. 3-layer architecture (application / processing / driver). MISRA-C analysed with Cppcheck. IEC 62304 requirements traceability in source.

Hardware-verified: TMP117 25.3 C, MAX30102 IR ~87000 with finger contact, BPM 119 stable over 50+ rows, zero IWDG resets during run.

---

## How it was built

Four phases. The commit history preserves the full evolution.

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
├── main.c          application  - RTOS init; creates queue, spawns task_sensor + task_uart
├── tasks_vitals.c  application  - task_sensor (sensors + filter + BPM + IWDG) and task_uart (queue receive + UART format)
├── filter.c/h      processing   - moving average, 8-sample circular buffer
├── bpm.c/h         processing   - BPM detection, dynamic threshold, 333 ms refractory
├── uart.c/h        driver       - USART2
├── i2c.c/h         driver       - I2C1
├── iwdg.c/h        driver       - IWDG watchdog, LSI oscillator
├── tmp117.c/h      driver       - TMP117 register read
└── max30102.c/h    driver       - MAX30102 FIFO init + read
```

Rules: application never touches registers. Driver never calls application. No heap allocation in project code (FreeRTOS heap_4 used by kernel only). `syscalls.c`/`sysmem.c` are newlib stubs from STM32CubeIDE - not project code.

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
STM32 Vitals Monitor
========================
Temp(C) | IR raw  | IR filt | BPM
--------+---------+---------+----
TMP117   OK
MAX30102 OK
========================
  25.3  |     778 |     778 | ---
  25.3  |     780 |     779 | ---
  25.3  |   62657 |    8615 | 119
  25.3  |   70174 |   17288 | 119
  25.3  |   80963 |   35808 | 119
  25.3  |   87266 |   46619 | 119
  25.3  |   87750 |   79062 | 119
  25.3  |   87543 |   87611 | 119
  25.3  |   87352 |   87419 | 119
  25.3  |    1077 |   11502 | 119
  25.3  |    1071 |    1074 | 119
```

IR raw: ~775 ambient, ~87000 with finger on sensor. BPM 119 verified on hardware. Filter window is 8 samples - ramp-up visible in rows 3-8, decay after removal.

---

## Register map

| Register     | CMSIS access       | Used for |
|--------------|--------------------|----------|
| RCC_AHB2ENR  | `RCC->AHB2ENR`     | GPIOA clock, GPIOB clock |
| RCC_APB1ENR1 | `RCC->APB1ENR1`    | USART2 (bit 17), I2C1 (bit 21) |
| RCC_CSR      | `RCC->CSR`         | LSI oscillator enable + ready flag |
| GPIOA_MODER  | `GPIOA->MODER`     | PA2 as AF (bits 5:4) |
| GPIOA_AFR[0] | `GPIOA->AFR[0]`    | PA2 - AF7 = USART2_TX |
| USART2_BRR   | `USART2->BRR`      | 417 = 9600 baud at 4 MHz |
| GPIOB_OTYPER | `GPIOB->OTYPER`    | PB8, PB9 open-drain - I2C requires this |
| GPIOB_AFR[1] | `GPIOB->AFR[1]`    | PB8, PB9 - AF4 = I2C1 |
| I2C1_TIMINGR | `I2C1->TIMINGR`    | 0x00100D14 - 100 kHz at 4 MHz |
| IWDG_KR      | `IWDG->KR`         | 0xCCCC start, 0x5555 unlock, 0xAAAA kick |
| IWDG_PR      | `IWDG->PR`         | Prescaler /32 |
| IWDG_RLR     | `IWDG->RLR`        | Reload 4000 = 4 s timeout at 32 kHz LSI / 32 |

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

1. Open `firmware/` in STM32CubeIDE as the workspace - the `Core` project loads automatically
2. Ctrl+B (CMSIS include paths are pre-configured in `.cproject`)
3. Drag `firmware/Core/Debug/Core.bin` onto `NODE_L476RG`
4. CoolTerm - COM7, 9600 baud, 8N1, Flow Control: None
5. Press RESET

---

*Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg - May 2026*
