/* $Id: createIOReq.c,v 1.8 1994/05/04 21:39:23 vertex Exp $ */
/* file createIOReq.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "io.h"
#include "createNamedItem.h"


Item
SuperCreateIOReq(const char *name,uint8 pri,Item dev, Item mp)
{
    return SuperCreateNamedItemVA(MKNODEID(KERNELNODE,IOREQNODE),name,pri,
				  CREATEIOREQ_TAG_DEVICE,    dev,
				  (mp > 0 ? CREATEIOREQ_TAG_REPLYPORT : TAG_NOP), mp,
                                  TAG_END);
}
