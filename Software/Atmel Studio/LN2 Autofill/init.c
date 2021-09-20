#include "globals.h"
#include "buttons.h"
#include "eeprom.h"
#include "oled.h"
#include "rtc.h"
#include "twi.h"
#include "valves.h"
#include "encoder.h"
#include "pressure.h"
#include "usart.h"
#include "init.h"

void init_PORTS(void);
void init_STATUS(void);

void init(void)
{

	minute = FALSE;
	seconds = 0;
	init_PORTS();
	init_VALVES();
	init_USART();
	init_BUTTONS();
	init_ENCODER();
	init_TWI();
	init_RTC(511);
	init_ADC();
	init_STATUS();
	init_OLED(0);
	update_VERSION();
	display(SCRVERSION);

}

void init_STATUS()
{

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
/*
	PORTA.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN3CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN4CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN5CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN6CTRL = PORT_PULLUPEN_bm;
	PORTA.PIN7CTRL = PORT_PULLUPEN_bm;

	PORTB.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN3CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN4CTRL = PORT_PULLUPEN_bm;
	PORTB.PIN5CTRL = PORT_PULLUPEN_bm;

	PORTC.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN3CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN4CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN5CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN6CTRL = PORT_PULLUPEN_bm;
	PORTC.PIN7CTRL = PORT_PULLUPEN_bm;
*/
	// ADC pins are on port D
	PORTD.PIN0CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN1CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN2CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN3CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN6CTRL = PORT_ISC_INPUT_DISABLE_gc;
	PORTD.PIN7CTRL = PORT_ISC_INPUT_DISABLE_gc;
/*
	PORTE.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTE.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTE.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTE.PIN3CTRL = PORT_PULLUPEN_bm;

	PORTF.PIN0CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN2CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN3CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN4CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN5CTRL = PORT_PULLUPEN_bm;
	PORTF.PIN6CTRL = PORT_PULLUPEN_bm;
*/
}