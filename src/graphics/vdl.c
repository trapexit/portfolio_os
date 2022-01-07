/* *************************************************************************
 *
 * VDL (Video Display List) routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * $Id: vdl.c,v 1.46 1994/12/21 21:15:31 ewhac Exp $
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 930708 SHL              Fixed last line of display VDL glitch
 * 930706 SHL              Commented out blue support
 * 930612 SHL              Patched a bunch of stuff in SubmitVDL()
 * 930316 -RJ              Took out the WaitForLine( 7 );
 * 921010 -RJ Mical        Created this file!
 *
 * ********************************************************************** */


/***************************************************************\
* Header files                                                  *
\***************************************************************/

#define SUPER
#include "types.h"

#include "debug.h"
#include "item.h"
#include "nodes.h"
#include "interrupts.h"
#include "kernel.h"
#include "mem.h"
#include "list.h"
#include "task.h"
#include "folio.h"
#include "tags.h"
#include "kernelnodes.h"
#include "super.h"
#include <sherryvers.h>

#include "intgraf.h"
#include "overlay.h"

#include "stdarg.h"
#include "strings.h"

#include "vdl.h"
#include "vbl.h"


/***************************************************************************
 * #defines
 */
#define	VDL_DMA_BLANK		(VDL_PREVSEL | VDL_LDCUR | VDL_LDPREV | \
				 ((32 + 2) << VDL_LEN_SHIFT ) | 240)
#define VDL_DISP_BLANK		(VDL_DISPCTRL)

#define	VDL_DMA_LAST		(VDL_LDCUR | VDL_LDPREV | \
				 (4 << VDL_LEN_SHIFT) | 0)


/***************************************************************\
* Structures                                                    *
\***************************************************************/


struct _vdli {
  int32 height, length, vdltype, displaytype, disp_setflags, disp_resetflags;
  VDLEntry *vp;
};


/***************************************************************\
* Variables                                                     *
\***************************************************************/

extern void	TakedownOverlay(struct Overlay *);
extern MemHdr	*findVRAMbase(void);

static Err	createblankVDL (void *blankbuf);

void	applyVDLmods (struct VDL *, struct VDLHeader *, uint32, uint32);


/*
 * VDL instructions to load default color set.
 */
uint32	defaultcolors[] = {
	0x00000000,
	0x01080808,
	0x02101010,
	0x03181818,
	0x04212121,
	0x05292929,
	0x06313131,
	0x07393939,
	0x08424242,
	0x094a4a4a,
	0x0a525252,
	0x0b5a5a5a,
	0x0c636363,
	0x0d6b6b6b,
	0x0e737373,
	0x0f7b7b7b,
	0x10848484,
	0x118c8c8c,
	0x12949494,
	0x139c9c9c,
	0x14a5a5a5,
	0x15adadad,
	0x16b5b5b5,
	0x17bdbdbd,
	0x18c6c6c6,
	0x19cecece,
	0x1ad6d6d6,
	0x1bdedede,
	0x1ce7e7e7,
	0x1defefef,
	0x1ef7f7f7,
	0x1fffffff,
	0xe0000000	/*  Background color  */
};

/*
 * TagArgs to interrogate VBL attributes.
 */
static TagArg	gvatags[] = {		/*  Args filled in later.  */
	VBL_TAG_REPORTFORCEFIRST,	0,
	VBL_TAG_REPORTPATCHADDR,	0,
	TAG_END,			0,
	TAG_END,			0
};

static TagArg	sliptags[] = {
	VBL_TAG_SLIPSTREAM,	(void *) TRUE,
	TAG_END,		0
};


extern Overlay	*ActiveOverlay;

uint32 _forcecreatevdlflag = FALSE;
uint32 *_PatchDISPCTRL1, *_PatchDISPCTRL2, *_PatchDISPCTRL3;


extern volatile uint32	_tripaddress;

/***************************************************************\
* Code                                                          *
\***************************************************************/

Err
BuildSystemVDLs (void)
{
	ShortVDL	*last;
	ShortVDL	*first;
	MemHdr		*vram;
	Err		err;

	if (!(vram = findVRAMbase ())) {
		SDEBUGVDL
		 (("BuildSystemVDLs failed - could not find VRAM!?\n"));
		return (GRAFERR_NOMEM);
	}

	/*
	 * Create and initialize VDL manager for VBlank.
	 */
	if ((err = createVBL ()) < 0)
		return (err);

	/*
	 * Create "first" VDL, which is actually used as the stuff point
	 * for Screens.  It's also where we'll do vertical centering.
	 */
	if (!(first = SUPER_ALLOCMEM (sizeof (ShortVDL),
				      MEMTYPE_VRAM | MEMTYPE_FILL)))
		return (GRAFERR_NOMEM);

	first->sv.DMACtl  = VDL_DMA_LAST |
			    1;
	first->sv.CurBuf  = GrafBase->gf_ZeroPage;
	first->sv.PrevBuf = GrafBase->gf_ZeroPage;

	first->sv_DispCtl = VDL_DISP_NOP | VDL_FORCETRANS |
			    slipstream_dispctrl;
	if (pal2flag)
		first->sv_DispCtl |= VDL_PALSEL;

	GrafBase->gf_VDLPreDisplay	= (VDLEntry *) first;
	GrafBase->gf_VDLDisplayLink	= (VDLEntry *) &first->sv.NextVDL;

	/*
	 * Create VDL which terminates the display.
	 */
	if (!(last = SUPER_ALLOCMEM (sizeof (ShortVDL),
				     MEMTYPE_VRAM | MEMTYPE_FILL)))
		return (GRAFERR_NOMEM);

	last->sv.DMACtl  = VDL_DMA_LAST;
	last->sv.CurBuf  = GrafBase->gf_ZeroPage;
	last->sv.PrevBuf = GrafBase->gf_ZeroPage;

	last->sv_DispCtl = VDL_DISP_NOP;
	if (pal2flag)
		last->sv_DispCtl |= VDL_PALSEL;
	last->sv_Misc[0] = MakeCLUTColorEntry (0, 0, 0, 0);
	last->sv_Misc[1] = MakeCLUTBackgroundEntry (0, 0, 0);
	last->sv_Misc[2] = VDL_NOP;

	GrafBase->gf_VDLPostDisplay	= (VDLEntry *) last;

	/*
	 * Create VDL which displays a great deal of nothing.
	 */
	if ((err = createblankVDL (vram->memh_MemBase)) < 0)
		return (err);

	/*
	 * Initialize some GrafBase stuff.
	 */
	GrafBase->gf_VDLForcedFirst	= NULL;

	/*
	 * Do some final tweaking of the VBL list.  (Clearly this VBL stuff
	 * needs more design time.)
	 */
	if (slipstream_dispctrl)
		SetVBLAttrs (sliptags);
	if (pal2flag)
		PALpatch (VDL_PALSEL);

	/*
	 * Now then; how'd you set stuff up?
	 */
	gvatags[0].ta_Arg = &GrafBase->gf_ForcedFirstPtr;
	gvatags[1].ta_Arg = &GrafBase->gf_DisplayLinkPtr;
	if ((err = internalGetVBLAttrs (gvatags)) < 0)
		return (err);

	last->sv.NextVDL = (VDLHeader *) *GrafBase->gf_ForcedFirstPtr;
	**GrafBase->gf_DisplayLinkPtr = first;

	return (0);
}


static Err
createblankVDL (blankbuf)
void	*blankbuf;
{
	FullVDL		*fv;
	VDL		*vdl;
	Item		vdlItem;
	uint32		savestate;
	static TagArg	ta[] = {
		CREATEVDL_TAG_VDLTYPE,	(void *) VDLTYPE_SYSTEM,
		TAG_END,		0
	};

	/*
	 * This is an unbelievably disgusting hack...
	 * I'll be fixing this at my earliest opportunity.  ewhac 9407.05
	 */
	savestate = Disable ();
	_forcecreatevdlflag = TRUE;
	vdlItem = SuperCreateItem (MKNODEID (NODE_GRAPHICS, TYPE_VDL), ta);
	_forcecreatevdlflag = FALSE;
	Enable (savestate);

	if ((int32) vdlItem < 0) {
		SDEBUGVDL
		 (("createblankVDL failed - could not create VDL Item\n"));
		return ((Err) vdlItem);
	}

	if (!(fv = SUPER_ALLOCMEM (sizeof (FullVDL),
				   MEMTYPE_VRAM | MEMTYPE_FILL)))
	{
		SDEBUGVDL (("Can't alloc blank VDL RAM.\n"));
		return (GRAFERR_NOMEM);
	}

	/*
	 * Initialize Item.
	 */
	vdl = (VDL *) LookupItem (vdlItem);
	vdl->vdl_Type		= VDLTYPE_SYSTEM;
	vdl->vdl_DataPtr	= (VDLEntry *) fv;
	vdl->vdl_DataSize	= sizeof (*fv);
	vdl->vdl_Flags		= 0;
	vdl->vdl_DisplayType	= GrafBase->gf_DefaultDisplayType;
	vdl->vdl_Height		= 240;	/*  Is this right?  */
	vdl->vdl_Offset		= (_DisplayHeight[vdl->vdl_DisplayType] -
				   vdl->vdl_Height) / 2 +
				  _DisplayOffset[vdl->vdl_DisplayType];

	/*
	 * Initialize VDL.
	 */
	fv->fv.DMACtl	= VDL_DMA_BLANK;
	fv->fv.CurBuf	= blankbuf;
	fv->fv.PrevBuf	= blankbuf;
	fv->fv.NextVDL	= (VDLHeader *) GrafBase->gf_VDLPostDisplay;
	fv->fv_DispCtl	= VDL_DISP_BLANK | slipstream_dispctrl;
	memcpy (fv->fv_Colors, defaultcolors, sizeof (fv->fv_Colors));

	/*
	 * Install.
	 */
	GrafBase->gf_VDLBlank		= (VDLEntry *) fv;
	GrafBase->gf_BlankVDL		= vdl;

	GrafBase->gf_CurrentVDLEven	=
	GrafBase->gf_CurrentVDLOdd	= vdl;

	return (0);
}



/* JCR */
bool
IsVDLInUse(VDL *vdl)
/* Return TRUE if the vdl is in use, FALSE if it is not.
 * A vdl is in use if it is accessed by
 * a group structure that is displayed, or if it matches the "current vdl"
 * in the group field.
 *
 * A vdl is in use even if the group is temporarily not in the list
 * of those displayed.
 */
/* CURRENT ASSUMPTION: there is ONE sg, each w/ TWO VDL's. JCR*/
/* Sc group lists are NOT implemented; we look directly at the Even/Odd ptrs */
/* 5-10-93 */
{
	if (GrafBase->gf_CurrentVDLEven == vdl  ||
	    GrafBase->gf_CurrentVDLOdd == vdl)
		return (TRUE);

	return (FALSE);
}



Err
internalSubmitVDL (VDL *vdl, struct _vdli *vdli)
{
  Err retvalue=0;
  VDLEntry *Proof_new_loc; /* source,dest of copy */

  switch (vdli->vdltype)
    {
    case VDLTYPE_FULL:
      break;
    case VDLTYPE_SIMPLE:
      break;
    case VDLTYPE_COLOR:
      retvalue = GRAFERR_NOTYET;
      goto DONE;
      break;
    case VDLTYPE_ADDRESS:
      /* type not yet implemented */
      retvalue = GRAFERR_NOTYET;
      goto DONE;
      break;
    case VDLTYPE_DYNAMIC:
      /* type not yet implemented */
      retvalue = GRAFERR_NOTYET;
      goto DONE;
      break;
    default:
      retvalue = GRAFERR_NOTYET;
      goto DONE;
      break;
    }

  /* Proof and relocate user's VDLEntry list. */
  /* Return ptr to new sys ram location. */
  Proof_new_loc = ProofVDLEntry (vdli->vp, vdli->length, vdli->displaytype, vdli->height);
  if ((int32)Proof_new_loc < 0) {
    retvalue = (int32) Proof_new_loc;
    goto DONE; /* Invalid VDLEntry(s) */
  }

  vdl->vdl_Type = vdli->vdltype;
  vdl->vdl_DataPtr = Proof_new_loc;
  vdl->vdl_DataSize = vdli->length*sizeof(VDLEntry); /* for SUPER_FREEMEM() */
  vdl->vdl_Flags = 0;
  vdl->vdl_DisplayType = vdli->displaytype;
  vdl->vdl_Height = vdli->height;
  vdl->vdl_Offset = (_DisplayHeight[vdl->vdl_DisplayType]-vdl->vdl_Height) / 2
    + _DisplayOffset[vdl->vdl_DisplayType];

DONE:
  return ( retvalue );
}


Item
SetVDL( Item screenItem, Item vdlItem )
/* Connect the screen and the vdl so that submitting the screen for display
 * will result in the display of the vdl.
 *
 * If the vdl is currently being displayed, an error is returned.
 * If the screen will be displayed
 * (as seen by the group's screen pointers), then this returns an error.
 * If the screen is currently
 * being displayed but another screen has already been specified to take over,
 * no error arises.
 */
{
  VDL *vdl;
  Screen *screen;
  Item cur_vdl_item;

  if ( (vdl = (VDL *)CheckItem( vdlItem, NODE_GRAPHICS, TYPE_VDL )) == 0 ) {
    /* bad VDL Item number */
    return GRAFERR_BADITEM;
  }
  if (vdl->vdl.n_Owner != CURRENTTASK->t.n_Item) {
    PRINTNOTOWNER (vdl->vdl.n_Item, CURRENTTASK->t.n_Item);
    return GRAFERR_NOTOWNER;
  }

  if ( (screen = (Screen *)CheckItem( screenItem, NODE_GRAPHICS, TYPE_SCREEN )) == 0 ) {
    /* bad screen Item number */
    return GRAFERR_BADITEM;
  }
  if (screen->scr.n_Owner != CURRENTTASK->t.n_Item) {
    PRINTNOTOWNER (screen->scr.n_Item, CURRENTTASK->t.n_Item);
    return GRAFERR_NOTOWNER;
  }

#if 0
/* It seems to me that there's no problem linking an in use VDL to a screen - SHL 9307.23 */
  if ( IsVDLInUse( vdl ) ) {
    /* VDL in use */
    return GRAFERR_VDLINUSE;
  }
#endif

  cur_vdl_item= screen->scr_VDLItem; /* save, for return */

  /* Now we must interlink VDL and Screen, as we do in CreateScreenGroup()*/
  vdl->vdl_ScreenPtr = screen;
  screen->scr_VDLPtr = vdl; /* Back Acha */
  /* JCR */
  screen->scr_VDLItem = vdlItem; /* for next call */
  screen->scr_VDLType = vdl->vdl_Type;

  return cur_vdl_item;
}

void  /* Temp, until WaitVBL truely works */
jacks_waitVBL()		/* rewritten 9606.15 - SHL */
	{
	int32 i=GrafBase->gf_VBLNumber;
	while(i==GrafBase->gf_VBLNumber);
	}

static int
ween_HW_from_VDL (
struct VDL *weenvdl
)
{
	VDL	*vdl0, *vdl1;
	int	wasweened;

	wasweened = 0;
	vdl0 = GrafBase->gf_CurrentVDLEven;
	vdl1 = GrafBase->gf_CurrentVDLOdd;

	if (vdl0 == weenvdl) {
		if (vdl0->vdl_DisplayType == DI_TYPE_PAL2) {
			resync_displaywidth ();
		}
		addVDLtofree (vdl0);
		GrafBase->gf_CurrentVDLEven = GrafBase->gf_BlankVDL;
		wasweened++;
	}

	if (vdl1 == weenvdl) {
		if (vdl1 != vdl0) {
			if (vdl1->vdl_DisplayType == DI_TYPE_PAL2) {
				resync_displaywidth ();
			}
			addVDLtofree (vdl1);
		}
		GrafBase->gf_CurrentVDLOdd = GrafBase->gf_BlankVDL;
		wasweened++;
	}

	return (wasweened);
}


static int32
icv (struct VDL *vdl, struct _vdli *vdli, uint32 t, uint32 a)
{
  switch (t) {
  case CREATEVDL_TAG_SLIPSTREAM:
    if (a) {
      vdli->disp_setflags |= VDL_SLPDCEL|VDL_BACKTRANS;
    } else {
      vdli->disp_resetflags |= VDL_SLPDCEL|VDL_BACKTRANS;
    }
    break;
  case CREATEVDL_TAG_HAVG:
    if (a) {
      vdli->disp_setflags |= VDL_HINTEN;
    } else {
      vdli->disp_resetflags |= VDL_HINTEN;
    }
    break;
  case CREATEVDL_TAG_VAVG:
    if (a) {
      vdli->disp_setflags |= VDL_VINTEN;
    } else {
      vdli->disp_resetflags |= VDL_VINTEN;
    }
    break;
  case CREATEVDL_TAG_HSUB:
    switch (a) {
    case 0:
      vdli->disp_resetflags |= VDL_HSUB_MASK;
      vdli->disp_setflags |= VDL_HSUB_ZERO;
      break;
    case 1:
      vdli->disp_resetflags |= VDL_HSUB_MASK;
      vdli->disp_setflags |= VDL_HSUB_ONE;
      break;
    case 2:
      vdli->disp_resetflags |= VDL_HSUB_MASK;
      vdli->disp_setflags |= VDL_HSUB_FRAME;
      break;
    default:
      vdli->disp_resetflags |= VDL_HSUB_MASK;
      vdli->disp_setflags |= VDL_HSUB_NOP;
      break;
    }
    break;
  case CREATEVDL_TAG_VSUB:
    switch (a) {
    case 0:
      vdli->disp_resetflags |= VDL_VSUB_MASK;
      vdli->disp_setflags |= VDL_VSUB_ZERO;
      break;
    case 1:
      vdli->disp_resetflags |= VDL_VSUB_MASK;
      vdli->disp_setflags |= VDL_VSUB_ONE;
      break;
    case 2:
      vdli->disp_resetflags |= VDL_VSUB_MASK;
      vdli->disp_setflags |= VDL_VSUB_FRAME;
      break;
    default:
      vdli->disp_resetflags |= VDL_VSUB_MASK;
      vdli->disp_setflags |= VDL_VSUB_NOP;
      break;
    }
    break;
  case CREATEVDL_TAG_SWAPHV:
    if (a) {
      vdli->disp_resetflags |= VDL_SWAPHV;
    } else {
      vdli->disp_setflags |= VDL_SWAPHV;
    }
  case CREATEVDL_TAG_WINHAVG:
    if (a) {
      vdli->disp_setflags |= VDL_WINHINTEN;
    } else {
      vdli->disp_resetflags |= VDL_WINHINTEN;
    }
    break;
  case CREATEVDL_TAG_WINVAVG:
    if (a) {
      vdli->disp_setflags |= VDL_WINVINTEN;
    } else {
      vdli->disp_resetflags |= VDL_WINVINTEN;
    }
    break;
  case CREATEVDL_TAG_WINHSUB:
    switch (a) {
    case 0:
      vdli->disp_resetflags |= VDL_WINHSUB_MASK;
      vdli->disp_setflags |= VDL_WINHSUB_ZERO;
      break;
    case 1:
      vdli->disp_resetflags |= VDL_WINHSUB_MASK;
      vdli->disp_setflags |= VDL_WINHSUB_ONE;
      break;
    case 2:
      vdli->disp_resetflags |= VDL_WINHSUB_MASK;
      vdli->disp_setflags |= VDL_WINHSUB_FRAME;
      break;
    default:
      vdli->disp_resetflags |= VDL_WINHSUB_MASK;
      vdli->disp_setflags |= VDL_WINHSUB_NOP;
      break;
    }
    break;
  case CREATEVDL_TAG_WINVSUB:
    switch (a) {
    case 0:
      vdli->disp_resetflags |= VDL_WINVSUB_MASK;
      vdli->disp_setflags |= VDL_WINVSUB_ZERO;
      break;
    case 1:
      vdli->disp_resetflags |= VDL_WINVSUB_MASK;
      vdli->disp_setflags |= VDL_WINVSUB_ONE;
      break;
    case 2:
      vdli->disp_resetflags |= VDL_WINVSUB_MASK;
      vdli->disp_setflags |= VDL_WINVSUB_FRAME;
      break;
    default:
      vdli->disp_resetflags |= VDL_WINVSUB_MASK;
      vdli->disp_setflags |= VDL_WINVSUB_NOP;
      break;
    }
    break;
  case CREATEVDL_TAG_WINSWAPHV:
    if (a) {
      vdli->disp_resetflags |= VDL_WINSWAPHV;
    } else {
      vdli->disp_setflags |= VDL_WINSWAPHV;
    }
    break;
  case CREATEVDL_TAG_CLUTBYPASS:
    if (a) {
      vdli->disp_resetflags |= VDL_CLUTBYPASSEN;
    } else {
      vdli->disp_setflags |= VDL_CLUTBYPASSEN;
    }
    break;

  default:
   {
    if (vdli->vdltype < 0)
      /*
       * Doing a modify; can't change these fields.
       */
      return (GRAFERR_BADTAG);

    else
      /*
       * Doing a create.
       */
      switch (t) {
      case CREATEVDL_TAG_VDLTYPE:
        vdli->vdltype = a;
        break;
      case CREATEVDL_TAG_DISPLAYTYPE:
        vdli->displaytype = a;
        break;
      case CREATEVDL_TAG_LENGTH:
        vdli->length = a;
        break;
      case CREATEVDL_TAG_HEIGHT:
        vdli->height = a;
        break;
      case CREATEVDL_TAG_DATAPTR:
        vdli->vp = (VDLEntry *)a;
        break;
      default:
        return (GRAFERR_BADTAG);
      }
    break;
   }
  }
  return 0;
}


Item
internalCreateVDL( VDL *vdl, TagArg *args )
{
  struct _vdli vdli;
  Err e;

  memset (&vdli, 0, sizeof(vdli));

  vdli.height = 240;
  vdli.vdltype = VDLTYPE_FULL;

  if ((e = TagProcessor ((void *)vdl, args, icv, (void *)&vdli)) < 0) {
    return e;
  }
  vdl->vdl.n_Next = vdl->vdl.n_Prev = NULL;

  /* Special case - system VDL is a blank template to be filled in by supervisor code */
  if (_forcecreatevdlflag) {
    return vdl->vdl.n_Item;
  }

  if (vdli.vdltype != VDLTYPE_FULL)
    if (vdli.vdltype == VDLTYPE_SIMPLE) {
      if (SuperDiscOsVersion (0) > DiscOs_1_3) {
	DEVBUG (("VDLTYPE_SIMPLE no longer supported; use VDLTYPE_FULL.\n"));
	return (GRAFERR_BADVDLTYPE);
      }
    } else
      return (GRAFERR_BADVDLTYPE);

#if 0
/*  The old way.  */
  if (vdli.vdltype != VDLTYPE_FULL  &&  vdli.vdltype != VDLTYPE_SIMPLE) {
    return GRAFERR_BADVDLTYPE;
  }
#endif

  if (vdli.displaytype == DI_TYPE_DEFAULT) {
    vdli.displaytype = GrafBase->gf_DefaultDisplayType;
  }

  if ((vdli.displaytype>31) || !((1L<<vdli.displaytype)&GrafBase->gf_DisplayTypeMask)) {
    return GRAFERR_BADDISPLAYTYPE;
  }

  if ((vdli.height<1) || (vdli.height>_DisplayHeight[vdli.displaytype])) {
    return GRAFERR_BADDISPDIMS;
  }

  if ((e = internalSubmitVDL (vdl, &vdli)) < 0) {
    return e;
  }

  return vdl->vdl.n_Item;
}


Item
internalOpenVDL( VDL *vdl, void *args )
{
  return (vdl->vdl.n_Item);
}



int32
internalCloseVDL (VDL *vdl, Task *t)
{
  return (0);
}


int32
internalDeleteVDL (VDL *vdl, Task *t)
{
  register int	wasweened;

  /* Dont change horses in the middle of a screen. */
  /* IF the HW is currently dependent on this addr, ween it */
  wasweened = ween_HW_from_VDL (vdl);

  /*
   * The links are only non-NULL when they are in the FIRQ's
   * "vdlstofree" list.  When they are guaranteed no longer in use, they
   * will be nulled.
   */
  while (vdl->vdl.n_Next)
    graf_waitVBL ();

  if (wasweened  &&  ActiveOverlay)
    TakedownOverlay (ActiveOverlay);

  /* ALLOC for VDL data could have failed! JCR */
  if (vdl->vdl_DataPtr) {
    SUPER_FREEMEM (vdl->vdl_DataPtr, vdl->vdl_DataSize);
    vdl->vdl_DataPtr = NULL;
  }

  return 0;
}


Err
ModifyVDL (Item vdlItem, TagArg* vdlTags)
{
  struct _vdli	vdli;
  VDL		*v;
  Err		e;

#if 0
  VDLEntry *vp;
  TagArg *state, *tag;
  uint32 disp_setflags, disp_resetflags;
  uint32 t, a, d;
  uint32 dmactrl, bufptr1, bufptr2, nextptr;
  int32 count;
#endif

  v = (VDL*) CheckItem (vdlItem, NODE_GRAPHICS, TYPE_VDL);
  if (v==0) {
    return GRAFERR_BADITEM;
  }

  if (v->vdl.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,vdlItem)<0) {
      PRINTNOTOWNER (vdlItem, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

  memset (&vdli, 0, sizeof (vdli));
  vdli.vdltype = -1;

  if ((e = TagProcessor (v, vdlTags, icv, &vdli)) < 0)
    return (e);

#if 0
  disp_setflags = disp_resetflags = 0;
  state = vdlTags;
  while (tag = NextTagArg ((const TagArg **) &state)) {
    t = tag->ta_Tag;
    a = (uint32) tag->ta_Arg;
    switch (t) {
    case CREATEVDL_TAG_SLIPSTREAM:
      if (a) {
	disp_setflags |= VDL_SLPDCEL|VDL_BACKTRANS;
      } else {
	disp_resetflags |= VDL_SLPDCEL|VDL_BACKTRANS;
      }
      break;
    case CREATEVDL_TAG_HAVG:
      if (a) {
	disp_setflags |= VDL_HINTEN;
      } else {
	disp_resetflags |= VDL_HINTEN;
      }
      break;
    case CREATEVDL_TAG_VAVG:
      if (a) {
	disp_setflags |= VDL_VINTEN;
      } else {
	disp_resetflags |= VDL_VINTEN;
      }
      break;
    case CREATEVDL_TAG_HSUB:
      switch (a) {
      case 0:
	disp_resetflags |= VDL_HSUB_MASK;
	disp_setflags |= VDL_HSUB_ZERO;
	break;
      case 1:
	disp_resetflags |= VDL_HSUB_MASK;
	disp_setflags |= VDL_HSUB_ONE;
	break;
      case 2:
	disp_resetflags |= VDL_HSUB_MASK;
	disp_setflags |= VDL_HSUB_FRAME;
	break;
      default:
	disp_resetflags |= VDL_HSUB_MASK;
	disp_setflags |= VDL_HSUB_NOP;
	break;
      }
      break;
    case CREATEVDL_TAG_VSUB:
      switch (a) {
      case 0:
	disp_resetflags |= VDL_VSUB_MASK;
	disp_setflags |= VDL_VSUB_ZERO;
	break;
      case 1:
	disp_resetflags |= VDL_VSUB_MASK;
	disp_setflags |= VDL_VSUB_ONE;
	break;
      case 2:
	disp_resetflags |= VDL_VSUB_MASK;
	disp_setflags |= VDL_VSUB_FRAME;
	break;
      default:
	disp_resetflags |= VDL_VSUB_MASK;
	disp_setflags |= VDL_VSUB_NOP;
	break;
      }
      break;
    case CREATEVDL_TAG_SWAPHV:
      if (a) {
	disp_resetflags |= VDL_SWAPHV;
      } else {
	disp_setflags |= VDL_SWAPHV;
      }
    case CREATEVDL_TAG_WINHAVG:
      if (a) {
	disp_setflags |= VDL_WINHINTEN;
      } else {
	disp_resetflags |= VDL_WINHINTEN;
      }
      break;
    case CREATEVDL_TAG_WINVAVG:
      if (a) {
	disp_setflags |= VDL_WINVINTEN;
      } else {
	disp_resetflags |= VDL_WINVINTEN;
      }
      break;
    case CREATEVDL_TAG_WINHSUB:
      switch (a) {
      case 0:
	disp_resetflags |= VDL_WINHSUB_MASK;
	disp_setflags |= VDL_WINHSUB_ZERO;
	break;
      case 1:
	disp_resetflags |= VDL_WINHSUB_MASK;
	disp_setflags |= VDL_WINHSUB_ONE;
	break;
      case 2:
	disp_resetflags |= VDL_WINHSUB_MASK;
	disp_setflags |= VDL_WINHSUB_FRAME;
	break;
      default:
	disp_resetflags |= VDL_WINHSUB_MASK;
	disp_setflags |= VDL_WINHSUB_NOP;
	break;
      }
      break;
    case CREATEVDL_TAG_WINVSUB:
      switch (a) {
      case 0:
	disp_resetflags |= VDL_WINVSUB_MASK;
	disp_setflags |= VDL_WINVSUB_ZERO;
	break;
      case 1:
	disp_resetflags |= VDL_WINVSUB_MASK;
	disp_setflags |= VDL_WINVSUB_ONE;
	break;
      case 2:
	disp_resetflags |= VDL_WINVSUB_MASK;
	disp_setflags |= VDL_WINVSUB_FRAME;
	break;
      default:
	disp_resetflags |= VDL_WINVSUB_MASK;
	disp_setflags |= VDL_WINVSUB_NOP;
	break;
      }
      break;
    case CREATEVDL_TAG_WINSWAPHV:
      if (a) {
	disp_resetflags |= VDL_WINSWAPHV;
      } else {
	disp_setflags |= VDL_WINSWAPHV;
      }
      break;
    case CREATEVDL_TAG_CLUTBYPASS:
      if (a) {
	disp_resetflags |= VDL_CLUTBYPASSEN;
      } else {
	disp_setflags |= VDL_CLUTBYPASSEN;
      }
      break;
    default:
      return GRAFERR_BADTAG;
    }
  }
#endif

  applyVDLmods (v, NULL, vdli.disp_setflags, vdli.disp_resetflags);

  if (ActiveOverlay)
    modifyundervdl (ActiveOverlay, v, vdli.disp_setflags, vdli.disp_resetflags);

#if 0
  vp = v->vdl_DataPtr;
  do {
    dmactrl = *vp++;
    bufptr1 = *vp++;
    bufptr2 = *vp++;
    nextptr = *vp++;

    count = (dmactrl&VDL_LEN_MASK)>>VDL_LEN_SHIFT;

    while (count--) {
      d = *vp;
      if ((d&(VDL_DISPCTRL|VDL_BACKGROUND|VDL_NULLAMY)) == VDL_DISPCTRL) {
	*vp = (d &~ disp_resetflags) | disp_setflags;
      }
      vp++;
    }

  } while ( (uint32)(nextptr-(int32)v->vdl_DataPtr) < v->vdl_DataSize );
#endif

  return 0;
}


/*
 * Apply specified changes to VDL display control words.  Optionally start
 * at 'vh' in the chain.  Follow the VDL chain until we hop outside the VDL's
 * declared dataspace.
 */
void
applyVDLmods (vdl, vh, setflags, clearflags)
struct VDL		*vdl;
struct VDLHeader	*vh;
uint32			setflags, clearflags;
{
	register uint32	v, *vp;
	register int	count;
	uint32		nextvdl;

	if (!vh)
		vh = (VDLHeader *) vdl->vdl_DataPtr;

	do {
		nextvdl = (uint32) vh->NextVDL;

		count = (vh->DMACtl & VDL_LEN_MASK) >> VDL_LEN_SHIFT;
		vp = (uint32 *) (vh + 1);

		while (--count >= 0) {
			v = *vp;
			if ((v & (VDL_DISPCTRL |
				  VDL_BACKGROUND |
				  VDL_NULLAMY)) == VDL_DISPCTRL)
				*vp = (v & ~clearflags) | setflags;
			vp++;
		}
		vh = (VDLHeader *) nextvdl;

	} while ((nextvdl - (uint32) vdl->vdl_DataPtr) < vdl->vdl_DataSize);
}




/* === ================================================================== */
/* === ================================================================== */
/* === ================================================================== */

static int32
ControlVDL (
Item	screenItem,
int32	clearflag,
int32	setflag
)
{
	Screen		*screen;
	VDL		*vdl;
	VDLEntry	*entry, value;

	if (!(screen = (Screen *) CheckItem
				   (screenItem, NODE_GRAPHICS, TYPE_SCREEN)))
		return (GRAFERR_BADITEM);

	if (screen->scr.n_Owner != CURRENTTASK->t.n_Item) {
		if (ItemOpened (CURRENTTASK->t.n_Item,screenItem) < 0) {
			PRINTNOTOWNER (screen->scr.n_Item,
				       CURRENTTASK->t.n_Item);
			return (GRAFERR_NOTOWNER);
		}
	}

	vdl = screen->scr_VDLPtr;
	if (vdl->vdl_Type != VDLTYPE_SIMPLE) {
		/*
		 * I can see this breaking some boneheaded title that
		 * empirically discovered this would work with a VDLTYPE_FULL
		 * if they laid it out right.  *sigh*  Maybe I should do a
		 * longhand search in this case.
		 ***
		 * Remove this printf() after PQA validates that I didn't
		 * break anything.
		 */
		Superkprintf ("ControlVDL() applied to non-VDLTYPE_SIMPLE.\n");
		return (GRAFERR_BADVDLTYPE);
	}

	/* JCR */
	entry = vdl->vdl_DataPtr;
	value = *(entry + 4);
	value |= setflag;
	value = value & ~clearflag;
	*(entry + 4) = value;

	if (ActiveOverlay)
		controlundervdl (ActiveOverlay, vdl, setflag, clearflag);

	return (0);
}


int32
EnableVAVG (
Item screenItem
)
{
	return (ControlVDL (screenItem, 0, VDL_VINTEN));
}



int32
DisableVAVG (
Item screenItem
)
{
	return (ControlVDL (screenItem, VDL_VINTEN, 0));
}



int32
EnableHAVG (
Item screenItem
)
{
	return (ControlVDL (screenItem, 0, VDL_HINTEN));
}



int32
DisableHAVG (
Item screenItem
)
{
	return (ControlVDL (screenItem, VDL_HINTEN, 0));
}



/************************************************/
/* Proof user supplied VDLEntry list for a screen. */
/* As we proof, we copy into sys ram. The new VDL */
/* list will be compressed, eg, spaces due to "skipping" ptrs */
/* in the submitted VDL list will be squeezed out. */

/* RETURN: address of sys ram holding proofed ('proven'?) VDl list.*/

#define VDL_BADMASK (0xf8000000|VDL_640SC|VDL_SLIPEN|VDL_SLIPCOMMSEL)
#define VDL_BADCTRLMASK (VDL_NULLAMY|VDL_PALSEL|VDL_S640SEL)

VDLEntry
*ProofVDLEntry (VDLEntry *VDLDataPtr, int32 length, int32 DisplayType,
		int32 numlines)
{
  register int32 Vword;
  register VDLEntry *CLUTptr, *CEnd;
  register VDLEntry *curD, *DEnd;
  int32 rel,p,len,cur_base;
  VDLEntry *post_patch,*patch_ptr,*retvalue,mode,*CLUTptrN;

/*   Superkprintf ("numlines = %ld\n", numlines); */

  retvalue = NULL;
  CLUTptr = VDLDataPtr;

  if (length < 5) {		/* SHL 9306.12 */
    DEVBUG (("VDL Rejected - bad length (%ld)\n", length));
    goto ERR;			/* SHL 9306.12 */
  }
  cur_base = (int32)CLUTptr;	/* (int32)ptr to base of his current VDL */
  curD = (VDLEntry*)SUPER_ALLOCMEM ((sizeof(VDLEntry)*length), MEMTYPE_VRAM|MEMTYPE_DMA);
  if ( curD == NULL ) {
    /* out of memory */
    DEVBUG (("VDL Rejected - unable to allocate VRAM\n"));
    return (VDLEntry *)GRAFERR_NOMEM;
  }
  retvalue = curD;
  DEnd = curD+length;			/* SHL 9306.12 */
  CEnd = CLUTptr+length;		/* SHL 9306.12 */
/*  numlines = 240; */			/* SHL 9306.12 */

  do {
    if (curD+4>DEnd) {
      DEVBUG (("VDL Rejected - system copy of VDL exceeded specified length\n"));
      goto ERR;
    }

    Vword = (int32) *CLUTptr++; /* Get DMA CTRL word */
    if (Vword & VDL_BADMASK) {
      DEVBUG (("VDL Rejected - DMA control word at 0x%lx has illegal/reserved bits set\n",
	       (int32)(CLUTptr-1)));
      goto ERR; /* Any bits set in the BADMASK are currently disallowed */
    }
    len = (Vword & VDL_LEN_MASK) >> VDL_LEN_SHIFT;
    if (len<1||len>34) {		/* SHL 9306.12 */
      DEVBUG (("VDL Rejected - DMA control word at 0x%lx specifies bad length\n",
	       (int32)(CLUTptr-1)));
      DEVBUG (("SC portion of VDL must be at least 1 and no more than 34 words\n"));
      goto ERR;			/* SHL 9306.12 */
    }
    p = (Vword & VDL_LINE_MASK) >> VDL_LINE_SHIFT;
    numlines -= p;			/* SHL 9306.12 */
    if (p==0) {
      Vword |= (numlines<<VDL_LINE_SHIFT);
      numlines = 0;		/* SHL 9306.12 */
    }
    mode = (Vword & VDL_DISPMOD_MASK) >> 23;
    if ( mode>=5) {
      DEVBUG (("VDL Rejected - DMA control word at 0x%lx contains illegal display mode\n",
	       (int32)(CLUTptr-1)));
      goto ERR;  /* 5,6,7 illegal */
    }
    rel = Vword & VDL_RELSEL; /* set if ptr to next CLUT relative */
    /* Word 0 ok. Copy. */
    *curD++ = Vword & ~VDL_RELSEL; /* Do abs, regardless of his scheme*/
    /* copy 2 FB ptrs */
    *curD++ = *CLUTptr++;
    *curD++ = *CLUTptr++;
    /* calc ptr to user's next VDL */
    CLUTptrN =  (VDLEntry *)*CLUTptr++;
    if (rel) {
      CLUTptrN += cur_base/sizeof(VDLEntry) + 4;	/* SHL 9306.12 */
    }
    cur_base = (int32)CLUTptrN;
    post_patch = patch_ptr = curD++;
    if (curD+len>DEnd) {
      DEVBUG (("VDL Rejected - system copy of VDL exceeded specified length\n"));
      goto ERR;
    }
    /* copy pallete as is */
    for (p=0; p< len; p++) {
      VDLEntry v;
      v = *CLUTptr++;
      if (v&VDL_CONTROL) {
	if ((v&VDL_DISPCTRL)==VDL_DISPCTRL) {
	  if (!(v&VDL_BACKGROUND)) {
	    if (v&VDL_BADCTRLMASK) {
	      DEVBUG (("VDL Rejected - display control word at 0x%lx has illegal/reserved flags\n",
		       (int32)(CLUTptr-1)));
	      goto ERR;
	    }
	    v |= _DisplayDISPCTRL[DisplayType]; /* |slipstream_dispctrl; */
	  }
	} else {
	  DEVBUG (("VDL Rejected - AMY control code at 0x%lx\n", (int32)(CLUTptr-1)));
	  goto ERR;
	}
      }
      *curD++ = v;
    }
    if (curD > DEnd) {
      /*
       * Overran kernel-allocated memory region; might have trashed
       * something.  Assume the universe has ended.
       * (If the above checks are solid, this should never happen.)
       */
      Superkprintf ("Error - VDL size overrun - possible security breach!\n");
      Superkprintf ("curD: %08lx  DEnd: %08lx\n", curD, DEnd);
      Superkprintf ("Locking system up in while(1) loop...");
      while (1)
	;
    }
    curD = (VDLEntry *) (((int32) curD + 15) & ~0xF);	/* force 4 word alignment - SHL 9306.12 */
    *patch_ptr = (VDLEntry)curD;
    CLUTptr = CLUTptrN; /*  follow link to next VDL in source */
  } while (CLUTptr<CEnd && curD<DEnd && numlines>0);	/* SHL 9306.12 */
  if (numlines<0) {					/* SHL 9306.12 */
    DEVBUG (("VDL Rejected - VDL list attempts too many lines of display\n"));
    goto ERR;						/* SHL 9306.12 */
  }
  if (numlines) {
    if (curD >= DEnd) {
      DEVBUG (("VDL Rejected - system copy of VDL exceeded specified length\n"));
      goto ERR;	/*  Look down.  */
    }
    if (CLUTptr >= CEnd) {
      DEVBUG (("VDL Rejected - summed VDL line count doesn't tally with declared count.\n"));
      goto ERR;	/*  Look down.  */
    }
  }

  *post_patch = (VDLEntry)GrafBase->gf_VDLPostDisplay;

  return retvalue;


ERR:
  SUPER_FREEMEM ((void *) retvalue, sizeof (VDLEntry) * length);
  return ((VDLEntry *) GRAFERR_PROOF_ERR);
}
