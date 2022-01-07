/* $Id: drawtext16.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
DrawText16( GrafCon *gcon, Item bitmapItem, uint16 *text )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DRAWTEXT16_, ( gcon, bitmapItem, text ), rval, (int32));
  return rval;
}
