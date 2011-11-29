#include <avr/io.h>
#include "ds18b20.h"
#include <util/delay.h>

// ========================= LOW-LEVEL FUNCTIONS ======================

void ddr_in()
{
  _DDR &= ~(1<<_PIN);
  //_PORT |= (1<<_PIN);
}

void ddr_out(unsigned char value)
{
  _DDR |= (1<<_PIN);
  if( value > 0 )
    _PORT |= (1<<_PIN);
  else
    _PORT &= ~(1<<_PIN);
}

unsigned char read_pin()
{
  return (_PORT & (1<<_PIN))>>_PIN;
}

void write_bit(unsigned char bit)
{
  ddr_out(0);
  _delay_us(SLOT_START_W);
  if( bit > 0 )
    ddr_in();
  _delay_us(SLOT_SAMPLE);
}

unsigned char read_bit()
{
  unsigned char bit = 0;
  ddr_out(0);
  _delay_us(SLOT_START_R);
  ddr_in();
  bit = read_pin();
  _delay_us(SLOT_SAMPLE);
  return bit;
}

void ds_reset()
{
  ddr_out(0);
  _delay_us(RESET_PULSE);
  ddr_in();
  while( read_pin() != 0 );
  _delay_us(RESET_PULSE);
  return;
}

// ============================= BYTE READ & WRITE ========================


void ds_write(unsigned char data)
{
  unsigned char i = 0;
  while( i < 8 )
  {
    write_bit( data & 1 );
    data = data >> 1;
    _delay_us(SLOT_RECOVER);
    i++;
  }
}

unsigned char ds_read()
{
  unsigned char i = 0;
  unsigned char data = 0;
  while( i < 8 )
  {
    data = data << 1;
    data += read_bit();// & 1;
    _delay_us(SLOT_RECOVER);
    i++;
  }
  return data;
}

// =============================================

void ds_convert_temp()
{
  ds_reset();
  _delay_us(SLOT_RECOVER);
  ds_write(SKIP_ROM);
  ds_write(CONVERT_T);
  while( read_bit() == 0 )
    _delay_us(SLOT_RECOVER);
}

