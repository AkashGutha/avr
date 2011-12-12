#include <avr/eeprom.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "global.h"
#include "communication.h"


void port_init()
{
	DDRC = 0x3F;
	PORTC = 1;
}

//*************************************************************
//*******************        MAIN           *******************
//*************************************************************

int main()
{
	port_init();
	uart_init();
	sei();
	codes_init();
	PORTC = 2;

	while (1)
	{
		__asm__ volatile("nop");		// so the endless loop isn't optimized away
	}

	return (1);	// should never happen
}

//*************************************************************
//*******************      END  MAIN        *******************
//*************************************************************
