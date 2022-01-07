/* $Id: drawcels.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
DrawCels( Item bitmapItem, CCB *ccb )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DRAWCELS_, ( bitmapItem, ccb ), rval, (int32));
  return rval;
}
