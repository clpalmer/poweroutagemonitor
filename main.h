#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "twimaster.h"
#include "lcd.h"
#include "ds3232.h"
#include "ds18b20.h"

#define MAX_OUTAGES		14
#define RECORD_LENGTH	18
#define NVM_REC_LENGTH	6

#define RECORD_COMPLETE	0xAA

#define BUT_PI			PINB
#define BUT_PU			PORTB
#define BUT_UP			_BV(PORT2)
#define BUT_DOWN		_BV(PORT1)
#define BUT_MODE		_BV(PORT0)

#define PB_UP			(buttonStatus & BUT_UP)
#define PB_DOWN			(buttonStatus & BUT_DOWN)
#define PB_MODE			(buttonStatus & BUT_MODE)

#define LED_DR			DDRB
#define LED_PO			PORTB
#define LED_G			_BV(PORT6)
#define LED_R			_BV(PORT7)

#define CLEAR_UP		buttonStatus &= ~_BV(PORT0)
#define CLEAR_DOWN		buttonStatus &= ~_BV(PORT1)
#define CLEAR_MODE		buttonStatus &= ~_BV(PORT2)
#define CLEAR_ALL		buttonStatus = 0;

#define STATE_MAIN		0
#define STATE_TIMESET1 	1
#define STATE_TIMESET2 	2
#define STATE_LIST		3
#define STATE_CLEAROUT	4

#define SET_MNTH		0
#define SET_DATE		1
#define SET_YEAR		2
#define SET_HOUR		3
#define SET_MINS		4
#define SET_AMPM		5
#define SET_DONE		6

#define FALSE			0
#define TRUE			1

#define SLEEP           asleep = 1; while (asleep) { sleep_enable(); sleep_cpu(); sleep_disable(); }

