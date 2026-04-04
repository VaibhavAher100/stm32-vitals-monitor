# Session Log — STM32 Vitals Monitor Phase 4

**Date:** April 4, 2026
**Branch:** dev copy at `C:/Users/vaibh/Projects/dev/phase4-stm32-vitals-monitor`
**Cloned from:** `C:/Users/vaibh/Projects/dev/phase3-stm32-vitals-monitor` (Phase 3 complete)
**Original repo (untouched):** `C:/Users/vaibh/Projects/stm32-vitals-monitor`

---

## What Phase 3 Delivered (already in this clone)

- `firmware/Core/Inc/bpm.h` + `Src/bpm.c` — dynamic-threshold rising-edge BPM detector;
  8-sample rolling min/max window; 333 ms refractory; `60000 / interval_ms`; `BPM_INVALID` until 2 crossings
- `main.c` — BPM column added to UART output; `---` until first valid beat pair
- `docs/requirements.md` — REQ-SYS-08 (BPM) added; §2.7 BPM Detection + §2.8 Measurement Output; coverage 52/52
- `docs/testing.md` — TC-12 (BPM at rest) added; all tests PASS

---

## Full Roadmap

- Phase 1 — Code quality: REQUIREMENTS.md, TESTING.md, MISRA-C analysis — DONE
- Phase 2 — Hardware quality: SysTick delay, IWDG watchdog — DONE
- Phase 3 — Signal processing: BPM from PPG — DONE
- **Phase 4 — RTOS: FreeRTOS task separation** — NEXT
- Phase 5 — Power: STOP2 sleep mode

---

## Session Goal

Integrate FreeRTOS into the bare-metal project by vendoring the kernel source
directly into the repo (no CubeMX, no HAL). Split the monolithic main loop into
two tasks: one for sensing + processing, one for UART output, communicating via
a FreeRTOS queue.

---

## FreeRTOS Approach — Vendor Source Directly

**Why vendoring:** STM32CubeMX firmware pack (STM32Cube_FW_L4) is not installed.
Vendoring the kernel source directly into `firmware/Middlewares/FreeRTOS/` keeps
the repo self-contained and is valid for a bare-metal portfolio project.

**FreeRTOS version to use:** V10.5.1 (MIT license, stable, widely used in industry).

**Source location:** Official FreeRTOS kernel GitHub releases:
`https://github.com/FreeRTOS/FreeRTOS-Kernel/releases/tag/V10.5.1`

---

## Files Needed from FreeRTOS Kernel

### Kernel source (place in `firmware/Middlewares/FreeRTOS/Source/`)
- `tasks.c`
- `queue.c`
- `list.c`
- `timers.c`
- `event_groups.c`
- `stream_buffer.c`

### Port files for Cortex-M4F (GCC)
Place in `firmware/Middlewares/FreeRTOS/Source/portable/GCC/ARM_CM4F/`:
- `port.c`
- `portmacro.h`

### Heap allocator
Place in `firmware/Middlewares/FreeRTOS/Source/portable/MemMang/`:
- `heap_4.c`

### Headers (place in `firmware/Middlewares/FreeRTOS/Source/include/`)
- `FreeRTOS.h`
- `task.h`
- `queue.h`
- `semphr.h`
- `timers.h`
- `event_groups.h`
- `stream_buffer.h`
- `list.h`
- `projdefs.h`
- `portable.h`
- `mpu_wrappers.h`
- `stack_macros.h`
- `deprecated_definitions.h`
- `message_buffer.h`
- `croutine.h`

### Config (we write this — place in `firmware/Core/Inc/`)
- `FreeRTOSConfig.h` — see spec below

---

## FreeRTOSConfig.h Spec

Target: STM32L476RG, Cortex-M4, 4 MHz MSI clock, no HAL.

Key settings:
```c
#define configUSE_PREEMPTION                    1
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCPU_CLOCK_HZ                      4000000UL
#define configTICK_RATE_HZ                      1000U        /* 1 ms tick */
#define configMAX_PRIORITIES                    5U
#define configMINIMAL_STACK_SIZE                128U         /* words */
#define configTOTAL_HEAP_SIZE                   4096U        /* 4 KB from 128 KB SRAM */
#define configMAX_TASK_NAME_LEN                 8U
#define configUSE_TRACE_FACILITY                0
#define configUSE_16_BIT_TICKS                  0
#define configIDLE_SHOULD_YIELD                 1
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           0
#define configQUEUE_REGISTRY_SIZE               0
#define configUSE_QUEUE_SETS                    0
#define configUSE_TIME_SLICING                  1
#define configUSE_NEWLIB_REENTRANT              0
#define configENABLE_BACKWARD_COMPATIBILITY     0
#define configUSE_TIMERS                        0
#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

/* Cortex-M interrupt priority settings */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5U
#define configKERNEL_INTERRUPT_PRIORITY         (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << 4U)
#define configMAX_SYSCALL_INTERRUPT_PRIORITY    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << 4U)

/* Map FreeRTOS handlers to CMSIS names */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

/* API includes */
#define INCLUDE_vTaskDelay          1
#define INCLUDE_vTaskDelete         0
#define INCLUDE_vTaskSuspend        0
#define INCLUDE_xTaskGetCurrentTaskHandle 0
```

**CRITICAL NOTE on SysTick:** FreeRTOS takes over SysTick for its own tick.
`systick.c` / `SysTick_Handler` must be **removed or disabled** in Phase 4.
Replace `delay_ms()` usage with `vTaskDelay()`. Replace `get_tick()` usage in
`bpm.c` with `xTaskGetTickCount()`. Both return ms ticks at the same 1 kHz rate.

---

## Task Architecture

### Task 1 — `task_sensor` (priority 2)
- Reads TMP117 and MAX30102 every 500 ms using `vTaskDelay(pdMS_TO_TICKS(500U))`
- Applies `filter_update()` and `bpm_update()`
- Packs result into a `VitalsMsg` struct
- Sends to queue: `xQueueSend(vitals_queue, &msg, 0)`
- Kicks IWDG

### Task 2 — `task_uart` (priority 1)
- Blocks on queue: `xQueueReceive(vitals_queue, &msg, portMAX_DELAY)`
- Formats and transmits the row over UART
- Lower priority than sensor task — pre-empted if sensor task wakes

### Queue
```c
typedef struct {
    int32_t  temp_x10;   /* Celsius × 10 */
    uint32_t ir_raw;
    uint32_t ir_filt;
    uint32_t bpm;
} VitalsMsg;

QueueHandle_t vitals_queue;  /* depth = 1 */
```

### Startup (`main.c`)
```
systick_init()  ← REMOVED (FreeRTOS owns SysTick)
iwdg_init()
uart_init()
i2c_init()
vitals_queue = xQueueCreate(1, sizeof(VitalsMsg))
xTaskCreate(task_sensor, ...)
xTaskCreate(task_uart, ...)
vTaskStartScheduler()
/* never returns */
```

---

## New / Modified Files

**New:**
- `firmware/Middlewares/FreeRTOS/` — vendored kernel source tree (see above)
- `firmware/Core/Inc/FreeRTOSConfig.h` — project-specific RTOS config
- `firmware/Core/Src/tasks_vitals.c` — `task_sensor()` + `task_uart()` + `VitalsMsg` + queue handle

**Modified:**
- `main.c` — remove `systick_init()`; remove sensor/filter/bpm/uart loop; create queue + tasks; call `vTaskStartScheduler()`
- `bpm.c` — replace `get_tick()` with `xTaskGetTickCount()`; add `#include "FreeRTOS.h"` + `#include "task.h"`
- `systick.c` / `systick.h` — **delete both** (FreeRTOS owns SysTick via `xPortSysTickHandler`)
- `i2c.c`, `max30102.c` — replace `delay_ms()` with `vTaskDelay(pdMS_TO_TICKS(...))` (called from task context)

**STM32CubeIDE project:** The `.cproject` file will need the new source paths added
under Project → Properties → C/C++ Build → Settings → MCU GCC Compiler → Include paths:
- `Middlewares/FreeRTOS/Source/include`
- `Middlewares/FreeRTOS/Source/portable/GCC/ARM_CM4F`

And source folders:
- `Middlewares/FreeRTOS/Source`
- `Middlewares/FreeRTOS/Source/portable/GCC/ARM_CM4F`
- `Middlewares/FreeRTOS/Source/portable/MemMang`

---

## Docs to Update

- `docs/requirements.md` — add REQ-RTOS-01 through REQ-RTOS-05 (task creation, queue, scheduler, priority, IWDG from task)
- `docs/testing.md` — add TC-13 (two tasks running; UART output continues; sensor task pre-empts UART task)

---

## What Was Done This Session

- Cloned `phase3-stm32-vitals-monitor` → `phase4-stm32-vitals-monitor`; removed origin
- Decided on vendoring FreeRTOS V10.5.1 kernel source (no CubeMX required)
- Designed full task architecture, FreeRTOSConfig.h spec, file plan
- Written this SESSION_LOG — ready to implement next session

**Implementation not yet started.**

---

## Resume Checklist

When starting the next session (Phase 4 implementation):
1. Read this SESSION_LOG.md fully
2. Run: `cd C:/Users/vaibh/Projects/dev/phase4-stm32-vitals-monitor && git log --oneline -3`
3. Download FreeRTOS V10.5.1 kernel source files listed above (Claude can do this via WebFetch from GitHub raw URLs)
4. Create the directory structure under `firmware/Middlewares/FreeRTOS/`
5. Write `FreeRTOSConfig.h` per spec above
6. Write `firmware/Core/Src/tasks_vitals.c`
7. Modify `main.c`, `bpm.c`; delete `systick.c`/`systick.h`; update `i2c.c`, `max30102.c`
8. Update STM32CubeIDE `.cproject` include/source paths
9. Update docs
10. Commit

---

*Log written: April 4, 2026*
*Dev path: C:/Users/vaibh/Projects/dev/phase4-stm32-vitals-monitor*
