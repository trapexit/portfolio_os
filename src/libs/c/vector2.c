/* $Id: vector2.c,v 1.2 1993/12/14 18:38:01 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void
AddHead(List *l, Node *n)
{
	CALLFOLIO(KernelBase,KBV_ADDHEAD,(l,n));
}
