/* *************************************************************************
 *
 * FIRQ handler for the graphics folio.
 *
 * Copyright (C) 1992,1993,1994, The 3DO Company
 * 3DO Trade Secrets  -  Confidential and Proprietary
 *
 * $Id: firq.c,v 1.6 1994/12/21 21:13:02 ewhac Exp $
 *
 * DATE    NAME             DESCRIPTION
 * ------- ---------------- -------------------------------------------------
 * 9403.30 slandrum         Split FIRQ handler back out of intgraf.c
 * 9403.30 slandrum         Modified firq handler to be state engine for startup
 *
 * ********************************************************************** */

/***************************************************************\
* Header files
\***************************************************************/


#include "types.h"

#include "mem.h"

#include "debug.h"
#include "item.h"
#include "nodes.h"
#include "interrupts.h"
#include "kernel.h"
#include "list.h"
#include "task.h"
#include "folio.h"
#include "kernelnodes.h"
#include "super.h"
#include "time.h"

#include "intgraf.h"

#include "stdarg.h"
#include "strings.h"
#include "stdio.h"

#include "inthard.h"
#include "clio.h"

#include "overlay.h"


/***************************************************************************
 * Local prototypes
 */
static void	signalVBLwaiters (void);
static void	markVDLsfree (void);


/***************************************************************\
* Data & necessary structures
\***************************************************************/
typedef struct VBLWaiter {
	MinNode	vw_Link;
	Task	*vw_Task;
} VBLWaiter;


extern Overlay	*ActiveOverlay;
extern uint32	_HDelayOverride;
extern uint32	*_PatchDISPCTRL1, *_PatchDISPCTRL2, *_PatchDISPCTRL3;
extern uint32	_fieldcount;
extern volatile uint32	_tripaddress;

uint32		_suppress_MCTL;
uint32		SEEDv = 0xC693B70F;
GrafFolio	*GrafBase;
uint32		_firqstate=0;


static List	vdlstofree = INITLIST (vdlstofree, NULL);
static List	vblwaiters = INITLIST (vblwaiters, NULL);
static int32	nvdlstofree;
static int32	nvblwaiters;

static struct timeval _tv1, _tv2;
int32 _timediff;


/***************************************************************\
* Code
\***************************************************************/




uint32
graphicsFirq (void)
{
  int32 i, j, k, l;
  VDL *v;

  _fieldcount++;

  switch (_firqstate) {
  case 0:
    /* kill one VBL in case the first interrupt is delayed */
    _firqstate++;
    break;
  case 1:
    /* get initial timestamp */
    TimeStamp (&_tv1);
    _firqstate++;
    break;
  case 2:
    /* get timestamp difference */
    TimeStamp (&_tv2);
    _timediff = _tv2.tv_usec - _tv1.tv_usec;
    if (_timediff<0) _timediff += 1000000;
    _firqstate++;
    break;
  case 3:
    /* sit in a holding pattern */
    break;
  case 4:
    /* normal vblank interrupt processing */

    /* Initialize the video display RNG */
    *SEED = SEEDv;

    /* write to a location that a state analyzer can catch if we get in too late */
    i = *VCNT;
    j = i & VCNT_MASK;
    if (j>=15) {
      _tripaddress = j;
    }

    /* Increment the VBLNumber field */
    /* Make sure LSB matches odd/evenness of the video field */
    k = GrafBase->gf_VBLNumber;
    k++;
    k += (k & 1) ^ ((i & VCNT_FIELD) >> VCNT_FIELD_SHIFT);
    GrafBase->gf_VBLNumber = k;

    /* Select the current VDL to display */
    if (k&1) {
      v = GrafBase->gf_CurrentVDLOdd;
    } else {
      v = GrafBase->gf_CurrentVDLEven;
    }
    i = *VCNT;	/*  Sample VCNT just as we stuff this pointer.  */
    *GrafBase->gf_VDLDisplayLink = (uint32)v->vdl_DataPtr;
/*    **GrafBase->gf_DisplayLinkPtr = v->vdl_DataPtr;	*/

    /*
     * "Flag" old VDLs as safe to discard (but only if we got here in time).
     * (That hard-coded "15" has got to go...)
     */
    if (nvdlstofree  &&  (i & VCNT_MASK) < 15)
      markVDLsfree ();

    /*
     * Man, we're makin' this FIRQ do boatloads of work...
     */
    if (ActiveOverlay) {
      register Overlay		*ov;
      register OverlayVDL	*ovv;
      register FullVDL		*fv;
      register uint32		f, bank;

      ov = ActiveOverlay;
      if ((f = ov->ov_Flags) & OVF_INSTALLBANKMASK) {
	/*
	 * See if we need to patch a VDL (this is the only place we can do
	 * it safely).
	 */
	if (f & OVF_INSTALLBANK_0)
	  bank = 0;
	else
	  bank = 2;
	ov->ov_Flags = f & ~OVF_INSTALLBANKMASK;

	ovv = &ov->ov_VDL[bank];
	if ((fv = ovv->ovv_Stomped) != NULL) {
	  f = fv->fv.DMACtl;
	  fv->fv.NextVDL = (VDLHeader *) ovv->ovv_Receive;
	  fv->fv.DMACtl  = ovv->ovv_StompDMAC;
	  ovv->ovv_StompDMAC = f;
	}
	ovv++;
	if ((fv = ovv->ovv_Stomped) != NULL) {
	  f = fv->fv.DMACtl;
	  fv->fv.NextVDL = (VDLHeader *) ovv->ovv_Receive;
	  fv->fv.DMACtl  = ovv->ovv_StompDMAC;
	  ovv->ovv_StompDMAC = f;
	}
      }
      else if ((f = ov->ov_Flags) & OVF_PENDINGBANKMASK) {
	/*
	 * Mark pending bank as in use.  Mark other bank as free.
	 */
	if (f & OVF_PENDINGBANK_0) {
	  f &= ~(OVF_FREEBANK_0 | OVF_PENDINGBANK_0);
	  f |= OVF_FREEBANK_1;
	  bank = 2;
	} else {
	  f &= ~(OVF_FREEBANK_1 | OVF_PENDINGBANK_1);
	  f |= OVF_FREEBANK_0;
	  bank = 0;
	}
	ov->ov_Flags = f;

	/*
	 * Unsplice VDLs.
	 */
	ovv = &ov->ov_VDL[bank];
	if ((fv = ovv->ovv_Stomped) != NULL) {
	  fv->fv.NextVDL = (VDLHeader *) ovv->ovv_StompNext;
	  fv->fv.DMACtl  = ovv->ovv_StompDMAC;
	  ovv->ovv_Stomped = NULL;
	  ovv->ovv_Spliced->vdl_Flags &= ~VDLF_SPLICED;
	  ovv->ovv_Spliced = NULL;
	}
	ovv++;
	if ((fv = ovv->ovv_Stomped) != NULL) {
	  fv->fv.NextVDL = (VDLHeader *) ovv->ovv_StompNext;
	  fv->fv.DMACtl  = ovv->ovv_StompDMAC;
	  ovv->ovv_Stomped = NULL;
	  ovv->ovv_Spliced->vdl_Flags &= ~VDLF_SPLICED;
	  ovv->ovv_Spliced = NULL;
	}
      }

      if (ov->ov_SigTask) {
	SuperInternalSignal (ov->ov_SigTask, ov->ov_SigMask);
	ov->ov_SigTask = NULL;
      }
    }

    /*
     * Signal anyone waiting for a vertical blank.
     */
    if (nvblwaiters)
      signalVBLwaiters ();

/*
 * I'll be re-engineering this forthwith, once I get the cable $#!+ working...
 */
    /* Get number of lines to display for this VDL and center display vertically */
    l = (*GrafBase->gf_VDLPreDisplay & ~VDL_LINE_MASK) |
        ((v->vdl_Offset + 1) << VDL_LINE_SHIFT);
    *GrafBase->gf_VDLPreDisplay = l;

    /* Program HDelay for the current display type */
    l = v->vdl_DisplayType;
    if (_suppress_MCTL) {
      if (_suppress_MCTL == 7) {
	*MCTL &= ~SC384EN;
      }
      if (_suppress_MCTL == 3) {
	*MCTL |= SC384EN;
      }
      _suppress_MCTL--;
    } else {
      *MCTL = (*MCTL & ~SC384EN) | _DisplayMCTL[l];
    }

    if (_HDelayOverride) {
      if (*HDELAY != _HDelayOverride) {
	*HDELAY = _HDelayOverride;
      }
    } else {
      if (*HDELAY != _DisplayHDelay[l]) {
	*HDELAY = _DisplayHDelay[l];
      }
    }

    /* Patch system portion of VDLs to reflect correct display width */
    PALpatch (_DisplayDISPCTRL[l]);

    break;
  default:
    break;
  }

  return 0;
}


/***************************************************************************
 * Schedule a VDL Item for disposal.  The FIRQ will mark the Item as
 * disposable on the next VBL.
 */
void
addVDLtofree (vdl)
struct VDL	*vdl;
{
	register uint32	oldints;

	oldints = Disable ();
	if (!vdl->vdl.n_Next) {
		AddTail (&vdlstofree, (Node *) vdl);
		nvdlstofree++;
	}
	Enable (oldints);
}

void
removeVDLtofree (vdl)
struct VDL	*vdl;
{
	register uint32	oldints;

	oldints = Disable ();
	if (vdl->vdl.n_Next) {
		RemNode ((Node *) vdl);
		vdl->vdl.n_Next = vdl->vdl.n_Prev = NULL;
		nvdlstofree--;
	}
	Enable (oldints);
}


/*
 * A little goodie to assist internal graphics routines.
 */
void
graf_waitVBL (void)
{
	register uint32	oldints;
	VBLWaiter	vw;

	ClearSignals (CURRENTTASK, SIGF_ONESHOT);

	vw.vw_Task = CURRENTTASK;

	oldints = Disable ();
	AddTail (&vblwaiters, (Node *) &vw);
	nvblwaiters++;
	Enable (oldints);

	do {
		SuperWaitSignal (SIGF_ONESHOT);
	} while (vw.vw_Task);
}


/***************************************************************************
 * Routines called by the FIRQ to process deferred actions.
 */
static void
signalVBLwaiters (void)
{
	register VBLWaiter	*vw;

	while (vw = (VBLWaiter *) RemHead (&vblwaiters)) {
		SuperInternalSignal (vw->vw_Task, SIGF_ONESHOT);
		vw->vw_Task = NULL;
	}
	nvblwaiters = 0;
}

static void
markVDLsfree (void)
{
	register ItemNode	*n;

	while (n = (ItemNode *) RemHead (&vdlstofree))
		n->n_Next = n->n_Prev = NULL;

	nvdlstofree = 0;
}
