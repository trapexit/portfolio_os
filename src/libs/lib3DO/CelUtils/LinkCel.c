
/******************************************************************************
**
**  $Id: LinkCel.c,v 1.5 1994/10/05 18:27:33 vertex Exp $
**
**  Lib3DO routine to link a pair of cels together.
**
**  This links nextCCB to ccb.  Works only if 'ccb' is a single cel; if it
**  is a list of cels, the existing list links are broken.  To preserve
**  existing links, use the ChainCels() family of functions.
**
******************************************************************************/


#include "celutils.h"

void LinkCel(CCB *ccb, CCB *nextCCB)
{
	ccb->ccb_NextPtr	 = nextCCB;
	ccb->ccb_Flags		|= CCB_NPABS;
	ccb->ccb_Flags		&= ~CCB_LAST;
}
