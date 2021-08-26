#include "globals.h"
#include "valves.h"

/*----------------------------------------------------------------------
INITIALIZE VALVES

	These pins drive the MOSFETs that control the solenoid valves. All
	are set  low and output.

	VALVn is the electronics schematic net name.

	VALV1 is on pin PC7 (Blue dewar vent)
	VALV2 is on pin PC6 (Red dewar vent)
	VALV3 is on pin PC5 (Buffer dewar vent)
	VALV4 is on pin PC4 (Supply input valve)
----------------------------------------------------------------------*/
void init_VALVES(void)
{

	PORTC.OUTCLR = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;
	PORTC.DIRSET = PIN7_bm | PIN6_bm | PIN5_bm | PIN4_bm;

}
