/* $Id: vector9.c,v 1.3 1994/09/16 17:34:59 limes Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void
InitList(List *l, const char *name)
{
	CALLFOLIO(KernelBase,KBV_INITLIST,(l,name));
}
