#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>


#define nop()				__asm__ __volatile__ ("nop");
#define reset()				__asm__ __volatile__ ("rjmp 0");

#define ABS(x)				((x) < 0 ? -(x) : (x))

#define NUM_CODES			(sizeof(code_map) / sizeof(struct code_entry))

/* ir_state values */
#define IR_IDLE				0
#define IR_HEADER			1
#define IR_DATA_HI			2
#define IR_DATA_LO			3
#define IR_DISCARD			4

/* mouse acceleration values */
#define IR_ACCEL_DEF		2
#define IR_ACCEL_INC_LO		1
#define IR_ACCEL_INC_HI		2
#define IR_ACCEL_MAX		50

/* remote codes */
#define IRCODE_UP			0x30cf
#define IRCODE_DOWN			0x08f7
#define IRCODE_LEFT			0x10ef
#define IRCODE_RIGHT		0x20df
#define IRCODE_FULL			0xc03f
#define IRCODE_FINE_LT		0x9867
#define IRCODE_FINE_RT		0x18e7
		    
#define IR_TIMEOUT			8	/* in 21.8ms intervals */
#define IR_BUFLEN			8	/* this _must_ be an even value */


// LED default on :
//#define led_on()			do { PORTB |= _BV(0); } while (0)
//#define led_off()			do { PORTB &= ~_BV(0); } while (0)
//#define led_toggle()		do { PORTB ^= _BV(0); } while (0)

// LED default off :
#define led_on()			do { PORTB &= ~_BV(0); } while (0)
#define led_off()			do { PORTB |= _BV(0); } while (0)
#define led_toggle()		do { PORTB ^= _BV(0); } while (0)

//#define led_on()			do { PORTC &= ~_BV(4); } while (0)
//#define led_off()			do { PORTC |= _BV(4); } while (0)
//#define led_toggle()		do { PORTC ^= _BV(4); } while (0)

//#define led_on()			do { PORTC |= _BV(4); } while (0)
//#define led_off()			do { PORTC &= ~_BV(4); } while (0)
//#define led_toggle()		do { PORTC ^= _BV(4); } while (0)
