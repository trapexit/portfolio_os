/* $Id: drawtext8.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
DrawText8( GrafCon *gcon, Item bitmapItem, uint8 *text )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DRAWTEXT8_, ( gcon, bitmapItem, text ), rval, (int32));
  return rval;
}
