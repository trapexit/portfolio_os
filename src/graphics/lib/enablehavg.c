/* $Id: enablehavg.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
EnableHAVG( Item screenItem )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _ENABLEHAVG_, ( screenItem ), rval, (int32));
  return rval;
}
