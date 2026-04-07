# MISRA C Analysis - STM32 Vitals Monitor

## Tool

Cppcheck 2.20.0 with misra.py addon.
Run date: April 2026.
Rules checked against: MISRA C:2012.
Note: MISRA C:2025 is the current published standard (March 2025). Cppcheck's
MISRA addon targets 2012 - this analysis reflects the rules that tooling supports.

```
cppcheck --addon=misra.py --enable=style,warning,performance,portability
         --std=c99 --platform=arm32-wchar_t2
         --suppress=missingInclude --suppress=missingIncludeSystem
         -I firmware/Core/Inc
         firmware/Core/Src/main.c firmware/Core/Src/filter.c
         firmware/Core/Src/uart.c firmware/Core/Src/i2c.c
         firmware/Core/Src/tmp117.c firmware/Core/Src/max30102.c
```

---

## Summary

| Rule | Original findings | Remaining | Disposition |
|---|---|---|---|
| 11.4 | 26 | 26 | Justified deviation |
| 12.2 | 43 | 43 | Justified deviation |
| 13.3 | 1 | 1 | Justified deviation |
| 13.5 | 22 | 22 | Justified deviation |
| 14.4 | 6 | 2 | Justified deviation (4 eliminated as side-effect of fixes) |
| 15.5 | 4 | 4 | Justified deviation |
| 10.4 | 5 | 0 | Fixed |
| 15.6 | 11 | 0 | Fixed |
| 17.7 | 8 | 0 | Fixed |
| 17.8 | 6 | 0 | Fixed |
| 5.9  | 3 | 0 | Fixed |
| 8.7  | 1 | 0 | Fixed (uart_hex removed) |

**Total: 136 findings. 38 fixed. 98 justified deviations.**

Post-fix cppcheck re-run confirmed: 98 violations, all in justified categories.
Rules 10.4, 15.6, 17.7, 17.8, 5.9, 8.7 no longer appear in output.

---

## Justified Deviations

---

### Rule 11.4 - Conversion between pointer to object and integer type (26 findings)

**Location:** All register macro definitions in `uart.c`, `i2c.c`, `max30102.c`.

**Example:**
```c
#define USART2_BRR (*((volatile uint32_t*)0x4000440C))
```

**Deviation rationale:**
This project performs bare-metal register access with no HAL. Every peripheral
is configured by writing to fixed memory-mapped addresses documented in RM0351.
The `volatile uint32_t *` cast from a literal address is the only mechanism
available for this. Using a HAL would eliminate these casts, but HAL use is
explicitly excluded by REQ-NF-05.

This pattern is standard and universally accepted practice in bare-metal embedded
firmware. Every professional embedded C codebase that accesses hardware without
a HAL contains this pattern. The `volatile` qualifier is always present, which
addresses the primary safety concern of rule 11.4 (unexpected compiler optimisation
of hardware-visible side effects).

**Risk assessment:** Low. Addresses are verified against RM0351. No arithmetic is
performed on the pointers - they are used exclusively for single read/write
operations.

---

### Rule 12.2 - Operands of shift operators (43 findings)

**Location:** Register bit manipulation throughout `uart.c`, `i2c.c`, `max30102.c`.

**Example:**
```c
RCC_AHB2ENR |= (1U << 0);
GPIOB_OTYPER |= (1U << 8) | (1U << 9);
```

**Deviation rationale:**
Cppcheck flags these as potential violations because it cannot statically prove
the shift count is within bounds without evaluating the constant expressions.
All shift operands in this codebase are unsigned integer literals (`1U`) shifted
by compile-time constants. Every shift count is between 0 and 31, which is within
the 32-bit width of `uint32_t`. No undefined behaviour is possible.

These patterns directly reflect the bit definitions in RM0351 and the sensor
datasheets. Replacing `(1U << 17)` with a named constant would require defining
a constant for every bit field in every peripheral register, producing 50+
definitions that add no clarity and increase the maintenance surface.

**Risk assessment:** None. All shift counts are verified constants within range.

---

### Rule 13.3 - Side effects in increment/decrement expressions (1 finding)

**Location:** `uart.c:39` - `uart_int()` function.

**Example:**
```c
while(v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
while(i > 0) uart_char(buf[--i]);
```

**Deviation rationale:**
The digit extraction loop uses `i++` and `--i` in standard idioms for building
a reversed digit buffer and then printing it forwards. These are common and
well-understood patterns. The increment and decrement are not combined with
other expressions that read the same variable. There is no ambiguity about
evaluation order.

**Risk assessment:** Low. The loop is short, the behaviour is deterministic,
and the output is verified by TC-01.

---

### Rule 13.5 - Side effects in the right-hand side of `&&` or `||` (22 findings)

**Location:** I2C polling loops in `i2c.c` and `max30102.c`.

**Example:**
```c
timeout = 50000;
while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
```

**Deviation rationale:**
This is the standard pattern for I2C polling with timeout in bare-metal embedded
firmware. The `timeout--` side effect is deliberate and its behaviour is
well-defined: C evaluates `&&` left-to-right with short-circuit semantics, so
`timeout--` executes only when the left condition is true (the hardware flag
has not yet been set). When the flag is set, the loop exits before `timeout--`
fires.

The alternative - a separate decrement inside the loop body - requires more
lines, an additional `if(!timeout) break;` statement, and makes the timeout
intent less clear. In a safety-critical production context, the refactored
form would be preferred. For this project, the pattern is acceptable and is
consistent across all I2C transactions.

**Risk assessment:** Low. Timeout limits the maximum blocking time to 50000
cycles per polling loop. Documented as a known limitation in `docs/limitations.md`.

---

### Rule 14.4 - Condition not essentially Boolean (2 findings remaining)

**Location:** `main.c:35` (`if(tmp117_init())`), `main.c:41` (`if(max30102_init())`).

Note: `while(*s)` in `uart.c` and `while(n--)` in delay functions were eliminated
as side-effects of the Rule 17.8 and Rule 5.9 fixes applied in this analysis.

**Example:**
```c
if(tmp117_init()) {         /* returns uint8_t 0 or 1 */
    uart_str("TMP117 OK\r\n");
}
```

**Deviation rationale:**
The init functions return `uint8_t` with values 0 (failure) or 1 (success).
Using `if(tmp117_init() != 0U)` would be more strictly MISRA-compliant but adds
no clarity. The non-zero-means-success convention is standard in embedded C and
is documented in the function contracts.

**Risk assessment:** Low. The condition has unambiguous zero/non-zero semantics.

---

### Rule 15.5 - A function should have a single point of exit (4 findings)

**Location:** `i2c.c:47,51` - early returns on timeout in `i2c_write_reg()`.

**Example:**
```c
while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
if(!timeout) return 0;
```

**Deviation rationale:**
The early return on timeout is an error-handling path, not a normal return.
Restructuring to a single exit point would require either a flag variable
and nested conditions, or a `goto` - both of which reduce readability in
this case. The function has two clearly defined exit points: success (`return 1`)
at the bottom and timeout failure (`return 0`) on each polling step.

In a full MISRA-compliant implementation, a result variable and single return
would be used. This is noted for future refactoring.

**Risk assessment:** Low. The function behaviour is tested by TC-02.

---

## Fixed Violations

The following violations were corrected in the firmware source after this analysis.
All fixes are in the dev branch.

---

### Rule 15.6 - Single-statement body not enclosed in braces (11 findings - FIXED)

**Rule text:** The body of an iteration/selection statement shall be enclosed
in braces.

**Files affected:** `main.c`, `filter.c`, `uart.c`, `i2c.c`, `max30102.c`.

**Fix applied:** Added braces to all single-statement `if`, `else`, and `while`
bodies across all affected files.

**Example:**
```c
/* Before */
if(tmp117_init())
    uart_str("TMP117  OK\r\n");
else
    uart_str("TMP117  FAIL\r\n");

/* After */
if(tmp117_init() != 0U) {
    uart_str("TMP117  OK\r\n");
} else {
    uart_str("TMP117  FAIL\r\n");
}
```

---

### Rule 17.7 - Return value of non-void function not used (8 findings - FIXED)

**Rule text:** The value returned by a function having non-void return type
shall be used.

**Files affected:** `max30102.c` - seven calls to `i2c_write_reg()` in
`max30102_init()` where the return value was discarded.

**Fix applied:** Cast each call to `(void)` to make the intent explicit:
the return value is deliberately ignored during initialisation because a
failure here would propagate to a visible `MAX30102 FAIL` at startup.

```c
/* Before */
i2c_write_reg(MAX30102_ADDR, REG_MODE, 0x40U);

/* After */
(void)i2c_write_reg(MAX30102_ADDR, REG_MODE, 0x40U);
```

---

### Rule 17.8 - Function parameter should not be modified (6 findings - FIXED)

**Rule text:** A function parameter shall not be modified.

**Files affected:** `main_delay()` in `main.c`, `i2c_delay()` in `i2c.c`,
`max30102_delay()` in `max30102.c`, and `uart_str()`/`uart_int()` in `uart.c`.

**Fix applied:** Copy the parameter to a local variable before modifying it.

```c
/* Before - delay functions */
static void delay(volatile uint32_t n) { while(n--) {} }

/* After - delay functions */
static void main_delay(volatile uint32_t n)
{
    volatile uint32_t count = n;
    while(count-- != 0U) {}
}

/* Before - uart_str */
void uart_str(const char *s) { while(*s) { uart_char(*s++); } }

/* After - uart_str */
void uart_str(const char *s)
{
    const char *p = s;
    while(*p != '\0') { uart_char(*p); p++; }
}

/* Before - uart_int */
void uart_int(int32_t v) { if(v < 0) { uart_char('-'); v = -v; } ... v /= 10; }

/* After - uart_int */
void uart_int(int32_t v)
{
    int32_t val = v;
    if(val < 0) { uart_char('-'); val = -val; }
    ...
    while(val > 0) { ...; val /= 10; }
}
```

---

### Rule 5.9 - Duplicate identifier in same scope (3 findings - FIXED)

**Rule text:** Identifiers that define objects or functions with internal linkage
should be unique.

**Files affected:** `delay()` static function duplicated in `main.c`, `i2c.c`,
and `max30102.c`.

**Fix applied:** Renamed each static `delay()` to a unique per-file name:
`main_delay()` in `main.c`, `i2c_delay()` in `i2c.c`, and `max30102_delay()`
in `max30102.c`. All call sites within each file updated accordingly.
Each function remains `static` with internal linkage.

---

### Rule 8.7 - Object should be defined at block scope (1 finding - FIXED)

**Rule text:** Functions and objects should not be defined with external linkage
if they are referenced in only one translation unit.

**Files affected:** `uart_hex()` in `uart.c` - declared in `uart.h` with external
linkage but never called from any translation unit.

**Fix applied:** Removed `uart_hex()` from both `uart.c` and `uart.h`. The function
was dead code: it had no callers inside or outside `uart.c`. Retaining an unused
static definition would produce a different class of warning; deletion is the
correct resolution.

---

### Rule 10.4 - Type mismatch in arithmetic or comparison (5 findings - FIXED)

**Rule text:** Both operands of an operator in which the usual arithmetic
conversions are performed shall have the same essential type.

**Files affected:** `filter.c` - `uint8_t` loop counter compared against
`FILTER_WINDOW` (unqualified integer constant); `uart.c` - `uint8_t` counter
`i` compared against integer literal `0`, and `uint8_t` masked with plain
hex literal `0xF`.

**Fix applied:**
- `filter.c`: Added `(uint8_t)` cast to `FILTER_WINDOW` uses; made constants
  unsigned with `U` suffix.
- `uart.c:40`: Changed `while(i > 0)` to `while(i > 0U)` - matches `uint8_t i`.
- `uart.c:48`: Removed by deleting `uart_hex()` (see Rule 8.7 fix).

---

*Document version: 1.0*
*Cppcheck version: 2.20.0*
*Standard: MISRA C:2012*
*Author: Vaibhav Aher - M.Sc. ICT, FAU Erlangen-Nürnberg*
*Date: April 2026*
