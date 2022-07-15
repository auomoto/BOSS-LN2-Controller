#ifndef BUTTONS_H
#define BUTTONS_H

#include "valves.h"
#include "oled.h"
#include "encoder.h"	// For SCRVALVES
#include "eeprom.h"		// RED/BLUENABLE, FILLINTERVAL, MAXOPENTIME

#define BLUEBUTTON		1	// None of these should be 0
#define REDBUTTON		2
#define BUFFERBUTTON	3
#define SUPPLYBUTTON	4

void disp_coldtherm(void);
void handle_button(void);
void clear_BUTTONS(void);
void init_BUTTONS(void);
uint8_t scan_buttons(void);

extern volatile uint8_t button_pushed;

#endif