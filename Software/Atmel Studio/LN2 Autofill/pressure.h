#ifndef PRESSURE_H
#define PRESSURE_H

#include "encoder.h"		// SCRPRESSURE, display()

// Pressure sensor defines (Panasonic ADP5151)
#define PSSLOPE			(0.8505)
#define PSINTERCEPT		(-24.69)
#define PSFREQ			(250)		// 250 results in 1/2 sec updates
//#define PSFREQ			(500)		// 500 results in 1/4 sec updates

#define BUILDPRESSURETIME	3		// Seconds before closing supply valve

void init_ADC(void);
uint8_t read_PRESSURE(void);

#endif