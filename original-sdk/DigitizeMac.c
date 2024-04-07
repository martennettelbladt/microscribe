/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * DigitizeMac.c  |  SDK1-2  |  September 1996
 *
 * Immersion Corp. Software Developer's Kit
 *      Sample source code file showing digitizing functions
 *      Requires HCI firmware version MSCR1-1C or later
 */

#include <stdio.h>
#include <console.h>
#include "hci.h"
#include "arm.h"


/* Declare an arm_rec to represent the 6DOF arm */
arm_rec arm;

FILE* fp; 
int port = 1;
long int baud = 9600L;

arm_result ChangePortBaud(void);
void DigitizeDemo1(void);
void DigitizeDemo2(void);

void DigitizeDemo1(void)
{
	int Button = 0;

   	fprintf(fp,"\nDepress left pedal to sample points.\n");
    fprintf(fp,"Depress right pedal to sample last point and quit.\n\n");
   	while (Button != RIGHT_PEDAL) {
		Button = GetPoint(&arm);
      	if (Button) {
			fprintf(fp,"X = %f, Y = %f, Z = %f, ",
				arm.stylus_tip.x, arm.stylus_tip.y,arm.stylus_tip.z);
			if (Button == LEFT_PEDAL) fprintf(fp,"LEFT\n");
			if (Button == RIGHT_PEDAL) fprintf(fp,"RIGHT\n");
			if (Button == BOTH_PEDALS) fprintf(fp,"BOTH\n");
         	}
         }
}


void DigitizeDemo2(void)
{
	float DistanceSetting;
	int Button = 0;

	fprintf(fp,"\nEnter Auto Plot Distance: \n");
	fscanf(fp,"%f",&DistanceSetting);
    fprintf(fp,"\nHold left pedal down to autosample points.\n");
    fprintf(fp,"Depress right pedal to sample last point and quit.\n\n");
   	while ( Button != RIGHT_PEDAL ) {
		Button = AutoPlotPoint(&arm, DistanceSetting);
      	if (Button) {
			fprintf(fp,"X = %f, Y = %f, Z = %f, D = %f, ",
				arm.stylus_tip.x, arm.stylus_tip.y,
				arm.stylus_tip.z, arm.pt2ptdist);
			if (Button == LEFT_PEDAL) fprintf(fp,"LEFT\n");
			if (Button == RIGHT_PEDAL) fprintf(fp,"RIGHT\n");
         	}
		}

}

arm_result ChangePortBaud(void) 
{
	fprintf(fp,"Which Serial Port? (1=modem, 2=printer): \n");
	fscanf(fp,"%d", &port);
	if( port != 2) port=1;
	fprintf(fp,"Baud Rate (1200,2400,9600,19200,38400,etc): \n");
	fscanf(fp,"%ld", &baud);
	arm_disconnect(&arm);
	return(arm_connect(&arm, port, baud));
}

void main(void)
{
	int choice;
   	int quit = 0;
	arm_result result;

	fp = fopenc();
	
	fprintf(fp,"MicroScribe Digitizing Sample Program\n");
	fprintf(fp,"Which Serial Port? (1=modem, 2=printer): \n");
	fscanf(fp,"%d", &port);
	if( port != 2) port=1;

	arm_init(&arm);
	result = arm_connect(&arm, port, baud);


	while ( !quit ) {
		cgotoxy(1,1,fp);
		ccleos(fp);
		if (result == SUCCESS) 
			fprintf(fp,"Connected on Serial Port %d\n", port);
		else 
			fprintf(fp,"Unable to connect on Serial Port %d at %ld baud\n", port, baud );
		cgotoxy(1,3,fp);
		fprintf(fp,"Choose a function:\n");
		fprintf(fp," 3 - Autosample points w/ left pedal\n");	
		fprintf(fp," 2 - Get discrete point w/ left pedal\n");	
		fprintf(fp," 1 - Change port/baud settings\n");
		fprintf(fp," 0 - Quit\n");
		fscanf(fp,"%d",&choice);
		cgotoxy(1,1,fp);
		ccleos(fp);

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
	fprintf(fp,"Session ended on port %d at %ld baud\n",
		arm.hci.port_num, arm.hci.baud_rate);
	fprintf(fp,"Exit condition: %s\n", result );
}


