/* $Id: vector20.c,v 1.1 1993/12/14 19:02:19 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void
InsertNodeFromHead(List *l, Node *n)
{
	CALLFOLIO(KernelBase,KBV_INSERTHEAD,(l,n));
}
