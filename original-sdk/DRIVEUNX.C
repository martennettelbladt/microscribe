/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *               UNIX software series              *
 *                Copyright (c) 1993               *
 ***************************************************
 * DRIVEUNX.C   |   SDK1-2a   |   January 1996
 *
 * Immersion Corp. Software Developer's Kit
 *      Unix/SGI-specific serial communications functions
 *  	  for the Immersion Corp. MicroScribe-3D
 *		  Not for use with the Probe or Personal Digitizer
 *      Requires HCI firmware version MSCR1-1C or later
 */

#include <stdio.h>
#include <termio.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stropts.h>
#include "drive.h"

#define NUM_PORTS       2

/* Names of serial port device files */

/* Fill in your UNIX system type from the following choices:
 *   SGI_PLATFORM, SUN_PLATFORM
 * If yours is not listed, it can be added.  Contact Immersion Corp.
 */
#define SGI_PLATFORM

#ifdef SGI_PLATFORM
#define PORT1_DEV  "/dev/ttyd1"
#define PORT2_DEV  "/dev/ttyd2"
#define OFLAGS  (OPOST | OLCUC | ONLCR | OCRNL | ONOCR  \
			| ONLRET | OFILL | OFDEL)
#define LFLAGS  (ICANON | ISIG | XCASE | ECHO | ECHOE | ECHOK  \
			| ECHONL | NOFLSH)
#define IFLAGS  (IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK | INLCR  \
			| ICRNL | IGNCR | IUCLC | ISTRIP | IXON  \
			| IXOFF)
#endif

#ifdef SUN_PLATFORM
#define PORT1_DEV  "/dev/ttya"
#define PORT2_DEV  "/dev/ttyb"
#define OFLAGS  (OPOST | OLCUC | ONLCR | OCRNL | ONOCR  \
			| ONLRET | OFILL | OFDEL)
#define LFLAGS  (ICANON | ISIG | XCASE | ECHO | ECHOE | ECHOK  \
			| ECHONL | NOFLSH | TOSTOP | ECHOCTL  \
			| ECHOPRT | ECHOKE | IEXTEN)
#define IFLAGS  (IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK  \
			| INLCR | ICRNL | IGNCR | IUCLC | ISTRIP  \
			| IXON | IXOFF | IMAXBEL)
#endif




/* Use timeval (usec) ticks for timing out on receive attempts */
static struct timeval   timeout[NUM_PORTS+1];
static struct timeval   stop[NUM_PORTS+1];


/*------------------*/
/* Timing Functions */
/*------------------*/


/* host_pause() pauses for the given number of seconds
 */
void host_pause(float delay_sec)
{
	struct timeval  start, now, stop;
	struct timezone tz;
	int     delay_int = (int) delay_sec;
	long int        delay_frac
		= (long int) ((delay_sec - (float) delay_int) * 1e+6);

	gettimeofday(&start, &tz);
	stop.tv_sec = start.tv_sec + delay_int;
	stop.tv_usec = start.tv_usec + delay_frac;
	if (stop.tv_usec >= 1000000L)
	{
		stop.tv_sec++;
		stop.tv_usec -= 1000000L;
	}

	gettimeofday(&now, &tz);

	while(now.tv_sec < stop.tv_sec) gettimeofday(&now, &tz);
	while( (now.tv_sec == stop.tv_sec) && (now.tv_usec < stop.tv_usec) )
		gettimeofday(&now, &tz);
}


/* host_get_timeout() gets the timeout period of the given port in seconds
 */
float host_get_timeout(int port)
{
	return (float) timeout[port].tv_sec + (float) timeout[port].tv_usec * 1e-6;
}


/* host_set_timeout() sets the length of all future timeout periods
 *   to the given # of seconds
 */
void host_set_timeout(int port, float timeout_sec)
{
	timeout[port].tv_sec = (long int) timeout_sec;
	timeout[port].tv_usec
		= (long int) ((timeout_sec - (float) timeout[port].tv_sec) * 1e+6);
}


/* host_start_timeout() starts a timer for the specified port.
 *   Call host_timed_out() to find out whether time is up.
 */
void host_start_timeout(int port)
{
	struct timeval  start;
	struct timezone tz;

	gettimeofday(&start, &tz);
	stop[port].tv_sec = start.tv_sec + timeout[port].tv_sec;
	stop[port].tv_usec = start.tv_usec + timeout[port].tv_usec;
	if (stop[port].tv_usec >= 1000000L)
	{
		stop[port].tv_sec++;
		stop[port].tv_usec -= 1000000L;
	}

}


/* host_timed_out() returns True if the previously-started timeout
 *   period is over.  Returns False if not.
 */
int host_timed_out(int port)
{
	struct timeval now;
	struct timezone tz;

	gettimeofday(&now, &tz);
	return ( (now.tv_sec > stop[port].tv_sec)
		|| ( (now.tv_sec == stop[port].tv_sec)
			&& (now.tv_usec >= stop[port].tv_usec) ) );
}



/*----------------------*/
/* Serial i/o Functions */
/*----------------------*/

#define FRAME_BUF_SIZE  240
struct termio  old_setup[NUM_PORTS+1];
int     port_ref[NUM_PORTS+1];
char    frame_buffer[NUM_PORTS+1][FRAME_BUF_SIZE];
int     frame_head[NUM_PORTS+1] = { 0, 0};      /* chars come in here */
int     frame_tail[NUM_PORTS+1] = { 0, 0};      /* chars are read out here */


int host_get_id(int port)
{
	return 0;
}


/*--------------------------------*/
/* Fixing up baud rate parameters */
/*--------------------------------*/


/* host_fix_baud() finds nearest valid baud rate to the one given.
 *    Takes small arguments as shorthand:
 *      38 or 384 --> 38400, 96 --> 9600 etc.
 */
void host_fix_baud(long int *baud)
{
	switch(*baud)
	{
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
		case 9600L:
		case 96L:
			*baud = 9600L;
			break;
		default:
			if (*baud < 1000L) *baud *= 1000;
			else if (*baud > 28800L) *baud = 38400L;
			else if (*baud > 14400L) *baud = 19200L;
			else *baud = 9600L;
			break;
	}
}



/*--------------------------*/
/* Configuring Serial Ports */
/*--------------------------*/


/* host_open_serial() opens the given serial port with specified baud rate
 *    Always uses 8 data bits, 1 stop bit, no parity.
 *    Assumes port number is valid and baud rate has been fixed up.
 *    Returns False (zero) if can't open the port.
 */
int host_open_serial(int port, long int baud)
{
	int     result = 0;     /* Assume the port will not be accessible */
	tcflag_t        baud_code;
	struct termio  new_setup;

	/* Get reference number for the desired port */
	if (port == 1)
	{
		port_ref[port] = open(PORT1_DEV, O_RDWR);
	}
	else
	{
		port_ref[port] = open(PORT2_DEV, O_RDWR);
	}

	/* If port and its parameters are accessible, reconfigure for Probe */
	if ((port_ref[port] >= 0)
		&& (ioctl(port_ref[port], TCGETA, &old_setup[port]) >= 0))
	{
		/* Save original set-up */
		new_setup = old_setup[port];

		/* Turn off all post-processing and char translation */
		new_setup.c_oflag &= ~OFLAGS;
		new_setup.c_lflag &= ~LFLAGS;
		new_setup.c_iflag &= ~IFLAGS;

		/* Set for no waiting during char read */
		new_setup.c_cc[VMIN] = 0;
		new_setup.c_cc[VTIME] = 0;

		/* Find appropriate baud rate flags */
		switch(baud)
		{
			case 9600L:
				baud_code = B9600;
				break;
			case 19200L:
				baud_code = B19200;
				break;
			case 38400L:
				baud_code = B38400;
				break;
		}

		/* Set for baud rate, 8 data bits, 1 stop bit, no parity */
		new_setup.c_cflag = baud_code | CS8 | CREAD;

		/* Set these new parameters.  Record success if it works. */
		if (ioctl(port_ref[port], TCSETA, &new_setup) >= 0)
		{
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
	ioctl(port_ref[port], TCSETA, &old_setup[port]);
}


/* host_flush_serial() flushes and resets the serial i/o buffers
 */
void host_flush_serial(int port)
{
	ioctl(port_ref[port], TCFLSH, 2);      /* 2 means flush both buffers */
}



/*------------------*/
/* Input and Output */
/*------------------*/



/* host_read_char() reads one character from the serial input buffer.
 *    returns -1 if input buffer is empty
 */
int host_read_char(int port)
{
	char    ch;

	if (frame_head[port] > frame_tail[port])
	{
		ch = frame_buffer[port][frame_tail[port]++];
		if (frame_head[port] == frame_tail[port])
			frame_head[port] = frame_tail[port] = 0;
		return (unsigned char) ch;
	}
	else
	{
		if (read(port_ref[port], &ch, 1)) return (unsigned char) ch;
		else return -1;
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
	char send_me = byt;

	/* write() returns # of chars actually written successfully */
	return (write(port_ref[port], &send_me, 1) == 1);
}


/* host_write_string() writes a null-terminated string to the output buffer
 *    Returns False (zero) if not able to write the whole string
 *    Returns True (non-zero) if successful
 */
int host_write_string(int port, char *str)
{
	char *temp = str;
	int     length=0;

	/* Calculate length of null-terminated string */
	while(*temp++) length++;

	/* Write that many bytes */
	return (write(port_ref[port], str, length) == length);
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
	char    ch;

	while(read(port_ref[port], &ch, 1))
	{
		frame_buffer[port][frame_head[port]++] = ch;
	}

	return frame_head[port] - frame_tail[port];
}

int host_input_full(int port)
{
	return 0;       /* this doesn't really do the test */
}