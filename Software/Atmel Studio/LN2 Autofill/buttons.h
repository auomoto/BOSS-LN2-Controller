#ifndef BUTTONS_H
#define BUTTONS_H

#define BLUEBUTTON		1	// None of these should be 0
#define REDBUTTON		2
#define BUFFERBUTTON	3
#define SUPPLYBUTTON	4

void disp_coldtherm(void);
void handle_button(void);
void init_BUTTONS(void);

extern volatile uint8_t button_pushed;

#endif