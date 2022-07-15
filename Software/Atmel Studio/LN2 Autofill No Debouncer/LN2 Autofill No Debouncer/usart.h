#ifndef USART_H
#define USART_H

#include "timers.h"
#include "eeprom.h"
#include "buttons.h"

#define BUFSIZE 255
#define	USART_BAUD_RATE(BAUD_RATE)	((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

typedef struct {
	uint8_t				// Serial I/O buffer
		data[BUFSIZE];	// Data to send or data received
	uint8_t	volatile
		done,			// Is the transfer complete?
		nxfrd,			// Number of bytes transferred
		length;			// Amount in the data buffer
} USARTBuf;

extern USARTBuf send0_buf, recv0_buf;

void handle_serial(void);
void init_USART(void);
void send_USART(uint8_t*, uint8_t);

#endif