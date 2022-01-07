/* $Id: getpixeladdress.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


void *
GetPixelAddress( Item bitmapItem, Coord x, Coord y )
{
	void * i;

	CALLFOLIORET (GrafBase, _GETPIXELADDRESS_, (bitmapItem, x, y), i, (void *));
	return i;
}
