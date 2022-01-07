/* $Id: moveto.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


void
MoveTo( GrafCon *gc, Coord x, Coord y )
{
  CALLFOLIO (GrafBase, _MOVETO_, ( gc, x, y ));
}
