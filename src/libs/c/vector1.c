/* $Id: vector1.c,v 1.3 1993/12/14 18:36:20 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

Node *
RemHead(List *l)
{
	Node *n;
	CALLFOLIORET(KernelBase,KBV_REMHEAD,(l),n,(Node *));
	return n;
}
