#ifndef RTC_H
#define RTC_H

#include "oled.h"
#include "eeprom.h"
#include "encoder.h"
#include "pressure.h"
#include "buttons.h"

void handle_ticks(void);
void handle_minute(void);
void handle_hour(void);
void init_RTC(void);

#endif