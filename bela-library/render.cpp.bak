/*
  M I C R O S C R I B E   -   B E L A   S E T U P
  
  Mårten Nettelbladt / PEGGY INSTRUMENTS
  2024-04-07
  
  This is a basic example playing 3 sine tones with frequencies according to x, y, and z coordinates of the arm tip.

 H A R D W A R E
 Serial communication on the Microscribe uses RS-232 standard and needs to be converted to work with Bela TTL.
 This can be done with the Max3232 chip from Sparkfun.
 
 Max3232 connections:
 T1 OUT to Microscribe RX = left pin on middle row, SERIAL port
 T2 OUT -
 R1 IN to Micriscribe TX = Right pin on middle row, SERIAL port
 R2 IN -
 3V-5.5V to 3.3V on the Bela PLEASE NOTE: 5V can damage Bela, so it should be avoided!!!
 GND to GND on the Bela AND to ground on Microscribe, Center pin on middle row, SERIAL port
 T1 IN to TX uart4 on the Bela Mini, pin 7 (next to D4)
 T2 IN -
 R1 OUT to RX uart4 on the Bela Mini, pin 5 (next to D3)
 R2 OUT -
 
*/
#include <Bela.h>
#include <cmath>
#include <string.h>

extern "C" {
#include "hci.h"		// Microscribe fundamentals
#include "arm.h"		// Arm specific functions
}

#include "drive.h"		// Platfom specific functions



long baud = 115200L;
int port = 0;

float gFrequency[3];
float gPhase[3];
float gInverseSampleRate;

float joint[5];

arm_rec arm;

void serialIo(void* arg) {
	while(!Bela_stopRequested())
	{
		arm_result result;
		result = arm_stylus_6DOF_update(&arm);
		
		gFrequency[0] = arm.stylus_tip.x;
		gFrequency[1] = arm.stylus_tip.y;
		gFrequency[2] = arm.stylus_tip.z;
		
		// arm joint angles
		for(int i = 0; i < 5; i++) {
			joint[i] = arm.joint_deg[i];
		}
		
		// handle continuity i joint positions
		if (joint[2] < 180) joint[2] += 360;
		if (joint[3] < 225) joint[3] += 360; // Easy to move joint past 180, so 225 is better
		if (joint[4] < 180) joint[4] += 360;
		
	}
}

bool setup(BelaContext *context, void *userData) {
	
	gInverseSampleRate = 1.0 / context->audioSampleRate;
	gFrequency[0] = gFrequency[1] = gFrequency[2] = 220;
	gPhase[0] = gPhase[1] = gPhase[2] = 0.0;

	rt_printf("arm_init\n");
	arm_init(&arm);
	
	rt_printf("arm_install_simple\n");
	arm_install_simple(&arm);
	
	arm_result result;
	rt_printf("arm_connect\n");
	result = arm_connect(&arm, port, baud);
	rt_printf("Result: %s\n", result); //-----------------------------------------------------------------------
	
	AuxiliaryTask serialCommsTask = Bela_createAuxiliaryTask(serialIo, 0, "serial-thread", NULL);
	Bela_scheduleAuxiliaryTask(serialCommsTask);

	return true;
}

void render(BelaContext *context, void *userData) 
{
	
	for(unsigned int n = 0; n < context->audioFrames; ++n)
	{
		float out = 0;	
		
		for (int i = 0; i < 3; i++)
		{
			out += 0.2f * sinf(gPhase[i]);
			gPhase[i] += 2.0f * (float)M_PI * gFrequency[i] * gInverseSampleRate;
			while(gPhase[i] > M_PI)
			{
				gPhase[i] -= 2.0f * (float)M_PI;
			}
		}


		for(unsigned int ch = 0; ch < context->audioOutChannels; ++ch)
		{
			audioWrite(context, n, ch, out);
		}
	}
}

void cleanup(BelaContext *context, void *userData) {}
