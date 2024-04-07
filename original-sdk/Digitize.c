/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * DIGITIZE.C  |  SDK1-2  |  September 1996
 *
 * Immersion Corp. Software Developer's Kit
 *      Sample source code file of digitizing functions
 *      Requires HCI firmware version MSCR1-1C or later
 */

#include <stdio.h>
#include <conio.h>
#include "hci.h"
#include "arm.h"


/* Declare an arm_rec to represent the 6DOF arm */
arm_rec arm;

int port = 1;
long int baud = 9600L;

arm_result ChangePortBaud(void);
void DigitizeDemo1(void);
void DigitizeDemo2(void);


void DigitizeDemo1(void)
{
	int Button = 0;

   printf("\nDepress left pedal to sample points.\n");
   printf("Depress right pedal to sample last point and quit.\n\n");
   while (!kbhit() && (Button != RIGHT_PEDAL)) {
		Button = GetPoint(&arm);
     	if (Button) {
			printf("X = %f, Y = %f, Z = %f, ",
				arm.stylus_tip.x, arm.stylus_tip.y,arm.stylus_tip.z);
			if (Button == LEFT_PEDAL) printf("LEFT\n");
			if (Button == RIGHT_PEDAL) printf("RIGHT\n");
			if (Button == BOTH_PEDALS) printf("BOTH\n");
        	}
   	}
   if (kbhit()) getch();
}


void DigitizeDemo2(void)
{
	float DistanceSetting;
	int Button = 0;

	printf("\nEnter Auto Plot Distance: \n");
	scanf("%f",&DistanceSetting);
	printf("\nHold left pedal down to autosample points.\n");
   printf("Depress right pedal to sample last point and quit.\n\n");
  	while ( !kbhit() && (Button != RIGHT_PEDAL) ) {
		Button = AutoPlotPoint(&arm, DistanceSetting);
     	if (Button) {
			printf("X = %f, Y = %f, Z = %f, D = %f, ",
				arm.stylus_tip.x, arm.stylus_tip.y,
				arm.stylus_tip.z, arm.pt2ptdist);
			if (Button == LEFT_PEDAL) printf("LEFT\n");
			if (Button == RIGHT_PEDAL) printf("RIGHT\n");
         }
		}
   if (kbhit()) getch();
}

arm_result ChangePortBaud(void)
{
	printf("Which COM Port? (1 or 2): \n");
	scanf("%d", &port);
	if( port != 2) port=1;
	printf("Baud Rate (1200,2400,9600,19200,38400,etc): \n");
	scanf("%ld", &baud);
	arm_disconnect(&arm);
	return(arm_connect(&arm, port, baud));
}



void main(void)
{
	int choice;
   int quit = 0;
	arm_result result;

	printf("MicroScribe Digitizing Sample Program\n");
	printf("Which Serial Port? (1=modem, 2=printer): \n");
	scanf("%d", &port);
	if( port != 2) port=1;

	arm_init(&arm);
	result = arm_connect(&arm, port, baud);

	while ( !quit ) {
		clrscr();
		if (result == SUCCESS)
			printf("Connected on COM%d\n", port);
		else
			printf("Unable to connect on COM%d at %ld baud\n", port, baud );
		gotoxy(1,3);
		printf("Choose a function:\n");
		printf(" 3 - Autosample points w/ left pedal\n");
		printf(" 2 - Get discrete point w/ left pedal\n");
		printf(" 1 - Change port/baud settings\n");
		printf(" 0 - Quit\n");
		scanf("%d",&choice);
		gotoxy(1,1);
		clrscr();

		switch(choice) {
			case 0:
				quit = 1;
				break;
			case 1:
				result = ChangePortBaud();
				break;
			case 2:
				DigitizeDemo1();
				break;
			case 3:
				DigitizeDemo2();
				break;
			}
		}

	arm_disconnect(&arm);
	printf("Session ended on port %d at %ld baud\n",
		arm.hci.port_num, arm.hci.baud_rate);
	printf("Exit condition: %s\n", result );
}


