/* $Id: createMsgPort.c,v 1.3 1994/05/03 22:56:22 vertex Exp $ */
/* file createMsgPort.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"
#include "createNamedItem.h"
#include "super.h"


Item
SuperCreateMsgPort(const char *name,uint8 pri,uint32 signal)
{
    return SuperCreateNamedItemVA(MKNODEID(KERNELNODE,MSGPORTNODE),name,pri,
                                  (signal ? CREATEPORT_TAG_SIGNAL : TAG_NOP), signal,
                                  TAG_END);
}
