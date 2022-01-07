/*  :ts=8 bk=0
 *
 * overlay.c:	Routines to create, display, and manage Overlays.
 *		These routines are a stopgap measure until the mythical
 *		new sooper-dooper all good nifty Screen system is created.
 *
 * Afternote:	This is *extremely* stopgap stuff; an egregious	hack.
 *		As soon as the cable guys get this, I'm working on
 *		a new display engine.
 *
 * Leo L. Schwab					9404.27
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 * $Id: overlay.c,v 1.12 1994/12/21 20:28:48 ewhac Exp $
 *
 * $Log: overlay.c,v $
 * Revision 1.12  1994/12/21  20:28:48  ewhac
 * Altered autodoc.  I want the high bit in overlay pixels to be an alpha
 * channel.
 *
 * Revision 1.11  1994/11/17  20:59:10  vertex
 * Changed SIGF_SEMAPHORE to SIGF_ONESHOT. Also now makes sure
 * to clear the bit before waiting for it.
 *
 * Revision 1.10  1994/11/17  02:48:07  ewhac
 * Changed TakedownOverlay() to overload use of SIGF_SEMAPHORE rather
 * than allocating a transient one (and risking failure).  Martin Taillefer
 * assures me this is a safe usage of the signal bit.
 *
 * Revision 1.9  1994/10/24  05:20:45  ewhac
 * Fixed VDL tracking bug; Overlays now sit on top of custom VDLs in a more
 * correct way.
 *
 * Overlays now also observe changes via ControlVDL() and ModifyVDL().
 *
 * Revision 1.8  1994/09/10  02:36:54  vertex
 * Updated autodoc headers per Yvonne's request.
 *
 * Revision 1.7  1994/09/08  23:31:57  vertex
 * Corrected version number of release
 *
 * Revision 1.6  1994/09/08  23:28:03  vertex
 * Updated for new release numbering scheme
 *
 * Revision 1.5  1994/08/25  22:55:16  ewhac
 * Altered to deal with new VDL structure definitions.
 * Removed defaultcolors[] array to vdl.c.
 *
 * Revision 1.4  1994/06/10  01:57:26  ewhac
 * Updated autodocs.
 *
 * Revision 1.3  1994/06/10  01:51:40  ewhac
 * Added support for SetScreenColors().
 *
 * Revision 1.2  1994/05/31  22:40:01  ewhac
 * Removed compiler warnings (as if any of the warnings were useful).
 *
 * Revision 1.1  1994/05/27  01:10:07  ewhac
 * Initial revision
 *
 */
#include <types.h>
#include <mem.h>
#include <hardware.h>
#include <intgraf.h>
#include <string.h>

#include "overlay.h"


/***************************************************************************
 * #defines
 */
#define	UVF_COLORCHANGE		1


/***************************************************************************
 * Statics.
 */
/*
 * Table of display modulos based on VDL_DISPMOD value.
 * 0 == DISPMOD value is invalid.
 */
static uint16 linewidths[8] = {
	320,
	384,
	512,
	640,
	1024,
	0,
	0,
	0
};
#define	VDL_DISPMOD_SHIFT	23


extern uint32	defaultcolors[];

Overlay	*ActiveOverlay;	/*  If non-NULL, there's an overlay present.  */

/*int32	OVDBUG;*/


/***************************************************************************
 * Notes to myself.

	In order to splice a Bitmap into an existing VDL, it is necessary to
do two things:
	o Follow all VDL links up to the video line where we wish the new
	  Bitmap to appear;
	o Track all VDL changes thereafter up to the point where the
	  spliced-in Bitmap ends, so the overlaid display will appear as it
	  would have done had we not stomped over it.

	An overlay (and, more generally, a Screen (which I haven't invented
yet)) therefore has two component VDL pieces:  A "receiver", which is the
VDL instruction set that transfers control to the top of the Bitmap; and a
"handoff", which relinquishes control from the bottom of the Bitmap to
whatever was underneath.

*****************/


/***************************************************************************
 * Prototypes.
 */
/* overlay.c */
Item internalCreateOverlay(struct Overlay *ov, struct TagArg *args);
Err buildoverlayVDL(struct Overlay *ov);
int32 SpliceOverlay(struct Overlay *ov, int32 op, struct VDL *vdl0, struct VDL *vdl1);
void PostSpliceOverlay(struct Overlay *ov, int32 bank);
void PinupOverlay(struct Overlay *ov);
void TakedownOverlay(struct Overlay *ov);
uint32 updatevdl(struct FullVDL *target, struct FullVDL *src, int docopy);
void *advanceraster(void *startaddr, int32 linewidth, int32 nlines);
uint32 updatecontrol(uint32 orig, uint32 current);
struct FullVDL *nextVDL(struct FullVDL *vh);

void dumpvdl(struct FullVDL *fv, char *name);

extern void	applyVDLmods (struct VDL *, struct VDLHeader *, uint32, uint32);


/***************************************************************************
 * Client Interface.
 */
/**
|||	AUTODOC PUBLIC gpg/graphics/displayoverlay
|||	DisplayOverlay - Display a Bitmap overlaying all Screens.
|||
|||	  Synopsis
|||
|||	    Item DisplayOverlay (Item bitmap, int32 topedge);
|||
|||	  Description
|||
|||	    This routine creates and installs an Overlay into the prevailing
|||	    display.  This is used to present information of a critical
|||	    nature to the user.  The underlying Screen and application
|||	    driving it continue to operate normally.
|||
|||	    The specified Bitmap is spliced into the prevailing display; its
|||	    top edge will be at the location specified by 'topedge'.
|||	    DisplayOverlay() will return an Item representing the Overlay,
|||	    or an error if anything went wrong.  The specified Bitmap will
|||	    remain in front of all other Screens until the Overlay is
|||	    removed via DeleteItem().
|||
|||	    Only one Overlay may be active at a time; the most recent Bitmap
|||	    supplied to DisplayOverlay() is the one displayed.
|||
|||	    The top edge of the Bitmap must be >= 0, and the bottom edge
|||	    must be no lower than the total height of the prevailing display
|||	    mode (NTSC or PAL).  The Overlay will not be created if these
|||	    bounds are violated.
|||
|||	  Note
|||
|||	    It is strongly recommended that this facility be used only to
|||	    display information of a time-critical and transient nature to
|||	    the user.  Using this facility for the purposes of permanently
|||	    installing a static graphic (like a flight control panel) is
|||	    expressly discouraged.
|||
|||	    It is anticipated that, one day, maybe, possibly, overlays will
|||	    be translucent.  In anticipation of this eventuality, you are
|||	    encouraged to observe the following rendering rules:
|||
|||	    Imagery that is to be translucent should be rendered with the
|||	    high bit (bit 15) clear.  Imagery that is to be opaque (like
|||	    text) should be rendered with the high bit set.  (Think of it as
|||	    an alpha channel bit.)
|||
|||	  Arguments
|||
|||	    bitmap	Item number of the Bitmap to be displayed.
|||
|||	    topedge	Y-position where the top edge of the Bitmap is to
|||	    		appear.
|||
|||	  Result
|||
|||	    Item	An Item representing the Overlay, or a negative
|||	    		error code.
|||
|||	  Implementation
|||
|||	    Folio call implemented in graphics folio V22.
|||
|||	  Associated Files
|||
|||	    graphics.h		ANSI Prototype
|||
|||	    graphics.lib	ARM Link Library
|||
|||	  Caveats
|||
|||	    Currently, if the top edge is zero, it is silently bumped to
|||	    one.  This simplifies stuff internally.  This behavior will
|||	    change in the future.
|||
|||	    The internal nature of the Overlay shall forever remain private.
|||	    This is because its implementation is going to change in the
|||	    future.  If you attempt to rely on any characteristics of an
|||	    Overlay Item that aren't documented here, YOUR APPLICATION WILL
|||	    BREAK.
|||
|||	    The CLUT used for the Overlay is the standard ascending color
|||	    set; it cannot be changed.
|||
|||	    All sub-positioning and anti-aliasing features are turned OFF in
|||	    the Overlay region; they cannot be changed.
|||
|||	    Due to a hardware weirdness, the top and bottom lines
|||	    of the Overlay may appear unusual if vertical averaging is
|||	    enabled in the underlying Screen.  This is especially true of
|||	    pseudo-24-bit displays.
|||
|||	  See Also
|||
|||	    CreateItem(), CreateScreenGroup(), DeleteItem()
|||
**/
Item
DisplayOverlay (Item bitmap, int32 topedge)
{
	TagArg	args[3];

	args[0].ta_Tag = OVTAG_BITMAPITEM;
	args[0].ta_Arg = (void *) bitmap;

	args[1].ta_Tag = OVTAG_TOPEDGE;
	args[1].ta_Arg = (void *) topedge;

	args[2].ta_Tag = TAG_END;
	args[2].ta_Arg = NULL;

	return (CreateItem (MKNODEID (NODE_GRAPHICS, TYPE_OVERLAY), args));
}


/***************************************************************************
 * Item Management.
 */
static Err
ico (
struct Overlay	*ov,
void		*dummy,
uint32		t,
uint32		a
)
{
	switch (t) {
	case OVTAG_BITMAPITEM:
		ov->ov_bm = (Bitmap *) a;
		break;
	case OVTAG_TOPEDGE:
		ov->ov_YPos = (int32) a;
		break;
	default:
		return (GRAFERR_BADTAG);
	}
	return (0);
}


Item
internalCreateOverlay (ov, args)
struct Overlay	*ov;
struct TagArg	*args;
{
	FullVDL	*fv;
	Bitmap	*bm;
	Err	e;
	int32	i;

	if ((e = TagProcessor (ov, args, ico, NULL)) < 0)
		return (e);

	/*
	 * I'm not checking for task ownership, since I'm not actually
	 * modifying the Bitmap.  Others may differ with this philosophy.
	 */
	if ((bm = (Bitmap *) CheckItem ((Item) ov->ov_bm,
					NODE_GRAPHICS,
					TYPE_BITMAP)) == NULL)
		return (GRAFERR_BADITEM);

	/*
	 * Some may observe this to be wasteful, allocating an extra pair of
	 * VDL's for potential stereo display that may never get used.
	 * They're probably right.  Maybe on the next pass...
	 */
	if ((fv = (FullVDL *)
		  SUPER_ALLOCMEM (sizeof (FullVDL) * N_OVVDLS * 2,
				  MEMTYPE_VRAM | MEMTYPE_FILL)) == NULL)
		return (GRAFERR_NOMEM);

	/*
	 * Check to make sure the boundaries of the Overlay don't stray past
	 * the limits of the display.
	 */
	i = _DisplayHeight[GrafBase->gf_CurrentVDLEven->vdl_DisplayType];
	if (ov->ov_YPos < 0  ||  ov->ov_YPos + ov->ov_Height > i)
		return (GRAFERR_COORDRANGE);

	/*  Cheesoid hack to make things simpler for the moment.  */
	if (ov->ov_YPos == 0)
		ov->ov_YPos++;
	else if (ov->ov_YPos + ov->ov_Height == i)
		ov->ov_YPos--;

	/*
	 * Initialize fields in Overlay.
	 */
	for (i = N_OVVDLS;  --i >= 0; ) {
		ov->ov_VDL[i].ovv_Receive = fv + 2 * i;
		ov->ov_VDL[i].ovv_Handoff = fv + 2 * i + 1;
	}

	ov->ov_bm	= bm;	/*  Turn into pointer.  */
	ov->ov_XPos	= 0;
	ov->ov_Width	= bm->bm_Width;
	ov->ov_Height	= bm->bm_Height;
	ov->ov_Flags	= OVF_FREEBANK_0 | OVF_FREEBANK_1;

	if ((e = buildoverlayVDL (ov)) < 0) {
		SUPER_FREEMEM (fv, sizeof (FullVDL) * N_OVVDLS * 2);
		return (e);
	}

	/*
	 * Install overlay
	 */
	PinupOverlay (ov);

	return (ov->ov_Node.n_Item);
}

Item
internalOpenOverlay (ov, args)
struct Overlay	*ov;
TagArg		*args;
{
	return (ov->ov_Node.n_Item);
}

Err
internalCloseOverlay (ov, t)
struct Overlay	*ov;
struct Task	*t;
{
	return (0);
}

Err
internalDeleteOverlay (ov, t)
struct Overlay	*ov;
struct Task	*t;
{
	/* ###
	 * Insert check to remove from display if already installed.
	 */
	if (ov == ActiveOverlay)
		TakedownOverlay (ov);

	SUPER_FREEMEM
	 (ov->ov_VDL[0].ovv_Receive, sizeof (FullVDL) * N_OVVDLS * 2);

	return (0);
}


/***************************************************************************
 * Overlay creation, installation, and management.
 */
#define	VDL_DMACTL_DEFAULT	(VDL_ENVIDDMA | VDL_LDCUR | VDL_LDPREV)
#define	VDL_HANDOFF_LENGTH	VDL_LEN_FULL_FMT


Err
buildoverlayVDL (ov)
struct Overlay	*ov;
{
	ValidWidth	*vw;
	FullVDL		*fv;
	int32		i;

	if ((vw = FindValidWidth (ov->ov_Width, BMF_DISPLAYABLE)) == NULL)
		return (GRAFERR_BUFWIDTH);

	for (i = N_OVVDLS;  --i >= 0; ) {
		fv = ov->ov_VDL[i].ovv_Receive;

		fv->fv.DMACtl	= VDL_DMACTL_DEFAULT |
				  VDL_HANDOFF_LENGTH |
				  vw->vw_DispMods |
				  (ov->ov_Height & VDL_LINE_MASK);
		fv->fv.CurBuf	= ov->ov_bm->bm_Buffer;
		fv->fv.PrevBuf	= ov->ov_bm->bm_Buffer;
		fv->fv.NextVDL	= NULL;

		fv->fv_DispCtl	= VDL_DISPCTRL | VDL_CLUTBYPASSEN |
				  VDL_WINREPEN | /* VDL_ONEVINTDIS | */
				  VDL_VSUB_ONE | VDL_WINVSUB_ONE |
				  VDL_HSUB_ONE | VDL_WINHSUB_ONE |
				  VDL_BLSB_BLUE | VDL_WINBLSB_BLUE;

		memcpy (fv->fv_Colors, defaultcolors, sizeof (fv->fv_Colors));
	}

	return (0);
}


/***************************************************************************
 * This is the Big Deal.
 *
 * This routine is very messy; please accept apologies.  If and when my
 * super good nifty Screen system comes to fruition, this operation will
 * be part of that system (which will, of course, be beautiful).
 *
 * Non-obvious considerations:
 *
 * When a new screen is displayed, we need to undo the changes we made to
 * the previous screen.  This needs to be done after the altered VDL's have
 * been installed (else we risk a flash of junk on the screen).  More
 * generally, all VDL modification needs to be done before they can be
 * installed.
 *
 * This entails some ugly weirdness with resplicing an already-spliced VDL.
 * We have to scan it as if it weren't spliced, but we can't actually
 * unsplice it, since doing so can hose the display.
 * (It turns out this case appears to be insoluble without consuming
 * potentially large amounts of RAM.  Therefore, I'm going to assume that,
 * if an already-spliced VDL comes in, it's been spliced in the needed way
 * and doesn't need to be respliced.  This shortcut will prevent subsequent
 * changes to ov_YTop from working correctly in this case.  Since there will
 * be no facility for modifying an existing Overlay, this shouldn't be a
 * problem. (And of course, all this goes away in the future system.))
 */
int32
SpliceOverlay (
struct Overlay	*ov,
int32		op,
struct VDL	*vdl0,
struct VDL	*vdl1
)
{
	register int	i, f;
	OverlayVDL	*ovv;
	FullVDL		*cur, *scan, *rcv, *ho;
	VDL		*vdl;
	int32		nlines, holines, curline, nextline, choplength;
	int32		width;
	int32		bank;
	uint32		dmac, ovflags;
	int		fields;


	if (ov->ov_Flags & OVF_DEATHLOCK)
		/*
		 * We're in the process of deleting this item.
		 */
		return (-1);

	if (vdl0 == vdl1)	fields = 1;
	else			fields = 2;

	bank = 0;
retry:	if ((i = (int) ov->ov_Flags) & OVF_FREEBANKMASK) {
		if (i & OVF_FREEBANK_0)
			bank = 0;
		else
			bank = 2;
		ov->ov_Flags = i;
	} else {
		/*
		 * Whoops!  No free banks; something obviously pending.
		 * Let's see if we can un-pending it.
		 ****
		 * One possible visual side-effect of this approach is a
		 * one-field blink where the overlay doesn't appear.  Short
		 * of going to a triple-buffered system (which is ridiculous
		 * in this environment), this is unavoidable.  It's only
		 * possible in a pathological case (DisplayScreen() called
		 * twice in one field), so it shouldn't be a problem.
		 */
		uint32	oldints;

		if (bank < 0)
			/*
			 * Nothing pending?  Nothing free?
			 * Well I give up...
			 */
			return (bank);

		oldints = Disable ();
		i = (int) ov->ov_Flags;
		if (i & OVF_PENDINGBANKMASK) {
			if (i & OVF_PENDINGBANK_0) {
				bank = 0;
				i &= ~OVF_PENDINGBANK_0;
			} else {
				bank = 2;
				i &= ~OVF_PENDINGBANK_1;
			}
			ov->ov_Flags = i;
		} else
			bank = -1;

		Enable (oldints);

		if (bank < 0)
			/*
			 * YOUCH!  No pending banks, either.  The interrupt
			 * must have slipped in while we weren't looking.
			 * Try again.
			 */
			goto retry;	/*  Look up.  */
	}

	/*
	 * Frolick through the fields, patching VDL's.
	 */
	scan = NULL;
	for (f = fields;  --f >= 0; ) {
		if (f)	vdl = vdl1;
		else	vdl = vdl0;

		if (vdl == ov->ov_VDL[0].ovv_Spliced  ||
		    vdl == ov->ov_VDL[1].ovv_Spliced  ||
		    vdl == ov->ov_VDL[2].ovv_Spliced  ||
		    vdl == ov->ov_VDL[3].ovv_Spliced)
			/*
			 * This screen's VDL has already been spliced by us.
			 * Assume no more work needed.
			 */
			continue;
#if 0
/*  Debugging assist removed 9406.08  */
if (vdl->vdl_Flags & VDLF_SPLICED)
 Superkprintf ("What the...?\n");
#endif

		scan = (FullVDL *) vdl->vdl_DataPtr;

		/*
		 * Initialize handoff VDL.
		 */
		ovv = &ov->ov_VDL[f + bank];
		ho = ovv->ovv_Handoff;
		ho->fv_DispCtl = VDL_NOP;
		for (i = 33;  --i >= 0; )
			ho->fv_Colors[i] = VDL_NOP;

		updatevdl (ho, scan, TRUE);	/*  Copy first VDL  */

		/*
		 * Scan through VDL list until we encounter the one that
		 * crosses the top edge of the overlay.
		 */
		ovflags = nextline = 0;
		while (1) {
			curline = nextline;
			nlines = (scan->fv.DMACtl & VDL_LINE_MASK) >>
				 VDL_LINE_SHIFT;
			nextline += nlines;
			if (nextline >= ov->ov_YPos)
				break;

			scan = nextVDL (scan);
			ovflags |= OVF_VDLCHANGE |
				   updatevdl (ho, scan, FALSE);
		}
		cur = scan;

		/*
		 * Record who we're stepping on for later restoral.
		 */
		ovv->ovv_Stomped	= cur;
		ovv->ovv_StompNext	= (FullVDL *) cur->fv.NextVDL;
		ovv->ovv_StompDMAC	= cur->fv.DMACtl;
		ovv->ovv_Spliced	= vdl;
		vdl->vdl_Flags |= VDLF_SPLICED;
		choplength = ov->ov_YPos - curline;

		/*
		 * 'cur' now points to the VDL we will be stomping on.
		 * Now continue to track VDL changes until the handoff point.
		 */
		while (1) {
			if (nextline >= ov->ov_YPos + ov->ov_Height)
				/*
				 * We're past the bottom; handoff completed.
				 */
				break;

			scan = nextVDL (scan);
			ovflags |= OVF_VDLCHANGE |
				   updatevdl (ho, scan, FALSE);
			curline = nextline;
			nlines = (scan->fv.DMACtl & VDL_LINE_MASK) >>
				 VDL_LINE_SHIFT;
			nextline += nlines;
		}
		holines = nextline - (ov->ov_YPos + ov->ov_Height);
		ho->fv.DMACtl = (ho->fv.DMACtl & ~VDL_LINE_MASK) | holines;

/*		ho->fv_DispCtl |= VDL_ONEVINTDIS;	*/

		width = linewidths[(scan->fv.DMACtl & VDL_DISPMOD_MASK) >>
				   VDL_DISPMOD_SHIFT];
		nlines = ov->ov_YPos + ov->ov_Height - curline;
		if (ho->fv.DMACtl & VDL_480RES)
			nlines += nlines;
		ho->fv.CurBuf =
		 advanceraster (scan->fv.CurBuf, width, nlines);
		ho->fv.PrevBuf =
		 advanceraster (scan->fv.PrevBuf, width, nlines);

		ov->ov_Flags |= ovflags;

		/*
		 * Handoff VDL compiled.  Now start prancing on things.
		 */
		rcv = ovv->ovv_Receive;

		dmac = (cur->fv.DMACtl & ~VDL_LINE_MASK) | choplength;

		if (holines) {
			rcv->fv.NextVDL = (VDLHeader *) ho;
			ho->fv.NextVDL = scan->fv.NextVDL;
		} else
			/*
			 * Degenerate handoff; patch directly to next VDL.
			 */
			rcv->fv.NextVDL = scan->fv.NextVDL;

		/*
		 * Patch over existing VDL.  Hopefully the hardware isn't
		 * currently trying to display this.
		 */
		if (op == SPLICEOP_NEW) {
			cur->fv.DMACtl = dmac;
			cur->fv.NextVDL = (VDLHeader *) rcv;
		} else
			/*
			 * Okay, the hardware *is* trying to display this.
			 * Tell the poor overworked graphics FIRQ what to
			 * stuff into the VDL we desire to stomp.
			 */
			ovv->ovv_StompDMAC = dmac;
#if 0
/* Installed for early debugging; no longer used as of 9405.26. */
if (OVDBUG) {
	dumpvdl (cur, "cur");
	dumpvdl (rcv, "receive");
	dumpvdl (ho, "handoff");
	OVDBUG = 0;
}
#endif

	}

	/*
	 * Return the state information required by PostSpliceOverlay().
	 */
	ov->ov_Flags |= OVF_INUSE;
	if (scan) {
		i = (int) ov->ov_Flags;
		if (bank)	i &= ~OVF_FREEBANK_1;
		else		i &= ~OVF_FREEBANK_0;
		ov->ov_Flags = i;

		if (op == SPLICEOP_EXISTING) {
			if (bank)	i |= OVF_INSTALLBANK_1;
			else		i |= OVF_INSTALLBANK_0;
			ov->ov_Flags = i;

			return (-1);
		} else
			return (bank);
	} else
		return (-1);

}


void
PostSpliceOverlay (ov, bank)
struct Overlay		*ov;
int32			bank;
{
	if (bank >= 0) {
		/*
		 * Mark the bank as pending for display.  The graphics FIRQ
		 * will then swizzle the appropriate bits to mark the other
		 * bank as free and unsplice the original VDL.
		 * This operation must be protected from interrupts.
		 */
		register uint32	i;

		i = ov->ov_Flags;
		if (bank)	i |= OVF_PENDINGBANK_1;
		else		i |= OVF_PENDINGBANK_0;

		ov->ov_Flags = i;
	}
}



void
PinupOverlay (ov)
struct Overlay	*ov;
{
	if (ov != ActiveOverlay) {
		if (ActiveOverlay)
			TakedownOverlay (ActiveOverlay);

		if (GrafBase->gf_CurrentVDLEven != GrafBase->gf_BlankVDL)
			SpliceOverlay (ov,
				       SPLICEOP_EXISTING,
				       GrafBase->gf_CurrentVDLEven,
				       GrafBase->gf_CurrentVDLOdd);

		ActiveOverlay = ov;
	}
}



void
TakedownOverlay (ov)
register struct Overlay	*ov;
{
	register FullVDL	*fv;
	register OverlayVDL	*ovv;
	register uint32		oldints;
	register int		i;
	int32			sigf;

	if (!(ov->ov_Flags & OVF_INUSE))
		/*
		 * Not in use; don't bother.
		 */
		return;

	/*
	 * Prevent any possible further patching.
	 */
	ov->ov_Flags |= OVF_DEATHLOCK;

	if (ov == ActiveOverlay) {
		/*
		 * Wait for any pending bank installation to take place.
		 * Overload SIGF_ONESHOT to tell us when VBlank has
		 * happened (Martin assures me this is safe).
		 */
		ClearSignals(CURRENTTASK,SIGF_ONESHOT);
		ov->ov_SigMask = sigf = SIGF_ONESHOT;	/*SuperAllocSignal(0)*/

		while (ov->ov_Flags & OVF_PENDINGBANKMASK) {
			ov->ov_SigTask = CURRENTTASK;
			SuperWaitSignal (sigf);    /*  Weird WaitVBL()  */
		}

		/*
		 * Determine which bank is currently in use and force it out.
		 */
		oldints = Disable ();

		i = (int) ov->ov_Flags;
		i &= ~OVF_PENDINGBANKMASK;
		if (i & OVF_FREEBANK_0)
			i |= OVF_PENDINGBANK_0;
		else
			i |= OVF_PENDINGBANK_1;
		ov->ov_Flags = i;

		Enable (oldints);

		/*
		 * Wait for graphics FIRQ to detach the old list.
		 */
		ov->ov_SigTask = CURRENTTASK;
		SuperWaitSignal (sigf);		/*  Weird WaitVBL()	*/

/*		SuperFreeSignal (sigf);		*/
		ov->ov_SigMask = 0;
		ActiveOverlay = NULL;
	}

	/*
	 * Make certain all banks are unspliced and marked as unused.
	 * (This is probably redundant.)
	 */
	ovv = ov->ov_VDL;
	for (i = N_OVVDLS;  --i >= 0;  ovv++) {
		if ((fv = ovv->ovv_Stomped) != NULL) {
			fv->fv.NextVDL = (VDLHeader *) ovv->ovv_StompNext;
			fv->fv.DMACtl = ovv->ovv_StompDMAC;
			ovv->ovv_Stomped = NULL;
		}
		ovv->ovv_Spliced = NULL;
	}
	ov->ov_Flags = OVF_FREEBANK_0 | OVF_FREEBANK_1;
}


void
setundercolor (ov, vdl)
register struct Overlay	*ov;
register struct VDL	*vdl;
{
	register int	i;

	if (!(ov->ov_Flags & OVF_VDLCHANGE)) {
		for (i = N_OVVDLS;  --i >= 0; ) {
			if (vdl == ov->ov_VDL[i].ovv_Spliced)
			{
/*- - - - - - - - - - -*/
/*
 * I'm making a $#!+load of assumptions here.
 */
memcpy (ov->ov_VDL[i].ovv_Handoff->fv_Colors,
	((FullVDL *) vdl->vdl_DataPtr)->fv_Colors,
	sizeof (uint32) * 33);
/*- - - - - - - - - - -*/
			}
		}
	}
}

void
controlundervdl (ov, vdl, setflags, clearflags)
struct Overlay		*ov;
register struct VDL	*vdl;
uint32			setflags, clearflags;
{
	register int	i;

	if (!(ov->ov_Flags & OVF_VDLCHANGE)) {
		uint32	dctl;

		for (i = N_OVVDLS;  --i >= 0; ) {
			if (vdl == ov->ov_VDL[i].ovv_Spliced) {
				dctl = ov->ov_VDL[i].ovv_Handoff->fv_DispCtl;
				dctl |= setflags;
				dctl &= ~clearflags;
				ov->ov_VDL[i].ovv_Handoff->fv_DispCtl = dctl;
			}
		}
	}
}


void
modifyundervdl (ov, vdl, setflags, clearflags)
struct Overlay		*ov;
register struct VDL	*vdl;
uint32			setflags, clearflags;
{
	register int	i;
	uint32		dctl;

	for (i = N_OVVDLS;  --i >= 0; ) {
		if (vdl == ov->ov_VDL[i].ovv_Spliced) {
			dctl = ov->ov_VDL[i].ovv_Handoff->fv_DispCtl;
			dctl |= setflags;
			dctl &= ~clearflags;
			ov->ov_VDL[i].ovv_Handoff->fv_DispCtl = dctl;

			applyVDLmods (vdl,
				      &ov->ov_VDL[i].ovv_StompNext->fv,
				      setflags,
				      clearflags);
		}
	}
}



#define	VDL_DMAJAM_MASK	(~(VDL_RELSEL | VDL_LDCUR | VDL_LDPREV | VDL_LEN_MASK))

uint32
updatevdl (target, src, docopy)
struct FullVDL	*target, *src;
int		docopy;
{
	register int32	i;
	register uint32	n;
	register uint32	*vp;
	uint32		retflags;

	retflags = 0;

	/*
	 * Update DMA control word.  Also update next vdl pointer, and
	 * current and previous buffer pointers.
	 */
	i = src->fv.DMACtl;
	if (docopy) {
		/*
		 * Do straight copy.
		 */
		memcpy (target, src, sizeof (VDLHeader));
		target->fv.NextVDL = (VDLHeader *) nextVDL (src);
		target->fv.DMACtl &= ~VDL_RELSEL;
	} else {
		/*
		 * Perform update operation.
		 */
		n = (i & VDL_DMAJAM_MASK) | VDL_LEN_FULL_FMT;

		target->fv.NextVDL = (VDLHeader *) nextVDL (src);

		if (i & VDL_LDCUR) {
			target->fv.CurBuf = src->fv.CurBuf;
			n |= VDL_LDCUR;
		}

		if (i & VDL_LDPREV) {
			target->fv.PrevBuf = src->fv.PrevBuf;
			n |= VDL_LDPREV;
		}

		target->fv.DMACtl = n;
	}

	/*
	 * Scan remaining VDL words.
	 */
	vp = &src->fv_DispCtl;
	i = (i & VDL_LEN_MASK) >> VDL_LEN_SHIFT;
	while (--i >= 0) {
		n = *vp++;

		if (n & VDL_CONTROL) {
			if (n == VDL_NOP)
				continue;
			if ((n & VDL_DISPCTRL) == VDL_DISPCTRL) {
				if (n & VDL_BACKGROUND) {
					/*
					 * Background color.
					 */
					target->fv_Colors[32] = n;
					retflags |= OVF_COLORCHANGE;
				} else
					/*
					 * Normal control word.
					 */
					if (docopy)
						target->fv_DispCtl = n;
					else
						target->fv_DispCtl =
						 updatecontrol
						  (target->fv_DispCtl, n);
			} else
				/*
				 * It's an AMY word.  I know not what to do
				 * with these.
				 */
				continue;
		} else {
			/*
			 * Wow!  Colors!
			 */
			register uint32	reg;

			reg = (n & VDL_PEN_MASK) >> VDL_PEN_SHIFT;
			target->fv_Colors[reg] = n;
			retflags |= OVF_COLORCHANGE;
		}
	}
	return (retflags);
}











void *
advanceraster (startaddr, linewidth, nlines)
void	*startaddr;
int32	linewidth, nlines;
{
	register uint32	addr;

	addr = (uint32) startaddr;
	addr += (nlines >> 1) * linewidth * sizeof (uint32);
	if (nlines & 1) {
		if ((uint32) startaddr & 2)
			addr += linewidth * sizeof (uint32) - sizeof (uint16);
		else
			addr += sizeof (uint16);
	}
	return ((void *) addr);
}


#define	VDL_CTLJAM_MASK	(~(VDL_WINVSUB_MASK | VDL_WINHSUB_MASK | \
			   VDL_WINBLSB_MASK | VDL_VSUB_MASK | \
			   VDL_HSUB_MASK | VDL_BLSB_MASK | VDL_COLORSONLY))

uint32
updatecontrol (orig, current)
uint32	orig, current;
{
	uint32	newbits;

	/*
	 * Stuff new control values that are independent of previous
	 * settings.
	 */
	newbits = current & VDL_CTLJAM_MASK;

	/*
	 * Update control values that depend on the value of previous
	 * control words.
	 */
	if ((current & VDL_WINVSUB_MASK) == VDL_WINVSUB_NOP)
		newbits |= orig & VDL_WINVSUB_MASK;
	else
		newbits |= current & VDL_WINVSUB_MASK;

	if ((current & VDL_WINHSUB_MASK) == VDL_WINHSUB_NOP)
		newbits |= orig & VDL_WINHSUB_MASK;
	else
		newbits |= current & VDL_WINHSUB_MASK;

	if ((current & VDL_WINBLSB_MASK) == VDL_WINBLSB_NOP)
		newbits |= orig & VDL_WINBLSB_MASK;
	else
		newbits |= current & VDL_WINBLSB_MASK;

	if ((current & VDL_VSUB_MASK) == VDL_VSUB_NOP)
		newbits |= orig & VDL_VSUB_MASK;
	else
		newbits |= current & VDL_VSUB_MASK;

	if ((current & VDL_HSUB_MASK) == VDL_HSUB_NOP)
		newbits |= orig & VDL_HSUB_MASK;
	else
		newbits |= current & VDL_HSUB_MASK;

	if ((current & VDL_BLSB_MASK) == VDL_BLSB_NOP)
		newbits |= orig & VDL_BLSB_MASK;
	else
		newbits |= current & VDL_BLSB_MASK;

	return (newbits);
}


struct FullVDL *
nextVDL (vh)
struct FullVDL *vh;
{
	if (vh->fv.DMACtl & VDL_RELSEL)
		return ((FullVDL *)
			((uint8 *) vh + (int32) vh->fv.NextVDL + 16));
	else
		return ((FullVDL *) vh->fv.NextVDL);
}


#if 0
/* No longer used as of 9405.26 */
void
dumpvdl (struct FullVDL *fv, char *name)
{
	register int	i;

	Superkprintf ("&%s: 0x%08lx\n DMACtl  = 0x%08lx\n", name, fv, fv->fv.DMACtl);

	Superkprintf (" CurBuf  = 0x%08lx\n PrevBuf = 0x%08lx\n NextVDL = 0x%08lx\n",
		      fv->fv.CurBuf, fv->fv.PrevBuf, fv->fv.NextVDL);
	Superkprintf (" DispCtl = 0x%08lx\n Colors[0...32]:\n", fv->fv_DispCtl);

	for (i = 0;  i <= 32;  i++)
		Superkprintf ("  0x%08lx\n", fv->fv_Colors[i]);

	Superkprintf ("\n");
}
#endif
