/*****

$Id: 

$Log: 
*****/

/*
  Copyright The 3DO Company Inc., 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  EventUtil.c - Event convenience utilities.
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
#include "msgport.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "operror.h"
#include "event.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

#include "strings.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *inputlib_version;
static void *inpulib_version = &inputlib_version;
#endif


/****************************************************************************/


#define MAXPAD   255
#define MAXMOUSE 255

static int32 NumControlPads, NumMice;

static struct pad {
  int32               pad_Avail;
  ControlPadEventData pad_Data;
} *pads;

static struct mouse {
  int32               mouse_Avail;
  MouseEventData      mouse_Data;
} *mice;

static Item msgPortItem;
static Item brokerPortItem;
static Item msgItem;
static MsgPort *msgPort;

static Err ChewOneEvent(int32 wait);

/* #define DEBUG */

#ifdef DEBUG
# define DBUG(x)  printf x
#else
# define DBUG(x) /* x */
#endif


Err InitEventUtility (int32 numControlPads, int32 numMice, int32 isFocused)
{
  Err err;

  static ConfigurationRequest config;

  int32 mouseSize, padSize;

  brokerPortItem = FindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),
				 EventPortName);

  if (brokerPortItem < 0) {
    err =  brokerPortItem;
    goto bail;
  }

  msgPortItem = CreateMsgPort(CURRENTTASK->t.n_Name, 0, 0);

  if (msgPortItem < 0) {
    err = msgPortItem;
    goto bail;
  }

  msgPort = (MsgPort *) LookupItem(msgPortItem);

  msgItem = CreateMsg(NULL, 0, msgPortItem);

  if (msgItem < 0) {
    err = msgItem;
    goto bail;
  }

  if (numControlPads > MAXPAD) {
    NumControlPads = MAXPAD;
  } else if (numControlPads == 0) {
    NumControlPads = 1;
  } else {
    NumControlPads = numControlPads;
  }

  if (numMice > MAXMOUSE) {
    NumMice = MAXMOUSE;
  } else if (numMice == 0) {
    NumMice = 1;
  } else {
    NumMice = numMice;
  }

  padSize = (NumControlPads + 1) * sizeof (struct pad);

  pads = (struct pad *) malloc(padSize);

  if (!pads) {
    err = -1; /* need better */
    goto bail;
  } else {
    memset(pads, 0, padSize);
  }

  DBUG(("Pads structure allocated at 0x%x\n", pads));
    
  mouseSize = (NumMice + 1) * sizeof (struct mouse);

  mice = (struct mouse *) malloc(mouseSize);

  if (!mice) {
    err = -1; /* need better */
    goto bail;
  } else {
    memset(mice, 0, mouseSize);
  }
    
  DBUG(("Mice structure allocated at 0x%x\n", mice));
    
  config.cr_Header.ebh_Flavor = EB_Configure;
  config.cr_Category = (isFocused == LC_ISFOCUSED) ?
		       LC_FocusListener : LC_Observer;
  memset(&config.cr_TriggerMask, 0x00, sizeof config.cr_TriggerMask);
  memset(&config.cr_CaptureMask, 0x00, sizeof config.cr_CaptureMask);
  config.cr_QueueMax = 0; /* let the Broker set it */;
  config.cr_TriggerMask[0] =
    EVENTBIT0_ControlButtonUpdate |
      EVENTBIT0_MouseUpdate |
	EVENTBIT0_MouseMoved ;

  err = SendMsg(brokerPortItem, msgItem, &config, sizeof config);

  if (err < 0) {
    goto bail;
  }

  return 0;

 bail:
  KillEventUtility();
  return err;
}

Err GetControlPad(int32 padNumber, int32 wait, ControlPadEventData *data)
{
  Err err;
  if (padNumber < 1 || padNumber > NumControlPads || !pads) {
    return -1;
  }
  DBUG(("Get pad %d, wait flag %d\n", padNumber, wait));
  while (TRUE) {
    if (pads[padNumber].pad_Avail) {
      pads[padNumber].pad_Avail = FALSE;
      *data = pads[padNumber].pad_Data;
      return 1;
    }
    err = ChewOneEvent(wait);
    if (err < 0) {
      return err;
    }
    if (err == 0 && !wait && !pads[padNumber].pad_Avail) {
      *data = pads[padNumber].pad_Data;
      return 0;
    }
  }
}

Err GetMouse(int32 mouseNumber, int32 wait, MouseEventData *data)
{
  Err err;
  if (mouseNumber < 1 || mouseNumber > NumMice || !mice) {
    return -1;
  }
  DBUG(("Get mouse %d, wait flag %d\n", mouseNumber, wait));
  while (TRUE) {
    if (mice[mouseNumber].mouse_Avail) {
      mice[mouseNumber].mouse_Avail = FALSE;
      *data = mice[mouseNumber].mouse_Data;
      return 1;
    }
    err = ChewOneEvent(wait);
    if (err < 0) {
      return err;
    }
    if (err == 0 && !wait && !mice[mouseNumber].mouse_Avail) {
      *data = mice[mouseNumber].mouse_Data;
      return 0;
    }
  }
}

static Err ChewOneEvent(int32 wait)
{
  int32 eventItem;
  int32 generic;
  uint32 theSignal;
  Message *event;
  EventBrokerHeader *hdr;
  EventFrame *frame;
  do {
    eventItem = GetMsg(msgPortItem);
    if (eventItem < 0) {
      return eventItem;
    }
    if (eventItem == 0) {
      if (!wait) {
	return 0;
      }
      theSignal = WaitSignal(msgPort->mp_Signal);
      if ((theSignal & msgPort->mp_Signal) == 0) {
	return 0; /* something odd - abort signal? */
      }
      eventItem = GetMsg(msgPortItem);
    }
    if (eventItem != 0) {
      DBUG(("Got message 0x%x\n"));
      event = (Message *) LookupItem(eventItem);
      if (eventItem == msgItem) {
	if ((int32) event->msg_Result < 0) {
	  return ((int32) event->msg_Result);
	}
	eventItem = 0; /* keep going */
      } else {
	hdr = (EventBrokerHeader *) event->msg_DataPtr;
	switch (hdr->ebh_Flavor) {
	case EB_EventRecord:
	  frame = (EventFrame *) (hdr + 1); /* Gawd that's ugly! */
	  while (TRUE) {
	    if (frame->ef_ByteCount == 0) {
	      break;
	    }
	    generic = frame->ef_GenericPosition;
	    switch (frame->ef_EventNumber) {
	    case EVENTNUM_ControlButtonUpdate:
	      if (generic >= 1 && generic <= NumControlPads) {
		memcpy(&pads[generic].pad_Data, frame->ef_EventData,
		       sizeof (ControlPadEventData));
		pads[generic].pad_Avail = TRUE;
	      }
	      break;
	    case EVENTNUM_MouseUpdate:
	    case EVENTNUM_MouseMoved:
	      if (generic >= 1 && generic <= NumMice) {
		memcpy(&mice[generic].mouse_Data, frame->ef_EventData,
		       sizeof (MouseEventData));
		mice[generic].mouse_Avail = TRUE;
	      }
	      break;
	    default:
	      DBUG(("Event type %d\n", frame->ef_EventNumber));
	      break;
	    }
	    frame = (EventFrame *) (frame->ef_ByteCount + (char *) frame);
	  }
	  break;
	default:
	  break;
	}
	ReplyMsg(eventItem, 0, NULL, 0);
      }
    }
  } while (eventItem == 0);
  return 1;
}

Err KillEventUtility(void)
{
  if (pads) {
    free(pads);
    pads = NULL;
  }
  if (mice) {
    free(mice);
    mice = NULL;
  }
  if (msgItem > 0) {
    DeleteItem(msgItem);
    msgItem = 0;
  }
  if (msgPortItem > 0) {
    DeleteItem(msgPortItem);
    msgPortItem = 0;
  }
  return 0;
}  
