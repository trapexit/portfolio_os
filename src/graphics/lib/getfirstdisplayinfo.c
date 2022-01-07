/* $Id: getfirstdisplayinfo.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


DisplayInfo* GetFirstDisplayInfo (void)
{
  DisplayInfo* rval;
  CALLFOLIORET (GrafBase, _GETFIRSTDISPLAYINFO_, ( ), rval, (DisplayInfo*));
  return rval;
}
