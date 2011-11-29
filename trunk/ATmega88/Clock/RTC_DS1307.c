// File: RTC_DS1307.c
// Credits: J F Main.
// Copyright : Copyright © John Main 2006
//   http://www.best-microcontroller-projects.com
//   Free for non commercial use as long as this entire copyright notice
//   is included in source code and any other documentation.
//
// Description:
//   A Real time clock using the Dallas chip DS1307.
//
// Compiler : PICC C compiler
//            for Microchip PIC microcontrollers
//   Target : 18F1320


//#include "I2C.c"

// macros / defines
#define UP    1
#define DOWN -1

//////////////////////////////////////////////////////////////////////
//typedef unsigned int Byte;
typedef unsigned int UINT8;

// File scope variables
static Byte ds1307_started = 0;

int int_to_bcd(BYTE data);
int bcd_to_int(BYTE data);

void write_DS1307(unsigned short address, unsigned short data) {
short i;

   I2C_Start();
   I2C_Write(0xd0);
   I2C_Write(address);
   I2C_Write(data);
   I2C_Stop();

   I2C_Start();

   // Get an ACK from the slave but give up after 3 goes don't hang waiting.
   for(i=0;i<3;i++) {
      if (I2C_Write(0xd0)==1) {  // Check for NACK
         I2C_start();
      } else break;            // found ACK
   }
   I2C_Stop();
}

//////////////////////////////////////////////////////////////////////
unsigned int read_ds1307(unsigned int address) {
unsigned int data;

   I2C_Start();
   I2C_Write(0xd0);
   I2C_Write(address);
   I2C_Start();
   I2C_Write(0xd1);           // b0 is read
   data= I2C_Read(0);         // Read with NACK.
   I2C_stop();
   return(data);
}


//////////////////////////////////////////////////////////////////////
void start_ds1307() {
Byte sec;

   if (ds1307_started) return;

   sec = read_ds1307(0) & ~0x80; // CH = 0 - start.
   write_DS1307(0,sec);
   ds1307_started = 1;
}

//////////////////////////////////////////////////////////////////////
void stop_ds1307() {
   Byte sec;
   sec = read_ds1307(0) | 0x80; // CH = 1 - stop.
   write_DS1307(0,sec);

   ds1307_started = 0;
}

//////////////////////////////////////////////////////////////////////
// Register update only happens if year data is not present.
// Have to change this next centuary!
//
void init_ds1307(void) {

   // Check if initialisation is needed by checking a ram location
   // use the year value : RAM location 3F is used to hold the year data

   if (read_ds1307(0x3f)==0x20) {return;} // initialised so return

   start_ds1307(); // Start the oscillator

   // Set to 24H mode
   write_ds1307(2,read_ds1307(2) & ~(1<<6) ); // reset bit 6 for 24 hour mode.

   // Set the time
   write_ds1307(0,0x59 & 0x7F); // bit 7 - oscailltor halt (if=1).  : secs  0-59
   write_ds1307(1,0x59);  //                                        : mins  0-59
   write_ds1307(2,0x23 & ~(1<<6) ); // reset bit 6 for 24 hour mode.:0-23,1-12 hour
   write_ds1307(3,0x07);  // week day day 0-7                       : day   1-7
   write_ds1307(4,0x31);  // (exceptions Feb,Apr,Jun,Sep,Nov 1-30)  : date  1-31
   write_ds1307(5,0x12);  //                                        : month 1-12
   write_ds1307(6,0x99);  //                                        : year  0-99
   // Write high digit of year into RAM easier code and gives initialisation check.
   write_ds1307(0x3f,0x20);    //                                   : year  20
}


//////////////////////////////////////////////////////////////////////
// Edit the DS1307 at address Addr
// add or subtract dir e.g. -1, +1, -10, +10.
//
// Stores the special case bits for
//   CH and 12/24hour
// restoring them at the end.
//
void edit_DS1307(Byte Addr, signed int8 dir) {
UINT8 store=0;
signed int8 lim,low_lim;
signed int data=0;

   data = read_ds1307(Addr); // Get the data.

   switch (Addr) { // Control special case bits
      case 0 : store = data & 0x80;  data &=0x7f; break; // CH Clock Halt.
      case 2 : store = data &(1<<6); data &=0x3f; break; // 12/24 Hour
   }

   data = bcd_to_int(data);    // Convert

   // Limits
   low_lim=0;
   switch (Addr) {
   case 0 :
   case 1 : lim=59; break;
   case 2 : lim=23; break;
   case 3 : lim=7;  low_lim=1; break;
   case 4 : lim=31; low_lim=1; break;
   case 5 : lim=12; low_lim=1; break;
   case 6 : lim=99; break;
   }

   data += dir;

   // Process limits
   if (data<low_lim) {
      data=lim;
   }
   else if (data>lim){
      data=low_lim;
   }

   // finish up and restore ctrl bits
   write_DS1307( Addr, int_to_bcd(data) | store );
}

int int_to_bcd(BYTE data)
{
   int nibh;
   int nibl;

   nibh=data/10;
   nibl=data-(nibh*10);

   return((nibh<<4)|nibl);
}

int bcd_to_int(BYTE data)
{
   int i;

   i=data;
   data=(i>>4)*10;
   data=data+(i<<4>>4);

   return data;
}
