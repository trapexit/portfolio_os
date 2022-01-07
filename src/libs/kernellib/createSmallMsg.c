/* $Id: createSmallMsg.c,v 1.4 1994/05/03 22:56:22 vertex Exp $ */
/* file createSmallMsg.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"
#include "createNamedItem.h"
#include "super.h"


Item
SuperCreateSmallMsg(const char *name,uint8 pri,Item mp)
{
    return SuperCreateNamedItemVA(MKNODEID(KERNELNODE,MESSAGENODE),name,pri,
                                  CREATEMSG_TAG_REPLYPORT,    mp,
                                  CREATEMSG_TAG_MSG_IS_SMALL, 0,
                                  TAG_END);
}
