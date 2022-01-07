/*****

$Id: ControlPadDriver.c,v 1.9 1994/12/07 18:18:36 dplatt Exp $

$Log: ControlPadDriver.c,v $
 * Revision 1.9  1994/12/07  18:18:36  dplatt
 * Generic command to control audio channels does not work properly...
 * it always turns audio off.  Picked up the wrong byte from the
 * generic command sequence...
 *
 * Revision 1.8  1994/09/30  18:59:27  dplatt
 * Rename EventTable to PodStateTable to avoid misrepresenting its
 * true meaning and intended use.
 *
 * Revision 1.7  1994/09/24  04:01:28  dplatt
 * Implement the in-memory Current Events table.
 *
 * Revision 1.6  1994/03/19  01:14:59  dplatt
 * Set new pod-type flags to manage Control Port DMA characteristics.
 *
 * Revision 1.5  1994/01/04  00:43:49  dplatt
 * Remove residue of old-style control-pad support.
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  ControlPadDriver.c - Control Port driverlet code for 3DO Control
  Pads.

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
  3DO Control Pad driverlet.  Handles the old-style 8-bit control pad
  (at least for now), and the standard and two extended-form control
  pads.

  Private data assignments:  0 is the current field's standard bit
  settings, 1 is the previous field's, 2 is the output data, 3 is
  unused.
*/

static void StampFrame(EventFrame *thisFrame, Pod *thisPod) {
  thisFrame->ef_PodNumber = thisPod->pod_Number;
  thisFrame->ef_PodPosition = thisPod->pod_Position;
  thisFrame->ef_GenericPosition =
    thisPod->pod_GenericNumber[GENERIC_ControlPad];
  DBUG0(("Stamped event pod %d position %d generic %d\n",thisFrame->ef_PodNumber, thisFrame->ef_PodPosition, thisFrame->ef_GenericPosition));
}

Err ControlPadDriver(PodInterface *interfaceStruct)
{
  Pod *pod;
  uint32 decodedBits, oldBits;
  uint32 buttonsDown, buttonsUp;
  uint32 events;
  uint8 *base;
  EventFrame *thisFrame;
  PodStateTable *pst;
  uint8 genericNum;
  ControlPadEventData *eventData;
  pod = interfaceStruct->pi_Pod;
  DBUG(("Control Pad driver, function %d\n", interfaceStruct->pi_Command));
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG0(("Control Pad driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG0(("Control Pad pod init\n"));
    pod->pod_PrivateData[0] =
      pod->pod_PrivateData[1] =
	pod->pod_PrivateData[3] = 0;
    pod->pod_PrivateData[2] = AUDIO_Channels_Stereo;
    pod->pod_Flags = POD_IsControlPad + POD_IsAudioCtlr +
      POD_ShortFramesOK + POD_MultipleFramesOK + POD_FilteringOK;
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ReconnectPod:
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ParsePodInput:
    DBUG(("Control Pad parse data\n"));
    oldBits = pod->pod_PrivateData[1] = pod->pod_PrivateData[0];
    base = interfaceStruct->pi_ControlPortBuffers->
	mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    switch (pod->pod_Type) {
    case 0x80:
    case 0xA0:
      decodedBits = (((uint32) base[0] << 8) + base[1]) << 19;
      break;
    case 0xC0:
      decodedBits = (((((uint32) base[0] << 8) + base[1]) << 8) + base[2]) << 11;
      break;
    }
    pod->pod_PrivateData[0] = decodedBits;
    buttonsDown = decodedBits & ~ oldBits;
    buttonsUp   = oldBits & ~ decodedBits;
    events = buttonsDown ?
      (buttonsUp ?
       (EVENTBIT0_ControlButtonPressed + EVENTBIT0_ControlButtonReleased +
	EVENTBIT0_ControlButtonUpdate + EVENTBIT0_ControlButtonArrived) :
       (EVENTBIT0_ControlButtonPressed +
	EVENTBIT0_ControlButtonUpdate + EVENTBIT0_ControlButtonArrived)) :
	  (buttonsUp ?
	   (EVENTBIT0_ControlButtonReleased +
	    EVENTBIT0_ControlButtonUpdate + EVENTBIT0_ControlButtonArrived) :
	   (EVENTBIT0_ControlButtonArrived));
    pod->pod_EventsReady[0] = events;
    if (buttonsDown) {
      DBUG(("Pod bytes 0x%x 0x%x\n", base[0], base[1]));
    }
    DBUG(("Pod down 0x%x up 0x%x events 0x%x\n", buttonsDown, buttonsUp, events));
    break;
  case PD_AppendEventFrames:
    DBUG0(("Control Pad event-frame append\n"));
    DBUG0(("Ready 0x%x look for 0x%x\n", pod->pod_EventsReady[0], interfaceStruct->pi_TriggerMask[0] | interfaceStruct->pi_CaptureMask[0]));
    events = (interfaceStruct->pi_TriggerMask[0] |
	      interfaceStruct->pi_CaptureMask[0]) & pod->pod_EventsReady[0];
    DBUG0(("Combined mask 0x%x\n", events));
    if (events == 0 && ! interfaceStruct->pi_RecoverFromLostEvents) {
      break;
    }
    if (events & EVENTBIT0_ControlButtonPressed) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlButtonPressed,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	thisFrame->ef_EventData[0] = pod->pod_PrivateData[0] &
	  ~ pod->pod_PrivateData[1];
	DBUG0(("Append a pad-button-pressed frame\n"));
      }
    }
    if (events & EVENTBIT0_ControlButtonReleased) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlButtonReleased,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	thisFrame->ef_EventData[0] = pod->pod_PrivateData[1] &
	  ~ pod->pod_PrivateData[0];
	DBUG0(("Append a pad-button-released frame\n"));
      }
    }
    if (interfaceStruct->pi_RecoverFromLostEvents ||
	(events & EVENTBIT0_ControlButtonUpdate) != 0) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_ControlButtonUpdate,
					 sizeof (ControlPadEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	StampFrame(thisFrame, pod);
	thisFrame->ef_EventData[0] = pod->pod_PrivateData[0];
	DBUG0(("Append a pad-button-update frame\n"));
      }
    }
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
    break;
  case PD_ProcessCommand:
    interfaceStruct->pi_CommandOutLen = 0;
    if (interfaceStruct->pi_CommandIn[0] != GENERIC_AudioCtlr ||
	interfaceStruct->pi_CommandIn[1] != GENERIC_AUDIO_SetChannels) {
      return MAKEEB(ER_SEVERE,ER_C_STND,ER_NotSupported);
    }
/*
  Nasty sleaze - simply sets output to command data byte, no
  mapping or verification.
*/
    pod->pod_PrivateData[2] = interfaceStruct->pi_CommandIn[2];
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ConstructPodOutput:
    DBUG(("Build output for a control pad\n"));
    if (pod->pod_BitsOut == 2) {
      (*interfaceStruct->pi_PackBits)(pod->pod_PrivateData[2],
				      2,
				      FALSE,
				      interfaceStruct->pi_ControlPortBuffers,
				      MB_OUTPUT_SEGMENT);
    }
    break;
  case PD_UpdatePodStateTable:
    pst = interfaceStruct->pi_PodStateTable;
    genericNum = pod->pod_GenericNumber[GENERIC_ControlPad];
    if (pst && pst->pst_ControlPadTable.gt_HowMany > genericNum) {
      pst->pst_ControlPadTable.gt_EventSpecificData[genericNum].cped_ButtonBits =
	pod->pod_PrivateData[0];
      pst->pst_ControlPadTable.gt_ValidityTimeStamps[genericNum] =
	interfaceStruct->pi_VCNT;
    }
    break;
  default:
    break;
  }
  return 0;
}
