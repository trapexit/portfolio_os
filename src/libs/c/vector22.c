/* $Id: vector22.c,v 1.1 1993/12/14 19:10:06 sdas Exp $ */

#include "types.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

void
GetSysErr(char *s, int32 n, Item i)
{
	CALLFOLIO(KernelBase,KBV_GETSYSERR,(s,n,i));
}
