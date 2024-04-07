/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * MSTestMac.C   |   Rev 2.0   |   September 1996 
 *
 * Mac/PowerMac Test program for MicroScribe 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <console.h>
#include <math.h>
#include "hci.h"
#include "arm.h"

arm_rec arm;

FILE* fp; 
int port = 1;
long int baud = 9600L;

void print_strings(void);
void print_encoders(void);
void print_angles(void);
void print_xyz(void);
arm_result ChangePortBaud(void);


void print_strings(void)
{
	fprintf(fp,"Product Name:     %s\n", arm.hci.product_name);
	fprintf(fp,"Product ID:       %s\n", arm.hci.product_id);
	fprintf(fp,"Model Name:       %s\n", arm.hci.model_name);
	fprintf(fp,"Serial Number:    %s\n", arm.hci.serial_number);
	fprintf(fp,"Comment:          %s\n", arm.hci.comment);
	fprintf(fp,"Parameter Format: %s\n", arm.hci.param_format);
	fprintf(fp,"Firmware Version: %s\n", arm.hci.version);
	return;
}


void print_encoders(void)
{
	arm_result result = SUCCESS;

	fprintf(fp,"Joint Encoder Counts. Hit right footpedal to quit");

	result = arm_6joint_update( &arm );

	while( (arm.hci.buttons != 1)  && (result == SUCCESS)){
		hci_std_cmd( &(arm.hci), 0, 0, 7);
		result = hci_wait_packet(&arm.hci);
		if( result == SUCCESS ){
			cgotoxy(1,3,fp);
			fprintf(fp,"Shoulder Yaw:\t %d (16384 PPR)\t\t\n", arm.hci.encoder[0] );
			fprintf(fp,"Shoulder Pitch:\t %d (16384 PPR)\t\t\n", arm.hci.encoder[1] );
			fprintf(fp,"Elbow:\t\t %d (8192 PPR)\t\t\n", arm.hci.encoder[2] );
			fprintf(fp,"Wrist Roll:\t %d (8192 PPR)\t\t\n", arm.hci.encoder[3] );
			fprintf(fp,"Wrist Pitch:\t %d (4095 PPR)\t\t\n", arm.hci.encoder[4] );
			fprintf(fp,"\n");
			fprintf(fp,"Aux/Expansion:\t %d (16384 PPR)\t\t\n", arm.hci.encoder[6] );
		}
	}
	return;
}

void print_angles(void)
{
	arm_result result = SUCCESS;

	fprintf(fp,"Joint Angles. Hit right footpedal to quit");

	result = arm_6joint_update( &arm );

	while( (arm.hci.buttons != 1) && (result == SUCCESS)){
		result = arm_6joint_update( &arm );
		if( result == SUCCESS ){
			cgotoxy(1,3,fp);
			fprintf(fp,"Shoulder Yaw:\t %3.0f degrees\t\n", arm.joint_deg[0] );
			fprintf(fp,"Shoulder Pitch:\t %3.0f degrees\t\n", arm.joint_deg[1] );
			fprintf(fp,"Elbow:\t\t %3.0f degrees\t\n", arm.joint_deg[2] );
			fprintf(fp,"Wrist Roll:\t %3.0f degrees\t\n", arm.joint_deg[3] );
			fprintf(fp,"Wrist Pitch:\t %3.0f degrees\t\n", arm.joint_deg[4] );
		}
	}
	return;
}

void print_xyz(void)
{
	arm_result result = SUCCESS;

	fprintf(fp,"XYZ/Button Data. Hit right footpedal to quit");

	result=arm_stylus_3DOF_update(&arm);
	while( (arm.hci.buttons != 1) && (result == SUCCESS) ) {
		result=arm_stylus_3DOF_update(&arm);
		if (result == SUCCESS) {
			cgotoxy(1,5,fp);
			fprintf(fp,"Pedal: %2X\n", arm.hci.buttons );
			fprintf(fp,"X: %3.2f Y: %3.2f Z: %3.2f\t\n", arm.stylus_tip.x, arm.stylus_tip.y, arm.stylus_tip.z);
			hci_std_cmd( &(arm.hci), 0, 0, 7);
			if( hci_wait_packet(&arm.hci)==SUCCESS ){
				fprintf(fp,"Aux port: %d\t\n", arm.hci.encoder[6] );
				}
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
	arm_result result = SUCCESS;
	int choice;
	int quit = 0;

	fp = fopenc();
	
	fprintf(fp,"MicroScribe Functionality Test Program\n");
	fprintf(fp,"Which Serial Port? (1=modem, 2=printer): \n");
	fscanf(fp,"%d", &port);
	if( port != 2) port=1;

	arm_init(&arm);
	result = arm_connect(&arm, port, baud);

	while ( !quit ) {
		cgotoxy(1,1,fp);
		ccleos(fp);
		if (result == SUCCESS) 
			fprintf(fp,"Connected on serial port %d\n", port);
		else 
			fprintf(fp,"Unable to connect on Serial Port %d at %ld baud\n", port, baud );
		cgotoxy(1,11,fp);
		print_strings();
		cgotoxy(1,3,fp);
		fprintf(fp,"Choose a function:\n");
		fprintf(fp," 4 - Display joint encoder counts\n");	
		fprintf(fp," 3 - Display joint angles\n");	
		fprintf(fp," 2 - Display XYZ & button data\n");	
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
				cgotoxy(1,11,fp);
				print_strings();
				cgotoxy(1,1,fp);
				print_xyz();
				break;
			case 3:
				print_angles();
				break;
			case 4:
				print_encoders();
				break;
			}
		}	

	arm_disconnect(&arm);
	fprintf(fp,"Session ended on port %d at %ld baud\n",
		arm.hci.port_num, arm.hci.baud_rate);
	fprintf(fp,"Exit condition: %s\n", result );
	return;
}


