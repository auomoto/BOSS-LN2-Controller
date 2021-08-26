#ifndef OLED_H
#define OLED_H

#define OLEDADDR0		(0x3C)		// TWI bus address
#define OLEDADDR1		(0x3D)		// TWI bus address
#define OLEDCMD			0x00		// Newhaven command was 1
#define OLEDDATA		0x40		// Newhaven command was 0
#define OLEDLINE1		0x80		// Newhaven command
#define OLEDLINE2		0xC0		// Newhaven command

void clear_OLED(uint8_t);
void init_OLED(uint8_t);
void write_OLED(uint8_t, uint8_t, uint8_t);
void writestr_OLED(uint8_t, char*, uint8_t);
extern uint16_t timerOLED, timeoutOLED;	// Used to blank the display
extern uint8_t display_off;				// TRUE if lit

#endif
