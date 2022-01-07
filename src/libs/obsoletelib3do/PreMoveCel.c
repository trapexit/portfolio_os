
/******************************************************************************
**
**  $Id: PreMoveCel.c,v 1.5 1994/10/05 19:02:57 vertex Exp $
**
**  Lib3DO routine to calculate the increments for each corner of a Cel in order to move it from
**  the quadrilateral defined by beginQuad to the quadrilateral defined by endQuad.
**  The numberOfFrames determines how many iterations will be needed to complete
**  the movement.
**
******************************************************************************/


#include "utils3do.h"

void PreMoveCel (CCB *ccb, Point *beginQuad, Point *endQuad, int32 numberOfFrames, MoveRec *pMove)
{
	int			i;
	frac16		rem;
	Point		q[4];

	for (i=0;i<4;i++) {
		pMove->curQuadf16[i].xVector = Convert32_F16 (beginQuad[i].pt_X);
		pMove->curQuadf16[i].yVector = Convert32_F16 (beginQuad[i].pt_Y);

		q[i].pt_X = beginQuad[i].pt_X;
		q[i].pt_Y = beginQuad[i].pt_Y;

		pMove->quadIncr[i].xVector
			= DivRemSF16 (&rem, Convert32_F16 (endQuad[i].pt_X - beginQuad[i].pt_X), Convert32_F16 (numberOfFrames) );

		pMove->quadIncr[i].yVector
			= DivRemSF16 (&rem, Convert32_F16 (endQuad[i].pt_Y - beginQuad[i].pt_Y), Convert32_F16 (numberOfFrames) );
	}

	/*	Map Cel corners to the starting position.
	 */

	MapCel (ccb , q);

}
