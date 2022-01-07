/* $Id: setscreencolors.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
SetScreenColors( Item screenItem, uint32 *entries, int32 count )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _SETSCREENCOLORS_, ( screenItem, entries, count ), rval, (int32));
  return rval;
}
