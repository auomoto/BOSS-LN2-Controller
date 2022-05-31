#ifndef PRESSURE_H
#define PRESSURE_H

#include "encoder.h"		// SCRPRESSURE, display()

// Pressure sensor defines (Panasonic ADP5151)
#define PSSLOPE			(0.8505)
#define PSINTERCEPT		(-24.69)

#define BUILDPRESSURETIME	3		// Seconds before closing supply valve

void init_ADC(void);
uint8_t read_PRESSURE(void);

#endif