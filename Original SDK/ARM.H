/***************************************************
 * - - - - - - -   IMMERSION CORP.   - - - - - - - *
 *                                                 *
 *       Platform-independent software series      *
 *                Copyright (c) 1993               *
 ***************************************************
 * ARM.H   |   SDK1-2a   |   January 1996
 *
 * Immersion Corp. Software Developer's Kit
 *      Definitions and prototypes for the Immersion Corp. MicroScribe
 *      Not for use with the Probe or Personal Digitizer
 *      Requires HCI firmware version MSCR1-1C or later
 */

#ifndef arm_h
#define arm_h

/*-----------*/
/* Constants */
/*-----------*/

#define NUM_DOF         6

/* # linkage points that can be calc'd from only 3 joint angles */
#define QUICK_3DOF_POINTS       4
#define STD_3DOF_POINTS 4

/* # discrete positions of table rotation (option included with some models) */
#define NUM_TABLE_POS   4

/* # of bytes in HCI parameter block, plus extra room */
#define PARAM_BLOCK_SIZE			40
#define EXT_PARAM_BLOCK_SIZE		10

#define PI              3.1415926535898

#define RIGHT_PEDAL	1
#define LEFT_PEDAL	2
#define BOTH_PEDALS	3

/*------------*/
/* Data Types */
/*------------*/


/* Synonym for convenience */
typedef hci_result      arm_result;

/* Datatypes for physical quantities:
 *   Future implementations may have a different implementation
 *   of these types
 */
typedef float   length;
typedef float   angle;
typedef float   ratio;
typedef float   matrix_4[4][4];

/* General 3D spatial coordinate data type */
typedef struct
{
	length  x;
	length  y;
	length  z;
} length_3D;

/* General 3D angular coordinate data type */
typedef struct
{
	angle   x;
	angle   y;
	angle   z;
} angle_3D;


/* Strings as enumerated types.
 *    Variables of these types will point to one of several global
 *    string constants.  This means you can compare these variables
 *    to the global string pointers, just as you would compare a regular
 *    enumerated type to a set of enumerated constants.  You can also
 *    directly print these labels, since they are actually strings.
 */
typedef char*   length_units;
typedef char*   angle_units;
typedef char*   angle_format;

/* Constants for length_units variables */
extern char     INCHES[];
extern char     MM[];

/* Constants for angle_units variables */
extern char     DEGREES[];
extern char     RADIANS[];

/* Constants for angle_format variables */
extern char     XYZ_FIXED[];
extern char     ZYX_FIXED[];
extern char     YXZ_FIXED[];
extern char     ZYX_EULER[];
extern char     XYZ_EULER[];
extern char     ZXY_EULER[];


/* Record containing all Arm data
 *   Declare one of these structs for each Arm in use.
 *   Each arm_rec must be init'ed with arm_init() before use.
 *   Example references: (assuming 'arm' is declared as a arm_rec)
 *      arm.stylus_tip.x - x coord of stylus tip (stylus is last joint of arm)
 *      arm.stylus_dir.y - the y-axis angle of stylus orientation
 *      arm.joint_deg[ELBOW] - joint angle #2 (elbow) in degrees
 */
typedef struct arm_rec
{
  /*--------------------------------------
   * Fields for direct use by programmers:
   *   These fields will be maintained and used consistently in future releases
   *   If a field's units are not specified, then they are user-settable
   *      as inches/mm or radians/degrees
   */
	/* Fundamental 6DOF quantities */
	length_3D       stylus_tip;     /* Coordinates of stylus tip */
	angle_3D        stylus_dir;     /* Direction (roll,pitch,yaw) of stylus */

	/* Transformation matrix representing stylus 6DOF coordinates
	 *   4-by-4 matrix, arm.T[0][0] through arm.T[3][3] */
	matrix_4        T;

	/* Series of linkage endpoints */
	length_3D       endpoint [NUM_DOF];     /* also includes stylus tip */

	/* Joint angles */
	angle           joint_rad [NUM_DOF];    /* radians */
	angle           joint_deg [NUM_DOF];    /* degrees */


   /*------------------------------
    * Units and orientation format:
    *   READ-ONLY.  Use functions provided to set these.
    */
	/* Unit conversion and data format specifiers: */
	length_units    len_units;      /* inches/mm for xyz coordinates */
	angle_units     ang_units;      /* radians/degrees for stylus angles */
	angle_format    ang_format;     /* xyz_fixed/zyx_fixed ... for stylus angles */


   /*-----------------------------------------------
    * Fields describing internal physical structure:
    *   These may change with refinements to the Arm and calibration
    *   techniques.
    */
	length  D[NUM_DOF];     /* Offsets between joint axes */
	length  A[NUM_DOF];     /* Offsets between joint axes */
	angle   ALPHA[NUM_DOF]; /* Skew angles between joint axes */

   angle   BETA;				/* the beta angle in T23 */


  /*----------------------------
   * Internal working variables:
   *   These are used internally for efficient calculation and are subject to
   *   change.
   */
	/* Pre-computed conversion factors */
	ratio   JOINT_RADIANS_FACTOR[NUM_DOF]; /* Factors to multiply by
			encoder counts in order to get angles in radians */
	ratio   JOINT_DEGREES_FACTOR[NUM_DOF]; /* Factors to multiply by
			encoder counts in order to get angles in degrees */

	/* Intermediate matrix products */
	matrix_4        M[NUM_DOF];

	/* Trigonometric quantities: */
	ratio           cs[NUM_DOF]; /* cosines of all angles */
	ratio           sn[NUM_DOF]; /* sines of all angles */
	ratio           csALPHA[NUM_DOF];       /* cs & sn of ALPHA const's */
	ratio           snALPHA[NUM_DOF];

   /*---------------------------
    * Internal status variables:
    *   Do not access directly.  Use functions provided to manipulate these.
    */
	/* Fields requested in subsequent reports */
	int             timer_report;   /* Flag telling whether to report timer */
	int             anlg_reports;   /* # of analog values to report */

	/* Number of points needed in next endpoint calculation */
	int             num_points;

	/* Calculation function to execute after getting next packet */
	void            (*packet_calc_fn)(struct arm_rec*);

   /*----------------
    * Low-level data:
    */
	hci_rec         hci;
	byte            param_block[PARAM_BLOCK_SIZE];
	int             p_block_size;

	byte				 ext_param_block[EXT_PARAM_BLOCK_SIZE];
   int				 ext_p_block_size;

   float		pt2ptdist;		/* distance between points in AutoPlotPoint() */
	float		lastX, lastY, lastZ;	/* last point taken */

   float    D5Point; 		/* standard stylus length with standard point tip */
} arm_rec;



/*-------------------------*/
/* Macros & Easy reference */
/*-------------------------*/

/* Joint angle references
 *   Use these in any ...[NUM_DOF] array to refer to a specific joint */
#define BASE            0
#define SHOULDER        1
#define ELBOW           2
#define FOREARM         3
#define WRIST           4
#define STYLUS          5




/*-----------------------------*/
/* --- Function prototypes --- */
/*-----------------------------*/


/*---------------------*/
/* Essential Functions */
/*---------------------*/

/* Initialization required once for each arm_rec */
void            arm_init(arm_rec *arm);

/* Communications */
arm_result      arm_connect(arm_rec *arm, int port, long int baud);
void            arm_disconnect(arm_rec *arm);
void            arm_change_baud(arm_rec *arm, long int new_baud);

/* Point Gathering Functions for Digitizing
	All use 'foreground' arm_stylus_3DOF_update() function	*/
int GetPoint(arm_rec* arm);
int AutoPlotPoint(arm_rec* arm, float DistanceSetting);
void AutoPlotPointUndo(arm_rec* arm, float newX, float newY, float newZ);

/* Tip Change Functions for Standard Point Tip, Standard Ball Tip, or
	Custom Tip */
void PointTip(arm_rec* arm);
void BallTip(arm_rec* arm);
void CustomTip(arm_rec* arm, float delta);

/* Getting data using simplest 'foreground' method
 *   (Host waits idly for response to come back) */
	/* Stylus coordinates */
arm_result      arm_stylus_6DOF_update(arm_rec *arm);
arm_result      arm_stylus_3DOF_update(arm_rec *arm);
	/* Joint angles ONLY */
arm_result      arm_3joint_update(arm_rec *arm);
arm_result      arm_6joint_update(arm_rec *arm);
	/* All Arm data */
arm_result      arm_full_update(arm_rec *arm);


/*------------------------------*/
/* More advanced data reporting */
/*------------------------------*/

/* Getting data using 'background' method
 *   Host can do other processing while waiting for response */
	/* Checking for incoming 'background' data */
arm_result      arm_check_bckg(arm_rec *arm);
	/* Stylus coordinates */
void            arm_stylus_6DOF_bckg(arm_rec *arm);
void            arm_stylus_3DOF_bckg(arm_rec *arm);
	/* Joint angles ONLY */
void            arm_3joint_bckg(arm_rec *arm);
void            arm_6joint_bckg(arm_rec *arm);
	/* All Arm data */
void            arm_full_bckg(arm_rec *arm);


/* Getting data using 'motion-sensing' method
 *   Data is automatically reported whenever the Arm moves sufficiently
 *     or a button is pressed. */
	/* Checking for incoming 'motion-sensing' data */
arm_result      arm_check_motion(arm_rec *arm);
	/* Canceling motion-sensing mode */
void            arm_end_motion(arm_rec *arm);
	/* Stylus coordinates */
void            arm_stylus_6DOF_motion(arm_rec *arm, int motion_thresh,
				int packet_delay, int btns_active);
void            arm_stylus_3DOF_motion(arm_rec *arm, int motion_thresh,
				int packet_delay, int btns_active);
	/* Joint angles ONLY */
void            arm_3joint_motion(arm_rec *arm, int motion_thresh,
			int packet_delay, int btns_active);
void            arm_6joint_motion(arm_rec *arm, int motion_thresh,
			int packet_delay, int btns_active);
	/* All Arm data */
void            arm_full_motion(arm_rec *arm, int motion_thresh,
			int packet_delay, int btns_active);



/*---------------------------*/
/* Setting Modes and Options */
/*---------------------------*/

/* Selecting units & data formats
 *   Units default to inches & degrees, unless these functions are used */
void            arm_length_units(arm_rec *arm, length_units units);
void            arm_angle_units(arm_rec *arm, angle_units units);
void            arm_angle_format(arm_rec *arm, angle_format format);

/* Requesting timer & analog data
 *   Default is not to report timer or analog data,
 *   unless these functions are used */
void            arm_report_timer(arm_rec *arm);
void            arm_skip_timer(arm_rec *arm);
void            arm_report_analog(arm_rec *arm, int analog_reports);
void            arm_skip_analog(arm_rec *arm);


/*-------------*/
/* Calculation */
/*-------------*/
void    arm_calc_stylus_6DOF(arm_rec *arm);
void    arm_calc_stylus_3DOF(arm_rec *arm);
void    arm_calc_trig(arm_rec *arm);
void    arm_calc_joints(arm_rec *arm);
void    arm_calc_full(arm_rec *arm);
void    arm_calc_nothing(arm_rec *arm);


/*---------------------*/
/* Calculation Helpers */
/*---------------------*/
void    arm_calc_T(arm_rec *arm);
void    arm_calc_M(arm_rec *arm);
void    arm_calc_stylus_dir(arm_rec *arm);
void    arm_mul_4x4(matrix_4 M1, matrix_4 M2, matrix_4 X);
void    arm_identity_4x4(matrix_4 M);
void    arm_assign_4x4(matrix_4 to, matrix_4 from);


/*---------------*/
/* Home Position */
/*---------------*/
arm_result      arm_home_pos(arm_rec *arm);


/*-----------------*/
/* Parameter Block */
/*-----------------*/
arm_result      arm_convert_params(arm_rec *arm);
arm_result		 arm_convert_ext_params(arm_rec *arm);
int             arm_params_DH0_5(arm_rec *arm);
void            arm_calc_params(arm_rec *arm);


/*-------------------------------*/
/* Simple example error handlers */
/*-------------------------------*/
arm_result      simple_NO_HCI(hci_rec *hci, arm_result condition);
arm_result      simple_CANT_BEGIN(hci_rec *hci, arm_result condition);
arm_result      simple_CANT_OPEN_PORT(hci_rec *hci, arm_result condition);
arm_result      simple_TIMED_OUT(hci_rec *hci, arm_result condition);
arm_result      simple_BAD_PORT(hci_rec *hci, arm_result condition);
arm_result      simple_BAD_PACKET(hci_rec *hci, arm_result condition);
void            arm_install_simple(arm_rec *arm);


/*-------------------------------------*/
/* Low-level functions used internally */
/*-------------------------------------*/

arm_result      arm_get_constants(arm_rec *arm);
void            arm_start_motion(arm_rec *arm, int num_encoders,
			int motion_thresh, int packet_delay, int btns_active);


#endif /* arm_h */
