#include <avr\io.h>
#include <avr\interrupt.h>
#include <util/delay.h>
#include "ds1307.h"

void send(unsigned char digit, unsigned char data)
{
  if( digit < 4 )
  {
    cli();
    // Close mux & data lines :
    PORTC = 0x30;
    _delay_ms(5);
    PORTD = data;
    _delay_ms(5);
    // Open WE on the selected buffer :
    PORTC = 0x30 | (1 << digit);
    _delay_ms(5);
    // Close the mux & data lines :
    PORTC = 0x30;
    _delay_ms(5);
    PORTD = 0;
    _delay_ms(5);
    sei();
  }
}

#define DISP_TIME   (0)
#define DISP_DATE   (1)
#define DISP_YEAR   (2)
#define DISP_DAY   (3)

unsigned short count = 1562;
unsigned char prescaler = 3;
unsigned short press_count = 0;

unsigned char nr[10] = {0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,0x00,0x10};
unsigned char days[14] = {0x40,0x47,0x09,0x09,0x61,0x41,0x12,0x41,0x41,0x08,0x4F,0x40,0x4F,0x08};
unsigned char q = 0;
unsigned char port = 0;

unsigned char seconds = 0;
clock_buffer clock = {0};
unsigned char DST = 1;

unsigned char disp = DISP_TIME;

void refresh_display()
{
  switch( disp )
  {
    case DISP_TIME:
    {
      send(0, nr[ clock.hours >> 4 ] );
      send(3, nr[ clock.hours & 0xF ] );
      send(2, nr[ clock.minutes >> 4 ] );
      send(1, nr[ clock.minutes & 0xF ] );
      break;
    }

    case DISP_DATE:
    {
      send(0, nr[ clock.date >> 4 ] );
      send(3, nr[ clock.date & 0xF ] );
      send(2, nr[ clock.month >> 4 ] );
      send(1, nr[ clock.month & 0xF ] );
      break;
    }

    case DISP_YEAR:
    {
      send(0, nr[ 2 ] );
      send(3, nr[ 0 ] );
      send(2, nr[ clock.year >> 4 ] );
      send(1, nr[ clock.year & 0xF ] );
      break;
    }

    case DISP_DAY:
    {
      send(0, 0x7F );
      send(3, 0x7F );
      send(2, days[ clock.day - 1 ] );
      send(1, days[ clock.day + 6 ] );     
      break;
    }

    default: ;
  }
}

void init()
{
  DDRB = 0x01;
  PORTB |= (1<<PB6);  // internal pull-up for SQW_OUT

  DDRC = 0x0F;
  PORTC |= 0x30; // internal pull-ups for SDA and SCL

  DDRD = 0x7F;
  PORTD = 0;
  PORTC = 0x30 | 0xF; // broadcast

  PCICR |= (1<<PCIE0);
  PCMSK0 |= (1<<PCINT6);

  OCR1A = count;
  TIMSK1 |= (1<<OCIE1A);
  TCCR1B |= prescaler;
}

int main()
{
  cli();
  init();  
	sei();

  clock = rtc_read();
  seconds = (clock.seconds >> 4) * 10 + (clock.seconds & 0xF);
  q = twi_write_byte(7,( 0<<OUT | 1<<SQWE | 0<<RS1 | 0<<RS0 )); // Set DS1307 square wave output on, freq = 1Hz
  refresh_display();
  while(1);

  return 0;
}

ISR(PCINT0_vect)
{
  cli();
  PORTB = (PORTB & 0xFE) | ((PINB>>6) & 1);
  if( ((PINB>>6) & 1) == 0 )
    seconds++;
  
  if( seconds == 60 )
  {
    clock = rtc_read();
    seconds = (clock.seconds >> 4) * 10 + (clock.seconds & 0xF);
    
    // DST - on
    if( clock.month == 0x03 )
      if( clock.date > 0x25 )
        if( clock.day == 1 )
          if( clock.hours == 3 )
            if( clock.minutes == 0 )
              if( seconds == 0 )
              {
                DST = 1;
                clock.hours++;
                q = twi_write_byte(2,clock.hours);
              }

    // DST - off
    if( (clock.month == 0x10) && (DST == 1) )
      if( clock.date > 0x25 )
        if( clock.day == 1 )
          if( clock.hours == 4 )
            if( clock.minutes == 0 )
              if( seconds == 0 )
              {
                DST = 0;
                clock.hours--;
                q = twi_write_byte(2,clock.hours);
              }

    refresh_display();
  }
  
  sei();
}

ISR(TIMER1_COMPA_vect)
{
  TCNT1 = 0;
  uint8_t keypad = PINB & 0b10000110;
  cli();
  
  if( keypad == 0b10000110 )  // no key pressed
  {
    OCR1A = count;
  }
  else
  {
    OCR1A = 3 * count;

    switch(keypad)
    {
      case 0b10000100:  // hour++, date++, year++
      {
        if( disp == DISP_TIME )
        {
          clock.hours = bcd_inc(clock.hours);
          if(clock.hours == 0x24)
            clock.hours = 0;
          q = twi_write_byte( 2, clock.hours );
        }
        else if( disp == DISP_DATE )
        {
          clock.date = bcd_inc(clock.date);
          switch(clock.month)
          {
            case 0x01: case 0x03: case 0x05: case 0x07: case 0x08: case 0x10: case 0x12:
              if(clock.date == 0x32)
                clock.date = 1;
              break;
            case 0x04: case 0x06: case 0x09: case 0x11:
              if(clock.date == 0x31)
                clock.date = 1;
              break;
            default :
              if(clock.date == 0x29)
                clock.date = 1;
              break;
          }
          q = twi_write_byte( 4, clock.date );
        }
        else if( disp == DISP_YEAR )
        {
          clock.year = bcd_inc(clock.year);
          if(clock.year == 0x51 )
            clock.year = 0x11;
          q = twi_write_byte( 6, clock.year );
        }
        else if( disp == DISP_DAY )
        {
          ( clock.day == 7 ) ? clock.day = 1 : clock.day++;
          q = twi_write_byte( 3, clock.day );
        }

        refresh_display();
        break;
      }

      case 0b10000010:  // minute++, month++, year--
      {
        if( disp == DISP_TIME )
        {
          clock.minutes = bcd_inc(clock.minutes);
          if(clock.minutes == 0x60)
            clock.minutes = 0;
          q = twi_write_byte( 0, 0 );
          q = twi_write_byte( 1, clock.minutes );
        }
        else if( disp == DISP_DATE )
        {
          clock.month = bcd_inc(clock.month);
          if( clock.month == 0x13 )
            clock.month = 1;
          q = twi_write_byte( 5, clock.month );
        }
        else if( disp == DISP_YEAR )
        {
          clock.year = bcd_dec(clock.year);
          if( clock.year == 0x10 )
            clock.year = 0x50;
          q = twi_write_byte( 6, clock.year );
        }
        else if( disp == DISP_DAY )
        {
          ( clock.day == 1 ) ? clock.day = 7 : clock.day--;
          q = twi_write_byte( 3, clock.day );
        }

        refresh_display();
        break;
      }

      case 0b00000110:  // switch display mode
      {
        (disp == 3) ? disp = 0 : disp++;
        refresh_display();
        break;
      }

      default: ;
    }
  }
  sei();
}

