#include "globals.h"
#include "encoder.h"

volatile uint8_t encoder_sensed;
volatile int8_t encoder_change;
uint8_t encoder_value, screen_value;

/*----------------------------------------------------------------------
DISPLAY
	Writes the top level OLED display screens. Each screen has a value
	defined in encoder.h. Pushing the encoder knob for some screens
	takes you to a lower level for more options.
----------------------------------------------------------------------*/
void display(uint8_t value)
{
	char line1[21], line2[21];

	switch (value) {
		case SCRVERSION:						// See encoder.h
			strcpy(line1, "  LN2 Autofill");
			get_VERSION(status.version);
			strcpy(line2, "   ");
			strcat(line2, status.version);
			screen_value = SCRVERSION;
			break;
	
		case SCRVALVES:							// encoder.h
			strcpy(line1, "SUP BUF RED BLU");
			if (SUPVALVEOPEN) {					// valves.h
				strcpy(line2, " O  ");
			} else {
				strcpy(line2, " C  ");
			}
			if (BUFVALVEOPEN) {					// valves.h
				strcat(line2, " O  ");
			} else if (status.maxopen_BUF) {
				strcat(line2, " T  ");
			} else {
				strcat(line2, " C  ");
			}
			if (!REDENABLED) {					// eeprom.h
				strcat(line2, " X  ");
			} else if (REDVALVEOPEN) {			// valves.h
				strcat(line2, " O  ");
			} else if (status.maxopen_RED) {
				strcat(line2, " T  ");
			} else {
				strcat(line2, " C  ");
			}
			if (!BLUENABLED) {
				strcat(line2, " X");
			} else if (BLUVALVEOPEN) {			// valves.h
				strcat(line2, " O");
			} else if (status.maxopen_BLU) {
				strcat(line2, " T");
			} else {
				strcat(line2, " C");
			}
			screen_value = SCRVALVES;			// encoder.h
			break;

		case SCRNEXTFILL:
			strcpy(line1, "Next fill in");
			if (status.next_fill < 2) {
				sprintf(line2, "%d minute", status.next_fill);
			} else {
				sprintf(line2, "%d minutes", status.next_fill);
			}
			screen_value = SCRNEXTFILL;			// encoder.h
			break;

		case SCRFILLINT:
			strcpy(line1, "Fill interval");
			if (FILLINTERVAL == 1) {			// eeprom.h
				sprintf(line2, "%d minute", FILLINTERVAL);
			} else {
				sprintf(line2, "%d minutes", FILLINTERVAL);
			}
			screen_value = SCRFILLINT;			// encoder.h
			break;
			
		case SCRMAXOPENTIME:					// encoder.h
			strcpy(line1, "Max open time");
			if (MAXOPENTIME < 2) {				// eeprom.h
				sprintf(line2, "%d minute", MAXOPENTIME);
			} else {
				sprintf(line2, "%d minutes", MAXOPENTIME);
			}
			screen_value = SCRMAXOPENTIME;		// encoder.h
			break;

		case SCRPRESSURE:						// encoder.h
			strcpy(line1, "LN2 pressure");
			sprintf(line2, "%d kPa", status.pressure);
			screen_value = SCRPRESSURE;
			break;

		case SCRDISABLEBLU:						// encoder.h
			if (BLUENABLED) {					// eeprom.h
				strcpy(line1, "BLUE is ENABLED");
				strcpy(line2, "Push to disable");
			} else {
				strcpy(line1, "Blue is DISABLED");
				strcpy(line2, "Push to enable");
			}
			screen_value = SCRDISABLEBLU;		// encoder.h
			break;

		case SCRDISABLERED:						// encoder.h
			if (REDENABLED) {					// eeprom.h
				strcpy(line1, "RED is ENABLED");
				strcpy(line2, "Push to disable");
			} else {
				strcpy(line1, "RED is DISABLED");
				strcpy(line2, "Push to enable");
			}
			screen_value = SCRDISABLERED;		// encoder.h
			break;

		default:
			strcpy(line1, "default");
			strcpy(line2, "overrun");
			screen_value = SCRVERSION;			// encoder.h
			break;
	}
	writestr_OLED(0, line1, 1);
	writestr_OLED(0, line2, 2);
}

/*----------------------------------------------------------------------
DISABLE or ENABLE BLUE FILLS
	Pushing the knob toggles the ENABLE/DISABLE state by writing the
	state to eeprom.
----------------------------------------------------------------------*/
void change_DISABLEBLU(void)
{
	eeprom_update_byte((uint8_t *)BLUENABLEADDR, !BLUENABLED);
	encoder_sensed = FALSE;
	display(SCRDISABLEBLU);						// encoder.h
}

/*----------------------------------------------------------------------
DISABLE or ENABLE RED FILLS
	Pushing the button toggles the ENABLE/DISABLE state by writing the
	state to eeprom.
----------------------------------------------------------------------*/
void change_DISABLERED(void)
{
	eeprom_update_byte((uint8_t *)REDENABLEADDR, !REDENABLED);
	encoder_sensed = FALSE;
	display(SCRDISABLERED);						// encoder.h
}

/*----------------------------------------------------------------------
CHANGE FILL INTERVAL
	Pushing the knob lets you change the fill interval in minutes. Push
	the knob again to save the value.
----------------------------------------------------------------------*/
void change_FILLINT(void)
{

	char strbuf[21];

	PORTC.INTFLAGS = PIN1_bm;					// Clear interrupt flag
	encoder_sensed = FALSE;
	encoder_value = FILLINTERVAL;				// eeprom.h
	if (encoder_value == 1) {
		sprintf(strbuf, "%d minute", encoder_value);
	} else {
		sprintf(strbuf, "%d minutes", encoder_value);
	}
	writestr_OLED(0, "Push to set intv", 1);
	writestr_OLED(0, strbuf, 2);

	while (encoder_sensed != ENCODERBUTTON) {	// encoder.h
		if (encoder_sensed == ENCODERA) {
			if ((ENCODERACLOSED && !ENCODERBCLOSED) || (!ENCODERACLOSED && ENCODERBCLOSED)) {
				encoder_value++;
				encoder_change = 1;
			} else {
				encoder_value--;
				encoder_change = -1;
			}
			if (encoder_value <= 1) {
				encoder_value = 2;
			} else if (encoder_value == 255) {
				encoder_value = 254;
			}
			sprintf(strbuf, "%d minutes", encoder_value);
			writestr_OLED(0, "Push to set intv", 1);
			writestr_OLED(0, strbuf, 2);
			PORTC.INTFLAGS = PIN2_bm;			// Clear interrupt flag
			encoder_sensed = FALSE;
		}
	}
	PORTC.INTFLAGS = PIN1_bm;			// Clear interrupt flag
	encoder_sensed = FALSE;
	eeprom_update_byte((uint8_t *)FILLINTADDR, encoder_value);
	if (MAXOPENTIME >= FILLINTERVAL) {
		eeprom_update_byte((uint8_t *)MAXOPENADDR, (FILLINTERVAL - 1));
	}

	display(SCRFILLINT);

}

/*----------------------------------------------------------------------
CHANGE MAXIMUM OPEN TIME
	Pushing and then rotating the knob lets you change the maximum
	open time in minutes. Push the knob again to save the new value.
----------------------------------------------------------------------*/
void change_MAXOPENTIME(void)
{
	char strbuf[21];

	PORTC.INTFLAGS = PIN1_bm;				// Clear interrupt flag
	encoder_sensed = FALSE;
	encoder_value = MAXOPENTIME;			// eeprom.h
	if (encoder_value == 1) {
		sprintf(strbuf, "%d minute", encoder_value);
	} else {
		sprintf(strbuf, "%d minutes", encoder_value);
	}
	writestr_OLED(0, "Push to set maxo", 1);
	writestr_OLED(0, strbuf, 2);

	while (encoder_sensed != ENCODERBUTTON) {
		if (encoder_sensed == ENCODERA) {
			if ((ENCODERACLOSED && !ENCODERBCLOSED) || (!ENCODERACLOSED && ENCODERBCLOSED)) {
				encoder_value++;
				encoder_change = 1;
			} else {
				encoder_value--;
				encoder_change = -1;
			}
			if (encoder_value <= 0) {
				encoder_value = 1;
			} else if (encoder_value == 255) {
				encoder_value = 254;
			}

			if (encoder_value >= FILLINTERVAL) {
				encoder_value = FILLINTERVAL - 1;
			}
			if (encoder_value == 1) {
				sprintf(strbuf, "%d minute", encoder_value);
			} else {
				sprintf(strbuf, "%d minutes", encoder_value);
			}
			writestr_OLED(0, "Push to set maxo", 1);
			writestr_OLED(0, strbuf, 2);			
			PORTC.INTFLAGS = PIN2_bm;		// Clear interrupt flag
			encoder_sensed = FALSE;
		}
	}
	PORTC.INTFLAGS = PIN1_bm;				// Clear interrupt flag
	encoder_sensed = FALSE;
	eeprom_update_byte((uint8_t *)MAXOPENADDR, encoder_value);
	display(SCRMAXOPENTIME);

}

/*----------------------------------------------------------------------
HANDLE ENCODER INPUT
	A signal from the encoder, either a rotation or a pushbutton,
	generates an interrupt. The interupt routine assigns a value to
	the encoder_sensed variable that is either for rotation (ENCODERA)
	or pushbutton (ENCODERBUTTON).

	The main() routine waits for either of these non-zero values after
	which it calls this.
----------------------------------------------------------------------*/
void handle_encoder(void)
{

	if (display_off) {
		display(screen_value);
	} else if (encoder_sensed == ENCODERA) {
		if ((ENCODERACLOSED && !ENCODERBCLOSED) || (!ENCODERACLOSED && ENCODERBCLOSED)) {
			encoder_value++;
			encoder_change = 1;
		} else {
			encoder_value--;
			encoder_change = -1;
		}
		screen_value = (screen_value + encoder_change);
		if (screen_value < 0) {
			screen_value = MAXSCREENS-1;
		} else {
			screen_value %= MAXSCREENS;
		}
		PORTC.INTFLAGS = PIN2_bm;			// Clear interrupt flag
		encoder_sensed = FALSE;
		display(screen_value);
	} else if (encoder_sensed == ENCODERBUTTON) {
		switch (screen_value) {
			case SCRNEXTFILL:				// Immediate start fill
				start_FILL();
				break;
			case SCRFILLINT:				// Change fill interval
				change_FILLINT();
				break;
			case SCRMAXOPENTIME:			// Change max open time
				change_MAXOPENTIME();
				break;
			case SCRDISABLEBLU:				// Disable blue fills
				change_DISABLEBLU();
				break;
			case SCRDISABLERED:				// Disable red fills
				change_DISABLERED();
				break;
			default:
				break;
		}
		PORTC.INTFLAGS = PIN1_bm;			// Clear interrupt flag
		encoder_sensed = FALSE;
	}
}

/*----------------------------------------------------------------------
INITIALIZE ENCODER

	PC2 is ENCA encoder A signal
	PC3 is ENCB encoder B signal (does not cause an interrupt)
	PC1 is PUSH encoder pushbutton
----------------------------------------------------------------------*/
void init_ENCODER(void)
{
	
	// Rotary Encoder
//	PORTC.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// ENCA
//	PORTC.PIN2CTRL = PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;	// ENCA
	PORTC.PIN2CTRL = PORT_ISC_BOTHEDGES_gc;	// ENCA
//	PORTC.PIN3CTRL = PORT_PULLUPEN_bm;							// ENCB
//	PORTC.PIN1CTRL = PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;	// PUSH
	PORTC.PIN1CTRL = PORT_ISC_FALLING_gc;	// PUSH
}

/*----------------------------------------------------------------------
START A FILL
	Pushing the encoder knob while looking at the "Next fill in" screen
	starts an immediate fill cycle. This is not obvious from the
	display screen, which doesn't prompt for a button push.
----------------------------------------------------------------------*/
void start_FILL(void)
{
	if (BLUENABLED) {
		if (!BLUVALVEOPEN && BLUTHERMWARM) {
			OPENVALVE(BLUVALVE);
			status.opentime_BLU = 0;
			status.maxopen_BLU = FALSE;
		}
	}
	if (REDENABLED) {
		if (!REDVALVEOPEN && REDTHERMWARM) {
			OPENVALVE(REDVALVE);
			status.opentime_RED = 0;
			status.maxopen_RED = FALSE;
		}
	}
	if (screen_value == SCRVALVES) {
		display(SCRVALVES);
	}
	status.next_fill = FILLINTERVAL;
}

/*----------------------------------------------------------------------
Interrupt routine for encoder rotation or pushbutton.
	PC1 is the encoder pushbutton
	PC2 is ENCODER A signal
	PC3 is ENCODER B signal
----------------------------------------------------------------------*/
ISR(PORTC_PORT_vect)
{
	if (PORTC.INTFLAGS & PIN1_bm) {			// Encoder pushbutton
//		PORTC.INTFLAGS = PIN1_bm;			// Clear interrupt flag
		encoder_sensed = ENCODERBUTTON;
	} else if (PORTC.INTFLAGS & PIN2_bm) {	// Encoder rotary
//		PORTC.INTFLAGS = PIN2_bm;			// Clear interrupt flag
/*
		if (PORTC.IN & PIN3_bm) {			// Check ENCB state
			encoder_value++;
			encoder_change = 1;
		} else {
			encoder_value--;
			encoder_change = -1;
		}
*/
		encoder_sensed = ENCODERA;
	}
}
