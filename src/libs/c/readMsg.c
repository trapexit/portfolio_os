/* $Id: readMsg.c,v 1.4 1994/05/11 00:21:55 tongue Exp $ */
/* file readMsg.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "operror.h"
#include "msgport.h"
#include "string.h"

Err ReadMsg(Item msgItem,uint8 *buffer, int32 size)
{
    Message *msg;

    msg = (Message *)LookupItem(msgItem);

    if (!msg) return BADITEM;
    /* small message ? */
    if (msg->msg.n_Flags & MESSAGE_SMALL) {
	size = size > 8 ? 8: size;
    }
    else {
        size=(size > msg->msg_DataPtrSize) ? msg->msg_DataPtrSize : size;
    }
    memcpy(buffer,(uint8 *)msg->msg_DataPtr,size);
    return 0;
}
