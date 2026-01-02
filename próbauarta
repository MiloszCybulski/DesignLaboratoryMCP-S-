#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
/* ==== ATmega328PB TWI FIX ==== */
#define TWCR TWCR0
#define TWBR TWBR0
#define TWSR TWSR0
#define TWDR TWDR0
/* ================= UART ================= */
#define BAUD 9600
#define UBRR_VAL ((F_CPU/16/BAUD)-1)
void uart_init(void)
{
    UBRR0H = (uint8_t)(UBRR_VAL >> 8);
    UBRR0L = (uint8_t)UBRR_VAL;
    UCSR0B = (1<<TXEN0)|(1<<RXEN0);
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}
void uart_putc(char c)
{
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}
void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}
char uart_getc(void)
{
    while (!(UCSR0A & (1<<RXC0)));
    return UDR0;
}
/* ================= I2C ================= */
void i2c_init(void)
{
    TWSR = 0;
    TWBR = 72;   // ~100kHz @16MHz
}
void i2c_start(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
}
void i2c_stop(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
    _delay_us(10);
}
void i2c_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
}
uint8_t i2c_read_ack(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
    while (!(TWCR & (1<<TWINT)));
    return TWDR;
}
uint8_t i2c_read_nack(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
    return TWDR;
}
/* ================= BMP280 ================= */
#define BMP280_ADDR 0x76
uint16_t dig_T1;
int16_t dig_T2, dig_T3;
uint16_t dig_P1;
int16_t dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
int32_t t_fine;
void bmp280_read_calib(void)
{
    uint8_t d[24];
    i2c_start();
    i2c_write(BMP280_ADDR<<1);
    i2c_write(0x88);
    i2c_start();
    i2c_write((BMP280_ADDR<<1)|1);
    for (uint8_t i=0;i<23;i++) d[i]=i2c_read_ack();
    d[23]=i2c_read_nack();
    i2c_stop();
    dig_T1 = (d[1]<<8)|d[0];
    dig_T2 = (d[3]<<8)|d[2];
    dig_T3 = (d[5]<<8)|d[4];
    dig_P1 = (d[7]<<8)|d[6];
    dig_P2 = (d[9]<<8)|d[8];
    dig_P3 = (d[11]<<8)|d[10];
    dig_P4 = (d[13]<<8)|d[12];
    dig_P5 = (d[15]<<8)|d[14];
    dig_P6 = (d[17]<<8)|d[16];
    dig_P7 = (d[19]<<8)|d[18];
    dig_P8 = (d[21]<<8)|d[20];
    dig_P9 = (d[23]<<8)|d[22];
}
void bmp280_init(void)
{
    i2c_start();
    i2c_write(BMP280_ADDR<<1);
    i2c_write(0xF4);
    i2c_write(0x27); // normal mode
    i2c_stop();
}
void bmp280_read_raw(int32_t *temp, int32_t *press)
{
    uint8_t d[6];
    i2c_start();
    i2c_write(BMP280_ADDR<<1);
    i2c_write(0xF7);
    i2c_start();
    i2c_write((BMP280_ADDR<<1)|1);
    for(uint8_t i=0;i<5;i++) d[i]=i2c_read_ack();
    d[5]=i2c_read_nack();
    i2c_stop();
    *press = (d[0]<<12)|(d[1]<<4)|(d[2]>>4);
    *temp  = (d[3]<<12)|(d[4]<<4)|(d[5]>>4);
}
int32_t bmp280_comp_temp(int32_t adc_T)
{
    int32_t v1 = ((((adc_T>>3)-(dig_T1<<1)))*dig_T2)>>11;
    int32_t v2 = (((((adc_T>>4)-dig_T1)*((adc_T>>4)-dig_T1))>>12)*dig_T3)>>14;
    t_fine = v1 + v2;
    return (t_fine*5+128)>>8;
}
uint32_t bmp280_comp_press(int32_t adc_P)
{
    int64_t v1 = t_fine - 128000;
    int64_t v2 = v1*v1*dig_P6;
    v2 += (v1*dig_P5)<<17;
    v2 += ((int64_t)dig_P4)<<35;
    v1 = ((v1*v1*dig_P3)>>8) + ((v1*dig_P2)<<12);
    v1 = (((int64_t)1<<47)+v1)*dig_P1>>33;
    if (v1==0) return 0;
    int64_t p = 1048576 - adc_P;
    p = (((p<<31)-v2)*3125)/v1;
    v1 = (dig_P9*(p>>13)*(p>>13))>>25;
    v2 = (dig_P8*p)>>19;
    p = ((p+v1+v2)>>8)+(dig_P7<<4);
    return p;
}
/* ================= MAIN ================= */
int main(void)
{
    uart_init();
    i2c_init();
    _delay_ms(100);
    uart_puts("UART + BMP280 READY\r\n");
    uart_puts("Commands: t p a\r\n");
    bmp280_read_calib();
    bmp280_init();
    char buf[64];
    int32_t rt, rp;
    while (1)
    {
        char c = uart_getc();
        bmp280_read_raw(&rt,&rp);
        int32_t T = bmp280_comp_temp(rt);
        uint32_t P = bmp280_comp_press(rp);
        switch(c)
        {
            case 't':
                sprintf(buf,"TEMP: %ld.%02ld C\r\n",T/100,T%100);
                uart_puts(buf);
                break;
            case 'p':
                sprintf(buf,"PRESS: %lu Pa\r\n",P);
                uart_puts(buf);
                break;
            case 'a':
                sprintf(buf,"T=%ld.%02ld C  P=%lu Pa\r\n",T/100,T%100,P);
                uart_puts(buf);
                break;
            default:
                uart_puts("Unknown cmd (t,p,a)\r\n");
        }
    }
