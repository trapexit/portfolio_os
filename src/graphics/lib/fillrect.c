/* $Id: fillrect.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
FillRect( Item bitmapItem, GrafCon *gc, Rect *r )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _FILLRECT_, ( bitmapItem, gc, r ), rval, (int32));
  return rval;
}
