// AVR Studio 4.19 b730 :
// http://www.atmel.com/dyn/resources/prod_documents/AvrStudio4Setup.exe

#include <avr/io.h>
#include <avr/interrupt.h>

void init()
{

}

int main(void)
{
	init();
    while (1)
    {
        __asm__ volatile("nop");		// so the endless loop isn't optimized away
    }

    return (1);	// should never happen
}

// ==================== Interrupt vectors ==========================

ISR(INT0_vect)
{

}
