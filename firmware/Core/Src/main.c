#include <stdint.h>

/* ── Register definitions ─────────────────────────── */
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

/* ── Sensor addresses and registers ──────────────── */
#define TMP117_ADDR          0x49U
#define TMP117_REG_TEMP      0x00U
#define TMP117_REG_ID        0x0FU

#define MAX30102_ADDR        0x57U
#define MAX30102_REG_INTR1   0x00U
#define MAX30102_REG_FIFO_WR 0x04U
#define MAX30102_REG_FIFO_RD 0x06U
#define MAX30102_REG_FIFO_DA 0x07U
#define MAX30102_REG_FIFO_CF 0x08U
#define MAX30102_REG_MODE    0x09U
#define MAX30102_REG_SPO2    0x0AU
#define MAX30102_REG_LED1    0x0CU
#define MAX30102_REG_PARTID  0xFFU

/* ── Moving average filter ───────────────────────── */
#define FILTER_WINDOW 8

typedef struct {
    uint32_t buf[FILTER_WINDOW];
    uint8_t  idx;
    uint8_t  count;
    uint32_t sum;
} Filter;

static void filter_init(Filter *f)
{
    uint8_t i;
    for(i = 0; i < FILTER_WINDOW; i++) f->buf[i] = 0;
    f->idx   = 0;
    f->count = 0;
    f->sum   = 0;
}

static uint32_t filter_update(Filter *f, uint32_t val)
{
    f->sum -= f->buf[f->idx];
    f->buf[f->idx] = val;
    f->sum += val;
    f->idx = (f->idx + 1) % FILTER_WINDOW;
    if(f->count < FILTER_WINDOW) f->count++;
    return f->sum / f->count;
}

/* ── Peripherals ─────────────────────────────────── */
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

static uint8_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    volatile uint32_t timeout;
    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (2U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    if(!timeout) return 0;
    I2C1_TXDR = reg;
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    if(!timeout) return 0;
    I2C1_TXDR = val;
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 5)) && timeout--) {}
    delay(10);
    return 1;
}

static uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
{
    volatile uint32_t timeout;
    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 16);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    I2C1_TXDR = reg;
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 6)) && timeout--) {}
    I2C1_CR2 |= (1U << 14);
    delay(10);
    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 10) | (1U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    uint8_t val = (uint8_t)I2C1_RXDR;
    delay(10);
    return val;
}

static uint16_t i2c_read_2bytes(uint8_t addr, uint8_t reg)
{
    volatile uint32_t timeout;
    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 16);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    I2C1_TXDR = reg;
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 6)) && timeout--) {}
    I2C1_CR2 |= (1U << 14);
    delay(10);
    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)addr << 1) | (1U << 10) | (2U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);
    uint16_t result = 0;
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result = (uint16_t)(I2C1_RXDR << 8);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= (uint16_t)I2C1_RXDR;
    delay(10);
    return result;
}

static uint32_t max30102_read_fifo(void)
{
    volatile uint32_t timeout;
    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)MAX30102_ADDR << 1) | (1U << 16);
    I2C1_CR2 |= (1U << 13);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 1)) && timeout--) {}
    I2C1_TXDR = MAX30102_REG_FIFO_DA;
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 6)) && timeout--) {}
    I2C1_CR2 |= (1U << 14);
    delay(10);
    I2C1_ICR = 0x3F38U;
    timeout = 50000;
    while((I2C1_ISR & (1U << 15)) && timeout--) {}
    I2C1_CR2 = ((uint32_t)MAX30102_ADDR << 1) | (1U << 10) | (3U << 16) | (1U << 25);
    I2C1_CR2 |= (1U << 13);
    uint32_t result = 0;
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result = ((uint32_t)I2C1_RXDR << 16);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= ((uint32_t)I2C1_RXDR << 8);
    timeout = 50000;
    while(!(I2C1_ISR & (1U << 2)) && timeout--) {}
    result |= (uint32_t)I2C1_RXDR;
    result &= 0x3FFFFU;
    delay(10);
    return result;
}

int main(void)
{
    uart_init();
    i2c_init();

    uart_str("========================\r\n");
    uart_str("STM32 Vitals Monitor\r\n");
    uart_str("========================\r\n");

    /* Verify TMP117 */
    uint16_t tmp_id = i2c_read_2bytes(TMP117_ADDR, TMP117_REG_ID);
    uart_str("TMP117  ID: 0x");
    uart_hex((uint8_t)(tmp_id >> 8));
    uart_hex((uint8_t)(tmp_id & 0xFF));
    uart_str(tmp_id == 0x0117U ? "  OK\r\n" : "  FAIL\r\n");

    /* Verify MAX30102 */
    uint8_t max_id = i2c_read_reg(MAX30102_ADDR, MAX30102_REG_PARTID);
    uart_str("MAX30102 ID: 0x");
    uart_hex(max_id);
    uart_str(max_id == 0x15U ? "  OK\r\n" : "  FAIL\r\n");

    uart_str("========================\r\n");

    /* Configure MAX30102 */
    i2c_write_reg(MAX30102_ADDR, MAX30102_REG_MODE,    0x40U); /* reset */
    delay(200000);
    i2c_write_reg(MAX30102_ADDR, MAX30102_REG_FIFO_CF, 0x4FU); /* 4 avg, rollover on */
    i2c_write_reg(MAX30102_ADDR, MAX30102_REG_MODE,    0x02U); /* HR mode */
    i2c_write_reg(MAX30102_ADDR, MAX30102_REG_SPO2,    0x27U); /* 100sps, 411us */
    i2c_write_reg(MAX30102_ADDR, MAX30102_REG_LED1,    0x1FU); /* 6.4mA IR LED */
    i2c_write_reg(MAX30102_ADDR, MAX30102_REG_FIFO_WR, 0x00U);
    i2c_write_reg(MAX30102_ADDR, MAX30102_REG_FIFO_RD, 0x00U);

    uart_str("Both sensors ready\r\n");
    uart_str("Place finger on MAX30102\r\n");
    uart_str("========================\r\n");
    uart_str("Temp(C) | IR raw  | IR filt\r\n");
    uart_str("--------+---------+--------\r\n");

    /* Init moving average filter */
    Filter ir_filter;
    filter_init(&ir_filter);

    while(1)
    {
        /* Read TMP117 temperature */
        uint16_t raw_temp = i2c_read_2bytes(TMP117_ADDR, TMP117_REG_TEMP);
        int32_t temp_c = ((int16_t)raw_temp * 78) / 10000;
        int32_t temp_f = (((int16_t)raw_temp * 78) % 10000);
        if(temp_f < 0) temp_f = -temp_f;
        temp_f /= 1000;

        /* Read MAX30102 IR */
        uint32_t ir_raw      = max30102_read_fifo();
        uint32_t ir_filtered = filter_update(&ir_filter, ir_raw);

        /* Print formatted row */
        uart_int(temp_c);
        uart_char('.');
        uart_int(temp_f);
        uart_str("       | ");
        uart_int((int32_t)ir_raw);
        uart_str("  | ");
        uart_int((int32_t)ir_filtered);
        uart_str("\r\n");

        delay(500000);
    }
}
