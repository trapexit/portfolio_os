/* $Id: setbgpen.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


void
SetBGPen( GrafCon *gc, Color c )
{
  CALLFOLIO (GrafBase, _SETBGPEN_, ( gc, c ));
}
