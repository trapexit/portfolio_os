/* $Id: setfgpen.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


void
SetFGPen( GrafCon *gc, Color c )
{
  CALLFOLIO (GrafBase, _SETFGPEN_, ( gc, c ));
}
