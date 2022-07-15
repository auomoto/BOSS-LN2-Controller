#include "globals.h"
#include "eeprom.h"

/*------------------------------------------------------------------------------
eeprom.c
	Reads and writes the software version.
------------------------------------------------------------------------------*/
void get_VERSION(char *version)
{
	eeprom_read_block((void *)version, (const void *)VERSIONADDR, VERSIONSIZE);
}

void update_VERSION(void)
{
	char version[11];

	strcpy(version, VERSION);
	eeprom_update_block((const void *)version, (void *)VERSIONADDR, VERSIONSIZE);
}
