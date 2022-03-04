/*----------------------------------------------------------------------
PUSHBUTTONS ON THE FRONT PANEL
	Four momentary contact pushbutton switches with LED indicators
	control the solenoid valves. Each button push toggles the valve
	state.

	PB2 is SW1 Blue dewar vent
	PB3 is SW2 Red dewar vent
	PB0 is SW3 Buffer dewar vent
	PB1 is SW4 Buffer dewar supply inlet
----------------------------------------------------------------------*/

#include "globals.h"
#include "buttons.h"
#include "valves.h"
#include "oled.h"
#include "encoder.h"	// For SCRVALVES
#include "eeprom.h"		// RED/BLUENABLE, FILLINTERVAL, MAXOPENTIME

volatile uint8_t button_pushed;

void disp_coldtherm(void)
{
	clear_OLED(0);
	writestr_OLED(0, "Sensor cold or", 1);
	writestr_OLED(0, "cable unplugged", 2);
}

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
			if (BLUVALVEOPEN) {				// See valves.h
				CLOSEVALVE(BLUVALVE);
			} else if (BLUENABLED && BLUTHERMWARM) {
				OPENVALVE(BLUVALVE);
				status.opentime_BLU = 0;
				status.maxopen_BLU = FALSE;
			} else if (BLUENABLED && !BLUTHERMWARM) {
				disp_coldtherm();
				_delay_ms(2000);
				display(screen_value);
			}
			break;

		case REDBUTTON:
			if (REDVALVEOPEN) {				// See valves.h
				CLOSEVALVE(REDVALVE);
			} else if (REDENABLED && REDTHERMWARM) {
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
				status.maxopen_RED = FALSE;
			} else if (REDENABLED && !REDTHERMWARM) {
				disp_coldtherm();
				_delay_ms(2000);
				display(screen_value);
			}
			break;

		case BUFFERBUTTON:
			if (BUFVALVEOPEN) {				// See valves.h
				CLOSEVALVE(BUFVALVE);
			} else if (BUFTHERMWARM) {
				OPENVALVE(BUFVALVE);
				status.opentime_BUF = 0;
				status.maxopen_BUF = FALSE;
			} else if (!BUFTHERMWARM) {
				disp_coldtherm();
				_delay_ms(2000);
				display(screen_value);
			}
			break;

		case SUPPLYBUTTON:
			if (SUPVALVEOPEN) {				// See valves.h
				CLOSEVALVE(SUPVALVE);
			} else {
// NEW
				status.supply_button_pushed = TRUE;	// Button was pushed; cleared in handle_ticks()
				status.supply_button_time = 0;		// # secs button has been held down
// NEW
//OLD:
//				OPENVALVE(SUPVALVE);
//				status.opentime_SUP = 0;
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

void clear_BUTTONS(void)
{

	PORTB.INTFLAGS = 0x0F;	// Clear spurious interrupts from debouncer?

}

/*----------------------------------------------------------------------
INITIALIZE BUTTON PINS
	Set up the pins for interrupts.
----------------------------------------------------------------------*/
void init_BUTTONS(void)
{
	// Four solenoid valve control buttons
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
