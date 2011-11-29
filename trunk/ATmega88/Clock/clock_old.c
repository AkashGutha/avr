
#define DISPLAY_TIME  (0)
#define DISPLAY_DATE  (1)

// TIMER0 :
char count0 = 100;
char prescaler0 = 7;             // clk/1024

// TIMER1 :
unsigned int keyboard_poll = 1562;    // 100 msec
unsigned int keyboard_delay = 15625;  // 1 sec
unsigned char prescaler1 = 3;

unsigned short inverted_display = 1;

unsigned char nr[16] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0x88,0x83,0xC6,0xA1,0x86,0x8E};
unsigned char q = 0;

//char mux[4] = {0xFE,0xFD,0xFB,0xF7};
char mux[4] = {1,2,4,8};

char display[4] = {0xFF,0xFF,0xFF,0xFF};
//char display[4] = {0xF9,0xB0,0x80,0xFF};
//char display[4] = {0xF9,0xF9,0xF9,0xF9};

unsigned short keyboard = 0;
unsigned short int_dir = 0;
unsigned short tick = 0;

clock_buffer clock = {0};

void invert_display(unsigned short bInvert)
{
	unsigned short i = 0;
	if( bInvert > 0 )
	{
		for( i=0; i<16; i++ )
		{
			nr[i] = 0xFF - nr[i];
		}
	}
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

void set_display(char set)
{
  switch(set)
  {
    case DISPLAY_TIME :
      display[0] = nr[(clock.hours >> 4)];
      display[1] = nr[(clock.hours & 0xF)];
      display[2] = nr[(clock.minutes >> 4)];
      display[3] = nr[(clock.minutes & 0xF)];
      break;
    case DISPLAY_DATE :
      display[0] = nr[(clock.date >> 4)];
      display[1] = nr[(clock.date & 0xF)];
      display[2] = nr[(clock.month >> 4)];
      display[3] = nr[(clock.month & 0xF)];
      break;
    default : 
      break;
  }
}

void init()
{
  // PORTS
  
	DDRA	 = 0xFF;										// PORTA = out
	PORTA	 = 0xFF;
  DDRC   = 0xFF;                    // PORTC = out
  PORTC  = 0xFF;
  DDRD   = 0x00;                    // PORTD = in
  //DDRC   = 0x00;                    // PORTC = in
  PORTD  = (1<<0) | (1<<1);         // Enable internal pull-ups on SDA and SCL
  
  // EXTERNAL INTERRUPTS

  asm("cli"); 
  //MCUCR = 0x0E;           												// Set INT1 sense control
  //MCUCR |= (1<<1)|(0<<0);           							// Set INT0 sense control
  //MCUCR = 0x0E;
  //GICR  |= (1<<6)|(1<<7);           							// Enable external INT0, INT1
	EICRA |= (1<<ISC31)|(1<<ISC30)|(1<<ISC21)|(1<<ISC20);	// Set INT3, INT2 sense control to rising edge
	EICRB  = (1<<ISC71)|(1<<ISC70);											// Set INT7 sense control to rising edge
	EIMSK |= (1<<INT2)|(1<<INT3)|(1<<INT7);						// Enable INT2,INT3,INT7

  // TIMER 1 - used for handling the keyboard
  
  TCCR1B = 0x00;      //stop
  TCNT1 = 0x00;
  OCR1A = keyboard_poll;      // set output compare A
  TIMSK |= (1<<OCIE1A);      // Set output compare A interrupt
  TCCR1A = 0x00;  
  TCCR1B = prescaler1; //start Timer

  // STATUS REGISTER
  
  //SREG  |= (1<<7);                  // Set general interrupt flag
  //GIFR |= (0<<7);
  asm("sei");
}

// ============== MAIN ===================

int main()
{
  //clock_buffer test = {1,2,3,4,5,6,7};
  init();
	invert_display(inverted_display);

  q = twi_write_byte(0,0);
  clock = rtc_read();
  q = twi_write_byte(7,( 0<<OUT | 1<<SQWE | 0<<RS1 | 0<<RS0 )); // Set DS1307 square wave output on, freq = 1Hz
  set_display(DISPLAY_TIME);
	
	//DDRA = 0xFF;
	//PORTA = mux[0];
	//DDRC = 0xFF;
	//PORTC = nr[3];
  
	while(1);

  return 0;
}

// ============== INT ====================

ISR(INT2_vect)
{
	EIMSK -= 4; // disable INT2
  clock.hours = bcd_inc(clock.hours);
  if(clock.hours == 0x24)
    clock.hours = 0;
  q = twi_write_byte(2,clock.hours);
	EIMSK += 4; // enable INT2
}

ISR(INT3_vect)
{
	EIMSK -= 8;	// disable INT3
  clock.minutes = bcd_inc(clock.minutes);
  if(clock.minutes == 0x60)
    clock.minutes = 0;
  q = twi_write_byte(1,clock.minutes);
	EIMSK += 8;	// enable INT3
}

ISR(INT7_vect)
{
  // set up interrupt triggering :
  clock.seconds++;
  if(clock.seconds & 1)
		EICRB = (1<<ISC71)|(0<<ISC70);											// Set INT7 sense control to rising edge
  else
		EICRB = (1<<ISC71)|(1<<ISC70);											// Set INT7 sense control to rising edge

  if(clock.seconds == 60)
  {
    clock = rtc_read();
  }
}

ISR(TIMER0_OVF_vect)
{
  TCNT0 = 0xFF - count0;        // reset counter
  tick++;
  tick &= 3;
  
  set_display(DISPLAY_TIME);

  PORTC = (inverted_display == 0) ? 0xFF : 0x00;
  PORTA = mux[tick];
  PORTC = display[tick];  
}

ISR(TIMER1_COMPA_vect)
{
  TCNT1 = 0x00;
  keyboard = PINC & 0x0F;
  //set_display(DISPLAY_TIME);

	/*
  if(keyboard == 0x0F)
  {
    set_display(DISPLAY_TIME);
    OCR1A = keyboard_poll;
  }
  else
  {
    OCR1A = keyboard_delay;
    switch(keyboard)
    {
      case 0x0E:  // date button pressed
        set_display(DISPLAY_DATE);
        break;
        
      case 0x0D:
        set_display(DISPLAY_TIME);
        clock.hours = bcd_inc(clock.hours);
        if(clock.hours == 0x24)
          clock.hours = 0;
        q = twi_write_byte(2,clock.hours);
        break;

      case 0x0B:
        set_display(DISPLAY_TIME);
        clock.minutes = bcd_inc(clock.minutes);
        if(clock.minutes == 0x60)
          clock.minutes = 0;
        q = twi_write_byte(1,clock.minutes);
        break;
        
      case 0x0C:
        set_display(DISPLAY_DATE);
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
        q = twi_write_byte(4,clock.date);
        break;
        
      case 0x0A:
        set_display(DISPLAY_DATE);
        clock.month = bcd_inc(clock.month);
        if(clock.month == 0x13)
          clock.month = 1;
        q = twi_write_byte(5,clock.month);
        break;

      default :
        break;
    }
  }
	*/
}