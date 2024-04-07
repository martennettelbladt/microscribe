/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * ARMMOTN.C  |  SDK1-2  |  November 1995
 *
 * Immersion Corp. Software Developer's Kit
 *      Sample source code file prints stylus coordinates to the screen
 *      Illustrates use of MOTION-REPORTING mode
 *      Requires HCI firmware version MSCR1-1C or later
 */

#include <stdio.h>
#include <math.h>
#include "hci.h"
#include "arm.h"


/* Declare an arm_rec to represent the 6DOF arm */
arm_rec arm;


void main(void)
{
	/* --- Declaration Stuff --- */

	/* Reasonable default values */
	int port = 1;
	long baud = 38400L;

	/* Flag to tell us when we're ready to exit */
	int done = 0;

	/* A result code that will let us monitor the success
	 *  of each operation
	 */
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

	/* Greet the user & get comm params */
	printf("\n\nImmersion 6DOF Arm sample program\n");
	printf("---------------------------------\n\n");
	printf("Which port is the interface connected to?  ");
	scanf("%d", &port);
	printf("What baud rate would you like to use?  ");
	scanf("%ld", &baud);

	printf("\nNote: if you chose an unavailable baud rate, we will\n");
	printf("use the closest available one.\n");

	/* Time to connect to the hardware */
	printf("Connecting on port %d ...\n", port);
	result = arm_connect(&arm, port, baud);

	if (result == SUCCESS) printf("Connection established.\n");

	/* Get initial coordinates */
	if (result == SUCCESS) result = arm_stylus_6DOF_update(&arm);


	/* --- Real Stuff --- */

	/* Motion-generated reporting: 5 pulses, 10 ms, all buttons */
	arm_stylus_3DOF_motion(&arm, 5, 10, 0x0F);

	/* Do this until there is an error or a button is pressed */
	while ( ((result == SUCCESS) || (result == NO_PACKET_YET)) && !done)
	{
		/* Update the stylus position */
		result = arm_check_motion(&arm);

		if (result == SUCCESS)
		{
			/* Print stylus coordinates */
			printf("X = %f, Y = %f, Z = %f\n",
				arm.stylus_tip.x, arm.stylus_tip.y,
				arm.stylus_tip.z);
		}

		/* If a button is being pressed, we want to exit */
		if (arm.hci.buttons)
		{
			done = 1;
			result = SUCCESS;
		}
	}

	/* Cancel motion-reporting mode. */
	arm_end_motion(&arm);

	/* Time to end this session.  Another session can be begun with
	 *   another call to arm_connect(), without manually resetting the
	 *   hardware. */
	arm_disconnect(&arm);

	/* Print diagnostic info as we exit. */
	printf("Session ended on port %d at %ld baud\n",
		arm.hci.port_num, arm.hci.baud_rate);
	printf("Exit condition: %s\n", result);
}


