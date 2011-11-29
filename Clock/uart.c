#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "uart.h"

int8_t tx_buf[TX_BUF_SIZE] = "";
int8_t rx_buf[RX_BUF_SIZE] = "";

int8_t tx_pos = 0;
int8_t rx_pos = 0;

//UART0 initialisation
// desired baud rate: 4800
// actual: baud rate:4808 (0,2%)
// char size: 8 bit
// parity: Disabled
void uart0_init(void)
{
  UCSR0B = 0x00; //disable while setting baud rate
  UCSR0A = 0x00;
  UCSR0C = 0x06; //set character size to 8-bit
  UBRR0L = 0x0C; //set baud rate lo
  UBRR0H = 0x00; //set baud rate hi
  UCSR0B = _BV(RXCIE0) | _BV(TXCIE0) | _BV(RXEN0) | _BV(TXEN0);
}  

void dbg(char* buf)
{
  if( strlen((char*)tx_buf) + strlen(buf) + 2 <= TX_BUF_SIZE )
  {
    strcat((char*)&tx_buf, (const char*)buf);
    strcat((char*)&tx_buf, "\r\n");
  }
  if( tx_pos == 0 ) // data register empty
  {
    UDR0 = tx_buf[tx_pos];
    tx_pos++;
  }
  return;
}

ISR(USART0_RX_vect)
{
  rx_buf[rx_pos] = UDR0;
  rx_pos++;
}

ISR(USART0_TX_vect)
{
  if( tx_pos < TX_BUF_SIZE && tx_buf[tx_pos] != 0 )
  {
    UDR0 = tx_buf[tx_pos];
    tx_pos++;
  }
  else
  {
    tx_buf[0] = 0;
    tx_pos = 0;
  }
}

ISR(USART0_UDRE_vect)
{
  if( tx_pos < TX_BUF_SIZE && tx_buf[tx_pos] != 0 )
  {
    UDR0 = tx_buf[tx_pos];
    tx_pos++;
  }
}
