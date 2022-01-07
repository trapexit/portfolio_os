/* $Id: setclipwidth.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
SetClipWidth( Item bitmapItem, int32 clipWidth )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _SETCLIPWIDTH_, ( bitmapItem, clipWidth ), rval, (int32));
  return rval;
}
