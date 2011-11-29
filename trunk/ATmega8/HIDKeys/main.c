/* Name: main.c
 * Project: HID-Test
 * Author: Christian Starkjohann
 * Creation Date: 2006-02-02
 * Tabsize: 4
 * Copyright: (c) 2006 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt) or proprietary (CommercialLicense.txt)
 * This Revision: $Id: main.c 299 2007-03-29 17:07:19Z cs $
 */

#define F_CPU	12000000L	/* evaluation board runs on 4MHz */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "oddebug.h"
#include "defines.h"

/* ----------------------- hardware I/O abstraction ------------------------ */

static uchar inputBuffer1[5] = { 1, 0,0,0,0, };		/* kbd */
static uchar inputBuffer2[4] = { 2, 0,0,0, };		/* mouse */
static uchar idleRate = 0;			/* in 4 ms units */
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


static struct code_entry 
{
	unsigned short remote_code;
	uchar hid_code;
}
code_map[] = 
{
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

static const struct 
{
	unsigned short header_hi, header_lo, repeat_lo, bit_hi, one_lo, zero_lo;
} 
irconf = 
{
/* original values (in microseconds)
	9062, 4442, 2193, 616, 1633, 508,
*/
/* values in 5.33us units: */
	1699, 833, 411, 115, 306, 95,
};

static void hardwareInit(void)
{
uchar	i, j;


	DDRC = 0;
	DDRD = 0;
	PORTB = 0xff;
	PORTC = 0xff;
	PORTD = 0xff;


	PORTB = 0xff;	/* activate all pull-ups */
	DDRB = 0;		/* all pins input */
	PORTC = 0xff;	/* activate all pull-ups */
	DDRC = 0;		/* all pins input */
	PORTD = 0xfa;	/* 1111 1010 bin: activate pull-ups except on USB lines */
	DDRD = 0x07;	/* 0000 0111 bin: all pins input except USB (-> USB reset) */
	j = 0;
	while(--j)
	{	 /* USB Reset by device only required on Watchdog Reset */
		i = 0;
		while(--i); /* delay >10ms for USB reset */
	}
	DDRD = 0x02;	/* 0000 0010 bin: remove USB reset condition */
	/* configure timer 0 for a rate of 12M/(1024 * 256) = 45.78 Hz (~22ms) */
	TCCR0 = 5;		/* timer 0 prescaler: 1024 */

	GICR |= _BV(INT1);   /* enable INT1 */
	MCUCR = (MCUCR & ~(_BV(ISC11) | _BV(ISC10))) | _BV(ISC11);   /* set INT1 on falling edge */
	TCCR0 = 5;      /* prescaler = 1/1024 -> overflow at 21.8 ms */
	TCCR1B = 3;      /* prescaler = 1/64 -> 1 count = 5.33333 us */
}

/* ------------------------------------------------------------------------- */

#define NUM_KEYS	17

/* The following function returns an index for the first key pressed. It
 * returns 0 if no key is pressed.
 */
static uchar keyPressed(void)
{
	uchar i, mask, x;

	x = PINB;
	mask = 1;
	for(i=0;i<6;i++)
	{
		if((x & mask) == 0)
			return i + 1;
		mask <<= 1;
	}
	x = PINC;
	mask = 1;
	for(i=0;i<6;i++)
	{
		if((x & mask) == 0)
			return i + 7;
		mask <<= 1;
	}
	x = PIND;
	mask = 1 << 3;
	for(i=0;i<5;i++)
	{
		if((x & mask) == 0)
			return i + 13;
		mask <<= 1;
	}
	return 0;
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uchar	reportBuffer[2];	/* buffer for HID reports */
static uchar	idleRate;			/* in 4 ms units */

PROGMEM char usbHidReportDescriptor[35] = { /* USB report descriptor */
	0x05, 0x01,					// USAGE_PAGE (Generic Desktop)
	0x09, 0x06,					// USAGE (Keyboard)
	0xa1, 0x01,					// COLLECTION (Application)
	0x05, 0x07,					//	USAGE_PAGE (Keyboard)
	0x19, 0xe0,					//	USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,					//	USAGE_MAXIMUM (Keyboard Right GUI)
	0x15, 0x00,					//	LOGICAL_MINIMUM (0)
	0x25, 0x01,					//	LOGICAL_MAXIMUM (1)
	0x75, 0x01,					//	REPORT_SIZE (1)
	0x95, 0x08,					//	REPORT_COUNT (8)
	0x81, 0x02,					//	INPUT (Data,Var,Abs)
	0x95, 0x01,					//	REPORT_COUNT (1)
	0x75, 0x08,					//	REPORT_SIZE (8)
	0x25, 0x65,					//	LOGICAL_MAXIMUM (101)
	0x19, 0x00,					//	USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0x65,					//	USAGE_MAXIMUM (Keyboard Application)
	0x81, 0x00,					//	INPUT (Data,Ary,Abs)
	0xc0							// END_COLLECTION
};
/* We use a simplifed keyboard report descriptor which does not support the
 * boot protocol. We don't allow setting status LEDs and we only allow one
 * simultaneous key press (except modifiers). We can therefore use short
 * 2 byte input reports.
 * The report descriptor has been created with usb.org's "HID Descriptor Tool"
 * which can be downloaded from http://www.usb.org/developers/hidpage/.
 * Redundant entries (such as LOGICAL_MINIMUM and USAGE_PAGE) have been omitted
 * for the second INPUT item.
 */

/* Keyboard usage values, see usb.org's HID-usage-tables document, chapter
 * 10 Keyboard/Keypad Page for more codes.
 */
#define MOD_CONTROL_LEFT		(1<<0)
#define MOD_SHIFT_LEFT			(1<<1)
#define MOD_ALT_LEFT			(1<<2)
#define MOD_GUI_LEFT			(1<<3)
#define MOD_CONTROL_RIGHT		(1<<4)
#define MOD_SHIFT_RIGHT			(1<<5)
#define MOD_ALT_RIGHT			(1<<6)
#define MOD_GUI_RIGHT			(1<<7)
#define MOD_CONTROL_ALT_LEFT	(1<<0)+(1<<2)

#define KEY_A		4
#define KEY_B		5
#define KEY_C		6
#define KEY_D		7
#define KEY_E		8
#define KEY_F		9
#define KEY_G		10
#define KEY_H		11
#define KEY_I		12
#define KEY_J		13
#define KEY_K		14
#define KEY_L		15
#define KEY_M		16
#define KEY_N		17
#define KEY_O		18
#define KEY_P		19
#define KEY_Q		20
#define KEY_R		21
#define KEY_S		22
#define KEY_T		23
#define KEY_U		24
#define KEY_V		25
#define KEY_W		26
#define KEY_X		27
#define KEY_Y		28
#define KEY_Z		29
#define KEY_1		30
#define KEY_2		31
#define KEY_3		32
#define KEY_4		33
#define KEY_5		34
#define KEY_6		35
#define KEY_7		36
#define KEY_8		37
#define KEY_9		38
#define KEY_0		39

#define KEY_F1		58
#define KEY_F2		59
#define KEY_F3		60
#define KEY_F4		61
#define KEY_F5		62
#define KEY_F6		63
#define KEY_F7		64
#define KEY_F8		65
#define KEY_F9		66
#define KEY_F10	 67
#define KEY_F11	 68
#define KEY_F12	 69

static const uchar  keyReport[NUM_KEYS + 1][2] PROGMEM = {
/* none */  {0, 0},						/* no key pressed */
/*  1 */	{MOD_SHIFT_LEFT, KEY_A},
/*  2 */	{MOD_SHIFT_LEFT, KEY_B},
/*  3 */	{MOD_SHIFT_LEFT, KEY_C},
/*  4 */	{MOD_SHIFT_LEFT, KEY_D},
/*  5 */	{MOD_SHIFT_LEFT, KEY_E},
/*  6 */	{MOD_SHIFT_LEFT, KEY_F},
/*  7 */	{MOD_SHIFT_LEFT, KEY_G},
/*  8 */	{MOD_SHIFT_LEFT, KEY_H},
/*  9 */	{MOD_SHIFT_LEFT, KEY_I},
/* 10 */	{MOD_SHIFT_LEFT, KEY_J},
/* 11 */	{MOD_SHIFT_LEFT, KEY_K},
/* 12 */	{MOD_SHIFT_LEFT, KEY_L},
///* 13 */	{MOD_SHIFT_LEFT, KEY_M},
/* 12 */	{MOD_CONTROL_ALT_LEFT, KEY_1},
/* 14 */	{MOD_SHIFT_LEFT, KEY_N},
/* 15 */	{MOD_SHIFT_LEFT, KEY_O},
/* 16 */	{MOD_SHIFT_LEFT, KEY_P},
/* 17 */	{MOD_SHIFT_LEFT, KEY_Q},
};

static void buildReport(uchar key)
{
/* This (not so elegant) cast saves us 10 bytes of program memory */
	*(int *)reportBuffer = pgm_read_word(keyReport[key]);
}

uchar	usbFunctionSetup(uchar data[8])
{
	usbRequest_t	*rq = (void *)data;

	usbMsgPtr = reportBuffer;
	if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS)
	{	
		/* class request type */
		if(rq->bRequest == USBRQ_HID_GET_REPORT)
		{  
			/* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* we only have one report type, so don't look at wValue */
			buildReport(keyPressed());
			return sizeof(reportBuffer);
		}
		else if(rq->bRequest == USBRQ_HID_GET_IDLE)
		{
			usbMsgPtr = &idleRate;
			return 1;
		}
		else if(rq->bRequest == USBRQ_HID_SET_IDLE)
		{
			idleRate = rq->wValue.bytes[1];
		}
	}
	else
	{
		/* no vendor specific requests implemented */
	}
	return 0;
}


SIGNAL(SIG_INTERRUPT1)
{
	ir_buf[ir_head] = TCNT1;				/* store pulse length */
	ir_head = (ir_head + 1) % IR_BUFLEN;

	MCUCR ^= _BV(ISC10);					/* toggle rising/falling edge */
	TCNT1 = 0;								/* clear counter */
}


/* ------------------------------------------------------------------------- */

static void hid_clear(void)
{
	inputBuffer2[1] = 0;	/* mouse buttons */
	inputBuffer2[2] = 0;	/* X axis delta */
	inputBuffer2[3] = 0;	/* Y axis delta */

	inputBuffer1[1] = 0;	/* key */
}

static uchar ir_check(const unsigned short ref, unsigned short value)
{
	short delta = ABS((short) value - (short) ref);

	if (delta > 100)		/* aeps = 100us */
		return 0;
	if (delta > ref / 3)		/* eps = 30%*/
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

	if (ir_data == 0xa857)	/* "scan" button */
		reset();	/* XXX HACK */

	if (!usbInterruptIsReady()) 
	{
		led_on();
		return;		/* still sending interrupt packet */
	}
	hid_clear();

	if (repeat) 
	{
		if (ir_accel < IR_ACCEL_MAX / 2)
			ir_accel += IR_ACCEL_INC_LO;
		else if (ir_accel < IR_ACCEL_MAX)
			ir_accel += IR_ACCEL_INC_HI;
	} 
	else 
	{
		ir_accel = IR_ACCEL_DEF;
	}

	for (k = 0; k < NUM_CODES; k++) 
	{
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
		else 
		{
			inputBuffer1[1] = action;
			tx_kbd = 1;
			usbSetInterrupt(inputBuffer1, sizeof(inputBuffer1));	/* send kbd event */
			break;
		}
		tx_mouse = 1;
		usbSetInterrupt(inputBuffer2, sizeof(inputBuffer2));	/* send mouse event */
		break;
	}

	ir_timeout = IR_TIMEOUT;
	led_toggle();
}

static uchar ir_decode(uchar level, unsigned short time)
{
	/* as pointed out by other people as well, if () cascades are better
	* than switch () since they operate on chars instead of ints. */

	if (ir_state == IR_DISCARD) 
	{
		/* counter overflowed */
		return ir_reset();
	} 
	else if (ir_state == IR_IDLE) 
	{
		if (level && ir_check(irconf.header_hi, time)) 
		{
			ir_state = IR_HEADER;
		} 
		else 
		{
			return ir_reset();
		}
	} 
	else if (ir_state == IR_HEADER) 
	{
		if (!level && ir_check(irconf.repeat_lo, time)) 
		{
			DBG1(0x99, (uchar *) &ir_data, 2);
			ir_input();
			return ir_reset();
		} 
		else if (!level && ir_check(irconf.header_lo, time)) 
		{
			ir_state = IR_DATA_HI;
			ir_data = 0;
		} 
		else 
		{
			return ir_reset();
		}
	} 
	else if (ir_state == IR_DATA_HI) 
	{
		if (level && ir_check(irconf.bit_hi, time)) 
		{
			ir_state = IR_DATA_LO;
		} 
		else 
		{
			return ir_reset();
		}
	} 
	else if (ir_state == IR_DATA_LO) 
	{
		ir_bits++;
		ir_data <<= 1;
		if (!level && ir_check(irconf.one_lo, time)) 
		{
			ir_data |= 1;
		} 
		else if (!level && ir_check(irconf.zero_lo, time)) 
		{
			/* nothing */
		} else 
		{
			return ir_reset();
		}
		ir_state = IR_DATA_HI;
		if (ir_bits == 16) 
		{
			if (ir_data != 0xc03f)
				return ir_reset();
			ir_data = 0;
		} 
		else if (ir_bits == 32) 
		{
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
	if (idleRate) 
	{
		if (idleCounter > 4) 
		{
			idleCounter -= 5;
		}
		else 
		{
			idleCounter = idleRate;
			tx_kbd = 1;
			tx_mouse = 1;
		}
	}
			
	/* process button releases */
	if (ir_timeout)
	{
		if (!--ir_timeout)
		{
			hid_clear();
			tx_kbd = 1;
			tx_mouse = 1;
			led_on();
		}
	}
}

/* ------------------------------------------------------------------------- */

int	main(void)
{
	uchar	key, lastKey = 0, keyDidChange = 0;
	uchar	idleCounter = 0;

	wdt_enable(WDTO_2S);
	hardwareInit();
	odDebugInit();
	usbInit();
	sei();
	DBG1(0x00, 0, 0);
	for(;;)		/* main event loop */
	{
		wdt_reset();
		usbPoll();
		key = keyPressed();
		if(lastKey != key)
		{
			lastKey = key;
			keyDidChange = 1;
		}
		if(TIFR & (1<<TOV0))		/* 22 ms timer */
		{
			TIFR = 1<<TOV0;
			if(idleRate != 0)
			{
				if(idleCounter > 4)
				{
					idleCounter -= 5;	/* 22 ms in units of 4 ms */
				}
				else
				{
					idleCounter = idleRate;
					keyDidChange = 1;
				}
			}
		}
		if(keyDidChange && usbInterruptIsReady())
		{
			keyDidChange = 0;
			/* use last key and not current key status in order to avoid lost
			changes in key status. */
			buildReport(lastKey);
			usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
		}
	}
	return 0;
}

/* ------------------------------------------------------------------------- */
