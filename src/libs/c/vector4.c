/* $Id: vector4.c,v 1.2 1993/12/14 18:40:44 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void
AddTail(List *l, Node *n)
{
	CALLFOLIO(KernelBase,KBV_ADDTAIL,(l,n));
}
