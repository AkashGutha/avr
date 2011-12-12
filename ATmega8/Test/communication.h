/*
 * communication.h
 *
 *  Created on: Dec 12, 2011
 *      Author: ro1v02y4
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#include <avr/eeprom.h>
#include "global.h"


void uart_init();
void codes_init();

char HexToByte(char *str, char start, char count, char *dest);

void send_message(char *msg);
void receive_message();

void Connect();
void Disconnect();
void SetData();
void GetData();

#endif /* COMMUNICATION_H_ */
