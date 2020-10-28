/*
 * LN2 Controller.c
 *
 * Created: 2/28/2020 12:32:37 PM
 * Author : Alan Uomoto
 */ 

#define F_CPU			3333333UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>
#include <string.h>

#define TRUE			1
#define FALSE			0

// LED on the Curiosity Nano board
#define TOGGLELED		(PORTF.OUTTGL |= PIN5_bm)

// Oscillator signal for debouncer on PC0 (temporary)
#define TCB2FREQ		1500		// Debouncer PWM frequency in Hz

// Pushbutton defines
#define NONE			0
#define BLUEBUTTON		1
#define REDBUTTON		2
#define BUFFERBUTTON	3
#define SUPPLYBUTTON	4
#define ENCODERBUTTON	5
#define NANOBUTTON		6
#define ENCODERA		7

// TWI defines
#define TWIFREQ			100000UL	// TWI bus speed
#define TWIBAUD			((uint8_t) (F_CPU/(2*TWIFREQ)) - 5)	// Ignore rise time
#define TWIREAD			1
#define TWIWRITE		0

// OLED Display defines
#define CLEARDISPLAY	0x01
#define DISPLAYON		0x0C
#define DISPLAYOFF		0x08
#define OLEDADDR		(0x3c << 1)
#define OLEDCMD			1
#define OLEDDATA		0
#define OLEDLINE1		0x80
#define OLEDLINE2		0xC0

// EEPROM defines
#define VERSIONADDR		(0)
#define FILLINTADDR		(10)
#define MAXOPENADDR		(11)

// Valve defines
#define BUFMAXOPEN				30		// In minutes
#define OPENVALVE(VALVEPIN)		(PORTC.OUTSET |= VALVEPIN)
#define CLOSEVALVE(VALVEPIN)	(PORTC.OUTCLR |= VALVEPIN)
#define BLUVALVE				(PIN7_bm)	// VALVEPIN value
#define REDVALVE				(PIN6_bm)	// VALVEPIN value
#define BUFVALVE				(PIN5_bm)	// VALVEPIN value
#define SUPVALVE				(PIN4_bm)	// VALVEPIN value
// These are TRUE if the valve is open
#define BLUVALVEOPEN			(PORTC.IN & PIN7_bm)
#define REDVALVEOPEN			(PORTC.IN & PIN6_bm)
#define BUFVALVEOPEN			(PORTC.IN & PIN5_bm)
#define SUPVALVEOPEN			(PORTC.IN & PIN4_bm)

// Thermistor defines. These are TRUE if the thermistor is warm
#define BLUTHERMWARM	(PORTE.IN & PIN0_bm)
#define REDTHERMWARM	(PORTE.IN & PIN1_bm)
#define BUFTHERMWARM	(PORTE.IN & PIN2_bm)

// ADC defines

// Serial USART0 defines
#define	USART_BAUD_RATE(BAUD_RATE)	((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)
#define USARTBAUD		9600

// Function prototypes
void clear_OLED(void);
void handle_serial(void);
void handle_switches(void);
void initialize(void);
void init_ADC(void);
void init_LED(void);
void init_OLED(void);
void init_ports(void);
void init_RTC(void);
void init_Serial(void);
void init_status(void);
void init_switches(void);
void init_Thermistors(void);
void init_TCB2(void);
void init_TWI(void);
void init_Valves(void);
void print_status(void);
void read_TWI(uint8_t *, uint8_t);
void send_serial(void);
void start_TWI(uint8_t, uint8_t);
void stop_TWI(void);
void write_OLED(uint8_t, uint8_t);
void write_OLED_string(char *, uint8_t);
void write_TWI(uint8_t *, uint8_t);

struct serial_data {
	uint8_t data[100],		// Data to send or data received
	nBytes;					// Number of data bytes in data[];
	volatile uint8_t done,	// Is the transfer complete?
	nXfrd;					// Temporary counter (number of bytes transferred)
};

struct statusbuf {
	char version[17];
	uint8_t enable_BLU, enable_RED,
		fill_interval,
		next_fill,
		opentime_BLU, opentime_RED,
		opentime_BUF, opentime_SUP,
		maxopentime,
		maxopen_BLU, maxopen_RED,
		maxopen_BUF,
		pressure;
};

// Globals
volatile uint8_t encoder_value, switch_sensed;
struct statusbuf status;
struct serial_data send_buf, recv_buf;

int main(void)
{

	initialize();
	sei();
OPENVALVE(BLUVALVE);

	for (;;) {
		_delay_ms(2);

		if (switch_sensed) {
			handle_switches();
			TOGGLELED;
		}

		if (recv_buf.done) {
			handle_serial();
		}

	}

}

void initialize(void)
{

	init_ports();		// Set all ports to input w/pullups or disabled
	init_LED();			// Set up on-board LED port
	init_RTC();
	init_TCB2();		// Turn on PWM for debouncer
	init_switches();
	init_Valves();
	init_Serial();
	init_TWI();
	init_OLED();
	init_ADC();
	init_status();
	write_OLED_string("LN2 Autofill", 1);
	write_OLED_string(status.version, 2);

}

/*----------------------------------------------------------------------
Set all I/O ports to default input with pullups enabled except ADC
ports that have input disabled. This is just to make sure nothing
lurks in the background by accident.
----------------------------------------------------------------------*/
void init_ports(void)
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
INITIALIZE Timer-counter TCB2

At startup, the TCB is in periodic interrupt mode. To enable this
1. Write a TOP value to TCBn.CCMP
2a. Enable the counter by writing a '1' to the ENABLE bit in TCBn.CTRLA
2b. Set the CLKSEL prescaler bit field in TCBn.CTRLA
3. A CAPT interrupt is genterated when TOP is reached. Enable CAPT
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

	PORTC_DIR |= PIN0_bm;					// PIN PC0 has haywire
	PORTC_OUT |= PIN0_bm;					// to debouncer Osc-in
	TCB2.CCMP = F_CPU/(4 * TCB2FREQ) + 1;
	TCB2.CTRLA |= TCB_ENABLE_bm;			// Enable the timer
	TCB2.CTRLA |= TCB_CLKSEL_CLKDIV2_gc;	// 2X divider
	TCB2.INTCTRL |= TCB_CAPT_bm;			// Enable interrupts
	
}

/*----------------------------------------------------------------------
Generate a square wave by toggling the state of PC0
----------------------------------------------------------------------*/
ISR(TCB2_INT_vect)
{

	TCB2.INTFLAGS = TCB_CAPT_bm;
	PORTC.OUTTGL |= PIN0_bm;

}

/*----------------------------------------------------------------------
PUSHBUTTONS and SWITCHES INITIALIZATION

Four pushbuttons with integrated LED indicators control the solenoid
valves. These switches go through a debounce conditioner IC.

The rotary encoder channels A and B also go through the debouncer IC.

The on-board pushbutton switch on the Curiosity Nano has an external
pullup on it but no debouncing.

The pushbutton on the rotary encoder has a pullup and an RC debouncer.

PB2 is SW1 Blue dewar vent
PB3 is SW2 Red dewar vent
PB0 is SW3 Buffer dewar vent
PB1 is SW4 Buffer dewar supply inlet
PC3 is Encoder A
PC2 is Encoder B
PC1 is Encoder pushbutton
PF6 is Curiosity Nano on-board pushbutton
----------------------------------------------------------------------*/
void init_switches(void)
{

	// Four solenoid valve control switches
	PORTB.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW1
	PORTB.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW2
	PORTB.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW3
	PORTB.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW4

	// Encoder
	PORTC.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// EncA
	PORTC.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// EncP

	// Curiosity Nano on-board pushbutton for testing
	PORTF.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;

	encoder_value = 0;
	switch_sensed = NONE;

}

/*----------------------------------------------------------------------
Interrupt routine for the four ASCO valve pushbuttons
----------------------------------------------------------------------*/
ISR(PORTB_PORT_vect)
{

	if (PORTB.INTFLAGS & PIN2_bm) {		// Blue pushbutton
		PORTB.INTFLAGS = PIN2_bm;		// Clear the interrupt flag
		switch_sensed = BLUEBUTTON;
	}
	if (PORTB.INTFLAGS & PIN3_bm) {		// Red pushbutton
		PORTB.INTFLAGS = PIN3_bm;		// Clear the interrupt flag
		switch_sensed = REDBUTTON;
	}
	if (PORTB.INTFLAGS & PIN0_bm) {		// Buffer pushbutton
		PORTB.INTFLAGS = PIN0_bm;
		switch_sensed = BUFFERBUTTON;
	}
	if (PORTB.INTFLAGS & PIN1_bm) {		// Supply button
		PORTB.INTFLAGS = PIN1_bm;		// Clear the interrupt flag
		switch_sensed = SUPPLYBUTTON;
	}

}

/*----------------------------------------------------------------------
Interrupt routine for encoder rotation or pushbutton
----------------------------------------------------------------------*/
ISR(PORTC_PORT_vect)
{
	if (PORTC.INTFLAGS & PIN1_bm) {		// Encoder pushbutton
		PORTC.INTFLAGS = PIN1_bm;		// Clear the interrupt flag
		switch_sensed = ENCODERBUTTON;
	}
	if (PORTC.INTFLAGS & PIN3_bm) {		// Encoder A pin
		PORTC.INTFLAGS = PIN3_bm;		// Clear the interrupt flag
		if (PORTC.IN & PIN2_bm) {
			encoder_value++;
			} else {
			encoder_value--;
		}
		switch_sensed = ENCODERA;
	}

}

/*----------------------------------------------------------------------
Interrupt routine for Curiosity Nano on-board pushbutton
----------------------------------------------------------------------*/
ISR(PORTF_PORT_vect)
{

	if (PORTF.INTFLAGS & PIN6_bm) {		// Curiosity Nano button
		PORTF.INTFLAGS = PIN6_bm;		// Clear the interrupt flag
		encoder_value = 0;				// Reset encoder count
		switch_sensed = NANOBUTTON;
	}

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
Write an array of data from the TWI bus.
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
	write_OLED(OLEDDATA, 0x00);	// disable internal VDD regulator (2.8V I/O). data(0x5C) = enable regulator (5V I/O)
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
Clear the OLED display
----------------------------------------------------------------------*/
void clear_OLED(void)
{

	write_OLED(OLEDCMD, 0x01);

}

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
Write a string to the Newhaven TWI OLED display. Positions the cursor
at the beginning of a line (1 or 2) and then writes 16 characters to
the display, padded with spaces at the end.
----------------------------------------------------------------------*/
void write_OLED_string(char *str, uint8_t lineno)
{

	uint8_t i;
	char strbuf[41], blanks[] = "                ";

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
Pushbutton or encoder interrupt detected; do something here.
----------------------------------------------------------------------*/
void handle_switches(void)
{

	char strbuf[17];

	write_OLED(OLEDCMD, CLEARDISPLAY);
	_delay_ms(4);

	switch (switch_sensed) {
		case BLUEBUTTON:
			write_OLED_string("Blue button", 1);
			break;
		case REDBUTTON:
			write_OLED_string("Red button", 1);
			break;
		case BUFFERBUTTON:
			write_OLED_string("Buffer button", 1);
			break;
		case SUPPLYBUTTON:
			write_OLED_string("Supply button", 1);
			break;
		case ENCODERBUTTON:
			sprintf(strbuf, "Encoder %d", encoder_value);
			write_OLED_string(strbuf, 1);
			write_OLED_string("Encoder button", 2);
			break;
		case ENCODERA:
			sprintf(strbuf, "Encoder %d", encoder_value);
			write_OLED_string(strbuf, 1);
			break;
		case NANOBUTTON:
			sprintf(strbuf, "Encoder %d", encoder_value);
			write_OLED_string(strbuf, 1);
			write_OLED_string("Reset encoder", 2);
			break;
		default:
			write_OLED_string("Error", 1);
			break;
	}
	switch_sensed = NONE;
}

/*----------------------------------------------------------------------
Fill the status structure with initial values
----------------------------------------------------------------------*/
void init_status(void)
{
	strcpy(status.version, "2020-02-29");
	eeprom_update_block((const void *)status.version, (void *)VERSIONADDR, 10);
	status.enable_BLU = TRUE;	// USE EEPROM INSTEAD
	status.enable_RED = TRUE;	// USE EEPROM INSTEAD
	status.fill_interval = eeprom_read_byte((uint8_t *)FILLINTADDR);
	status.next_fill = status.fill_interval;
	status.opentime_BLU = 0;
	status.opentime_RED = 0;
	status.opentime_BUF = 0;
	status.opentime_SUP = 0;
	status.maxopentime = eeprom_read_byte((uint8_t *)MAXOPENADDR);
	status.maxopen_BLU = FALSE;
	status.maxopen_RED = FALSE;
	status.maxopen_BUF = FALSE;
	status.pressure = 0;		// fix this
}

/*----------------------------------------------------------------------
INITIALIZE VALVES

All pins set to low and output. These pins drive MOSFETs that control
the LN2 solenoid valves.

VALVn is the electronics schematic name.

VALV1 is on pin PC7 (Blue dewar vent)
VALV2 is on pin PC6 (Red dewar vent)
VALV3 is on pin PC5 (Buffer dewar vent)
VALV4 is on pin PC4 (Supply input valve)
----------------------------------------------------------------------*/
void init_Valves(void)
{

	// Remove pullups (set at init_ports) (DON'T NEED THIS?)
	PORTC.PIN4CTRL = 0;
	PORTC.PIN5CTRL = 0;
	PORTC.PIN6CTRL = 0;
	PORTC.PIN7CTRL = 0;
	// Set valve control pins as outputs, turned off initially.
	PORTC.OUTCLR = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;
	PORTC.DIRSET = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;

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
	USART0.BAUD = (uint16_t) USART_BAUD_RATE(USARTBAUD);
	USART0.CTRLA |= USART_RXCIE_bm;
	USART0.CTRLB |= USART_TXEN_bm;
	USART0.CTRLB |= USART_RXEN_bm;

	send_buf.done = TRUE;
	recv_buf.done = FALSE;

}

/*----------------------------------------------------------------------
Send serial data on USART0

To do this, fill send_buf.data, set send_buf.nBytes, then call this.
This routine turns on interrupts. The interrupt routine will continue
to fill send.data until a '\r' is seen, after which send_buf.done is
set to TRUE and the command loop can proceed.
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
	if ((char) c == '\r') {					// If a cr is seen
		recv_buf.done = TRUE;				// End the data input
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
Parse a serial line command and reply.
----------------------------------------------------------------------*/
void handle_serial(void)
{

	uint8_t *strptr, prompt;

	strptr = recv_buf.data;
	switch (*strptr) {
		case 'c':							// Close a valve
			strptr++;
			prompt = 1;
			if (*strptr == 'b') {
				CLOSEVALVE(BLUVALVE);		// Close blue vent
				status.opentime_BLU = 0;
			} else if (*strptr == 'r') {
				CLOSEVALVE(REDVALVE);		// Close red vent
				status.opentime_RED = 0;
			} else if (*strptr == 'B') {
				CLOSEVALVE(BUFVALVE);		// Close buffer vent
				status.opentime_BUF = 0;
			} else if (*strptr == 's') {
				CLOSEVALVE(SUPVALVE);		// Close supply valve
				status.opentime_SUP = 0;
			} else {						// Error
				prompt = 0;
			}
			break;

		case 'd':							// Disable red or blue
			strptr++;
			prompt = 1;
			if (*strptr == 'b') {
				CLOSEVALVE(BLUVALVE);		// Immediately close valve
				status.opentime_BLU = 0;
				status.enable_BLU = FALSE;	// Set enable flag
			} else if (*strptr == 'r') {
				CLOSEVALVE(REDVALVE);
				status.opentime_RED = 0;
				status.enable_RED = FALSE;	// Set enable flag
			} else {						// Error
				prompt = 0;
			}
			break;

		case 'e':							// Enable red or blue
			strptr++;
			prompt = 1;
			if (*strptr == 'b') {
				status.enable_BLU = TRUE;	// Set enable flag
			} else if (*strptr == 'r') {
				status.enable_RED = TRUE;	// Set enable flag
			} else {						// Error
				prompt = 0;
			}
			break;

		case 'o':							// Open a valve
			strptr++;
			prompt = 1;
			if ((*strptr == 'b') && status.enable_BLU) {
				OPENVALVE(BLUVALVE);
				status.opentime_BLU = 0;
			} else if ((*strptr == 'r') && status.enable_RED) {
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
			} else if (*strptr == 'B') {
				OPENVALVE(BUFVALVE);
				status.opentime_BUF = 0;
			} else if (*strptr == 's') {
				OPENVALVE(SUPVALVE);
				status.opentime_SUP = 0;
			} else {						// Error
				prompt = 0;
			}
			break;

		case '\0':							// Carriage return read
			prompt = 1;
			break;

		case 's':							// Print status
			print_status();
			prompt = 1;
			break;

		case 'w':							// Write eeprom
			strptr++;
			prompt = 1;
			if (*strptr == 'v') {			// Version (10 bytes)
				strptr++;
				eeprom_update_block((const void *)strptr, (void *)VERSIONADDR, 10);
				eeprom_read_block((void *)status.version, (const void *)VERSIONADDR, 10);
			} else if (*strptr == 'i') {	// Fill interval (1 byte)
				strptr++;
				status.fill_interval = atoi((char *)strptr);
				if (status.fill_interval > 60) {
					status.fill_interval = 60;
					} else if (status.fill_interval < 2) {
					status.fill_interval = 2;
				}
				eeprom_update_byte((uint8_t *)FILLINTADDR, status.fill_interval);
				status.fill_interval = eeprom_read_byte((uint8_t *)FILLINTADDR);
				if (status.maxopentime > (status.fill_interval/2)) {
					status.maxopentime = status.fill_interval/2;
				}
				if (status.next_fill > status.fill_interval) {
					status.next_fill = status.fill_interval;
				}
			} else if (*strptr == 'm') {	// Maximum open time
				strptr++;
				status.maxopentime = atoi((char *)strptr);
				if (status.maxopentime > (status.fill_interval/2)) {
					status.maxopentime = status.fill_interval/2;
				} else if (status.maxopentime <= 2) {
					status.maxopentime = 1;
				}
				eeprom_update_byte((uint8_t *)MAXOPENADDR, status.maxopentime);
				status.maxopentime = eeprom_read_byte((uint8_t *)MAXOPENADDR);
			} else {
				prompt = 0;
			}
			break;

		default:
			prompt = 0;
			break;
	}

	while (!send_buf.done) {		// Wait for print status if necessary
		asm("nop");
	}

	if (prompt) {
		strcpy ((char *)send_buf.data, ">");
		} else {
		strcpy((char *)send_buf.data, "?");		// On error
	}
	send_buf.nBytes = strlen((char*) send_buf.data);
	send_serial();
	recv_buf.done = FALSE;
}

/*----------------------------------------------------------------------
Prints the status block to the serial line
----------------------------------------------------------------------*/
void print_status(void)
{

	char sbuf[20];

	ADC0_COMMAND |= ADC_STCONV_bm;		// Start ADC conversion for pressure
	strcpy((char *)send_buf.data, "Ver");
	strcat((char *)send_buf.data, (char *)status.version);
	strcat((char *)send_buf.data, " ");
	strcat((char *)send_buf.data, "Nxt");
	strcat((char *)send_buf.data, itoa(status.next_fill, sbuf, 10));
	strcat((char *)send_buf.data, " ");
	strcat((char *)send_buf.data, "BlV");
	if (status.enable_BLU == FALSE) {
		strcat((char *)send_buf.data, "D ");
	} else if (BLUVALVEOPEN) {
		strcat((char *)send_buf.data, itoa(status.opentime_BLU, sbuf, 10));
		strcat((char *)send_buf.data, " ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}
	strcat((char *)send_buf.data, "RdV");
	if (status.enable_RED == FALSE) {
		strcat((char *)send_buf.data, "D ");
	} else if (REDVALVEOPEN) {
		strcat((char *)send_buf.data, itoa(status.opentime_RED, sbuf, 10));
		strcat((char *)send_buf.data, " ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}
	strcat((char *)send_buf.data, "BuV");
	if (BUFVALVEOPEN) {
		strcat((char *)send_buf.data, itoa(status.opentime_BUF, sbuf, 10));
		strcat((char *)send_buf.data, " ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}
	strcat((char *)send_buf.data, "SuV");
	if (SUPVALVEOPEN) {
		strcat((char *)send_buf.data, itoa(status.opentime_SUP, sbuf, 10));
		strcat((char *)send_buf.data, " ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}
	strcat((char *)send_buf.data, "BlT");
	if (BLUTHERMWARM) {
		strcat((char *)send_buf.data, "W ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}
	strcat((char *)send_buf.data, "RdT");
	if (REDTHERMWARM) {
		strcat((char *)send_buf.data, "W ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}
	strcat((char *)send_buf.data, "BuT");
	if (BUFTHERMWARM) {
		strcat((char *)send_buf.data, "W ");
	} else {
		strcat((char *)send_buf.data, "C ");
	}
	strcat((char *)send_buf.data, "Int");
	strcat((char *)send_buf.data, itoa(status.fill_interval, sbuf, 10));
	strcat((char *)send_buf.data, " ");
	strcat((char *)send_buf.data, "Max");
	strcat((char *)send_buf.data, itoa(status.maxopentime, sbuf, 10));
	strcat((char *)send_buf.data, " ");

	while (ADC0_COMMAND & ADC_STCONV_bm) {
		asm("nop");
	}
	status.pressure = ADC0_RES;
	//compute pressure here
	strcat ((char *)send_buf.data, "Psi");
	strcat((char *)send_buf.data, itoa(status.pressure, sbuf, 10));
	strcat((char *)send_buf.data, "\r\n");
	send_buf.nBytes = strlen((char *)send_buf.data);
	send_serial();

}

/*----------------------------------------------------------------------
REAL TIME CLOCK INITIALIZATION

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

		RTC.PER = 30719;		// Set period for overflow interrupts
//	RTC.PER = 511;		// Set period for overflow interrupts (testing value)

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
longer than status.max_opentime or BUFMAXOPEN, then close the valve
and set the MAXOPEN flag. Usually, the valve should be closed by a
thermistor signal before MAXOPEN times are reached.
----------------------------------------------------------------------*/
ISR(RTC_CNT_vect)
{

	RTC.INTFLAGS = RTC_OVF_bm;				// Clear interrupt flag

	if (BLUVALVEOPEN) {
		if (++status.opentime_BLU >= status.maxopentime) {
			CLOSEVALVE(BLUVALVE);			// Maximum open time reached
			status.opentime_BLU = 0;
			status.maxopen_BLU = TRUE;
			// Red dewar fill always follows the end of a blue dewar fill
			if (status.enable_RED && (REDTHERMWARM)) {
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
			}
		}
	}

	if (REDVALVEOPEN) {
		if (++status.opentime_RED >= status.maxopentime) {
			CLOSEVALVE(REDVALVE);			// Maximum open time reached
			status.opentime_RED = 0;
			status.maxopen_RED = TRUE;
		}
	}

	if (BUFVALVEOPEN) {
		if (++status.opentime_BUF > BUFMAXOPEN) {
			CLOSEVALVE(SUPVALVE);			// Maximum open time reached
			status.opentime_SUP = 0;
			_delay_ms(250);
			CLOSEVALVE(BUFVALVE);
			status.opentime_SUP = 0;
			status.maxopen_BUF = TRUE;
		}
	}

	if (SUPVALVEOPEN) {
		++status.opentime_SUP;
	}

	if (--status.next_fill == 0) {
		// Start a fill cycle
		if (status.enable_BLU && (BLUTHERMWARM)) {
			OPENVALVE(BLUVALVE);
			status.opentime_BLU = 0;
			status.maxopen_BLU = FALSE;
		} else if (status.enable_RED && (REDTHERMWARM)) {
			OPENVALVE(REDVALVE);
			status.opentime_RED = 0;
			status.maxopen_RED = FALSE;
		}
		status.next_fill = status.fill_interval;
	}

}

/*---------------------------------------------------------------------
INITIALIZE THERMISTORS

Thermistor signals are digital, conditioned by a comparator. Signals
are on PORTE, pins 0, 1, 2, and 3, but thermistors are connected only
to pins 0, 1, and 2 (the original SDSS system had a thermistor at the
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
that is warming up is probably closed already but doing this doesn't
hurt anything.
----------------------------------------------------------------------*/
ISR(PORTE_PORT_vect)
{

	if (PORTE.INTFLAGS & PIN0_bm) {		// Blue dewar thermistor changed state
		PORTE.INTFLAGS |= PIN0_bm;		// Clear the interrupt flag
		CLOSEVALVE(BLUVALVE);			// Close the valve whether open or not
		status.opentime_BLU = 0;
		if (!BLUTHERMWARM) {
			status.maxopen_BLU = FALSE;	// Clear a max_opentime limit
			if (status.enable_RED && REDTHERMWARM) {	// Start red fill after blue finishes
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
			}
		}
	}

	if (PORTE.INTFLAGS & PIN1_bm) {		// Red dewar thermistor changed state
		PORTE.INTFLAGS |= PIN1_bm;		// Clear the interrupt flag
		CLOSEVALVE(REDVALVE);
		status.opentime_RED = 0;
		if (!REDTHERMWARM) {
			status.maxopen_RED = FALSE;	// Clear a max_opentime limit
		}
	}

	if (PORTE.INTFLAGS & PIN2_bm) {		// Buffer dewar thermistor changed state
		PORTE.INTFLAGS |= PIN2_bm;
		CLOSEVALVE(BUFVALVE);
		status.opentime_BUF = 0;
		status.maxopen_BUF = FALSE;
		_delay_ms(250);					// Wait a bit
		CLOSEVALVE(SUPVALVE);
		status.opentime_SUP = 0;
		if (!BUFTHERMWARM) {
			status.maxopen_BUF = FALSE;	// Clear a BUFMAXOPEN time
		}
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
	VREF.CTRLA |= VREF_ADC0REFSEL_4V34_gc;	// Use 4.3V
	ADC0_CTRLC |= ADC_REFSEL_INTREF_gc;		// Use internal vref
//	VREF.CTRLA |= VREF_ADC0REFSEL_2V5_gc;	// Use 2.5V
	ADC0_CTRLC |= ADC_SAMPCAP_bm;			// Reduce sampling capacitance
	ADC0_MUXPOS = ADC_MUXPOS0_bm;			// PD0
	ADC0.CTRLA |= ADC_ENABLE_bm;			// Enable ADC
	ADC0_COMMAND |= ADC_STCONV_bm;		// Start ADC conversion and throw away
	while (ADC0_COMMAND & ADC_STCONV_bm) {
		asm("nop");
	}
	status.pressure = ADC0_RES;

}