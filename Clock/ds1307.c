#include <avr\io.h>
#include "ds1307.h"

char TWI_S = 0;      // TWI Status

void delay(int cycles)
{
  while(cycles > 0)
    cycles--;
}

// ================= INITIALIZATION ==================

char twi_start()
{
  TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);   // Send START condition
  while (!(TWCR & (1<<TWINT)));             // Wait for TWINT flag set. This indicates that the START condition has been transmitted
  TWI_S = (TWSR & 0xF8);
  if ((TWI_S != START) && (TWI_S != RESTART))  // Check value of TWI Status Register. Mask prescaler bits. 
    return 1;                                // If status different from START go to ERROR
  else
    return 0;
}

char twi_stop()
{
  TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
  //delay(20);
  return 0;
}

char rtc_set_address(char addr)
{
  if(twi_set_write() != 0)
    return 1;
  twi_write(addr);
  return 0;
}

// =============== READING ====================

char twi_set_read()
{
  if(twi_start() != 0)
    return 1;
  
  TWDR = SLA_R;                             // Load slave address (SLA) + read bit(R) into TWDR Register.
  TWCR = (1<<TWINT) | (1<<TWEN);            // Clear TWINT bit in TWCR to start transmission of address
  while (!(TWCR & (1<<TWINT)));             // Wait for TWINT flag set. This indicates that the SLA+W has been transmitted, and ACK/NACK has been received.
  TWI_S = (TWSR & 0xF8);
  if (TWI_S != MR_SLA_ACK)          // Check value of TWI Status Register. Mask prescaler bits.
    return 1;                                // If status different from ST_SLA_ACK go to ERROR
  else
    return 0;
}

int twi_read(char ack)
{
  if(ack)
  {
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);  // If NOT the last read byte, send ACK after read

    while (!(TWCR & (1<<TWINT)));
    TWI_S = (TWSR & 0xF8);
    
    if(TWI_S != MR_DATA_ACK) 
      return ((1<<8) | 0x00);
    else
      return TWDR;
  }
  else
  {
    TWCR = (1<<TWINT) | (1<<TWEN) | (0<<TWEA);  // If the last read byte, send NACK after read

    while (!(TWCR & (1<<TWINT)));
    TWI_S = (TWSR & 0xF8);
    
    if(TWI_S != MR_DATA_NACK) 
      return ((1<<8) | 0x00);
    else
      return TWDR;
  }
}

clock_buffer rtc_read()
{
  clock_buffer buf = {0};
 
  rtc_set_address(0x00);
  
  if(twi_set_read() != 0)
    return buf;
  
  buf.seconds = twi_read(1);
  buf.minutes = twi_read(1);
  buf.hours = twi_read(1);
  buf.day = twi_read(1);
  buf.date = twi_read(1);
  buf.month = twi_read(1);
  buf.year = twi_read(0);
  
  twi_stop();
  return buf;
}

// ==================== WRITING =========================

char twi_set_write()
{
  if(twi_start() != 0)
    return 1;
  
  TWDR = SLA_W;                             // Load slave address (SLA) + write bit(W) into TWDR Register.
  TWCR = (1<<TWINT) | (1<<TWEN);            // Clear TWINT bit in TWCR to start transmission of address
  while (!(TWCR & (1<<TWINT)));             // Wait for TWINT flag set. This indicates that the SLA+W has been transmitted, and ACK/NACK has been received.
  TWI_S = (TWSR & 0xF8);
  if (TWI_S != MT_SLA_ACK)          // Check value of TWI Status Register. Mask prescaler bits.
    return 1;                                // If status different from ST_SLA_ACK go to ERROR
  else
    return 0;
}

char twi_write(char c)
{
  TWDR = c;                               // Load DATA into TWDR Register
  TWCR = (1<<TWINT) | (1<<TWEN);          // Clear TWINT bit in TWCR to start transmission of data
  while (!(TWCR & (1<<TWINT)));           // Wait for TWINT flag set. This indicates that the DATA has been transmitted, and ACK/NACK has been received
  TWI_S = (TWSR & 0xF8);
  if (TWI_S != MT_DATA_ACK)
    return 1;
  else
    return 0;
}

char rtc_write(clock_buffer buf)
{
  if(twi_set_write() != 0)
    return 8;
  twi_write(0x00);                       // Base address for writing time data
  
  if(twi_write(buf.seconds)!=0)
    return 7;
  if(twi_write(buf.minutes)!=0)
    return 6;
  if(twi_write(buf.hours)!=0)
    return 5;
  if(twi_write(buf.day)!=0)
    return 4;
  if(twi_write(buf.date)!=0)
    return 3;
  if(twi_write(buf.month)!=0)
    return 2;
  if(twi_write(buf.year)!=0)
    return 1;
  
  twi_stop();
  return 0;
}


int twi_read_byte(char addr)
{
  int data = 0;
  rtc_set_address(addr);
  
  if(twi_set_read() != 0)
    return ((1<<8) | 0x00);

  data = twi_read(0);
  twi_stop();
  
  return data;  
}

char twi_write_byte(char addr, char data)
{
  if(twi_set_write() != 0)
    return 2;
  
  twi_write(addr);                       // Base address for writing time data
  
  if(twi_write(data)!=0)
    return 1;

  twi_stop();
  return 0;
}

char to_bcd(char nr)
{
  return ( ((nr/10)<<4) | (nr%10) );
}

char from_bcd(char nr)
{
  return ( ((nr>>4)*10) + (nr & 0x0F) );
}

char bcd_inc(char nr)
{
  nr++;
  if((nr & 0x0F) == 0x0A)
    nr += 6;
  if(nr > 99)
    nr = 0;
  return nr;
}

char bcd_dec(char nr)
{
  nr--;
  if((nr & 0x0F) == 0x0F)
    nr -= 6;
  if(nr > 99)
    nr = 0;
  return nr;
}
