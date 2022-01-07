/* $Id: setwatchdog.c,v 1.6 1994/02/09 01:22:45 limes Exp $ */

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




int32  /* JCR */
SetCEWatchDog( Item bitmapItem, int32 ctr ) /* JCR */
{
  Bitmap *bitmap;
  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }
  if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,bitmapItem)<0) {
      PRINTNOTOWNER (bitmap->bm.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }
  if ( ctr<0  ) {
    return GRAFERR_BADDEADBOLT;
  }
  bitmap->bm_WatchDogCtr = ctr>>4;
  return 0;
}



