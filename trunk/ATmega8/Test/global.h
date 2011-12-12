/*
 * global.h
 *
 *  Created on: Dec 12, 2011
 *      Author: ro1v02y4
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#define MAX_CODE_COUNT	50

typedef struct
{
	uint32_t ir_code;
	uint16_t key_code;
}
RemoteCode;

#if defined (__AVR_ATmega128__)

#define USART_RXC_vect	USART0_RX_vect

#define UCSRA	(UCSR0A)
#define	UCSRB	(UCSR0B)
#define	UBRR	(UBRR0)
#define	UBRRH	(UBRR0H)
#define	UBRRL	(UBRR0L)
#define	UDR		(UDR0)

#endif

#endif /* GLOBAL_H_ */
