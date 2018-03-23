#include "ds18b20.h"
#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#define DS18B20_DECIMAL_STEPS_12BIT 625 //.0625
#define DS18B20_DECIMAL_STEPS_9BIT 500 //.500

uint8_t DS18B20_reset(uint8_t num)
{
	// Pull line low and wait for 480uS
	DS18B20_LOW(num);
	DS18B20_OUTPUT_MODE(num);
	_delay_us(480);
	
	// Release line and wait for 60uS
	DS18B20_INPUT_MODE(num);
	_delay_us(60);

	// Store line value and wait until the completion of 480uS period
	uint8_t i;
	if (num) {
		i = (DS18B20_PIN & (1 << DS18B20_DQ1));
	} else {
		i = (DS18B20_PIN & (1 << DS18B20_DQ0));
	}
	_delay_us(420);

	// Return the value read from the presence pulse (0=OK, 1=WRONG)
	return i;
}

void DS18B20_write_bit(uint8_t num, uint8_t bit)
{
	// Pull line low for 1uS
	DS18B20_LOW(num);
	DS18B20_OUTPUT_MODE(num);
	_delay_us(1);
	
	// If we want to write 1, release the line (if not will keep low)
	if ( bit ) {
		DS18B20_INPUT_MODE(num);
		_delay_us(60);
	} else {
		_delay_us(59);
		DS18B20_INPUT_MODE(num);
		_delay_us(1);
	}
}

uint8_t DS18B20_read_bit(uint8_t num)
{
	uint8_t bit = 0;

	// Pull line low for 1uS
	DS18B20_LOW(num);
	DS18B20_OUTPUT_MODE(num);
	_delay_us(1);
	
	// Release line and wait for 12uS
	DS18B20_INPUT_MODE(num);
	_delay_us(12);
	
	// Read line value
	if (num) {
		if (DS18B20_PIN & (1 << DS18B20_DQ1)) {
			bit = 1;
		}
	} else {
		if (DS18B20_PIN & (1 << DS18B20_DQ0)) {
			bit = 1;
		}
	}
	_delay_us(48);
	return bit;
}

uint8_t DS18B20_read_byte(uint8_t num)
{
	uint8_t i = 8;
	uint8_t n = 0;
	while ( i-- ) {
		// Shift one position right and store read value
		n >>= 1;
		n |= (DS18B20_read_bit(num) << 7);
	}
	return n;
}

void DS18B20_write_byte(uint8_t num, uint8_t byte)
{
	uint8_t i = 8;
	while ( i-- ) {
		// Write actual bit and shift one position right to make the next bit ready
		DS18B20_write_bit(num, byte & 1);
		byte >>= 1;
	}
}

void DS18B20_init()
{
	int i;
	for (i = 0; i <= 1; i++)
	{
		DS18B20_reset(i);
		DS18B20_write_byte(i, DS18B20_CMD_SKIPROM);
		DS18B20_write_byte(i, DS18B20_CMD_WSCRATCHPAD);
		DS18B20_write_byte(i, 0x00);
		DS18B20_write_byte(i, 0x00);
		DS18B20_write_byte(i, 0x00);
		DS18B20_reset(i);
	}
}

void DS18B20_temp(uint8_t num, int8_t *temp, int8_t *half)
{
	cli();

	uint8_t temperature[2];
	
	// Reset, skip ROM and start temperature conversion
	DS18B20_reset(num);
	DS18B20_write_byte(num, DS18B20_CMD_SKIPROM);
	DS18B20_write_byte(num, DS18B20_CMD_CONVERTTEMP);
	
	//Wait until conversion is complete
	while ( !DS18B20_read_bit(num) ) {}
		
	// Reset, skip ROM and send command to read Scratchpad
	DS18B20_reset(num);
	DS18B20_write_byte(num, DS18B20_CMD_SKIPROM);
	DS18B20_write_byte(num, DS18B20_CMD_RSCRATCHPAD);
	
	//Read Scratchpad (only 2 first bytes)
	temperature[0] = DS18B20_read_byte(num);
	temperature[1] = DS18B20_read_byte(num);
	DS18B20_reset(num);
	
	//Store temperature integer digits and decimal digits
	*temp = ((temperature[0] >> 4) & 0x0F) | ((temperature[1] << 4) & 0xF0);
    if ( temperature[0] & 0x08 ) {
		*half = 1;
	} else {
		*half = 0;
	}
	
	sei();
}



