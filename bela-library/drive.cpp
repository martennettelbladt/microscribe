/**************************************************
   - - - P E G G Y   I N S T R U M E N T S - - -
*                                                 *
         MicroScribe BELA Driver
*                                                 *
***************************************************
   DRIVE.CPP | April 2024 | Mårten Nettelbladt
*/

#include <Bela.h>
#include <libraries/Serial/Serial.h> // Dev branch at the moment including function available()
#include <sys/time.h>
#include <stdio.h>

extern "C" {
	#include "drive.h"
}

struct timeval time_now;
const int maxLen = 128;
unsigned long timeout_start;

Serial gSerial;

/*------------------*/
/* Timing Functions */
/*------------------*/

//   H O S T _ P A U S E
// host_pause() pauses for the given number of seconds
void host_pause(float delay_sec) {

	sleep(delay_sec);

}


//   H O S T _ G E T _ T I M E O U T
// host_get_timeout() gets the timeout period of the given port in seconds
float   host_get_timeout(int port) {
  return 1;
  // can't see this function being called anywhere
}

//   H O S T _ S E T _ T I M E O U T
// host_set_timeout() sets the length of all future timeout periods to the given # of seconds
void host_set_timeout(int port, float timeout_sec) {
  //SerialArm.setTimeout(timeout_sec * 1000);
  // blank
  //gSerial.setTimeout(timeout_sec);
  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< FIX?
}


//   H O S T _ S T A R T _ T I M E O U T
// host_start_timeout() starts a timer for the specified port.
// Call timed_out_yet() to find out whether time is up.
void host_start_timeout(int port) {
  //timeout_start = millis();
  timeout_start = gettimeofday(&time_now, NULL);
  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}


//   H O S T _ T I M E D _ O U T
// host_timed_out() returns True if the previously-started timeout
// period is over.  Returns False if not.
int host_timed_out(int port) {
  return ((gettimeofday(&time_now, NULL) - timeout_start) > 3);
  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}


/*----------------------*/
/* Serial i/o Functions */
/*----------------------*/

/*--------------------------------*/
/* Fixing up baud rate parameters */
/*--------------------------------*/


//   H O S T _ F I X _ B A U D
// host_fix_baud() finds nearest valid baud rate to the one given.
// Takes small arguments as shorthand:
// 115 --> 115200, 38 or 384 --> 38400, 96 --> 9600 etc.
void host_fix_baud(long int *baud) {
  // unify baud rate formats
}

/*--------------------------*/
/* Configuring Serial Ports */
/*--------------------------*/


//   H O S T _ O P E N _ S E R I A L
// host_open_serial() opens the given serial port with specified baud rate
// Always uses 8 data bits, 1 stop bit, no parity.
// Returns False (zero) if called with zero baud rate.
int host_open_serial(int port, long int baud) {
  if (baud == 0) {
    return 0;
  }
  
  rt_printf("host_open_serial inside drive.cpp\n"); //-----------------------------
  
  if (gSerial.setup ("/dev/ttyS4", baud) == 0) {
  	rt_printf("gSerial.setup() returned 0\n"); //-----------------------------
  	return 1;
  }
  else {
  	return 0;
  }
  
}


//   H O S T _ C L O S E _ S E R I A L
// host_close_serial() closes the given serial port.
//  NEVER call this without first calling host_open_serial() on the same port.
void host_close_serial(int port) {
  //SerialArm.end();
  // blank
  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
}


//   H O S T _ F L U S H _ S E R I A L
// host_flush_serial() flushes and resets the serial i/o buffers
void host_flush_serial(int port) {
	/*
	char tempBuffer[maxLen];
	int ret = gSerial.read(tempBuffer, maxLen, 100);
	while (ret > 0)
	{	
		// Do nothing
	}
	*/
}

/*------------------*/
/* Input and Output */
/*------------------*/


//   H O S T _ R E A D _ C H A R
// host_read_char() reads one character from the serial input buffer.
// returns -1 if input buffer is empty
int host_read_char(int port) {
	
	char tempBuffer[maxLen];
	int ret = gSerial.read(tempBuffer, 1, 100);
	if (ret > 0)
	{	
		int ch = tempBuffer[0];
		
			
		//rt_printf(tempBuffer);
		
		return ch;
	}
	//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

	else {
		return -1;
	}
}


//   H O S T _ R E A D _ B Y T E S
// host_read_bytes() will try to read a specified number of bytes
// until the timeout period of time expires.  It returns the number
// of bytes it actually read.
int host_read_bytes(int port, char *buf, int count, float timeout) {
  // host_set_timeout(port, timeout);
  // Serial.print("HOST_READ_BYTES");
  // return SerialArm.readBytesUntil('\0', buf, count);
  int read;
  int ch;

  /* setup the timeout */
  read = 0;
  host_set_timeout(port, timeout);
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


//   H O S T _ W R I T E _ C H A R
// host_write_char() writes one character to the serial output buffer
// Returns False (zero) if buffer is full
// Returns True (non-zero) if successful
int host_write_char(int port, int ch) {
	
  char c = ch;	
  gSerial.write(&c, 1);
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

  return 1;
}


//   H O S T _ W R I T E _ S T R I N G
// host_write_string() writes a null-terminated string to the output buffer
// Returns False (zero) if not enough rooom
// Returns True (non-zero) if successful
int host_write_string(int port, char *str) {
	
	rt_printf("writing string from drive.cpp \n"); //--------------------------------------------------------	
  gSerial.write(str);
  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  
  return 1;
  // Is this correct? Should NULL be added?
}

/*----------------------------*/
/* Getting Serial Port Status */
/*----------------------------*/


//   H O S T _ P O R T _ V A L I D
// host_port_valid() returns True if the specified port number is valid
int host_port_valid(int port) {
  return true;
  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< FIX?
}


//   H O S T _ I N P U T _ C O U N T
// host_input_count() returns the number of chars waiting in the input queue
int host_input_count(int port) {
  //return SerialArm.available();
  
  return gSerial.available();
  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< FIX?
}

//   H O S T _ I N P U T _ F U L L
// host_input_full() tells whether or not the serial input queue is full
int host_input_full() {
	return false;
  //return ((64 - SerialArm.available()) == 0);
  //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< FIX?
}
