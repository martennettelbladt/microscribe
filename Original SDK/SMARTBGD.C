
/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * SMARTBGND.C   |   SDK1-2   |  November 1995
 *
 * Immersion Corp. Software Development Kit
 * Sample source code file prints stylus coordinates to the screen
 * This file demonstrates customized error handling.
 * Requires HCI firmware version MSCR1-1C or later
 */
 
#include <stdio.h>
#include <math.h>
#include "hci.h"
#include "arm.h"
  


/* USED FOR CUSTOM ERROR HANDLING:
 *  This example shows how to improve the handling of TIMED_OUT errors.
 *  Multi-tasking systems often require this approach, because we do
 *     not have completely real-time control of the processor.
 *
 *  'timeouts' keeps track of the # of TIMED_OUT errors that have
 *     occurred in a row.  Our custom error handler will keep track
 *     by incrementing 'timeouts'
 *  When data has been received successfully, 'timeouts' is reset to zero.
 *
 *  MAX_TIMEOUTS is the # of errors in a row that will be considered
 *     unrecoverable.  Feel free to increase it if necessary.
 *
 *  We are essentially assuming that every once in
 *     a while, data will take longer than usual to come in.  We might
 *     instead have tried increasing the TIME_OUT period, but this can
 *     still be foiled by interrupted processing.
 */
int timeouts = 0;
#define MAX_TIMEOUTS 20

   
/* Declare an arm_rec to represent the 6DOF arm */
arm_rec arm;
    


/* Custom error handler:
 *   If there have not been very many TIMED_OUT conditions in a row,
 *   we will ignore the error.  Otherwise, we will call the simple
 *   handler, and behavior will be as in the other example programs.
 */
arm_result smart_TIMED_OUT(hci_rec *hci, arm_result condition)
{
    if (timeouts++ > MAX_TIMEOUTS)
	    return simple_TIMED_OUT(hci, condition);
    else return NO_PACKET_YET;
}
     


void main(void)
{
	/* --- Declaration Stuff --- */
 
	/* Reasonable default values */
	int port = 1;
	long baud = 38400L;

	/* Flag to tell us when we're ready to exit */
	int done = 0;

	/* A result code that will let us monitor the success
	 *  of each operation */
	arm_result result;
 
  
   
	/* --- Initialization Stuff --- */
	 
	/* Every arm_rec must be init'd one time at the very beginning */
	arm_init(&arm);
			 
	/* (Optional) installs simple error handlers.  You can create
	 *   your own error handlers and install them instead, but that
	 *   is an 'advanced' feature.  If you leave this call out, things
	 *   will still work, but any errors will cause the program to
	 *   exit without giving you a chance to fix them and continue.
	 */
	arm_install_simple(&arm);

	/* USE CUSTOM HANDLER INSTEAD OF SIMPLE ONE */
	arm.hci.TIMED_OUT_handler = smart_TIMED_OUT;
 
	/* Greet the user & get comm params */
	printf("\n\nImmersion 6DOF Arm sample program\n");
	printf("---------------------------------\n\n");
	printf("Which port is the interface connected to?  ");
	scanf("%d", &port);
	printf("What baud rate would you like to use? ");
	scanf("%ld", &baud);
 
	printf("\nNote: if you chose an unavailable baud rate, we will\n");
	printf("use the closest available one.\n");
		 
	/* Time to connect to the hardware */
	printf("Connecting on port %d ...\n", port);
	result = arm_connect(&arm, port, baud);
 
	if (result == SUCCESS) printf("Connection established.\n");
 


	/* --- Real Stuff --- */
 
	/* Do one foreground update just to get valid coordinate data */
	if (result == SUCCESS) result = arm_stylus_3DOF_update(&arm);

	/* Do this until there is an error or a button is pressed */
	while ( (result == SUCCESS) && !done)
	{
		/* Ask for data from Arm in BACKGROUND */
		arm_stylus_3DOF_bckg(&arm);
						 
		/* Print stylus coordinates while waiting for data */
		printf("X = %f, Y = %f, Z = %f\n",
			arm.stylus_tip.x, arm.stylus_tip.y,
			arm.stylus_tip.z);
 
		/* Keep checking until response comes back.
		 * See armbgnd.c for additional comments. */
		while((result = arm_check_bckg(&arm)) == NO_PACKET_YET);

		/* If successful, reset the timeout count. */
		if (result == SUCCESS) timeouts = 0;

		/* If a button is being pressed, we want to exit */
		if (arm.hci.buttons) done = 1;
	}
 
	/* Time to end this session.  Another session can be begun with
	 *   another call to arm_connect(), without manually resetting the
	 *   hardware. */
	arm_disconnect(&arm);
 
	/* Print diagnostic info as we exit. */
	printf("Session ended on port %d at %ld baud\n",
		arm.hci.port_num, arm.hci.baud_rate);
	printf("Exit condition: %s\n", result);
}



