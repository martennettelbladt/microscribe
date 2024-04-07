/**********************************************************************
	Copyright (c) 1995 Immersion Corporation

	Permission to use, copy, modify, distribute, and sell this
	software and its documentation may be granted without fee;
	interested parties are encouraged to request permission from
		Immersion Corporation
		2158 Paragon Drive
		San Jose, California 95131

	IMMERSION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
	INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
	IN NO EVENT SHALL AFX BE LIABLE FOR ANY SPECIAL, INDIRECT OR
	CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
	LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
	NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
	CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

	FILE: ARMWIN32.H
	STARTED: 10-18-95
	NOTES/REVISIONS:
**********************************************************************/

#ifndef ARMWIN32_H
#define ARMWIN32_H

	/* === File Includes === */

	/* === Defintions === */
#define EXPORT WINAPI

	/* === Data Structures === */

	/* === External Access Globals === */

	/* === Function Prototypes === */

	/* === Definitions === */

#define ARM_MESSAGE	"MscribeUpdate"

#define ARM_SUCCESS			0
#define ARM_ALREADY_USED	-1
#define ARM_NOT_ACCESSED	-2
#define ARM_CANT_CONNECT	-3
#define ARM_INVALID_PARENT	-4
#define ARM_NOT_CONNECTED	-5
#define ARM_BAD_PORTBAUD	-6

#define ARM_3DOF			0x0001
#define ARM_6DOF			0x0002
#define ARM_3JOINT		0x0004
#define ARM_6JOINT		0x0008
#define ARM_FULL			0x0010

#define ARM_INCHES		1
#define ARM_MM				2

#define ARM_DEGREES		1
#define ARM_RADIANS		2

	/* === Data Structures === */

#ifdef __BCPLUSPLUS__
extern "C" {
#endif

int EXPORT ArmStart(HWND parent);
void EXPORT ArmEnd(void);

int EXPORT ArmConnect(int port, long baud);
void EXPORT ArmDisconnect(void);
int EXPORT ArmReconnect(void);

int EXPORT ArmSetBckgUpdate(int type);
int EXPORT ArmSetUpdate(int type);

	// this really returns an arm_rec, but then you need to include arm.h
void * EXPORT ArmGetArmRec(void);

void EXPORT ArmSetLengthUnits(int type);
void EXPORT ArmSetAngleUnits(int type);

#ifdef __BCPLUSPLUS__
}
#endif

	/* === Globals (External Access) === */

	/* === Function Prototypes === */



#endif // ARMWIN32_H