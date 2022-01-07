/* $Id: querygraphics.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


Err
QueryGraphics ( int32 tag, void *ret )
{
  Err r;
  CALLFOLIORET (GrafBase, _QUERYGRAPHICS_, ( tag, ret ), r, (Err));
  return r;
}
