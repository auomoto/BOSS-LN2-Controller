#ifndef ENCODER_H
#define ENCODER_H

#define ENCODERBUTTON	5
#define ENCODERA		7
#define SCRVERSION		0
#define SCRVALVES		1
#define SCRNEXTFILL		2
#define SCRFILLINT		3
#define SCRMAXOPENTIME	4
#define SCRPRESSURE		5
#define SCRDISABLEBLU	6
#define SCRDISABLERED	7
#define MAXSCREENS		8

void change_DISABLEBLU(void);
void change_DISABLERED(void);
void change_MAXOPENTIME(void);
void change_FILLINT(void);
void display(uint8_t);
void handle_encoder(void);
void init_ENCODER(void);
void start_FILL(void);

extern volatile uint8_t encoder_sensed, encoder_value, screen_value;

#endif