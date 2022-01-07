/* $Id: displayscreen.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
DisplayScreen( Item screenItem0, Item screenItem1 )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DISPLAYSCREEN_, ( screenItem0, screenItem1 ), rval, (int32));
  return rval;
}
