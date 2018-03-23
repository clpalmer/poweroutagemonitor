#-------------------
# Compilation flags
CFLAGS=-g -DF_CPU=8000000UL -mmcu=atmega88 -Wall -Wstrict-prototypes -Os -mcall-prologues
#-------------------
# avrdude settings for programming the programmer
DUDEHW=dragon_isp
DUDEPORT=/dev/ttyACM0
DUDEBAUD=115200
DUDECMD=avrdude -p m88 -c $(DUDEHW) -P $(DUDEPORT) -b $(DUDEBAUD)
#-------------------
# ATMega88 fuses
# High(0xDF) - Default
# Low(0xE2) - Disable CLKDIV, internal 8MHz oscillator
HIGHFUSE=0xdf
LOWFUSE=0xe2
#-------------------
.PHONY: all help ld wf rf
#-------------------
all: poweroutagemonitor.hex
#-------------------
help: 
	@echo "Print this help"
	@echo "  make help"
	@echo ""
	@echo "Compile all and show size of poweroutagemonitor.hex"
	@echo "  make all|show"
	@echo ""
	@echo "Load programmer software, write fuses and read fuses with external programmer"
	@echo "  make ld|wf|rf"
	@echo ""
	@echo "Delete all generated files"
	@echo "  make clean"
#-------------------
# main
show: main.out
	avr-objdump -d main.out
poweroutagemonitor.hex : main.out 
	avr-objcopy -R .eeprom -O ihex main.out poweroutagemonitor.hex 
	avr-size main.out
	@echo " "
	@echo "Expl.: data=initialized data, bss=uninitialized data, text=code"
	@echo " "
main.out : main.o lcd.o ds18b20.o ds3232.o twimaster.o
	avr-gcc $(CFLAGS) -o main.out -Wl,-Map,main.map main.o lcd.o ds18b20.o ds3232.o twimaster.o
main.o : main.c main.h twimaster.h lcd.h ds18b20.h ds3232.h
	avr-gcc $(CFLAGS) -Os -c main.c
#-------------------
# LCD
lcd.o : lcd.c lcd.h
	avr-gcc $(CFLAGS) -Os -c lcd.c
#-------------------
# DS18B20
ds18b20.o : ds18b20.c ds18b20.h
	avr-gcc $(CFLAGS) -Os -c ds18b20.c
#-------------------
# DS3232
ds3232.o : ds3232.c ds3232.h twimaster.h
	avr-gcc $(CFLAGS) -Os -c ds3232.c
#-------------------
# TWI
twimaster.o : twimaster.c twimaster.h
	avr-gcc $(CFLAGS) -Os -c twimaster.c
#-------------------
# Load firmware with external programmer
ld: poweroutagemonitor.hex
	$(DUDECMD) -e -U flash:w:poweroutagemonitor.hex
#-------------------
# Set fuses with external programmer
wf: 
	echo "Set HIGH fuse to $(HIGHFUSE) and LOW fuse to $(LOWFUSE)..."
	$(DUDECMD) -u -v -U hfuse:w:$(HIGHFUSE):m -U lfuse:w:$(LOWFUSE):m
#-------------------
# Read fuses with external programmer
rf: 
	echo "HIGH fuse should be $(HIGHFUSE) and LOW fuse should be $(LOWFUSE)..."
	$(DUDECMD) -v -q
#-------------------
clean:
	rm -f *.o *.map *.out poweroutagemonitor.hex
#-------------------
