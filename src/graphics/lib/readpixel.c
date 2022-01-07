/* $Id: readpixel.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


Color
ReadPixel( Item bitmapItem, GrafCon *gc, Coord x, Coord y )
{
	Color i;

	CALLFOLIORET (GrafBase, _READPIXEL_, (bitmapItem, gc, x, y), i, (Color));
	return i;
}
