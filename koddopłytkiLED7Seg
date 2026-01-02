#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

/* ================= 7-SEGMENT ================= */

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

// Kolejność bitów: [d c b a dp g f e]
// Wspólna katoda → 0 = ON, 1 = OFF → odwracamy (~)
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
	SEG_DDRB |= 0x0F; // PB0–PB3 = e,f,g,dp
	SEG_DDRD |= 0xF0; // PD4–PD7 = a,b,c,d

	DIG_DDRC |= (1<<DIG1) | (1<<DIG2) | (1<<DIG3);
	DIG_DDRB |= (1<<DIG4);

	SEG_PORTB |= 0x0F; // wszystkie segmenty OFF
	SEG_PORTD |= 0xF0;

	DIG_PORTC |= (1<<DIG1) | (1<<DIG2) | (1<<DIG3);
	DIG_PORTB |= (1<<DIG4);
}

void seg_set_digit(uint8_t digit)
{
	SEG_PORTD = (SEG_PORTD & ~0xF0) | (digit & 0xF0);
	SEG_PORTB = (SEG_PORTB & ~0x0F) | (digit & 0x0F);
}

void seg_enable(uint8_t d)
{
	DIG_PORTC |= (1<<DIG1) | (1<<DIG2) | (1<<DIG3);
	DIG_PORTB |= (1<<DIG4);

	if (d == 1) DIG_PORTC &= ~(1<<DIG1);
	if (d == 2) DIG_PORTC &= ~(1<<DIG2);
	if (d == 3) DIG_PORTC &= ~(1<<DIG3);
	if (d == 4) DIG_PORTB &= ~(1<<DIG4);
}

void seg_show_digit_all(uint8_t digit)
{
	for (uint16_t t = 0; t < 500; t++) // ~0.5 sekundy
	{
		seg_set_digit(segment_map[digit]);
		seg_enable(1);
		_delay_ms(1);

		seg_set_digit(segment_map[digit]);
		seg_enable(2);
		_delay_ms(1);

		seg_set_digit(segment_map[digit]);
		seg_enable(3);
		_delay_ms(1);

		seg_set_digit(segment_map[digit]);
		seg_enable(4);
		_delay_ms(1);
	}
}

/* ================= MAIN ================= */

int main()
{
	seg_init();

	while (1)
	{
		for (uint8_t d = 0; d < 10; d++)
		{
			seg_show_digit_all(d);
		}
	}
}
