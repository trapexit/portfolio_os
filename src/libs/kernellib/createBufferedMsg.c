/* $Id: createBufferedMsg.c,v 1.4 1994/05/26 20:34:08 vertex Exp $ */
/* file createBufferedMsg.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"
#include "createNamedItem.h"
#include "super.h"


Item
SuperCreateBufferedMsg(const char *name,uint8 pri,Item mp,uint32 datasize)
{
    return SuperCreateNamedItemVA(MKNODEID(KERNELNODE,MESSAGENODE),name,pri,
                                  CREATEMSG_TAG_REPLYPORT, mp,
                                  (datasize ? CREATEMSG_TAG_DATA_SIZE : TAG_NOP), datasize,
                                  TAG_END);
}
