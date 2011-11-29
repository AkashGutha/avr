#include <avr\io.h>


#define OUT   7
#define SQWE  4
#define RS1   1
#define RS0   0

#define START         (0x08)
#define RESTART       (0x10)
#define SLA_W         (0xD0)
#define SLA_R         (0xD1)
#define MT_SLA_ACK    (0x18)
#define MR_SLA_ACK    (0x40)
#define MT_DATA_ACK   (0x28)
#define MR_DATA_ACK   (0x50)
#define MR_DATA_NACK  (0x58)

typedef struct
{
  unsigned char seconds;
  unsigned char minutes;
  unsigned char hours;
  
  unsigned char day;
  
  unsigned char date;
  unsigned char month;
  unsigned char year;
}
clock_buffer;

void delay(int cycles);

// Low-level TWI (I2C) communication :

char twi_start();
char twi_stop();
char twi_set_read();
char twi_set_write();
int  twi_read(char ack);
char twi_write(char);

int  twi_read_byte(char addr);
char twi_write_byte(char addr, char data);

// DS1307 communication :

char rtc_set_address(char);
clock_buffer rtc_read();
char rtc_write(clock_buffer);

// Misc. converwsion functions :

char to_bcd(char nr);
char from_bcd(char nr);
char bcd_inc(char nr);
char bcd_dec(char nr);
