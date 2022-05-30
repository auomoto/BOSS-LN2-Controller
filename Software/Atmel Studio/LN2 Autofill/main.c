#include "globals.h"
#include "init.h"
#include "buttons.h"
#include "encoder.h"
#include "rtc.h"
#include "usart.h"

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
		}
		if (recv0_buf.done) {	// Serial communication received
writestr_OLED(0, "got a CR", 1);
			handle_serial();	// See usart.c
		}
	}
}

