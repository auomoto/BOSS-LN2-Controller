#ifndef GLOBALS_H
#define GLOBALS_H

#define F_CPU		3333333UL
#define VERSION		"2022-07-17"		// Must be exactly 10 characters
#define FALSE		0
#define TRUE		1
#define BIGTANK		FALSE		// Autofill from large supply dewar?
#define BIGFILLINTERVAL		12	// Big tank fill interval in hours

#include <avr/io.h>			// AVR macros
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>			// strcat, strlen, strcpy
#include <util/delay.h>
#include <stdio.h>			// sprintf

struct statusbuf {
	char version[11];			// Version (date string)
	uint8_t opentime_BLU,		// Minutes that valve has been open
		opentime_RED,			// Minutes that valve has been open
		opentime_BUF,			// Minutes that valve has been open
		opentime_SUP,			// Minutes that valve has been open
		maxopen_BLU,			// TRUE if MAXOPENTIME is exceeded
		maxopen_RED,			// TRUE if MAXOPENTIME is exceeded
		maxopen_BUF,			// TRUE if MAXOPENTIME is exceeded
		overfill_BUF,			// Seconds after thermistor gets cold
		overfill_BLU,			// Seconds after thermistor gets cold
		overfill_RED,			// Seconds after thermistor gets cold
		next_fill,				// Minutes until next fill
		next_buftank_fill,		// Hours until next buffer tank fill (if BIGTANK)
		supply_button_pushed,	// If you push the supply button
		supply_button_time,		// # seconds the button has been held down
		buildpressure_time,		// Seconds to build LN2 pressure
		pressure;				// LN2 pressure in kPa
};

// Global variables
struct statusbuf status;
volatile uint8_t tick, seconds, minutes, minute_flag, hours, hour_flag;

#endif