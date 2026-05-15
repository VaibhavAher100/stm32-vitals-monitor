# Architecture - STM32 Vitals Monitor

## Overview

Two development phases visible in git history:

1. **Prototype phase** - All code in `main.c`. Gets verified hardware output first, structure second.

2. **Refactor phase** - Monolithic `main.c` split into a three-layer architecture once sensors produced clean output. The refactor commit marks this transition.

Phase 4 added FreeRTOS: `main.c` now owns scheduler startup and task creation. Measurement logic lives in `tasks_vitals.c`.

---

## Architecture (Current - FreeRTOS + Three-Layer)

```
┌─────────────────────────────────────────────────────┐
│                  APPLICATION LAYER                  │
│          main.c + tasks_vitals.c                    │
│                                                     │
│  main.c: RTOS init, queue create, task spawn        │
│  task_sensor: reads sensors, filters, detects BPM,  │
│               kicks IWDG, posts to vitals_queue     │
│  task_uart: receives from queue, formats UART output │
│  No direct register access.                         │
└───────────────────┬─────────────────────────────────┘
                    │ calls
┌───────────────────▼─────────────────────────────────┐
│                  PROCESSING LAYER                   │
│           filter.c / filter.h                       │
│           bpm.c / bpm.h                             │
│                                                     │
│  Moving average filter (circular buffer, 8 samples) │
│  BPM detector (dynamic threshold, rising-edge)      │
│  No register access. No UART calls.                 │
│  Note: bpm.c uses xTaskGetTickCount() for timing.   │
└───────────────────┬─────────────────────────────────┘
                    │ calls
┌───────────────────▼─────────────────────────────────┐
│                   DRIVER LAYER                      │
│  uart  i2c  tmp117  max30102  iwdg                  │
│                                                     │
│  CMSIS struct register writes only.                 │
│  Hardware initialisation and raw sensor reads.      │
│  No application logic. No signal processing.        │
└─────────────────────────────────────────────────────┘
                    │ writes to
┌───────────────────▼─────────────────────────────────┐
│                   HARDWARE                          │
│     STM32L476RG  |  TMP117  |  MAX30102             │
└─────────────────────────────────────────────────────┘
```

Three rules: application never touches registers. Driver never calls application. Processing layer has no hardware dependencies (except bpm.c FreeRTOS tick access).

---

## File Structure

```
firmware/Core/Src/
├── main.c           Application - RTOS init, queue create, task spawn
├── tasks_vitals.c   Application - task_sensor and task_uart implementations
├── filter.c         Processing  - 8-sample circular buffer moving average
├── bpm.c            Processing  - dynamic threshold BPM detection
├── uart.c           Driver      - USART2 register-level init and transmit
├── i2c.c            Driver      - I2C1 register-level init, read, write
├── tmp117.c         Driver      - TMP117 init and temperature read
├── max30102.c       Driver      - MAX30102 FIFO init and IR read
├── iwdg.c           Driver      - IWDG init and kick
├── syscalls.c       Newlib stubs (IDE-generated, do not modify)
└── sysmem.c         Memory stubs (IDE-generated, do not modify)

firmware/Core/Inc/
├── tasks_vitals.h   VitalsMsg struct, queue handle declaration
├── filter.h         Filter struct and function declarations
├── bpm.h            BpmDetector struct, constants, function declarations
├── uart.h, i2c.h, tmp117.h, max30102.h, iwdg.h
└── FreeRTOSConfig.h FreeRTOS configuration (4 MHz, 1 kHz tick, heap_4)
```

---

## Design Decisions

### No HAL

Every peripheral configured by direct register writes via CMSIS device headers (`stm32l476xx.h`), not STM32CubeMX HAL.

Reason: this project exists to show register-level work. HAL hides the actual configuration.

### No Dynamic Memory Allocation in Project Code

No `malloc`, `calloc`, or `free` in project source files. FreeRTOS heap_4 is present for kernel use (task stacks, queue buffers) only.

Reason: dynamic allocation in embedded firmware is non-deterministic. MISRA C and IEC 62304 restrict or prohibit it. Stack-only allocation gives deterministic memory layout.

### No Recursion

All loops are iterative. No recursive calls.

Reason: recursion makes stack depth non-deterministic on a 128 KB SRAM device with no MPU.

### CMSIS Register Access

Register access uses CMSIS peripheral struct pointers (`RCC->APB1ENR1`, `IWDG->KR`, etc.) rather than hand-defined `volatile uint32_t *` macros. CMSIS headers are the vendor-maintained definition of the register map.

### FreeRTOS Queue over Shared Globals

task_sensor and task_uart communicate through a depth-1 queue (xQueueOverwrite), not shared globals.

Reason: a context switch mid-copy of a shared struct produces a partially-written value. Queue transfer is atomic at the message level. depth-1 means task_uart always gets the latest reading, never a stale one.

---

## Clock Configuration

System clock: MSI at 4 MHz (default power-on). No PLL, no external crystal.

UART BRR: 4,000,000 / 9600 = 417.
I2C TIMINGR: 0x00100D14 for 100 kHz at 4 MHz.
FreeRTOS tick: 1 ms (configTICK_RATE_HZ = 1000).

---

*Last updated: May 2026*
*Hardware: STM32 Nucleo-L476RG*
*Reference: RM0351 STM32L47xxx/L48xxx Reference Manual*
