#include <avr/io.h>
#include <avr/interrupt.h>
#include "ds18b20.h"
#include <stdio.h>
#include <util/delay.h>
#include "settings.h"

char temp_buf[12] = "";
char set_temp_buf[6] = "";
short current_temp = 0;
short set_temp = TEMP_DEFAULT;
short set_temp_dec = TEMP_DEFAULT * 6.25;
unsigned char status = 0;
unsigned char pause = 0;

unsigned char nr[13] = {0b00000011, 0b10011111, 0b00100101, 0b00001101, 0b10011001, 0b01001001, 0b01000001, 0b00011111, 0b00000001, 0b00001001, 0b00111001, 0b01100010, 0b11111101};
unsigned char mux[4] = {0xE,0xD,0xB,0x7};
unsigned char display[8] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};  // display buffer
unsigned char i = 0;
unsigned char mode = MODE_DISPLAY;

// ============= Blink parameters =========================

unsigned char blink_status = 1; // 1:display on, 0:display off
unsigned short blink_mode = 0; // if greater than 0, every nth trigger of the display mux interrupt the blink status is toggled.
unsigned char blink_counter = 0;
unsigned char wait_counter = 0;

// ============= Timer parameters =========================

uint8_t count0 = 75;
uint8_t prescaler0 = 4;

uint16_t count1 = TEMP_READ_PERIOD;
uint8_t prescaler1 = 5;

uint8_t count2 = 50;
uint8_t count2_1 = KEY_LOOP_SHORT;
uint8_t i2 = 0;
uint8_t prescaler2 = 7;

// ========================================================

void temp_to_buffer(short temp)
{
  short conv = temp * 6.25;
  
  sprintf(set_temp_buf,"%+05d", conv);

  if(conv < 0)
  {
    display[4] = nr[ CHAR_MINUS ];
    display[5] = nr[ set_temp_buf[2] - 0x30 ] & 0xFE;
    display[6] = nr[ set_temp_buf[3] - 0x30 ];
    display[7] = 0xFF;
  }
  else if(conv < 1000)
  {
    display[4] = 0xFF;
    display[5] = nr[ set_temp_buf[2] - 0x30 ] & 0xFE;
    display[6] = nr[ set_temp_buf[3] - 0x30 ];
    display[7] = 0xFF;
  }
  else// if(conv < 100)
  {
    display[4] = nr[ set_temp_buf[1] - 0x30 ];
    display[5] = nr[ set_temp_buf[2] - 0x30 ] & 0xFE;
    display[6] = nr[ set_temp_buf[3] - 0x30 ];
    display[7] = 0xFF;
  }
}

void init()
{
  cli();

  DDRC = 0xFF;
  PORTC = 0x00;

  DDRD = 0xFF;
  PORTD = 0xFF;

  DDRB = 0x03;
  PORTB = 0x00;
  //PORTB = 0xFF;

  //EIMSK |= (1<<INT0);
  //EICRA = 0x02;

  OCR0A = count0;
  TIMSK0 |= (1<<OCIE0A);
  TCCR0B |= prescaler0;

  OCR1A = count1;
  TIMSK1 |= (1<<OCIE1A);
  TCCR1B |= prescaler1;

  OCR2A = count2;
  TIMSK2 |= (1<<OCIE2A);
  TCCR2B |= prescaler2;


  /*
  ADMUX = (1<<REFS0) | (1<<ADLAR) | 0x07;
  ADCSRA = (1<<ADEN) | (1<<ADSC) | (1<<ADATE) | (1<<ADIE) | 0x07;
  */

  sei();  
}

int main()
{
  init();

  display[0] = nr[ CHAR_MINUS ];
  display[1] = nr[ CHAR_MINUS ];
  display[2] = nr[ CHAR_MINUS ];
  display[3] = nr[ CHAR_MINUS ];

  therm_set_resolution(THERM_RES_10BIT);
  temp_to_buffer(set_temp);

  while(1);
  return 0;
}

// Display multiplexing

ISR(TIMER0_COMPA_vect)
{
  TCNT0 = 0;
  PORTD = 0xFF;

  if( blink_mode > 0 )
  {
    blink_counter++;
    if( blink_counter == blink_mode )
    {
      blink_counter = 0;
      blink_status = 1 - blink_status;
    }
  }
  else
    blink_status = 1;

  PORTC = (PINC & 16) | mux[i];

  if( blink_status == 1 )
  {
    if( mode == MODE_SET )
      PORTD = display[ i + 4 ];
    else
      PORTD = display[ i ];
  }
  else
    PORTD = 0xFF;

  (i==3) ? i=0 : i++;
}

// Keyboard polling

ISR(TIMER2_COMPA_vect)
{
  TCNT2 = 0;
  i2++;
  if( i2 == count2_1 )
  {
    uint8_t keypad = PINB>>6;
    i2=0;
    if( keypad != 3 ) // key pressed
    {
      mode = MODE_SET;
      blink_mode = BLINK_MODE_SET;
      wait_counter = KEY_LOOP_WAIT; // the nr of loops to stay in set mode
      count2_1 = KEY_LOOP_LONG;  // slow sown the polling
      switch( keypad )
      {
        case 0b10:  // decrease temp.
          set_temp -= TEMP_STEP;
          if( set_temp < TEMP_MIN )
            set_temp = TEMP_MIN;
          break;

        case 0b01:  // increase temp.
          set_temp += TEMP_STEP;
          if( set_temp > TEMP_MAX )
            set_temp = TEMP_MAX;
          break;

        default:
          break;
      }
      temp_to_buffer(set_temp);
    }
    else if( pause == 0 ) // key released
    {
      count2_1 = KEY_LOOP_SHORT; // speed up polling

      if( wait_counter > 0 )
        wait_counter--;
      else
      {
        mode = MODE_DISPLAY;
        blink_mode = BLINK_MODE_NORMAL;
      }
    }
    else
    {
      mode = MODE_DISPLAY;
    }
  }
}

// Temperature reading

ISR(TIMER1_COMPA_vect, ISR_NOBLOCK)
{
  TCNT1 = 0;
  if( mode == MODE_DISPLAY )
  {
    current_temp = therm_read_temperature(temp_buf);

    if( temp_buf[11]<10 )
      PORTB |= (1<<PB1);
    else
      PORTB &= ~(0<<PB1);
    if( temp_buf[0] != 0 )
    {
      if( current_temp < 0 )
      {
        display[0] = nr[ CHAR_MINUS ];
        display[1] = nr[ temp_buf[3] - 0x30 ] & 0xFE;
        display[2] = nr[ temp_buf[5] - 0x30 ];
        display[3] = nr[ CHAR_C ];
      }
      else if( temp_buf[11]>=0 && temp_buf[11]<10 )
      {
        display[0] = 0xFF;
        display[1] = nr[ temp_buf[3] - 0x30 ] & 0xFE;
        display[2] = nr[ temp_buf[5] - 0x30 ];
        display[3] = nr[ CHAR_C ];
      }
      else if( temp_buf[11]>=10 && temp_buf[11]<100 )
      {
        display[0] = nr[ temp_buf[2] - 0x30 ];
        display[1] = nr[ temp_buf[3] - 0x30 ] & 0xFE;
        display[2] = nr[ temp_buf[5] - 0x30 ];
        display[3] = nr[ CHAR_C ];
      }
      else
      {
        display[0] = nr[ temp_buf[1] - 0x30 ];
        display[1] = nr[ temp_buf[2] - 0x30 ];
        display[2] = nr[ temp_buf[3] - 0x30 ] & 0xFE;
        display[3] = nr[ temp_buf[5] - 0x30 ];
      }
    }
    else  // DS18B20 init failed
    {
      display[0] = nr[ CHAR_MINUS ];
      display[1] = nr[ CHAR_MINUS ];
      display[2] = nr[ CHAR_MINUS ];
      display[3] = nr[ CHAR_MINUS ];
    }
  }


  if( pause == 0 )  
  {
    #ifdef FRIDGE

    if( current_temp >= set_temp + TEMP_DELTA )
    {
      status = 1;
      PORTC |= (1 << 4);
    }
    else if( current_temp <= set_temp - TEMP_DELTA )
    {
      status = 0;
      PORTC &= ~(1 << 4);
    }

    #else

    if( current_temp >= set_temp + TEMP_DELTA )
    {
      status = 0;
      PORTC &= ~(1 << 4);
    }
    else if( current_temp <= set_temp - TEMP_DELTA )
    {
      status = 1;
      PORTC |= (1 << 4);
    }

    #endif
  }
  else
  {
    PORTC &= ~(1 << 4);
  }
}

// =====================================================
// ==================  T  E  S  T  =====================
// =====================================================

/*
ISR(ADC_vect)
{
  char buf[4] = "";
  uint8_t i = 0;
  uint8_t value = ADCH;

  sprintf(buf, "%d", value );
  
  while(i<4)
  {
    if( buf[i] > 0 )
      display[i] = nr[ buf[i] - 0x30 ];
    else
      display[i] = 0xFF;
    i++;
  }
}
*/
