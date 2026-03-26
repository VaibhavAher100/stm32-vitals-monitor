# Wiring Reference — STM32 Vitals Monitor

## Safety Rules — Read Before Wiring

1. **Always unplug the USB cable before connecting or disconnecting any wires.**
   Hot-wiring with incorrect connections can permanently damage the sensor or the Nucleo board.

2. **Always connect GND first and disconnect it last.**
   This prevents floating voltage states that can damage components.

3. **Do not exceed 3.3V on any sensor pin.**
   Both the TMP117 and MAX30102 are 3.3V devices. The Nucleo 5V pin must never be connected
   to any sensor power or signal pin.

4. **Check wiring before powering on.**
   A misplaced SDA/SCL wire will prevent communication.
   A misplaced power/GND wire can permanently damage a sensor.

---

## Nucleo-L476RG Pin Headers

The Nucleo board has two main connector rows: **CN5** (top) and **CN6** (bottom right).

### CN5 — Left connector (Arduino-compatible)

```
CN5 Pin 1  →  PB8  / D15  (I2C1_SCL)
CN5 Pin 2  →  PB9  / D14  (I2C1_SDA)
CN5 Pin 3  →  AVDD
CN5 Pin 4  →  GND
CN5 Pin 5  →  PA5  / D13  (LED LD2 / SPI_SCK)
CN5 Pin 6  →  PA6  / D12
CN5 Pin 7  →  PA7  / D11
CN5 Pin 8  →  PB6  / D10
CN5 Pin 9  →  PC7  / D9
CN5 Pin 10 →  PA9  / D8
```

### CN6 — Right connector (Power)

```
CN6 Pin 1  →  PC10 / D0
CN6 Pin 2  →  PC11 / D1
CN6 Pin 3  →  PC12 / D2
CN6 Pin 4  →  3.3V         ← SENSOR POWER
CN6 Pin 5  →  PC13 (User button)
CN6 Pin 6  →  GND           ← SENSOR GROUND
CN6 Pin 7  →  PB7
CN6 Pin 8  →  PA15
```

---

## TMP117 — Temperature Sensor

**I2C Address:** 0x49 (Soldered Electronics breakout default)

### Wiring Table

| TMP117 Pin | Nucleo Pin | Header Location | Wire Colour | Note |
|---|---|---|---|---|
| VCC | 3.3V | CN6 Pin 4 | Red | 3.3V only. Never 5V. |
| GND | GND | CN6 Pin 6 | Black | |
| SDA | PB9 | CN5 Pin 2 | Blue | I2C1 data line |
| SCL | PB8 | CN5 Pin 1 | Yellow | I2C1 clock line |

### Wiring Diagram (Text)

```
TMP117 Breakout          Nucleo-L476RG
                          CN6
VCC ────────────────────► 3.3V  (Pin 4)
GND ────────────────────► GND   (Pin 6)
                          CN5
SDA ────────────────────► PB9   (Pin 2)
SCL ────────────────────► PB8   (Pin 1)
```

### Verification

After wiring and flashing, the UART output should show:

```
TMP117 Device ID: 0x117
TMP117 detected correctly
Temp: 22 C
```

If "TMP117 not detected" appears: unplug USB, recheck all 4 wires, confirm SDA/SCL are not swapped.

---

## MAX30102 — Heart Rate and SpO2 Sensor

**I2C Address:** 0x57
**Important:** Shares the same I2C bus (SDA and SCL wires) as the TMP117.
This is normal I2C bus behaviour. Both sensors are distinguished by their addresses.

### Wiring Table

| MAX30102 Pin | Nucleo Pin | Header Location | Wire Colour | Note |
|---|---|---|---|---|
| VIN | 3.3V | CN6 Pin 4 | Red | Shared 3.3V rail with TMP117 |
| GND | GND | CN6 Pin 6 | Black | Shared GND rail with TMP117 |
| SDA | PB9 | CN5 Pin 2 | Blue | Shared SDA with TMP117 |
| SCL | PB8 | CN5 Pin 1 | Yellow | Shared SCL with TMP117 |
| INT | Not connected | — | — | Interrupt pin — not used in this phase |
| IRD | Not connected | — | — | IR LED ground — internal, no connection needed |
| RD | Not connected | — | — | Red LED ground — internal, no connection needed |

### Wiring Diagram (Text) — Both Sensors Connected

```
TMP117 Breakout          Nucleo-L476RG           MAX30102 Breakout
                          CN6
VCC ────────────────────► 3.3V  (Pin 4) ◄──────── VIN
GND ────────────────────► GND   (Pin 6) ◄──────── GND
                          CN5
SDA ────────────────────► PB9   (Pin 2) ◄──────── SDA
SCL ────────────────────► PB8   (Pin 1) ◄──────── SCL
```

### Verification

After wiring and flashing, the UART output should show:

```
MAX30102 Part ID: 0x15
MAX30102 detected correctly
IR raw: 45231
IR raw: 45187
```

---

## Breadboard Layout

Use the 400-point mini breadboard to organise connections:

1. Place both sensor breakout boards on the breadboard — one on each side
2. Use the breadboard power rails to distribute 3.3V and GND
3. Connect CN6 Pin 4 (3.3V) to the positive power rail
4. Connect CN6 Pin 6 (GND) to the negative power rail
5. Connect each sensor VCC to the positive rail
6. Connect each sensor GND to the negative rail
7. Connect SDA and SCL from both sensors directly to CN5 pins

---

## Dupont Wire Reference

| Wire Type | Female end | Male end | Usage |
|---|---|---|---|
| Female-to-Male | Connects to sensor breakout header pins | Connects to Nucleo pin headers | All sensor connections in this project |

Both ends must seat fully. A partially inserted Dupont connector causes intermittent I2C failures
that are difficult to diagnose.

---

## Common Wiring Mistakes

| Mistake | Symptom | Fix |
|---|---|---|
| SDA and SCL swapped | I2C bus stuck, no device response | Swap the two wires on either sensor |
| Sensor powered from 5V instead of 3.3V | Sensor may be damaged | Replace sensor, rewire to 3.3V |
| GND not connected | No I2C communication | Check GND wire is fully seated |
| VCC not connected | Device ID reads 0x00 or 0xFF | Check VCC wire is fully seated |
| Wrong I2C address in code | Device ID mismatch | Verify address matches sensor breakout variant |

---

*Last updated: March 2026*
*Hardware: Nucleo-L476RG, TMP117 Soldered breakout (333175), AEDIKO MAX30102 breakout*
*Reference: UM1724 STM32 Nucleo-64 User Manual, TMP117 Datasheet, MAX30102 Datasheet*
