#ifndef TIMERS_H
#define TIMERS_H

extern volatile uint16_t ticks_TCB0;

void start_TCB0(uint16_t);
void stop_TCB0(void);

#endif