#ifndef EEPROMH
#define EEPROMH

#define VERSIONADDR		(0)		// Version string must have exactly 11 bytes
#define VERSIONSIZE		(10)	// Version is the date, as YYYY-DD-MM
#define FILLINTADDR		(10)
#define MAXOPENADDR		(11)
#define BLUENABLEADDR	(12)
#define REDENABLEADDR	(13)

#define BLUENABLED		(eeprom_read_byte((uint8_t *)BLUENABLEADDR))
#define REDENABLED		(eeprom_read_byte((uint8_t *)REDENABLEADDR))
#define FILLINTERVAL	(eeprom_read_byte((uint8_t *)FILLINTADDR))
#define MAXOPENTIME		(eeprom_read_byte((uint8_t *)MAXOPENADDR))

void get_VERSION(char*);
void update_VERSION(void);

#endif