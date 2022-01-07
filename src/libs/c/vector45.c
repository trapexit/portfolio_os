/* $Id: vector45.c,v 1.1 1994/09/15 23:34:28 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "tags.h"


/*****************************************************************************/


extern struct KernelBase *KernelBase;

TagArg *NextTagArg(const TagArg **tagList)
{
TagArg *result;

    CALLFOLIORET(KernelBase,KBV_NEXTTAGARG,(tagList),result,(TagArg *));

    return result;
}
