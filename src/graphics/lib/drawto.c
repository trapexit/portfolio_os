/* $Id: drawto.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
DrawTo( Item bitmapItem, GrafCon *grafcon, Coord x, Coord y )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DRAWTO_, ( bitmapItem, grafcon, x, y ), rval, (int32));
  return rval;
}
