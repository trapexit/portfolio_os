/* $Id: vector17.c,v 1.2 1993/12/14 18:59:34 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void
UniversalInsertNode(List *l,Node *n, bool (*f)())
{
	CALLFOLIO(KernelBase,KBV_FINSERT,(l,n,f));
}
