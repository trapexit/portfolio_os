/* $Id: vector10.c,v 1.3 1994/09/16 18:01:36 limes Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

Node *
FindNamedNode(const List *l, const char *name)
{
	Node *n;
	CALLFOLIORET(KernelBase,KBV_FINDNAMEDNODE,(l,name),n,(Node *));
	return n;
}
