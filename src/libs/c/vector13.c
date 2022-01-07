/* $Id: vector13.c,v 1.1 1993/12/14 18:54:59 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void *
memset(void *s, int c, size_t n)
{
	void *result;
	CALLFOLIORET(KernelBase,KBV_MEMSET,(s,c,n),result,(void *));
	return result;
}
