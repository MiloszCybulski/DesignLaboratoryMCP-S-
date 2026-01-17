#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>


/* ============================================================
   UART – Asynchronous Serial Communication
   Used for diagnostics, user commands, and sensor readout.
   ============================================================ */

int32_t rT, rP;

void led_update(uint8_t active);
void uart_puts(const char *s);



void uart_init()
{
    // Baud rate 9600 (UBRR = 103 for 16 MHz)
    UBRR0H = 0;
    UBRR0L = 103;

    UCSR0B = (1<<TXEN0)|(1<<RXEN0)|(1<<RXCIE0); 

    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
}

char uart_read()
{
	return UDR0;
}

static volatile uint8_t choosenTEMP = 0;
static volatile uint8_t choosenPRESSURE = 0;
static volatile uint8_t choosenTEMPds18b20 = 0;
static volatile uint8_t choosenLEDON = 0;
static volatile uint8_t choosenLEDOFF = 0;
static volatile uint8_t choosenRESET = 0;


ISR(USART0_RX_vect)
{
	char c = uart_read();
	
	switch(c)
	{
		case 't':
		choosenTEMP = 1;
		break;
		case 'p':
		choosenPRESSURE = 1;
		break;
		case 's':
		choosenTEMPds18b20 = 1;
		break;
		case 'd':
		choosenLEDON = 1;
		break;
		case 'a':
		choosenLEDOFF = 1;
		break;
		case 'r':
		choosenRESET = 1;
		break;
	}
	
}


void uart_putc(char c)
{
	uint8_t timeout = 0;
	while (!(UCSR0A & (1<<UDRE0)) && timeout < 100) 
	{
		_delay_us(10);  
		timeout++;
	}
	if (timeout < 100) {
		UDR0 = c;
	}
	else {
		uart_puts("UART Timeout Error\r\n");
	}
}

void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

uint8_t uart_available()
{
    return (UCSR0A & (1<<RXC0));
}


/* ============================================================
   I²C (TWI) – Communication with BMP/QMP Sensor
   ============================================================ */

#define TWCR TWCR0
#define TWBR TWBR0
#define TWSR TWSR0
#define TWDR TWDR0

void i2c_init()
{
    TWSR = 0;     
    TWBR = 72;   
}

void i2c_start()
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
}

void i2c_stop()
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
}

void i2c_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
}

uint8_t i2c_read_ack()
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
    while (!(TWCR & (1<<TWINT)));
    return TWDR;
}

uint8_t i2c_read_nack()
{
    TWCR = (1<<TWINT)|(1<<TWEN);
    while (!(TWCR & (1<<TWINT)));
    return TWDR;
}

/* ============================================================
   BMP Sensor – Initialization and Raw Data Readout
   ============================================================ */

#define SENSOR_ADDR 0x76

void sensor_init()
{
    // Control register: temperature + pressure oversampling
    i2c_start();
    i2c_write(SENSOR_ADDR<<1);
    i2c_write(0xF4);
    i2c_write(0x27);
    i2c_stop();

    // Config register
    i2c_start();
    i2c_write(SENSOR_ADDR<<1);
    i2c_write(0xF5);
    i2c_write(0x00);
    i2c_stop();
}

void sensor_read_raw(int32_t *rawT, int32_t *rawP)
{
    uint8_t d[6];

    i2c_start();
    i2c_write(SENSOR_ADDR<<1);
    i2c_write(0xF7); // first data register
    i2c_start();
    i2c_write((SENSOR_ADDR<<1)|1);

    for (uint8_t i=0;i<5;i++) d[i] = i2c_read_ack();
    d[5] = i2c_read_nack();
    i2c_stop();

    *rawP = ((int32_t)d[0]<<12) | ((int32_t)d[1]<<4) | (d[2]>>4);
    *rawT = ((int32_t)d[3]<<12) | ((int32_t)d[4]<<4) | (d[5]>>4);
}

/* ============================================================
   BMP Calibration – Simplified Scaling Model
   ============================================================ */

float T_scale  = 0.0007801f;
float T_offset = -394.5f;

int32_t temp_times100(int32_t rawT)
{
    float T = rawT * T_scale + T_offset;
    return (int32_t)(T * 100.0f);
}

float P_scale  = 0.2506f;
float P_offset = 0.0f;

int32_t press_pa(int32_t rawP)
{
    float P = rawP * P_scale + P_offset;
    return (int32_t)P;
}

/* ============================================================
   LED – Simple GPIO Indicator
   ============================================================ */

#define LED_PORT PORTC
#define LED_DDR  DDRC
#define LED_PIN  PC3

void led_init()
{
    LED_DDR |= (1<<LED_PIN);
    LED_PORT &= ~(1<<LED_PIN);
}

void led_update(uint8_t active)
{
    if (active) LED_PORT |= (1<<LED_PIN);
    else LED_PORT &= ~(1<<LED_PIN);
}

/* ============================================================
   DS18B20 – Full 1-Wire Implementation
   ============================================================ */

#define DS_PORT PORTC
#define DS_PIN  PC4
#define DS_DDR  DDRC
#define DS_PINREG PINC

static inline void ds_output() { DS_DDR |= (1<<DS_PIN); }
static inline void ds_input()  { DS_DDR &= ~(1<<DS_PIN); }

uint8_t ds_reset()
{
    ds_output();
    DS_PORT &= ~(1<<DS_PIN);
    _delay_us(480);

    ds_input();
    _delay_us(70);

    uint8_t presence = !(DS_PINREG & (1<<DS_PIN));

    _delay_us(410);
    return presence;
}

void ds_write_bit(uint8_t bit)
{
    ds_output();
    DS_PORT &= ~(1<<DS_PIN);
    _delay_us(2);

    if (bit) ds_input();
    _delay_us(60);

    ds_input();
}

uint8_t ds_read_bit()
{
    uint8_t bit;

    ds_output();
    DS_PORT &= ~(1<<DS_PIN);
    _delay_us(2);

    ds_input();
    _delay_us(10);

    bit = (DS_PINREG & (1<<DS_PIN)) ? 1 : 0;

    _delay_us(50);
    return bit;
}

void ds_write_byte(uint8_t data)
{
    for (uint8_t i=0;i<8;i++)
    {
        ds_write_bit(data & 1);
        data >>= 1;
    }
}

uint8_t ds_read_byte()
{
    uint8_t data = 0;

    for (uint8_t i=0;i<8;i++)
    {
        data >>= 1;
        if (ds_read_bit()) data |= 0x80;
    }
    return data;
}

int16_t ds_read_temperature()
{
    if (!ds_reset()) return 0;

    ds_write_byte(0xCC); // SKIP ROM
    ds_write_byte(0x44); // CONVERT T

    _delay_ms(750);

    ds_reset();
    ds_write_byte(0xCC);
    ds_write_byte(0xBE);

    uint8_t tempL = ds_read_byte();
    uint8_t tempH = ds_read_byte();

    int16_t raw = (tempH << 8) | tempL;

    float tempC = raw * 0.0625f;
    return (int16_t)(tempC * 100);
}

/* ============================================================
   7-Segment Display – Common Cathode, Multiplexed
   ============================================================ */

#define SEG_PORTB PORTB
#define SEG_DDRB  DDRB
#define SEG_PORTD PORTD
#define SEG_DDRD  DDRD

#define DIG_PORTC PORTC
#define DIG_DDRC  DDRC
#define DIG_PORTB PORTB
#define DIG_DDRB  DDRB

#define DIG1 PC1
#define DIG2 PC0
#define DIG3 PC2
#define DIG4 PB5

// Your verified segment map (common cathode)
const uint8_t segment_map[10] = {
    0b00001100, // 0
    0b10011111, // 1
    0b01001010, // 2
    0b00001011, // 3
    0b10011001, // 4
    0b00101001, // 5
    0b00101000, // 6
    0b10001111, // 7
    0b00001000, // 8
    0b00001001  // 9
};

void seg_init()
{
    SEG_DDRB |= 0x0F;
    SEG_DDRD |= 0xF0;

    DIG_DDRC |= (1<<DIG1)|(1<<DIG2)|(1<<DIG3);
    DIG_DDRB |= (1<<DIG4);

    SEG_PORTB |= 0x0F;
    SEG_PORTD |= 0xF0;

    DIG_PORTC |= (1<<DIG1)|(1<<DIG2)|(1<<DIG3);
    DIG_PORTB |= (1<<DIG4);
}

void seg_set_digit(uint8_t digit)
{
    SEG_PORTD = (SEG_PORTD & ~0xF0) | (digit & 0xF0);
    SEG_PORTB = (SEG_PORTB & ~0x0F) | (digit & 0x0F);
}

void seg_enable(uint8_t d)
{
    DIG_PORTC |= (1<<DIG1)|(1<<DIG2)|(1<<DIG3);
    DIG_PORTB |= (1<<DIG4);

    if (d==1) DIG_PORTC &= ~(1<<DIG1);
    if (d==2) DIG_PORTC &= ~(1<<DIG2);
    if (d==3) DIG_PORTC &= ~(1<<DIG3);
    if (d==4) DIG_PORTB &= ~(1<<DIG4);
}

void seg_show_4digits_once(int16_t value)
{
    uint8_t d4 = (value/1000)%10;
    uint8_t d3 = (value/100)%10;
    uint8_t d2 = (value/10)%10;
    uint8_t d1 = (value/1)%10;

    seg_set_digit(segment_map[d4]); seg_enable(1); _delay_ms(1);
    seg_set_digit(segment_map[d3]); seg_enable(2); _delay_ms(1);
    seg_set_digit(segment_map[d2]); seg_enable(3); _delay_ms(1);
    seg_set_digit(segment_map[d1]); seg_enable(4); _delay_ms(1);
}

/* ============================================================
   MAIN – System Logic
   ============================================================ */

int main()
{
    uart_init();
	uart_puts("Starting I2C communication...\r\n");
	i2c_init();
	uart_puts("Sensor initialized.\r\n");
    
    led_init();
    seg_init();
    sensor_init();
	sei();  


    uart_puts("=== SYSTEM READY ===\r\n");
    uart_puts("Available commands:\r\n");
    uart_puts(" t - read temperature (BMP/QMP)\r\n");
    uart_puts(" p - read pressure\r\n");
    uart_puts(" s - read temperature (DS18B20)\r\n");
    uart_puts(" d - LED ON\r\n");
    uart_puts(" a - LED OFF\r\n");
    uart_puts(" r - reset message\r\n");
    uart_puts("====================\r\n");



     while (1)
     {
			if (choosenTEMP)
			{
				sensor_read_raw(&rT,&rP);
				int32_t T = temp_times100(rT);

				char buf[32];
				sprintf(buf,"BMP Temp: %ld.%02ld C\r\n",T/100,T%100);
				uart_puts(buf);
				choosenTEMP = 0;
			}

			if (choosenPRESSURE)
			{
				sensor_read_raw(&rT,&rP);
				int32_t P = press_pa(rP);

				char buf[32];
				sprintf(buf,"Pressure: %ld Pa\r\n",P);
				uart_puts(buf);
				choosenPRESSURE = 0;
			}

			if (choosenTEMPds18b20)
			{
				int16_t t = ds_read_temperature();
				char buf[32];
				sprintf(buf,"DS18B20: %d.%02d C\r\n", t/100, abs(t%100));
				uart_puts(buf);
				choosenTEMPds18b20 = 0;
			}

			if (choosenLEDON)
			{
				led_update(1);
				uart_puts("LED ON\r\n");
				choosenLEDON = 0;
			}

			if (choosenLEDOFF)
			{
				led_update(0);
				uart_puts("LED OFF\r\n");
				choosenLEDOFF = 0;
			}

			if (choosenRESET)
			{
				uart_puts("RESET OK\r\n");
				uart_puts("\r\n");
				uart_puts("=== SYSTEM READY ===\r\n");
				uart_puts("Available commands:\r\n");
				uart_puts(" t - read temperature (BMP/QMP)\r\n");
				uart_puts(" p - read pressure\r\n");
				uart_puts(" s - read temperature (DS18B20)\r\n");
				uart_puts(" d - LED ON\r\n");
				uart_puts(" a - LED OFF\r\n");
				uart_puts(" r - reset message\r\n");
				uart_puts("====================\r\n");
				choosenRESET = 0;
			}
		}

		// Display temperature from BMP on 7-seg
		sensor_read_raw(&rT,&rP);
		int32_t T = temp_times100(rT);

		seg_show_4digits_once(T);
		_delay_ms(100);  
}