
/******************************************************************************
**
**  $Id: SetRectf16.c,v 1.5 1994/10/05 19:17:02 vertex Exp $
**
**  Lib3DO routine to set a frac16 rect from integer coords.
**
******************************************************************************/


#include "utils3do.h"


void SetRectf16( Rectf16 *r, Coord left, Coord top, Coord right, Coord bottom )
{
	r->rectf16_XLeft	= Convert32_F16 (left);
	r->rectf16_YTop		= Convert32_F16 (top);
	r->rectf16_XRight	= Convert32_F16 (right);
	r->rectf16_YBottom	= Convert32_F16 (bottom);
}
