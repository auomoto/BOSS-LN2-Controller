#ifndef TIMERS_H
#define TIMERS_H

// Oscillator signal for debouncer on PC0 (temporary)
//#define TCB2FREQ		1500		// Debouncer PWM frequency in Hz
#define TCB2FREQ		30		// Debouncer PWM frequency in Hz

extern volatile uint16_t ticks_TCB0;

void init_TCB2(void);
void start_TCB0(uint16_t);
void stop_TCB0(void);

#endif