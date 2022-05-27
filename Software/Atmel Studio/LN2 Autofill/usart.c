#include "globals.h"
#include "usart.h"

USARTBuf send0_buf, recv0_buf;

/*------------------------------------------------------------------------------
STATUS REPORT OVER SERIAL LINE
	[0] - Supply valve status <O|C>
	[1] - Buffer valve status <O|C|T>
	[2] - Red dewar valve status <O|C|T|X>
	[3] - Blue dewar valve status <O|C|T|X>
	[4-6] - Fill interval in minutes

LN2,C,SUP,T,BUF,X,RED,C,BLU,111,NXT,222,MAX,333,INT,100,PRES,H,TBUF,H,TRED,H,TBLU

------------------------------------------------------------------------------*/
void handle_serial(void)
{

	const char fmt0[] = "LN2,%c,SUP,%c,BUF,%c,RED,%c,BLU,%d,NXT,%d,MAX,%d,INT,%d,PRES,%c,TBUF,%c,TRED,%c,TBLU\r";
	char strbuf[100], supvalve, bufvalve, redvalve, bluvalve, bufther, redther, bluther;
	uint8_t nextfill, maxopen, fillint, pressure;

	recv0_buf.done = FALSE;

	// Buffer dewar supply valve state
	if (SUPVALVEOPEN) {
		supvalve = 'O';
	} else {
		supvalve = 'C';
	}

	// Buffer dewar vent valve
	if (status.maxopen_BUF) {
		bufvalve = 'T';
	} else if (BUFVALVEOPEN) {
		bufvalve = 'O';
	} else {
		bufvalve = 'C';
	}

	// Red CCD vent valve
	if (!REDENABLED) {
		redvalve = 'X';
	} else if (status.maxopen_RED) {
		redvalve = 'T';
	} else if (REDVALVEOPEN) {
		redvalve = 'O';
	} else {
		redvalve = 'C';
	}

	// Blue CCD vent valve
	if (!BLUENABLED) {
		bluvalve = 'X';
	} else if (status.maxopen_BLU) {
		bluvalve = 'T';
	} else if (BLUVALVEOPEN) {
		bluvalve = 'O';
	} else {
		bluvalve = 'C';
	}

	// Fill interval
	fillint = FILLINTERVAL;

	// Next fill in
	nextfill = status.next_fill;

	// Max open time
	maxopen = MAXOPENTIME;

	// Pressure
	pressure = status.pressure;

	// BUF thermistor
	if (BUFTHERMWARM) {
		bufther = 'H';
	} else {
		bufther = 'C';
	}

	// Red thermistor
	if (REDTHERMWARM) {
		redther = 'H';
	} else {
		redther = 'C';
	}

	// Blue thermistor
	if (BLUTHERMWARM) {
		bluther = 'H';
	} else {
		bluther = 'C';
	}

	sprintf(strbuf, fmt0, supvalve, bufvalve, redvalve, bluvalve, nextfill, maxopen,
		fillint, pressure, bufther, redther, bluther);

	start_TCB0(100);			// 100 ms ticks
	while (!send0_buf.done) {
		if (ticks_TCB0 > 10) {	// See timers.h
			return;
		}
		asm("nop");
	}

	send_USART((uint8_t*) strbuf, strlen(strbuf));

}


void handle_serialX(void)
{

	
	char strbuf[81], tempstr[81];

	recv0_buf.done = FALSE;

	// Buffer dewar supply Valve
	strcpy(strbuf, "SUP,");
	if (SUPVALVEOPEN) {
		strcat(strbuf, "O");
	} else {
		strcat(strbuf, "C");
	}

	// Buffer dewar vent valve
	strcat(strbuf, ",BUF,");
	if (status.maxopen_BUF) {
		strcat(strbuf, "T");
	} else if (BUFVALVEOPEN) {
		strcat(strbuf, "O");
	} else {
		strcat(strbuf, "C");
	}

	// Red CCD vent valve
	strcat(strbuf,",RED,");
	if (!REDENABLED) {
		strcat(strbuf, "X");
	} else if (status.maxopen_RED) {
		strcat(strbuf, "T");
	} else if (REDVALVEOPEN) {
		strcat(strbuf, "O");
	} else {
		strcat(strbuf, "C");
	}

	// Blue CCD vent valve
	strcat(strbuf, ",BLU,");
	if (!BLUENABLED) {
		strcat(strbuf, "X");
		} else if (status.maxopen_BLU) {
		strcat(strbuf, "T");
		} else if (BLUVALVEOPEN) {
		strcat(strbuf, "O");
		} else {
		strcat(strbuf, "C");
	}

	// Fill interval
	strcat(strbuf, ",INT,");
	sprintf(tempstr, "%d", FILLINTERVAL);
	strcat(strbuf, tempstr);

	// Time to next fill
	strcat(strbuf, ",NEXT,");
	sprintf(tempstr, "%d", status.next_fill);
	strcat(strbuf, tempstr);

	// Max open time
	strcat(strbuf, ",MAX,");
	sprintf(tempstr, "%d", MAXOPENTIME);
	strcat(strbuf, tempstr);	

	// Pressure
	strcat(strbuf, ",PRES,");
	sprintf(tempstr, "%d", status.pressure);
	strcat(strbuf, tempstr);

	// Thermistors
	strcat(strbuf, ",TBUF,");
	if (BUFTHERMWARM) {
		strcat(strbuf, "H");
	} else {
		strcat(strbuf, "C");
	}
	strcat(strbuf, ",TRED,");
	if (REDTHERMWARM) {
		strcat(strbuf, "H");
	} else {
		strcat(strbuf, "C");
	}
	strcat(strbuf, ",TBLU,");
	if (BLUTHERMWARM) {
		strcat(strbuf, "H");
	} else {
		strcat(strbuf, "C");
	}

	strcat(strbuf,"\r");
	start_TCB0(100);			// 100 ms ticks
	while (!send0_buf.done) {
		if (ticks_TCB0 > 10) {	// See timers.h
			return;
		}
		asm("nop");
	}
	send_USART((uint8_t*) strbuf, strlen(strbuf));
}


/*------------------------------------------------------------------------------
void init_USART(void)
	Set up USART port USART0 for 9600 baud, 8 bits, no parity, 1 stop bit.
	Initialize the serial buffers.
------------------------------------------------------------------------------*/
void init_USART(void)
{
	// USART0 PA0 is TxD, PA1 is RxD, Default pin position
	PORTA.OUTSET = PIN0_bm;
	PORTA.DIRSET = PIN0_bm;
	USART0.BAUD = (uint16_t) USART_BAUD_RATE(9600);
	USART0.CTRLA |= USART_RXCIE_bm;		// Enable receive complete interrupt
	USART0.CTRLB |= USART_TXEN_bm;		// Enable USART transmitter
	USART0.CTRLB |= USART_RXEN_bm;		// Enable USART receiver
	recv0_buf.length = 0;
	send0_buf.length = 0;
	recv0_buf.nxfrd = 0;
	send0_buf.nxfrd = 0;
	recv0_buf.done = FALSE;				// Ready to receive data
	send0_buf.done = TRUE;				// No data to send

}

/*------------------------------------------------------------------------------
void send_USART(char *data)
	Send data out a serial USART port.

	Returns:
		Nothing

	How it works:
		This copies the data array into the send0_buf data buffer and enables
		"transmit data register empty" interrupt (DREIE). The USART0_DRE_vect
		starts puts the bytes into the transmit register.
------------------------------------------------------------------------------*/
void send_USART(uint8_t *data, uint8_t nbytes)
{
	uint8_t i;
	send0_buf.length = nbytes;
	send0_buf.nxfrd = 0;
	send0_buf.done = FALSE;
	for (i = 0; i < nbytes; i++) {		// Copy nbytes to send buffer
		send0_buf.data[i] = *data++;
	}
	USART0.CTRLA |= USART_DREIE_bm;		// Enable interrupts
}

/*------------------------------------------------------------------------------
ISR(USART0_RXC_vect)
	A byte at USART0 has been received.

	If the character received is a <CR> ('\r'), a string terminator ('\0') is
	inserted into the buffer instead of the '\r' and the done flag is set.
------------------------------------------------------------------------------*/
ISR(USART0_RXC_vect)
{
	uint8_t c;

	c = USART0.RXDATAL;

	if (recv0_buf.length < BUFSIZE) {
		if ((char) c == '\r') {
			recv0_buf.data[recv0_buf.length] = '\0';
			recv0_buf.done = TRUE;						// CR received
		} else {
			recv0_buf.data[recv0_buf.length++] = c;		// Gather a character
		}
	}
}

/*------------------------------------------------------------------------------
ISR(USART0_DRE_vect)
	Transmit data register empty interrupt. When the transmit data register
	(USART0.TXDATAL) is empty and the interrupt is enabled, you end up here.
	
	Here, we send out another byte of data and then compare the number of
	bytes already transferred (nxfrd) with the number requested (nbytes).
	If nxfrd == nbytes, then we set the send0_buf.done flag to YES and turn
	off this interrupt.

	Sending is started by calling send_USART(port).
------------------------------------------------------------------------------*/
ISR(USART0_DRE_vect)
{

	USART0.CTRLA &= ~USART_DREIE_bm;		// Turn off interrupts
	USART0.TXDATAL = send0_buf.data[send0_buf.nxfrd++];
	if (send0_buf.length == send0_buf.nxfrd) {
		send0_buf.done = TRUE;				// Last character sent
	} else {
		USART0.CTRLA |= USART_DREIE_bm;		// Turn on interrupts
	}
}
