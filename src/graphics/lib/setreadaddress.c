/* $Id: setreadaddress.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
SetReadAddress( Item bitmapItem, ubyte *buffer, int32 width )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _SETREADADDRESS_, ( bitmapItem, buffer, width ), rval, (int32));
  return rval;
}
