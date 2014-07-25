/** \file Access to AVR pins via constants.
 *
 * ddr(0xA3, 1) == enable DDRA |= (1 << 3)
 * out(0xA3, 1) == PORTA |= (1 << 3)
 * in(0xA3) == PINA & (1 << 3)
 */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "bits.h"

#define set_bit(PORT, PIN, VALUE) do { \
		if (VALUE) \
			sbi(PORT, PIN); \
		else \
			cbi(PORT, PIN); \
	} while (0)

#define get_bit(PORT, PIN) \
	((PORT) & (1 << PIN))


void
ddr(
	const uint8_t id,
	const uint8_t value
)
{
	const uint8_t port = (id >> 4) & 0xF;
	const uint8_t pin = (id >> 0) & 0xF;

	switch (port)
	{
	case 0xA:
		set_bit(DDRA, pin, value);
		return;
	case 0xB:
		set_bit(DDRB, pin, value);
		return;
	case 0xC:
		set_bit(DDRC, pin, value);
		return;
	case 0xD:
		set_bit(DDRD, pin, value);
		return;
	case 0xE:
		set_bit(DDRE, pin, value);
		return;
	case 0xF:
		set_bit(DDRF, pin, value);
		return;
	}
}


void
out(
	const uint8_t id,
	const uint8_t value
)
{
	const uint8_t port = (id >> 4) & 0xF;
	const uint8_t pin = (id >> 0) & 0xF;

	switch (port)
	{
	case 0xA:
		set_bit(PORTA, pin, value);
		return;
	case 0xB:
		set_bit(PORTB, pin, value);
		return;
	case 0xC:
		set_bit(PORTC, pin, value);
		return;
	case 0xD:
		set_bit(PORTD, pin, value);
		return;
	case 0xE:
		set_bit(PORTE, pin, value);
		return;
	case 0xF:
		set_bit(PORTF, pin, value);
		return;
	}
}


uint8_t
in(
	const uint8_t id
)
{
	const uint8_t port = (id >> 4) & 0xF;
	const uint8_t pin = (id >> 0) & 0xF;

	switch (port)
	{
	case 0xA:
		return get_bit(PINA, pin);
	case 0xB:
		return get_bit(PINB, pin);
	case 0xC:
		return get_bit(PINC, pin);
	case 0xD:
		return get_bit(PIND, pin);
	case 0xE:
		return get_bit(PINE, pin);
	case 0xF:
		return get_bit(PINF, pin);
	}

	return 0;
}
