/* $Id: createFIRQ.c,v 1.5 1994/05/03 23:22:09 vertex Exp $ */
/* file createFIRQ.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "interrupts.h"
#include "createNamedItem.h"


Item
SuperCreateFIRQ(const char *name,uint8 pri,int32 (*code)(),int32 num)
{
    return SuperCreateNamedItemVA(MKNODEID(KERNELNODE,FIRQNODE),name,pri,
				  CREATEFIRQ_TAG_CODE, code,
				  CREATEFIRQ_TAG_NUM,  num,
                                  TAG_END);
}
