/*
 * communication.c
 *
 *  Created on: Dec 12, 2011
 *      Author: ro1v02y4
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

#include "communication.h"
#include "global.h"

unsigned char USED_CODE_COUNT = MAX_CODE_COUNT;

RemoteCode EEMEM EEPROMCodes[MAX_CODE_COUNT];
RemoteCode Codes[MAX_CODE_COUNT] = {};

static char tx_buffer[16] = "";
static char rx_buffer[16] = "";
unsigned char tx_pos = 0;
unsigned char rx_pos = 0;


void uart_init()
{
	UCSRA = (1<<U2X);
	UCSRB = (1<<RXCIE)|(1<<RXEN)|(1<<TXEN);	// |(1<<TXCIE)
	UBRRH = 0x02;
	UBRRL = 0x70;
}

void codes_init()
{
	unsigned char i = 0;
	for( i = 0; i < MAX_CODE_COUNT; i++ )
	{
		Codes[i].ir_code = eeprom_read_dword(&EEPROMCodes[i].ir_code);
		Codes[i].key_code = eeprom_read_word(&EEPROMCodes[i].key_code);
		if(Codes[i].ir_code == 0)
		{
			USED_CODE_COUNT = i;
			break;
		}
	}
}

char HexToByte(char *str, char start, char count, char *dest)
{
	unsigned char i = 0;
	unsigned char j = 0;
//	char buf[10] = "";
	for(i = start; i <= start + count; i += 2)
	{
		char a = (str[i] < 60 ) ? (str[i] - 48) : (str[i] - 55);
		char b = (str[i+1] < 60 ) ? (str[i+1] - 48) : (str[i+1] - 55);
		dest[j] = a * 16 + b;
		j++;
	}
	return j-1;
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

void receive_message()
{
	switch( rx_buffer[0] )
	{
		case 'h': Connect(); break;
		case 'b': Disconnect(); break;
		case 's': SetData(); break;
		case 'r': GetData(); break;
		default : break;
	}
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

/*
 * s<nn><cccccccc><kkkk>
 * nn       = index,    2 bytes - 1 hex char
 * cccccccc = IR code,  8 bytes - 4 hex char
 * kkkk     = key code, 4 bytes - 2 hex char
 */

void SetData()
{
	unsigned char addr = 0;
	unsigned char count = 0;
	char buf[10] = "";

	HexToByte(rx_buffer, 1, 2, (char*)&buf);
	addr = buf[0];
	count = HexToByte(rx_buffer, 3, 12, (char*)&buf);
	if(count == 6)
	{
		eeprom_write_block((void*)&buf, (void*)&EEPROMCodes[addr], 6);
		Codes[addr].ir_code = eeprom_read_dword(&EEPROMCodes[addr].ir_code);
		Codes[addr].key_code = eeprom_read_word(&EEPROMCodes[addr].key_code);
		if(Codes[addr].ir_code == 0)
			USED_CODE_COUNT = addr;
	}
	send_message("ok.");
}

void GetData()
{
	unsigned char addr = 0;
	char buf[10] = "";
	HexToByte(rx_buffer, 1, 2, (char*)&buf);
	addr = buf[0];
	if(addr < USED_CODE_COUNT)
	{
		sprintf((char*)&tx_buffer, "s%02X%08lX%04X.", addr, Codes[addr].ir_code, Codes[addr].key_code);
		send_message(tx_buffer);
	}
	else
		send_message("ERROR.");
}

/*
 * ================= INTERRUPT FUNCTIONS =====================
 */

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

