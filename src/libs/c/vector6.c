/* $Id: vector6.c,v 1.2 1993/12/14 18:45:35 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void
RemNode(Node *n)
{
	CALLFOLIO(KernelBase,KBV_REMOVENODE,(n));
}
