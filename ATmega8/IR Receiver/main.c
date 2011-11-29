#include <avr/io.h>
#include <avr/interrupt.h>

int ir_code[32] = {};
int ir_bit = 0;

void init()
{
	// timer1:
	TCCR1B = (1<<CS11) | (1<<CS10);

	// int0:
	MCUCR = (1<<ISC00);
	GICR = (1<<INT0);
	sei();
}

int main(void)
{
	init();
	while (1)
	{
		__asm__ volatile("nop");        // so the endless loop isn't optimized away
	}

	return (1);    // should never happen
}

// ==================== Interrupt vectors ==========================

ISR(INT0_vect)
{
	ir_code[ir_bit] = TCNT1;
	TCNT1 = 0;
	ir_bit++;
}
