/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *        IBM PC/Compatibles software series       *
 *                Copyright (c) 1993               *
 ***************************************************
 * DRIVEWIN.C   |   SDK1-2a   |   January 1996
 *
 * Immersion Corp. Software Developer's Kit
 *      Windows for serial communications functions
 *  	  for the Immersion Corp. MicroScribe-3D
 *		  Not for use with the Probe or Personal Digitizer
 *      Requires HCI firmware version MSCR1-1C or later
 */

#include <stdio.h>
#include <windows.h>
#include <string.h>

#include "drive.h"

#define NUM_PORTS       4

/* Number of biostime ticks in one timeout period */
static long     timeout_ticks[NUM_PORTS+1];
static long     stop_tick[NUM_PORTS+1];


/*------------------*/
/* Timing Functions */
/*------------------*/


/* host_pause() pauses for the given number of seconds
 */
void host_pause(float delay_sec)
{
	long int start_tick = GetTickCount();
	long int stop_tick = start_tick + (long int) (delay_sec * 1000.0);

	if (stop_tick == start_tick) stop_tick++;

	while(GetTickCount() < stop_tick);
}


/* host_get_timeout() gets the timeout period of the given port in seconds
 */
float host_get_timeout(int port)
{
	return (float) timeout_ticks[port] / 1000.0;
}


/* host_set_timeout() sets the length of all future timeout periods
 *   to the given # of seconds
 */
void host_set_timeout(int port, float timeout_sec)
{
	timeout_ticks[port] = (long) (timeout_sec * 1000.0);
	if (timeout_ticks == 0L)
		timeout_ticks[port] = 1L;
}


/* host_start_timeout() starts a timer for the specified port.
 *   Call timed_out_yet() to find out whether time is up.
 */
void host_start_timeout(int port)
{
	stop_tick[port] = GetTickCount() + timeout_ticks[port];
}


/* host_timed_out() returns True if the previously-started timeout
 *   period is over.  Returns False if not.
 */
int host_timed_out(int port)
{
	return (stop_tick[port] < GetTickCount());
}



/*----------------------*/
/* Serial i/o Functions */
/*----------------------*/

	/* the structure to set the baud rates */
typedef struct {
	long rate;
	unsigned int value;
} baud_list_st;

	/* Constants for PC/compatible serial COM */
static const baud_list_st baud_list[] = {
	{ 300L, CBR_300 },
	{ 600L, CBR_600 },
	{ 1200L, CBR_1200 },
	{ 2400L, CBR_2400 },
	{ 4800L, CBR_4800 },
	{ 9600L, CBR_9600 },
	{ 19200L, CBR_19200 },
	{ 38400L, CBR_38400 },
	{ 0L, 0 }                                       /* end of list indicator */
};

static const char *port_names[] = { "COM1", "COM2", "COM3", "COM4" };

	/* I/O buffers and supporting variables */
typedef struct {
	int id;         /* the port id from OpenComm */
	long baud;      /* the actual baud rate */
   DCB dcb;             /* the device control block */
} com_port;

/* com port data structure */
com_port com_ports[4];


int host_get_id(int port)
{
	switch (port) {
		case 1 : return com_ports[0].id;
		case 2 : return com_ports[1].id;
		case 3 : return com_ports[2].id;
		case 4 : return com_ports[3].id;
		default : return -1;
	}
}


/*--------------------------------*/
/* Fixing up baud rate parameters */
/*--------------------------------*/


/* host_fix_baud() finds nearest valid baud rate to the one given.
 *    Takes small arguments as shorthand:
 *      115 --> 115200, 38 or 384 --> 38400, 96 --> 9600 etc.
 */
void host_fix_baud(long int *baud)
{
	switch(*baud)
	{
		case 115200L:
		case 1152L:
		case 115L:
			*baud = 115200L;
			break;
		case 57600L:
		case 576L:
		case 58L:
		case 57L:
			*baud = 57600L;
			break;
		case 38400L:
		case 384L:
		case 38L:
			*baud = 38400L;
			break;
		case 19200L:
		case 192L:
		case 19L:
			*baud = 19200L;
			break;
		case 14400L:
		case 144L:
		case 14L:
			*baud = 14400L;
			break;
		case 9600L:
		case 96L:
			*baud = 9600L;
			break;
		default:
			if (*baud < 1000L) *baud *= 1000;
			if (*baud > 86400L) *baud = 115200L;
			else if (*baud > 48000L) *baud = 57600L;
			else if (*baud > 28800L) *baud = 38400L;
			else if (*baud > 16800L) *baud = 19200L;
			else if (*baud > 12000L) *baud = 14400L;
			else *baud = 9600L;
			break;
	}
}



/*--------------------------*/
/* Configuring Serial Ports */
/*--------------------------*/


/* host_open_serial() opens the given serial port with specified baud rate
 *    Always uses 8 data bits, 1 stop bit, no parity.
 *    Returns False (zero) if called with zero baud rate.
 */
int host_open_serial(int port, long int baud)
{
	com_port *cport;
	int index;
	int     result = 0;
   char buffer[20];
	int i;

	/* Calculate baud rate divisor, watch out for div by zero */
	if (baud != 0L) {
			/* Get parameters for the required port */
		switch (port) {
			case 1 :        index = 0;      break;
			case 2 :        index = 1;      break;
			case 3 :        index = 2;      break;
			case 4 :        index = 3;      break;
			default : return result;
		}
		cport = &(com_ports[index]);

			/* open the port */
		if ((cport->id = OpenComm(port_names[index],2048,2048)) < 0)
			return result;

			/* build the settings string and DCB */
		wsprintf(buffer,"%s:9600,n,8,1",port_names[index]);
		if (BuildCommDCB(buffer,&(cport->dcb)) < 0)
			return result;

			/* finish the setup */
		cport->dcb.Id = cport->id;
		for (i=0; baud_list[i].rate != 0; i++) {
			if (baud_list[i].rate == baud)
				break;
		}
		if (baud_list[i].value == 0)
			cport->dcb.BaudRate = CBR_9600;
		else
      	cport->dcb.BaudRate = baud_list[i].value;
		if (SetCommState(&(cport->dcb)) < 0)
			return result;

		result = 1;
	}

	return result;
}


/* host_close_serial() closes the given serial port.
 *    NEVER call this without first calling host_open_serial() on the
 *       same port.
 */
void host_close_serial(int port)
{
	com_port *cport;

	switch (port) {
		case 1 : cport = &(com_ports[0]); break;
		case 2 : cport = &(com_ports[1]); break;
		case 3 : cport = &(com_ports[2]); break;
		case 4 : cport = &(com_ports[3]); break;
		default : cport = NULL;
	}

	if (cport != NULL) {
			/* clear the queues */
		FlushComm(cport->id,0);
		FlushComm(cport->id,1);

			/* close the port */
		CloseComm(cport->id);
	}
}


/* host_flush_serial() flushes and resets the serial i/o buffers
 */
void host_flush_serial(int port)
{
	com_port *cport;

	switch (port) {
		case 1 : cport = &(com_ports[0]); break;
		case 2 : cport = &(com_ports[1]); break;
		case 3 : cport = &(com_ports[2]); break;
		case 4 : cport = &(com_ports[3]); break;
		default : cport = NULL;
	}

	if (cport != NULL) {
		FlushComm(cport->id,0);
		FlushComm(cport->id,1);
	}
}



/*------------------*/
/* Input and Output */
/*------------------*/



/* host_read_char() reads one character from the serial input buffer.
 *    returns -1 if input buffer is empty
 */
int host_read_char(int port)
{
	com_port *cport;
   unsigned char ch;

	switch (port) {
		case 1 : cport = &(com_ports[0]); break;
		case 2 : cport = &(com_ports[1]); break;
		case 3 : cport = &(com_ports[2]); break;
		case 4 : cport = &(com_ports[3]); break;
		default : return -1;
	}

	if (ReadComm(cport->id,&ch,1) <= 0)
		return -1;
	else
		return (int) ch;
}


/* host_read_bytes() will try to read a specified number of bytes
 * until the timeout period of time expires.  It returns the number
 * of bytes it actually read.
 */
int host_read_bytes(int port, char *buf, int count, float timeout)
{
	int read;
	int ch;

		/* setup the timeout */
	read = 0;
	host_set_timeout(port,timeout);
	host_start_timeout(port);

	while (!host_timed_out(port))
	{
		if ((ch = host_read_char(port)) != -1) {
			*buf++ = (char) ch;
			if (++read == count)
				break;
		}
	}

	return read;
}




/* host_write_char() writes one character to the serial output buffer
 *    Returns False (zero) if buffer is full
 *    Returns True (non-zero) if successful
 */
int host_write_char(int port, int byt)
{
	com_port *cport;
	unsigned char ch;

	switch (port) {
		case 1 : cport = &(com_ports[0]); break;
		case 2 : cport = &(com_ports[1]); break;
		case 3 : cport = &(com_ports[2]); break;
		case 4 : cport = &(com_ports[3]); break;
		default : return 0;
	}

	ch = byt;
	if (WriteComm(cport->id,&ch,1) == 1)
		return 1;
	else
	return 0;
}


/* host_write_string() writes a null-terminated string to the output buffer
 *    Returns False (zero) if not enough rooom
 *    Returns True (non-zero) if successful
 */
int host_write_string(int port, char *str)
{
	com_port *cport;
	COMSTAT cs;

	switch (port) {
		case 1 : cport = &(com_ports[0]); break;
		case 2 : cport = &(com_ports[1]); break;
		case 3 : cport = &(com_ports[2]); break;
		case 4 : cport = &(com_ports[3]); break;
		default : return 0;
	}

	GetCommError(cport->id,&cs);
	if (strlen(str) >= (2048 - cs.cbOutQue))
		return 0;
	else if (WriteComm(cport->id,str,strlen(str)) < strlen(str))
		return 0;
	else
	return 1;
}



/*----------------------------*/
/* Getting Serial Port Status */
/*----------------------------*/


/* host_port_valid() returns True if the specified port number is valid
 */
int host_port_valid(int port)
{
	return (port > 0) && (port <= NUM_PORTS);
}


/* host_input_count() returns the number of chars waiting in the input queue
 */
int host_input_count(int port)
{
	COMSTAT cs;

	switch (port) {
		case 1 : GetCommError(com_ports[0].id,&cs);     break;
		case 2 : GetCommError(com_ports[1].id,&cs);     break;
		case 3 : GetCommError(com_ports[2].id,&cs);     break;
		case 4 : GetCommError(com_ports[3].id,&cs);     break;
		default : return 0;
	}

	return cs.cbInQue;
}


/* host_input_full() tells whether or not the serial input queue is full
 */
int host_input_full(int port)
{
	return ((2048 - host_input_count(port)) == 0);
}