/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *             Macintosh software series           *
 *                Copyright (c) 1993               *
 ***************************************************
 * DRIVEMAC.C   |   SDK1-2a   |   January 1996
 *
 * Immersion Corp. Software Developer's Kit
 *      Mac-Specific serial communications functions
 *  	  for the Immersion Corp. MicroScribe-3D
 *		  Not for use with the Probe or Personal Digitizer
 *      Requires HCI firmware version MSCR1-1C or later
 */

#include <stdio.h>
#include "string.h"
#include "drive.h"
#include "Serial.h"
#include "Devices.h"
#include "Events.h"
#include "Timer.h"
#include "Types.h"
#include "Controls.h"

#define CANNOT_DO   1
#define SHOWERR                 display_error_message(errcode);
#define BEEP                    SysBeep(5)

#define MODEM_PORT 1
#define PRINTER_PORT 2
#define NUM_PORTS 2

/* the following #defines are related to PBserialdemo code from Apple */
#define rPortOpenALRT   256
#define kSerBufSize             16384
#define kSerHShakeDTR   14
#define kReset                  2
#define drvrName                0x12

short           gSysVersion;

int             port_in_ref[NUM_PORTS+1], port_out_ref[NUM_PORTS+1];
int             port_config[NUM_PORTS+1];

OSErr   AssertDrvrOpen (Str255 name, short *refNum);
static void display_error_message(OSErr errcode);



/*------------------*/
/* Timing Functions */
/*------------------*/


static long int timeout_ticks[NUM_PORTS+1];
static long int stop_tick[NUM_PORTS+1];


/* host_pause() pauses for the given number of seconds
 */
void host_pause(float delay_sec)
{
	long int        final_ticks;

	Delay( (int) (delay_sec / 1.33e-2), &final_ticks);
}


/* host_get_timeout() gets the timeout period of the given port in seconds
 */
float host_get_timeout(int port)
{
	return (float) timeout_ticks[port] * 1.33e-2;
}


/* host_set_timeout() sets the length of all future timeout periods
 *   to the given # of seconds
 */
void host_set_timeout(int port, float timeout_sec)
{
	timeout_ticks[port] = (long int) (timeout_sec / 1.33e-2);
	if (timeout_ticks[port] == 0L) timeout_ticks[port] = 1L;
}


/* host_start_timeout() starts a timer for the specified port.
 *   Call timed_out_yet() to find out whether time is up.
 */
void host_start_timeout(int port)
{
	long int        now_tick;

	Delay(0, &now_tick);
	stop_tick[port] = now_tick + timeout_ticks[port];
}


/* host_timed_out() returns True if the previously-started timeout
 *   period is over.  Returns False if not.
 */
int host_timed_out(int port)
{
	long int        now_tick;

	Delay(0, &now_tick);
	return now_tick > stop_tick[port];
}

/* dummy function -> needed for windows */
int host_get_id(int port)
{
	return 0;
}


/*----------------------*/
/* Serial i/o Functions */
/*----------------------*/


/*--------------------------------*/
/* Fixing up baud rate parameters */
/*--------------------------------*/


/* host_fix_baud() finds nearest valid baud rate to the one given.
 *    Takes small arguments as shorthand:
 *      19 or 192 --> 19200, 96 --> 9600 etc.
 */
void host_fix_baud(long int *baud)
{
	switch(*baud)
	{
		case 96L:
		case 9600L:
			*baud = 9600L;
			break;
		case 19L:
		case 192L:
			*baud = 19200L;
			break;
		case 57L:
		case 576L:
			*baud = 57600L;
			break;
		default:
			if (*baud < 1000L) *baud *= 1000;
			if (*baud > 38400L) *baud = 57600L;
			else if (*baud > 14400L) *baud = 19200L;
			else *baud = 9600L;
			break;
	}
}



/*--------------------------*/
/* Configuring Serial Ports */
/*--------------------------*/


/* host_open_serial() opens the RAM Serial Drivers for either port
 *    at the specified baud rate, using 8 data bits, 1 stop bit,
 *    and no parity
 */
int host_open_serial(int port, long int baud)
{
	OSErr errcode;
	int     RMSPortRef;
	OSErr   openOutErr,openInErr;
	OSErr   setBufErr,setCfgErr,setHskErr;
	SerShk  hskFlags;
	long    finalTicks;
	Boolean    takeOverPort = true;
	Boolean    openOut,openIn;
	char     *portNameOut, *portNameIn;

	/* set for the appropriate port */
	if (port == 2)  /* Printer Port */
	{
		RMSPortRef=sPortB;
		port_in_ref[port] = binRefNum;
		port_out_ref[port] = boutRefNum;
		portNameOut = (char*) "\p.BOut";
		portNameIn = (char*) "\p.BIn";
	}
	else    /* Modem Port */
	{
		RMSPortRef=sPortA;
		port_in_ref[port] = ainRefNum;
		port_out_ref[port] = aoutRefNum;
		portNameOut = (char*) "\p.AOut";
		portNameIn = (char*) "\p.AIn";
	}

	/*  Set baud rate  */
	switch(baud)
	{
		case 57600L:
			port_config[port] = baud57600;
			break;
		case 19200L:
			port_config[port] = baud19200;
			break;
		default:
			port_config[port] = baud9600;
			break;
	}
	port_config[port]+=data8;
	port_config[port]+=stop10;
	port_config[port]+=noParity;

	openOut = false; /* (AssertDrvrOpen(portNameOut, &port_out_ref[port]) == noErr); */
	openIn = false;  /* (AssertDrvrOpen(portNameIn, &port_in_ref[port]) == noErr); */
	/* following lines are temporary fixes until the AssertDrvrOpen bug is found */
	takeOverPort=true;
	KillIO(port_in_ref[port]);
	KillIO(port_out_ref[port]);

	if (openOut || openIn) {
		if (takeOverPort = CautionAlert(rPortOpenALRT, nil) == kReset) {
			if (openIn) {
				KillIO(port_in_ref[port]);
				CloseDriver(port_in_ref[port]);
			}
			if (openOut) {
				KillIO(port_out_ref[port]);
				CloseDriver(port_out_ref[port]);
			}
		}
	}

	if (takeOverPort) {
		openOutErr = OpenDriver(portNameOut, &port_out_ref[port]);
		openInErr = OpenDriver(portNameIn, &port_in_ref[port]);

		if (openOutErr == noErr && openInErr == noErr) {

			/* There is a bug in the Macintosh IIfx IOP serial driver, in which      */
			/* SerGetBuf() will always return zero characters, even if characters    */
			/* have been received. The bug is exhibited when a remote serial device  */
			/* attempts to send data to the Macintosh IIfx when its serial driver is */
			/* not yet open and it holding off data with hardware handshaking. In    */
			/* such a case, data will flow in immediately when the serial driver     */
			/* opens--quickly filling up the default 64-byte buffer. If the buffer   */
			/* fills up before setting a larger buffer with SerSetBuf(), SerGetBuf() */
			/* "sticks" and stubbornly maintains that there is nothing coming in.    */
			/* At 9600 baud, it takes only a little more than four ticks to fill the */
			/* input buffer. This application demonstrates the bug by virtue of the  */
			/* following delay.                                                      */

			Delay(5, &finalTicks);

			/* It's always good to first set a non-default input buffer, if desired. */
			/* There is no output buffering, so specify only the input driver.       */


			hskFlags.fXOn = false;
			hskFlags.fCTS = false;
			hskFlags.xOn = 0x11;
			hskFlags.xOff = 0x13;
			hskFlags.errs = 0;

			if (gSysVersion >= 0x0700) {
				hskFlags.evts = 0;
			}
			else {
				hskFlags.evts = breakEvent;
			}

			hskFlags.fInX = false;
			hskFlags.fDTR = false;

			/* SerHShake() does not support full DTR/CTS hardware handshaking. You   */
			/* accomplish the same thing and more with a Control call and csCode 14. */
			/* You only need to specify hskFlags once, to the output driver.         */

			setHskErr = Control(port_out_ref[port], kSerHShakeDTR, (Ptr) &hskFlags);

			/*  Maybe these lines aren't needed?  */
			/*  errcode=RAMSDOpen(RMSPortRef);
			if (errcode) SHOWERR;   */

			/*  Now reset both input and output drivers with the same configuration. */
			/*  Only a single call to the output driver is necessary since differing */
			/*  concurrent input/output baud rates are not supported.                */

			setCfgErr=SerReset(port_out_ref[port],port_config[port]); /* modem out */
			if (setCfgErr) SHOWERR;
		}
	}

	return (baud == 0L ? 0 : 1);
}


/* host_close_serial() closes the given serial port.
 *    NEVER call this without first calling host_open_serial() on the
 *       same port.
 */
void host_close_serial(int port)
{
	OSErr   killErr, closeOutErr, closeInErr;

	killErr = KillIO(port_in_ref[port]);
	closeInErr = CloseDriver(port_in_ref[port]);

	killErr = KillIO(port_out_ref[port]);
	closeOutErr = CloseDriver(port_out_ref[port]);
}


/* host_flush_serial() flushes and resets the serial i/o buffers
 */
void host_flush_serial(int port)
{
	SerSetBuf(port_out_ref[port], 0L, 0);
}



/*------------------*/
/* Input and Output */
/*------------------*/



/* host_read_char() reads one character from the serial input buffer.
 *    returns -1 if input buffer is empty
 */
int host_read_char(int port)
{
	long int length=1;
	long int check_buffer;
	unsigned char result;
	OSErr errcode;

	errcode=SerGetBuf(port_in_ref[port], &check_buffer);
	if (errcode) SHOWERR;

	if (!check_buffer)
		return (int) -1;
	else
	{
		FSRead(port_in_ref[port], &length, &result);
		return (unsigned int) result;
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
	long int length = 1;
	char sendchar = (char) byt;

	FSWrite(port_out_ref[port], &length, &sendchar);

	return length;
}


/* host_write_string() writes a null-terminated string to the output buffer
 *    Returns False (zero) if not able to write the whole string
 *    Returns True (non-zero) if successful
 */
int host_write_string(int port, char *str)
{
	char *temp = str;
    long int    length = 0;

	/* Calculate length of null-terminated string */
	while(*temp++) length++;

	/* Write that many bytes */
	FSWrite(port_out_ref[port], &length, str);

	return length;
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
	long int        num_chars;

	SerGetBuf(port_in_ref[port], &num_chars);

	return (int) num_chars;
}

int host_input_full(int port)
{
	return 0;       /* not used (only for the PC) */
}



/*  The following functions are not globally accessible  */

static void display_error_message(OSErr errcode)
{
	BEEP;
	printf("\n ERROR: ");
	switch (errcode) {
		abortErr:       printf(" I/O request aborted by KillIO");
					break;
		badUnitErr:     printf(" Driver reference number doesn't match unit table");
					break;
		controlErr:     printf(" Driver can't respond to this driver call");
					break;
		dInstErr:       printf(" Couldn't find driver in resource table");
					break;
		dRemovErr:      printf(" Attempt to remve an open driver");
					break;
		notOpenErr:     printf(" Driver isn't open");
					break;
		openErr:        printf(" Requested read/write permission not valid");
					break;
		readErr:        printf(" Driver can't respond to read calls");
					break;
		statusErr:      printf(" Driver can't respond to status call");
					break;
		unitEmptyErr:
					printf(" Driver reference number specifies NIL handle");
					break;
		writErr:        printf(" Driver can't respond to Write calls");
					break;
		default:        printf("unknown error code");
					break;
	}
	printf("\nError codes listed on page II-204 of Inside Macintosh\n");

}



OSErr AssertDrvrOpen (Str255 name, short *refNum)
{
	DCtlHandle      *pUTEntry;
	Ptr                     pDrvr;
	OSErr           result = notOpenErr;            /* assume not open */
	short           unitNo;
	char            *aDrvrName;

	/* The point here is to determine whether a driver is open, given its name.  */
	/* This allows one to check a driver to see if it's open without hard coding */
	/* its reference number. (Normally, the way to get the refNum is to open     */
	/* the driver--but that defeats the whole purpose!)                          */
	/* This is an extension of the code discussed in Tech Note #71.              */

	*refNum = 0;
	pUTEntry = *(DCtlHandle **) UTableBase;
	for (unitNo = 0; unitNo < *(short *) UnitNtryCnt; unitNo++, pUTEntry++)
	{
		if (*pUTEntry != nil && **pUTEntry != nil) {
			if (((***pUTEntry).dCtlFlags & 1 << 6) != 0)
				pDrvr = *(Handle) (***pUTEntry).dCtlDriver;
			else
				pDrvr = (***pUTEntry).dCtlDriver;

			if (pDrvr != nil) {
				aDrvrName = pDrvr + drvrName;
				if (memcmp(aDrvrName, name, 1 + name[0]) == 0) {
					/* We found the one we're after. */
					*refNum = ~unitNo;
					if (((***pUTEntry).dCtlFlags & 1 << 5) != 0)
						result = noErr;
					break;
				}
			}
		}
	}

	return result;
}
