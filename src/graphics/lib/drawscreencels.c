/* $Id: drawscreencels.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
DrawScreenCels( Item screenItem, CCB *ccb )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DRAWSCREENCELS_, ( screenItem, ccb ), rval, (int32));
  return rval;
}
