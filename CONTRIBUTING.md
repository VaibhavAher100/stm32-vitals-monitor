# Git Conventions

## Commit Message Format

```
<type>(<scope>): <summary>

<body — optional, explains WHY>
```

### Types

| Type | Use for |
|------|---------|
| `driver` | peripheral driver implementation |
| `hw` | hardware config, register map, pin assignment |
| `feat` | new application-level feature |
| `fix` | bug fix |
| `refactor` | restructuring without behaviour change |
| `docs` | documentation only |
| `chore` | maintenance, config |

### Scopes (optional)

`uart` · `i2c` · `tmp117` · `max30102` · `filter` · `main` · `gpio`

### Examples

```
driver(uart): implement USART2 TX at 9600 baud on PA2 AF7
fix(uart): correct BRR register offset to 0x0C, was writing to CR3
hw(uart): set BRR=417 for 9600 baud at 4MHz MSI clock
driver(i2c): init I2C1 on PB8/PB9, 100kHz standard mode
driver(tmp117): read device ID 0x0117 over I2C to verify comms
refactor: split monolith into 3-layer architecture
docs: document all register addresses against RM0351
```

### Rules

- Summary under 72 characters, imperative mood — "add" not "added"
- One peripheral, one register block, or one fix per commit
- Body explains **why** — especially useful when a bug cost hours to find
- Blank line between summary and body

---

## Branching Strategy

```
main          verified working builds only — UART confirmed, no regressions
driver/       new peripheral driver    e.g. driver/tmp117, driver/max30102
fix/          bug fixes                e.g. fix/baud-rate-register
refactor/     restructuring            e.g. refactor/3-layer-arch
experiment/   throwaway work           never merge directly into main
```

### Workflow

1. Branch off `main` → `driver/<peripheral>` or `fix/<name>`
2. Make atomic commits on the branch
3. Merge into `main` only when the driver/fix is verified on hardware
4. Delete the branch after merging

---

## Atomic Commits

Each commit should do exactly one thing and leave the firmware in a compilable state.

**Good:**
```
hw(gpio): enable GPIOA clock in RCC_AHB2ENR
hw(uart): configure PA2 as AF7 in GPIOA_AFRL
driver(uart): implement blocking TX with TXE polling
```

**Bad:**
```
uart stuff
made it work finally
WIP
```

The Git history on this repo is intentionally preserved — the prototype commits before the refactor commit tell the story of how the architecture evolved.
