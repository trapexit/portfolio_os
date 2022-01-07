/* $Id: vector5.c,v 1.2 1993/12/14 18:43:37 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void
InsertNodeFromTail(List *l, Node *n)
{
	CALLFOLIO(KernelBase,KBV_INSERTTAIL,(l,n));
}
