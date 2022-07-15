#ifndef BUTTONS_H
#define BUTTONS_H

#include "oled.h"
#include "encoder.h"	// For SCRVALVES
#include "eeprom.h"		// RED/BLUENABLE, FILLINTERVAL, MAXOPENTIME

#define BLUEBUTTON		1	// None of these should be 0
#define REDBUTTON		2
#define BUFFERBUTTON	3
#define SUPPLYBUTTON	4

#define BLUEBUTTONCLOSED	(~PORTB.IN & PIN0_bm)
#define REDBUTTONCLOSED		(~PORTB.IN & PIN1_bm)
#define BUFBUTTONCLOSED		(~PORTB.IN & PIN2_bm)
#define SUPBUTTONCLOSED		(~PORTB.IN & PIN3_bm)

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
#define BUFMAXOPEN		20						// In minutes
#define OVERFILLBUF		5						// In seconds
#define OVERFILLBLU		3						// In seconds
#define OVERFILLRED		3						// In seconds

// Thermistor pins
#define BLUTHERMWARM	(PORTE.IN & PIN0_bm)	// TRUE if the thermistor is warm
#define REDTHERMWARM	(PORTE.IN & PIN1_bm)
#define BUFTHERMWARM	(PORTE.IN & PIN2_bm)

void disp_coldtherm(void);
void handle_button(void);
void clear_BUTTONS(void);
void init_BUTTONS(void);
uint8_t scan_buttons(void);

extern volatile uint8_t button_pushed;

#endif