#ifndef RTC_H
#define RTC_H

#include "oled.h"
#include "valves.h"
#include "eeprom.h"
#include "encoder.h"
#include "pressure.h"

void handle_ticks(void);
void handle_minute(void);
void handle_hour(void);
void init_RTC(uint16_t);

#endif