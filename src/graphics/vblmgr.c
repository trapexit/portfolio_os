/*  :ts=8 bk=0
 *
 * vblmgr.c:	Vertical Blank Manager (idea mongering at the moment).
 *
 * Leo L. Schwab					9406.30
 ****
 * Salutations, Imperfect One!  I am The Master, and you are mysteriously
 * drawn to me!
 ****
 * Notes to myself:
 *
 * 9407.03: Should think about one set of VDLs for each field.
 *
 * 9407.05: New philosophy (wish I'd thought of it earlier):  The VBL
 *	manager is responsible only for the portion of the VBL from forced
 *	CLUT transfer to first line of displayed video.  After that, it's
 *	Somebody Else's Problem (thereby making it invisible).  This may seem
 *	obvious, but I was trying to handle the blank screen and the
 *	terminator here, too.  Ack.
 *
 ***************************************************************************
 * Copyright 1994 The 3DO Company.  All Rights Reserved.
 *
 * 3DO Trade Secrets  -  Confidential and Proprietary
 ***************************************************************************
 *			     --== RCS Log ==--
 *
 * $Id: vblmgr.c,v 1.6 1994/10/24 05:25:13 ewhac Exp $
 *
 * $Log: vblmgr.c,v $
 * Revision 1.6  1994/10/24  05:25:13  ewhac
 * Added code to deal with late forced VDL transfers under PAL-wide
 * displays.  Probably needs more work...
 *
 * Removed some dead code.
 *
 * Revision 1.5  1994/09/10  02:36:54  vertex
 * Updated autodoc headers per Yvonne's request.
 *
 * Revision 1.4  1994/09/08  23:28:03  vertex
 * Updated for new release numbering scheme
 *
 * Revision 1.3  1994/08/29  17:35:59  sdas
 * Chnaged comment (MUD is now SysInfo)
 *
 * Revision 1.2  1994/08/26  21:49:46  ewhac
 * Fixed synopsis line in autodocs.
 *
 * Revision 1.1  1994/08/25  23:07:53  ewhac
 * Initial revision
 *
 */
#include <types.h>
#include <mem.h>
#include <string.h>

#include "intgraf.h"

#include "vdl.h"
#include "vbl.h"


/***************************************************************************
 * #defines and constants
 */
#define	SETLINES(dma,lines)	(((dma) & ~VDL_LINE_MASK) | (lines))

#define	SPORTSLEW_PALWIDE	3


/***************************************************************************
 * Structure definitions.
 */
typedef struct VBLState {
	ItemNode	vs;		/*  So TagProcessor() will work	*/
	int32		vs_VBLLen;	/*  Total length of VBlank	*/
	int32		vs_ForceLine;	/*  Line # where force CLUT happens*/
	int32		vs_SlipLine;	/*  Line # to enable SlipStream	*/
	int32		vs_VIRSLine;	/*  Line where VIRS is displayed  */
	int32		vs_DispLine;	/*  Line where active display starts*/
	int32		vs_SPORTLims[2];	/*  First line, last line */
	VDLHeader	*vs_ForceFirst;	/*  Addr of force CLUT transfer	*/
	VDLHeader	**vs_PatchPoint;/*  Last VDLHeader in VBL chain	*/
	VDLHeader	*vs_FirstSlip;	/*  First VDL enabling Slipstream */
	uint8		vs_DoVIRS,	/*  Is VIRS on?			*/
			vs_DoSlip;	/*  Is SlipStream on?		*/
} VBLState;

typedef struct SlipSwitch {
	uint32		ss_DMAMask,	/*  Bits to change		*/
			ss_DMAOn,	/*  Bits to turn SlipStream ON	*/
			ss_DMAOff;	/*  Bits to turn SlipStream OFF	*/
	uint32		ss_DispMask,
			ss_DispOn,
			ss_DispOff;
} SlipSwitch;


/***************************************************************************
 * Global data.
 */
/*
 * These all kind of go together.
 */
enum VBLPieces {
	VBLP_FORCE = 0,
	VBLP_ENSLIP,
	VBLP_DISPVIRS,
	VBLP_KILLVIRS,
	MAX_VBLP
};

static ShortVDL		*vblvdl[2][MAX_VBLP];

#define	VDLSIZE		(sizeof (ShortVDL) * MAX_VBLP * 2)


static uint32	VIRScolors[] = {
	0x00000000,	/*  0,   0,   0		*/
	0x01A0B539,	/*  160, 181, 57	*/
	0x026D6D6D	/*  109, 109, 109	*/
};

static TagArg	defaulttags[] = {
	VBL_TAG_LENGTH,		(void *) 16,
	VBL_TAG_FORCELINE,	(void *) 6,
	VBL_TAG_SLIPLINE,	(void *) 18,
	VBL_TAG_SLIPSTREAM,	(void *) FALSE,
	VBL_TAG_VIRS,		(void *) TRUE,
	TAG_END,		0
};

static SlipSwitch	vblswitches[] = {
 {	/*  Forceds first  */
	VDL_ENVIDDMA,
	VDL_ENVIDDMA,
	0,
	VDL_SLPDCEL | VDL_FORCETRANS,
	VDL_SLPDCEL | VDL_FORCETRANS,
	0
 }, {	/*  Slip On (theoretically never touched)  */
	VDL_ENVIDDMA,
	VDL_ENVIDDMA,
	0,
	VDL_SLPDCEL | VDL_FORCETRANS,
	VDL_SLPDCEL | VDL_FORCETRANS,
	0
 }, {	/*  VIRS  */
	0, 0, 0,
	VDL_SLPDCEL | VDL_BACKTRANS,
	VDL_SLPDCEL | VDL_BACKTRANS,
	0
 }, {	/*  Kill VIRS  */
	VDL_ENVIDDMA,
	VDL_ENVIDDMA,
	0,
	VDL_SLPDCEL | VDL_FORCETRANS,
	VDL_SLPDCEL | VDL_FORCETRANS,
	0
 }
};

static SlipSwitch	preswitch = {
	VDL_LDCUR | VDL_LDPREV | VDL_ENVIDDMA,
	VDL_ENVIDDMA,
	0,
	VDL_SLPDCEL | VDL_FORCETRANS,
	VDL_SLPDCEL | VDL_FORCETRANS,
	0
};

static SlipSwitch	blankswitch = {
	VDL_LDCUR | VDL_LDPREV | VDL_ENVIDDMA,
	VDL_LDCUR | VDL_LDPREV | VDL_ENVIDDMA,
	0,
	VDL_SLPDCEL | VDL_FORCETRANS,
	VDL_SLPDCEL | VDL_FORCETRANS,
	0
};


static VBLState	vblstate;

uint8		phase;


/* vblmgr.c */
Err SetVBLAttrs(TagArg *args);
Err GetVBLAttrs(TagArg *args);
Err createVBL(void);
void assembleVBL(void);
void initshortVDL(struct ShortVDL *sv, void *bufmem);
MemHdr *findVRAMbase(void);
void PALpatch(uint32 newflag);
void slippatch (struct ShortVDL *sv, struct SlipSwitch *ss, int state);



/***************************************************************************
 * Client interface.
 */
/**
|||	AUTODOC PRIVATE gpg/graphics/setvblattrs
|||	SetVBLAttrs - Configure vertical blank VDL attributes.
|||
|||	  Synopsis
|||
|||	    Err SetVBLAttrs (struct TagArg *args);
|||
|||	  Description
|||
|||	    Reconfigures the behavior of the vertical blanking interval.
|||	    This is done through a series of Tags.  They are:
|||
|||	    VBL_TAG_LENGTH
|||	    	Argtype: int32
|||
|||	    	The total length of the vertical blanking interval, in video
|||	    	lines.  This is defined as the number of whole lines between
|||	    	forced CLUT transfer and the first line on which imagery is
|||	    	to appear.
|||
|||	    VBL_TAG_FORCELINE
|||	    	Argtype: int32
|||
|||	    	The video line number where forced CLUT transfer occurs.
|||
|||	    VBL_TAG_SLIPLINE
|||	    	Argtype: int32
|||
|||	    	The video line number on which SlipStream is to be enabled.
|||	    	This must be after VBL_TAG_FORCELINE and before the first
|||	    	line of imagery.
|||
|||	    VBL_TAG_SLIPSTREAM
|||	    	Argtype: boolean (int32)
|||
|||	    	Turns SlipStream on/off within the vertical blanking
|||	    	interval.
|||
|||	    VBL_TAG_VIRS
|||	    	Argtype: boolean (int32)
|||
|||	    	Turns internally-generated VIRS on/off.
|||
|||	    The initial startup configuration of the vertical blanking
|||	    interval is as follows (under no circumstances should you assume
|||	    it is in this state):
|||
|||		TagArg defaults[] = {
|||			VBL_TAG_LENGTH,		(void *) 16,
|||			VBL_TAG_FORCELINE,	(void *) 6,
|||			VBL_TAG_SLIPLINE,	(void *) 18,
|||			VBL_TAG_SLIPSTREAM,	(void *) FALSE,
|||			VBL_TAG_VIRS,		(void *) TRUE,
|||			TAG_END,		0
|||		};
|||
|||	  Arguments
|||
|||	    args	Pointer to TagArg array.
|||
|||	  Result
|||
|||	    Err		An Opera error code if anything went awry, or
|||	    		zero if normal.
|||
|||	  Implementation
|||
|||	    Supervisor-mode folio call implemented in graphics folio V24.
|||
|||	  Associated Files
|||
|||	    vbl.h		ANSI Prototype, TagArg defines
|||
|||	    graphics.lib	ARM Link Library
|||
|||	  Notes
|||
|||	    VIRS always happens at line 19.  You must adjust VBL_TAG_LENGTH
|||	    and VBL_TAG_FORCELINE such that VIRS is encompassed by the
|||	    vertical blanking region.
|||
|||	    You should always interrogate the current state of the vertical
|||	    blanking region before attempting to modify it; see
|||	    GetVBLAttrs().  Once this facility starts listening to SysInfo,
|||	    the default startup conditions will likely become dynamic, so
|||	    hard-coded assumptions will likely kill you.
|||
|||	  Bugs
|||
|||	    Almost certainly.
|||
|||	    9407.12: Currently, VBL_TAG_SLIPLINE *must* be at line 18 or
|||	    before (so it won't collide with VIRS).  This must be true
|||	    whether VIRS is on or not.
|||
|||	  See Also
|||
|||	    GetVBLAttrs()
|||
**/
static Err
sva (
struct VBLState	*vs,
uint32		*flags,
uint32		t,
uint32		a
)
{
	switch (t) {
	case VBL_TAG_LENGTH:
		vs->vs_VBLLen = (int32) a - 1;
		break;

	case VBL_TAG_FORCELINE:
		vs->vs_ForceLine = (int32) a;
		break;

	case VBL_TAG_SLIPLINE:
		vs->vs_SlipLine = (int32) a;
		break;

	case VBL_TAG_VIRS:
		vs->vs_DoVIRS = (a != 0);
		break;

	case VBL_TAG_SLIPSTREAM:
		vs->vs_DoSlip = (a != 0);
		break;

	default:
		return (GRAFERR_BADTAG);
	}
	return (0);
}

Err
SetVBLAttrs (args)
TagArg	*args;
{
	struct VBLState	vs;
	ShortVDL	*sv;
	Err		e;
	int32		n;
	uint32		oldints;

	/*
	 * Make copy of current state for client to modify.
	 */
	memcpy (&vs, &vblstate, sizeof (vs));

	if ((e = TagProcessor (&vs, args, sva, NULL)) < 0)
		return (e);

	/*
	 * Recalculate state and sanity-check.
	 */
	vs.vs_DispLine = vs.vs_ForceLine + vs.vs_VBLLen;

	/*  Convert relative offset to absolute line number.  */
	/*  OK, I lied; we're not doing relative SLIP offsets...  */
/*	vs.vs_SlipLine = vs.vs_DispLine - vs.vs_SlipLine;*/

	if (vs.vs_SlipLine < vs.vs_ForceLine  ||
	    vs.vs_VIRSLine < vs.vs_ForceLine)
		/*  Can't enable features before the beginning of time.  */
		return (GRAFERR_BADCLIP);

	if (vs.vs_SlipLine > vs.vs_DispLine)
		/*  Can't turn SlipStream on after display has started.  */
		return (GRAFERR_INDEXRANGE);

	if (vs.vs_VIRSLine >= vs.vs_DispLine)
		/*  Display starts too early; don't do VIRS.  */
		vs.vs_DoVIRS = FALSE;

	/*
	 * Calculate SPORT limits.
	 */
	n = vs.vs_DispLine;
	if (vs.vs_DoSlip  &&  vs.vs_SlipLine < n)
		n = vs.vs_SlipLine;
	if (vs.vs_DoVIRS  &&  vs.vs_VIRSLine < n)
		n = vs.vs_VIRSLine;

	/*
	 * This part *must* be atomic.
	 */
	oldints = Disable ();

	vs.vs_SPORTLims[0] = vs.vs_ForceLine + 1;
	vs.vs_SPORTLims[1] = n - 1 - 1;	/*  Extra fencepost for early VDL  */
					/*  load on the "alternate" field  */

	if (sv = (ShortVDL *) GrafBase->gf_VDLPostDisplay)
		if (sv->sv_DispCtl & VDL_PALSEL) {
			/*  $*#^@!! hardware guys...  :-)  */
			vs.vs_SPORTLims[0] += SPORTSLEW_PALWIDE;
			vs.vs_SPORTLims[1] += SPORTSLEW_PALWIDE;
		}

	Enable (oldints);

	/*
	 * Off we go...  (Might think about flipping phase here...)
	 */
	memcpy (&vblstate, &vs, sizeof (vs));
	assembleVBL ();

	return (0);
}


/**
|||	AUTODOC PRIVATE gpg/graphics/getvblattrs
|||	GetVBLAttrs - Interrogate current configuration of vertical blank VDL.
|||
|||	  Synopsis
|||
|||	    Err GetVBLAttrs (struct TagArg *args);
|||
|||	  Description
|||
|||	    Interrogates the current configuration of the vertical blanking
|||	    interval.  This is done via TagArgs, which are:
|||
|||	    VBL_TAG_LENGTH
|||	    	Argtype: int32*
|||
|||	    	The total length of the vertical blanking interval, in video
|||	    	lines.  This is defined as the number of whole lines between
|||	    	forced CLUT transfer and the first line on which imagery is
|||	    	to appear.
|||
|||	    VBL_TAG_FORCELINE
|||	    	Argtype: int32*
|||
|||	    	The video line number where forced CLUT transfer occurs.
|||
|||	    VBL_TAG_SLIPLINE
|||	    	Argtype: int32*
|||
|||	    	The video line number on which SlipStream is enabled.
|||
|||	    VBL_TAG_SLIPSTREAM
|||	    	Argtype: boolean (int32*)
|||
|||	    	The current state of SlipStream; zero == off.
|||
|||	    VBL_TAG_VIRS
|||	    	Argtype: boolean (int32*)
|||
|||	    	The current state of internally-generated VIRS; zero == off.
|||
|||	    Note that the arguments in the TagArg array are pointers to
|||	    storage where you wish the results deposited.  For example:
|||
|||		int32	forceline;
|||
|||		TagArg vblquery[] = {
|||			VBL_TAG_FORCELINE, (void *) &forceline,
|||			TAG_END,	   0
|||		};
|||
|||	  Arguments
|||
|||	    args	Pointer to TagArg array.
|||
|||	  Result
|||
|||	    Err		An Opera error code if anything went awry, or
|||	    		zero if normal.
|||
|||	  Implementation
|||
|||	    Supervisor-mode folio call implemented in graphics folio V24.
|||
|||	  Associated Files
|||
|||	    vbl.h		ANSI Prototype, TagArg defines
|||
|||	    graphics.lib	ARM Link Library
|||
|||	  Notes
|||
|||	  Bugs
|||
|||	  See Also
|||
|||	    SetVBLAttrs()
|||
**/
static Err
gva (
struct VBLState	*vs,
int32		checkmem,
uint32		t,
void		*a
)
{
	Err	e;

	if (checkmem)
		if ((e = SuperValidateMem
			  (CURRENTTASK, (uint8 *) a, sizeof (uint32))) < 0)
			return (e);

	switch (t) {
	case VBL_TAG_LENGTH:
		* (int32 *) a = vs->vs_VBLLen + 1;
		break;

	case VBL_TAG_FORCELINE:
		* (int32 *) a = vs->vs_ForceLine;
		break;

	case VBL_TAG_SLIPLINE:
		* (int32 *) a = vs->vs_SlipLine;
		break;

	case VBL_TAG_SLIPSTREAM:
		* (int32 *) a = vs->vs_DoSlip;
		break;

	case VBL_TAG_VIRS:
		* (int32 *) a = vs->vs_DoVIRS;
		break;

	case VBL_TAG_REPORTSPORTLINES:
		* (int32 **) a = vs->vs_SPORTLims;
		break;

	case VBL_TAG_REPORTFORCEFIRST:
		* (VDLHeader ***) a = &vs->vs_ForceFirst;
		break;

	case VBL_TAG_REPORTPATCHADDR:
		* (VDLHeader ****) a = &vs->vs_PatchPoint;
		break;

	default:
		return (GRAFERR_BADTAG);
	}
	return (0);
}

Err
GetVBLAttrs (args)
struct TagArg	*args;
{
	return (TagProcessor (&vblstate, args, gva, (void *) TRUE));
}

Err
internalGetVBLAttrs (args)
struct TagArg	*args;
{
	return (TagProcessor (&vblstate, args, gva, (void *) FALSE));
}


/***************************************************************************
 * Minor implementation details...					:-)
 */
Err
createVBL ()
{
	register int	i, n;
	ShortVDL	*vdlmem;
	MemHdr		*m;
	void		*vrambase;

	if (!(m = findVRAMbase ()))
		return (GRAFERR_NOMEM);
	vrambase = m->memh_MemBase;

	if (!(vdlmem = SUPER_ALLOCMEM (VDLSIZE, MEMTYPE_VRAM |
						MEMTYPE_DMA |
						MEMTYPE_FILL)))
		return (GRAFERR_NOMEM);

	/*
	 * Set pointers and perform gross initialization of VDLs.
	 */
	for (n = 0;  n < 2;  n++)
		for (i = 0;  i < MAX_VBLP;  i++) {
			initshortVDL (vdlmem, vrambase);
			vblvdl[n][i] = vdlmem++;
		}

	/*
	 * Complete VDL initialization.
	 */
	for (i = 2;  --i >= 0; ) {
		ShortVDL	*forced, *enslip, *dispvirs, *killvirs;

		/*
		 * Setup pointers.
		 */
		forced	= vblvdl[i][VBLP_FORCE];
		enslip	= vblvdl[i][VBLP_ENSLIP];
		dispvirs= vblvdl[i][VBLP_DISPVIRS];
		killvirs= vblvdl[i][VBLP_KILLVIRS];


		/*
		 * Build forced first transfer VDL.
		 */
		/*  Nothing additional to do here.  */

		/*
		 * Build SlipStream enabler.
		 */
		enslip->sv.DMACtl	|= VDL_ENVIDDMA;
		enslip->sv_DispCtl	|= VDL_FORCETRANS;

		/*
		 * Build VIRS displayer.
		 */
		dispvirs->sv.DMACtl	|= VDL_ENVIDDMA |
					   1;	/*  One video line.  */
		dispvirs->sv.CurBuf	=
		dispvirs->sv.PrevBuf	= GrafBase->gf_VIRSPage;
		dispvirs->sv.NextVDL	= (VDLHeader *) killvirs;
		memcpy (dispvirs->sv_Misc, VIRScolors, sizeof (VIRScolors));

		/*
		 * Build VIRS killer.
		 */
		killvirs->sv.DMACtl	&= ~(VDL_LDCUR | VDL_LDPREV);
		killvirs->sv.CurBuf	=
		killvirs->sv.PrevBuf	= GrafBase->gf_ZeroPage;
		killvirs->sv_DispCtl	|= VDL_FORCETRANS;
	}

	/*
	 * At this point, we would interrogate SysInfo for the default
	 * conditions.  Until that happens, we'll suck the default state from
	 * some hard-coded numbers I just happen to have laying around ...
	 */
	vblstate.vs_VIRSLine = 19;	/*  Does it ever move?  */
	phase = 0;

	return (SetVBLAttrs (defaulttags));
}


/*
 * Why do I get the feeling this could be data driven?
 */
void
assembleVBL ()
{
	ShortVDL	*patch, *firstslip;
	ShortVDL	*forced, *enslip, *dispvirs, *killvirs;
	VDLHeader	*oldpatch;
	int		i;

	/*
	 * Setup pointers.
	 */
	forced	= vblvdl[phase][VBLP_FORCE];
	enslip	= vblvdl[phase][VBLP_ENSLIP];
	dispvirs= vblvdl[phase][VBLP_DISPVIRS];
	killvirs= vblvdl[phase][VBLP_KILLVIRS];

	patch = firstslip = NULL;
	oldpatch = NULL;

	/*
	 * Save old patch value.
	 */
	if (vblstate.vs_PatchPoint)
		oldpatch = *vblstate.vs_PatchPoint;

	if (vblstate.vs_DoVIRS  &&  vblstate.vs_DoSlip) {
		if (vblstate.vs_VIRSLine <= vblstate.vs_SlipLine) {
			/*  Icky case; worry about it later...  */
			Superkprintf ("You don't handle this yet...\n");
		} else {
			forced->sv.DMACtl =
			 SETLINES (forced->sv.DMACtl,
				   vblstate.vs_SlipLine -
				    vblstate.vs_ForceLine);
			forced->sv.NextVDL = (VDLHeader *) enslip;

			enslip->sv.DMACtl =
			 SETLINES (enslip->sv.DMACtl,
				   vblstate.vs_VIRSLine -
				    vblstate.vs_SlipLine);
			enslip->sv.NextVDL = (VDLHeader *) dispvirs;

			firstslip = enslip;

			goto patchkill;		/*  Look down.  */
		}
	} else if (vblstate.vs_DoVIRS) {
		/*
		 * Display VIRS signal on line #19.  Slightly icky in that
		 * we have to remember to kill it at line #20.
		 */
		forced->sv.DMACtl =
		 SETLINES (forced->sv.DMACtl,
			   vblstate.vs_VIRSLine - vblstate.vs_ForceLine);
		forced->sv.NextVDL = (VDLHeader *) dispvirs;

patchkill:	killvirs->sv.DMACtl =
		 SETLINES (killvirs->sv.DMACtl,
			   vblstate.vs_DispLine - (vblstate.vs_VIRSLine + 1));

		patch = killvirs;

	} else if (vblstate.vs_DoSlip) {
		/*
		 * Enable SlipStream at the specified position.
		 */
		forced->sv.DMACtl =
		 SETLINES (forced->sv.DMACtl,
			   vblstate.vs_SlipLine - vblstate.vs_ForceLine);
		forced->sv.NextVDL = (VDLHeader *) enslip;

		enslip->sv.DMACtl =
		 SETLINES (enslip->sv.DMACtl,
			   vblstate.vs_DispLine - vblstate.vs_SlipLine);

		firstslip = enslip;

		patch = enslip;

	} else {
		/*
		 * Dead simple; no special features turned on.
		 */
		forced->sv.DMACtl =
		 SETLINES (forced->sv.DMACtl, vblstate.vs_VBLLen);

		patch = forced;
	}
	vblstate.vs_ForceFirst = (VDLHeader *) forced;
	vblstate.vs_PatchPoint = &patch->sv.NextVDL;
	*vblstate.vs_PatchPoint = oldpatch;	/*  Restore old value.	*/

	/*
	 * Turn SlipStream on/off for all VBL VDL's.
	 */
	if (!firstslip)
		firstslip = forced;

	while (1) {
		for (i = MAX_VBLP;  --i >= 0; )
			if (firstslip == vblvdl[phase][i])
				slippatch (firstslip,
					   &vblswitches[i],
					   vblstate.vs_DoSlip);
		if (firstslip == patch)
			break;
		firstslip = (ShortVDL *) firstslip->sv.NextVDL;
	}

	if (patch = (ShortVDL *) GrafBase->gf_VDLBlank)
		slippatch (patch, &blankswitch, vblstate.vs_DoSlip);
	if (patch = (ShortVDL *) GrafBase->gf_VDLPreDisplay)
		slippatch (patch, &preswitch, vblstate.vs_DoSlip);
}



void
initshortVDL (sv, bufmem)
struct ShortVDL	*sv;
void		*bufmem;
{
	sv->sv.DMACtl	= VDL_LDCUR | VDL_LDPREV | VDL_LEN_SHORT_FMT;
	sv->sv.CurBuf	=
	sv->sv.PrevBuf	= bufmem;
	sv->sv.NextVDL	= NULL;
	sv->sv_DispCtl	= VDL_DISP_NOP;
	sv->sv_Misc[0]	= VDL_NOP;
	sv->sv_Misc[1]	= VDL_NOP;
	sv->sv_Misc[2]	= VDL_NOP;
}


MemHdr *
findVRAMbase ()
{
	register MemHdr	*m;

	for (m = (MemHdr *) FIRSTNODE (KernelBase->kb_MemHdrList);
	     NEXTNODE (m);
	     m = (MemHdr *) NEXTNODE (m))
		if (m->memh_Types & MEMTYPE_VRAM)
			return (m);

	return (NULL);
}

void
PALpatch (newflag)
uint32	newflag;
{
	register int	i;
	ShortVDL	*sv;
	uint32		oldflag;

	for (i = MAX_VBLP;  --i >= 0; ) {
		sv = vblvdl[phase][i];
		sv->sv_DispCtl = (sv->sv_DispCtl & ~VDL_PALSEL) | newflag;
	}

	if (sv = (ShortVDL *) GrafBase->gf_VDLPreDisplay)
		sv->sv_DispCtl = (sv->sv_DispCtl & ~VDL_PALSEL) | newflag;
	if (sv = (ShortVDL *) GrafBase->gf_VDLPostDisplay) {
		oldflag = sv->sv_DispCtl & VDL_PALSEL;
		sv->sv_DispCtl = (sv->sv_DispCtl & ~VDL_PALSEL) | newflag;
	} else
		oldflag = newflag;

	/*
	 * PAL has no VIRS signal.  I really should reconstruct the VDL
	 * here, but this is meant to be called from a FIRQ (for some reason
	 * (I'm going to love redesigning all this stuff...)).
	 */
	sv = vblvdl[phase][VBLP_DISPVIRS];
	if (newflag)
		sv->sv.DMACtl &= ~VDL_ENVIDDMA;
	else
		sv->sv.DMACtl |= VDL_ENVIDDMA;

	/*
	 * Determine if we need to slew the SPORT line adjustments (forced
	 * VDL transfer happens three lines later under PAL-wide).
	 */
	if (oldflag ^ newflag) {
		if (newflag)
			i = SPORTSLEW_PALWIDE;
		else
			i = -SPORTSLEW_PALWIDE;
	} else
		i = 0;

	if (i) {
		vblstate.vs_SPORTLims[0] += i;
		vblstate.vs_SPORTLims[1] += i;	/*### Does this move, too? */
	}
}

void
slippatch (sv, ss, state)
struct ShortVDL		*sv;
struct SlipSwitch	*ss;
int			state;
{
	uint32	newdma, newdisp, foo;

	if (state) {
		newdma = ss->ss_DMAOn;
		newdisp = ss->ss_DispOn;
	} else {
		newdma = ss->ss_DMAOff;
		newdisp = ss->ss_DispOff;
	}
	foo = sv->sv.DMACtl;
	foo &= ~ss->ss_DMAMask;
	newdma |= foo;

	foo = sv->sv_DispCtl;
	foo &= ~ss->ss_DispMask;
	newdisp |= foo;

	/*
	 * Try and do this as close to atomically as possible.
	 */
	sv->sv.DMACtl = newdma;
	sv->sv_DispCtl = newdisp;
}
