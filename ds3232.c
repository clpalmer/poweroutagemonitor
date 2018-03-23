#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "twimaster.h"
#include "ds3232.h"

#define DS3232_ADR 0xD0

uint8_t DS3232_read(int8_t address, uint8_t* data)
{
	if ( i2c_start(DS3232_ADR+I2C_WRITE) )   { return false; } // Issue start on I2C bus in write mode
    if ( i2c_write(address) )                { return false; } // Send the address of data to read
    if ( i2c_rep_start(DS3232_ADR+I2C_READ)) { return false; } // Re-issue Start but read this time
	
	*data = i2c_readNak();  // Read byte
	i2c_stop();             // Issue a stop on I2C bus
	return true;
}

uint8_t DS3232_write(int8_t address, uint8_t data)
{
	if ( i2c_start(DS3232_ADR+I2C_WRITE) ) { return false; } // Issue start on I2C bus in write mode
	if ( i2c_write(address) )              { return false; } // Send the address of data to write
	if ( i2c_write(data) )                 { return false; } // Send the data

	i2c_stop();             // Issue a stop on I2C bus
	return true;
}

uint8_t DS3232_readtime(DS3232_time* time)
{
	uint8_t tmp;
	
	if (!DS3232_read(0x0, &tmp)) { return false; }
    time->secs = ((tmp >> 4) * 10) + (tmp & 0x0F);
	
	if (!DS3232_read(0x1, &tmp)) { return false; }
	time->mins = ((tmp >> 4) * 10) + (tmp & 0x0F);

	if (!DS3232_read(0x2, &tmp)) { return false; }
	time->hours = (((tmp >> 4) & 0x1) * 10) + (tmp & 0x0F);
	time->ampm = ((tmp >> 5) & 0x1);
	
	if (!DS3232_read(0x3, &tmp)) { return false; }
	time->day = tmp;

	if (!DS3232_read(0x4, &tmp)) { return false; }
	time->date = ((tmp >> 4) * 10) + (tmp & 0x0F);

	if (!DS3232_read(0x5, &tmp)) { return false; }
	time->month = (((tmp >> 4) & 0x1) * 10) + (tmp & 0x0F);

	if (!DS3232_read(0x6, &tmp)) { return false; }
	time->year = ((tmp >> 4) * 10) + (tmp & 0x0F);

	return true;
}

uint8_t DS3232_savetime(DS3232_time time)
{
	uint8_t tmp;
	
	tmp = ((time.secs / 10) << 4) + (time.secs % 10);
	if (!DS3232_write(0x0, tmp)) { return false; }
	
	tmp = ((time.mins / 10) << 4) + (time.mins % 10);
	if (!DS3232_write(0x1, tmp)) { return false; }

	if (!DS3232_read(0x2, &tmp)) { return false; }
	tmp &= 0x60;
	tmp |= ((time.hours / 10) << 4) | (time.hours % 10) | (time.ampm << 5);
	if (!DS3232_write(0x2, tmp)) { return false; }
	
	if (!DS3232_write(0x3, time.day)) { return false; }
    
	tmp = ((time.date / 10) << 4) + (time.date % 10);
	if (!DS3232_write(0x4, tmp)) { return false; }

	if (!DS3232_read(0x5, &tmp)) { return false; }
	tmp &= 0x80;
	tmp |= ((time.month / 10) << 4) + (time.month % 10);
	if (!DS3232_write(0x5, tmp)) { return false; }

	tmp = ((time.year / 10) << 4) + (time.year % 10);
	if (!DS3232_write(0x6, tmp)) { return false; }

	return true;
}

float DS3232_temp()
{
	uint8_t whole, frac;
	
	DS3232_read(0x12, &frac);
	frac = (frac >> 6);
	
	DS3232_read(0x11, &whole);	
	if (whole & 0x80)
	{
		whole ^= 0xFF;
		return (float)-1 * ((float)((whole ^ 0xFF) + 1) + ((float)frac * .25));
	} else {
		return (float)whole + ((float)frac * 0.25);
	}
}

void DS3232_init()
{
	uint8_t temp;
	DS3232_read(0x02, &temp);   // Read hour register
	temp |= 0x40;				// Set 12-Hour bit
	DS3232_write(0x02, temp);   // Write back to RTC
	DS3232_write(0x0E, 0x00);   // Write control register
	DS3232_write(0x0F, 0x20);   // Write control register
}
