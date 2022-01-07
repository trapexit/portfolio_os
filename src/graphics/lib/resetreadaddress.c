/* $Id: resetreadaddress.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
ResetReadAddress( Item bitmapItem )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _RESETREADADDRESS_, ( bitmapItem ), rval, (int32));
  return rval;
}
