/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * ARM.C   |   SDK1-2a   |   January 1996
 *
 * Immersion Corp. Software Developer's Kit
 *      Main source file for the Immersion Corp. MicroScribe
 *      Not for use with Probe or Personal Digitizer
 *      Requires HCI firmware version MSCR1-1C or later
 *
 */

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "hci.h"
#include "arm.h"
#include "drive.h"



/*-----------------------------*/
/* Strings as enumerated types */
/*-----------------------------*/

/* Length units */
char    INCHES[7] = "inches";
char    MM[3] = "mm";

/* Angle units */
char    RADIANS[8] = "radians";
char    DEGREES[8] = "degrees";

/* Angle formats */
char    XYZ_FIXED[10] = "xyz fixed";
char    ZYX_FIXED[10] = "zyx fixed";
char    YXZ_FIXED[10] = "yxz fixed";
char    ZYX_EULER[10] = "zyx Euler";
char    XYZ_EULER[10] = "xyz Euler";
char    ZXY_EULER[10] = "zxy Euler";




/*---------------------------------*/
/* ----- Essential Functions ----- */
/*---------------------------------*/



/*----------------*/
/* Initialization */
/*----------------*/


/* arm_init() initializes an arm_rec.
 *   Call this function only once FOR EACH arm_rec in use.
 */
void arm_init(arm_rec *arm)
{
	rt_printf("arm_init inside arm.c\n"); //-----------------------------------------------------------------------
	
	/* Temporarily use default port & baud rate
	 *   We're not connecting yet; so these params
	 *   will not nec. be used for communication. */
	hci_init(&arm->hci, 1, 9600L);

	arm->len_units = MM;
	arm->ang_units = DEGREES;
	arm->ang_format = XYZ_FIXED;	/* same as ZYX_EULER */

	arm->timer_report = 0;
	arm->anlg_reports = 0;
	arm->num_points = -1;
	arm->packet_calc_fn = arm_calc_nothing;

   arm->BETA = 0.0;
}


/*----------------*/
/* Communications */
/*----------------*/


/* arm_connect() initializes an arm_rec and establishes communication
 *   with its corresponding Arm hardware.
 */
arm_result arm_connect(arm_rec *arm, int port, long int baud)
{
	rt_printf("arm_connect inside arm.c\n"); //--------------------------------------------------------
	
	arm_result result = TRY_AGAIN;

	while (result == TRY_AGAIN)
	{
		hci_com_params(&arm->hci, port, baud);
		hci_clear_packet(&arm->hci);
		result = hci_connect(&arm->hci);
		port = arm->hci.port_num;
		baud = arm->hci.baud_rate;
	}

	if (result == SUCCESS) result = arm_get_constants(arm);

	return result;
}


/* arm_disconnect() ends the current session and leaves hardware in a mode
 *    waiting for the autosynch process.  The Arm can be accessed again
 *    without manual reset by running arm_connect().
 */
void arm_disconnect(arm_rec *arm)
{
	hci_disconnect(&arm->hci);
}


/* arm_change_baud() changes the Arm's baud rate and changes the host's
 *   baud rate to match.  ANY PENDING SERIAL DATA IS LOST
 */
void arm_change_baud(arm_rec *arm, long int new_baud)
{
	hci_change_baud(&arm->hci, new_baud);
}


/*---------------------------------------*/
/* Getting data with 'foreground' method */
/*---------------------------------------*/
/* These commands request data from the Arm,
 *                wait idly until the Arm responds,
 *            and calculate appropriate arm_rec data fields.
 */

/* GetPoint(arm_rec* arm)

	Gets point from digitizer and returns LEFT_PEDAL or RIGHT_PEDAL value
   	if either footpedal is pressed
   Only returns LEFT_PEDAL or RIGHT_PEDAL once for each pedal press even
   	if pedal is held down indefinitely (i.e., software debounced)

   	xyz coordinates are stored in length3d struct arm->hci.stylus_tip
    		x - arm->hci.stylus_tip.x
	     	y - arm->hci.stylus_tip.y
   	  	z - arm->hci.stylus_tip.z
	   Footpedal status is stored in arm->hci.buttons
      	right pedal pressed - arm->hci.buttons = RIGHT_PEDAL (#define'd = 1)
      	left pedal pressed - arm->hci.buttons = LEFT_PEDAL (#define'd = 2)
      	both pedals pressed - arm->hci.buttons = BOTH_PEDALS (#define'd = 3)

	parameters:
  		arm -	pointer to arm_rec struct

   return value:
		0 - if no footpedal pressed
   	1 - if right footpedal pressed
   	2 - if left footpedal pressed
   	3 - if both footpedals pressed

	Suggestions for easy, hands-free digitizing:
   	If making a POLYLINE:
	      Use GetPoint() to gather points
   	   If no polyline currently active and return value is LEFT_PEDAL,
         	start new polyline with current point
   	   If next return value is LEFT_PEDAL, add current point
         	to current polyline
         If next return value is RIGHT_PEDAL, add current point to current
         	polyline	and end current polyline
         Start again with new polyline

	See Digitize.c for an example program
*/
int GetPoint(arm_rec* arm) {

	static int PedalReset = 1;
	arm_result result;

	result = arm_stylus_3DOF_update(arm);
	if (result != SUCCESS) return 0;
	if ( (arm->hci.buttons == LEFT_PEDAL) ||
   	  (arm->hci.buttons == RIGHT_PEDAL)) {	/* check for any footpedal press */
		if (PedalReset) {
      	PedalReset = 0;
      	return (arm->hci.buttons);		/* return which footpedal was pressed */
         }
      else
      	return 0;
      }
   else {
		PedalReset = 1;
      return 0;
      }
}

/* AutoPlotPoint(arm_rec* arm, float DistanceSetting)

	Allows sampling of multiple points at specified discrete intervals
   	while tracing stylus
   Gets point and returns LEFT_PEDAL value if current point is at least
   	DistanceSetting away from last sampled point which returned
      LEFT_PEDAL value
   DistanceSetting is in INCHES unless units are changed to MM using
   	arm_length_units(arm_rec *arm, length_units units)
   Left footpedal must be held down while tracing stylus; letting up and
   	immediately depressing the left footpedal again can cause the next
      point to be more than DistanceSetting away from the previous point
   Right footpedal returns RIGHT_PEDAL value for each pedal press (debounced),
   	even if this point is less than DistanceSetting away from last point
      that returned LEFT_PEDAL or RIGHT_PEDAL value

   	xyz coordinates are stored in length3d struct arm->hci.stylus_tip
    		x - arm->hci.stylus_tip.x
	     	y - arm->hci.stylus_tip.y
   	  	z - arm->hci.stylus_tip.z
	   footpedal status is stored in arm->hci.buttons
      	right pedal pressed - arm->hci.buttons = RIGHT_PEDAL (#define'd = 1)
      	left pedal pressed - arm->hci.buttons = LEFT_PEDAL (#define'd = 2)
      	both pedals pressed - arm->hci.buttons = BOTH_PEDALS (#define'd = 3)

	parameters:
  		arm -	pointer to arm_rec struct
		DistanceSetting - minimum distance between sampled points

   return value:
		0 - if no footpedal pressed
   	1 - if right footpedal pressed
   	2 - if left footpedal pressed
   	3 - if both footpedals pressed


	Suggestions for easy, hands-free, autoplot digitizing:
   	If making a POLYLINE:
	      Use AutoPlotPoint() to gather points
   	   If no polyline currently active and return value is LEFT_PEDAL,
         	start new polyline with current point
   	   If next return value is LEFT_PEDAL, add current point
         	to current polyline
         If next return value is RIGHT_PEDAL, add current point to current
         	polyline	and end current polyline
         Start again with new polyline

		If the left footpedal is let up then immediately depressed again,
      AutoPlotPoint() will return LEFT_PEDAL only when the current point
      is more than DistanceSetting away from the last point which caused
      AutoPlotPoint() to return LEFT_PEDAL.  This feature allows the user
      maintain the DistanceSetting spacing between sampled points even
      if the footpedal is let up then depressed again.  Only a right footpedal
      press will force AutoPlotPoint() to return a point LESS than
      DistanceSetting away.  This makes the right footpedal useful for ending
      a polyline even if the last point is less than DistanceSetting away
      from the previous point.  Otherwise, the user must sometimes move the
      stylus past the desired endpoint to cause AutoPlotPoint() to return
      LEFT_PEDAL.

	See Digitize.c for an example program
*/
int AutoPlotPoint(arm_rec* arm, float DistanceSetting) {

	static int LeftPedalReset = 1;
	static int RightPedalReset = 1;
	float tempx, tempy, tempz;
	arm_result result;

	arm->pt2ptdist = 0;

	result = arm_stylus_3DOF_update(arm);
	if (result != SUCCESS) return 0;
	if (arm->hci.buttons == LEFT_PEDAL) {
		if (LeftPedalReset) {
		 	arm->lastX = arm->stylus_tip.x;
		   arm->lastY = arm->stylus_tip.y;
   		arm->lastZ = arm->stylus_tip.z;
      	LeftPedalReset = 0;
         return (arm->hci.buttons);
         }
		else {
			tempx = arm->stylus_tip.x - arm->lastX;
			tempy = arm->stylus_tip.y - arm->lastY;
			tempz = arm->stylus_tip.z - arm->lastZ;
			arm->pt2ptdist = sqrt(tempx*tempx+tempy*tempy+tempz*tempz);
         if (arm->pt2ptdist >= DistanceSetting) {
			 	arm->lastX = arm->stylus_tip.x;
			   arm->lastY = arm->stylus_tip.y;
   			arm->lastZ = arm->stylus_tip.z;
         	return (arm->hci.buttons);
            }
         else
         	return 0;
			}
      }
	if (arm->hci.buttons == RIGHT_PEDAL) {
	   LeftPedalReset = 1;
		if (RightPedalReset) {
      	RightPedalReset = 0;
      	return (arm->hci.buttons);
         }
      else
      	return 0;
      }
	RightPedalReset = 1;
   return 0;
}

/* AutoPlotPointUndo(arm_rec* arm, float newX, float newY, float newZ)

	Sets lastX, lastY, and lastZ to X, Y, and Z.  Useful in conjunction
   	with AutoPlotPoint to maintain correct Last Point Sampled if user
      performs an Undo on a digitized AutoPlot point.
	parameters:
  		arm -	pointer to arm_rec struct
  		newX - new X coordinate for last point sampled
  		newY - new Y coordinate for last point sampled
  		newZ - new Z coordinate for last point sampled

   return value:
		none

*/
void AutoPlotPointUndo(arm_rec* arm, float newX, float newY, float newZ) {
	arm->lastX = newX;
   arm->lastY = newY;
   arm->lastZ = newZ;
}

/* BallTip(arm_rec *arm)
		adjusts stylus length from standard point tip to standard ball tip.
*/
void BallTip(arm_rec *arm) {
	float len_factor = (arm->len_units == INCHES ? 1.0 : 25.4 );
	arm->D[5] = arm->D5Point + 0.242 * len_factor;
}

/* PointTip(arm_rec *arm)
		resets stylus length to standard point tip.
*/
void PointTip(arm_rec *arm) {
	arm->D[5] = arm->D5Point;
}

/* CustomTip(arm_rec *arm, float delta)
		adjusts stylus length from standard point tip to custom tip.
*/
void CustomTip(arm_rec *arm, float delta) {
	arm->D[5] = arm->D5Point + delta;
}


/* arm_stylus_6DOF_update() updates the stylus position and direction.
 */
arm_result arm_stylus_6DOF_update(arm_rec *arm)
{
	arm_result result;

	/* Get 6 encoder values from HCI */
	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 6);
	if ( (result = hci_wait_packet(&arm->hci)) == SUCCESS)
	{
		arm_calc_joints(arm);
		arm_calc_stylus_6DOF(arm);
	}

	return result;
}


/* arm_stylus_3DOF_update() updates the stylus position.
 *   This is faster than arm_stylus_6DOF_update() but does not calculate
 *   stylus direction.
 */
arm_result arm_stylus_3DOF_update(arm_rec *arm)
{
	arm_result result;

	/* Get 6 encoder values from HCI */
	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 6);
	if ( (result = hci_wait_packet(&arm->hci)) == SUCCESS)
	{
		arm_calc_joints(arm);
		arm_calc_stylus_3DOF(arm);
	}

	return result;
}


/* arm_3joint_update() updates the first three joint angles but nothing else
 */
arm_result arm_3joint_update(arm_rec *arm)
{
	arm_result      result;

	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 3);
	if ( (result = hci_wait_packet(&arm->hci)) == SUCCESS)
		arm_calc_joints(arm);

	return result;
}


/* arm_6joint_update() updates all six joint angles but nothing else
 */
arm_result arm_6joint_update(arm_rec *arm)
{
	arm_result      result;

	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 6);
	if ( (result = hci_wait_packet(&arm->hci)) == SUCCESS)
		arm_calc_joints(arm);

	return result;
}


/* arm_full_update() updates all quantities in the arm_rec,
 *   subject to the timer_report and anlg_reports flags
 */
arm_result arm_full_update(arm_rec *arm)
{
	arm_result result;

	/* Get 6 encoder values from HCI */
	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 6);
	if ( (result = hci_wait_packet(&arm->hci)) == SUCCESS)
	{
		arm_calc_joints(arm);
		arm_calc_full(arm);
	}

	return result;
}




/*-----------------------------------------------*/
/* ----- Advanced Data Reporting Functions ----- */
/*-----------------------------------------------*/



/*---------------------------------------*/
/* Getting data with 'background' method */
/*---------------------------------------*/
/* These commands request data from the Arm and immediately exit.
 *   The host can attend to other processing while waiting for data.
 *   Call arm_check_bckg() to check for incoming data.
 */


/* arm_check_bckg() checks for incoming 'background' data.
 *   If a complete packet has come in, it will be automatically
 *     parsed, and appropriate arm_rec fields will be calculated.
 */
arm_result arm_check_bckg(arm_rec *arm)
{
	arm_result result;

	if ( (result = hci_check_packet(&arm->hci,HCI_CHECK_BGND)) == SUCCESS)
	{
		arm_calc_joints(arm);
		(*(arm->packet_calc_fn))(arm);
	}

	return result;
}


/* arm_stylus_6DOF_bckg() updates the stylus position and direction.
 */
void arm_stylus_6DOF_bckg(arm_rec *arm)
{
	/* Get 6 encoder values from HCI */
	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 6);
	arm->packet_calc_fn = arm_calc_stylus_6DOF;
}


/* arm_stylus_3DOF_bckg() updates the stylus position.
 *   This is faster than arm_stylus_6DOF_update() but does not calculate
 *   stylus direction.
 */
void arm_stylus_3DOF_bckg(arm_rec *arm)
{
	/* Get 6 encoder values from HCI */
	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 6);
	arm->packet_calc_fn = arm_calc_stylus_3DOF;
}


/* arm_3joint_bckg() updates the first three joint angles but nothing else
 */
void arm_3joint_bckg(arm_rec *arm)
{
	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 3);
	arm->packet_calc_fn = arm_calc_nothing;
}


/* arm_6joint_bckg() updates all six joint angles but nothing else
 */
void arm_6joint_bckg(arm_rec *arm)
{
	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 6);
	arm->packet_calc_fn = arm_calc_nothing;
}


/* arm_full_bckg() updates all quantities in the arm_rec,
 *   subject to the timer_report and anlg_reports flags
 */
void arm_full_bckg(arm_rec *arm)
{
	/* Get 6 encoder values from HCI */
	hci_std_cmd(&arm->hci, arm->timer_report, arm->anlg_reports, 6);
	arm->packet_calc_fn = arm_calc_full;
}


/*---------------------------------------------*/
/* Getting data with 'motion-generated' method */
/*---------------------------------------------*/
/* These commands initiate a stream of data and immediately exit.
 *   Call arm_check_motion() to check for incoming data.
 *   You must check for data often enough not to overrun the serial buffer.
 *   Packets will be sent automatically whenever the Arm moves sufficiently,
 *     or whenever a button changes state.  A minimum between-packet time
 *     can be sent to prevent the host from being overrun with data.
 *   Use of arguments:
 *      motion_thresh = # of pulses of encoder change (on any encoder)
 *          that will trigger a new packet.  If it is zero, then motion
 *          will not trigger a packet.
 *      btns_active = flag telling whether or not to report button presses.
 *      packet_delay = approx minimum # of ms between packets.
 */


/* arm_check_motion() checks for a complete response packet from the Arm.
 *    If a complete packet is waiting in the serial input buffer,
 *    then arm_check_motion() parses it and calculates joint angles
 *    for the encoders that were updated.
 *    arm_check_motion() is similar to arm_check_packet() but is for use
 *       when motion-generated packets have been requested.
 */
arm_result arm_check_motion(arm_rec *arm)
{
	arm_result result;

	if ( (result = hci_check_motion(&arm->hci)) == SUCCESS)
	{
		arm_calc_joints(arm);
		(*(arm->packet_calc_fn))(arm);
	}

	return result;
}


/* arm_stylus_6DOF_motion() puts the Arm in motion-reporting mode.
 *   All joint angles will be reported if Arm state changes sufficiently.
 *   Packets will be separated by at least packet_delay milliseconds.
 *   Call arm_check_motion() periodically to receive incoming packets
 *     and perform appropriate calculations.
 */
void arm_stylus_6DOF_motion(arm_rec *arm, int motion_thresh,
				int packet_delay, int btns_active)
{
	arm_start_motion(arm, 6, motion_thresh, packet_delay, btns_active);
	arm->packet_calc_fn = arm_calc_stylus_6DOF;
}


/* arm_stylus_3DOF_motion() puts the Arm in motion-reporting mode.
 *   All joint angles will be reported if Arm state changes sufficiently.
 *   Packets will be separated by at least packet_delay milliseconds.
 *   Call arm_check_motion() periodically to receive incoming packets
 *     and perform appropriate calculations.
 */
void arm_stylus_3DOF_motion(arm_rec *arm, int motion_thresh,
				int packet_delay, int btns_active)
{
	arm_start_motion(arm, 6, motion_thresh, packet_delay, btns_active);
	arm->packet_calc_fn = arm_calc_stylus_3DOF;
}


/* arm_6joint_motion() puts the Arm in motion-reporting mode.
 *   All joint angles will be reported if Arm state changes sufficiently.
 *   Packets will be separated by at least packet_delay milliseconds.
 *   Call arm_check_motion() periodically to receive incoming packets
 *     and perform appropriate calculations.
 */
void arm_6joint_motion(arm_rec *arm, int motion_thresh,
				int packet_delay, int btns_active)
{
	arm_start_motion(arm, 6, motion_thresh, packet_delay, btns_active);
	arm->packet_calc_fn = arm_calc_nothing;
}


/* arm_3joint_motion() puts the Arm in motion-reporting mode.
 *   All joint angles will be reported if Arm state changes sufficiently.
 *   Packets will be separated by at least packet_delay milliseconds.
 *   Call arm_check_motion() periodically to receive incoming packets
 *     and perform appropriate calculations.
 */
void arm_3joint_motion(arm_rec *arm, int motion_thresh,
				int packet_delay, int btns_active)
{
	arm_start_motion(arm, 3, motion_thresh, packet_delay, btns_active);
	arm->packet_calc_fn = arm_calc_nothing;
}


/* arm_full_motion() puts the Arm in motion-reporting mode.
 *   All joint angles will be reported if Arm state changes sufficiently.
 *   Packets will be separated by at least packet_delay milliseconds.
 *   Call arm_check_motion() periodically to receive incoming packets
 *     and perform appropriate calculations.
 */
void arm_full_motion(arm_rec *arm, int motion_thresh,
				int packet_delay, int btns_active)
{
	arm_start_motion(arm, 3, motion_thresh, packet_delay, btns_active);
	arm->packet_calc_fn = arm_calc_full;
}


/* arm_end_motion() cancels motion-reporting mode and clears all unparsed
 *   data.  Use hci_insert_marker() to cancel without clearing.
 */
void arm_end_motion(arm_rec *arm)
{
	hci_end_motion(&arm->hci);
}




/*-----------------------------------------------*/
/* ----- Setting Modes, Options, and Units ----- */
/*-----------------------------------------------*/



/* arm_length_units() sets the units for all position coordinates and lengths
 */
void arm_length_units(arm_rec *arm, length_units units)
{
	arm->len_units = units;
	arm_convert_params(arm);
}


/* arm_angle_units() sets the units for the stylus direction angles.
 */
void arm_angle_units(arm_rec *arm, angle_units units)
{
	arm->ang_units = units;
}


/* arm_angle_format() sets the format for the stylus direction angles.
 *    e.g. xyz fixed, zxy Euler, etc.
 */
void arm_angle_format(arm_rec *arm, angle_format format)
{
	arm->ang_format = format;
}


/* arm_report_timer() makes all subsequent reports include timestamp
 */
void arm_report_timer(arm_rec *arm)
{
	arm->timer_report = 1;
}


/* arm_skip_timer() makes all subsequent reports omit timestamp
 */
void arm_skip_timer(arm_rec *arm)
{
	arm->timer_report = 0;
}


/* arm_report_analog() makes all subsequent reports include
 *   the given number of analog values
 *   Note: Only certain custom configurations support analog inputs.
 */
void arm_report_analog(arm_rec *arm, int analog_reports)
{
	arm->anlg_reports = analog_reports;
}


/* arm_skip_analog() makes all subsequent reports omit analog values.
 *   Note: Only certain custom configurations support analog inputs.
 */
void arm_skip_analog(arm_rec *arm)
{
	arm->anlg_reports = 0;
}




/*-------------------------*/
/* ----- Calculation ----- */
/*-------------------------*/
/* The joint_rad[] fields are considered primary quantities for these functions.
 *   arm_calc_joints() computes either 3 or 6 of the joint_rad[] fields,
 *     depending on how many angles were reported in the previous frame.
 *   All other calculation functions assume that arm_calc_joints() has
 *     already been called.  All foreground, background, and motion-sensing
 *     commands provided here take care of calling arm_calc_joints() before
 *     calling higher-order calculations.
 */




/* arm_calc_stylus_6DOF() calculates the position and direction of the stylus tip.
 *   Also calculates the full matrix T, and all linkage endpoints.
 */
void arm_calc_stylus_6DOF(arm_rec *arm)
{
	arm_calc_trig(arm);
	arm_calc_M(arm);
	arm_calc_T(arm);

	arm->stylus_tip.x = arm->T[0][3];
	arm->stylus_tip.y = arm->T[1][3];
	arm->stylus_tip.z = arm->T[2][3];

	arm_calc_stylus_dir(arm);
}


/* arm_calc_stylus_3DOF() calculates the position of the stylus tip.
 */
void arm_calc_stylus_3DOF(arm_rec *arm)
{
	arm_calc_trig(arm);
	arm_calc_M(arm);
	arm_calc_T(arm);

	arm->stylus_tip.x = arm->T[0][3];
	arm->stylus_tip.y = arm->T[1][3];
	arm->stylus_tip.z = arm->T[2][3];
}


/* arm_calc_trig() pre-calculates sines and cosines of the joint angles
 *    Calculates either 3 or 6 joints' worth, depending on encoders reported
 *      in previous frame.
 */
void arm_calc_trig(arm_rec *arm)
{
	angle   *jnt = arm->joint_rad;
	ratio   *sn = arm->sn, *cs = arm->cs;

	*sn++ = sin(*jnt);
	*cs++ = cos(*jnt++);
	*sn++ = sin(*jnt);
	*cs++ = cos(*jnt++);
	*sn++ = sin(*jnt);
	*cs++ = cos(*jnt++);
	if (arm->hci.encoder_updated[5])
	{
		*sn++ = sin(*jnt);
		*cs++ = cos(*jnt++);
		*sn++ = sin(*jnt);
		*cs++ = cos(*jnt++);
		*sn++ = sin(*jnt);
		*cs++ = cos(*jnt++);
	}
}


/* arm_calc_joints() calculates either 3 or 6 joints, based on encoders that
 *   were updated in the most recent packet.
 */
void arm_calc_joints(arm_rec *arm)
{
	ratio   *d_fac = arm->JOINT_DEGREES_FACTOR;
	ratio   *r_fac = arm->JOINT_RADIANS_FACTOR;
	angle   *d_jnt = arm->joint_deg;
	angle   *r_jnt = arm->joint_rad;
	unsigned *encd = (unsigned*) arm->hci.encoder, hex;
	unsigned *max = (unsigned*) arm->hci.max_encoder;

	/* Update joints 0-2 if their encoders were reported last time */
	if (arm->hci.encoder_updated[2])
	{
		hex = *encd++ & *max++;
		*d_jnt++ = *d_fac++  *  hex;
		*r_jnt++ = *r_fac++  *  hex;
		hex = *encd++ & *max++;
		*d_jnt++ = *d_fac++  *  hex;
		*r_jnt++ = *r_fac++  *  hex;
		hex = *encd++ & *max++;
		*d_jnt++ = *d_fac++  *  hex;
		*r_jnt++ = *r_fac++  *  hex;
	}

	/* Update joints 3-5 if their encoders were reported last time */
	if (arm->hci.encoder_updated[5])
	{
		hex = *encd++ & *max++;
		*d_jnt++ = *d_fac++  *  hex;
		*r_jnt++ = *r_fac++  *  hex;
		hex = *encd++ & *max++;
		*d_jnt++ = *d_fac++  *  hex;
		*r_jnt++ = *r_fac++  *  hex;
		hex = *encd++ & *max++;
		*d_jnt++ = *d_fac++  *  hex;
		*r_jnt++ = *r_fac++  *  hex;
	}
}


/* arm_calc_full() calculates all arm_rec fields.
 */
void arm_calc_full(arm_rec *arm)
{
	arm_calc_stylus_6DOF(arm);
}


/* arm_calc_nothing() is a dummy function that does nothing.
 */
#pragma argsused
void arm_calc_nothing(arm_rec *arm)
{
	;
}




/*---------------------------------*/
/* ----- Calculation Helpers ----- */
/*---------------------------------*/


/* arm_identity_4x4() initializes a 4-by-4 identity matrix.
 */
void arm_identity_4x4(matrix_4 M)
{
	M[0][3] = M[0][1] = M[0][2] = 0.0; M[0][0] = 1.0;
	M[1][0] = M[1][3] = M[1][2] = 0.0; M[1][1] = 1.0;
	M[2][0] = M[2][1] = M[2][3] = 0.0; M[2][2] = 1.0;
	M[3][0] = M[3][1] = M[3][2] = 0.0; M[3][3] = 1.0;
}


/* arm_mul_4x4() computes X = M1 * M2 as 4-by-4 matrix multiplication.
 *   Only assumes that all bottom rows are {0,0,0,1}
 *   All three parameters must point to DISTINCT matrices.
 */
void arm_mul_4x4(matrix_4 M1, matrix_4 M2, matrix_4 X)
{
	X[0][0] = M1[0][0]*M2[0][0] + M1[0][1]*M2[1][0] + M1[0][2]*M2[2][0];
	X[0][1] = M1[0][0]*M2[0][1] + M1[0][1]*M2[1][1] + M1[0][2]*M2[2][1];
	X[0][2] = M1[0][0]*M2[0][2] + M1[0][1]*M2[1][2] + M1[0][2]*M2[2][2];
	X[0][3] = M1[0][0]*M2[0][3] + M1[0][1]*M2[1][3] + M1[0][2]*M2[2][3]
								+ M1[0][3];
	X[1][0] = M1[1][0]*M2[0][0] + M1[1][1]*M2[1][0] + M1[1][2]*M2[2][0];
	X[1][1] = M1[1][0]*M2[0][1] + M1[1][1]*M2[1][1] + M1[1][2]*M2[2][1];
	X[1][2] = M1[1][0]*M2[0][2] + M1[1][1]*M2[1][2] + M1[1][2]*M2[2][2];
	X[1][3] = M1[1][0]*M2[0][3] + M1[1][1]*M2[1][3] + M1[1][2]*M2[2][3]
								+ M1[1][3];
	X[2][0] = M1[2][0]*M2[0][0] + M1[2][1]*M2[1][0] + M1[2][2]*M2[2][0];
	X[2][1] = M1[2][0]*M2[0][1] + M1[2][1]*M2[1][1] + M1[2][2]*M2[2][1];
	X[2][2] = M1[2][0]*M2[0][2] + M1[2][1]*M2[1][2] + M1[2][2]*M2[2][2];
	X[2][3] = M1[2][0]*M2[0][3] + M1[2][1]*M2[1][3] + M1[2][2]*M2[2][3]
								+ M1[2][3];
	X[3][0] = X[3][1] = X[3][2] = 0.0;  X[3][3] = 1.0;
}


/* arm_assign_4x4() copies a 4-by-4 matrix.
 */
void arm_assign_4x4(matrix_4 to, matrix_4 from)
{
	to[0][0] = from[0][0], to[0][1] = from[0][1], to[0][2] = from[0][2];
	to[0][3] = from[0][3];
	to[1][0] = from[1][0], to[1][1] = from[1][1], to[1][2] = from[1][2];
	to[1][3] = from[1][3];
	to[2][0] = from[2][0], to[2][1] = from[2][1], to[2][2] = from[2][2];
	to[2][3] = from[2][3];
	to[3][0] = from[3][0], to[3][1] = from[3][1], to[3][2] = from[3][2];
	to[3][3] = from[3][3];
}


/* arm_calc_T() calculates the  NUM_DOF-matrix chain, resulting in matrix T.
 *   Stores intermediate endpoints along the way.
 */
void arm_calc_T(arm_rec *arm)
{
	int i;
	matrix_4 temp;

	i=0;
	arm_assign_4x4(temp, arm->M[0]);
	arm->endpoint[i].x = temp[0][3];
	arm->endpoint[i].y = temp[1][3];
	arm->endpoint[i].z = temp[2][3];
	for(i=1;i<NUM_DOF;i++)
	{
		arm_mul_4x4(temp, arm->M[i], arm->T);
		arm_assign_4x4(temp, arm->T);
		arm->endpoint[i].x = temp[0][3];
		arm->endpoint[i].y = temp[1][3];
		arm->endpoint[i].z = temp[2][3];
	}
}


/* arm_calc_M() calculates all the M[] matrices */
void arm_calc_M(arm_rec *arm)
{
	int i;
	ratio   c, s;
	ratio   ca, sa;
	matrix_4 *M;

	for(i=0;i<NUM_DOF;i++)
	{
		c = arm->cs[i], s = arm->sn[i];
		ca = arm->csALPHA[i], sa = arm->snALPHA[i];
		M = &arm->M[i];
		if (i != 2) {
			(*M)[0][0] = c, (*M)[0][1] = -s;
			(*M)[0][2] = 0.0, (*M)[0][3] = arm->A[i];
			(*M)[1][0] = s*ca, (*M)[1][1] = c*ca;
			(*M)[1][2] = -sa, (*M)[1][3] = -sa*arm->D[i];
			(*M)[2][0] = s*sa, (*M)[2][1] = c*sa;
			(*M)[2][2] = ca, (*M)[2][3] = ca*arm->D[i];
			(*M)[3][0] = (*M)[3][1] = (*M)[3][2] = 0.0;
			(*M)[3][3] = 1.0;
		} else {
			ratio cb, sb;
			cb = cos(arm->BETA);            sb = sin(arm->BETA);

			(*M)[0][0] = c*cb,                      (*M)[0][1] = -s*cb;
			(*M)[0][2] = sb,                                (*M)[0][3] = sb*arm->D[i]+arm->A[i];
			(*M)[1][0] = s*ca+sa*sb*c, (*M)[1][1] = c*ca-sa*sb*s;
			(*M)[1][2] = -sa*cb,                    (*M)[1][3] = -sa*cb*arm->D[i];
			(*M)[2][0] = s*sa-ca*sb*c,      (*M)[2][1] = c*sa+s*sb*ca;
			(*M)[2][2] = ca*cb,                     (*M)[2][3] = cb*ca*arm->D[i];
			(*M)[3][0] = (*M)[3][1] = (*M)[3][2] = 0.0;
			(*M)[3][3] = 1.0;
		}
	}
}


/* arm_calc_stylus_dir() calculates the direction of the stylus from matrix T.
 */
void arm_calc_stylus_dir(arm_rec *arm)
{
	if ( (arm->ang_format == XYZ_FIXED)
		|| (arm->ang_format == ZYX_EULER) )
	{
		arm->stylus_dir.x = atan2(arm->T[2][1], arm->T[2][2]);
		arm->stylus_dir.y = atan2(-arm->T[2][0],
			sqrt(arm->T[0][0]*arm->T[0][0]
				+ arm->T[1][0]*arm->T[1][0]));
		arm->stylus_dir.z = atan2(arm->T[1][0], arm->T[0][0]);
	}
	else if ( (arm->ang_format == YXZ_FIXED)
		|| (arm->ang_format == ZXY_EULER) )
	{
		arm->stylus_dir.x = atan2(-arm->T[1][2],
			sqrt(arm->T[0][2]*arm->T[0][2]
				+ arm->T[2][2]*arm->T[2][2]));
		arm->stylus_dir.y = atan2(arm->T[0][2], arm->T[2][2]);
		arm->stylus_dir.z = atan2(arm->T[1][0], arm->T[1][1]);
	}
	else if ( (arm->ang_format == ZYX_FIXED)
		|| (arm->ang_format == XYZ_EULER) )
	{
		arm->stylus_dir.x = atan2(-arm->T[1][2], arm->T[2][2]);
		arm->stylus_dir.y = atan2(arm->T[0][2],
			sqrt(arm->T[1][2]*arm->T[1][2]
				+ arm->T[2][2]*arm->T[2][2]));
		arm->stylus_dir.z = atan2(-arm->T[0][1], arm->T[0][0]);
	}
	if (arm->ang_units == DEGREES)
	{
		arm->stylus_dir.x *= 180.0/PI;
		arm->stylus_dir.y *= 180.0/PI;
		arm->stylus_dir.z *= 180.0/PI;
	}
}




/*----------------------------------*/
/* ----- Calibration & Homing ----- */
/*----------------------------------*/



/*---------------*/
/* Home position */
/*---------------*/


/* arm_home_pos() sets Arm angles to the home position
 *    Do this ONLY when the Arm is physically in the home position.
 *    This does not put the new (home-pos) Arm angles into the arm_rec.
 */
arm_result arm_home_pos(arm_rec *arm)
{
	return hci_go_home_pos(&arm->hci);
}


/*-----------------*/
/* Parameter Block */
/*-----------------*/


/* arm_convert_params() uses param_block data to compute arm constants
 *   in appropriate units.
 *   Returns SUCCESS or BAD_FORMAT
 */
arm_result arm_convert_params(arm_rec *arm)
{
	int converted = 0;

	if (hci_strcmp(arm->hci.param_format, "Format DH0.5"))
		converted = arm_params_DH0_5(arm);

/*  Add new format handlers here like so:

	else if (hci_strcmp(arm->hci.param_format, "Format DH1.0"))
		converted = arm_params_DH1_0(arm);
*/

	/* Now calculate all const matrix elements from new params */
	if (converted) arm_calc_params(arm);

	return (converted ? SUCCESS : BAD_FORMAT);
}



/* arm_convert_ext_params() uses ext_param_block data to compute arm
 *	  constants in appropriate units.
 *   Returns SUCCESS or BAD_FORMAT
 */
arm_result arm_convert_ext_params(arm_rec *arm)
{
	int temp;
	byte *pb = arm->ext_param_block;

   	/* if BETA was sent, get beta */
	if (strstr(arm->hci.comment,"Beta") != NULL && arm->ext_p_block_size >= 2) {
		temp = ((signed char) pb[0]) * 256 + pb[1];
		arm->BETA = (temp/32768.0)*PI;
	}

	return SUCCESS;
}


/* arm_params_DH0_5() uses param_block data to compute arm constants
 *   in appropriate units,
 *   FOR FORMAT DH0.5 (Denavit-Hartenberg form 0.5)
 *   Returns 1 if successful, 0 if # params is wrong
 */
int arm_params_DH0_5(arm_rec *arm)
{
	FILE *fpin;
   float D5delta, D4delta, A5delta;
	char buffer[200];		/* the is oversized on purpose */
	char *paramname, *paramvalue;
	int     temp;
	byte *pb = arm->param_block;
	float len_factor = (arm->len_units == INCHES ? 1.0 : 25.4 );

	if (arm->p_block_size != 36) return 0;

	temp = ((signed char)pb[0])*256 + pb[1];
	arm->ALPHA[0] = (temp/32768.0)*PI;
	temp = ((signed char)pb[2])*256 + pb[3];
	arm->ALPHA[1] = (temp/32768.0)*PI;
	temp = ((signed char)pb[4])*256 + pb[5];
	arm->ALPHA[2] = (temp/32768.0)*PI;
	temp = ((signed char)pb[6])*256 + pb[7];
	arm->ALPHA[3] = (temp/32768.0)*PI;
	temp = ((signed char)pb[8])*256 + pb[9];
	arm->ALPHA[4] = (temp/32768.0)*PI;
	temp = ((signed char)pb[10])*256 + pb[11];
	arm->ALPHA[5] = (temp/32768.0)*PI;

	temp = ((signed char)pb[12])*256 + pb[13];
	arm->A[0] = temp/1000.0*len_factor;
	temp = ((signed char)pb[14])*256 + pb[15];
	arm->A[1] = temp/1000.0*len_factor;
	temp = ((signed char)pb[16])*256 + pb[17];
	arm->A[2] = temp/1000.0*len_factor;
	temp = ((signed char)pb[18])*256 + pb[19];
	arm->A[3] = temp/1000.0*len_factor;
	temp = ((signed char)pb[20])*256 + pb[21];
	arm->A[4] = temp/1000.0*len_factor;
	temp = ((signed char)pb[22])*256 + pb[23];
	arm->A[5] = temp/1000.0*len_factor;

	temp = ((signed char)pb[24])*256 + pb[25];
	arm->D[0] = temp/1000.0*len_factor;
	temp = ((signed char)pb[26])*256 + pb[27];
	arm->D[1] = temp/1000.0*len_factor;
	temp = ((signed char)pb[28])*256 + pb[29];
	arm->D[2] = temp/1000.0*len_factor;
	temp = ((signed char)pb[30])*256 + pb[31];
	arm->D[3] = temp/1000.0*len_factor;
	temp = ((signed char)pb[32])*256 + pb[33];
	arm->D[4] = temp/1000.0*len_factor;
	temp = ((signed char)pb[34])*256 + pb[35];
	arm->D[5] = temp/1000.0*len_factor;
	arm->D5Point = arm->D[5];

	fpin = fopen("MSTIP.DAT","r");
   D5delta = 0.0;
   D4delta = 0.0;
   A5delta = 0.0;
   if (fpin != NULL) {
		/* read the new values */
		while (fgets(buffer,sizeof(buffer),fpin) != NULL && strlen(buffer) > 0) {
			paramname = strtok(buffer," =");
			paramvalue = strtok(NULL," =");
			if (strcmp("D5Delta",paramname) == 0)
         	D5delta = atof(paramvalue);
			else if (strcmp("D4Delta",paramname) == 0)
         	D4delta = atof(paramvalue);
			else if (strcmp("A5Delta",paramname) == 0)
         	A5delta = atof(paramvalue);
      }
		arm->D[5] += D5delta * len_factor;
		arm->D[4] -= D4delta * len_factor;
		arm->A[5] += A5delta * len_factor;
   fclose(fpin);
   }

	return 1;
}

/* arm_calc_params() calculates arm_rec constants that are found from
     the params in the downloaded block.
 */
void arm_calc_params(arm_rec *arm)
{
	int  i;

	for(i=0;i<NUM_DOF;i++)
	{
		arm->csALPHA[i] = cos(arm->ALPHA[i]);
		arm->snALPHA[i] = sin(arm->ALPHA[i]);
	}
}






/*-----------------------------------*/
/* ----- Simple Error Handlers ----- */
/*-----------------------------------*/


/* arm_install_simple() installs all the following simple handlers.
 */
void arm_install_simple(arm_rec *arm)
{
	arm->hci.TIMED_OUT_handler = simple_TIMED_OUT;
	arm->hci.BAD_PORT_handler = simple_BAD_PORT;
	arm->hci.BAD_PACKET_handler = simple_BAD_PACKET;
	arm->hci.NO_HCI_handler = simple_NO_HCI;
	arm->hci.CANT_BEGIN_handler = simple_CANT_BEGIN;
	arm->hci.CANT_OPEN_handler = simple_CANT_OPEN_PORT;
}


/* simple_TIMED_OUT() handles any failure to receive a complete response.
 */
arm_result simple_TIMED_OUT(hci_rec *hci, arm_result condition)
{
	printf("\n%s: port %d, %ld baud\n", condition,
		hci->port_num, hci->baud_rate);

	/* Empty the host buffer so we can try again */
	hci_reset_com(hci);

	return condition;
}


/* simple_BAD_PORT() handles any attempt to open an invalid port.
 */
arm_result simple_BAD_PORT(hci_rec *hci, arm_result condition)
{
	char    ch[5];

	printf("\n%s: port %d, %ld baud\n", condition,
		hci->port_num, hci->baud_rate);
	printf("   Type 'p' to try a different PORT,\n");
	printf("   Type any other key to ABORT.\n");
	scanf("%s", ch);
	if ((*ch == 'p') || (*ch == 'P'))
	{
		printf("Type a new port to try -> ");
		scanf("%d", &hci->port_num);
		condition = TRY_AGAIN;
	}

	return condition;
}


/* simple_BAD_PACKET() handles any errors in data reception
 */
arm_result simple_BAD_PACKET(hci_rec *hci, arm_result condition)
{
	char    ch[5];

	printf("\n%s: port %d, %ld baud\n", condition,
		hci->port_num, hci->baud_rate);
	printf("   Type 'f' to FLUSH host serial buffer.\n");
	printf("   Type any other key to ABORT.\n");
	scanf("%s", ch);
	if ( (*ch == 'f') || (*ch == 'F') )
	{
		/* Make sure Arm is not in motion-reporting mode,
		 *    and clear host serial port. */
		hci_end_motion(hci);
	}

	return condition;
}


/* simple_NO_HCI() handles failure for the hardware to respond at all
 *   during start-up.  Likely causes: no arm plugged in, baud rate
 *   is too high for host to transmit reliably, baud rate is not
 *   achievable by the hardware, Arm has already received BEGIN command.
 */
arm_result simple_NO_HCI(hci_rec *hci, arm_result condition)
{
	char    ch[5];
	arm_result result;

	printf("\n%s: port %d, %ld baud\n", condition,
		hci->port_num, hci->baud_rate);
	printf("Check that the electronics module is plugged in and turned on.\n");
	printf("   Type 'c' to change baud rate or port number and RETRY,\n");
	printf("   Type 'a' to ABORT,\n");
	printf("   Type any other key to retry at same baud rate on same port.\n");
	scanf("%s", ch);
	switch(*ch)
	{
		case 'a':
		case 'A':
			result = NO_HCI;
			break;
		case 'c':
		case 'C':
			hci_disconnect(hci);
			printf("Type the new port -> ");
			scanf("%d", &hci->port_num);
			printf("Type the new baud rate -> ");
			scanf("%ld", &hci->baud_rate);
		default:
			result = TRY_AGAIN;
			printf("Retrying...\n");
			break;
	}

	return result;
}


/* simple_CANT_BEGIN() handles failure to BEGIN the session after a
 *   successful baud-rate synch.  Likely causes: none.
 *   If this happens, something is wrong with communications hardware.
 */
arm_result simple_CANT_BEGIN(hci_rec *hci, arm_result condition)
{
	char ch[5];
	arm_result result;

	printf("\n%s: port %d, %ld baud\n", condition,
		hci->port_num, hci->baud_rate);
	printf("You must reset the electronics module and check all connections.\n");
	printf("   Type 'a' to ABORT,\n");
	printf("   Type any other key to restart.\n");
	scanf("%s", ch);
	switch(*ch)
	{
		case 'a':
		case 'A':
			result = CANT_BEGIN;
			break;
		default:
			result = TRY_AGAIN;
			hci_disconnect(hci);
			printf("Retrying...\n");
			break;
	}

	return result;
}


/* simple_CANT_OPEN_PORT() handles failure to open a serial port.
 *   This means either the port parameters were bad
 *                  or there is a problem with host communications hardware.
 */
arm_result simple_CANT_OPEN_PORT(hci_rec *hci, arm_result condition)
{
	char ch[5];
	arm_result result;

	printf("\n%s: port %d, %ld baud\n", condition,
		hci->port_num, hci->baud_rate);
	printf("Check communications parameters and host hardware.\n");
	printf("   Type 'c' to change baud rate or port number and RETRY,\n");
	printf("   Type 'a' to ABORT,\n");
	printf("   Type any other key to restart.\n");
	scanf("%s", ch);
	switch(*ch)
	{
		case 'a':
		case 'A':
			result = CANT_OPEN_PORT;
			break;
		case 'c':
		case 'C':
			hci_disconnect(hci);
			printf("Type the new port -> ");
			scanf("%d", &hci->port_num);
			printf("Type the new baud rate -> ");
			scanf("%ld", &hci->baud_rate);
		default:
			result = TRY_AGAIN;
			printf("Retrying...\n");
			break;
	}

	return result;
}




/*-------------------------------------------------*/
/* ----- Low-level Functions Used Internally ----- */
/*-------------------------------------------------*/



/* arm_start_motion() initiates a motion-sensing series.
 *   Do not call this function directly.
 */
void arm_start_motion(arm_rec *arm, int num_encoders, int motion_thresh,
				int packet_delay, int btns_active)
{
	int     anlg[NUM_ANALOGS];
	int     encd[NUM_ENCODERS];

	anlg[0] = anlg[1] = anlg[2] = anlg[3]
		= anlg[4] = anlg[5] = anlg[6] = anlg[7] = 0;
	encd[0] = encd[1] = encd[2] = encd[3]
		= encd[4] = encd[5] = encd[6] = motion_thresh;
	hci_report_motion(&arm->hci, arm->timer_report,
		arm->anlg_reports, num_encoders, packet_delay, btns_active,
		anlg, encd);
}


/* arm_get_constants() gets all constants for this individual Arm.
 *   This is called by arm_connect() to get all constants at the beginning
 *   of a session.
 */
arm_result arm_get_constants(arm_rec *arm)
{
	arm_result result;

	result = hci_get_strings(&arm->hci);
	if (result == SUCCESS) result = hci_get_maxes(&arm->hci);
	if (result == SUCCESS) result = hci_get_params(&arm->hci,
				arm->param_block, &arm->p_block_size);
	if (result == SUCCESS && strstr(arm->hci.comment,"Beta") != NULL)
		result = hci_get_ext_params(&arm->hci, arm->ext_param_block,
										&arm->ext_p_block_size);
	if (result == SUCCESS) result = arm_convert_params(arm);
	if (result == SUCCESS && strstr(arm->hci.comment,"Beta") != NULL)
		result = arm_convert_ext_params(arm);
	if (result == SUCCESS)
	{
		arm->JOINT_RADIANS_FACTOR[0] = 2.0 * PI / (arm->hci.max_encoder[0] + 1);
		arm->JOINT_RADIANS_FACTOR[1] = 2.0 * PI / (arm->hci.max_encoder[1] + 1);
		arm->JOINT_RADIANS_FACTOR[2] = 2.0 * PI / (arm->hci.max_encoder[2] + 1);
		arm->JOINT_RADIANS_FACTOR[3] = 2.0 * PI / (arm->hci.max_encoder[3] + 1);
		arm->JOINT_RADIANS_FACTOR[4] = 2.0 * PI / (arm->hci.max_encoder[4] + 1);
		arm->JOINT_RADIANS_FACTOR[5] = 2.0 * PI / (arm->hci.max_encoder[5] + 1);

		arm->JOINT_DEGREES_FACTOR[0] = 360.0 / (arm->hci.max_encoder[0] + 1);
		arm->JOINT_DEGREES_FACTOR[1] = 360.0 / (arm->hci.max_encoder[1] + 1);
		arm->JOINT_DEGREES_FACTOR[2] = 360.0 / (arm->hci.max_encoder[2] + 1);
		arm->JOINT_DEGREES_FACTOR[3] = 360.0 / (arm->hci.max_encoder[3] + 1);
		arm->JOINT_DEGREES_FACTOR[4] = 360.0 / (arm->hci.max_encoder[4] + 1);
		arm->JOINT_DEGREES_FACTOR[5] = 360.0 / (arm->hci.max_encoder[5] + 1);
	}

	return result;
}
