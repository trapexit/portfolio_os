/*****

$Id: StickDriver.c,v 1.7 1994/09/30 18:59:27 dplatt Exp $

$Log: StickDriver.c,v $
 * Revision 1.7  1994/09/30  18:59:27  dplatt
 * Rename EventTable to PodStateTable to avoid misrepresenting its
 * true meaning and intended use.
 *
 * Revision 1.6  1994/09/24  04:01:28  dplatt
 * Implement the in-memory Current Events table.
 *
 * Revision 1.5  1994/03/19  01:14:59  dplatt
 * Set new pod-type flags to manage Control Port DMA characteristics.
 *
 * Revision 1.4  1994/02/24  23:41:30  dplatt
 * Make sure that the capability bits get added into the events.
 *
 * Revision 1.3  1994/02/05  01:24:35  dplatt
 * Support the new spec...
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  StickDriver.c - Control Port driverlet code for 3DO analog JoyStick.
*/

#ifdef CONTROLPORT

#include "types.h"
#include "debug.h"
#include "poddriver.h"

#ifdef XXXXXX
extern int32 debugFlag;
#endif /* XXXXXX */

/* #define DEBUG */

#ifdef DEBUG
# define DBUG(x)  kprintf x
#else
# define DBUG(x) /* x */
#endif

#ifdef DEBUG2
# define DBUG2(x)  printf x
#else
# define DBUG2(x) /* x */
#endif

#ifdef XXXXXX
#define DBUG0(x) if (debugFlag) { printf x ; }
#else
#define DBUG0(x) kprintf x
#endif /* XXXXXX */

#include "strings.h"

/*
  3DO analog JoyStick driverlet.

  Private data assignments:
  0 is the current field's standard bit settings,
  1 is the previous field's,
  2 is the current X position,
  3 is the current Y position,
  4 is the current Z position,
  5 is the previous fields X position
  6 is the previous fields Y position
  7 is the previous fields Z position
*/

static void StampFrame(EventFrame *thisFrame, Pod *thisPod) {
  thisFrame->ef_PodNumber = thisPod->pod_Number;
  thisFrame->ef_PodPosition = thisPod->pod_Position;
  thisFrame->ef_GenericPosition =
    thisPod->pod_GenericNumber[GENERIC_Stick];
  DBUG2(("Stamped event pod %d position %d generic %d\n",thisFrame->ef_PodNumber, thisFrame->ef_PodPosition, thisFrame->ef_GenericPosition));
}

static void FillFrame(EventFrame *thisFrame, Pod *thisPod, uint32 buttons)
{
  StickEventData *stk;
  StampFrame(thisFrame, thisPod);
  stk = (StickEventData *) &thisFrame->ef_EventData[0];
  stk->stk_ButtonBits = buttons;
  stk->stk_HorizPosition = thisPod->pod_PrivateData[2];
  stk->stk_VertPosition = thisPod->pod_PrivateData[3];
  stk->stk_DepthPosition = thisPod->pod_PrivateData[4];
}

Err DriverletEntry(PodInterface *interfaceStruct, struct KernelBase *kbase)
{
  Pod *pod;
  uint32 buttonBits, oldButtonBits;
  uint32 buttonsDown, buttonsUp;
  uint32 events;
  uint8 *base;
  int32 xPos, yPos, zPos;
  EventFrame *thisFrame;
  PodStateTable *pst;
  uint8 genericNum;
  pod = interfaceStruct->pi_Pod;
  switch (interfaceStruct->pi_Command) {
  case PD_InitDriver:
    DBUG(("Stick72 driverlet init\n"));
    break;
  case PD_InitPod:
    DBUG(("Stick72 pod init\n"));
    pod->pod_PrivateData[0] =
      pod->pod_PrivateData[1] =
	pod->pod_PrivateData[2] =
	  pod->pod_PrivateData[3] =
	    pod->pod_PrivateData[4] =
	      pod->pod_PrivateData[5] =
		pod->pod_PrivateData[6] =
		  pod->pod_PrivateData[7] = 0;
    pod->pod_Flags = POD_IsStick + POD_ShortFramesOK;
    break;
  case PD_ParsePodInput:
    DBUG2(("Stick parse data:"));
    oldButtonBits = pod->pod_PrivateData[1] = pod->pod_PrivateData[0];
    base = interfaceStruct->pi_ControlPortBuffers->
      mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase + pod->pod_InputByteOffset;
    xPos = (((uint32) base[3]) << 2) + ((((uint32) base[4]) & 0xc0) >> 6);
    yPos = ((((uint32) base[4]) & 0x3f) << 4) + ((((uint32) base[5]) & 0xf0) >> 4);
    zPos = ((((uint32) base[5]) & 0xf) << 6) + ((((uint32) base[6]) & 0xfc) >> 2);
    DBUG2(("X pos 0x%x Y pos 0x%x Z pos 0x%x\n", xPos, yPos, zPos));
    buttonBits = ((((uint32) base[6]) & 0x3)<<18) +
							(((uint32) base[7])<<24) +
								((((uint32) base[8]) & 0xf0)<<16);
    buttonsDown = buttonBits & ~ oldButtonBits;
    buttonsUp   = oldButtonBits & ~ buttonBits;
    DBUG2(("base[6] = 0x%x base[7] = 0x%x base[8] = 0x%x\n", base[6], base[7], base[8]));
    DBUG2(("buttonBits = 0x%x buttonsUp = 0x%x buttonsDown = 0x%x\n", buttonBits, buttonsUp, buttonsDown));
#ifdef XXXXXX
	if((buttonsUp != 0 || buttonsDown != 0) && xPos > 512
	) {
		DBUG(("X pos 0x%x Y pos 0x%x Z pos 0x%x\n", xPos, yPos, zPos));
		DBUG(("buttonBits = 0x%x buttonsUp = 0x%x buttonsDown = 0x%x\n", buttonBits, buttonsUp, buttonsDown));
	}
#endif
    events = buttonsDown ?
      (buttonsUp ?
       (EVENTBIT0_StickButtonPressed + EVENTBIT0_StickButtonReleased +
	EVENTBIT0_StickUpdate + EVENTBIT0_StickDataArrived) :
       (EVENTBIT0_StickButtonPressed +
	EVENTBIT0_StickUpdate + EVENTBIT0_StickDataArrived)) :
	  (buttonsUp ?
	   (EVENTBIT0_StickButtonReleased +
	    EVENTBIT0_StickUpdate + EVENTBIT0_StickDataArrived) :
	   (EVENTBIT0_StickDataArrived));
#define FUZZ_VAL 1
#define abs(x) ((x) > 0)?(x):-(x)
    if (abs(pod->pod_PrivateData[5] - xPos) > FUZZ_VAL ||
	abs(pod->pod_PrivateData[6] - yPos) > FUZZ_VAL ||
	abs(pod->pod_PrivateData[7] - zPos) > FUZZ_VAL) {
      pod->pod_PrivateData[2] = xPos;
      pod->pod_PrivateData[3] = yPos;
      pod->pod_PrivateData[4] = zPos;
      events |= EVENTBIT0_StickMoved;
    }
    pod->pod_EventsReady[0] = events;
    DBUG2(("Pod down 0x%x up 0x%x events 0x%x\n", buttonsDown, buttonsUp, events));
    pod->pod_PrivateData[0] = buttonBits;
    break;
  case PD_AppendEventFrames:
    DBUG2(("Stick event-frame append\n"));
    DBUG2(("Ready 0x%x look for 0x%x\n", pod->pod_EventsReady[0], interfaceStruct->pi_TriggerMask[0] | interfaceStruct->pi_CaptureMask[0]));
    events = (interfaceStruct->pi_TriggerMask[0] |
	      interfaceStruct->pi_CaptureMask[0]) & pod->pod_EventsReady[0];
    DBUG2(("Combined mask 0x%x\n", events));
    if (events == 0 && ! interfaceStruct->pi_RecoverFromLostEvents) {
      break;
    }
    if (events & EVENTBIT0_StickButtonPressed) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_StickButtonPressed,
					 sizeof (StickEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, (pod->pod_PrivateData[0] &
		  ~ pod->pod_PrivateData[1]) | (pod->pod_PrivateData[0] & 0xC0000));
	DBUG2(("Append a stick-button-pressed frame\n"));
      }
    }
    if (events & EVENTBIT0_StickButtonReleased) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_StickButtonReleased,
					 sizeof (StickEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, (pod->pod_PrivateData[1] &
		  ~ pod->pod_PrivateData[0]) | (pod->pod_PrivateData[0] & 0xC0000));
	DBUG2(("Append a stick-button-released frame\n"));
      }
    }
    if (interfaceStruct->pi_RecoverFromLostEvents ||
	(events & EVENTBIT0_StickUpdate) != 0) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_StickUpdate,
					 sizeof (StickEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG2(("Append a stick-button-update frame\n"));
      }
    }
    if (events & EVENTBIT0_StickMoved) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_StickMoved,
					 sizeof (StickEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG2(("Append a stick-moved frame\n"));
      }
    }
    if (events & EVENTBIT0_StickDataArrived) {
      thisFrame =
	(*interfaceStruct->pi_InitFrame)(EVENTNUM_StickDataArrived,
					 sizeof (StickEventData),
					 &interfaceStruct->pi_NextFrame,
					 &interfaceStruct->pi_EndOfFrameArea);
      if (thisFrame) {
	FillFrame(thisFrame, pod, pod->pod_PrivateData[0]);
	DBUG2(("Append a stick-data-arrival frame\n"));
      }
    }
    break;
  case PD_ProcessCommand:
    DBUG(("Stick driverlet precess command\n"));
    break;
  case PD_TeardownPod:
    DBUG(("Stick driverlet tear down\n"));
    break;
  case PD_ShutdownDriver:
    DBUG(("Stick driverlet shut down\n"));
    break;
  case PD_ReconnectPod:
    DBUG(("Stick driverlet reconnect\n"));
    break;
  case PD_UpdatePodStateTable:
    pst = interfaceStruct->pi_PodStateTable;
    genericNum = pod->pod_GenericNumber[GENERIC_Stick];
    if (pst && pst->pst_StickTable.gt_HowMany > genericNum) {
      pst->pst_StickTable.gt_EventSpecificData[genericNum].stk_ButtonBits =
	pod->pod_PrivateData[0];
      pst->pst_StickTable.gt_EventSpecificData[genericNum].stk_HorizPosition =
	pod->pod_PrivateData[2];
      pst->pst_StickTable.gt_EventSpecificData[genericNum].stk_VertPosition =
	pod->pod_PrivateData[3];
      pst->pst_StickTable.gt_EventSpecificData[genericNum].stk_DepthPosition =
	pod->pod_PrivateData[4];
      pst->pst_StickTable.gt_ValidityTimeStamps[genericNum] =
	interfaceStruct->pi_VCNT;
    }
    break;
  default:
    break;
  }
  return 0;
}

#endif
