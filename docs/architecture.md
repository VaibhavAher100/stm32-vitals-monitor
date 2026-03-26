# Architecture — STM32 Vitals Monitor

## Overview

This project follows a deliberate two-phase development strategy:

1. **Prototype phase** — All code in `main.c`. Every peripheral driver, signal processing
   function, and application logic lives in one file. This phase prioritises getting verified
   hardware output before introducing structural complexity.

2. **Refactor phase** — Once all sensors produce clean verified output, the monolithic `main.c`
   is split into a three-layer architecture. The refactor commit in the Git history marks this
   transition explicitly.

This approach reflects how professional embedded firmware teams work: prototype fast, then
structure deliberately. The Git history preserves both phases, demonstrating the reasoning
behind architectural decisions rather than presenting only the final polished result.

---

## Three-Layer Architecture (Post-Refactor)

```
┌─────────────────────────────────────────────────────┐
│                  APPLICATION LAYER                  │
│                     main.c                          │
│                                                     │
│  System initialisation, main loop, output           │
│  formatting, alert thresholds, UART printing.       │
│  No direct register access.                         │
└───────────────────┬─────────────────────────────────┘
                    │ calls
┌───────────────────▼─────────────────────────────────┐
│                  PROCESSING LAYER                   │
│                   filter.c / filter.h               │
│                                                     │
│  Moving average filter (circular buffer).           │
│  Threshold detection.                               │
│  No register access. No UART calls.                 │
└───────────────────┬─────────────────────────────────┘
                    │ calls
┌───────────────────▼─────────────────────────────────┐
│                   DRIVER LAYER                      │
│  uart.c/h   i2c.c/h   tmp117.c/h   max30102.c/h    │
│                                                     │
│  Direct register writes only.                       │
│  Hardware initialisation and raw sensor reads.      │
│  No application logic. No signal processing.        │
└─────────────────────────────────────────────────────┘
                    │ writes to
┌───────────────────▼─────────────────────────────────┐
│                   HARDWARE                          │
│     STM32L476RG  │  TMP117  │  MAX30102             │
└─────────────────────────────────────────────────────┘
```

---

## Layer Rules — Non-Negotiable

These rules are enforced at the refactor stage and maintained throughout:

| Rule | Reason |
|---|---|
| Driver layer never calls application layer | Prevents circular dependencies |
| Application layer never writes to registers | Keeps hardware abstraction clean |
| Processing layer never accesses hardware directly | Enables unit testing of filter logic |
| Each layer communicates only with the layer directly below | Maintains clean separation |

---

## File Structure (Post-Refactor Target)

```
firmware/Core/Src/
├── main.c           Application layer — system init, main loop, output
├── uart.c           UART driver — register-level USART2 configuration and transmit
├── i2c.c            I2C driver — register-level I2C1 configuration, read/write
├── tmp117.c         TMP117 driver — sensor init, register read, temperature conversion
├── max30102.c       MAX30102 driver — sensor init, FIFO read, raw IR data
├── filter.c         Processing layer — circular buffer moving average filter
├── syscalls.c       System call stubs (auto-generated, do not modify)
└── sysmem.c         Memory management stubs (auto-generated, do not modify)

firmware/Core/Inc/
├── uart.h           UART function declarations
├── i2c.h            I2C function declarations
├── tmp117.h         TMP117 function declarations and register defines
├── max30102.h       MAX30102 function declarations and register defines
└── filter.h         Filter function declarations and type definitions
```

---

## Design Decisions

### No HAL

The Hardware Abstraction Layer (HAL) provided by STM32CubeMX is intentionally not used.
Every peripheral is configured by writing directly to the registers documented in RM0351.

Reason: HAL abstracts away the register-level details that this project is specifically designed
to demonstrate. A recruiter or engineer reviewing this code should see direct register writes,
not generated HAL calls that hide the actual configuration.

### No Dynamic Memory Allocation

No `malloc`, `calloc`, or `free` is used anywhere in this project.
All buffers are statically allocated at compile time.

Reason: Dynamic allocation in embedded firmware introduces non-deterministic behaviour,
heap fragmentation risk, and undefined failure modes. Safety-critical embedded domains
(IEC 62304, MISRA C) explicitly prohibit or restrict dynamic allocation.

### No Recursion

No recursive function calls are used.
All loops use iterative constructs.

Reason: Recursion makes stack depth non-deterministic, which is unacceptable in
a microcontroller with 128KB RAM and no memory protection unit (MPU) active.

### Volatile on All Register Accesses

Every register access uses `volatile uint32_t*` casting.

Reason: Without `volatile`, the compiler may optimise away register writes it cannot
observe side effects from — for example, polling a status bit in a loop.

---

## Clock Configuration

System clock: MSI (Multi-Speed Internal) oscillator at 4 MHz after reset.
No external crystal. No PLL. Default power-on configuration.

BRR calculation for UART: 4,000,000 / 9600 = 417 (rounded).
I2C timing register: 0x00100D14 for 100 kHz standard mode at 4 MHz.

---

*Last updated: March 2026*
*Hardware: STM32 Nucleo-L476RG*
*Reference: RM0351 STM32L47xxx/L48xxx Reference Manual*
