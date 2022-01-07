
/******************************************************************************
**
**  $Id: CenterCelOnScreen.c,v 1.5 1994/10/05 19:35:09 vertex Exp $
**
**  Lib3DO routine to center a cel in the display.
**
**  This function will change all of the projection parameters for the cel.
**  IE, if the cel is set to project at twice its normal width/height before
**  this call, it will back to x1 (and centered, of course) after the call.
**
**  The CenterRectCel() family of CelUtils functions will not change the
**  projection parms, only the XY.  They're faster too.
**
******************************************************************************/

#include "utils3do.h"

void CenterCelOnScreen (CCB *ccb)
{
	Rectf16	celRect, frameRect;
	Point	q[4];

	SetRectf16 (&frameRect, 0, 0, 320, 240);
	SetRectf16 (&celRect, 0, 0, ccb->ccb_Width, ccb->ccb_Height);
	CenterRectf16 (q, &celRect, &frameRect);
	MapCel (ccb, q);
}
