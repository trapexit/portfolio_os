
/******************************************************************************
**
**  $Id: MoveCel.c,v 1.5 1994/10/05 18:46:03 vertex Exp $
**
**  Lib3DO routines to calc cel moves over time.
**
**  Move a Cel from beginQuad to endQuad over numberOfFrames. Each time MoveCel
**  is called the increments calculated in PreMoveCel are added to the current
**  corner values and the results are applied to the corners of the Cel using
**  MapCel().
**
**  MoveCel does NOT draw the cel into a frame buffer.
**
******************************************************************************/


#include "utils3do.h"

void MoveCel (CCB *ccb, MoveRec *pMove)
{
	register int	i;
	Point			q[4];
	frac16			fx, fy;

	for (i=0;i<4;i++) {
		fx = pMove->curQuadf16[i].xVector += pMove->quadIncr[i].xVector;
		fy = pMove->curQuadf16[i].yVector += pMove->quadIncr[i].yVector;
		q[i].pt_X = ConvertF16_32 (fx);
		q[i].pt_Y = ConvertF16_32 (fy);
	}

	/*	Map Cel corners to the next position.
	 */

	MapCel (ccb , q);
}
