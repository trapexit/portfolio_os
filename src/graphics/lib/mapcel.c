/* $Id: mapcel.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


void
MapCel( CCB *ccb, Point *quad )
{
	CALLFOLIO (GrafBase, _MAPSPRITE_, (ccb, quad));
}
