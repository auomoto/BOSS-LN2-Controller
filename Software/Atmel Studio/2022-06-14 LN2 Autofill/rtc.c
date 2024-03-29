#include "globals.h"
#include "rtc.h"

/*----------------------------------------------------------------------
THINGS TO DO EVERY HOUR
----------------------------------------------------------------------*/
void handle_hour(void)
{

	hour_flag = FALSE;

	if (BIGTANK) {
		status.next_buftank_fill--;
		if (status.next_buftank_fill == 0) {
			if (BUFTHERMWARM) {
				OPENVALVE(BUFVALVE);
				status.opentime_BUF = 0;
				OPENVALVE(SUPVALVE);
				status.opentime_SUP = 0;
				status.next_buftank_fill = BIGFILLINTERVAL;
			}
		}
	}
}


/*----------------------------------------------------------------------
THINGS TO DO EVERY MINUTE
----------------------------------------------------------------------*/
void handle_minute(void)
{

	if (++minutes >= 60) {
		hour_flag = TRUE;
		minutes = 0;
	}

	minute_flag = FALSE;

	if (BLUVALVEOPEN) {
		status.opentime_BLU++;
		if (status.opentime_BLU >= MAXOPENTIME) {	// MAX OPEN ERROR
			CLOSEVALVE(BLUVALVE);
			status.maxopen_BLU = TRUE;
		}
	}

	if (REDVALVEOPEN) {
		status.opentime_RED++;
		if (status.opentime_RED >= MAXOPENTIME) {	// MAX OPEN ERROR
			CLOSEVALVE(REDVALVE);
			status.maxopen_RED = TRUE;
		}
	}

	if (BUFVALVEOPEN) {
		status.opentime_BUF++;
		if (status.opentime_BUF >= BUFMAXOPEN) {	// valves.h MAX OPEN ERROR
			CLOSEVALVE(BUFVALVE);
			CLOSEVALVE(SUPVALVE);
			status.maxopen_BUF = TRUE;
			status.next_buftank_fill = BIGFILLINTERVAL;
		}
	}

	if (SUPVALVEOPEN) {
		status.opentime_SUP++;				// Increment supply dewar valve time
	}

	status.next_fill--;
	if (status.next_fill == 0) {
		start_FILL();						// See encoder.c
	}
}

/*----------------------------------------------------------------------
THINGS TO DO EVERY SECOND
----------------------------------------------------------------------*/
void handle_ticks(void)
{

	tick = FALSE;

	if (seconds >= 60) {		// seconds incremented in ISR
		minute_flag = TRUE;
		seconds = 0;
	}

	status.pressure = read_PRESSURE();				// LN2 pressure

	if (BLUVALVEOPEN && !BLUTHERMWARM) {			// Valve open, therm cold
		if (status.overfill_BLU >= OVERFILLBLU) {	// valves.h
			CLOSEVALVE(BLUVALVE);
			status.overfill_BLU = 0;
		} else {
			status.overfill_BLU++;
		}
		if ((screen_value == SCRVALVES) && timerOLED) {
			display(SCRVALVES);
		}
	}

	if (REDVALVEOPEN && !REDTHERMWARM) {			// Valve open, therm cold
		if (status.overfill_RED >= OVERFILLRED) {	// valves.h
			CLOSEVALVE(REDVALVE);
			status.overfill_RED = 0;
		} else {
			status.overfill_RED++;
		}
		if ((screen_value == SCRVALVES) && timerOLED) {
			display(SCRVALVES);
		}
	}

	if (BUFVALVEOPEN && !BUFTHERMWARM) {			// Valve open, therm cold
		if (status.overfill_BUF >= OVERFILLBUF) {	// valves.h
			CLOSEVALVE(BUFVALVE);
			status.overfill_BUF = 0;
			_delay_ms(2000);
			CLOSEVALVE(SUPVALVE);
		} else {
			status.overfill_BUF++;
		}
		if ((screen_value == SCRVALVES) && timerOLED) {
			display(SCRVALVES);
		}
	}

	if (status.supply_button_pushed) {
		if (~PORTB.IN & PIN1_bm) {			// Button still pushed
			if (status.supply_button_time++ >= 2) {
				OPENVALVE(SUPVALVE);
				status.opentime_SUP = 0;
				status.supply_button_pushed = FALSE;
				status.supply_button_time = 0;
				if ((screen_value == SCRVALVES)) {
					display(SCRVALVES);
				}
			}
		} else {								// Button was released
			status.supply_button_pushed = FALSE;
			status.supply_button_time = 0;
		}
	}

	if (timerOLED) {
		if (timerOLED > OLEDTIMEOUT) {	// Display timeout
			clear_OLED(0);
			timerOLED = 0;
		} else {
			timerOLED++;
		}
	}

}

/*----------------------------------------------------------------------
void init_RTC(uint16_t ticksRTC)
	Initialize the real time clock

	The real time clock (RTC) controller uses an on-board 32.768 kHz
	crystal to generate interrupts. The RTC is configured to run at
	512 Hz (64x divider).

	Input:
		ticksRTC - sets the time between interrupts such that
			ticksRTC = (512*secs)-1 where secs is the number of seconds
			between interrupts. ticksRTC = 30719 generates an interrupt
			every minute. ticksRTC = 511 generates an interrupt every
			second.
----------------------------------------------------------------------*/
void init_RTC(uint16_t ticksRTC)
{

	uint8_t temp;

	// Disable the external oscillator by clearing the enable bit 0
	temp = CLKCTRL.XOSC32KCTRLA;
	temp &= ~CLKCTRL_ENABLE_bm;		// set bit 0 of CLKCTRL.XOSC32KCTRLA to 0
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSC32KCTRLA = temp;

	// Wait for status bit (bit 6) in MCLKSTATUS to go to 0 (XOSC32K not running)
	while (CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm) {
		asm("nop");
	}

	// Select the external crystal (as opposed to external clock)
	// by setting the SEL bit on XOSC32KCTRLA (bit 2) to 0
	temp = CLKCTRL.XOSC32KCTRLA;
	temp &= ~CLKCTRL_SEL_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSC32KCTRLA = temp;

	// Enable the external oscillator by setting the enable bit
	// (bit 0) in CLKCTRL.XOSC32KCTRLA to 1
	temp = CLKCTRL.XOSC32KCTRLA;
	temp |= CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSC32KCTRLA = temp;

	while (RTC.STATUS) {	// Wait for all registers to sync
		asm("nop");
	}

//	RTC.PER = 30719;	// Set 1 minute period for overflow interrupts 
//	RTC.PER = 511;		// Set 1 sec period for overflow interrupts
	RTC.PER = ticksRTC;

	// Select the external crystal oscillator in RTC.CLKSEL register
	RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc;

	// Enable running in debug mode by setting bit 0 in RTC.CLKSEL to 1
	RTC.DBGCTRL |= RTC_DBGRUN_bm;

	// Enable running in standby mode by setting bit 7 in RTC.CTRLA to 1
	// & set the prescaler to DIV64 (512 Hz)
	// & set the RTC enable bit
	RTC.CTRLA = RTC_PRESCALER_DIV64_gc | RTC_RUNSTDBY_bm | RTC_RTCEN_bm;

	RTC.INTCTRL |= RTC_OVF_bm;	// Enable overflow interrupt

}

/*---------------------------------------------------------------------
Interrupt routine for RTC
	Every tick of the RTC executes here
----------------------------------------------------------------------*/
ISR(RTC_CNT_vect)
{

	RTC.INTFLAGS = RTC_OVF_bm;		// Clear interrupt flag

	seconds++;
	tick = TRUE;

}
