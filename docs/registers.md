# Register Reference - STM32 Vitals Monitor

All addresses and bit descriptions are sourced from:
**RM0351** - STM32L47xxx/L48xxx/L49xxx/L4Axxx Reference Manual
**UM1724** - STM32 Nucleo-64 Boards User Manual

---

## Critical Note - USART2_BRR Address

> **The USART2 Baud Rate Register (BRR) is at offset 0x0C from the USART2 base address.**
> Absolute address: `0x4000440C`
>
> Address `0x40004408` is **CR3** (Control Register 3), NOT BRR.
> Writing a baud rate value to CR3 silently corrupts UART configuration.
> This was the root cause of several hours of debugging in this project.
> Always verify register offsets against the RM0351 register map table.

---

## RCC - Reset and Clock Control

Base address: `0x40021000`

### RCC_AHB2ENR - AHB2 Peripheral Clock Enable Register
Address: `0x4002104C` | RM0351 Section 6.4.17

| Bit | Name | Value Set | Reason |
|---|---|---|---|
| 0 | GPIOAEN | 1 | Enable GPIOA peripheral clock before any GPIOA register access |
| 1 | GPIOBEN | 1 | Enable GPIOB peripheral clock for I2C1 pins PB8/PB9 |

### RCC_APB1ENR1 - APB1 Peripheral Clock Enable Register 1
Address: `0x40021058` | RM0351 Section 6.4.21

| Bit | Name | Value Set | Reason |
|---|---|---|---|
| 17 | USART2EN | 1 | Enable USART2 peripheral clock for UART transmission |
| 21 | I2C1EN | 1 | Enable I2C1 peripheral clock for sensor communication |

### RCC_APB1RSTR1 - APB1 Peripheral Reset Register 1
Address: `0x40021038`

| Bit | Name | Usage | Reason |
|---|---|---|---|
| 17 | USART2RST | Set then clear | Reset USART2 to known state before configuration |

---

## GPIOA - General Purpose Input/Output Port A

Base address: `0x48000000`

### GPIOA_MODER - Port Mode Register
Address: `0x48000000` | RM0351 Section 8.4.1

Mode values: `00` = Input, `01` = Output, `10` = Alternate Function, `11` = Analog

| Bits | Pin | Value Set | Reason |
|---|---|---|---|
| 5:4 | PA2 | 10 | Alternate Function mode - PA2 carries USART2_TX signal |
| 11:10 | PA5 | 01 | Output mode - PA5 drives LD2 green LED (debug heartbeat) |

### GPIOA_AFRL - Alternate Function Low Register
Address: `0x48000020` | RM0351 Section 8.4.9

Controls alternate function selection for pins 0–7. Each pin uses 4 bits.

| Bits | Pin | Value Set | AF | Reason |
|---|---|---|---|---|
| 11:8 | PA2 | 0111 (AF7) | USART2_TX | Routes PA2 to USART2 transmit function |

### GPIOA_ODR - Output Data Register
Address: `0x48000014` | RM0351 Section 8.4.6

| Bit | Pin | Usage | Reason |
|---|---|---|---|
| 5 | PA5 | Toggle | Controls LD2 LED state. Set = LED on. Clear = LED off. |

---

## GPIOB - General Purpose Input/Output Port B

Base address: `0x48000400`

### GPIOB_MODER - Port Mode Register
Address: `0x48000400` | RM0351 Section 8.4.1

| Bits | Pin | Value Set | Reason |
|---|---|---|---|
| 17:16 | PB8 | 10 | Alternate Function mode - PB8 carries I2C1_SCL |
| 19:18 | PB9 | 10 | Alternate Function mode - PB9 carries I2C1_SDA |

### GPIOB_OTYPER - Output Type Register
Address: `0x48000404` | RM0351 Section 8.4.2

`0` = Push-pull, `1` = Open-drain. **I2C requires open-drain.**

| Bit | Pin | Value Set | Reason |
|---|---|---|---|
| 8 | PB8 | 1 | Open-drain output required by I2C specification |
| 9 | PB9 | 1 | Open-drain output required by I2C specification |

### GPIOB_OSPEEDR - Output Speed Register
Address: `0x48000408` | RM0351 Section 8.4.3

`11` = Very high speed

| Bits | Pin | Value Set | Reason |
|---|---|---|---|
| 17:16 | PB8 | 11 | High speed to support 100 kHz I2C transitions cleanly |
| 19:18 | PB9 | 11 | High speed to support 100 kHz I2C transitions cleanly |

### GPIOB_AFRH - Alternate Function High Register
Address: `0x48000424` | RM0351 Section 8.4.10

Controls alternate function for pins 8–15. Each pin uses 4 bits.

| Bits | Pin | Value Set | AF | Reason |
|---|---|---|---|---|
| 3:0 | PB8 | 0100 (AF4) | I2C1_SCL | Routes PB8 to I2C1 clock line |
| 7:4 | PB9 | 0100 (AF4) | I2C1_SDA | Routes PB9 to I2C1 data line |

---

## USART2

Base address: `0x40004400`

### USART2_CR1 - Control Register 1
Address: `0x40004400` | RM0351 Section 40.8.1

| Bit | Name | Value Set | Reason |
|---|---|---|---|
| 0 | UE | 1 | USART Enable - activates the peripheral |
| 3 | TE | 1 | Transmitter Enable - enables TX output |

Set TE before UE to ensure the transmitter is ready before the peripheral activates.

### USART2_BRR - Baud Rate Register
Address: **`0x4000440C`** | RM0351 Section 40.8.4 | **Offset 0x0C**

| Value | Clock | Baud Rate | Calculation |
|---|---|---|---|
| 417 | 4 MHz | 9600 | 4,000,000 / 9600 = 416.67 → 417 |

### USART2_ISR - Interrupt and Status Register
Address: `0x4000441C` | RM0351 Section 40.8.10

| Bit | Name | Usage | Reason |
|---|---|---|---|
| 7 | TXE | Poll until 1 | Transmit data register empty. Write next byte only when this is 1. |

### USART2_TDR - Transmit Data Register
Address: `0x40004428` | RM0351 Section 40.8.15

Write one byte (8 bits) here to transmit. Hardware serialises and transmits at configured baud rate.

---

## I2C1

Base address: `0x40005400`

### I2C1_CR1 - Control Register 1
Address: `0x40005400` | RM0351 Section 37.7.1

| Bit | Name | Value Set | Reason |
|---|---|---|---|
| 0 | PE | 0 then 1 | Disable before configuring TIMINGR. Enable after all configuration is complete. |

### I2C1_TIMINGR - Timing Register
Address: `0x40005410` | RM0351 Section 37.7.5

| Value | Clock | Mode | Reason |
|---|---|---|---|
| 0x00100D14 | 4 MHz | Standard 100 kHz | Derived from STM32CubeMX I2C timing calculator for 4 MHz MSI clock |

### I2C1_CR2 - Control Register 2
Address: `0x40005404` | RM0351 Section 37.7.2

Used to initiate each I2C transaction. Fields set per transaction:

| Bits | Name | Usage |
|---|---|---|
| 9:1 | SADD | Slave address (7-bit, shifted left by 1) |
| 10 | RD_WRN | 0 = Write, 1 = Read |
| 23:16 | NBYTES | Number of bytes to transfer |
| 13 | START | Set 1 to generate START condition |
| 14 | STOP | Set 1 to generate STOP condition |
| 25 | AUTOEND | Set 1 to auto-generate STOP after NBYTES |

### I2C1_ISR - Interrupt and Status Register
Address: `0x40005418` | RM0351 Section 37.7.7

| Bit | Name | Usage |
|---|---|---|
| 1 | TXIS | TX data register empty and ready for next byte |
| 2 | RXNE | RX data register not empty - byte available to read |
| 6 | TC | Transfer complete - all bytes transferred |
| 15 | BUSY | I2C bus is busy - wait before initiating new transaction |

### I2C1_TXDR - Transmit Data Register
Address: `0x40005428`

Write one byte here when TXIS flag is set.

### I2C1_RXDR - Receive Data Register
Address: `0x40005424`

Read one byte from here when RXNE flag is set.

---

## Sensor Register Maps

### TMP117 - I2C Address: 0x49 (Soldered breakout default)

| Register | Address | R/W | Description |
|---|---|---|---|
| Temperature | 0x00 | R | 16-bit signed. Resolution: 0.0078125°C/LSB. Cast to int16_t. |
| Configuration | 0x01 | R/W | Conversion mode, averaging, alert settings |
| T_HIGH Limit | 0x02 | R/W | Upper temperature alert threshold |
| T_LOW Limit | 0x03 | R/W | Lower temperature alert threshold |
| EEPROM Unlock | 0x04 | R/W | Unlock EEPROM for writing |
| Device ID | 0x0F | R | Read-only. Expected: 0x0117. Use to verify I2C connection. |

Temperature conversion: `celsius = (int16_t)raw_register * 0.0078125`

### MAX30102 - I2C Address: 0x57

| Register | Address | R/W | Description |
|---|---|---|---|
| Interrupt Status 1 | 0x00 | R | Flags: PPG ready, ALC overflow, temperature ready |
| Interrupt Status 2 | 0x01 | R | Die temperature ready flag |
| FIFO Write Pointer | 0x04 | R/W | Next FIFO write position |
| FIFO Read Pointer | 0x06 | R/W | Next FIFO read position |
| FIFO Data | 0x07 | R | Read 3 bytes for red, 3 bytes for IR per sample |
| FIFO Configuration | 0x08 | R/W | Sample averaging, FIFO rollover, almost-full threshold |
| Mode Configuration | 0x09 | R/W | 0x02 = HR mode (IR only). 0x03 = SpO2 mode (IR + Red). |
| SpO2 Configuration | 0x0A | R/W | ADC range, sample rate, LED pulse width |
| LED1 Pulse Amp | 0x0C | R/W | Red LED current (0x00–0xFF, 0mA–51mA) |
| LED2 Pulse Amp | 0x0D | R/W | IR LED current |
| Die Temp Integer | 0x1F | R | Integer part of internal die temperature |
| Die Temp Fraction | 0x20 | R | Fractional part (0.0625°C resolution) |
| Part ID | 0xFF | R | Read-only. Expected: 0x15. Use to verify I2C connection. |

---

*Last updated: March 2026*
*Source: RM0351 Rev 8, TMP117 Datasheet Rev D, MAX30102 Datasheet Rev 1*
