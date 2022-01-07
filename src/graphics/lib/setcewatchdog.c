/* $Id: setcewatchdog.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
SetCEWatchDog( Item bitmapItem, int32 db_ctr )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _SETCEWATCHDOG_, ( bitmapItem, db_ctr ), rval, (int32));
  return rval;
}
