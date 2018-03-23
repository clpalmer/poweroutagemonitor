#define NVM_DATA_BASE 0x14

typedef struct DS3232_time
{
	uint8_t secs;
	uint8_t mins;
	uint8_t hours;
	uint8_t date;
	uint8_t month;
	uint8_t year;
	uint8_t ampm;
	uint8_t day;
} DS3232_time;

uint8_t DS3232_readtime(DS3232_time* time);
uint8_t DS3232_savetime(DS3232_time time);
uint8_t DS3232_read(int8_t address, uint8_t* data);
uint8_t DS3232_write(int8_t address, uint8_t data);
float DS3232_temp(void);
void DS3232_init(void);

