/* $Id: vector14.c,v 1.2 1993/12/14 18:56:10 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void *
memcpy(void *s1, void *s2, size_t n)
{
	void *result;
	CALLFOLIORET(KernelBase,KBV_MEMCPY,(s1,s2,n),result,(void *));
	return result;
}
