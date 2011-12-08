#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <string.h>
#include <avr/eeprom.h>

static char tx_buffer[16] = "";
static char rx_buffer[16] = "";

unsigned char tx_pos = 0;
unsigned char rx_pos = 0;

unsigned char buf[10] = {};

void port_init()
{
	DDRC = 0x3F;
	PORTC = 2;
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
}

void Connect()
{
	if(strcmp(rx_buffer, "hello") == 0)
	{
		PORTC = 1;
		send_message("world.");
	}
}

void Disconnect()
{
	if(strcmp(rx_buffer, "bye") == 0)
	{
		PORTC = 2;
		send_message("bye.");
	}
}

char HexToByte(char *str, char start, char count)
{
	unsigned char i = 0;
	unsigned char j = 0;
	for(i = start; i <= start + count; i += 2)
	{
		buf[j] = (str[i] - 48) * 16 + (str[i+1] - 48);
		j++;
	}
	return j;
}

// s<nn><cccccc><kkkk>
// nn     = index,    2 bytes
// cccccc = IR code,  6 bytes
// kkkk   = key code, 4 bytes
void SetData()
{
	char i = HexToByte(rx_buffer, 3, 6);
}

void receive_message()
{
	switch( rx_buffer[0] )
	{
		case 'h': Connect(); break;
		case 'b': Disconnect(); break;
		case 's': SetData(); break;
		default : break;
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
	if( data != '.' )
	{
		rx_buffer[rx_pos] = data;
		(rx_pos == 15) ? rx_pos = 0 : rx_pos++;	// should NEVER EVER EVER happen.
	}
	else
	{
		rx_buffer[rx_pos] = 0;
		rx_pos = 0;
		receive_message();
	}
}

//ISR(USART_TXC_vect)
//{
//
//}
//
