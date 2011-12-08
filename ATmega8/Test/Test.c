#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>

static char tx_buffer[16] = "";
static char rx_buffer[16] = "";

unsigned char tx_pos = 0;
unsigned char rx_pos = 0;

void port_init()
{
	DDRC = 0x3F;
	PORTC = 0;
}

void uart_init()
{
	UCSRA = (1<<U2X);
	UCSRB = (1<<RXCIE)|(1<<RXEN)|(1<<TXEN);	// |(1<<TXCIE)
	UBRRH = 0x02;
	UBRRL = 0x70;
}

void send_message(char *msg)
{
	strcpy(tx_buffer, msg);
	tx_pos = 0;
	do
	{
		UDR = tx_buffer[tx_pos];
		while ( !( UCSRA & (1<<UDRE)) )
			__asm__ volatile("nop");		// so the endless loop isn't optimized away
		tx_pos++;
	}
	while( (tx_buffer[tx_pos-1] != '.') && (tx_pos < 16) );
	PORTC = 2;
}

void receive_message()
{
	rx_pos = 0;
	if(strcmp(rx_buffer, "hello.") == 0)
	{
		PORTC = 1;
		send_message("world.");
	}
}

//*************************************************************
//*******************        MAIN           *******************
//*************************************************************

int main()
{
	port_init();
	uart_init();
	sei();

	while (1)
	{
		__asm__ volatile("nop");		// so the endless loop isn't optimized away
	}

	return (1);	// should never happen
}

//*************************************************************
//*******************      END  MAIN        *******************
//*************************************************************

ISR(USART_RXC_vect)
{
	char data = UDR;
	rx_buffer[rx_pos] = data;
	(rx_pos == 15) ? rx_pos = 0 : rx_pos++;	// should NEVER EVER EVER happen.
	if( data == '.' )
	{
		receive_message();
	}
}

//ISR(USART_TXC_vect)
//{
//
//}
//
