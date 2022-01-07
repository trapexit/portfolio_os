/* $Id: setvdl.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


Item
SetVDL( Item screenItem, Item vdlItem )
{
  Item rval;
  CALLFOLIORET (GrafBase, _SETVDL_, ( screenItem, vdlItem ), rval, (Item));
  return rval;
}
