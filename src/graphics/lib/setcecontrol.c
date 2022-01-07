/* $Id: setcecontrol.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
SetCEControl( Item bitmapItem, int32 controlWord, int32 controlMask )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _SETCECONTROL_, ( bitmapItem, controlWord, controlMask ), rval, (int32));
  return rval;
}
