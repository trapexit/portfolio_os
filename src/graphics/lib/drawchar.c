/* $Id: drawchar.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
DrawChar( GrafCon *gcon, Item bitmapItem, uint32 character )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DRAWCHAR_, ( gcon, bitmapItem, character ), rval, (int32));
  return rval;
}
