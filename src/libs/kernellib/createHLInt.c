/* $Id: createHLInt.c,v 1.4 1994/05/03 22:56:22 vertex Exp $ */
/* file createHLInt.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "interrupts.h"
#include "super.h"
#include "createNamedItem.h"


Item
SuperCreateHLInt(const char *name,uint8 pri,int32 (*code)(),int32 line)
{
    return SuperCreateNamedItemVA(MKNODEID(KERNELNODE,HLINTNODE),name,pri,
				  CREATEFIRQ_TAG_CODE,code,
				  CREATEFIRQ_TAG_NUM,line,
                                  TAG_END);
}
