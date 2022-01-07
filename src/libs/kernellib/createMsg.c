/* $Id: createMsg.c,v 1.3 1994/05/03 22:56:22 vertex Exp $ */
/* file createMsg.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"
#include "createNamedItem.h"
#include "super.h"


Item
SuperCreateMsg(const char *name,uint8 pri,Item mp)
{
    return SuperCreateNamedItemVA(MKNODEID(KERNELNODE,MESSAGENODE),name,pri,
                                  CREATEMSG_TAG_REPLYPORT,mp,
                                  TAG_END);
}
