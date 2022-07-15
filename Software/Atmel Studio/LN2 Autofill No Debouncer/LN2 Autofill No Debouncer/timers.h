#ifndef TIMERS_H
#define TIMERS_H

void start_TCB0(uint16_t);
void stop_TCB0(void);

extern volatile uint16_t ticks_TCB0;

#endif