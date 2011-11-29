#include <stdio.h>
#include <util/delay.h>
#include "ds18b20.h"

uint8_t therm_reset()
{
  uint8_t i;

  //Pull line low and wait for 480uS
  THERM_LOW();
  THERM_OUTPUT_MODE();
  _delay_us(480);

  //Release line and wait for 60uS
  THERM_INPUT_MODE();
  _delay_us(60);

  //Store line value and wait until the completion of 480uS period
  i=(THERM_PIN & (1<<THERM_DQ));
  _delay_us(420);

  //Return the value read from the presence pulse (0=OK, 1=WRONG)
  return i;
}

void therm_write_bit(uint8_t bit)
{
  //Pull line low for 1uS
  THERM_LOW();
  THERM_OUTPUT_MODE();
  _delay_us(1);

  //If we want to write 1, release the line (if not will keep low)
  if(bit) THERM_INPUT_MODE();

  //Wait for 60uS and release the line
  _delay_us(60);
  THERM_INPUT_MODE();
}

uint8_t therm_read_bit(void)
{
  uint8_t bit=0;

  //Pull line low for 1uS
  THERM_LOW();
  THERM_OUTPUT_MODE();
  _delay_us(1);

  //Release line and wait for 14uS
  THERM_INPUT_MODE();
  _delay_us(14);

  //Read line value
  if(THERM_PIN&(1<<THERM_DQ)) bit=1;

  //Wait for 45uS to end and return read value
  _delay_us(45);
  return bit;
}

uint8_t therm_read_byte(void)
{
  uint8_t i=8, n=0;
  while(i--)
  {
    //Shift one position right and store read value
    n>>=1;
    n|=(therm_read_bit()<<7);
  }
  return n;
}

void therm_write_byte(uint8_t byte)
{
  uint8_t i=8;
  while(i--)
  {
    //Write actual bit and shift one position right to make the next bit ready
    therm_write_bit(byte&1);
    byte>>=1;
  }
}

void therm_set_resolution(char res)
{
  //Reset, skip ROM and start temperature conversion
  therm_reset();
  therm_write_byte(THERM_CMD_SKIPROM);
  therm_write_byte(THERM_CMD_WSCRATCHPAD);
  therm_write_byte(0);
  therm_write_byte(0);
  therm_write_byte( 0x1F | (res<<5) );
  therm_reset();
}

short therm_read_temperature(char* buffer)
{
  // Buffer length must be at least 12bytes long! ["+XXX.XXXX C"]
  short value = 0;
  uint8_t temperature[2];
  int8_t digit;
  uint16_t decimal;

  //Reset, skip ROM and start temperature conversion
  if(therm_reset()>0)
  {
    buffer[0] = 0;
    return 0;
  }
  
  therm_write_byte(THERM_CMD_SKIPROM);
  therm_write_byte(THERM_CMD_CONVERTTEMP);

  //Wait until conversion is complete
  while(!therm_read_bit());

  //Reset, skip ROM and send command to read Scratchpad
  therm_reset();
  therm_write_byte(THERM_CMD_SKIPROM);
  therm_write_byte(THERM_CMD_RSCRATCHPAD);

  //Read Scratchpad (only 2 first bytes)
  temperature[0]=therm_read_byte();
  temperature[1]=therm_read_byte();
  therm_reset();

  //Store temperature integer digits and decimal digits
  value = (temperature[1]<<8) | temperature[0];
  if( value < 0 )  // negative value
  {
    uint16_t neg_temp = ~((temperature[1]<<8) | temperature[0]) + 1;
    temperature[0] = neg_temp & 0xFF;
    temperature[1] = neg_temp>>8;

    digit=temperature[0]>>4;
    digit|=(temperature[1]&0x7)<<4;
    digit *= -1;
  }
  else
  {
    digit=temperature[0]>>4;
    digit|=(temperature[1]&0x7)<<4;
  }
  //Store decimal digits
  decimal=temperature[0]&0xf;
  decimal*=THERM_DECIMAL_STEPS_12BIT;
  
  //Format temperature into a string [+XXX.XXXX C]
  sprintf(buffer, "%+04d.%04u", digit, decimal);
  buffer[11] = digit;

  return value;
}
