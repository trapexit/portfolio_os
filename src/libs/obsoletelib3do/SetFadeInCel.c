
/******************************************************************************
**
**  $Id: SetFadeInCel.c,v 1.6 1994/12/05 17:40:03 vertex Exp $
**
**  Lib3DO routine to calc a cel fade-in.
**
** This is pretty ugly stuff.  Check out the CrossFadeCels() routine for
** some code that may serve your needs better.
**
******************************************************************************/


#include "string.h"
#include "utils3do.h"


static void LinkACel(CCB *ccb, CCB *nextCCB)
{
	ccb->ccb_NextPtr	 = nextCCB;
	ccb->ccb_Flags		|= CCB_NPABS;
	ccb->ccb_Flags		&= ~CCB_LAST;
}

void
SetFadeInCel (CCB *ccb, CCB *maskccb, int32 *stepValue)
{

	/* Copy the CCB data to the maskCCB and set the maskCCB
	 * to use the data to generate a shadow of the asteroid.
	 * Set the asteroid CCB to blend with the shadow and set
	 * the starting scale for both CCB's.
	 */
	memcpy(maskccb, ccb, sizeof(CCB));
	SET_TO_SHADOW (maskccb);
	SET_TO_AVERAGE (ccb);
	*stepValue = 0;

	/* Link the cel in front of its shadow mask. The intensity
	 * of the shadow will be the inverse of the intensity of the
	 * cel. Since the cel will blend with the shadow, the
	 * effect will be to fade the cel in from the background.
	 */
	LinkACel (maskccb, ccb);
}
