#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "lcd.h"

#define GLUE(a, b)     a##b
#define PORT(x)        GLUE(PORT, x)
#define PIN(x)         GLUE(PIN, x)
#define DDR(x)         GLUE(DDR, x)

#define HD44780_PORTOUT    PORT(HD44780_PORT)
#define HD44780_PORTIN     PIN(HD44780_PORT)
#define HD44780_DDR        DDR(HD44780_PORT)

#define HD44780_DATABITS (_BV(HD44780_D4)|_BV(HD44780_D5)|_BV(HD44780_D6)|_BV(HD44780_D7))
#define HD44780_BUSYFLAG 0x80


const unsigned char PROGMEM LCDCustomChar[] =
{
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// Avoid 0
	0x04, 0x04, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04, // Down arrow
	0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x04, 0x04  // Up arrow
};

/* Send one pulse to the E signal (enable) */
static inline void hd44780_pulse_e(void)
{
    HD44780_PORTOUT |= _BV(HD44780_E);
#if F_CPU > 4000000UL
    _delay_ms(1);		/* guarantee 1 us high */
#elif F_CPU > 1000000UL
    __asm__ volatile("nop");
#endif
    HD44780_PORTOUT &= ~_BV(HD44780_E);
}

/* Send one nibble out to the LCD controller  */
static void hd44780_outnibble(uint8_t n, uint8_t rs)
{
    uint8_t x;

    HD44780_PORTOUT &= ~_BV(HD44780_RW);
    if (rs)
	HD44780_PORTOUT |= _BV(HD44780_RS);
    else
	HD44780_PORTOUT &= ~_BV(HD44780_RS);
    x = (HD44780_PORTOUT & ~HD44780_DATABITS) | (n & HD44780_DATABITS);
    HD44780_PORTOUT = x;
    hd44780_pulse_e();
}

/* Send one byte to the LCD controller as two nibbles */
void hd44780_outbyte(uint8_t b, uint8_t rs)
{
    hd44780_outnibble(b >> 4, rs);
    hd44780_outnibble(b & 0xf, rs);
}

/* Read one nibble from the LCD controller */
static uint8_t hd44780_innibble(uint8_t rs)
{
    uint8_t x;

    HD44780_PORTOUT |= _BV(HD44780_RW);
    HD44780_DDR &= ~HD44780_DATABITS;
    if (rs)
	HD44780_PORTOUT |= _BV(HD44780_RS);
    else
	HD44780_PORTOUT &= ~_BV(HD44780_RS);
    hd44780_pulse_e();
    x = HD44780_PORTIN & HD44780_DATABITS;
    HD44780_DDR |= HD44780_DATABITS;
    HD44780_PORTOUT &= ~_BV(HD44780_RW);

    return x & HD44780_DATABITS;
}

/* Read one byte from LCD as two nibbles */
uint8_t hd44780_inbyte(uint8_t rs)
{
    uint8_t x;

    x = hd44780_innibble(rs) << 4;
    x |= hd44780_innibble(rs);

    return x;
}

/* Wait until the busy flag is cleared */
void hd44780_wait_ready(void)
{
    while (hd44780_incmd() & HD44780_BUSYFLAG) ;
}

/* Initialize the LCD controller */
void hd44780_init(void)
{
    HD44780_DDR = _BV(HD44780_RS) | _BV(HD44780_RW) | _BV(HD44780_E) | HD44780_DATABITS;

    _delay_ms(30);
    hd44780_outnibble(HD44780_FNSET(1, 0, 0) >> 4, 0);
    _delay_ms(10);
    hd44780_outnibble(HD44780_FNSET(1, 0, 0) >> 4, 0);
    _delay_ms(1);
    hd44780_outnibble(HD44780_FNSET(1, 0, 0) >> 4, 0);

    hd44780_outnibble(HD44780_FNSET(0, 1, 0) >> 4, 0);
    hd44780_wait_ready();
    hd44780_outcmd(HD44780_FNSET(0, 1, 0));
    hd44780_wait_ready();
    hd44780_outcmd(HD44780_DISPCTL(0, 0, 0));
    hd44780_wait_ready();
}

/* Setup LCD. Call the hardware init, then adjust the display attributes we want */
void lcd_init(void)
{
    hd44780_init();				/* Call HW Init */

    hd44780_outcmd(HD44780_CLR);		/* Clear Display */
    hd44780_wait_ready();

    hd44780_outcmd(HD44780_ENTMODE(1, 0));	/* Entry mode - Auto-Inc, No Display Shift */
    hd44780_wait_ready();

    hd44780_outcmd(HD44780_DISPCTL(1, 0, 0));	/* Enable Display, Non-Blinking Cursor */
    hd44780_wait_ready();

	// Define custom LCD characters

	lcd_custom_char( (uint8_t*)LCDCustomChar, 1 );
	lcd_custom_char( (uint8_t*)LCDCustomChar, 2 );
}

/* Send character c to LCD. After '\n' is received, next char will clear the display */
int lcd_putchar(char c, FILE *unused)
{
	hd44780_wait_ready();
	hd44780_outdata(c);
    return 0;
}

/* Init custom LCD characters */
void lcd_custom_char( uint8_t* aCharacter, uint8_t aNum )
{
	register uint8_t i;

	aNum = aNum << 3;

	for(i=0; i<8; i++)
	{
		hd44780_wait_ready();
		hd44780_outcmd( 0x40 | (aNum + i) );
		hd44780_wait_ready();
		hd44780_outdata( pgm_read_byte( aCharacter + aNum + i ) );
		hd44780_wait_ready();
	}
}
