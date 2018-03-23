#include <stdio.h>

/* HD44780 LCD port connections */
#define HD44780_PORT D
#define HD44780_RS PORT4
#define HD44780_RW PORT5
#define HD44780_E  PORT6
#define HD44780_D4 PORT0
#define HD44780_D5 PORT1
#define HD44780_D6 PORT2
#define HD44780_D7 PORT3

/* Send byte b to the LCD.  RS=0 instruction register, RS=1 data register */
void	hd44780_outbyte(uint8_t b, uint8_t rs);

/* Read byte from LCD. RS=0 - busy flag (bit 7) & address counter, RS=1 - data register */
uint8_t	hd44780_inbyte(uint8_t rs);

/* Wait for the busy flag to clear */
void	hd44780_wait_ready(void);

/* Initialize the LCD controller hardware */
void	hd44780_init(void);

/* Send a command to the LCD controller. */
#define hd44780_outcmd(n)	hd44780_outbyte((n), 0)

/* Send a data byte to the LCD controller. */
#define hd44780_outdata(n)	hd44780_outbyte((n), 1)

/* Read the address counter and busy flag from the LCD. */
#define hd44780_incmd()		hd44780_inbyte(0)

/* Read the current data byte from the LCD. */
#define hd44780_indata()	hd44780_inbyte(1)

/* Clear LCD display command. */
#define HD44780_CLR 		0x01

/* Home cursor command. */
#define HD44780_HOME		0x02

/* Select entry mode -  inc = address counter auto-inc, shift = auto display shift */
#define HD44780_ENTMODE(inc, shift) (0x04 | ((inc)? 0x02: 0) | ((shift)? 1: 0))

/* Selects disp[lay] on/off, cursor on/off, cursor blink on/off */
#define HD44780_DISPCTL(disp, cursor, blink) \
	(0x08 | ((disp)? 0x04: 0) | ((cursor)? 0x02: 0) | ((blink)? 1: 0))

/* Shift = 1, shift display R/L - Shift = 0, move cursor R/L */
#define HD44780_SHIFT(shift, right) (0x10 | ((shift)? 0x08: 0) | ((right)? 0x04: 0))

/* Fn set - if8bit=8-bit data, twoline=2-line LCD, font5x10=5x10 font, 5x8 if clear */
#define HD44780_FNSET(if8bit, twoline, font5x10) \
	(0x20 | ((if8bit)? 0x10: 0) | ((twoline)? 0x08: 0) | ((font5x10)? 0x04: 0))

/* Set the next character generator address to addr */
#define HD44780_CGADDR(addr) (0x40 | ((addr) & 0x3f))

/* Set the next display address to addr */
#define HD44780_DDADDR(addr) (0x80 | ((addr) & 0x7f))

/* Initialize LCD controller.  Performs a software reset */
void 	lcd_init(void);

/* Send one character to the LCD */
int		lcd_putchar(char c, FILE *stream);

/* Set position to start of 2nd line */
#define lcd_line1()			hd44780_wait_ready(); hd44780_outcmd( HD44780_HOME );
#define lcd_line2()			hd44780_wait_ready(); hd44780_outcmd( 0xC0 );
#define lcd_clear();		hd44780_wait_ready(); hd44780_outcmd( HD44780_CLR );

/* Init custom LCD characters */
void lcd_custom_char( uint8_t* aCharacter, uint8_t aNum );
