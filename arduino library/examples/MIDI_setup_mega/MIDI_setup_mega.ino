/* 
 Mårten Nettelbladt 2024-04-07
 M I C R O S C R I B E   -   A R D U I N O   M E G A   
 M I D I   S E T U P
 5   P I N   D I N   
 
 Two serial ports are needed. One for Microscribe and one for MIDI.

 H A R D W A R E
 Serial communication on the Microscribe uses RS-232 standard and needs to be converted to work with Arduino TTL.
 This can be done with the Max3232 chip from Sparkfun.
 
 Max3232 connections:
 T1 OUT to Microscribe RX = left pin on middle row, SERIAL port
 T2 OUT -
 R1 IN to Micriscribe TX = Right pin on middle row, SERIAL port
 R2 IN -
 3V-5.5V to 5V on the Arduino Mega
 GND to GND on the Arduino Mega AND to ground on Microscribe, Center pin on middle row, SERIAL port
 T1 IN to TX1 on the Arduino Mega, pin 18
 T2 IN -
 R1 OUT to RX1 on the Arduino Mega, pin 19
 R2 OUT -

 SerialArm is defined as Serial1 in "drive.cpp" 

 5 pin DIN MIDI connections:
 DIN pin 5 through 220 ohm resistor to TX3 on the Arduino Mega, pin 14 
 DIN pin 2 to GND on the Arduino Mega
 DIN pin 4 through 220 ohm resistor to 5V on the Arduino Mega

 5 pin DIN Female front:     I

              / / I \ \
              3 5 2 4 1

 SerialMidi = Serial3


*/
#define SerialMidi Serial3

# include <hci.h>
# include <arm.h>
# include <drive.h>

arm_rec arm;

long baud = 9600L;
int port = 1;
int note = 0;

int tipX, tipY, tipZ;
int dirX, dirY, dirZ;
int joint0, joint1, joint2, joint3, joint4;



void setup() {
  // Set the MIDI baud rate	
  SerialMidi.begin(31250);

  // Initialize and connect Microscribe arm
  arm_init(&arm);
  arm_install_simple(&arm);
  arm_result result;
  result = arm_connect(&arm, port, baud);
  
}

void loop() {
   // get update from Microscribe
   arm_result result;
   result = arm_stylus_6DOF_update(&arm);

   // tip position as x, y, z coordinates
   tipX = map(arm.stylus_tip.x, 0, 500, 0, 100);
   tipY = map(arm.stylus_tip.y, -500, 500, 0, 100);
   tipZ = map(arm.stylus_tip.z, 0, 500, 0, 100);

   // arm joint angles   
   joint0 = arm.joint_deg[0];
   joint1 = arm.joint_deg[1];
   joint2 = arm.joint_deg[2];
   joint3 = arm.joint_deg[3];
   joint4 = arm.joint_deg[4];
   
   // handle continuity i joint positions
   if (joint2 < 180) joint2 += 360;
   if (joint3 < 225) joint3 += 360; // Easy to move joint past 180, so 225 is better
   if (joint4 < 180) joint4 += 360;
   
   // scale joint angles to MIDI value range 0-127
   joint0 = map(joint0, 270, 450, 0, 127);
   joint1 = map(joint1, 0, 140, 0, 127);
   joint2 = map(joint2, 270, 450, 0, 5);
   joint3 = map(joint3, 510, 210, 0, 127);
   joint4 = map(joint4, 419, 196, 0, 127);
   
	// print coordinates to Serial Monitor
	/*
    Serial.print(arm.stylus_tip.x);
    Serial.print("   ");
    Serial.print(arm.stylus_tip.y);
    Serial.print("   ");
    Serial.print(arm.stylus_tip.z);
    Serial.println(" ");
	*/

   // send midi CC, example

   midiCC(3, joint0);
   midiCC(4, joint3);
   midiCC(6, joint1);
   
   midiCC(24, tipX);
   midiCC(23, tipY);
   midiCC(26, tipZ);
   
   // send midi note
   // noteOn(60, 90);
   // notOn(60, 0); // velocity 0 = note off

}

void noteOn(int pitch, int velocity) {
  SerialMidi.write(0x90);
  SerialMidi.write(40);
  SerialMidi.write(constrain(velocity, 0, 127));
}

void midiCC(int controller, int value) {
  SerialMidi.write(0xb0);
  SerialMidi.write(controller);
  SerialMidi.write(constrain(value, 0, 127));
}
