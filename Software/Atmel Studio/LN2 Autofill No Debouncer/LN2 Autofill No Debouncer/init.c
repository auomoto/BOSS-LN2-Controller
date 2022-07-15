#include "globals.h"
#include "init.h"

void init_PORTS(void);
void init_STATUS(void);

void init(void)
{

	hour_flag = FALSE;
	minute_flag = FALSE;
	seconds = 0;
	init_PORTS();
	init_USART();
	init_BUTTONS();
	init_ENCODER();
	init_TWI();
	init_RTC();
	init_ADC();
	init_STATUS();
	init_OLED(0);
	update_VERSION();
	display(SCRVERSION);

}


void init_STATUS()
{

	status.next_buftank_fill = BIGFILLINTERVAL;
	status.next_fill = FILLINTERVAL;
	status.opentime_BLU = 0;
	status.opentime_RED = 0;
	status.opentime_BUF = 0;
	status.opentime_SUP = 0;
	status.maxopen_BLU = FALSE;		// TRUE when open too long
	status.maxopen_RED = FALSE;
	status.maxopen_BUF = FALSE;
	status.overfill_BUF = 0;
	status.overfill_BLU = 0;
	status.overfill_RED = 0;
	status.pressure = read_PRESSURE();

}


void init_PORTS(void)
{

	// Unused pins
	PORTA.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTA.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTC.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTF.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTF.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTF.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTF.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTF.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTF.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTF.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;

}
