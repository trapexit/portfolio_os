/* $Id: createBufferedMsg.c,v 1.9 1994/09/10 02:52:22 vertex Exp $ */
/* file createBufferedMsg.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"

/**
|||	AUTODOC PUBLIC spg/kernel/createbufferedmsg
|||	CreateBufferedMsg - Create a buffered message.
|||
|||	  Synopsis
|||
|||	    Item CreateBufferedMsg( const char *name, uint8 pri, Item mp,
|||	                            uint32 datasize )
|||
|||	  Description
|||
|||	    One of the ways tasks communicate is by sending messages to each
|||	    other.  This procedure creates an item for a buffered message (a
|||	    message that includes an internal buffer for sending data to the
|||	    receiving task).  You can use this procedure in place of
|||	    CreateItem() to create the message.
|||
|||	  Arguments
|||
|||	    name                        The name of the message (see `Notes').
|||
|||	    pri                         The priority of the message.
|||
|||	    mp                          The item number of the message port at
|||	                                which to receive the reply.
|||
|||	    datasize                    The maximum size of the message's
|||	                                internal buffer, in bytes.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the message or an error
|||	    code if an error occurs.
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
|||	    The advantage of using a buffered message instead of a standard
|||	    message is that the sending task doesn't need to keep the data
|||	    block containing the message data after the message is sent.  All
|||	    the necessary data is included in the message.
|||
|||	    The same message item can be resent any number of times. When you
|||	    are finished with a message item, use DeleteMsg() to delete it.
|||
|||	    You can use FindNamedItem() to find a message item by name.  When
|||	    creating messages, you should assign unique names whenever
|||	    possible.
|||
|||	    The Kernel may change the priority of a message to help optimize
|||	    throughput.
|||
|||	  Caveats
|||
|||	    The priority value (set with the pri argument) is not used
|||	    consistently.
|||
|||	  See Also
|||
|||	    CreateMsg(), CreateMsgPort(), CreateSmallMsg(), DeleteMsg(),
|||	    DeleteMsgPort(), SendMsg()
|||
**/

Item
CreateBufferedMsg(const char *name,uint8 pri,Item mp,uint32 datasize)
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

	if(datasize) {
		ta[3].ta_Tag = CREATEMSG_TAG_DATA_SIZE;
		ta[3].ta_Arg = (void *)datasize;
	}
	else ta[3].ta_Tag = TAG_NOP;

	ta[4].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,MESSAGENODE),ta);
}

