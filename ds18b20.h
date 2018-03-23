#include <avr/io.h> 
#include <util/delay.h> 

/* Thermometer Connections (At your choice) */
#define DS18B20_PORT           PORTC
#define DS18B20_DDR            DDRC
#define DS18B20_PIN            PINC
#define DS18B20_DQ0            PC1
#define DS18B20_DQ1            PC2

/* Utils */
#define DS18B20_INPUT_MODE(n)  if (n) { DS18B20_DDR &= ~(1 << DS18B20_DQ1); } else { DS18B20_DDR &= ~(1 << DS18B20_DQ0); }
#define DS18B20_OUTPUT_MODE(n) if (n) { DS18B20_DDR |= (1 << DS18B20_DQ1); } else { DS18B20_DDR |= (1 << DS18B20_DQ0); }
#define DS18B20_LOW(n)         if (n) { DS18B20_PORT &= ~(1 << DS18B20_DQ1); } else { DS18B20_PORT &= ~(1 << DS18B20_DQ0); }
#define DS18B20_HIGH(n)        if (n) { DS18B20_PORT |= (1 << DS18B20_DQ1); } else { DS18B20_PORT |= (1 << DS18B20_DQ0); }

#define DS18B20_CMD_CONVERTTEMP   0x44
#define DS18B20_CMD_RSCRATCHPAD   0xbe
#define DS18B20_CMD_WSCRATCHPAD   0x4e
#define DS18B20_CMD_CPYSCRATCHPAD 0x48
#define DS18B20_CMD_RECEEPROM     0xb8
#define DS18B20_CMD_RPWRSUPPLY    0xb4
#define DS18B20_CMD_SEARCHROM     0xf0
#define DS18B20_CMD_READROM       0x33
#define DS18B20_CMD_MATCHROM      0x55
#define DS18B20_CMD_SKIPROM       0xcc
#define DS18B20_CMD_ALARMSEARCH   0xec

void DS18B20_init(void);
void DS18B20_temp(uint8_t num, int8_t *temp, int8_t *half);