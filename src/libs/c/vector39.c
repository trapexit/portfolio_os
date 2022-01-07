/* $Id: vector39.c,v 1.1 1994/11/23 21:25:53 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "mem.h"


/*****************************************************************************/


extern struct KernelBase *KernelBase;

int32 GetMemAllocAlignment(uint32 typebits)
{
int32 result;

    CALLFOLIORET(KernelBase,KBV_GETMEMALLOCALIGNMENT,(typebits),result,(int32));

    return result;
}
