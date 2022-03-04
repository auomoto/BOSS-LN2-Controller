#ifndef VALVES_H
#define VALVES_H

#define OPENVALVE(VALVEPIN)		(PORTC.OUTSET = VALVEPIN)
#define CLOSEVALVE(VALVEPIN)	(PORTC.OUTCLR = VALVEPIN)
#define BLUVALVE		(PIN7_bm)	// These are the "VALVEPIN" values
#define REDVALVE		(PIN6_bm)
#define BUFVALVE		(PIN5_bm)
#define SUPVALVE		(PIN4_bm)
#define BLUVALVEOPEN	(PORTC.IN & PIN7_bm)	// TRUE if the valve is open
#define REDVALVEOPEN	(PORTC.IN & PIN6_bm)
#define BUFVALVEOPEN	(PORTC.IN & PIN5_bm)
#define SUPVALVEOPEN	(PORTC.IN & PIN4_bm)
#define BUFMAXOPEN		20						// In minutes
#define OVERFILLBUF		5						// In seconds
#define OVERFILLBLU		5						// In seconds
#define OVERFILLRED		5						// In seconds

// Thermistor pins
#define BLUTHERMWARM	(PORTE.IN & PIN0_bm)	// TRUE if the thermistor is warm
#define REDTHERMWARM	(PORTE.IN & PIN1_bm)
#define BUFTHERMWARM	(PORTE.IN & PIN2_bm)

void init_VALVES(void);

#endif