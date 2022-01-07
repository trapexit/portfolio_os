/* $Id: disablevavg.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
DisableVAVG( Item screenItem )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DISABLEVAVG_, ( screenItem ), rval, (int32));
  return rval;
}
