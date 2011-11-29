#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

int i = 0;
int j = 0;
int k = 0;

int count = 10;

void main()
{
	DDRD = 0xFF;
	DDRB = 0xFF;
	PORTD = 0xFF;
	PORTB = 0x00;

	for (i=0; i=10; i++)
		PORTD ++;

	while(1);
}



