/*****

$Id: LightGunRom.c,v 1.5 1994/09/30 18:59:27 dplatt Exp $

$Log: LightGunRom.c,v $
 * Revision 1.5  1994/09/30  18:59:27  dplatt
 * Rename EventTable to PodStateTable to avoid misrepresenting its
 * true meaning and intended use.
 *
 * Revision 1.4  1994/09/24  04:01:28  dplatt
 * Implement the in-memory Current Events table.
 *
 * Revision 1.3  1994/03/19  01:14:59  dplatt
 * Set new pod-type flags to manage Control Port DMA characteristics.
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  LightGunROM.c - Control Port driverlet code (ROMable) for standard lightgun.
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

/* #define DEBUG */
/* #define DEBUG1 */
/* #define DEBUG2 */

#ifdef DEBUG
# define DBUG(x)  kprintf x
#else
# define DBUG(x) /* x */
#endif

#ifdef DEBUG2
# define DBUG2(x)  kprintf x
#else
# define DBUG2(x) /* x */
#endif

#define DBUG0(x) { kprintf x ; }

#include "strings.h"

/*
  3DO LightGun driverlet.

  Private data assignments:  0 is the current field's standard bit
  settings, 1 is the previous field's, 2 is the counter value read from
  the drive, 3 is the line hit count, 4 is the LED settings, 5 through
  7 are unused.
*/

static void StampFrame(EventFrame *thisFrame, Pod *thisPod) {
  thisFrame->ef_PodNumber = thisPod->pod_Number;
  thisFrame->ef_PodPosition = thisPod->pod_Position;
  thisFrame->ef_GenericPosition =
    thisPod->pod_GenericNumber[GENERIC_LightGun];
  DBUG(("Stamped event pod %d position %d generic %d\n",thisFrame->ef_PodNumber, thisFrame->ef_PodPosition, thisFrame->ef_GenericPosition));
}

static void FillFrame(EventFrame *thisFrame, Pod *thisPod, uint32 buttons)
{
  LightGunEventData *lged;
  StampFrame(thisFrame, thisPod);
  lged = (LightGunEventData *) &thisFrame->ef_EventData[0];
  lged->lged_ButtonBits = buttons;
  lged->lged_Counter = thisPod->pod_PrivateData[2];
  lged->lged_LinePulseCount = thisPod->pod_PrivateData[3];
}

Err DriverletEntry(PodInterface *interfaceStruct)
{
  Pod *pod;
  uint32 buttonBits, oldBits;
  uint32 buttonsDown, buttonsUp;
  uint32 clock, hitCount;
  uint32 events;
  uint8 *base;
  EventFrame *thisFrame;
  PodStateTable *pst;
  uint8 genericNum;
  pod = interfaceStruct->pi_Pod;
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG(("LightGun driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG(("LightGun pod init\n"));
    pod->pod_PrivateData[0] =
      pod->pod_PrivateData[1] =
	pod->pod_PrivateData[2] =
	  pod->pod_PrivateData[3] =
	    pod->pod_PrivateData[4] = 
	      pod->pod_PrivateData[5] =
		pod->pod_PrivateData[6] =
		  pod->pod_PrivateData[7] = 0;
    pod->pod_Flags = POD_IsLightGun + POD_ShortFramesOK;
    break;
  case PD_ParsePodInput:
    DBUG(("LightGun parse data\n"));
    oldBits = pod->pod_PrivateData[1] = pod->pod_PrivateData[0];
    base = interfaceStruct->pi_ControlPortBuffers->
      mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    buttonBits = (((uint32) base[1]) << 24) & 0xF8000000;
    clock =
      ((((uint32) base[1]) << 19) +
	(((uint32) base[2]) << 11) +
	  (((uint32) base[3]) << 3) +
	    ((((uint32) base[4]) >> 5) & 0x0F)) & 0x000FFFFF;
    hitCount = (uint32) base[4] & 0x1F;
    pod->pod_PrivateData[0] = buttonBits;
    pod->pod_PrivateData[2] = clock;
    pod->pod_PrivateData[3] = hitCount;
    buttonsDown = buttonBits & ~ oldBits;
    buttonsUp   = oldBits & ~ buttonBits;
    events = buttonsDown ?
      (buttonsUp ?
       (EVENTBIT0_LightGunButtonPressed + EVENTBIT0_LightGunButtonReleased +
	EVENTBIT0_LightGunUpdate + EVENTBIT0_LightGunDataArrived) :
       (EVENTBIT0_LightGunButtonPressed +
	EVENTBIT0_LightGunUpdate + EVENTBIT0_LightGunDataArrived)) :
	  (buttonsUp ?
	   (EVENTBIT0_LightGunButtonReleased +
	    EVENTBIT0_LightGunUpdate + EVENTBIT0_LightGunDataArrived) :
	   (EVENTBIT0_LightGunDataArrived));
    if ((buttonBits | oldBits) & LightGunTrigger) {
      DBUG2(("Button-bits 0x%x old-bits 0x%x Setting fire-track\n", buttonBits, oldBits));
      events |= EVENTBIT0_LightGunFireTracking;
    }
    pod->pod_EventsReady[0] = events;
    DBUG2(("Pod down 0x%x up 0x%x events 0x%x\n", buttonsDown, buttonsUp, events));
    break;
  case PD_AppendEventFrames:
    DBUG(("LightGun event-frame append\n"));
    DBUG(("Ready 0x%x look for 0x%x\n", pod->pod_EventsReady[0], interfaceStruct->pi_TriggerMask[0] | interfaceStruct->pi_CaptureMask[0]));
    events = (interfaceStruct->pi_TriggerMask[0] |
	      interfaceStruct->pi_CaptureMask[0]) & pod->pod_EventsReady[0];
    DBUG(("Combined mask 0x%x\n", events));
    if (events == 0 && ! interfaceStruct->pi_RecoverFromLostEvents) {
      break;
    }
    if (events & EVENTBIT0_LightGunButtonPressed) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_LightGunButtonPressed,
					 sizeof (LightGunEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0] &
		  ~ pod->pod_PrivateData[1]);
	DBUG(("Append a LightGun-button-pressed frame\n"));
      }
    }
    if (events & EVENTBIT0_LightGunButtonReleased) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_LightGunButtonReleased,
					 sizeof (LightGunEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[1] &
		  ~ pod->pod_PrivateData[0]);
	DBUG(("Append a LightGun-button-released frame\n"));
      }
    }
    if (interfaceStruct->pi_RecoverFromLostEvents ||
	(events & EVENTBIT0_LightGunUpdate) != 0) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_LightGunUpdate,
					 sizeof (LightGunEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG(("Append a LightGun-button-update frame\n"));
      }
    }
    if (events & EVENTBIT0_LightGunFireTracking) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_LightGunFireTracking,
					 sizeof (LightGunEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG(("Append a LightGun-fire-tracking frame\n"));
      }
    }
    if (events & EVENTBIT0_LightGunDataArrived) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_LightGunDataArrived,
					 sizeof (LightGunEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG(("Append a LightGun-data-arrival frame\n"));
      }
    }
    break;
  case PD_ProcessCommand:
    interfaceStruct->pi_CommandOutLen = 0;
    if (interfaceStruct->pi_CommandIn[0] != GENERIC_LightGun ||
	interfaceStruct->pi_CommandIn[1] != GENERIC_LIGHTGUN_SetLEDs) {
      return MAKEEB(ER_SEVERE,ER_C_STND,ER_NotSupported);
    }
    pod->pod_PrivateData[4] = (uint32) interfaceStruct->pi_CommandIn[1] & 0xC0;
    pod->pod_Blipvert = TRUE; /* output bits changed, must send */
    break;
  case PD_ConstructPodOutput:
    DBUG(("Build output for a control pad\n"));
    (*interfaceStruct->pi_PackBits)(pod->pod_PrivateData[4],
				    8,
				    FALSE,
				    interfaceStruct->pi_ControlPortBuffers,
				    MB_OUTPUT_SEGMENT);
    break;
  case PD_UpdatePodStateTable:
    pst = interfaceStruct->pi_PodStateTable;
    genericNum = pod->pod_GenericNumber[GENERIC_LightGun];
    if (pst && pst->pst_LightGunTable.gt_HowMany > genericNum) {
      pst->pst_LightGunTable.gt_EventSpecificData[genericNum].lged_ButtonBits =
	pod->pod_PrivateData[0];
      pst->pst_LightGunTable.gt_EventSpecificData[genericNum].lged_Counter =
	pod->pod_PrivateData[2];
      pst->pst_LightGunTable.gt_EventSpecificData[genericNum].lged_LinePulseCount =
	pod->pod_PrivateData[3];
      pst->pst_LightGunTable.gt_ValidityTimeStamps[genericNum] =
	interfaceStruct->pi_VCNT;
    }
    break;
  default:
    break;
  }
  return 0;
}
