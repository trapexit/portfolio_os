
/******************************************************************************
**
**  $Id: MakeNewDupCCB.c,v 1.5 1994/10/05 19:35:33 vertex Exp $
**
**  Lib3DO routine to clone a cel.
**
**  This routine isn't very useful; use CloneCel() instead.
**
******************************************************************************/


#include "mem.h"
#include "string.h"
#include "utils3do.h"

CCB * MakeNewDupCCB( CCB *ccb )
{
	CCB	*newCCB;

	if ((newCCB = (CCB *)ALLOCMEM(sizeof(CCB),MEMTYPE_CEL)) == NULL)
		return NULL;

	memcpy(newCCB, ccb, sizeof(CCB));

	return newCCB;
}
