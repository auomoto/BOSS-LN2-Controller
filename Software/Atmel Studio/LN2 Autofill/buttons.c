/*----------------------------------------------------------------------
PUSHBUTTONS
	Four pushbutton switches with LED indicators control the solenoid
	valves.

	PB2 is SW1 Blue dewar vent
	PB3 is SW2 Red dewar vent
	PB0 is SW3 Buffer dewar vent
	PB1 is SW4 Buffer dewar supply inlet
----------------------------------------------------------------------*/

#include "globals.h"
#include "buttons.h"
#include "valves.h"
#include "encoder.h"	// For SCRVALVES
#include "eeprom.h"		// RED/BLUENABLE, FILLINTERVAL, MAXOPENTIME

volatile uint8_t button_pushed;

/*----------------------------------------------------------------------
BUTTONS
	Pushing a front panel button generates an interrupt. The
	button_pushed variable returns the button name and the corresponding
	valve or valves are acted upon, toggling the state of the valve.
----------------------------------------------------------------------*/
void handle_button(void)
{
	switch (button_pushed) {
		case BLUEBUTTON:
			if (BLUVALVEOPEN) {
				CLOSEVALVE(BLUVALVE);
			} else if (BLUENABLED) {
				OPENVALVE(BLUVALVE);
				status.opentime_BLU = 0;
				status.maxopen_BLU = FALSE;
			}
			break;

		case REDBUTTON:
			if (REDVALVEOPEN) {
				CLOSEVALVE(REDVALVE);
			} else if (REDENABLED) {
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
				status.maxopen_RED = FALSE;
			}
			break;

		case BUFFERBUTTON:
			if (BUFVALVEOPEN) {
				CLOSEVALVE(BUFVALVE);
			} else {
				OPENVALVE(BUFVALVE);
				status.opentime_BUF = 0;
				status.maxopen_BUF = FALSE;
			}
			break;

		case SUPPLYBUTTON:
			if (SUPVALVEOPEN) {
				CLOSEVALVE(SUPVALVE);
			} else {
				OPENVALVE(SUPVALVE);
				status.opentime_SUP = 0;
			}
			break;

		default:
			break;
	}

	button_pushed = FALSE;

	if (screen_value == SCRVALVES) {
		display(SCRVALVES);
	}
}

/*----------------------------------------------------------------------
INITIALIZE BUTTON PINS
	Set up the pins for interrupts.
----------------------------------------------------------------------*/
void init_BUTTONS(void)
{
	// Four solenoid valve control switches
	PORTB.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW1
	PORTB.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW2
	PORTB.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW3
	PORTB.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW4
}

/*----------------------------------------------------------------------
Interrupt routine for the four pushbuttons. The interrupt flag is
cleared and the button_pushed value is set.
----------------------------------------------------------------------*/
ISR(PORTB_PORT_vect)
{
	if (PORTB.INTFLAGS & PIN2_bm) {			// Blue pushbutton
		PORTB.INTFLAGS = PIN2_bm;			// Clear the interrupt flag
		button_pushed = BLUEBUTTON;
	}
	else if (PORTB.INTFLAGS & PIN3_bm) {	// Red pushbutton
		PORTB.INTFLAGS = PIN3_bm;			// Clear the interrupt flag
		button_pushed = REDBUTTON;
	}
	else if (PORTB.INTFLAGS & PIN0_bm) {	// Buffer pushbutton
		PORTB.INTFLAGS = PIN0_bm;			// Clear the interrupt flag
		button_pushed = BUFFERBUTTON;
	}
	else if (PORTB.INTFLAGS & PIN1_bm) {	// Supply pushbutton
		PORTB.INTFLAGS = PIN1_bm;			// Clear the interrupt flag
		button_pushed = SUPPLYBUTTON;
	}
}
