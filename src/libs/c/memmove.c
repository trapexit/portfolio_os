/* $Id: memmove.c,v 1.1 1994/08/29 19:35:40 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "string.h"

extern struct KernelBase *KernelBase;

void *
memmove(void *s1, const void *s2, size_t n)
{
	void *result;
	CALLFOLIORET(KernelBase,KBV_MEMCPY,(s1,s2,n),result,(void *));
	return result;
}
