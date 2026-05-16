# STM32 Vitals Monitor

Bare-metal firmware on STM32L476RG. No HAL. No CubeMX-generated peripheral init. All peripheral drivers written from RM0351 using CMSIS device headers (`stm32l476xx.h`). `syscalls.c` and `sysmem.c` are IDE-generated newlib stubs, retained unchanged.

TMP117 temperature + MAX30102 red PPG/BPM on a shared I2C bus. Moving average filter on the AC PPG signal. BPM detection from dynamic-threshold crossings. IWDG watchdog with LSI oscillator. FreeRTOS V10.5.1 two-task scheduler. 3-layer architecture (application / processing / driver). MISRA-C analysed with Cppcheck. IEC 62304-style requirements traceability in source.

Hardware-verified: TMP117 25.6-26.2 C, MAX30102 raw PPG ~197,000 with finger contact (12.6 mA LED), BPM 64-97 range at rest, zero IWDG resets during run.

---

## How it was built

Four phases. The commit history preserves the full evolution.

**Phase 1 - Foundation**
UART driver (USART2, 9600 baud), GPIO output, SysTick 1 ms counter. First characters on a terminal from a clean register state. No libraries, no scaffolding.

**Phase 2 - Sensor drivers**
I2C1 driver (PB8/PB9 open-drain, AF4, 100 kHz). TMP117 register read. MAX30102 FIFO init and read. Both sensors on the same bus. 3-layer refactor: application never touches registers, drivers never call application, no malloc.

**Phase 3 - Signal processing**
4-sample circular buffer moving average on the centered AC PPG signal. Dynamic-threshold rising-edge BPM detector with 600 ms refractory period. Per-sample timestamps for accurate inter-beat intervals. BPM column added to UART output. MISRA-C static analysis pass with Cppcheck.

**Phase 4 - FreeRTOS + IWDG**
FreeRTOS V10.5.1 (ARM_CM4F port). Two tasks: `task_sensor` (priority 2) reads sensors, filters, detects BPM, kicks IWDG every 500 ms. `task_uart` (priority 1) blocks on a depth-1 queue and formats output. IWDG watchdog with 4 s timeout - covers startup failures and task hangs. Hardware verified on Nucleo-L476RG.

---

## Architecture

```
firmware/Core/Src/
├── main.c          application  - RTOS init; creates queue, spawns task_sensor + task_uart
├── tasks_vitals.c  application  - task_sensor (sensors + filter + BPM + IWDG) and task_uart (queue receive + UART format)
├── filter.c/h      processing   - moving average, 4-sample circular buffer
├── bpm.c/h         processing   - BPM detection, dynamic threshold, 600 ms refractory
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
========================
STM32 Vitals Monitor
========================
Temp(C) | RED raw | RED filt| BPM
--------+---------+---------+----
TMP117   OK
MAX30102 OK
========================
  25.7  |    1738 |       0 | ---
  25.7  |    1740 |       0 | ---
  25.7  |  164136 |   17851 | ---
  25.7  |  187702 |   49752 | ---
  25.7  |  196430 |  168883 | ---
  25.7  |  196548 |  178174 | ---
  25.7  |  195773 |  195912 | 71
  25.8  |  195609 |  195713 | 79
  25.8  |  195826 |  195670 | 75
  25.8  |  195707 |  195689 | 75
  25.9  |    1530 |       0 | ---
```

No finger: raw ~1740 (ambient at 12.6 mA LED). Finger contact: raw ~164,000-197,000. DC estimator (filt column) converges over ~20 seconds before BPM appears. BPM 64-97 BPM range at rest, hardware-verified May 2026. Middle column is the IIR DC estimate used for finger detection quality; BPM detection uses the centered AC signal internally.

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

Current implementation status and remaining hardware phases are tracked in
`docs/roadmap.md`.

---

## Limitations

- BPM uses PPG threshold crossings only - not a clinically validated reading
- I2C uses timeout busy-loops - runtime bus faults return bounded error values; no 9-clock bus recovery is implemented yet
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

---

## Development timeline

Built April 4-9, 2026. Driver layer and two-sensor bring-up on April 4 (one evening, hardware debug included). FreeRTOS integration and the IWDG fix on April 7. Two-task split, CMSIS migration, and hardware validation on April 8-9.

---

*Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg - May 2026*
