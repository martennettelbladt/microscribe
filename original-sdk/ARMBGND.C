/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * ARMBGND.C  |  SDK1-2  |  November 1995
 *
 * Immersion Corp. Software Developer's Kit
 *      Sample source code file prints stylus coordinates to the screen
 *      Uses 'BACKGROUND' mode coordinate updates (advanced feature) 
 *      Requires HCI firmware version MSCR1-1C or later
 */

#include <stdio.h>
#include <math.h>
#include "hci.h"
#include "arm.h"


/* Declare a arm_rec to represent the Immersion 6DOF Arm */
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

	/* Variable to demonstrate 'background' operations */
	int counter = 0;



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

	/* Time to connect */
	printf("Connecting on port %d ...\n", port);
	result = arm_connect(&arm, port, baud);

	if (result == SUCCESS) printf("Connection established.\n");



	/* --- Real Stuff --- */

	/* Update the stylus position, using a 'background' update */
	arm_stylus_3DOF_bckg(&arm);

	/* Now, the hardware has not yet answered.  We can do something else
	 *   else and check for incoming data periodically.
	 */

	/* Do this until there is an error or a button is pressed
	 *   In Background mode, NO_PACKET_YET is not an error; it just
	 *   means you should continue what you were doing while waiting
	 *   for packets.
	 */
	while ( ((result == SUCCESS) || (result == NO_PACKET_YET)) && !done)
	{
		/* See if a full data packet has come in yet */
		result = arm_check_bckg(&arm);

		if (result == NO_PACKET_YET)
		{
			/* Background task is simply to count */
			counter++;
		}
		else if (result == SUCCESS)
		{
			/* Print stylus coordinates and our counter */
			printf("X = %f, Y = %f, Z = %f, counter = %d\n",
				arm.stylus_tip.x, arm.stylus_tip.y,
				arm.stylus_tip.z, counter);

			/* and request another update */
			arm_stylus_3DOF_bckg(&arm);
		}

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
