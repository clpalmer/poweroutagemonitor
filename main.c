#include "main.h"

FILE lcd_str = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE);

static volatile uint8_t buttonStatus = 0;
static volatile uint8_t secondInt = 0;
static volatile uint8_t asleep = 0;
static volatile uint8_t wakeOnInt = 0;

ISR( PCINT0_vect )
{
	uint8_t i;
	
	// Button pressed
    _delay_ms(40);
	for ( i = 0; i <= 2; i++ )
	{
		if ( (BUT_PI & _BV(i)) == 0 )
		{
			buttonStatus |= _BV(i);
			asleep = 0;
		}
	}
}

ISR( PCINT1_vect )
{
	if (PINC & _BV(3))
	{
		secondInt = 1;
		if ( wakeOnInt ) {
			asleep = 0;
		}
	}
}

static void Init(void)
{
	cli();

	// Configuration for LCD
    lcd_init();
    stdout = &lcd_str;
    stderr = &lcd_str;

	// Configuration for RTC
	i2c_init();
	DS3232_init();
	
	// Configuration for LEDs
	LED_DR |= LED_G | LED_R;
	LED_PO &= ~LED_G & ~LED_R;

	// Configuration for push buttons & 1Hz Interrupt
	BUT_PU |= BUT_UP | BUT_DOWN | BUT_MODE;
	PCICR |= _BV(PCIE0) | _BV(PCIE1);
	PCMSK0 |= _BV(PCINT0) | _BV(PCINT1) | _BV(PCINT2);
	PCMSK1 |= _BV(PCINT11);

	// Configuration for temperature sensors
    DS18B20_init();
	
    set_sleep_mode(SLEEP_MODE_IDLE);
	sei();

	CLEAR_ALL;
}


void ReadRecord( uint8_t aCount, uint8_t* aRecord )
{
	uint16_t i;
	uint8_t offset = (aCount * RECORD_LENGTH) + 1;

	for ( i = offset; i < (offset + RECORD_LENGTH); i++ )
	{
		eeprom_busy_wait();
		aRecord[i - offset] = eeprom_read_byte( (uint8_t*)i );
	}
}


void WriteNVMRecord( uint8_t* aRecord, uint8_t aRecNum )
{
	uint8_t i;

	DS3232_write(NVM_DATA_BASE + (NVM_REC_LENGTH * aRecNum) + (NVM_REC_LENGTH - 1), 0x00);

	for ( i = 0; i < NVM_REC_LENGTH; i++ )
	{
		DS3232_write(NVM_DATA_BASE + (NVM_REC_LENGTH * aRecNum) + i, aRecord[i]);
	}
}


void CompleteRecord( uint8_t aCount )
{
	uint8_t byte, i;
	uint8_t bytes[NVM_REC_LENGTH];	
	uint8_t ofst = (aCount * RECORD_LENGTH) + 1;
	
	DS3232_time time;
	DS3232_readtime(&time);
	
	int8_t tmp1, tmp2, half1, half2;
	DS18B20_temp(0, &tmp1, &half1);
	DS18B20_temp(1, &tmp2, &half2);

	bytes[0] = (time.month << 4) | time.hours;
	bytes[1] = (time.date << 1) | time.ampm;
	bytes[2] = time.mins;
	bytes[3] = tmp1;
	bytes[4] = tmp2;
	bytes[5] = RECORD_COMPLETE;

	if ( aCount != 255 )
	{
		// Copy previous data from RTC NVM to EEPROM
		for ( i = 0; i < (2 * NVM_REC_LENGTH); i++ )
		{
			DS3232_read(NVM_DATA_BASE + i, &byte);
			eeprom_busy_wait();
			eeprom_write_byte( (uint8_t*)(ofst + i), byte );
		}

		// Complete EEPROM record with current data
		for ( i = 0; i < NVM_REC_LENGTH; i++ )
		{
			eeprom_busy_wait();
			eeprom_write_byte( (uint8_t*)(ofst + (2 * NVM_REC_LENGTH) + i), bytes[i] );
		}
	}

	// Write record 1
	WriteNVMRecord( bytes, 0 );

	// Write record 2
	WriteNVMRecord( bytes, 1 );
}


void UpdateRecord(void)
{
	CompleteRecord( 255 );
}


int main(void)
{
	DS3232_time time;
	uint8_t count, state = STATE_MAIN;
	int8_t  tmp1, tmp2, half1, half2;
	uint8_t record[18];

    Init();

	lcd_clear();
	printf( "Initializing..." );
	
	_delay_ms(200);
	eeprom_busy_wait();
	count = eeprom_read_byte( (uint8_t*)0 );

	if ( count > MAX_OUTAGES )
	{
		count = 0;
		eeprom_busy_wait();
		eeprom_write_byte( (uint8_t*)0, count );
		UpdateRecord();
	}
	else if ( count < MAX_OUTAGES )
	{
		CompleteRecord( count );
		count++;
		eeprom_busy_wait();
		eeprom_write_byte( (uint8_t*)0, count );
		UpdateRecord();
	}

	if ( count == MAX_OUTAGES )
	{
		state = STATE_LIST;
	}
	
    while ( 1 )
    {
		if ( state == STATE_MAIN )
			LED_PO |= LED_G;
		else
			LED_PO &= ~LED_G & ~LED_R;

		switch( state )
		{
			case STATE_MAIN:
			{
				while (!PB_MODE)
				{
				    if ( secondInt ) {
						DS18B20_temp(0, &tmp1, &half1);
						DS18B20_temp(1, &tmp2, &half2);

						DS3232_readtime( &time );

						//lcd_clear();
						lcd_line1();
						printf( "%02d/%02d/%02d  %3d.%c\337", time.month, time.date, time.year, tmp1, (half1 ? '5' : '0') );
						lcd_line2();
						printf( "%02d:%02d:%02d%c %3d.%c\337", time.hours, time.mins, time.secs, (time.ampm ? 'P' : 'A'), tmp2, (half2 ? '5' : '0') );
				
						if ( count > 0 )
						{
							if ( !(LED_PO & LED_R) )
								LED_PO |= LED_R;
							else
								LED_PO &= ~LED_R;
						}
						UpdateRecord();
						secondInt = 0;
					}

                    if (!PB_MODE) {
						wakeOnInt = 1;
						SLEEP					
					}
				}
				
				wakeOnInt = 0;
				state = STATE_LIST;

				CLEAR_ALL;
				break;
			}
			
			case STATE_LIST:
			{
				lcd_line1();
				printf( "Outages: %2d     ", count );
				lcd_line2();

				if ( count > 0)
				{
				 	printf( "\x001: Show  *: Next" );
				}
				else
				{
					printf( "         *: Next" );
				}

				while (!PB_MODE && !PB_DOWN) {
					SLEEP
				}

				if ( PB_MODE )
				{
					if ( count < MAX_OUTAGES )
						state = STATE_TIMESET1;
					CLEAR_ALL;
					break;
				}

				if ( PB_DOWN && (count == 0) )
				{
					CLEAR_ALL;
					break;
				}
				
				CLEAR_ALL;

				uint8_t outage = 0;
				uint8_t onoff = 0;
				while ( 1 )
				{
					uint8_t recnum = 0;

					lcd_line1();
					ReadRecord( outage, record );
					if ( onoff == 0 )
					{
						if ( record[NVM_REC_LENGTH - 1] != RECORD_COMPLETE )
						{
							recnum = NVM_REC_LENGTH;
						}
							
						printf( "%02d-", outage + 1 );
					}
					else
					{
						printf( "%02d+", outage + 1 );
						recnum = NVM_REC_LENGTH * 2;
					}

					time.month = record[recnum] >> 4;
					time.hours = record[recnum] & 0x0F;
					time.date = record[recnum + 1] >> 1;
					time.ampm = record[recnum + 1] & 0x01;
					time.mins = record[recnum + 2];
					tmp1 = record[recnum + 3];
					tmp2 = record[recnum + 4];

					printf( " %02d/%02d %02d:%02d%c", time.month, time.date, time.hours, time.mins, (time.ampm ? 'P' : 'A') ); 
					lcd_line2();
					printf( " 1:%3d\337  2:%3d\337 ", tmp1, tmp2 );

					SLEEP

					if ( PB_DOWN )
					{
						if ( onoff == 1 )
						{
							if ( outage >= (count - 1) )
							{
								state = STATE_CLEAROUT;
								break;
							}
							onoff = 0;
							outage++;
						}
						else
						{
							onoff = 1;
						}
					}
					else
					{
						if ( onoff == 0 )
						{
							if ( outage <= 0 )
							{
								break;
							}
							onoff = 1;
							outage--;
						}
						else
						{
							onoff = 0;
						}
					}

					CLEAR_ALL;
				}

				CLEAR_ALL;
				break;
			}

			case STATE_CLEAROUT:
			{
				lcd_line1();
				printf( "Clear outages?  " );
				lcd_line2();
				printf( "*: YES    \x001\x002: NO" );

				SLEEP

				if ( (PB_DOWN) || (PB_UP) )
				{
					if ( count < MAX_OUTAGES )
						state = STATE_MAIN;
					else
						state = STATE_LIST;

					CLEAR_ALL;
					break;
				}

				count = 0;
				eeprom_busy_wait();
				eeprom_write_byte( (uint8_t*)0, count );
				UpdateRecord();

				lcd_clear();
				printf( "Cleared!" );
				_delay_ms(1000);

				state = STATE_MAIN;
				CLEAR_ALL;
				break;	
			}

			case STATE_TIMESET1:
			{
				lcd_line1();
				printf( "Set Time & Date " );
				lcd_line2();
				printf( "\x001: Set   *: Next" );

				SLEEP

				if ( PB_DOWN )
					state = STATE_TIMESET2;

				if ( PB_MODE )
					state = STATE_MAIN;

				CLEAR_ALL;
				break;
			}

			case STATE_TIMESET2:
			{
				char item = SET_MNTH;
				DS3232_readtime(&time);
				while ( item != SET_DONE )
				{
					lcd_line1();

					if ( item == SET_MNTH )
					{
						printf( "%02d/__/__ __:__ _", time.month );
						lcd_line2();
						printf( "\x001\x002: Mnth  *: Next" );
					}
					else if ( item == SET_DATE )
					{
						printf( "__/%02d/__ __:__ _", time.date );
						lcd_line2();
						printf( "\x001\x002: Date  *: Next" );
					}
					else if ( item == SET_YEAR )
					{
						printf( "__/__/%02d __:__ _", time.year );
						lcd_line2();
						printf( "\x001\x002: Year  *: Next" );
					}
					else if ( item == SET_HOUR )
					{
						printf( "__/__/__ %02d:__ _", time.hours );
						lcd_line2();
						printf( "\x001\x002: Hour  *: Next" );
					}
					else if ( item == SET_MINS )
					{
						printf( "__/__/__ __:%02d _", time.mins );
						lcd_line2();
						printf( "\x001\x002: Mins  *: Next" );
					}
					else if ( item == SET_AMPM )
					{
						printf( "__/__/__ __:__ %c", (time.ampm ? 'P' : 'A') );
						lcd_line2();
						printf( "\x001\x002 AM/PM *: Next" );
					}

					SLEEP

					if ( PB_DOWN )
					{
						if ( item == SET_MNTH )
						{
							if ( --time.month < 1 )
								time.month = 12;
						}
						else if ( item == SET_DATE )
						{
							if ( --time.date < 1 )
								time.date = 31;
						}
						else if ( item == SET_YEAR )
						{
							if ( --time.year > 99 )
								time.year = 99;
						}
						else if ( item == SET_HOUR )
						{
							if ( --time.hours < 1 )
								time.hours = 12;
						}
						else if ( item == SET_MINS )
						{
							if ( --time.mins > 59 )
								time.mins = 59;
						}
						else if ( item == SET_AMPM )
						{
							time.ampm = 1 - time.ampm;
						}
					}
					else if ( PB_UP )
					{
						if ( item == SET_MNTH )
						{
							if ( ++time.month > 12 )
									time.month = 1;
						}
						else if ( item == SET_DATE )
						{
							if ( ++time.date > 31 )
								time.date = 1;
						}
						else if ( item == SET_YEAR )
						{
							if ( ++time.year > 99 )
								time.year = 0;
						}
						else if ( item == SET_HOUR )
						{
							if ( ++time.hours > 12 )
								time.hours = 1;
						}
						else if ( item == SET_MINS )
						{
							if ( ++time.mins > 59 )
								time.mins = 0;
						}
						else if ( item == SET_AMPM )
						{
							time.ampm = 1 - time.ampm;
						}
					}					
					else if ( PB_MODE )
					{
						DS3232_savetime(time);
						
						switch (item)
						{
							case SET_MNTH:
								item = SET_DATE;
								break;
							case SET_DATE:
							    item = SET_YEAR;
								break;
							case SET_YEAR:
								item = SET_HOUR;
								break;
							case SET_HOUR:
								item = SET_MINS;
								break;
							case SET_MINS:
								item = SET_AMPM;
								state = STATE_MAIN;
								break;
							case SET_AMPM:
								item = SET_DONE;
								state = STATE_MAIN;
								break;
						}
					}

					CLEAR_ALL;
				}

				break;
			}
	    }
	}

    return 0;
}
