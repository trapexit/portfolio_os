
/******************************************************************************
**
**  $Id: FadeInCel.c,v 1.5 1994/10/05 18:12:37 vertex Exp $
**
**  Lib3DO routine to calc a cel fade-in.
**
**  This is pretty ugly stuff.  Check out the CrossFadeCels() routine for
**  some code that may serve your needs better.
**
******************************************************************************/


#include "utils3do.h"

Boolean FadeInCel(CCB *ccb, CCB *maskccb, int32 *stepValue)
{

	/* Fade the cel in one step at a time */
	if (*stepValue < MAX_SCALE) {
		SetCelScale (ccb, maskccb, (*stepValue)++);
		if (*stepValue == MAX_SCALE) {
			SET_TO_NORMAL (ccb);
			return false;
		}
		else
			return true;
	}

	return false;
}
