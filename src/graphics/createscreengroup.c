/* $Id: createscreengroup.c,v 1.9 1994/04/08 02:09:27 slandrum Exp $ */

/* ************************************************************************
 *
 * Graphics routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 930630 SHL              Split library into seperate source files
 * 930604 JCR              Re wrote SetVRAMPages()
 * 920724 -RJ Mical        Start overhaul
 * 920717 Stephen Landrum  Last edits before July handoff
 *
 * ********************************************************************** */

#include "types.h"

#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"

#include "stdarg.h"
#include "strings.h"
#include "operror.h"

#include "intgraf.h"

#include "device.h"
#include "driver.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"


__swi(_CREATESCREENGROUP)
     Item _CreateScreenGroup (Item *screenitemArray, CreateScreenArgs *stargs);


Item
CreateScreenGroup( Item *screenItemArray, TagArg *targs )
{
  int32 tagc, *tagp;
  Item retvalue;
  int32 *i32ptr, *i32ptr2;
  int32 i, i2, width, height, bigsize, type;
  CreateScreenArgs stargs;
  ubyte **bufarray;

  if (ItemOpened(KernelBase->kb_CurrentTask->t.n_Item, 2)) {
    return GRAFERR_GRAFNOTOPEN;
  }

  retvalue = 0;

  stargs.st_ScreenCount = 2;
  stargs.st_ScreenHeight = GrafBase->gf_DefaultDisplayHeight;
  stargs.st_BitmapCount = 1;
  stargs.st_BitmapWidthArray = NULL;
  stargs.st_BitmapHeightArray = NULL;
  stargs.st_BitmapBufArray = NULL;
  stargs.st_SPORTBankBits = 0;

  stargs.st_DisplayHeight = GrafBase->gf_DefaultDisplayHeight;

  stargs.st_VDLType = VDLTYPE_SIMPLE;
  stargs.st_VDLLengthArray = NULL;
  stargs.st_VDLPtrArray = NULL;

  stargs.st_DisplayType = GrafBase->gf_DefaultDisplayType;
  
  /* CreateScreenGroup() TagArgs
   *   - Display height
   *   - Display width
   *   - Screen count
   *   - Screen height
   *   - Bitmap count
   *   - Bitmap width array
   *   - Bitmap height array
   *   - Bitmap buffer ptr array
   *   - Build VDL of type xxx request
   *   - VDL ptr array
   *   - Control Flags
   */
  tagp = (int32 *)targs;
  if ( tagp ) {
    while ( (tagc = *tagp++) != CSG_TAG_DONE ) {
      switch ( tagc ) {
      case CSG_TAG_DISPLAYHEIGHT:
	stargs.st_DisplayHeight = *tagp++;
	break;
      case CSG_TAG_SCREENCOUNT:
	stargs.st_ScreenCount = *tagp++;
	break;
      case CSG_TAG_SCREENHEIGHT:
	stargs.st_ScreenHeight = *tagp++;
	break;
      case CSG_TAG_BITMAPCOUNT:
	stargs.st_BitmapCount = *tagp++;
	break;
      case CSG_TAG_BITMAPWIDTH_ARRAY:
	stargs.st_BitmapWidthArray = (int32 *)*tagp++;
	break;
      case CSG_TAG_BITMAPHEIGHT_ARRAY:
	stargs.st_BitmapHeightArray = (int32 *)*tagp++;
	break;
      case CSG_TAG_BITMAPBUF_ARRAY:
	stargs.st_BitmapBufArray = (ubyte **)*tagp++;
	break;
      case CSG_TAG_VDLTYPE:
	stargs.st_VDLType = *tagp++;
	break;
      case CSG_TAG_VDLPTR_ARRAY:
	stargs.st_VDLPtrArray = (VDLEntry **)*tagp++;
	break;
      case CSG_TAG_VDLLENGTH_ARRAY:
	stargs.st_VDLLengthArray = (int32 *)*tagp++;
	break;
      case CSG_TAG_SPORTBITS:
	stargs.st_SPORTBankBits = *tagp++;
	break;
      case CSG_TAG_DISPLAYTYPE:
	stargs.st_DisplayType = *tagp++;
	break;
      default:
	retvalue = GRAFERR_BADTAG;
	goto DONE;
      }
    }
  }

#if(0)
#define DEB kprintf
DEB("stargs.st_DisplayHeight=$%lx ", (unsigned long)(stargs.st_DisplayHeight));
DEB("stargs.st_ScreenCount=$%lx ", (unsigned long)(stargs.st_ScreenCount));
DEB("stargs.st_ScreenHeight=$%lx ", (unsigned long)(stargs.st_ScreenHeight));
DEB("\n");
DEB("stargs.st_BitmapCount=$%lx ", (unsigned long)(stargs.st_BitmapCount));
DEB("stargs.st_BitmapWidthArray=$%lx ", (unsigned long)(stargs.st_BitmapWidthArray));
DEB("stargs.st_BitmapHeightArray=$%lx ", (unsigned long)(stargs.st_BitmapHeightArray));
DEB("\n");
DEB("stargs.st_BitmapBufArray=$%lx ", (unsigned long)(stargs.st_BitmapBufArray));
DEB("stargs.st_VDLType=$%lx ", (unsigned long)(stargs.st_VDLType));
DEB("stargs.st_VDLPtrArray=$%lx ", (unsigned long)(stargs.st_VDLPtrArray));
DEB("\n");
DEB("stargs.st_SPORTBankBits=$%lx ", (unsigned long)(stargs.st_SPORTBankBits));
DEB("\n");
#undef DEB
#endif

  if (stargs.st_DisplayType == 0) {
    stargs.st_DisplayType = GrafBase->gf_DefaultDisplayType;
  }

  if ((stargs.st_DisplayType>31) || !((1L<<stargs.st_DisplayType)&GrafBase->gf_DisplayTypeMask)) {
    retvalue = GRAFERR_BADDISPLAYTYPE;
    goto DONE;
  }

  if ( (stargs.st_DisplayHeight < 1) 
      || (stargs.st_DisplayHeight > _DisplayHeight[stargs.st_DisplayType]) ) {
    /* bad height */
    retvalue = GRAFERR_BADDISPDIMS;
    goto DONE;
  }

  if ( stargs.st_VDLPtrArray && (stargs.st_VDLLengthArray == NULL) ) {
    retvalue = GRAFERR_VDL_LENGTH;
    goto DONE;
  }

  if ( stargs.st_ScreenCount < 1 ) {
    /* bad screen count */
    retvalue = GRAFERR_BADDISPDIMS;
    goto DONE;
  }

  if ( stargs.st_ScreenHeight < stargs.st_DisplayHeight ) {
    /* bad screen height */
    retvalue = GRAFERR_BADDISPDIMS;
    goto DONE;
  }

  if ( ( stargs.st_BitmapCount < 0 ) || ( (stargs.st_BitmapCount > 1) 
					 && (stargs.st_BitmapHeightArray == NULL) ) ) {
    /* bad bitmap setup */
    retvalue = GRAFERR_BADBITMAPSPEC;
    goto DONE;
  }

  stargs.st_bufarrayallocatedflag = FALSE;
  if ( stargs.st_BitmapBufArray == NULL ) {
    /* No bitmap buffers?  We must allocate bitmap buffers for the caller
     * out of the caller's memory space before we slip below the fence 
     */
    bufarray = (ubyte **)USER_ALLOCMEM( stargs.st_ScreenCount
				       * stargs.st_BitmapCount * sizeof( ubyte * ), 0 );
    if ( bufarray == NULL ) {
      /* out of memory */
      retvalue = GRAFERR_NOMEM;
      goto DONE;
    }
    stargs.st_bufarrayallocatedflag = TRUE;
    stargs.st_BitmapBufArray = bufarray;

    for ( i = 0; i < stargs.st_ScreenCount; i++ ) {
      i32ptr = stargs.st_BitmapHeightArray;
      i32ptr2 = stargs.st_BitmapWidthArray;
      height = stargs.st_ScreenHeight;
      width = _DisplayWidth[stargs.st_DisplayType];
      for ( i2 = 0; i2 < stargs.st_BitmapCount; i2++ ) {
	if ( i32ptr ) height = *i32ptr++;
	if ( i32ptr2 ) width = *i32ptr2++;
	bigsize = width * 2 * height;
	type = MEMTYPE_VRAM | MEMTYPE_CEL;
	if ( stargs.st_SPORTBankBits ) {
	  /* The presence of SPORTBankBits implies that 
	   * SPORT transfers with this bitmap will take place, 
	   * so the correct SPORT care must be taken
	   */
	  bigsize = ( bigsize + (GrafBase->gf_VRAMPageSize-1) )
	    / GrafBase->gf_VRAMPageSize;
	  bigsize *= GrafBase->gf_VRAMPageSize;
	  type |= stargs.st_SPORTBankBits | MEMTYPE_STARTPAGE;
	}
	*bufarray = (ubyte *)USER_ALLOCMEM( bigsize, type );

DEBUGGRAF(("--- width=%ld ", (unsigned long)(width)));
DEBUGGRAF(("height=%ld ", (unsigned long)(height)));
DEBUGGRAF(("bigsize=%ld ", (unsigned long)(bigsize)));
DEBUGGRAF(("($%lx) ", (unsigned long)(bigsize)));
DEBUGGRAF(("type=$%lx ", (unsigned long)(type)));
DEBUGGRAF(("*bufarray=$%lx ", (unsigned long)(*bufarray)));
DEBUGGRAF(("\n"));

	if ( *bufarray == NULL ) {
	  /* out of memory */
	  retvalue = GRAFERR_NOMEM;
	  goto DONE;
	}
	bufarray++;
      }
    }
  }

/*??? Check that bitmap Widths have valid values */

  retvalue = _CreateScreenGroup( screenItemArray, &stargs );

DONE:
  if (stargs.st_bufarrayallocatedflag) {
    FREEMEM (stargs.st_BitmapBufArray, stargs.st_ScreenCount*stargs.st_BitmapCount*sizeof(ubyte*));
  }
  return( retvalue );
}


Err
DeleteScreenGroup (Item sgi)
{
  ScreenGroup *sgptr;
  Item si, bi;
  Screen *sptr;
  Bitmap *bptr;

  sgptr = (ScreenGroup*)LookupItem (sgi);
  if ((int32)sgptr < 0) {
    return (Err)sgptr;
  }
  RemoveScreenGroup (sgi);	/* remove screengroup from active groups */
  for (sptr = (Screen*)FIRSTNODE(&sgptr->sg_ScreenList); ISNODE(&sgptr->sg_ScreenList,sptr);
       sptr = (Screen*)FIRSTNODE(&sgptr->sg_ScreenList)) {
    si = sptr->scr.n_Item;
    for (bptr = (Bitmap*)FIRSTNODE(&sptr->scr_BitmapList); ISNODE(&sptr->scr_BitmapList,bptr);
	 bptr = (Bitmap*)FIRSTNODE(&sptr->scr_BitmapList)) {
      bi = bptr->bm.n_Item;
      if (bptr->bm_SysMalloc) {		/* check to make sure we allocated bitmap memory */
	FREEMEM (bptr->bm_Buffer, bptr->bm_Width*bptr->bm_Height*2);	/* Free bitmap memory */
      }
      DeleteItem (bi);	/* delete the bitmap Item */
    }
    DeleteItem (sptr->scr_VDLItem);	/* delete the VDL Item */
    DeleteItem (si);	/* delete the screen Item */
  }
  return DeleteItem (sgi);	/* delete the screengroup Item (Whew!) */
}




