# Name: Makefile
# Project: InfraHID; CIR to HID mouse/keyboard converter
# Author: Alex Badea
# Creation Date: 2007-01-14
# Tabsize: 8
# Copyright: (c) 2007 Alex Badea <vamposdecampos@gmail.com>
# License: Proprietary, free under certain conditions. See Documentation.

COMPILE = avr-gcc -Wall -Os -Iusbdrv -I. -mmcu=atmega8 #-DDEBUG_LEVEL=1
OBJECTS = usbdrv/usbdrv.o usbdrv/usbdrvasm.o usbdrv/oddebug.o infrahid.o


# symbolic targets:
all:	infrahid.hex

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	$(COMPILE) -S $< -o $@

# Fuse high byte:
# 0xc9 = 1 1 0 0   1 0 0 1 <-- BOOTRST (boot reset vector at 0x0000)
#        ^ ^ ^ ^   ^ ^ ^------ BOOTSZ0
#        | | | |   | +-------- BOOTSZ1
#        | | | |   + --------- EESAVE (don't preserve EEPROM over chip erase)
#        | | | +-------------- CKOPT (full output swing)
#        | | +---------------- SPIEN (allow serial programming)
#        | +------------------ WDTON (WDT not always on)
#        +-------------------- RSTDISBL (reset pin is enabled)
# Fuse low byte:
# 0x9f = 1 0 0 1   1 1 1 1
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0 (external >8M crystal)
#        | |  +--------------- SUT 1..0 (crystal osc, BOD enabled)
#        | +------------------ BODEN (BrownOut Detector enabled)
#        +-------------------- BODLEVEL (2.7V)

clean:
	rm -f infrahid.hex infrahid.lst infrahid.obj infrahid.cof infrahid.list infrahid.map infrahid.eep.hex infrahid.bin *.o usbdrv/*.o infrahid.s usbdrv/oddebug.s usbdrv/usbdrv.s

# file targets:
infrahid.bin:	$(OBJECTS)
	$(COMPILE) -o infrahid.bin $(OBJECTS)

infrahid.hex:	infrahid.bin
	rm -f infrahid.hex infrahid.eep.hex
	avr-objcopy -j .text -j .data -O ihex infrahid.bin infrahid.hex
	./checksize infrahid.bin 8192 960

disasm:	infrahid.bin
	avr-objdump -d infrahid.bin

cpp:
	$(COMPILE) -E infrahid.c
