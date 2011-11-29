/* Name: infrahid.c
 * Project: InfraHID; CIR to HID mouse/keyboard converter
 * Author: Alex Badea
 * Creation Date: 2007-01-14
 * Tabsize: 8
 * Copyright: (c) 2007 Alex Badea <vamposdecampos@gmail.com>
 * License: Proprietary, free under certain conditions. See Documentation.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "oddebug.h"
#include "defines.h"


/* ------------------------------------------------------------------------- */

static uchar inputBuffer1[5] = { 1, 0,0,0,0, };		/* kbd */
static uchar inputBuffer2[4] = { 2, 0,0,0, };		/* mouse */
static uchar idleRate = 0;				/* in 4 ms units */
static uchar tx_kbd = 0;
static uchar tx_mouse = 0;

static uchar ir_state = IR_IDLE, ir_bits = 0;
static unsigned long ir_data = 0;
static uchar ir_timeout = 0;
static uchar ir_accel = IR_ACCEL_DEF;

#define IR_BUFLEN	8	/* this _must_ be an even value */
static unsigned short ir_buf[IR_BUFLEN];
static uchar ir_head = 0;
static uchar ir_tail = 0;


static struct code_entry {
	unsigned short remote_code;
	uchar hid_code;
} code_map[] = {
//	{ 0xC03F, 0xf0 },	/* Full -> mouse button 1 */
//	{ 0x28D7, 0xf1 },	/* Mute -> mouse button 2 */
//	{ 0xD827, 0xf2 },	/* MTS -> mouse button 3 */

	{ 0xA05F, 0x1E },	// 1 
	{ 0x609F, 0x1F },	// 2 
	{ 0xE01F, 0x20 },	// 3 
	{ 0x906F, 0x21 },	// 4 
	{ 0x50AF, 0x22 },	// 5 
	{ 0xD02F, 0x23 },	// 6 
	{ 0xB04F, 0x24 },	// 7 
	{ 0x708F, 0x25 },	// 8 
	{ 0xF00F, 0x26 },	// 9 
	{ 0x48B7, 0x27 },	// 0 
	{ 0x0086, 0x06 },	// .
	{ 0x0AF5, 0x29 },	// Esc 
	{ 0xC837, 0x28 },	// Enter 
	{ 0x30CF, 0x52 },	// Up 
	{ 0x08F7, 0x51 },	// Down 
	{ 0x10EF, 0x50 },	// Left 
	{ 0x20DF, 0x4F },	// Right 
	{ 0xF20D, 0x2B },	// TAB 
	{ 0x7A85, 0x2E },	// Vol+
	{ 0x5AA5, 0x2D },	// Vol-
	{ 0x28D7, 0x00 },	// Mute
	{ 0x7887, 0x00 },	// Video
	{ 0xC23D, 0x13 },	// Play
	{ 0xD827, 0x0A },	// Audio
	{ 0x926D, 0x4D },	// CLOSE PROGRAM
	{ 0xC03F, 0x31 },	// FULLSCREEN
	{ 0x8A75, 0x00 },	// APP SWITCH
	{ 0xF807, 0x17 },	// T
	{ 0x02FD, 0x00 },	// SLEEP
	{ 0x22DD, 0x09 },	// FFWD
	{ 0x42BD, 0x15 },	// REW
	{ 0x8877, 0x2A },	// BACK

	{ 0xD22D, 0x0F },	// RED
	{ 0x32CD, 0x0A },	// GREEN
	{ 0xB24D, 0x4C },	// YELLOW
	{ 0x728D, 0x49 },	// BLUE

	{ 0x6897, 0x10 },	// OSD
	{ 0xCA35, 0x00 },	// EPG
	{ 0x12ED, 0x00 },	// SNAPSHOT
	{ 0xC03F, 0x00 },	// FULLSCREEN
	{ 0xA25D, 0x00 },	// TIMER
	{ 0x2AD5, 0x36 },	// PREV
	{ 0xAA55, 0x37 },	// NEXT
	{ 0x629D, 0x1B },	// STOP
	{ 0xE21D, 0x2C },	// REC
	{ 0x6A95, 0x00 },	// TV
	{ 0x52AD, 0x4A },	// RESIZE
	{ 0xDA25, 0x4B },	// PAGE UP
	{ 0xFA05, 0x4E },	// PAGE DOWN
	{ 0x00FF, 0x66 },	// Power 
};

#define NUM_CODES	(sizeof(code_map) / sizeof(struct code_entry))

static const struct {
	unsigned short
		header_hi, header_lo, repeat_lo,
		bit_hi, one_lo, zero_lo;
} irconf = {
/* original values (in microseconds)
	9062, 4442, 2193,
	616, 1633, 508,
*/
/* values in 5.33us units: */
	1699, 833, 411,
	115, 306, 95,
};


/* ------------------------------------------------------------------------- */

static const char hidReportDescriptor0[] PROGMEM = {
	/* partial keyboard */
	0x05, 0x01,	/* Usage Page (Generic Desktop), */
	0x09, 0x06,	/* Usage (Keyboard), */
	0xA1, 0x01,	/* Collection (Application), */
	0x85, 0x01,		/* Report Id (1) */
	0x95, 0x04,		/* Report Count (4), */
	0x75, 0x08,		/* Report Size (8), */
	0x15, 0x00,		/* Logical Minimum (0), */
	0x25, 0xFF,		/* Logical Maximum(101), */
	0x05, 0x07,		/* Usage Page (Key Codes), */
	0x19, 0x00,		/* Usage Minimum (0), */
	0x29, 0xFF,		/* Usage Maximum (101), */
	0x81, 0x00,		/* Input (Data, Array),               ;Key arrays (4 bytes) */
	0xC0,		/* End Collection */

	/* mouse */
	0x05, 0x01,	/* Usage Page (Generic Desktop), */
	0x09, 0x02,	/* Usage (Mouse), */
	0xA1, 0x01,	/* Collection (Application), */
	0x09, 0x01,	/*   Usage (Pointer), */
	0xA1, 0x00,	/*   Collection (Physical), */
	0x05, 0x09,		/* Usage Page (Buttons), */
	0x19, 0x01,		/* Usage Minimum (01), */
	0x29, 0x03,		/* Usage Maximun (03), */
	0x15, 0x00,		/* Logical Minimum (0), */
	0x25, 0x01,		/* Logical Maximum (1), */
	0x85, 0x02,		/* Report Id (2) */
	0x95, 0x03,		/* Report Count (3), */
	0x75, 0x01,		/* Report Size (1), */
	0x81, 0x02,		/* Input (Data, Variable, Absolute), ;3 button bits */
	0x95, 0x01,		/* Report Count (1), */
	0x75, 0x05,		/* Report Size (5), */
	0x81, 0x01,		/* Input (Constant),                 ;5 bit padding */
	0x05, 0x01,		/* Usage Page (Generic Desktop), */
	0x09, 0x30,		/* Usage (X), */
	0x09, 0x31,		/* Usage (Y), */
	0x15, 0x81,		/* Logical Minimum (-127), */
	0x25, 0x7F,		/* Logical Maximum (127), */
	0x75, 0x08,		/* Report Size (8), */
	0x95, 0x02,		/* Report Count (2), */
	0x81, 0x06,		/* Input (Data, Variable, Relative), ;2 position bytes (X & Y) */
	0xC0,		/*   End Collection, */
	0xC0,		/* End Collection */
};


static const char configDescriptor[] PROGMEM = {	/* USB configuration descriptor */
	9,			/* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
	USBDESCR_CONFIG,	/* descriptor type */
	9 + 1 * (9 + 9 + 7), 0,
	/* total length of data returned (including inlined descriptors) */
	1,			/* number of interfaces in this configuration */
	1,			/* index of this configuration */
	0,			/* configuration name string index */
	USBATTR_BUSPOWER,	/* attributes */
	USB_CFG_MAX_BUS_POWER / 2,	/* max USB current in 2mA units */

/* --- interface 0 --- */
	9,			/* sizeof(usbDescrInterface): length of descriptor in bytes */
	USBDESCR_INTERFACE,	/* descriptor type */
	0,			/* index of this interface */
	0,			/* alternate setting for this interface */
	1,			/* endpoints excl 0: number of endpoint descriptors to follow */
	3,			/* 3=HID class */
	0,			/* subclass: 0=none, 1=boot */
	0,			/* protocol: 0=none, 1=keyboard, 2=mouse */
	0,			/* string index for interface */

	/* HID descriptor */
	9,			/* sizeof(usbDescrHID): length of descriptor in bytes */
	USBDESCR_HID,		/* descriptor type: HID */
	0x01, 0x01,		/* BCD representation of HID version */
	0x00,			/* target country code */
	0x01,			/* number of HID Report (or other HID class) Descriptor infos to follow */
	0x22,			/* descriptor type: report */
	sizeof(hidReportDescriptor0), 0,	/* total length of report descriptor */

	/* endpoint descriptor for endpoint 1 */
	7,			/* sizeof(usbDescrEndpoint) */
	USBDESCR_ENDPOINT,	/* descriptor type = endpoint */
	0x81,			/* IN endpoint number 1 */
	0x03,			/* attrib: Interrupt endpoint */
	8, 0,			/* maximum packet size */
	USB_CFG_INTR_POLL_INTERVAL,	/* in ms */
};

uchar usbFunctionDescriptor(usbRequest_t * rq)
{
	uchar *p = 0, len = 0;

	DBG1(0xa8, &rq->wValue.bytes[1], 1);

#if 0	/* default descriptor from driver */
	if (rq->wValue.bytes[1] == USBDESCR_DEVICE) {
		p = (uchar *) deviceDescriptor;
		len = sizeof(deviceDescriptor);
	} else
#endif
	if (rq->wValue.bytes[1] == USBDESCR_CONFIG) {
		p = (uchar *) configDescriptor;
		len = sizeof(configDescriptor);
	} else if (rq->wValue.bytes[1] == USBDESCR_HID) {
		p = (uchar *) (configDescriptor + 18);
		len = 9;
	} else if (rq->wValue.bytes[1] == USBDESCR_HID_REPORT) {
		p = (uchar *) hidReportDescriptor0;
		len = sizeof(hidReportDescriptor0);
	}
	usbMsgPtr = p;
	return len;
}


/* ------------------------------------------------------------------------- */



uchar usbFunctionSetup(uchar data[8])
{
	usbRequest_t *rq = (void *) data;

	DBG1(0xa0, data, 8);

	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {	/* class request type */
		if (rq->bRequest == USBRQ_HID_GET_REPORT) {
			/* wValue: ReportType (highbyte), ReportID (lowbyte) */
			if (rq->wValue.bytes[0] == 1) {
				usbMsgPtr = inputBuffer1;
				return sizeof(inputBuffer1);
			} else if (rq->wValue.bytes[0] == 2) {
				usbMsgPtr = inputBuffer2;
				return sizeof(inputBuffer2);
			}
			return 0;
		} else if (rq->bRequest == USBRQ_HID_SET_REPORT) {
			/* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* we have no output/feature reports */
			return 0;
		} else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
			usbMsgPtr = &idleRate;
			return 1;
		} else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
			idleRate = rq->wValue.bytes[1];
		}
		return 0;
	}

	return 0;
}

/* ------------------------------------------------------------------------- */



static void send_packets(void)
{
	/* send pending packets */
	if (usbInterruptIsReady()) {
		if (tx_kbd) {
			usbSetInterrupt(inputBuffer1, sizeof(inputBuffer1));	/* send kbd event */
			tx_kbd = 0;
		} else if (tx_mouse) {
			usbSetInterrupt(inputBuffer2, sizeof(inputBuffer2));	/* send mouse event */
			tx_mouse = 0;
		}
	}
}

/* ------------------------------------------------------------------------- */

/* "more thorough" reset: loop 4 times outputting SE0 then idle.
 * This works around some intermitent connection problems. */
static void usbReset(void)
{
	uchar i, j, k;

	for (k = 0; k < 8; k++) {
		/* on even iterations, output SE0 */
		if (!(k & 1)) {
			PORTD &= ~USBMASK;	/* no pullups on USB pins */
			DDRD |= USBMASK;	/* output SE0 for USB reset */
		} else {
			DDRD &= ~USBMASK;	/* set USB data as inputs */
		}

		/* delay */
		j = 0;
		while (--j) {		/* USB Reset by device only required on Watchdog Reset */
			i = 0;
			while (--i) {	/* delay >10ms for USB reset */
				nop();
				nop();
			}
		}
	}
}

/* ------------------------------------------------------------------------- */

SIGNAL(SIG_INTERRUPT1)
{
	ir_buf[ir_head] = TCNT1;	/* store pulse length */
	ir_head = (ir_head + 1) % IR_BUFLEN;

	MCUCR ^= _BV(ISC10);		/* toggle rising/falling edge */
	TCNT1 = 0;			/* clear counter */
}


/* ------------------------------------------------------------------------- */

static void hid_clear(void)
{
   inputBuffer2[1] = 0;   /* mouse buttons */
   inputBuffer2[2] = 0;   /* X axis delta */
   inputBuffer2[3] = 0;   /* Y axis delta */

   inputBuffer1[1] = 0;   /* key */
}

static uchar ir_check(const unsigned short ref, unsigned short value)
{
   short delta = ABS((short) value - (short) ref);

   if (delta > 100)      /* aeps = 100us */
      return 0;
   if (delta > ref / 3)      /* eps = 30%*/
      return 0;
   return 1;
}

static uchar ir_reset(void)
{
   ir_state = IR_IDLE;
   ir_bits = 0;
   return 0;
}

static void ir_input(void)
{
   char repeat = (ir_state == IR_HEADER);
   uchar k;

   if (ir_data == 0xa857)   /* "scan" button */
      reset();   /* XXX HACK */

   if (!usbInterruptIsReady()) {
      led_on();
      return;      /* still sending interrupt packet */
   }
   hid_clear();

   if (repeat) {
      if (ir_accel < IR_ACCEL_MAX / 2)
         ir_accel += IR_ACCEL_INC_LO;
      else if (ir_accel < IR_ACCEL_MAX)
         ir_accel += IR_ACCEL_INC_HI;
   } else {
      ir_accel = IR_ACCEL_DEF;
   }

   for (k = 0; k < NUM_CODES; k++) {
      uchar action;
   
      if (ir_data != code_map[k].remote_code)
         continue;
      action = code_map[k].hid_code;

      if (action == 0xfa)
         inputBuffer2[3] = -ir_accel; /* up */
      else if (action == 0xfb)
         inputBuffer2[3] = ir_accel; /* down */
      else if (action == 0xfc)
         inputBuffer2[2] = -ir_accel; /* left */
      else if (action == 0xfd)
         inputBuffer2[2] = ir_accel; /* right */
      else if ((action & 0xf8) == 0xf0)
         inputBuffer2[1] |= 1 << (action & 7); /* click */
      else {
         inputBuffer1[1] = action;
         tx_kbd = 1;
         usbSetInterrupt(inputBuffer1, sizeof(inputBuffer1));   /* send kbd event */
         break;
      }
      tx_mouse = 1;
      usbSetInterrupt(inputBuffer2, sizeof(inputBuffer2));   /* send mouse event */
      break;
   }

   ir_timeout = IR_TIMEOUT;
   led_toggle();
}

static uchar ir_decode(uchar level, unsigned short time)
{
   /* as pointed out by other people as well, if () cascades are better
    * than switch () since they operate on chars instead of ints. */

   if (ir_state == IR_DISCARD) {
      /* counter overflowed */
      return ir_reset();
   } else if (ir_state == IR_IDLE) {
      if (level && ir_check(irconf.header_hi, time)) {
         ir_state = IR_HEADER;
      } else {
         return ir_reset();
      }
   } else if (ir_state == IR_HEADER) {
      if (!level && ir_check(irconf.repeat_lo, time)) {
         DBG1(0x99, (uchar *) &ir_data, 2);
         ir_input();
         return ir_reset();
      } else if (!level && ir_check(irconf.header_lo, time)) {
         ir_state = IR_DATA_HI;
         ir_data = 0;
      } else {
         return ir_reset();
      }
   } else if (ir_state == IR_DATA_HI) {
      if (level && ir_check(irconf.bit_hi, time)) {
         ir_state = IR_DATA_LO;
      } else {
         return ir_reset();
      }
   } else if (ir_state == IR_DATA_LO) {
      ir_bits++;
      ir_data <<= 1;
      if (!level && ir_check(irconf.one_lo, time)) {
         ir_data |= 1;
      } else if (!level && ir_check(irconf.zero_lo, time)) {
         /* nothing */
      } else {
         return ir_reset();
      }
      ir_state = IR_DATA_HI;
      if (ir_bits == 16) {
         if (ir_data != 0xc03f)
            return ir_reset();
         ir_data = 0;
      } else if (ir_bits == 32) {
         DBG1(0x88, (uchar *) &ir_data, 2);
         ir_input();
         ir_reset();
      }
   }
   return 1;
}

static void ir_queue_poll(void)
{
	/* if any pulses were received, decode them */
	while (ir_head != ir_tail) {
		unsigned short timing = ir_buf[ir_tail];

		ir_decode(ir_tail & 1, timing);
		ir_tail = (ir_tail + 1) % IR_BUFLEN;
	}
}

/* this may well be SIGNAL(SIG_OVERFLOW0), but we want it called at well-defined times */
static void idle_timer(void)
{		
	static uchar idleCounter = 0;

	/* idle reports, if requested */
	if (idleRate) {
		if (idleCounter > 4) {
			idleCounter -= 5;
		} else {
			idleCounter = idleRate;
			tx_kbd = 1;
			tx_mouse = 1;
		}
	}
			
	/* process button releases */
	if (ir_timeout) {
		if (!--ir_timeout) {
			hid_clear();
			tx_kbd = 1;
			tx_mouse = 1;
			led_on();
		}
	}
}

/* ------------------------------------------------------------------------- */

int main(void)
{
	wdt_enable(WDTO_1S);
	odDebugInit();
	
	/* all inputs, pull-ups active */
	DDRB = _BV(0);	/* LED */
	DDRC = 0;
	DDRD = 0;
	PORTB = 0xff;
	PORTC = 0xff;
	PORTD = 0xff;

	GICR |= _BV(INT1);	/* enable INT1 */
	MCUCR = (MCUCR & ~(_BV(ISC11) | _BV(ISC10))) | _BV(ISC11);	/* set INT1 on falling edge */
	TCCR0 = 5;		/* prescaler = 1/1024 -> overflow at 21.8 ms */
	TCCR1B = 3;		/* prescaler = 1/64 -> 1 count = 5.33333 us */

	usbReset();
	usbInit();
	sei();

	for (;;) {		/* main event loop */
		wdt_reset();
		usbPoll();
		ir_queue_poll();

		/* on timer overflow (~22ms) */
		if (TIFR & _BV(TOV0)) {
			TIFR = _BV(TOV0);
			idle_timer();
		}
		
		send_packets();
	}
	return 0;
}

/* ------------------------------------------------------------------------- */
