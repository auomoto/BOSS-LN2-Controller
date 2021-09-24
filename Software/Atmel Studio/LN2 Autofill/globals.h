#ifndef GLOBALS_H
#define GLOBALS_H

#define F_CPU		3333333UL
#define VERSION		"2021-09-24"		// Must be exactly 10 characters
#define FALSE		0
#define TRUE		1

#include <avr/io.h>			// AVR macros
#include <avr/interrupt.h>	// Interrupt defines
#include <avr/eeprom.h>		// EEPROM defines
#include <string.h>			// strcat, strlen, strcpy
#include <util/delay.h>		// _delay_ms
#include <stdio.h>			// sprintf

struct statusbuf {
	char version[11];					// Version (date string)
	uint8_t opentime_BLU, opentime_RED,	// Minutes that valve has been open
		opentime_BUF, opentime_SUP,		// Minutes that valve has been open
		maxopen_BLU, maxopen_RED,		// TRUE if MAXOPENTIME is exceeded
		maxopen_BUF,					// TRUE if MAXOPENTIME is exceeded
		overfill_BUF,					// Seconds after thermistor gets cold
		overfill_BLU, overfill_RED,		// Seconds after thermistor gets cold
		next_fill,						// Minutes until next fill
		buildpressure_time,				// Seconds to build LN2 pressure
		pressure;						// LN2 pressure in kPa
};

struct statusbuf status;
volatile uint8_t tick, seconds, minute;

#endif