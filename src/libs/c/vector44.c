/* $Id: vector44.c,v 1.1 1994/09/15 23:34:15 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "tags.h"


/*****************************************************************************/


extern struct KernelBase *KernelBase;

TagArg *FindTagArg(const TagArg *tagList, uint32 tag)
{
TagArg *result;

    CALLFOLIORET(KernelBase,KBV_FINDTAGARG,(tagList,tag),result,(TagArg *));

    return result;
}
