/**
 * \file PROM dancer
 *
 * Read up to 40 pin DIP PROMs using a Teensy++ 2.0
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>
#include "usb_serial.h"
#include "bits.h"

void send_str(const char *s);
uint8_t recv_str(char *buf, uint8_t size);

static uint8_t
hexdigit(
	uint8_t x
)
{
	x &= 0xF;
	if (x < 0xA)
		return x + '0' - 0x0;
	else
		return x + 'A' - 0xA;
}


static uint8_t
printable(
	uint8_t x
)
{
	if ('A' <= x && x <= 'Z')
		return 1;
	if ('a' <= x && x <= 'z')
		return 1;
	if ('0' <= x && x <= '9')
		return 1;
	if (x == ' ')
		return 1;

	return 0;
}


static const uint8_t io_lines[8] = {
  0xF0, 0xF1, 0xF2, 0xF3,
  0xF4, 0xF5, 0xF6, 0xF7,
};

#define ADDR_WIDTH 13
static const uint8_t addr_lines[ADDR_WIDTH] = {
  0xB7, 0xD0, 0xD1, 0xD2,
  0xD3, 0xD4, 0xD5, 0xD6,
  0xD7, 0xE0, 0xE1, 0xC0,
  0xC1
};

static const uint8_t ce_line = 0xB6;
static const uint8_t oe_line = 0xB5;
static const uint8_t we_line = 0xB4;

static void prom_setup(bool read)
{
  // First make sure that chip enable is off-- no accidental writes please
  out(ce_line,1); ddr(ce_line,1);
  out(we_line,1); ddr(we_line,1);
  out(oe_line,1); ddr(oe_line,1);

  // Configure all of the address pins as outputs
  for (uint8_t i = 0; i < ADDR_WIDTH; i++) { 
    uint8_t pin = addr_lines[i];
    out(pin, 0);
    ddr(pin, 1);
  }

  // Configure all of the io pins
  for (uint8_t i = 0 ; i < 8; i++) {
    uint8_t pin = io_lines[i];
    if (read) {
      ddr(pin,0); out(pin,0);
    } else {
      out(pin,0); ddr(pin,1);
    }
  }
  // Let things stabilize for a little while
  _delay_ms(250);
}

static void prom_off_mode(void) {
  out(we_line,1); out(oe_line,1); out(ce_line,1);
}

static void prom_read_mode(void) {
  out(we_line,1); out(oe_line,0); out(ce_line,0);
}

/** Select a 13-bit address for the current PROM */
static void prom_set_address(uint16_t addr) {
  for (uint8_t i = 0 ; i < ADDR_WIDTH ; i++) {
    out(addr_lines[i], addr & 1);
    addr >>= 1;
  }
}

static void prom_set_data(uint8_t data) {
  for (uint8_t i = 0 ; i < 8 ; i++) {
    out(io_lines[i], data & 1);
    data >>= 1;
  }
}

/** Read a byte from the PROM at the specified address..
 */
static uint8_t prom_read(uint16_t addr) {
  uint8_t b = 0;
  prom_set_address(addr);
  for (uint8_t i = 0 ; i < 8  ; i++)
  {
    uint8_t bit = in(io_lines[i]) ? 0x80 : 0;
    b = (b >> 1) | bit;
  }
  return b;
}

/** Write a series of bytes */
static void prom_write_page(uint16_t addr, uint8_t* data, uint8_t len) {
  prom_setup(false);
  out(ce_line,0);
  uint8_t d;
  while (len--) {
    prom_set_address(addr++);
    d = *(data++);
    prom_set_data(d);
    out(we_line,0);
    out(we_line,1);
  }
  // Use !DATA polling
  addr--;
  prom_setup(true);
  while (1) {
    prom_read_mode();
    uint8_t r = prom_read(addr);
    if ( ((r ^ d)&0x80) == 0 ) {
      break;
    }
  }
}

static uint8_t
hexdigit_parse(
	uint8_t c
)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	if ('A' <= c && c <= 'F')
		return c - 'A' + 0xA;
	if ('a' <= c && c <= 'f')
		return c - 'a' + 0xA;
	return 0xFF;
}

static void
hex32(
	uint8_t * buf,
	uint32_t addr
)
{
	buf[7] = hexdigit(addr & 0xF); addr >>= 4;
	buf[6] = hexdigit(addr & 0xF); addr >>= 4;
	buf[5] = hexdigit(addr & 0xF); addr >>= 4;
	buf[4] = hexdigit(addr & 0xF); addr >>= 4;
	buf[3] = hexdigit(addr & 0xF); addr >>= 4;
	buf[2] = hexdigit(addr & 0xF); addr >>= 4;
	buf[1] = hexdigit(addr & 0xF); addr >>= 4;
	buf[0] = hexdigit(addr & 0xF); addr >>= 4;
}


static void
hexdump(
	uint16_t addr
)
{
	uint8_t buf[80];
	hex32(buf, addr);

	for (int i = 0 ; i < 16 ; i++)
	{
		uint8_t w = prom_read(addr++);
		uint8_t x = 8 + i * 3;
		buf[x+0] = ' ';
		buf[x+1] = hexdigit(w >> 4);
		buf[x+2] = hexdigit(w >> 0);

		buf[8 + 16*3 + i + 2] = printable(w) ? w : '.';
	}

	buf[8 + 16 * 3 + 0] = ' ';
	buf[8 + 16 * 3 + 1] = ' ';
	buf[8 + 16 * 3 + 18] = '\r';
	buf[8 + 16 * 3 + 19] = '\n';

	usb_serial_write(buf, 8 + 16 * 3 + 20);
}

int main(void)
{
  // set for 16 MHz clock
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
  CPU_PRESCALE(0);
  
  // Disable the ADC
  ADMUX = 0;
  
  prom_setup(true); // set up in read mode
  prom_read_mode();
  
  // initialize the USB, and then wait for the host
  // to set configuration.  If the Teensy is powered
  // without a PC connected to the USB port, this 
  // will wait forever.
  usb_init();
  while (!usb_configured()) /* wait */ ;
  _delay_ms(1000);
  
  // wait for the user to run their terminal emulator program
  // which sets DTR to indicate it is ready to receive.
  while (!(usb_serial_get_control() & USB_SERIAL_DTR))
    continue;
  
  
  // discard anything that was received prior.  Sometimes the
  // operating system or other software will send a modem
  // "AT command", which can still be buffered.
  usb_serial_flush_input();
  
  while (1) {
    send_str(PSTR("READY\r\n"));
    while (usb_serial_available() == 0) { }
    uint16_t c = usb_serial_getchar();
    if (c == 'R') {
      // do read
      prom_setup(true); // set up in read mode
      prom_read_mode();
      for (uint16_t addr = 0; addr < 8 * 1024; addr += 16) {
	hexdump(addr);
      }
      send_str(PSTR("OK\r\n"));
    } else if (c == 'W') {
      for (uint16_t j = 0; j < 8192; j+=64) {
	uint8_t data[64];
	for (int i = 0; i < 64; i++) { 
	  while (usb_serial_available() == 0) { }
	  data[i] = usb_serial_getchar();
	}
	prom_write_page(j,data,64);
      }
      send_str(PSTR("OK\r\n"));
    } else {
      send_str(PSTR("ERR\r\n"));
    }
  }
}


// Send a string to the USB serial port.  The string must be in
// flash memory, using PSTR
//
void send_str(const char *s)
{
  char c;
  while (1) {
    c = pgm_read_byte(s++);
    if (!c) break;
    usb_serial_putchar(c);
  }
}

void send_mem_str(const char *s)
{
  while (1) {
    char c = *(s++);
    if (!c) break;
    usb_serial_putchar(c);
  }
}
