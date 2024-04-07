/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *        IBM PC/Compatibles software series       *
 *                Copyright (c) 1993               *
 ***************************************************
 * DRIVEPC.C   |   SDK1-2a   |   January 1996
 *
 * Immersion Corp. Software Developer's Kit
 *      PC-Specific serial communications functions
 *  	  for the Immersion Corp. MicroScribe-3D
 *		  Not for use with the Probe or Personal Digitizer
 *      Requires HCI firmware version MSCR1-1C or later
 */

#include <stdio.h>
#include <bios.h>
#include <dos.h>
#include <conio.h>
#include "drive.h"

#define NUM_PORTS       2

#if defined(__TURBOC__)

#define INTERRUPT       interrupt
#define INPB                    inp
#define INPW                    inpw
#define OUTPB                   outp
#define OUTPW                   outpw
#define DISABLE()               disable()
#define ENABLE()                enable()
#define GETVECT(x)      getvect(x)
#define SETVECT(x,y)    setvect(x,y)

#elif defined (_MSC_VER)

#pragma message("Using Microsoft hardware I/O Functions")
#define INTERRUPT       __interrupt
#define INPB                    inp
#define INPW                    inpw
#define OUTPB                   outp
#define OUTPW                   outpw
#define DISABLE()               _disable()
#define ENABLE()                _enable()
#define GETVECT(x)      _dos_getvect(x)
#define SETVECT(x,y)    _dos_setvect(x,y)

#else

#error UNDEFINED Compiler for hardware access functions

#endif

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
	long int start_tick;
	long int stop_tick;
   long int current;

	_bios_timeofday(0,&start_tick);
	stop_tick = start_tick + (long int) (delay_sec * 18.2);

	if (stop_tick == start_tick)
		stop_tick++;

	do {
		_bios_timeofday(0,&current);
	} while(current < stop_tick);
}


/* host_get_timeout() gets the timeout period of the given port in seconds
 */
float host_get_timeout(int port)
{
	return (float) timeout_ticks[port] / 18.2;
}


/* host_set_timeout() sets the length of all future timeout periods
 *   to the given # of seconds
 */
void host_set_timeout(int port, float timeout_sec)
{
	timeout_ticks[port] = (long) (timeout_sec * 18.2);

	if (timeout_ticks == 0L)
		timeout_ticks[port] = 1L;
}


/* host_start_timeout() starts a timer for the specified port.
 *   Call timed_out_yet() to find out whether time is up.
 */
void host_start_timeout(int port)
{
	long int current;

	_bios_timeofday(0,&current);
	stop_tick[port] = current + timeout_ticks[port];
}


/* host_timed_out() returns True if the previously-started timeout
 *   period is over.  Returns False if not.
 */
int host_timed_out(int port)
{
	long int current;

	_bios_timeofday(0,&current);
	return (stop_tick[port] < current);
}



/*----------------------*/
/* Serial i/o Functions */
/*----------------------*/


/* Constants for PC/compatible serial COM */

#define STOP_BIT_1      0
#define DATA_BITS_8     3

#define COM_BASE_1      0x3F8
#define COM_BASE_2      0x2F8

#define COM_IRQ_1                       4
#define COM_IRQ_2                       3

#define COM_VECT_1              (COM_IRQ_1 + 0x08)
#define COM_VECT_2         (COM_IRQ_2 + 0x08)

#define COM_IRQ_MASK_1  (1 << COM_IRQ_1)
#define COM_IRQ_MASK_2  (1 << COM_IRQ_2)

#define OUT_BUFFER_SIZE         0x1000
#define IN_BUFFER_SIZE          0x200

#define DTR             0x01
#define RTS             0x02
#define OUT2            0x08
#define DLAB            0x80

#define NO_SERVICE      0x01
#define RX_SERVICE      0x04
#define TX_SERVICE      0x02
#define EOI             0x20


/* I/O buffers and supporting variables */
typedef struct {
	int count;
	int full;
	int error;

	int in_head;
	int in_tail;
	int out_head;
	int out_tail;

	int Tx_idle;

	unsigned char output[OUT_BUFFER_SIZE];
	unsigned char input[IN_BUFFER_SIZE];

	unsigned int base;
	unsigned int vector;
	unsigned int irq_mask;

	void (INTERRUPT *old_isr)();
} com_port;

/* com port data structure */
com_port com_ports[2];

/* Interrupt service routines for each port */
void INTERRUPT COM1_isr(void);
void INTERRUPT COM2_isr(void);

/* constants for port access */
static void (INTERRUPT *COM_ISR[])() = { COM1_isr, COM2_isr };
static const unsigned int COM_BASE[] = { COM_BASE_1, COM_BASE_2 };
static const unsigned int COM_VECT[] = { COM_VECT_1, COM_VECT_2 };
static const unsigned char COM_IRQ_MASK[] = {
	COM_IRQ_MASK_1, COM_IRQ_MASK_2
};

/* Low-level internal functions */
void start_Tx(com_port *cport);
void handle_interrupt(com_port *cport);

#pragma argsused
/* dummy function -> needed for windows */
int host_get_id(int port)
{
	return 0;
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
	int     divisor, MSB_divisor, LSB_divisor;
	int     result = 0;

	/* Calculate baud rate divisor, watch out for div by zero */
	if (baud != 0L)
	{
		divisor = (int)(115200L / baud);
		if (divisor > 0)
		{
			MSB_divisor = divisor / 256;
			LSB_divisor = divisor & 0xFF;

			/* Get parameters for the required port */

			if (port == 1) {
				index = 0;
				cport = &(com_ports[0]);
			} else {
				index = 1;
				cport = &(com_ports[1]);
			}

			cport->base = COM_BASE[index];
			cport->vector = COM_VECT[index];
			cport->irq_mask = COM_IRQ_MASK[index];
			cport->old_isr = GETVECT(cport->vector);

			/* Install 8259 handler */
			SETVECT(cport->vector,COM_ISR[index]);

			/* Low-level 8259 procedure */
			DISABLE();

			OUTPB(cport->base + 4, RTS | DTR);
			OUTPB(cport->base + 1, 0);
			OUTPB(cport->base + 3, DLAB | DATA_BITS_8 | STOP_BIT_1 );
			OUTPB(cport->base, LSB_divisor);
			OUTPB(cport->base + 1, MSB_divisor);
			OUTPB(cport->base + 3, DATA_BITS_8 | STOP_BIT_1 );
			INPB(cport->base);
			INPB(cport->base + 5);
			INPB(cport->base + 6);
			OUTPB(cport->base + 1, 0x07);
			OUTPB(cport->base + 1, 0);
			OUTPB(0x21, ~(cport->irq_mask) & INPB(0x21));
			OUTPB(cport->base + 4, OUT2 | RTS | DTR);
			OUTPB(cport->base + 1, 0x07);
			host_flush_serial(port);

			ENABLE();

			result = 1;
		}
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

	if (port == 1)
		cport = &(com_ports[0]);
	else
		cport = &(com_ports[1]);

	/* Low-level 8259 closing procedure */
	DISABLE();
	OUTPB(0x21, cport->irq_mask | INPB(0x21));
	OUTPB(cport->base + 4, RTS | DTR);
	OUTPB(cport->base + 1, 0);
	ENABLE();

	/* Reset the port's buffer variables and original isr */
	host_flush_serial(port);
	SETVECT(cport->vector, cport->old_isr);
}


/* host_flush_serial() flushes and resets the serial i/o buffers
 */
void host_flush_serial(int port)
{
	com_port *cport;

	if (port == 1)
		cport = &(com_ports[0]);
	else
		cport = &(com_ports[1]);

	DISABLE();
	cport->in_head = cport->in_tail = 0;
	cport->out_head = cport->out_tail = 0;
	cport->count = 0;
	cport->error = 0;
	cport->full = 0;
	cport->Tx_idle = 1;
	ENABLE();
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

	if (port == 1)
		cport = &(com_ports[0]);
	else
		cport = &(com_ports[1]);

		/* If empty, return -1 */
	if (cport->in_head == cport->in_tail)
	{
		return (int) -1;
	}
	else
	{
			/* If not empty, decrease the count and return the
			 *   input char */
		DISABLE();
		cport->count--;
		ENABLE();

		if (++(cport->in_tail) >= IN_BUFFER_SIZE)
			cport->in_tail = 0;
		return (int) (cport->input[cport->in_tail]);
	}
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
			*buf = (char) ch;
			buf++;
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

	if (port == 1)
		cport = &(com_ports[0]);
	else
		cport = &(com_ports[1]);

		/* get the next position */
	if(++(cport->out_head) >= OUT_BUFFER_SIZE)
		cport->out_head = 0;

		/* make sure we are not full */
	if (cport->out_head == cport->out_tail)
	{
		cport->out_head--;
		return 0;
	}

		/* if we make it here -> there is space for the char */
	cport->output[cport->out_head] = byt;

	DISABLE();
	start_Tx(cport);
	ENABLE();

	return 1;
}


/* host_write_string() writes a null-terminated string to the output buffer
 *    Returns False (zero) if not enough rooom
 *    Returns True (non-zero) if successful
 */
int host_write_string(int port, char *str)
{
	char *temp = str;
	int     save_head;
	com_port *cport;

	if (port == 1)
		cport = &(com_ports[0]);
	else
		cport = &(com_ports[1]);

	save_head = cport->out_head;
	if (cport->out_head >= cport->out_tail)
	{
		while (*temp && (cport->out_head < OUT_BUFFER_SIZE-1))
			cport->output[++(cport->out_head)] = *temp++;
		if (*temp)
			cport->out_head = -1;
	}

	while (*temp && (cport->out_head < cport->out_tail))
		cport->output[++(cport->out_head)] = *temp++;

	if (*temp)
	{
		cport->out_head = save_head;
		return 0;
	}
	else
	{
		DISABLE();
		start_Tx(cport);
		ENABLE();
		return 1;
	}
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
	if (port == 1)
		return com_ports[0].count;
	else
		return com_ports[1].count;
}


/* host_input_full() tells whether or not the serial input queue is full
 */
int host_input_full(int port)
{
	if (port == 1)
		return com_ports[0].full;
	else
		return com_ports[1].full;
}



/*-------------------------------*/
/* Low-level interrupt functions */
/*-------------------------------*/


/* start_Tx() starts a transmission by initiating a Tx interrupt on the port
 */
void start_Tx(com_port *cport)
{
	unsigned char oldval;

	if (cport->Tx_idle) {
		oldval = INPB(cport->base+1);
		OUTPB(cport->base+1,oldval & ~0x02);
		OUTPB(cport->base+1,oldval);
		cport->Tx_idle = 0;
	}
}


/* COM1_isr() is the interrupt handler for COM1
 */
void INTERRUPT COM1_isr(void)
{
	handle_interrupt(&(com_ports[0]));
}

void INTERRUPT COM2_isr(void)
{
	handle_interrupt(&(com_ports[1]));
}


void handle_interrupt(com_port *cport)
{
	unsigned char status;
	unsigned char data;
	register int temp;

		 /* See if COM service is required */
	while ((status = (INPB(cport->base+2) & 0x07)) != NO_SERVICE) {
		switch (status) {
			case RX_SERVICE :
				data = INPB(cport->base);
				temp = cport->in_head;
				if (++temp == IN_BUFFER_SIZE)
					temp = 0;
				if (temp != cport->in_tail) {
					cport->input[temp] = data;
					cport->in_head = temp;
					cport->count++;
				} else {
					cport->full++;
				}
				break;
			case TX_SERVICE :
				temp = cport->out_tail;
				if (temp != cport->out_head)
				{
					if (++temp == OUT_BUFFER_SIZE)
						temp = 0;
					OUTPB(cport->base,cport->output[temp]);
					cport->out_tail = temp;
				}
				else
				{
					cport->Tx_idle = 1;
				}
				break;

			default: /* this is a status interrupt */
				INPB(cport->base+5);
				INPB(cport->base+6);
				cport->error++;
				break;
		}
	}

		/* send the EOI */
	OUTPB(0x20,EOI);
}
