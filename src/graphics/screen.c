/* $Id: screen.c,v 1.32 1994/12/21 21:13:02 ewhac Exp $ */

/* *************************************************************************
 *
 * Screen routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 930830 SHL              Split CreateBitmap out of CreateScreenGroup
 * 930706 SHL              Commented out pre-red support
 * 930609 -JCR             Added ALLOC_MEM for bitmaps if not passed in.
 * 930421 -JCR             Completed VDL struct usage, added user VDL verify hook.
 * 921118 -RJ              Changed CCBCTL0 default to include CFBDSUB
 * 921010 -RJ Mical        Created this file!
 *
 * ********************************************************************** */


/***************************************************************\
* Header files                                                 *
\***************************************************************/

#define SHLDBUG(x) /* Superkprintf x */

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
#include "kernelnodes.h"
#include "super.h"

#include "intgraf.h"
#include "overlay.h"

#include "stdarg.h"
#include "strings.h"


extern int32 ValidateMem(Task *t,uint8 *p,int32 size);
extern int32 SpliceOverlay(struct Overlay *ov, int32 op, struct VDL *, struct VDL *);
extern void PostSpliceOverlay(struct Overlay *ov, int32);


extern MemHdr *vram;    /* pts to systems MemHdr for VRAM */

extern int32		_forcecreatevdlflag, _suppress_MCTL;
extern struct Overlay	*ActiveOverlay;
extern uint32		_tripaddress;


/* template tag list for bitmap creation */
/* !!! DO NOT CHANGE THE ORDER OF ELEMENTS IN TEMPLATE WITHOUT CHECKING CODE BELOW !!! */
TagArg _cbmta[] = {
	{ CBM_TAG_WIDTH,	0 },
	{ CBM_TAG_HEIGHT,	0 },
	{ CBM_TAG_BUFFER,	0 },
	{ CBM_TAG_DONE,		0 },
};



/**********************************************************/
Item
realCreateScreenGroup
(
Item			*screenItemArray,
CreateScreenArgs	*stargs
)
{
  VDL		*VDL_p;
  VDLEntry	*vdl, *vdl2, *vdl_PatchPost, **user_vdlList;
  VDLEntry	displayControl;
  ScreenGroup	*sgptr;
  Screen	*screen;
  Bitmap	*bitmap;
  Item		*iptr;
  Item		sgitem, vdlitem;
  Item		bitmapitem;
  Item		retvalue;
  int32		currentHeight, width, *user_vdlLengths;
  int32		*widthptr, *heightptr;
  int32		vdl_len, total_vdl_len, i, i2, i3, size, color;
  ubyte		**bufptr, **bufptr2, *zbufptr, *prevbufptr;

  uint32 savestate;

  retvalue = 0;
  sgitem = 0;

  sgitem = SuperCreateItem( MKNODEID(NODE_GRAPHICS,TYPE_SCREENGROUP), NULL );
  if ( (int32)sgitem < 0 ) {
    /* couldn't allocate screen group item */
    retvalue = (int32)sgitem;
    goto DONE;
  }

  /* Initialize the ScreenGroup item */
  sgptr = (ScreenGroup *)LookupItem( sgitem );

  InitList (&sgptr->sg_ScreenList, "ScreenList");

  /*???   sgptr->sg_DisplayHeight = sgptr->sg_CurrentDisplayHeight */
  sgptr->sg_DisplayHeight = stargs->st_DisplayHeight;
  sgptr->sg_ScreenHeight = stargs->st_ScreenHeight;
  sgptr->sg_Add_SG_Called = 0;

  retvalue = SuperValidateMem (CURRENTTASK, (uint8*)screenItemArray,
			       stargs->st_ScreenCount*sizeof(Item));
  if (retvalue<0) {
    goto DONE;
  }
  iptr = screenItemArray;
  bufptr = stargs->st_BitmapBufArray;
  stargs->st_BitmapCount = 1; /* DDD Force, for 1st release. JCR */
  /* Point to user-supplied VDL data/length arrays */
  user_vdlList = stargs->st_VDLPtrArray;
  user_vdlLengths = stargs->st_VDLLengthArray;
  total_vdl_len = 0;

  /* MAJOR "FOR EACH SCREEN" LOOP */
  for ( i = 0; i < stargs->st_ScreenCount; i++ ) {
    *iptr = SuperCreateItem (MKNODEID(NODE_GRAPHICS,TYPE_SCREEN), NULL);
    if( *iptr < 0 ) {
      retvalue = *iptr; /* couldn't allocate screen item */
      goto DONE;
    }
    screen = (Screen *)LookupItem( *iptr );
    screen->scr_ScreenGroupPtr = sgptr;
    iptr++;

    AddHead (&sgptr->sg_ScreenList, (Node*)screen);

    /* DO VDL FOR THIS SCREEN */
    if ( user_vdlList ) {
      /* USER */
      /* User is supplying HIS OWN VDL. Verify, & copy ->sys RAM*/
      int32 len;
      VDLEntry *user_vdl;
      user_vdl = *user_vdlList++;
      len = *user_vdlLengths++;
      if ( user_vdl==NULL || len<4 ) {	/* Proof will look closer at len */
	retvalue = GRAFERR_BADTAGVAL;
	goto DONE;
      }
      /* Proof user's VDLEntry list, and return 0 if OK. */
      /* set vdl to pt. to VDLEntry data, as do 4 cases below */
      vdl = ProofVDLEntry(user_vdl,len,stargs->st_DisplayType,stargs->st_DisplayHeight);
      if ((int32)vdl < 0) {
	retvalue = (Item)vdl;
	goto DONE;
      }
      total_vdl_len = len;
    } else {	/* else user has NOT supplied his own VDL. Default. */
      switch ( stargs->st_VDLType ) {
      case VDLTYPE_FULL:
/*	size = 4 + 1 + 1; */
/*???*/
size = 8;
	bufptr2 = bufptr;
	widthptr = stargs->st_BitmapWidthArray;
	heightptr = stargs->st_BitmapHeightArray;
	vdl = NULL;
	vdl_PatchPost = NULL;
	for ( i2 = 0; i2 < stargs->st_BitmapCount; i2++ ) {
	  if ( heightptr ) currentHeight = *heightptr++;
	  else currentHeight = stargs->st_ScreenHeight;
	  vdl_len = size * currentHeight + 32;
/*	  total_vdl_len += vdl_len; */
total_vdl_len = vdl_len;
	  vdl2 = (VDLEntry *)SUPER_ALLOCMEM ((sizeof(VDLEntry)*vdl_len),
					     MEMTYPE_VRAM | MEMTYPE_DMA );
	  if ( vdl2 == NULL ) {
	    /* out of memory */
	    retvalue = GRAFERR_NOMEM;
	    goto DONE;
	  }
	  zbufptr = *bufptr2++;
	  prevbufptr = zbufptr;

/*???                       if ( widthptr ) width = (*widthptr++) * 2;*/
/*???                       else width = GrafBase->gf_DefaultDisplayWidth * 2;*/

	  if ( widthptr ) width = (*widthptr++);
	  else width = _DisplayWidth[stargs->st_DisplayType];

/*??? Must handle starting on odd-line boundaries */
	  for ( i3 = 0; i3 < currentHeight; i3++ ) {
	    /* Assign the address of the first to vdl, else
	     * if we're beyond the first entry, patch the
	     * previous entry to point to this one
	     */
	    if ( vdl == NULL ) {
	      vdl = vdl2;
	    } else {
	      *vdl_PatchPost = (VDLEntry)vdl2;
	    }
	    /* Build this vdl entry */
/*???*/
	    if ( i3 == 0 ) {
	      displayControl = VDL_ENVIDDMA | VDL_LDCUR | VDL_LDPREV
		| ( (32+1+1) << VDL_LEN_SHIFT )
		  | (1 << VDL_LINE_SHIFT );
	    } else {
	      displayControl = VDL_ENVIDDMA | VDL_LDCUR | VDL_LDPREV
		| ( (2) << VDL_LEN_SHIFT )
		  | (1 << VDL_LINE_SHIFT );
	    }
	    {
	      ValidWidth	*vw;

	      if ((vw = FindValidWidth (width, BMF_DISPLAYABLE)) == 0) {
		retvalue = GRAFERR_VDLWIDTH;
		goto DONE;
	      }
	      displayControl &= ~VDL_DISPMOD_MASK;
	      displayControl |= vw->vw_DispMods;
	    }
	    *vdl2++ = displayControl;
	    /* Link previous to the data line before this one */
	    *vdl2++ = (VDLEntry)zbufptr;
	    *vdl2++ = (VDLEntry)prevbufptr;
	    prevbufptr = zbufptr;
	    if (( i3 & 1 ) == 0 ) zbufptr += 2;
	    else zbufptr = zbufptr - 2 + width * 2 * 2;
	    /* Save a pointer to the field to be patched */
	    vdl_PatchPost = vdl2++;

	    *vdl2++ = DEFAULT_DISPCTRL
	      | _DisplayDISPCTRL[stargs->st_DisplayType]; /*  | slipstream_dispctrl; */

	    if ( i3 == 0 ) {
	      for ( size = 0; size < 32; size++ ) {
		color = (ubyte)(( size * 255 ) / 31);
		*vdl2++ = MakeCLUTColorEntry (size, color, color, color);
	      }
	      *vdl2++ = MakeCLUTBackgroundEntry (0, 0, 0);
	      *vdl2++ = VDL_NULLVDL;
	      *vdl2++ = VDL_NULLVDL;
	    } else {
	      *vdl2++ = VDL_NULLVDL;
	      *vdl2++ = VDL_NULLVDL;
	      *vdl2++ = VDL_NULLVDL;
	    }
	  }
	}

	/* point the last vdl to the end vdl entry */
	if ( vdl_PatchPost ) {
	  *vdl_PatchPost = (VDLEntry)GrafBase->gf_VDLPostDisplay;
	}
	break;
      case VDLTYPE_COLOR:
	/* type not yet implemented */
	retvalue = GRAFERR_NOTYET;
	goto DONE;
	break;
      case VDLTYPE_ADDRESS:
	/* type not yet implemented */
	retvalue = GRAFERR_NOTYET;
	goto DONE;
	break;
      case VDLTYPE_SIMPLE:
	size = 4 + 32 + 1 + 1;
	bufptr2 = bufptr;
	widthptr = stargs->st_BitmapWidthArray;
	vdl = NULL;
	vdl_PatchPost = NULL;

	for ( i2 = 0; i2 < stargs->st_BitmapCount; i2++ ) {
	  if ( widthptr ) {
	    width = (*widthptr++);
	  } else {
	    width = _DisplayWidth[stargs->st_DisplayType];
	  }

	  vdl_len = size;
/*	  total_vdl_len += vdl_len; */
total_vdl_len = vdl_len;
	  vdl2 = (VDLEntry *)SUPER_ALLOCMEM ((sizeof(VDLEntry)*size), MEMTYPE_VRAM|MEMTYPE_DMA);
	  if ( vdl2 == NULL ) {
	    /* out of memory */
	    retvalue = GRAFERR_NOMEM;
	    goto DONE;
	  }
	  /* Assign the address of the first to vdl, else
	   * if we're beyond the first bitmap, patch the previous
	   * bitmap to point to this one
	   */
	  if ( i2 == 0 ) {
	    vdl = vdl2;
	  } else {
	    *vdl_PatchPost = (VDLEntry)vdl2;
	  }

	  /* Build this vdl entry */
	  /*??? Shouldn't use 240, should use height */
	  displayControl = VDL_ENVIDDMA
	    | VDL_LDCUR | VDL_LDPREV
	      | ( (size-4) << VDL_LEN_SHIFT )
		| (stargs->st_DisplayHeight << VDL_LINE_SHIFT );
	  {
	    ValidWidth	*vw;

	    if ((vw = FindValidWidth (width, BMF_DISPLAYABLE)) == 0) {
	      retvalue = GRAFERR_VDLWIDTH;
	      goto DONE;
	    }
	    displayControl &= ~VDL_DISPMOD_MASK;
	    displayControl |= vw->vw_DispMods;
	  }
	  *vdl2++ = displayControl;

	  *vdl2++ = (VDLEntry)(*bufptr2);
	  *vdl2++ = (VDLEntry)(*bufptr2++);

	  /* Save a pointer to the field to be patched */
	  vdl_PatchPost = vdl2++;

	  *vdl2++ = DEFAULT_DISPCTRL
	    | _DisplayDISPCTRL[stargs->st_DisplayType];  /* | slipstream_dispctrl; */

	  for ( i3 = 0; i3 < 32; i3++ ) {
/*???                           color = 16 + (( i3 * (235-16) ) / 31);*/
	    color = (int32)(( i3 * 255 ) / 31);
	    *vdl2++ = (VDLEntry) MakeCLUTColorEntry (i3, color, color, color);
	  }
	  *vdl2++ = MakeCLUTBackgroundEntry (0, 0, 0);
	}

	/* point the last vdl to the end vdl entry */
	if ( vdl_PatchPost ) {
	  *vdl_PatchPost = (VDLEntry)GrafBase->gf_VDLPostDisplay;
	}
	break;
      case VDLTYPE_DYNAMIC:
	/* type not yet implemented */
	retvalue = GRAFERR_NOTYET;
	goto DONE;
	break;
      default:
	/* Illegal VDL type. */
	retvalue = GRAFERR_BADTAGVAL;
	goto DONE;
	break;
      } /* end of "switch st_args type */
    }

    /* JCR */
    /* vdl now pts to a valid concat. of VDLEntry's in sys RAM. */
    /* total_vdl_len = size in words */

    /* Create a struct to hold this, & interlink it w/ screen*/
    savestate = Disable();
    _forcecreatevdlflag = TRUE;
    vdlitem = SuperCreateItem(MKNODEID(NODE_GRAPHICS,TYPE_VDL), NULL );
    _forcecreatevdlflag = FALSE;
    Enable (savestate);
    if ( (int32)vdlitem < 0 ) {
      /* couldn't allocate VDL item */
      retvalue = (int32)vdlitem;
      goto DONE;
    }

    /* Initialize the VDL */
    VDL_p = (VDL *)LookupItem( vdlitem );
    VDL_p->vdl_ScreenPtr = screen;
    screen->scr_VDLPtr = VDL_p; /* Back Acha */
    screen->scr_VDLItem = vdlitem; /* for SetVDL(). JCR */
    VDL_p->vdl_Type = stargs->st_VDLType;
    VDL_p->vdl_DisplayType = stargs->st_DisplayType;
    /* Each of the 4 cases has left vdl pointing to actual VDLEntry data*/
    /* (Or code for user_VDLdata did). */
    /* and has also set total_vdl_len. */
    VDL_p->vdl_DataPtr = vdl;  /* for SUPER_FREEMEM() */
    /* store size IN BYTES, for SUPER_FREEMEM() */
    VDL_p->vdl_DataSize = total_vdl_len*sizeof(VDLEntry);
    screen->scr_VDLType = stargs->st_VDLType;
    VDL_p->vdl_Height = stargs->st_DisplayHeight;
    VDL_p->vdl_Offset = (_DisplayHeight[VDL_p->vdl_DisplayType]-VDL_p->vdl_Height)/2
      +_DisplayOffset[VDL_p->vdl_DisplayType];

/*****************************************************************/
        /* END OF VDL PROCESSING FOR THIS SCREEN. */
        /* DO BITMAP STUFF FOR THIS SCREEN. */
/*****************************************************************/

    InitList( &screen->scr_BitmapList, "ScreenBitmapList" );

    heightptr = stargs->st_BitmapHeightArray;
    widthptr = stargs->st_BitmapWidthArray;
    currentHeight = 0;
    for ( i2 = 0; i2 < stargs->st_BitmapCount; i2++ ) {
      TagArg ta[sizeof(_cbmta)/sizeof(_cbmta[0])];
      memcpy (ta, _cbmta, sizeof(_cbmta));


      /* JCR */
      if ( widthptr ) ta[0].ta_Arg = (void*)*widthptr++;
      else ta[0].ta_Arg = (void*)_DisplayWidth[stargs->st_DisplayType];
      if ( heightptr ) ta[1].ta_Arg = (void*)*heightptr++;
      else ta[1].ta_Arg = (void*)stargs->st_ScreenHeight;
      /*JCR: NOTE, st_BitmapBufArray could be NULL, so we ALLOC here */
      if (bufptr == (ubyte **)NULL) {
	retvalue = GRAFERR_INTERNALERROR;
	goto DONE;
      } else {
	ta[2].ta_Arg = (void*)*bufptr++;
      }
      bitmapitem = SuperCreateItem (MKNODEID(NODE_GRAPHICS,TYPE_BITMAP), ta);
      if ( bitmapitem < 0 ) {
	retvalue = bitmapitem; /* couldn't allocate bitmap item */
	goto DONE;
      }
      bitmap = (Bitmap *)LookupItem( bitmapitem );

      screen->scr_TempBitmap = bitmap;
      AddHead (&screen->scr_BitmapList, (Node*)bitmap);

      bitmap->bm_VerticalOffset = currentHeight;
      if (stargs->st_bufarrayallocatedflag) {
	bitmap->bm_SysMalloc = TRUE;
      }
      currentHeight += bitmap->bm_Height;
    }
  } /* END OF MAJOR "FOR EACH SCREEN" LOOP */

  retvalue = sgitem;

DONE:
  if ( retvalue < 0 ) {
    if ( sgitem > 0 ) SuperDeleteItem( sgitem );	/* 9606.15 - SHL */
  }
  return( retvalue );
}  /* end of realCreateScreenGroup() */



int32
DisplayScreen( Item ScreenItem0, Item ScreenItem1 )
{
  Screen	*scr0, *scr1;
  ScreenGroup	*sg;
  int32		bank;
  uint32	oldints;

  bank = 0;
  scr0 = (Screen *)CheckItem( ScreenItem0, NODE_GRAPHICS, TYPE_SCREEN );
  if ( ScreenItem1 ) {
    scr1 = (Screen *)CheckItem( ScreenItem1, NODE_GRAPHICS, TYPE_SCREEN );
  } else {
    scr1 = scr0;
  }
  if (( scr0 == NULL ) || ( scr1 == NULL )) {
    /* invalid screen items */
    return GRAFERR_BADITEM;
  }
  if (scr0->scr.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,scr0->scr.n_Item)<0) {
      PRINTNOTOWNER (scr0->scr.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }
  if (scr1->scr.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,scr1->scr.n_Item)<0) {
      PRINTNOTOWNER (scr1->scr.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

  if (!scr0->scr_ScreenGroupPtr) {
    return GRAFERR_INTERNALERROR;
  }
  if (!CheckItem(scr0->scr_ScreenGroupPtr->sg.n_Item,NODE_GRAPHICS,TYPE_SCREENGROUP)) {
    return GRAFERR_SGNOTINUSE;
  }
  sg = scr0->scr_ScreenGroupPtr;
  if ( sg != scr1->scr_ScreenGroupPtr ) {
    /* screen items must be in the same screen group */
    return GRAFERR_MIXEDSCREENS;
  }

  if (scr0->scr_VDLPtr->vdl_DisplayType != scr1->scr_VDLPtr->vdl_DisplayType) {
    return GRAFERR_MIXEDDISPLAYS;
  };

  if ((GrafBase->gf_CurrentVDLEven->vdl_DisplayType == DI_TYPE_PAL2)
      && (scr0->scr_VDLPtr->vdl_DisplayType == DI_TYPE_PAL1)) {
    resync_displaywidth ();
  }

  if (ActiveOverlay)
    /*
     * Must be done before installing, and is too lengthy to be in the
     * critical section.
     */
    bank = SpliceOverlay
	    (ActiveOverlay, SPLICEOP_NEW, scr0->scr_VDLPtr, scr1->scr_VDLPtr);

  oldints = Disable ();	/*  The following operations must be atomic.  */

  /*
   * Mark VDLs currently in use for disposal.
   */
  addVDLtofree (GrafBase->gf_CurrentVDLEven);
  if (GrafBase->gf_CurrentVDLEven != GrafBase->gf_CurrentVDLOdd)
    addVDLtofree (GrafBase->gf_CurrentVDLOdd);

  /*
   * New VDLs might have been in the disposal queue.  Remove them.
   */
  removeVDLtofree (scr0->scr_VDLPtr);
  if (scr0->scr_VDLPtr != scr1->scr_VDLPtr)
    removeVDLtofree (scr1->scr_VDLPtr);

  /* graphicsFIRQ() will load these 2 addresses for HW */
  GrafBase->gf_CurrentVDLEven = scr0->scr_VDLPtr;
  GrafBase->gf_CurrentVDLOdd  = scr1->scr_VDLPtr;

  if (ActiveOverlay)
    PostSpliceOverlay (ActiveOverlay, bank);

  Enable (oldints);

  return 0;
}


void
resync_displaywidth (void)
{
  Superkprintf ("trigger\n");
  _suppress_MCTL = 8;
}


int32
AddScreenGroup( Item ScreenGroupItem, TagArg *targs )
/* Adds the screenGroup to the display.  After this call, the screens
 * of the screen group are made visible with calls to DisplayScreen().
 *
 * The targs argument allows the caller to specify initial values
 * such as:
 *         * Vertical offset
 *         * Depth from front (0 means frontmost, negative value
 *           means backmost)
 */
{
  ScreenGroup *sg;

  sg = (ScreenGroup *)CheckItem (ScreenGroupItem, NODE_GRAPHICS, TYPE_SCREENGROUP);
  if( !sg ) {
    return GRAFERR_BADITEM;	/* bad scr group Item number */
  }
  if (sg->sg.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,sg->sg.n_Item)<0) {
      PRINTNOTOWNER (sg->sg.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }
  if (sg->sg_Add_SG_Called) {
    return GRAFERR_SGINUSE; /* Called TWICE */
  }
  sg->sg_Add_SG_Called = 1;  /* Prove he called this routine. */
  return( 0 );
}



int32
RemoveScreenGroup( Item ScreenGroupItem )
/* Removes the screen group from the display.  After this call,
 * the screens of the group will not be visible.
 */
{
  ScreenGroup *sg;
  sg = (ScreenGroup *)CheckItem (ScreenGroupItem, NODE_GRAPHICS, TYPE_SCREENGROUP );
  if( !sg ) {
    return GRAFERR_BADITEM;  /* bad scr group Item number */
  }
  if (sg->sg.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,sg->sg.n_Item)<0) {
      PRINTNOTOWNER (sg->sg.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }
  if (!sg->sg_Add_SG_Called) {
    /* This SG wasn't previously added. */
    return GRAFERR_SGNOTINUSE;
  }
  return( 0 );
}
