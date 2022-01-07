/* $Id: createSema4.c,v 1.3 1994/05/03 22:56:22 vertex Exp $ */
/* file createSema4.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "semaphore.h"
#include "createNamedItem.h"
#include "super.h"


Item
SuperCreateSemaphore(const char *name,uint8 pri)
{
    return SuperCreateNamedItemVA(MKNODEID(KERNELNODE,SEMA4NODE),name,pri,
                                  TAG_END);
}
