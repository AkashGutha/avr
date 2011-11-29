#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define TX_BUF_SIZE 255
#define RX_BUF_SIZE 32

//UART0 initialisation
// desired baud rate: 4800
// actual: baud rate:4808 (0,2%)
// char size: 8 bit
// parity: Disabled
void uart0_init(void);

void dbg(char* buf);
