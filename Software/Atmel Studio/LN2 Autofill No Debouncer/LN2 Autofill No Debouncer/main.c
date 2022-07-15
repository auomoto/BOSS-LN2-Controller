/*
 * LN2 Autofill No Debouncer.c
 *
 * Created: 7/15/2022 10:09:16 AM
 * Author : alanu
 */ 

#include "globals.h"
#include "init.h"
#include <avr/io.h>

int main(void)
{

	init();
	sei();
	for (;;) {
		if (button_pushed) {	// Front panel button pushed
			handle_button();	// See buttons.c
		}
		if (encoder_sensed) {	// Front panel encoder changed
			handle_encoder();	// See encoder.c
		}
		if (hour_flag) {		// Things to do once an hour
			handle_hour();		// See rtc.c
		}
		if (minute_flag) {		// Things to do once a minute
			handle_minute();	// See rtc.c
		}
		if (tick) {				// Things to do once a second
			handle_ticks();		// See rtc.c
//			button_pushed = scan_buttons();
		}
		if (recv0_buf.done) {	// Serial communication received
			handle_serial();	// See usart.c
		}
	}

}

