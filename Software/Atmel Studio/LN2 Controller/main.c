/*
 * LN2 Controller
 *
 * Created: 1/30/2020 9:52:11 AM
 * Author : Alan Uomoto
 *
 */

// DEFINES
#define	F_CPU			3333333UL		// Out-of-the-box Curiosity Nano

#define VERSION			"2020-03-16"	// ISO date, 10 characters exactly
#define TRUE			1
#define FALSE			0

// On-board LED is on pin PF5
#define ONLED			(PORTF.OUTCLR = PIN5_bm)
#define OFFLED			(PORTF.OUTSET = PIN5_bm)
#define TOGGLELED		(PORTF.OUTTGL = PIN5_bm)

// Valve defines
#define OPENVALVE(VALVEPIN)		(PORTC.OUTSET = VALVEPIN)
#define CLOSEVALVE(VALVEPIN)	(PORTC.OUTCLR = VALVEPIN)
#define BLUVALVE		(PIN7_bm)	// These are the "VALVEPIN" values
#define REDVALVE		(PIN6_bm)
#define BUFVALVE		(PIN5_bm)
#define SUPVALVE		(PIN4_bm)
#define BLUVALVEOPEN	(PORTC.IN & PIN7_bm)	// TRUE if the valve is open
#define REDVALVEOPEN	(PORTC.IN & PIN6_bm)
#define BUFVALVEOPEN	(PORTC.IN & PIN5_bm)
#define SUPVALVEOPEN	(PORTC.IN & PIN4_bm)
#define BUFMAXOPEN		10			// In minutes

// Oscillator signal for debouncer on PC0 (temporary)
#define TCB2FREQ		1500		// Debouncer PWM frequency in Hz

// Thermistor defines
#define BLUTHERMWARM	(PORTE.IN & PIN0_bm)	// TRUE if the thermistor is warm
#define REDTHERMWARM	(PORTE.IN & PIN1_bm)
#define BUFTHERMWARM	(PORTE.IN & PIN2_bm)

// USART baud rate macro
#define	USART_BAUD_RATE(BAUD_RATE)	((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

// TWI defines
#define TWIFREQ			100000UL	// TWI bus speed
#define TWIBAUD			((uint8_t) (F_CPU/(2*TWIFREQ)) - 5)	// Ignore rise time
#define TWIREAD			1
#define TWIWRITE		0

// Valve pushbutton defines
#define BLUEBUTTON		1
#define REDBUTTON		2
#define BUFFERBUTTON	3
#define SUPPLYBUTTON	4

// Encoder defines
#define ENCODERBUTTON	5
#define NANOBUTTON		6		// lump this in here
#define ENCODERA		7

// Pressure sensor defines (Panasonic ADP5151)
#define PSSLOPE			(0.8505)
#define PSINTERCEPT		(-24.69)
#define PSFREQ			(250)		// 250 results in 1/2 sec updates
//#define PSFREQ			(500)		// 500 results in 1/4 sec updates

// OLED display
#define CLEARDISPLAY	0x01		// Newhaven command (not used)
#define DISPLAYON		0x0C		// Newhaven command (not used)
#define DISPLAYOFF		0x08		// Newhaven command (not used)
#define OLEDADDR		(0x3c << 1)	// TWI bus address
#define OLEDCMD			1			// Newhaven command
#define OLEDDATA		0			// Newhaven command
#define OLEDLINE1		0x80		// Newhaven command
#define OLEDLINE2		0xC0		// Newhaven command

// OLED display types
#define DARKDISPLAY		0		// Nothing on the screen
#define VALVESTATE		1		// Solenoid valve status
#define NEXTFILL		2		// Time to next fill
#define FILLINTDISPLAY	3		// Fill interval
#define MAXOPENDISPLAY	4		// Max open duration
#define PRESSUREDISPLAY	5		// LN2 pressure
#define DISBLUDISPLAY	6		// Enable/disable blue vent
#define DISREDDISPLAY	7		// Enable/disable red vent
#define TITLEDISPLAY	8		// Program name and version
#define DISPLAY9	9
#define DISPLAY10		10
#define DISPLAY11		11
#define DISPLAY12		12
#define DISPLAY13		13
#define DISPLAY14		14
#define DISPLAY15		15
#define MAXDISPLAYS		16		// Must be power of 2 for proper CCW encoder action

// Menu defines
#define CHANGEFILL		1		// Changing fill time menu
#define CHANGEMAX		2		// Changing max open time menu
#define CHANGEDISBLU	3		// Change disable blue valve
#define CHANGEDISRED	4		// Change disable red valve

// EEPROM defines
#define VERSIONADDR		(0)		// Version string must have exactly 10 bytes
#define FILLINTADDR		(10)
#define MAXOPENADDR		(11)
#define BLUENABLEADDR	(12)
#define REDENABLEADDR	(13)
#define BLUENABLED		(eeprom_read_byte((uint8_t *)BLUENABLEADDR))
#define REDENABLED		(eeprom_read_byte((uint8_t *)REDENABLEADDR))
#define FILLINTERVAL	(eeprom_read_byte((uint8_t *)FILLINTADDR))
#define MAXOPENTIME		(eeprom_read_byte((uint8_t *)MAXOPENADDR))

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>

// Function prototypes
//void clear_OLED(void);
void handle_buttons(void);
void handle_encoder(void);
void handle_pressure(void);
void handle_serial(void);
void initialize(void);
void init_ADC(void);
void init_ENC(void);
void init_LED(void);
void init_OLED(void);
void init_Ports(void);
void init_RTC(void);
void init_Serial(void);
void init_Status(void);
void init_Switches(void);
void init_TCB0(void);
void init_TCB2(void);
void init_Thermistors(void);
void init_TWI(void);
void init_Valves(void);
void print_Status(void);
void read_TWI(uint8_t *, uint8_t);
void send_serial(void);
void start_TWI(uint8_t, uint8_t);
void stop_TWI(void);
void update_display(uint8_t);
void write_OLED(uint8_t, uint8_t);
void write_OLED_string(char *, uint8_t);
void write_TWI(uint8_t *, uint8_t);

struct serial_data {
	uint8_t data[100],		// Data to send or data received
		nBytes;				// Number of data bytes in data[];
	volatile uint8_t done,	// Is the transfer complete?
		nXfrd;				// Temporary counter (number of bytes transferred)
};

struct statusbuf {
	char version[11];					// Version (date string)
	int8_t next_fill;					// Minutes until next fill start
	uint8_t opentime_BLU, opentime_RED,	// Minutes that valve has been open
		opentime_BUF, opentime_SUP,		// Minutes that valve has been open
		maxopen_BLU, maxopen_RED,		// TRUE if MAXOPENTIME is exceeded
		maxopen_BUF,					// BUFMAXOPEN happened
		pressure;						// LN2 pressure in tenths of psi
};

// Globals
volatile uint8_t encoder_sensed,		// Encoder caused interrupt
	encoder_value,						// Running encoder value
	button_sensed,						// One of the four pushbuttons
	valve_changed,						// A valve status changed
	display_type,						// What's showing on the screen
	in_menu,							// Are we waiting for encoder input?
	read_pressure;						// Timer interrupt triggers reading pressure

struct serial_data send_buf, recv_buf;

struct statusbuf status;

int main(void)
{

	initialize();
	update_display(TITLEDISPLAY);
	_delay_ms(2000);
	update_display(DARKDISPLAY);
	sei();
	for (;;) {

		_delay_ms(3);

		if (read_pressure) {
			handle_pressure();
			if (!in_menu) {
				update_display(encoder_value % MAXDISPLAYS);
			}
		}

		if (recv_buf.done) {
			handle_serial();
			if (!in_menu) {
				update_display(encoder_value % MAXDISPLAYS);
			}

		}

		if (button_sensed) {
			handle_buttons();
			if (!in_menu) {
				update_display(encoder_value % MAXDISPLAYS);
			}
		}

		if (valve_changed) {
			if (!in_menu) {
				update_display(encoder_value % MAXDISPLAYS);
			}
		}

		if (encoder_sensed) {
			handle_encoder();
			if (!in_menu) {
				update_display(encoder_value % MAXDISPLAYS);
			}
		}

		encoder_sensed = FALSE;
		button_sensed = FALSE;
		valve_changed = FALSE;
		read_pressure = FALSE;

	}
}

void initialize(void)
{

	// Set all ports to inputs with pullups enabled
	init_Ports();

	// Output ports
	init_LED();
	init_Valves();

	// Input ports
	init_TCB2();
	init_Switches();
	init_ENC();
	init_Thermistors();
	init_TCB0();
	init_ADC();

	// I/O peripherals
	init_Serial();
	init_TWI();
	init_OLED();

	// One minute interrupts
	init_RTC();

	// Initialize status structure
	init_Status();

}

/*----------------------------------------------------------------------
Set all I/O ports to input with pullups enabled except ADC ports that
have input disabled. This is just to make sure nothing lurks in the
background by accident.
----------------------------------------------------------------------*/
void init_Ports(void)
{

	PORTA.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN3CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN7CTRL = PORT_PULLUPEN_bm;

	PORTB.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN3CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN4CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN5CTRL = PORT_PULLUPEN_bm;

	PORTC.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN3CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN4CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN5CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN6CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN7CTRL = PORT_PULLUPEN_bm;

	// ADC pins are on port D
	PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;

	PORTE.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTE.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTE.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTE.PIN3CTRL = PORT_PULLUPEN_bm;

	PORTF.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN3CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN4CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN5CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN6CTRL = PORT_PULLUPEN_bm;

}

/*----------------------------------------------------------------------
INITIALIZE LED

The on-board LED is useful for development diagnostics but isn't used
for anything else. The LED is on pin PF5 and is turned on when that pin
is pulled low.
----------------------------------------------------------------------*/
void init_LED(void)
{

	PORTF.OUTSET = PIN5_bm;		// Set HIGH (LED off)
	PORTF.DIRSET = PIN5_bm;		// Set as Output

}

/*----------------------------------------------------------------------
INITIALIZE VALVES

These are the pins that control the solenoid valves. All are set to
low and output. These pins drive MOSFETs that control the LN2 solenoid
valves.

VALVn is the electronics schematic net name.

VALV1 is on pin PC7 (Blue dewar vent)
VALV2 is on pin PC6 (Red dewar vent)
VALV3 is on pin PC5 (Buffer dewar vent)
VALV4 is on pin PC4 (Supply input valve)
----------------------------------------------------------------------*/
void init_Valves(void)
{

	// Set valve control pins as outputs, turned off initially.
	PORTC.OUTCLR = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;
	PORTC.DIRSET = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;

}

/*----------------------------------------------------------------------
INITIALIZE TIMER-COUNTER TCB2

At startup, TCB is in periodic interrupt mode. To enable this:

1. Write a TOP value to TCBn.CCMP
2a. Enable the counter by writing a '1' to the ENABLE bit in TCBn.CTRLA
2b. Set the CLKSEL prescaler bit field in TCBn.CTRLA
3. A CAPT interrupt is generated when TOP is reached. Enable CAPT
   interrupts in the TCB2.INTCTRL register.

This counter generates an oscillator signal for the ON Semiconductor
MC14490DWG switch debouncer. The original setup had the oscillator
running too slowly for the rotary encoder so I set this up as a stopgap
until I could get the correct capacitor.

So nTicks = F_CPU/(2*F_osc) so if you want
F_osc = 1500 Hz on a 3333333 MHz system clock with 2X prescaler you get
nTicks = 3333333/(2*prescaler*1500)+1 = 0x022C (the +1 takes care of
the integer arithmetic that will truncate most of the time)

By measurement, 0xFFFF gets 12.69 Hz
----------------------------------------------------------------------*/
void init_TCB2(void)
{

	PORTC_DIRSET = PIN0_bm;					// PIN PC0 has haywire
	PORTC_OUTSET = PIN0_bm;					// to debouncer Osc-in
	TCB2.CCMP = F_CPU/(4 * TCB2FREQ) + 1;
	TCB2.CTRLA |= TCB_ENABLE_bm;			// Enable the timer
	TCB2.CTRLA |= TCB_CLKSEL_CLKDIV2_gc;	// 2X divider
	TCB2.INTCTRL |= TCB_CAPT_bm;			// Enable interrupts
	
}

/*----------------------------------------------------------------------
Generate a square wave by toggling PC0
----------------------------------------------------------------------*/
ISR(TCB2_INT_vect)
{

	TCB2.INTFLAGS = TCB_CAPT_bm;
	PORTC.OUTTGL = PIN0_bm;

}

/*----------------------------------------------------------------------
INITIALIZE SWITCHES and PUSHBUTTONS

Four pushbuttons with integrated LED indicators control the solenoid
valves. These switches go through a debounce conditioner IC.

The on-board pushbutton switch on the Curiosity Nano has an external
pullup on it but no debouncing.

PB2 is SW1 Blue dewar vent
PB3 is SW2 Red dewar vent
PB0 is SW3 Buffer dewar vent
PB1 is SW4 Buffer dewar supply inlet
PF6 is the Curiosity Nano on-board pushbutton
----------------------------------------------------------------------*/
void init_Switches()
{

	// Curiosity Nano on-board pushbutton for testing
	PORTF.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;

	// Four solenoid valve control switches
	PORTB.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW1
	PORTB.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW2
	PORTB.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW3
	PORTB.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW4

}

/*----------------------------------------------------------------------
Interrupt routine for the four ASCO valve pushbuttons. The interrupt
flag is cleared and the button_sensed value is set.
----------------------------------------------------------------------*/
ISR(PORTB_PORT_vect)
{

	if (PORTB.INTFLAGS & PIN2_bm) {		// Blue pushbutton
		PORTB.INTFLAGS = PIN2_bm;		// Clear the interrupt flag
		button_sensed = BLUEBUTTON;
	}
	if (PORTB.INTFLAGS & PIN3_bm) {		// Red pushbutton
		PORTB.INTFLAGS = PIN3_bm;		// Clear the interrupt flag
		button_sensed = REDBUTTON;
	}
	if (PORTB.INTFLAGS & PIN0_bm) {		// Buffer pushbutton
		PORTB.INTFLAGS = PIN0_bm;
		button_sensed = BUFFERBUTTON;
	}
	if (PORTB.INTFLAGS & PIN1_bm) {		// Supply pushbutton
		PORTB.INTFLAGS = PIN1_bm;		// Clear the interrupt flag
		button_sensed = SUPPLYBUTTON;
	}

}

/*----------------------------------------------------------------------
Interrupt routine Curiosity Nano on-board pushbutton
----------------------------------------------------------------------*/
ISR(PORTF_PORT_vect)
{

	if (PORTF.INTFLAGS & PIN6_bm) {		// Curiosity Nano button
		PORTF.INTFLAGS = PIN6_bm;		// Clear the interrupt flag
		button_sensed = NANOBUTTON;
	}

}

/*----------------------------------------------------------------------
What to do after a valve pushbutton is sensed.
----------------------------------------------------------------------*/
void handle_buttons(void)
{

//	_delay_ms(4);

	switch (button_sensed) {

		case BLUEBUTTON:
			if (BLUVALVEOPEN) {
				CLOSEVALVE(BLUVALVE);
			} else if (BLUENABLED) {
				OPENVALVE(BLUVALVE);
			}
			status.opentime_BLU = 0;
			status.maxopen_BLU = FALSE;
			status.next_fill = FILLINTERVAL;
			break;

		case REDBUTTON:
			if (REDVALVEOPEN) {
				CLOSEVALVE(REDVALVE);
			} else if (REDENABLED) {
				OPENVALVE(REDVALVE);
			}
			status.opentime_RED = 0;
			status.maxopen_RED = FALSE;
			break;

		case BUFFERBUTTON:
			if (BUFVALVEOPEN) {
				CLOSEVALVE(BUFVALVE);
			} else {
				OPENVALVE(BUFVALVE);
			}
			status.opentime_BUF = 0;
			status.maxopen_BUF = FALSE;
			break;

		case SUPPLYBUTTON:
			if (SUPVALVEOPEN) {
				CLOSEVALVE(SUPVALVE);
			} else {
				OPENVALVE(SUPVALVE);
			}
			status.opentime_SUP = 0;
			break;

		default:
			break;
	}
	button_sensed = FALSE;
}

/*----------------------------------------------------------------------
INITIALIZE ENCODER

The rotary encoder channels A and B go through the debouncer IC.
The pushbutton on the rotary encoder has a pullup and an RC debouncer.

PC3 is ENCA encoder A signal
PC2 is ENCB encoder B signal (does not cause an interrupt)
PC1 is PUSH encoder pushbutton

WAIT: TRIGGER ON RISING VALUES INSTEAD? AFTER 4 MS THIS WILL SETTLE
INTO A DETENT. TRIGGERING ON FALLING VALUES IS BETWEEN DETENTS
----------------------------------------------------------------------*/
void init_ENC(void)
{
	
	// Rotary Encoder
	PORTC.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// ENCA
	PORTC.PIN2CTRL = PORT_PULLUPEN_bm;							// ENCB
	PORTC.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// PUSH

}

/*----------------------------------------------------------------------
Interrupt routine for encoder rotation or pushbutton
----------------------------------------------------------------------*/
ISR(PORTC_PORT_vect)
{

	// Encoder pushbutton
	if (PORTC.INTFLAGS & PIN1_bm) {
		PORTC.INTFLAGS = PIN1_bm;		// Clear the interrupt flag
		encoder_sensed = ENCODERBUTTON;
	}

	// Encoder A pin
	if (PORTC.INTFLAGS & PIN3_bm) {
		PORTC.INTFLAGS = PIN3_bm;		// Clear the interrupt flag
		if (PORTC.IN & PIN2_bm) {
			encoder_value++;
		} else {
			encoder_value--;
		}
		encoder_sensed = ENCODERA;
	}

}

/*----------------------------------------------------------------------
What to do after encoder action
----------------------------------------------------------------------*/
void handle_encoder(void)
{

	char strbuf[20];

	// Knob is turned
	if (encoder_sensed == ENCODERA) {
		if (!in_menu) {
			update_display(encoder_value % MAXDISPLAYS);
		} else if (in_menu == CHANGEFILL) {
			write_OLED_string("Push to set", 1);
			if (encoder_value > 60) {
				encoder_value = 60;
			}
			if (encoder_value < 2) {
				encoder_value = 2;
			}
			if (encoder_value < 2) {
				sprintf(strbuf, "%d minute", encoder_value);
			} else {
				sprintf(strbuf, "%d minutes", encoder_value);
			}
			write_OLED_string(strbuf, 2);
		} else if (in_menu == CHANGEMAX) {
			write_OLED_string("Push to set",1);
			if (encoder_value > FILLINTERVAL/2) {
				encoder_value = FILLINTERVAL/2;
			}
			if (encoder_value < 1) {
				encoder_value = 1;
			}
			if (encoder_value < 2) {
				sprintf(strbuf, "%d minute", encoder_value);
			} else {
				sprintf(strbuf, "%d minutes", encoder_value);
			}
			write_OLED_string(strbuf, 2);
		}
	} else if (encoder_sensed == ENCODERBUTTON) {
		if (!in_menu) {
			if ((encoder_value % MAXDISPLAYS) == FILLINTDISPLAY) {
				in_menu = CHANGEFILL;
				encoder_value = FILLINTERVAL;
				write_OLED_string("Adjust interval", 1);
			} else if ((encoder_value % MAXDISPLAYS) == MAXOPENDISPLAY) {
				in_menu = CHANGEMAX;
				encoder_value = MAXOPENTIME;
				write_OLED_string("Adjust max open", 1);
			} else if ((encoder_value % MAXDISPLAYS) == DISBLUDISPLAY) {
				if (BLUENABLED) {
					CLOSEVALVE(BLUVALVE);		// Immediately close valve
					status.opentime_BLU = 0;
					status.maxopen_BLU = FALSE;
					eeprom_update_byte((uint8_t *)BLUENABLEADDR, FALSE);
				} else {
					eeprom_update_byte((uint8_t *)BLUENABLEADDR, TRUE);
				}
				update_display(DISBLUDISPLAY);
			} else if ((encoder_value % MAXDISPLAYS) == DISREDDISPLAY) {
				if (REDENABLED) {
					CLOSEVALVE(REDVALVE);		// Immediately close valve
					status.opentime_RED = 0;
					status.maxopen_RED = FALSE;
					eeprom_update_byte((uint8_t *)REDENABLEADDR, FALSE);
				} else {
					eeprom_update_byte((uint8_t *)REDENABLEADDR, TRUE);
				}
				update_display(DISREDDISPLAY);
			}
		} else {
			if (in_menu == CHANGEFILL) {
				eeprom_update_byte((uint8_t *)FILLINTADDR, encoder_value);
				encoder_value = FILLINTDISPLAY;
				if (MAXOPENTIME > FILLINTERVAL/2) {
					eeprom_update_byte((uint8_t *)MAXOPENADDR, FILLINTERVAL/2);
				}
				update_display(encoder_value);
				if (FILLINTERVAL < status.next_fill) {
					status.next_fill = FILLINTERVAL;
				}
				in_menu = FALSE;
			} else if (in_menu == CHANGEMAX) {
				eeprom_update_byte((uint8_t *)MAXOPENADDR, encoder_value);
				encoder_value = MAXOPENDISPLAY;
				update_display(encoder_value);
				in_menu = FALSE;
			}
		}
	}
	encoder_sensed = FALSE;

}

/*---------------------------------------------------------------------
INITIALIZE THERMISTORS

Thermistor signals are digital, generated by a comparator. Signals are
on PORTE, pins 0, 1, 2, and 3, but thermistors are connected only to
pins 0, 1, and 2 (the original SDSS system had a thermistor at the
bottom of the buffer dewar to signal that it was empty but our system
doesn't). We initialize the unused pin as an input just to avoid
accidentally turning it on.

All ports wills have internal pullups enabled but external pullups are
installed as well. Three pins (0, 1, and 2) have interrupts enabled.

PINn_bm assignments:
Thermistor 1 is on pin PE0 (Blue vent thermistor)
Thermistor 2 is on pin PE1 (Red vent thermistor)
Thermistor 3 is on pin PE2 (Buffer vent thermistor)
Thermistor 4 is on pin PE3 (No thermistor assigned)
----------------------------------------------------------------------*/
void init_Thermistors(void)
{

	// Set 4 pins as inputs with pullups, 3 pins have interrupts enabled.
	PORTE.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
	PORTE.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
	PORTE.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;
	PORTE.PIN3CTRL = PORT_PULLUPEN_bm;	// No thermistor here

}

/*----------------------------------------------------------------------
Interrupt routine for thermistor changes.

BLU PIN0_bm
RED PIN1_bm
BUF PIN2_bm

In all cases, close the corresponding valve. The valve on a thermistor
that is warming up is probably closed already.
----------------------------------------------------------------------*/
ISR(PORTE_PORT_vect)
{

	valve_changed = TRUE;				// Forces new OLED display

	if (PORTE.INTFLAGS & PIN0_bm) {		// Blue dewar thermistor changed state
		PORTE.INTFLAGS |= PIN0_bm;		// Clear the interrupt flag
		CLOSEVALVE(BLUVALVE);			// Close the valve whether open or not
		status.opentime_BLU = 0;
		if (!BLUTHERMWARM) {			// Thermistor senses cold
			status.maxopen_BLU = FALSE;	// Clear a MAXOPEN flag
			if (REDENABLED && REDTHERMWARM) {	// Start red fill after blue finishes
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
				status.maxopen_RED = FALSE;
			}
		}
	}

	if (PORTE.INTFLAGS & PIN1_bm) {		// Red dewar thermistor changed state
		PORTE.INTFLAGS |= PIN1_bm;		// Clear the interrupt flag
		CLOSEVALVE(REDVALVE);
		status.opentime_RED = 0;
		if (!REDTHERMWARM) {			// Thermistor senses cold
			status.maxopen_RED = FALSE;	// Clear a MAXOPEN limit
		}
	}

	if (PORTE.INTFLAGS & PIN2_bm) {		// Buffer dewar thermistor changed state
		PORTE.INTFLAGS |= PIN2_bm;
		CLOSEVALVE(BUFVALVE);
		status.opentime_BUF = 0;
		if (!BUFTHERMWARM) {			// Thermistor senses cold
			status.maxopen_BUF = FALSE;	// Clear a MAXOPEN limit
		}
		CLOSEVALVE(SUPVALVE);
		status.opentime_SUP = 0;
	}
}

/*----------------------------------------------------------------------
ADC INITIALIZATION

From page 398

 1. Configure the resolution in ADCn.CTRLA (p406; default is 10 bits)
 2. Optionally enable free-running mode (default is not)
 3. Optionally configure the # samples per converstion ADCn.CTRLB
 4. Configure voltage reference REFSEL in ADCn.CTRLC.
 5. Configure CLK_ADC by writing to prescaler PRESC in ADCn.CTRLC
 6. Configure an input by writing to the MUXPOS bit field ADCn.MUXPOS
 7. Optional: Enable start event input in event control
 8. Enable the ADC by writing 1 to ENABLE bit in ADCn.CTRLA
 
 Trigger a conversion by writing 1 to STCONV in ADCn.COMMAND
----------------------------------------------------------------------*/
void init_ADC()
{

	PORTD_PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;		// Disable digital input
	ADC0_CTRLA |= ADC_RESSEL_8BIT_gc;		// 8-bit resolution
//	ADC0_CTRLA |= ADC_RESSEL_10BIT_gc;		// 10-bit resolution
	VREF.CTRLA |= VREF_ADC0REFSEL_4V34_gc;	// Use 4.3V
//	VREF.CTRLA |= VREF_ADC0REFSEL_2V5_gc;	// Use 2.5V
	ADC0_CTRLC |= ADC_REFSEL_INTREF_gc;		// Use internal vref
	ADC0_CTRLC |= ADC_SAMPCAP_bm;			// Reduce sampling capacitance
	ADC0_MUXPOS = ADC_MUXPOS_AIN0_gc;		// PD0 pin feeds ADC
	ADC0.CTRLA |= ADC_ENABLE_bm;			// Enable ADC
	ADC0_COMMAND |= ADC_STCONV_bm;			// Start ADC conversion
	while (ADC0_COMMAND & ADC_STCONV_bm) {
		asm("nop");
	}
	status.pressure = ADC0_RES;				// Throw first one away
	read_pressure = FALSE;

}

/*----------------------------------------------------------------------
Read the pressure sensor and convert the output to kPa
----------------------------------------------------------------------*/
void handle_pressure(void)
{

	uint8_t i;
	float value, nsamp;

	value = 0.0;
	nsamp = 10.0;
	for (i = 0; i < nsamp; i++) {
		ADC0_COMMAND |= ADC_STCONV_bm;
		while (ADC0_COMMAND & ADC_STCONV_bm) {
			asm("nop");
		}
		value += (float) ADC0_RESL;
	}
	value = PSSLOPE * (value/nsamp) + PSINTERCEPT;
	status.pressure = (uint8_t)value;
	read_pressure = FALSE;

}

/*----------------------------------------------------------------------
Timer interrupt for periodic ADC reads from the pressure sensor
----------------------------------------------------------------------*/
ISR(TCB0_INT_vect)
{

	static uint8_t count = 0;

	TCB0.INTFLAGS = TCB_CAPT_bm;
	count++;
	if (count > 250) {
		read_pressure = TRUE;
		count = 0;
	}

}


/*----------------------------------------------------------------------
INITIALIZE TIMER-COUNTER TCB0

At startup, TCB is in periodic interrupt mode. To enable this:

1. Write a TOP value to TCBn.CCMP
2a. Enable the counter by writing a '1' to the ENABLE bit in TCBn.CTRLA
2b. Set the CLKSEL prescaler bit field in TCBn.CTRLA
3. A CAPT interrupt is generated when TOP is reached. Enable CAPT
   interrupts in the TCB0.INTCTRL register.

So nTicks = F_CPU/(2*F_osc) so if you want
F_osc = 1500 Hz on a 3333333 MHz system clock with 2X prescaler you get
nTicks = 3333333/(2*prescaler*1500)+1 = 0x022C (the +1 takes care of
the integer arithmetic that will truncate most of the time)

By measurement, 0xFFFF gets 12.69 Hz
----------------------------------------------------------------------*/
void init_TCB0(void)
{

	TCB0.CCMP = F_CPU/(4 * PSFREQ) + 1;	
	TCB0.CTRLA |= TCB_ENABLE_bm;			// Enable the timer
	TCB0.CTRLA |= TCB_CLKSEL_CLKDIV2_gc;	// 2X divider
	TCB0.INTCTRL |= TCB_CAPT_bm;			// Enable interrupts
	
}

/*----------------------------------------------------------------------
SERIAL PORT INITIALIZATION

Set USART0 for 9600 baud, 8N1.
----------------------------------------------------------------------*/
void init_Serial(void)
{

	// USART0 PA0 is TxD, PA1 is RxD, Default pin position
	PORTA.OUTSET = PIN0_bm;
	PORTA.DIRSET = PIN0_bm;
	USART0.BAUD = (uint16_t) USART_BAUD_RATE(9600);
	USART0.CTRLA |= USART_RXCIE_bm;
	USART0.CTRLB |= USART_TXEN_bm;
	USART0.CTRLB |= USART_RXEN_bm;

	send_buf.done = TRUE;
	recv_buf.done = FALSE;

}

/*----------------------------------------------------------------------
Send serial data on USART0

To do this, fill send_buf.data, set send_buf.nBytes, then call this.
----------------------------------------------------------------------*/
void send_serial(void)
{

	while (!send_buf.done) {
		asm("nop");
	}
	send_buf.nXfrd = 0;
	send_buf.done = FALSE;
	USART0.CTRLA |= USART_DREIE_bm;

}

/*---------------------------------------------------------------------
Interrupt routine for incoming serial data on USART0

Receives data, filling recv_buf.data until a carriage return '\r' is
seen, terminating the string.

Polling for recv_buf.done == TRUE indicates a string is available.
---------------------------------------------------------------------*/
ISR(USART0_RXC_vect)
{

	uint8_t c;

	c = USART0.RXDATAL;
	if ((char) c == '\r') {
		recv_buf.done = TRUE;
		recv_buf.data[recv_buf.nXfrd] = 0;	// String terminator
		recv_buf.nBytes = recv_buf.nXfrd;
		recv_buf.nXfrd = 0;
		return;
		} else {
		recv_buf.data[recv_buf.nXfrd++] = c;
	}

}

/*---------------------------------------------------------------------
Interrupt routine for transmit data register empty.

The transmit buffer is empty and you can send another byte.
---------------------------------------------------------------------*/
ISR(USART0_DRE_vect)
{

	USART0.CTRLA &= ~USART_DREIE_bm;	// Turn off interrupts
	USART0.TXDATAL = send_buf.data[send_buf.nXfrd++];
	if (send_buf.nXfrd >= send_buf.nBytes) {
		send_buf.done = TRUE;
		} else {
		USART0.CTRLA |= USART_DREIE_bm;	// Turn on interrupts
	}

}

/*----------------------------------------------------------------------
Serial command received ('\r' detected by ISR).
----------------------------------------------------------------------*/
void handle_serial(void)
{
	uint8_t *strptr, prompt, new_interval, new;

	strptr = recv_buf.data;
	switch (*strptr) {
		case 'c':						// Close a valve
			strptr++;
			prompt = 1;
			if (*strptr == 'b') {
				CLOSEVALVE(BLUVALVE);		// Close blue vent
				status.opentime_BLU = 0;
				status.maxopen_BLU = FALSE;
			} else if (*strptr == 'r') {
				CLOSEVALVE(REDVALVE);		// Close red vent
				status.opentime_RED = 0;
				status.maxopen_RED = FALSE;
			} else if (*strptr == 'B') {
				CLOSEVALVE(BUFVALVE);		// Close buffer vent
				status.opentime_BUF = 0;
				status.maxopen_BUF = FALSE;
			} else if (*strptr == 's') {
				CLOSEVALVE(SUPVALVE);		// Close supply valve
				status.opentime_SUP = 0;
			} else {						// Error
				prompt = 0;
			}
			valve_changed = TRUE;
			break;

		case 'd':						// Disable red or blue
			strptr++;
			prompt = 1;
			if (*strptr == 'b') {
				CLOSEVALVE(BLUVALVE);		// Immediately close valve
				status.opentime_BLU = 0;
				eeprom_update_byte((uint8_t *)BLUENABLEADDR, FALSE);
				status.maxopen_BLU = FALSE;
			} else if (*strptr == 'r') {
				CLOSEVALVE(REDVALVE);
				status.opentime_RED = 0;
				eeprom_update_byte((uint8_t *)REDENABLEADDR, FALSE);
				status.maxopen_RED = FALSE;
			} else {					// Error
				prompt = 0;
			}
			valve_changed = TRUE;
			break;

		case 'e':						// Enable red or blue
			strptr++;
			prompt = 1;
			if (*strptr == 'b') {
				eeprom_update_byte((uint8_t *)BLUENABLEADDR, TRUE);
				} else if (*strptr == 'r') {
				eeprom_update_byte((uint8_t *)REDENABLEADDR, TRUE);
				} else {					// Error
				prompt = 0;
			}
			valve_changed = TRUE;
			break;

		case 'o':						// Open a valve
			strptr++;
			prompt = 1;
			if (*strptr == 'b') {
				status.next_fill = FILLINTERVAL;
			}
			if ((*strptr == 'b') && BLUENABLED) {
				OPENVALVE(BLUVALVE);
				status.opentime_BLU = 0;
				status.maxopen_BLU = FALSE;
			} else if ((*strptr == 'r') && REDENABLED) {
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
				status.maxopen_RED = FALSE;
			} else if (*strptr == 'B') {
				OPENVALVE(BUFVALVE);
				status.opentime_BUF = 0;
				status.maxopen_BUF = FALSE;
			} else if (*strptr == 's') {
				OPENVALVE(SUPVALVE);
				status.opentime_SUP = 0;
			} else {						// Error
				prompt = 0;
			}
			valve_changed = TRUE;
			break;

		case '\0':						// Carriage return read by ISR
			prompt = 1;						// (ISR converts to NULL)
			break;

		case 's':						// Print status
			print_Status();
			prompt = 1;
			break;

		case 'w':						// Write eeprom
			strptr++;
			prompt = 1;
			if (*strptr == 'v') {			// Version (10 bytes)
				strptr++;
				eeprom_update_block((const void *)strptr, (void *)VERSIONADDR, 10);
				eeprom_read_block((void *)status.version, (const void *)VERSIONADDR, 10);
			} else if (*strptr == 'i') {	// Fill interval (1 byte)
				strptr++;
				new_interval = atoi((char *)strptr);
				if (new_interval > 60) {
					new_interval = 60;
					} else if (new_interval < 2) {
					new_interval = 2;
				}
				eeprom_update_byte((uint8_t *)FILLINTADDR, new_interval);

				// Adjust next_fill (could be better...)
				if (FILLINTERVAL < status.next_fill) {
					status.next_fill = FILLINTERVAL;
				}

				// Adjust MAXOPENTIME
				if (MAXOPENTIME > new_interval/2) {
					eeprom_update_byte((uint8_t *)MAXOPENADDR, new_interval/2);
				}
			} else if (*strptr == 'm') {	// Maximum open time
				strptr++;
				new = atoi((char *)strptr);
				if (new > (FILLINTERVAL/2)) {
					new = FILLINTERVAL/2;
				} else if (new <= 2) {
					new = 1;
				}
				eeprom_update_byte((uint8_t *)MAXOPENADDR, new);
			} else {
				prompt = 0;
			}
			break;

		default:
			prompt = 0;
			break;
	}

	while (!send_buf.done) {		// Wait for print_status if necessary
		asm("nop");
	}

	if (prompt) {
		strcpy ((char *)send_buf.data, ">");
		} else {
		strcpy((char *)send_buf.data, "?");		// On error
	}

	display_type = VALVESTATE;			// NOT DO THIS?

	send_buf.nBytes = strlen((char*) send_buf.data);
	send_serial();
	recv_buf.done = FALSE;


}

/*----------------------------------------------------------------------
INITIALIZE TWI

Instructions from the data sheet (p327):

1. Set SDASETUP and SDAHOLD in TWI.CTRLA. We will use the defaults
   so no special setup is needed.

2. Write Master Baud Rate Register TWIn.BAUD before enabling TWI master.
   The data sheet formula is (F_SCL is the TWI frequency):
	 F_SCL = F_CPU/(10 + 2*BAUD + F_CPU*T_RISE) or
	 BAUD = (F_CPU/2*F_SCL) - 5 - ((F_CPU*T_RISE)/2)
   I/O pin rise time (T_RISE) is 1.5 ns with 20 pF loading at 5V (p470).
   So we will ignore the rise time so
	 BAUD = (F_CPU/2*F_SCL) - 5
	 BAUD = 12 gives F_SCL =  98015 for F_CPU=3333333
	 BAUD = 11 gives F_SCL = 104140 for F_CPU=3333333

2a. Enable smart mode (p337) by setting the SMEN bit in TWI.MCTRLA. The
   ACKACT bit must also be set to ACK in TWI.MCTRLB (default; the data
   sheet seems to have an error in saying this bit is in TWI.MCTRLA).
   Smart mode causes TWI to send an ACK automatically after data are
   read from TWI.MDATA.

3. Write a '1' to the ENABLE bit in TWIn.MCTRLA.

4. Set bus state to IDLE by writing 0x01 to BUSSTATE in TWIn.MSTATUS.

A write sequence might be:

	start_TWI(slave_address, ReadOrWrite);
	write_TWI(uint8_t *data, nbytes);
	stop_TWI();
----------------------------------------------------------------------*/
void init_TWI(void)
{
	
	TWI0.MBAUD = TWIBAUD;
	TWI0.MCTRLA |= TWI_SMEN_bm;
	TWI0.MCTRLA |= TWI_ENABLE_bm;
	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc;

}

/*----------------------------------------------------------------------
Read an array of data from the TWI bus. Not tested.
----------------------------------------------------------------------*/
void read_TWI(uint8_t *result, uint8_t nbytes)
{
	
	uint8_t i;

	for (i = 0; i < nbytes; i++) {
		*result++ = TWI0.MDATA;
		while (!(TWI0.MSTATUS & TWI_WIF_bm)) {
			asm("nop");
		}
	}
}

/*--------------------------------------------------------------------
Put a start condition on the bus and send the slave address.
--------------------------------------------------------------------*/
void start_TWI(uint8_t addr, uint8_t rw)
{

	if (rw == TWIREAD) {
		addr |= 0x01;
	}
	TWI0.MADDR = addr;
	while(!(TWI0.MSTATUS & TWI_WIF_bm)) {
		asm("nop");
	}
	
}

/*----------------------------------------------------------------------
Put a stop condition on the bus.
----------------------------------------------------------------------*/
void stop_TWI(void)
{
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

/*----------------------------------------------------------------------
Write an array of data to the TWI bus.
----------------------------------------------------------------------*/
void write_TWI(uint8_t *data, uint8_t nbytes)
{

	uint8_t i;

	for (i = 0; i < nbytes; i++) {
		TWI0.MDATA = *data++;
		while (!(TWI0.MSTATUS & TWI_WIF_bm)) {
			asm("nop");
		}
	}
}

/*----------------------------------------------------------------------
INITIALIZE OLED DISPLAY

Newhaven NHD-0216AW-1B3 OLED display. This is taken straight from
https://github.com/NewhavenDisplay/NHD_US2066. The manual is not easy
to understand but this seems to work.
----------------------------------------------------------------------*/
void init_OLED(void)
{

	// Set reset pin high, then wait 10 ms here

	write_OLED(OLEDCMD, 0x2A);	// Function set (extended command set)
	write_OLED(OLEDCMD, 0x71);	//function selection A
//	write_OLED(OLEDDATA, 0x00);	// disable internal VDD regulator (2.8V I/O). data(0x5C) = enable regulator (5V I/O)
	write_OLED(OLEDDATA, 0x5C);	// Enable internal VDD regulator (data(0x5C) = enable regulator (5V I/O)
	write_OLED(OLEDCMD, 0x28);	//function set (fundamental command set)
	write_OLED(OLEDCMD, 0x08);	//display off, cursor off, blink off
	write_OLED(OLEDCMD, 0x2A);	//function set (extended command set)
	write_OLED(OLEDCMD, 0x79);	//OLED command set enabled
	write_OLED(OLEDCMD, 0xD5);	//set display clock divide ratio/oscillator frequency
	write_OLED(OLEDCMD, 0x70);	//set display clock divide ratio/oscillator frequency
	write_OLED(OLEDCMD, 0x78);	//OLED command set disabled
	write_OLED(OLEDCMD, 0x08);	//extended function set (2-lines)
	write_OLED(OLEDCMD, 0x06);	//COM SEG direction
	write_OLED(OLEDCMD, 0x72);	//function selection B
	write_OLED(OLEDDATA, 0x00);	//ROM CGRAM selection
	write_OLED(OLEDCMD, 0x2A);	//function set (extended command set)
	write_OLED(OLEDCMD, 0x79);	//OLED command set enabled
	write_OLED(OLEDCMD, 0xDA);	//set SEG pins hardware configuration
	write_OLED(OLEDCMD, 0x00);	//set SEG pins hardware configuration
	write_OLED(OLEDCMD, 0xDC);	//function selection C
	write_OLED(OLEDCMD, 0x00);	//function selection C
	write_OLED(OLEDCMD, 0x81);	//set contrast control
	write_OLED(OLEDCMD, 0x7F);	//set contrast control
	write_OLED(OLEDCMD, 0xD9);	//set phase length
	write_OLED(OLEDCMD, 0xF1);	//set phase length
	write_OLED(OLEDCMD, 0xDB);	//set VCOMH deselect level
	write_OLED(OLEDCMD, 0x40);	//set VCOMH deselect level
	write_OLED(OLEDCMD, 0x78);	//OLED command set disabled
	write_OLED(OLEDCMD, 0x28);	//function set (fundamental command set)
	write_OLED(OLEDCMD, 0x01);	//clear display
	write_OLED(OLEDCMD, 0x80);	//set DDRAM address to 0x00
	write_OLED(OLEDCMD, 0x0C);	// Display ON
	_delay_ms(100);				// Wait after display-on command

}

/*----------------------------------------------------------------------
Clear the OLED display (not used)
----------------------------------------------------------------------*/
/*
void clear_OLED(void)
{

	write_OLED(OLEDCMD, 0x01);

}
*/

/*--------------------------------------------------------------------
Write one byte to the display controller preceded by either a command
or data indicator. Used by write_OLED_string and init_OLED (probably
don't need to call this otherwise).
--------------------------------------------------------------------*/
void write_OLED(uint8_t type, uint8_t byteToSend)
{

	uint8_t buf[2];

	start_TWI(OLEDADDR, TWIWRITE);

	buf[1] = byteToSend;
	if (type == OLEDCMD) {
		buf[0] = 0x00;
		write_TWI(buf, 2);
	} else if (type == OLEDDATA) {
		buf[0] = 0x40;
		write_TWI(buf, 2);
	}

	stop_TWI();

}

/*----------------------------------------------------------------------
Write one of the screen options to the OLED display
----------------------------------------------------------------------*/
void update_display(uint8_t type)
{

	char line1[21], line2[21];

	switch (type) {

		case DARKDISPLAY:
			strcpy(line1, "                ");
			strcpy(line2, "                ");
			break;

		case VALVESTATE:
			strcpy(line1, "SUP BUF RED BLU");
			if (SUPVALVEOPEN) {
				strcpy(line2, " O  ");
			} else {
				strcpy(line2, " X  ");
			}
			if (BUFVALVEOPEN) {
				strcat(line2, " O  ");
			} else if (status.maxopen_BUF) {
				strcat(line2, " T  ");
			} else {
				strcat(line2, " X  ");
			}
			if (!REDENABLED) {
				strcat(line2, " D  ");
			} else if (REDVALVEOPEN) {
				strcat(line2, " O  ");
			} else if (status.maxopen_RED) {
				strcat(line2, " T  ");
			} else {
				strcat(line2, " X  ");
			}
			if (!BLUENABLED) {
				strcat(line2, " D");
			} else if (BLUVALVEOPEN) {
				strcat(line2, " O");
			} else if (status.maxopen_BLU) {
				strcat(line2, " T");
			} else {
				strcat(line2, " X");
			}
			break;

		case NEXTFILL:
			strcpy(line1, "Next fill in");
			if (status.next_fill < 2) {
				sprintf(line2, "%d minute", status.next_fill);
			} else {
				sprintf(line2, "%d minutes", status.next_fill);
			}
			break;

		case FILLINTDISPLAY:
			strcpy(line1, "Fill interval");
			if (FILLINTERVAL < 2) {
				sprintf(line2, "%d minute", FILLINTERVAL);
			} else {
				sprintf(line2, "%d minutes", FILLINTERVAL);
			}
			break;

		case TITLEDISPLAY:
			strcpy(line1, "  LN2 Autofill");
			strcpy(line2, " Ver:");
			strcat(line2, status.version);
			break;

		case MAXOPENDISPLAY:
			strcpy(line1, "Max open time");
			if (MAXOPENTIME < 2) {
				sprintf(line2, "%d minute", MAXOPENTIME);
			} else {
				sprintf(line2, "%d minutes", MAXOPENTIME);
			}
			break;;

		case PRESSUREDISPLAY:
			strcpy(line1, "LN2 pressure");
			sprintf(line2, "%d kPa", status.pressure);
			break;

		case DISBLUDISPLAY:
			if (BLUENABLED) {
				strcpy(line1, "BLUE is enabled");
				strcpy(line2, "Push to disable");
			} else {
				strcpy(line1, "Blue is DISABLED");
				strcpy(line2, "Push to enable");
			}
			break;

		case DISREDDISPLAY:
			if (REDENABLED) {
				strcpy(line1, "RED is enabled");
				strcpy(line2, "Push to disable");
				} else {
				strcpy(line1, "RED is DISABLED");
				strcpy(line2, "Push to enable");
			}
			break;

		case DISPLAY9:
			strcpy(line1, "*******");
			strcpy(line2, "         *******");
			break;

		case DISPLAY10:
			strcpy(line1, "******");
			strcpy(line2, "          ******");
			break;

		case DISPLAY11:
			strcpy(line1, "*****");
			strcpy(line2, "           *****");
			break;

		case DISPLAY12:
			strcpy(line1, "****");
			strcpy(line2, "            ****");
		break;

		case DISPLAY13:
			strcpy(line1, "***");
			strcpy(line2, "             ***");
			break;

		case DISPLAY14:
			strcpy(line1, "**");
			strcpy(line2, "              **");
			break;

		case DISPLAY15:
			strcpy(line1, "*");
			strcpy(line2, "               *");
			break;

		default:
			strcpy(line1, "?");
			strcpy(line2, "?");
			break;
	}

	write_OLED_string(line1, 1);
	write_OLED_string(line2, 2);

}

/*----------------------------------------------------------------------
Write a string to the Newhaven TWI OLED display. Positions the cursor
at the beginning of a line (1 or 2) and then writes 16 characters to
the display, padded with spaces at the end.
----------------------------------------------------------------------*/
void write_OLED_string(char *str, uint8_t lineno)
{

	uint8_t i;
	char strbuf[33], blanks[] = "                ";

	strcpy(strbuf, str);
	strcat(strbuf, blanks);
	if (lineno == 1) {
		write_OLED(OLEDCMD, OLEDLINE1);
		} else {
		write_OLED(OLEDCMD, OLEDLINE2);
	}
	for (i = 0; i < 16; i++) {
		write_OLED(OLEDDATA, strbuf[i]);
	}

}

/*----------------------------------------------------------------------
INITIALIZE REAL TIME CLOCK

The real time clock (RTC) controller uses an on-board 32.768 kHz crystal
to generate an interrupt every minute. The interrupt routine resets the
fill interval timer and if necessary opens an LN2 vent valve to start
abort fill. It also closes valves if they have been open too long.

The RTC is configured to run at 512 Hz (64x divider) and we load the
period register Period=30719 which generates an interrupt every minute.
----------------------------------------------------------------------*/
void init_RTC(void)
{

	uint8_t temp;

	// Disable the external oscillator by clearing the enable bit 0
	temp = CLKCTRL.XOSC32KCTRLA;
	temp &= ~CLKCTRL_ENABLE_bm;		// set bit 0 of CLKCTRL.XOSC32KCTRLA to 0
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSC32KCTRLA = temp;

	// Wait for status bit (bit 6) in MCLKSTATUS to go to 0 (XOSC32K not running)
	while (CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm) {
		asm("nop");
	}

	// Select the external crystal (as opposed to external clock)
	// by setting the SEL bit on XOSC32KCTRLA (bit 2) to 0
	temp = CLKCTRL.XOSC32KCTRLA;
	temp &= ~CLKCTRL_SEL_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSC32KCTRLA = temp;

	// Enable the external oscillator by setting the enable bit
	// (bit 0) in CLKCTRL.XOSC32KCTRLA to 1
	temp = CLKCTRL.XOSC32KCTRLA;
	temp |= CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSC32KCTRLA = temp;

	while (RTC.STATUS) {	// Wait for all registers to sync
		asm("nop");
	}

	RTC.PER = 30719;		// Set 1 minute period for overflow interrupts 
//	RTC.PER = 511;		// Set 1 sec period for overflow interrupts (4testing)

	// Select the external crystal oscillator in RTC.CLKSEL register
	RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc;

	// Enable running in debug mode by setting bit 0 in RTC.CLKSEL to 1
	RTC.DBGCTRL |= RTC_DBGRUN_bm;

	// Enable running in standby mode by setting bit 7 in RTC.CTRLA to 1
	// & set the prescaler to DIV64 (512 Hz)
	// & set the RTC enable bit
	RTC.CTRLA = RTC_PRESCALER_DIV64_gc | RTC_RUNSTDBY_bm | RTC_RTCEN_bm;

	RTC.INTCTRL |= RTC_OVF_bm;	// Enable overflow interrupt

}

/*---------------------------------------------------------------------
Interrupt routine for RTC

Start a fill after the fill interval has passed. Increment the fill
interval time and the valve-open times. If the valve has been open
longer than MAXOPENTIME or BUFMAXOPEN, then close the valve and set
the MAXOPEN flag. Usually, the valve should be closed by a thermistor
signal before MAXOPEN times are reached.
----------------------------------------------------------------------*/
ISR(RTC_CNT_vect)
{

	RTC.INTFLAGS = RTC_OVF_bm;				// Clear interrupt flag

	if (BLUVALVEOPEN) {
		if (++status.opentime_BLU >= MAXOPENTIME) {
			CLOSEVALVE(BLUVALVE);			// Maximum open time reached
			status.opentime_BLU = 0;
			status.maxopen_BLU = TRUE;
			// Red dewar fill always follows the end of a blue dewar fill
			if (REDENABLED && REDTHERMWARM) {
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
				status.maxopen_RED = FALSE;
			}
		}
	}

	if (REDVALVEOPEN) {		// Off-by-1 error somewhere...
		if (status.opentime_RED++ >= MAXOPENTIME) {
			CLOSEVALVE(REDVALVE);			// Maximum open time reached
			status.opentime_RED = 0;
			status.maxopen_RED = TRUE;
		}
	}

	if (BUFVALVEOPEN) {
		if (++status.opentime_BUF > BUFMAXOPEN) {
			CLOSEVALVE(BUFVALVE);
			status.opentime_BUF = 0;
			status.maxopen_BUF = TRUE;
			CLOSEVALVE(SUPVALVE);			// Maximum open time reached
			status.opentime_SUP = 0;
		}
	}

	if (SUPVALVEOPEN) {
		++status.opentime_SUP;
	}

	if (--status.next_fill <= 0) {
		// Start a fill cycle
		if (BLUENABLED && BLUTHERMWARM) {
			OPENVALVE(BLUVALVE);
			status.opentime_BLU = 0;
			status.maxopen_BLU = FALSE;
		} else if (REDENABLED && REDTHERMWARM) {
			OPENVALVE(REDVALVE);
			status.opentime_RED = 0;
			status.maxopen_RED = FALSE;
		}
		status.next_fill = FILLINTERVAL;
	}
	valve_changed = TRUE;			// Forces new display
	read_pressure = TRUE;			// Read the pressure every minute
}


/*----------------------------------------------------------------------
STATUS BLOCK INITIALIZATION
----------------------------------------------------------------------*/
void init_Status()
{

	strcpy((char *)status.version, VERSION);
	eeprom_update_block((const void *)status.version, (void *)VERSIONADDR, 10);
	status.next_fill = FILLINTERVAL;
	status.opentime_BLU = 0;
	status.opentime_RED = 0;
	status.opentime_BUF = 0;
	status.opentime_SUP = 0;
	status.maxopen_BLU = FALSE;		// TRUE when open too long
	status.maxopen_RED = FALSE;
	status.maxopen_BUF = FALSE;
	handle_pressure();

}

/*----------------------------------------------------------------------
Print the status on the serial port. Values are written to sbuf[] and
sent to the serial port at the end.
----------------------------------------------------------------------*/
void print_Status(void)
{

	char sbuf[20];

	ADC0_COMMAND |= ADC_STCONV_bm;		// Start ADC pressure conversion

	// Firmware version (a date YYYY-MM-DD)
	strcpy((char *)send_buf.data, "Ver");
	strcat((char *)send_buf.data, (char *)status.version);
	strcat((char *)send_buf.data, " ");

	// Time in minutes until next fill
	strcat((char *)send_buf.data, "Nxt");
	strcat((char *)send_buf.data, itoa(status.next_fill, sbuf, 10));
	strcat((char *)send_buf.data, " ");

	//  Blue valve status (time open, closed, disabled)
	strcat((char *)send_buf.data, "BlV");
	if (!BLUENABLED) {
		strcat((char *)send_buf.data, "D ");
	} else if (BLUVALVEOPEN) {
		strcat((char *)send_buf.data, itoa(status.opentime_BLU, sbuf, 10));
		strcat((char *)send_buf.data, " ");
	} else if (status.maxopen_BLU) {
		strcat((char *)send_buf.data, "T ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}

	//  Red valve status (time open, closed, disabled)
	strcat((char *)send_buf.data, "RdV");
	if (!REDENABLED) {
		strcat((char *)send_buf.data, "D ");
	} else if (REDVALVEOPEN) {
		strcat((char *)send_buf.data, itoa(status.opentime_RED, sbuf, 10));
		strcat((char *)send_buf.data, " ");
	} else if (status.maxopen_RED) {
		strcat((char *)send_buf.data, "T ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}

	//  Buffer valve status (time open, closed)
	strcat((char *)send_buf.data, "BuV");
	if (BUFVALVEOPEN) {
		strcat((char *)send_buf.data, itoa(status.opentime_BUF, sbuf, 10));
		strcat((char *)send_buf.data, " ");
	} else if (status.maxopen_BUF) {
		strcat((char *)send_buf.data, "T ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}

	//  Supply valve status (time open, closed)
	strcat((char *)send_buf.data, "SuV");
	if (SUPVALVEOPEN) {
		strcat((char *)send_buf.data, itoa(status.opentime_SUP, sbuf, 10));
		strcat((char *)send_buf.data, " ");
		} else {
		strcat((char *)send_buf.data, "C ");
	}

	// Blue thermistor (warm or cold)
	strcat((char *)send_buf.data, "BlT");
	if (BLUTHERMWARM) {
		strcat((char *)send_buf.data, "W ");
		} else {
		strcat((char *)send_buf.data, "C ");
	}

	// Red thermistor (warm or cold)
	strcat((char *)send_buf.data, "RdT");
	if (REDTHERMWARM) {
		strcat((char *)send_buf.data, "W ");
		} else {
		strcat((char *)send_buf.data, "C ");
	}

	// Buffer thermistor (warm or cold)
	strcat((char *)send_buf.data, "BuT");
	if (BUFTHERMWARM) {
		strcat((char *)send_buf.data, "W ");
		} else {
		strcat((char *)send_buf.data, "C ");
	}

	// Fill interval
	strcat((char *)send_buf.data, "Int");
	strcat((char *)send_buf.data, itoa(FILLINTERVAL, sbuf, 10));
	strcat((char *)send_buf.data, " ");

	// Maximum open duration
	strcat((char *)send_buf.data, "Max");
	strcat((char *)send_buf.data, itoa(MAXOPENTIME, sbuf, 10));
	strcat((char *)send_buf.data, " ");

	// LN2 pressure
	strcat((char *)send_buf.data, "kPa");
	strcat((char *)send_buf.data, itoa(status.pressure, sbuf, 10));

	// Send the string
	strcat((char *)send_buf.data, "\r");
	send_buf.nBytes = strlen((char *)send_buf.data);
	send_serial();
	
}
