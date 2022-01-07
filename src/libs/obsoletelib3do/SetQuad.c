
/******************************************************************************
**
**  $Id: SetQuad.c,v 1.5 1994/10/05 19:16:24 vertex Exp $
**
**  Lib3DO routine to set a quad from a rect.
**
******************************************************************************/


#include "utils3do.h"

void SetQuad( Point *r, Coord left, Coord top, Coord right, Coord bottom )
{
	r[0].pt_X =   left;
	r[0].pt_Y =   top;
	r[1].pt_X =   right;
	r[1].pt_Y =   top;
	r[2].pt_X =   right;
	r[2].pt_Y =   bottom;
	r[3].pt_X =   left;
	r[3].pt_Y =   bottom;
}
