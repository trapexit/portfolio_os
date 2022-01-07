/* $Id: setcliporigin.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
SetClipOrigin( Item bitmapItem, int32 x, int32 y )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _SETCLIPORIGIN_, ( bitmapItem, x, y ), rval, (int32));
  return rval;
}
