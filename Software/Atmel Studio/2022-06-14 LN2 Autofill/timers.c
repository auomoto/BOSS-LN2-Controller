#include "globals.h"
#include "timers.h"

volatile uint16_t ticks_TCB0;

void start_TCB0(uint16_t msPeriod)
{

	ticks_TCB0 = 0;
	TCB0.CCMP = msPeriod * (uint16_t) (F_CPU/1000UL);	// Check for overflows; msPeriod=19ms is max for 3.33MHz
	TCB0.INTCTRL = TCB_CAPT_bm;				// Interrupt at TOP
	//	TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm;	// Start the clock
	TCB0.CTRLA = TCB_ENABLE_bm;				// Start the clock

}

void stop_TCB0(void)
{
	
	TCB0.CTRLA = 0;

}

ISR(TCB0_INT_vect)
{

	TCB0_INTFLAGS = TCB_CAPT_bm;	// Clear interrupt flag
	ticks_TCB0++;

}

/*----------------------------------------------------------------------
INITIALIZE Timer-counter TCB2

At startup, the TCB is in periodic interrupt mode. To enable this
1. Write a TOP value to TCBn.CCMP
2a. Enable the counter by writing a '1' to the ENABLE bit in TCBn.CTRLA
2b. Set the CLKSEL prescaler bit field in TCBn.CTRLA
3. A CAPT interrupt is genterated when TOP is reached. Enable CAPT
   interrupts in the TCB2.INTCTRL register.

This counter generates an oscillator signal for the ON Semiconductor
MC14490DWG switch debouncer. The original setup had the oscillator
running too slowly for the rotary encoder so I set this up as a stopgap
until I could get the correct capacitor.

So nTicks = F_CPU/(2*F_osc) so if you want
F_osc = 1500 Hz on a 3333333 MHz system clock with 2X prescaler you get
nTicks = 3333333/(2*prescaler*1500)+1 = 0x022C (the +1 takes care of
the integer arithmetic that will truncate most of the time)

By measurement, 0xFFFF gets 12.69 Hz
----------------------------------------------------------------------*/
void init_TCB2(void)
{

	PORTC_DIR |= PIN0_bm;					// PIN PC0 has haywire
	PORTC_OUT |= PIN0_bm;					// to debouncer Osc-in
	TCB2.CCMP = F_CPU/(4 * TCB2FREQ) + 1;
	TCB2.CTRLA |= TCB_ENABLE_bm;			// Enable the timer
	TCB2.CTRLA |= TCB_CLKSEL_CLKDIV2_gc;	// 2X divider
	TCB2.INTCTRL |= TCB_CAPT_bm;			// Enable interrupts
	
}

/*----------------------------------------------------------------------
Generate a square wave by toggling the state of PC0
----------------------------------------------------------------------*/
ISR(TCB2_INT_vect)
{

	TCB2.INTFLAGS = TCB_CAPT_bm;
	PORTC.OUTTGL |= PIN0_bm;

}
