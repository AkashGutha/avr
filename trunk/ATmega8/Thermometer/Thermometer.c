 /*
    Copyright (c) 2009  Gediminas Labutis                   
   
  Permission is hereby granted, free of charge, to any       
 person obtaining a copy of this software and associated     
 documentation files (the "Software"), to deal in the        
 Software without restriction, including without             
 limitation the rights to use, copy, modify, merge, publish, 
 distribute, sublicense, and/or sell copies of the Software, 
 and to permit persons to whom the Software is furnished to  
 do so, subject to the following conditions:                 
  
   The above copyright notice and this permission notice     
 shall be included in all copies or substantial portions of  
 the Software.                                               
      
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF       
 ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED     
 TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         
 PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL   
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF         
 CONTRACT, TORT OR OTHERWISE, ARISING FROM,                  
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     
 OTHER DEALINGS IN THE SOFTWARE.   

*/

/*
             1-Wire Temperature Alarm     V1.0  
                 
              Compiler - WinAVR 20081205

  1-Wire code from Atmel appnote
  AVR318: Dallas 1-Wire master 
              
*/

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdio.h>
#include <util/delay.h>
#include "OWI.h" 


#define BUSES   (OWI_PIN_0) //!< Buses to search.
//#define F_CPU   1000000 //

#define HighLimitPort PIND   // High limit switch port 
#define LowLimitPort  PINB   // Low limit switch
#define SignPort      PINC   // Sing 
#define LLSignPin     PIN5   // Low level sign pin
#define HLSignPin     PIN4   // High level sign pin
#define Ind_Port      PORTC  // indication port
#define LLED          PIN2   // Low level LED
#define HLED          PIN3   // High level LED
#define Buzzer        PIN1   // Buzzer 

#define bit_get(p,m) ((p) & (m)) 
#define bit_set(p,m) ((p) |= (m)) 
#define bit_clear(p,m) ((p) &= ~(m)) 
#define bit_flip(p,m) ((p) ^= (m)) 
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m)) 
#define BIT(x) (0x01 << (x)) 
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))

#define FAMILY_DS18B20 0x28
#define FAMILY_DS18S20 0x10

#define OK 0
#define HIGH_LIMIT   1
#define LOW_LIMIT    2
#define NO_SENSOR    3
#define WRONG_LIMITS 4


int8_t temperature = 0 ;         // measured temperature
volatile int8_t check_temp ;     // 1-time to check temperature

int abs  (int i); // to avoid warning 

void port_init(void)
{
	PORTB = 0xFF;
	DDRB  = 0x00;
	PORTC = 0x71; 
	DDRC  = 0x0E;
	PORTD = 0xFF;
	DDRD  = 0x00;
}

///TIMER1 initialize - prescale:1024
// WGM: 0) Normal, TOP=0xFFFF
// desired value: 1Sec
// actual value:  1,000Sec (0,0%)
void timer1_init(void)
{
	TCCR1B = 0x00; //stop
	TCNT1H = 0xF0; //setup
	TCNT1L = 0xBE;
	OCR1AH = 0x0F;
	OCR1AL = 0x42;
	OCR1BH = 0x0F;
	OCR1BL = 0x42;
	ICR1H  = 0x0F;
	ICR1L  = 0x42;
	TCCR1A = 0x00;
	TCCR1B = 0x05; //start Timer
}


ISR(TIMER1_OVF_vect)
{
	//TIMER1 has overflowed
	TCNT1H = 0xF0; //reload counter high value
	TCNT1L = 0xBE; //reload counter low value

	uint8_t static count; 

	count++;
}

//call this routine to initialize all peripherals
void init_devices(void)
{
	//stop errant interrupts until set up
	cli(); //disable all interrupts
	port_init();
	timer1_init();

	MCUCR = 0x00;
	GICR  = 0x00;
	TIMSK = 0x05; //timer interrupt sources
	sei(); //re-enable interrupts
	//all peripherals are now initialized
}


int8_t DS1820_ReadTemperature(uint8_t bus)
{
	uint8_t tmp1;
	uint8_t tmp2;
	static uint8_t FAMILY_CODE;
    
  // Reset, presence.
  if (!OWI_DetectPresence(bus))
  {
      return -127; // Error
  }
  // Read ROM to check family code
	OWI_SendByte(OWI_ROM_READ, bus);
  FAMILY_CODE = OWI_ReceiveByte(bus); 

  if (!OWI_DetectPresence(bus))
  {
      return -127; // Error
  }
  
  OWI_SendByte(OWI_ROM_SKIP, bus);
  // Send start conversion command.
  OWI_SendByte(OWI_START_CONVERSION, bus);
  // Wait until conversion is finished.
  // Bus line is held low until conversion is finished.
  while (!OWI_ReadBit(bus))
  {
  
  }
  // Reset, presence.
  if(!OWI_DetectPresence(bus))
  {
      return -127; // Error
  }
  
  OWI_SendByte(OWI_ROM_SKIP, bus);
  // Send READ SCRATCHPAD command.
  OWI_SendByte(OWI_READ_SCRATCHPAD, bus);

	// Read only two first bytes (temperature low, temperature high)
  tmp1 = OWI_ReceiveByte(bus);
	tmp2 = OWI_ReceiveByte(bus);
	
	switch(FAMILY_CODE)
	{
     case FAMILY_DS18S20 :
     return  ((tmp2 & 0x80) | (tmp1 >> 1)); // DS18S20
     break;
	 
	 case FAMILY_DS18B20 :
     return  ((tmp2 << 4) | (tmp1 >> 4)); // DS18B20
     break;
   
     default:                            // should never go here
     return -127; // Error
    }
    
}  


//
int main(void)
{
	init_devices();

	// stdout = &mystdout; 

	// on reset all outputs set to "1".

	OWI_Init(BUSES);
	bit_set(Ind_Port, BIT(LLED));
	bit_set(Ind_Port, BIT(HLED));
	bit_set(Ind_Port, BIT(Buzzer));

	while(1)
	{
		temperature = DS1820_ReadTemperature(BUSES);
	}
 
	return 0;
}
