/*----------------------------------------------------------------------
PUSHBUTTONS ON THE FRONT PANEL
	Four momentary contact pushbutton switches with LED indicators
	control the solenoid valves. Each button push toggles the valve
	state.

	PB0 is SW1 Blue dewar vent
	PB1 is SW2 Red dewar vent
	PB2 is SW3 Buffer dewar vent
	PB3 is SW4 Buffer dewar supply inlet
----------------------------------------------------------------------*/

#include "globals.h"
#include "buttons.h"

volatile uint8_t button_pushed;


/*----------------------------------------------------------------------
	If the thermistor is cold (high resistance) or unplugged, then this
	message is displayed on the screen.
----------------------------------------------------------------------*/
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

	_delay_ms(10);
	switch (button_pushed) {
		case BLUEBUTTON:
			if (!BLUEBUTTONCLOSED) {
				return;
			}
			if (BLUVALVEOPEN) {				// See valves.h
				CLOSEVALVE(BLUVALVE);
			} else if (BLUENABLED && BLUTHERMWARM) {
				OPENVALVE(BLUVALVE);
				status.opentime_BLU = 0;
				status.maxopen_BLU = FALSE;
			}
			if (BLUENABLED && !BLUTHERMWARM) {
				disp_coldtherm();
			} else if ((screen_value == SCRVALVES)) {
				display(SCRVALVES);
			}
			break;

		case REDBUTTON:
			if (!REDBUTTONCLOSED) {
				return;
			}
			if (REDVALVEOPEN) {				// See valves.h
				CLOSEVALVE(REDVALVE);
			} else if (REDENABLED && REDTHERMWARM) {
				OPENVALVE(REDVALVE);
				status.opentime_RED = 0;
				status.maxopen_RED = FALSE;
			}
			if (REDENABLED && !REDTHERMWARM) {
				disp_coldtherm();
			} else if ((screen_value == SCRVALVES)) {
				display(SCRVALVES);
			}
			break;

		case BUFFERBUTTON:
			if (!BUFBUTTONCLOSED) {
				return;
			}
			if (BUFVALVEOPEN) {				// See valves.h
				CLOSEVALVE(BUFVALVE);
			} else if (BUFTHERMWARM) {
				OPENVALVE(BUFVALVE);
				status.opentime_BUF = 0;
				status.maxopen_BUF = FALSE;
			}
			if (!BUFTHERMWARM) {
				disp_coldtherm();
			} else if ((screen_value == SCRVALVES)) {
				display(SCRVALVES);
			}
			break;

		case SUPPLYBUTTON:
			if (!SUPBUTTONCLOSED) {
				return;
			}
			if (SUPVALVEOPEN) {						// See buttons.h
				CLOSEVALVE(SUPVALVE);
			} else {
				_delay_ms(2000);
				if (SUPBUTTONCLOSED) {
					OPENVALVE(SUPVALVE);
					status.opentime_SUP = 0;
					if ((screen_value == SCRVALVES)) {
						display(SCRVALVES);
					}
				}
			}
			if ((screen_value == SCRVALVES)) {
				display(SCRVALVES);
			}
			break;

		default:
			break;
	}

	button_pushed = FALSE;

}

void init_BUTTONS(void)
{
	// Four solenoid valve control buttons
	PORTB.DIRCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
	PORTB.PIN0CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW1
	PORTB.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW2
	PORTB.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW3
	PORTB.PIN3CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// SW4

	// Valve ports set to output
	PORTC.OUTCLR = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;
	PORTC.DIRSET = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;

}

/*----------------------------------------------------------------------
Interrupt routine for the four pushbuttons. The interrupt flag is
cleared and the button_pushed value is set.
----------------------------------------------------------------------*/

ISR(PORTB_PORT_vect)
{

	if (PORTB.INTFLAGS & PIN0_bm) {			// Blue pushbutton
		PORTB.INTFLAGS = PIN0_bm;			// Clear the interrupt flag
		button_pushed = BLUEBUTTON;
	} else if (PORTB.INTFLAGS & PIN1_bm) {	// Red pushbutton
		PORTB.INTFLAGS = PIN1_bm;			// Clear the interrupt flag
		button_pushed = REDBUTTON;
	} else if (PORTB.INTFLAGS & PIN2_bm) {	// Buffer pushbutton
		PORTB.INTFLAGS = PIN2_bm;			// Clear the interrupt flag
		button_pushed = BUFFERBUTTON;
	} else if (PORTB.INTFLAGS & PIN3_bm) {	// Supply pushbutton
		PORTB.INTFLAGS = PIN3_bm;			// Clear the interrupt flag
		button_pushed = SUPPLYBUTTON;
	}

}

/*
uint8_t scan_buttons(void)
{

	uint8_t value;

	if (~PORTB.IN & PIN2_bm) {
		value = BLUEBUTTON;
	} else if (~PORTB.IN & PIN3_bm) {
		value = REDBUTTON;
	} else if (~PORTB.IN & PIN0_bm) {
		value = BUFFERBUTTON;
	} else if (~PORTB.IN & PIN1_bm) {
		value = SUPPLYBUTTON;
		if (status.supply_button_pushed) {		// Button was already pushed
			status.supply_button_time++;		// # secs button has been held down
		} else {
			status.supply_button_pushed = TRUE;
			status.supply_button_time = 0;
		}
	} else {
		value = FALSE;
	}

	return(value);

}
*/