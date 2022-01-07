/*****

$Id: KeyboardDriver.c,v 1.1 1993/06/14 21:17:44 dplatt Exp $

$Log: KeyboardDriver.c,v $
 * Revision 1.1  1993/06/14  21:17:44  dplatt
 * Dragon beta
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  KeyboardDriver.c - Control Port driverlet code for 3DO keyboards.

*/

#include "types.h"
#include "item.h"
#include "kernel.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "operror.h"

#include "event.h"
#include "hardware.h" 
#include "controlport.h"

#include "event.h"
#include "poddriver.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

extern int32 debugFlag;

/* #define DEBUG */

#ifdef DEBUG
# define DBUG(x)  printf x
#else
# define DBUG(x) /* x */
#endif

#ifdef DEBUG2
# define DBUG2(x)  printf x
#else
# define DBUG2(x) /* x */
#endif

#define DBUG0(x) if (debugFlag) { printf x ; }

#include "strings.h"

/*
  3DO keyboard driverlet.

  Private data assignments:  0 is the command to be sent.  1 is the data
  bytes to be sent or echoed out.  2 is the handshaking/startup phase.
  3 is the pointer to the 256-bit key matrix.  4 is the LED bits to be
  sent, and 5 is nonzero if the LED bits need to be sent out.
*/

#define KB_COMMANDOUT 0
#define KB_DATAOUT    1
#define KB_PHASE      2
#define KB_KEYMATRIX  3
#define KB_LEDBITS    4
#define KB_SENDLEDS   5
#define KB_EVENT      6

#define KBCMD_HANDSHAKE 0x00
#define KBCMD_STATUS    0xED
#define KBCMD_ENABLE    0xF4
#define KBCMD_DISABLE   0xF5
#define KBCMD_RESENT    0xF6
#define KBCMD_RESET     0xFF

#define KBSTAT_OVERRUN  0x00
#define KBSTAT_INITOK   0xAA
#define KBSTAT_KEYUP    0xF0
#define KBSTAT_FAILURE  0xFD
#define KBSTAT_RESEND   0xFE

static void StampFrame(EventFrame *thisFrame, Pod *thisPod) {
  thisFrame->ef_PodNumber = thisPod->pod_Number;
  thisFrame->ef_PodPosition = thisPod->pod_Position;
  thisFrame->ef_GenericPosition =
    thisPod->pod_GenericNumber[GENERIC_Keyboard];
  DBUG0(("Stamped event pod %d position %d generic %d\n",thisFrame->ef_PodNumber, thisFrame->ef_PodPosition, thisFrame->ef_GenericPosition));
}

Err KeyboardDriver(PodInterface *interfaceStruct)
{
  Pod *pod;
  uint32 rawBits, decodedBits, oldBits;
  uint32 buttonsDown, buttonsUp;
  uint32 events, dataOut;
  uint8 *base;
  EventFrame *thisFrame;
  pod = interfaceStruct->pi_Pod;
  DBUG(("Keyboard driver, function %d\n", interfaceStruct->pi_Command));
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG0(("Keyboard driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG0(("Keyboard pod init\n"));
    memset(pod->pod_PrivateData, 0, sizeof pod->pod_PrivateData);
    pod->pod_PrivateData[KB_KEYMATRIX] = (uint32) malloc(32);
    if (!pod->pod_PrivateData[KB_KEYMATRIX]) {
      return MakeKErr(ER_SEVER,ER_C_STND,ER_NoMem);
    }
    memset((void *) pod->pod_PrivateData[KB_KEYMATRIX], 0, 32);
    pod->pod_PrivateData[KB_SENDLEDS] = TRUE;
    pod->pod_Flags = POD_IsKeyboard;
    break;
  case PD_ParsePodInput:
    DBUG(("Keyboard parse data\n"));
    base = interfaceStruct->pi_ControlPortBuffers->
	mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    break;
  case PD_AppendEventFrames:
#ifdef NOTDEF
    if (events & EVENTBIT0_ControlButtonArrived) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlButtonArrived,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	thisFrame->ef_EventData[0] = pod->pod_PrivateData[0];
	DBUG0(("Append a pad-button-arrival frame\n"));
      }
    }
#endif
    break;
  case PD_ProcessCommand:
    interfaceStruct->pi_CommandOutLen = 0;
    if (interfaceStruct->pi_CommandIn[0] != GENERIC_Keyboard ||
	interfaceStruct->pi_CommandIn[1] != GENERIC_KEYBOARD_SetLEDs) {
      return MAKEEB(ER_SEVERE,ER_C_STND,ER_NotSupported);
    }
/*
  Nasty sleaze - simply sets output to command data byte, no
  mapping or verification.
*/
    pod->pod_PrivateData[2] = interfaceStruct->pi_CommandIn[1];
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ConstructPodOutput:
    DBUG(("Build output for a keyboard\n"));
    dataOut = (pod->pod_PrivateData[KB_COMMANDOUT] << 24) |
      pod->pod_PrivateData[KB_DATAOUT];
    (*interfaceStruct->pi_PackBits)(dataOut,
				    24,
				    FALSE,
				    interfaceStruct->pi_ControlPortBuffers,
				    MB_OUTPUT_SEGMENT);
    break;
  default:
    break;
  }
  return 0;
}
