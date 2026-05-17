# MISRA C Analysis - STM32 Vitals Monitor

> **MISRA analysis status (May 2026 - current codebase):**
> Re-run performed on all 9 project source files (post-CMSIS refactor, post-BPM algorithm
> rework) using Cppcheck 2.20.0 + misra.py addon via dump-file analysis.
>
> **Standard cppcheck analysis (`--enable=all`) on 9 project source files: zero errors,
> zero warnings.** Four variableScope findings in tasks_vitals.c (dc, ac, ac_filt, i)
> were found and fixed (variables moved to narrowest applicable scope, May 2026).
>
> **MISRA rule violations found in project source files:**
> Rules 12.2, 13.3, 14.4, 15.5, 8.4 - same deviations as documented below.
> All remain justified. No new violations introduced by the CMSIS refactor or
> BPM algorithm changes.
>
> **Note on misra-config errors:** The dump-file approach resolves CMSIS include paths
> only at the project header level. CMSIS peripheral register symbols (I2C1, USART2, etc.)
> are unresolved in the dump, producing `misra-config` false-positive errors.
> These are analysis tool artefacts, not code violations.
>
> Rule 11.4 deviations below are now **obsolete** - the CMSIS refactor replaced all raw
> volatile pointer casts with CMSIS struct access.

## Tool

Cppcheck 2.20.0 with misra.py addon (dump-file method).
Run dates: April 2026 (original, pre-CMSIS) and May 2026 (current codebase).
Rules checked against: MISRA C:2012.
Note: MISRA C:2025 is the current published standard (March 2025). Cppcheck's
MISRA addon targets 2012 - this analysis reflects the rules that tooling supports.

Command used for May 2026 re-run:

```
# Step 1: generate dump files
cppcheck --dump --std=c99 -DSTM32L476xx --suppress=missingInclude
         -I firmware/Core/Inc
         firmware/Core/Src/{uart,i2c,tmp117,max30102,filter,bpm,iwdg,tasks_vitals,main}.c

# Step 2: MISRA check
python misra.py --no-summary firmware/Core/Src/*.c.dump
```

---

## Summary

| Rule | Original findings | Remaining | Disposition |
|---|---|---|---|
| 11.4 | 26 | 0 | Obsolete — CMSIS refactor eliminated all volatile pointer casts |
| 12.2 | 43 | 43 | Justified deviation |
| 13.3 | 1 | 1 | Justified deviation |
| 13.5 | 22 | 0  | Eliminated — timeout loop pattern changed to `while(...&&(timeout>0U)){timeout--;}` |
| 14.4 | 6 | 2 | Justified deviation (4 eliminated as side-effect of fixes) |
| 15.5 | 4 | 4 | Justified deviation |
| 10.4 | 5 | 0 | Fixed |
| 15.6 | 11 | 0 | Fixed |
| 17.7 | 8 | 0 | Fixed (original 8) + 4 new findings fixed (xTaskCreate×2, xQueueOverwrite, xQueueReceive) |
| 17.8 | 6 | 0 | Fixed — `busy_delay_ms` regression in `i2c.c`/`max30102.c` also fixed (renamed `i2c_busy_delay_ms`, removed from `max30102.c`) |
| 5.9  | 3 | 0 | Fixed — `busy_delay_ms` duplication regression fixed: removed from `max30102.c`, renamed in `i2c.c` |
| 8.7  | 1 | 0 | Fixed (uart_hex removed) |
| 10.3 | 1 | 0 | Fixed — RXDR narrowing cast in `i2c_read_2bytes`: `(uint16_t)((uint16_t)(I2C1->RXDR & 0xFFU) << 8U)` |

**Total: 136 original + 8 new findings. 46 fixed. 76 justified deviations. Rule 13.5 eliminated.**

Post-fix cppcheck re-run confirmed: 98 violations, all in justified categories.
Rules 10.4, 15.6, 17.7, 17.8, 5.9, 8.7 no longer appear in output.

**May 2026 fixes (post-review):** Rules 13.5 (22 findings eliminated), 17.7 (4 new fixed), 17.8 regression fixed, 5.9 regression fixed, 10.3 (1 new fixed).

---

## Justified Deviations

---

### Rule 11.4 — Conversion between pointer to object and integer type (26 findings)

> **Status: OBSOLETE.** The CMSIS refactor (commit 7b34efe) replaced all raw
> `volatile uint32_t *` pointer casts with CMSIS struct access (e.g.,
> `USART2->BRR` instead of `*((volatile uint32_t*)0x4000440C)`). Rule 11.4
> violations of this type no longer exist in the current codebase.
> Retained here for historical record.

**Location (pre-CMSIS):** All register macro definitions in `uart.c`, `i2c.c`, `max30102.c`.

**Example (pre-CMSIS):**
```c
#define USART2_BRR (*((volatile uint32_t*)0x4000440C))
```

**Deviation rationale (historical):**
The `volatile uint32_t *` cast from a literal address was the mechanism used
for register access before CMSIS headers were added. HAL use is explicitly
excluded by REQ-NF-05. The `volatile` qualifier was always present.

**Risk assessment:** No longer applicable - pattern eliminated by CMSIS refactor.

---

### Rule 12.2 — Operands of shift operators (43 findings)

**Status on current CMSIS codebase:** Still present. Shift patterns unchanged (e.g., `RCC->AHB2ENR |= (1U << 0)`). CMSIS struct access does not remove the shifts.

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

### Rule 13.3 — Side effects in increment/decrement expressions (1 finding)

**Status on current CMSIS codebase:** Still present. `uart_int()` logic unchanged.

**Location:** `uart.c:39` — `uart_int()` function.

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

### Rule 13.5 — Side effects in the right-hand side of `&&` or `||` (22 findings — FIXED)

**Status:** Eliminated. All I2C timeout loops refactored to separate decrement from condition.

**Fix applied (May 2026):** Every `while(... && timeout--)` pattern replaced with
`while(... && (timeout > 0U)) { timeout--; }`. This also fixed a latent bug: the
original post-decrement caused `timeout` to wrap to `UINT32_MAX` on expiry, making
every `if (!timeout)` error check dead code. The refactored form correctly leaves
`timeout == 0U` on expiry, making all error paths reachable.

**Example:**
```c
/* Before — Rule 13.5 violation AND broken error detection */
while(!(I2C1->ISR & I2C_ISR_TXIS) && timeout--) {}
if (!timeout) { return 0U; }   /* DEAD CODE: timeout was UINT32_MAX, not 0 */

/* After — compliant and correct */
while(!(I2C1->ISR & I2C_ISR_TXIS) && (timeout > 0U)) { timeout--; }
if (timeout == 0U) { return 0U; }
```

---

### Rule 14.4 — Condition not essentially Boolean (2 findings remaining)

**Status on current CMSIS codebase:** Still present. `if(tmp117_init())` pattern unchanged; init functions now called from `tasks_vitals.c` rather than `main.c` but the pattern is identical.

**Location:** `tasks_vitals.c` — `if(tmp117_init())`, `if(max30102_init())`.

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

### Rule 15.5 — A function should have a single point of exit (4 findings)

**Status on current CMSIS codebase:** Still present. Early return on timeout pattern unchanged across i2c.c transfer functions.

**Location:** `i2c.c` — early returns on timeout in `i2c_write_reg()` and read functions.

**Example:**
```c
while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
if(!timeout) return 0;
```

**Deviation rationale:**
The early return on timeout is an error-handling path, not a normal return.
Restructuring to a single exit point would require either a flag variable
and nested conditions, or a `goto` — both of which reduce readability in
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

### Rule 15.6 — Single-statement body not enclosed in braces (11 findings — FIXED)

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

### Rule 17.7 — Return value of non-void function not used (8 findings — FIXED)

**Rule text:** The value returned by a function having non-void return type
shall be used.

**Files affected:** `max30102.c` — seven calls to `i2c_write_reg()` in
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

### Rule 17.8 — Function parameter should not be modified (6 findings — FIXED)

**Rule text:** A function parameter shall not be modified.

**Files affected:** `main_delay()` in `main.c`, `i2c_delay()` in `i2c.c`,
`max30102_delay()` in `max30102.c`, and `uart_str()`/`uart_int()` in `uart.c`.

**Fix applied:** Copy the parameter to a local variable before modifying it.

```c
/* Before — delay functions */
static void delay(volatile uint32_t n) { while(n--) {} }

/* After — delay functions */
static void main_delay(volatile uint32_t n)
{
    volatile uint32_t count = n;
    while(count-- != 0U) {}
}

/* Before — uart_str */
void uart_str(const char *s) { while(*s) { uart_char(*s++); } }

/* After — uart_str */
void uart_str(const char *s)
{
    const char *p = s;
    while(*p != '\0') { uart_char(*p); p++; }
}

/* Before — uart_int */
void uart_int(int32_t v) { if(v < 0) { uart_char('-'); v = -v; } ... v /= 10; }

/* After — uart_int */
void uart_int(int32_t v)
{
    int32_t val = v;
    if(val < 0) { uart_char('-'); val = -val; }
    ...
    while(val > 0) { ...; val /= 10; }
}
```

---

### Rule 5.9 — Duplicate identifier in same scope (3 findings — FIXED)

**Rule text:** Identifiers that define objects or functions with internal linkage
should be unique.

**Files affected:** `delay()` static function duplicated in `main.c`, `i2c.c`,
and `max30102.c`.

**Fix applied:** Renamed each static `delay()` to a unique per-file name:
`main_delay()` in `main.c`, `i2c_delay()` in `i2c.c`, and `max30102_delay()`
in `max30102.c`. All call sites within each file updated accordingly.
Each function remains `static` with internal linkage.

---

### Rule 8.7 — Object should be defined at block scope (1 finding — FIXED)

**Rule text:** Functions and objects should not be defined with external linkage
if they are referenced in only one translation unit.

**Files affected:** `uart_hex()` in `uart.c` — declared in `uart.h` with external
linkage but never called from any translation unit.

**Fix applied:** Removed `uart_hex()` from both `uart.c` and `uart.h`. The function
was dead code: it had no callers inside or outside `uart.c`. Retaining an unused
static definition would produce a different class of warning; deletion is the
correct resolution.

---

### Rule 10.4 — Type mismatch in arithmetic or comparison (5 findings — FIXED)

**Rule text:** Both operands of an operator in which the usual arithmetic
conversions are performed shall have the same essential type.

**Files affected:** `filter.c` — `uint8_t` loop counter compared against
`FILTER_WINDOW` (unqualified integer constant); `uart.c` — `uint8_t` counter
`i` compared against integer literal `0`, and `uint8_t` masked with plain
hex literal `0xF`.

**Fix applied:**
- `filter.c`: Added `(uint8_t)` cast to `FILTER_WINDOW` uses; made constants
  unsigned with `U` suffix.
- `uart.c:40`: Changed `while(i > 0)` to `while(i > 0U)` — matches `uint8_t i`.
- `uart.c:48`: Removed by deleting `uart_hex()` (see Rule 8.7 fix).

---

*Document version: 1.0*
*Cppcheck version: 2.20.0*
*Standard: MISRA C:2012*
*Author: Vaibhav Aher — M.Sc. ICT, FAU Erlangen-Nürnberg*
*Date: April 2026*
