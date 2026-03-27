#include <stdint.h>

#define RCC_AHB2ENR    (*((volatile uint32_t*)0x4002104C))
#define RCC_APB1ENR1   (*((volatile uint32_t*)0x40021058))
#define GPIOA_MODER    (*((volatile uint32_t*)0x48000000))
#define GPIOA_AFRL     (*((volatile uint32_t*)0x48000020))
#define GPIOB_MODER    (*((volatile uint32_t*)0x48000400))
#define GPIOB_OTYPER   (*((volatile uint32_t*)0x48000404))
#define GPIOB_OSPEEDR  (*((volatile uint32_t*)0x48000408))
#define GPIOB_AFRH     (*((volatile uint32_t*)0x48000424))
#define USART2_CR1     (*((volatile uint32_t*)0x40004400))
#define USART2_BRR     (*((volatile uint32_t*)0x4000440C))
#define USART2_ISR     (*((volatile uint32_t*)0x4000441C))
#define USART2_TDR     (*((volatile uint32_t*)0x40004428))
#define I2C1_CR1       (*((volatile uint32_t*)0x40005400))
#define I2C1_CR2       (*((volatile uint32_t*)0x40005404))
#define I2C1_TIMINGR   (*((volatile uint32_t*)0x40005410))
#define I2C1_ISR       (*((volatile uint32_t*)0x40005418))
#define I2C1_ICR       (*((volatile uint32_t*)0x4000541C))
#define I2C1_TXDR      (*((volatile uint32_t*)0x40005428))
#define I2C1_RXDR      (*((volatile uint32_t*)0x40005424))

#define TMP117_ADDR     0x49U
#define TMP117_REG_TEMP 0x00U
#define TMP117_REG_ID   0x0FU

static void delay(volatile uint32_t n) { while(n--) {} }

static void uart_init(void)
{
    RCC_AHB2ENR  |= (1U << 0);
    RCC_APB1ENR1 |= (1U << 17);
    GPIOA_MODER  &= ~(3U << 4); GPIOA_MODER |= (2U << 4);
    GPIOA_AFRL   &= ~(0xFU << 8); GPIOA_AFRL |= (7U << 8);
    USART2_BRR    = 417U;
    USART2_CR1   |= (1U << 3);
    USART2_CR1   |= (1U << 0);
}

static void uart_char(char c)
{
    while(!(USART2_ISR & (1U << 7))) {}
    USART2_TDR = (uint8_t)c;
}

static void uart_str(const char *s) { while(*s) uart_char(*s++); }

static void uart_int(int32_t v)
{
    char buf[12]; uint8_t i = 0;
    if(v < 0) { uart_char('-'); v = -v; }
    if(v == 0) { uart_char('0'); return; }
    while(v > 0) { buf[i++] = '0' + (v % 10); v /= 10; }
    while(i > 0) uart_char(buf[--i]);
}

static void uart_hex(uint8_t v)
{
    const char *h = "0123456789ABCDEF";
    uart_char(h[v >> 4]);
    uart_char(h[v & 0xF]);
}

static void i2c_init(void)
{
    RCC_AHB2ENR  |= (1U << 1);
    RCC_APB1ENR1 |= (1U << 21);
    delay(100);
    GPIOB_MODER  &= ~(3U << 16); GPIOB_MODER |= (2U << 16);
    GPIOB_MODER  &= ~(3U << 18); GPIOB_MODER |= (2U << 18);
    GPIOB_OTYPER  |= (1U << 8) | (1U << 9);
    GPIOB_OSPEEDR |= (3U << 16) | (3U << 18);
    GPIOB_AFRH   &= ~(0xFFU);
    GPIOB_AFRH   |= (4U) | (4U << 4);
    I2C1_CR1     &= ~(1U << 0);
    delay(100);
    I2C1_TIMINGR  = 0x00100D14U;
    I2C1_CR1     |= (1U << 0);
    delay(100);
}

/* Write one register pointer byte to sensor */
static uint8_t i2c_write_reg(uint8_t addr, uint8_t reg)
{
    volatile uint32_t timeout;

    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}

    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 16);
    I2C1_CR2 |= (1U << 13);

    timeout = 50000;
    while(timeout--)
    {
        uint32_t isr = I2C1_ISR;
        if(isr & (1U << 4)) { I2C1_ICR = (1U << 4); I2C1_CR2 |= (1U << 14); return 0; }
        if(isr & (1U << 1)) { I2C1_TXDR = reg; break; }
    }
    if(!timeout) return 0;

    timeout = 50000;
    while(!(I2C1_ISR & (1U << 6)) && timeout--) {}
    I2C1_CR2 |= (1U << 14);
    delay(10);
    return 1;
}

/* Read 2 bytes from sensor */
static uint16_t i2c_read_2bytes(uint8_t addr)
{
    volatile uint32_t timeout;
    uint16_t result = 0;

    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}

    I2C1_CR2 = ((uint32_t)addr << 1)
             | (1U << 10)   /* read direction */
             | (2U << 16)   /* 2 bytes */
             | (1U << 25);  /* autoend */
    I2C1_CR2 |= (1U << 13);

    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result = (uint16_t)(I2C1_RXDR << 8);

    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= (uint16_t)I2C1_RXDR;

    delay(10);
    return result;
}

int main(void)
{
    uart_init();
    i2c_init();

    uart_str("========================\r\n");
    uart_str("STM32 Vitals Monitor\r\n");
    uart_str("UART OK  I2C OK\r\n");
    uart_str("========================\r\n");

    /* Read and verify device ID */
    i2c_write_reg(TMP117_ADDR, TMP117_REG_ID);
    uint16_t id = i2c_read_2bytes(TMP117_ADDR);

    uart_str("TMP117 ID: 0x");
    uart_hex((uint8_t)(id >> 8));
    uart_hex((uint8_t)(id & 0xFF));
    uart_str("\r\n");

    if(id == 0x0117U)
        uart_str("TMP117 OK\r\n");
    else
        uart_str("TMP117 ID wrong - check wiring\r\n");

    uart_str("========================\r\n");

    while(1)
    {
        i2c_write_reg(TMP117_ADDR, TMP117_REG_TEMP);
        uint16_t raw = i2c_read_2bytes(TMP117_ADDR);

        /* 0.0078125 deg C per LSB — multiply by 78, divide by 10000 */
        int32_t temp_c  = ((int16_t)raw * 78) / 10000;

        /* Fractional part — one decimal place */
        int32_t temp_f  = ((int16_t)raw * 78) % 10000;
        if(temp_f < 0) temp_f = -temp_f;
        temp_f = temp_f / 1000;

        uart_str("Temp: ");
        uart_int(temp_c);
        uart_char('.');
        uart_int(temp_f);
        uart_str(" C\r\n");

        delay(1000000);
    }
}
