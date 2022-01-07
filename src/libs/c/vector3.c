/* $Id: vector3.c,v 1.2 1993/12/14 18:39:16 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

Node *
RemTail(List *l)
{
	Node *n;
	CALLFOLIORET(KernelBase,KBV_REMTAIL,(l),n,(Node *));
	return n;
}
