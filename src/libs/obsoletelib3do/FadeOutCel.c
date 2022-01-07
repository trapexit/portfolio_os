
/******************************************************************************
**
**  $Id: FadeOutCel.c,v 1.5 1994/10/05 18:14:27 vertex Exp $
**
**  Lib3DO routine to calc a cel fade-out.
**
**  This is pretty ugly stuff.  Check out the CrossFadeCels() routine for
**  some code that may serve your needs better.
**
******************************************************************************/


#include "utils3do.h"

Boolean FadeOutCel(CCB *ccb, CCB *maskccb, int32 *stepValue)
{
	if (*stepValue == (MAX_SCALE-1))
		SET_TO_AVERAGE (ccb);

	/* Fade the cel out one step at a time */
	SetCelScale (ccb, maskccb, --(*stepValue));

	if ( *stepValue > 0)
		return true;

	return false;
}
