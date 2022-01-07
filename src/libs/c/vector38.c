/* $Id: vector38.c,v 1.1 1994/05/12 02:12:04 sdas Exp $ */

#include "types.h"
#include "folio.h"
#include "aif.h"

extern struct KernelBase *KernelBase;

AIFHeader *
FindImage(AIFHeader *aifp, uint32 pagemask, char *aifname)
{
	AIFHeader *newaifp;
	CALLFOLIORET(KernelBase,KBV_FINDIMAGE,(aifp,pagemask,aifname),newaifp,(AIFHeader *));
	return newaifp;
}
