/* $Id: createSmallMsg.c,v 1.11 1994/09/10 02:52:22 vertex Exp $ */
/* file createSmallMsg.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"

/**
|||	AUTODOC PUBLIC spg/kernel/createsmallmsg
|||	CreateSmallMsg - Create a small message.
|||
|||	  Synopsis
|||
|||	    Item CreateSmallMsg( const char *name, uint8 pri, Item mp )
|||
|||	  Description
|||
|||	    This convenience procedure creates the item for a small message (a
|||	    message that can contain up to eight bytes of data).  You can use
|||	    this procedure in place of CreateItem() to create the item.
|||
|||	    To create a standard message (a message in which any data to be
|||	    communicated to the receiving task is contained in a data block
|||	    allocated by the sending task), use CreateMsg().  To create a
|||	    buffered message (a message that includes an internal buffer for
|||	    sending data to the receiving task), use CreateBufferedMsg().
|||
|||	  Arguments
|||
|||	    name                        The name of the message (see `Notes').
|||
|||	    pri                         The priority of the message.
|||
|||	    mp                          The item number of the message port
|||	                                for the return message.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the message that was
|||	    created or an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    The same message item can be resent any number of times. When you
|||	    are finished with a message item, use DeleteMsg() to delete it.
|||
|||	    You can use FindNamedItem() to find a message by name. When naming
|||	    messages, you should assign unique names whenever possible.
|||
|||	    The Kernel may change the priority of a message to help optimize
|||	    throughput.
|||
|||	  See Also
|||
|||	    CreateMsg(), CreateMsgPort(), CreateBufferedMsg(), DeleteMsg(),
|||	    DeleteMsgPort(), SendMsg()
|||
**/

Item
CreateSmallMsg(const char *name,uint8 pri,Item mp)
{
	TagArg ta[5];

	ta[0].ta_Tag = TAG_ITEM_PRI;
	ta[0].ta_Arg = (void *)pri;

	if (name)
	{
	    ta[1].ta_Tag = TAG_ITEM_NAME;
	    ta[1].ta_Arg = (void *)name;
	}
	else ta[1].ta_Tag = TAG_NOP;

	/* Item dependent options */
	ta[2].ta_Tag = CREATEMSG_TAG_REPLYPORT;
	ta[2].ta_Arg = (void *)mp;

	ta[3].ta_Tag = CREATEMSG_TAG_MSG_IS_SMALL;
	ta[3].ta_Arg = 0;

	ta[4].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,MESSAGENODE),ta);
}
