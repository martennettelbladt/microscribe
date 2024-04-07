/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * MSTestPC.C   |   Rev 2.0   |   January 1996
 *
 * Developed for CE Compliance testing
 *      Requires HCI firmware version 2.0 or later
 */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "hci.h"
#include "arm.h"

arm_rec arm;

int port = 1;
long int baud = 9600L;
int quit;


void print_strings(void)
{
	printf("Product Name:     %s\n", arm.hci.product_name);
	printf("Product ID:       %s\n", arm.hci.product_id);
	printf("Model Name:       %s\n", arm.hci.model_name);
	printf("Serial Number:    %s\n", arm.hci.serial_number);
	printf("Comment:          %s\n", arm.hci.comment);
	printf("Parameter Format: %s\n", arm.hci.param_format);
	printf("Firmware Version: %s\n", arm.hci.version);
	return;
}


void print_encoders(void)
{
	arm_result result = SUCCESS;

	printf("Joint Encoder Counts. Hit any key to quit.");

	result = arm_6joint_update( &arm );

	while( !kbhit() && (result == SUCCESS)){
		hci_std_cmd( &(arm.hci), 0, 0, 7);
		result = hci_wait_packet(&arm.hci);
		if( result == SUCCESS ){
			gotoxy(1,3);
			printf("Shoulder Yaw:\t %d (16384 PPR)\t\t\n", arm.hci.encoder[0] );
			printf("Shoulder Pitch:\t %d (16384 PPR)\t\t\n", arm.hci.encoder[1] );
			printf("Elbow:\t\t %d (8192 PPR)\t\t\n", arm.hci.encoder[2] );
			printf("Wrist Roll:\t %d (8192 PPR)\t\t\n", arm.hci.encoder[3] );
			printf("Wrist Pitch:\t %d (4095 PPR)\t\t\n", arm.hci.encoder[4] );
			printf("\n");
			printf("Aux/Expansion:\t %d (16384 PPR)\t\t\n", arm.hci.encoder[6] );
			}
		}
   if (kbhit()) getch();
}

void print_angles(void)
{
	arm_result result = SUCCESS;

	printf("Joint Angles. Hit any key to quit.");

	result = arm_6joint_update( &arm );

	while( !kbhit() && (result == SUCCESS)){
		result = arm_6joint_update( &arm );
  		if( result == SUCCESS ){
			gotoxy(1,3);
			printf("Shoulder Yaw:\t %3.0f degrees\t\n", arm.joint_deg[0] );
			printf("Shoulder Pitch:\t %3.0f degrees\t\n", arm.joint_deg[1] );
			printf("Elbow:\t\t %3.0f degrees\t\n", arm.joint_deg[2] );
			printf("Wrist Roll:\t %3.0f degrees\t\n", arm.joint_deg[3] );
			printf("Wrist Pitch:\t %3.0f degrees\t\n", arm.joint_deg[4] );
			}
		}
   if (kbhit()) getch();
}

void print_xyz(void)
{
	arm_result result = SUCCESS;

	printf("XYZ/Button Data. Hit any key to quit");

	result=arm_stylus_6DOF_update(&arm);
	while( !kbhit() && (result == SUCCESS) ) {
		result=arm_stylus_6DOF_update(&arm);
		if (result == SUCCESS) {
			gotoxy(1,5);
			printf("Pedal: %2X\n", arm.hci.buttons );
			printf("X: %3.2f Y: %3.2f Z: %3.2f\t\n", arm.stylus_tip.x, arm.stylus_tip.y, arm.stylus_tip.z);
			printf("Roll: %3.3f Pitch: %3.3f Yaw: %3.3f\t\n",arm.stylus_dir.x, arm.stylus_dir.y, arm.stylus_dir.z);
			printf("Stylus Vector: [%3.3f,%3.3f,%3.3f]\t\n",arm.T[0][2],arm.T[1][2],arm.T[2][2]);
			hci_std_cmd( &(arm.hci), 0, 0, 7);
			if( hci_wait_packet(&arm.hci)==SUCCESS ){
				printf("Aux port: %d\t\n", arm.hci.encoder[6] );
				}
			}
		}
   if (kbhit()) getch();
}

arm_result ChangePortBaud(void)
{
	printf("Which Serial Port? (1=modem, 2=printer): \n");
	scanf("%d", &port);
	if( port != 2) port=1;
	printf("Baud Rate (1200,2400,9600,19200,38400,etc): \n");
	scanf("%ld", &baud);
	arm_disconnect(&arm);
	return(arm_connect(&arm, port, baud));
}

void main(void)
{
	arm_result result = SUCCESS;
   char choice;
	int quit = 0;

	printf("MicroScribe Functionality Test Program\n");
	printf("Which COM Port? (1 or 2): ");
	scanf("%d", &port);
	if( port != 2) port=1;

	arm_init(&arm);
	result = arm_connect(&arm, port, baud);
	while ( !quit ) {
		clrscr();
		if (result == SUCCESS)
			printf("Connected on COM%d\n", port);
		else
			printf("Unable to connect on Serial Port %d at %ld baud\n", port, baud );
		gotoxy(1,11);
		print_strings();
		gotoxy(1,3);
		printf("Choose a function:\n");
	  	printf(" 4 - Display joint encoder counts\n");
		printf(" 3 - Display joint angles\n");
		printf(" 2 - Display XYZ, orientation, & button data\n");
		printf(" 1 - Change port/baud settings\n");
		printf(" 0 - Quit\n");
		choice = getch();
		clrscr();

		switch(choice) {
			case '0':
				quit = 1;
				break;
			case '1':
				result = ChangePortBaud();
				break;
			case '2':
				gotoxy(1,11);
				print_strings();
				gotoxy(1,1);
				print_xyz();
				break;
			case '3':
				print_angles();
				break;
			case '4':
				print_encoders();
				break;
			}
		}
	arm_disconnect(&arm);
	printf("Session ended on port %d at %ld baud\n",
		arm.hci.port_num, arm.hci.baud_rate);
	printf("Exit condition: %s\n", result );
	return;
}


