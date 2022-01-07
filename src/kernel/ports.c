/* $Id: ports.c,v 1.93 1994/11/21 22:54:37 sdas Exp $ */

/* file: ports.c */

#include "types.h"
#include "nodes.h"
#include "item.h"
#include "list.h"
#include "listmacros.h"
#include "kernelnodes.h"
#include "task.h"
#include "msgport.h"
#include "kernel.h"
#include "internalf.h"
#include "mem.h"
#include "operror.h"
#include "string.h"
#include "stdio.h"

extern void Panic(int halt, char *);

#define DBUGDEL(x)	/*printf x*/
#define DBUG(x)	/*printf x*/
#define INFO(x) printf x

#ifdef MASTERDEBUG
#define DBUGPM(x)	if (CheckDebug(KernelBase,16)) printf x
#define DBUGRM(x)	if (CheckDebug(KernelBase,18)) printf x
#define DBUGWP(x)	if (CheckDebug(KernelBase,17)) printf x
#define DBUGGM(x)	if (CheckDebug(KernelBase,19)) printf x
#else
#define DBUGPM(x)
#define DBUGRM(x)
#define DBUGWP(x)
#define DBUGGM(x)
#endif

static int32
icmp_c(mp, p, tag, arg)
MsgPort *mp;
void *p;
uint32 tag;
uint32 arg;
{
  switch (tag)
  {
    case  CREATEPORT_TAG_SIGNAL: mp->mp_Signal = (uint32)arg;
			 break;
    case  CREATEPORT_TAG_USERDATA: mp->mp_UserData = (void *)arg;
			 break;
    default:	return BADTAG;
  }
  return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/createmsgport
|||	CreateMsgPort - Create a message port.
|||
|||	  Synopsis
|||
|||	    Item CreateMsgPort( const char *name, uint8 pri, uint32 signal )
|||
|||	  Description
|||
|||	    This convenience procedure creates a message port item. It also
|||	    creates a message queue for the port in system RAM.  You can use
|||	    this procedure in place of CreateItem() to create the item.
|||
|||	  Arguments
|||
|||	    name                        The name of the message port (see
|||	                                `Notes').
|||
|||	    pri                         The priority of the message port. The
|||	                                priority has no effect on the way
|||	                                message ports operate, but it does
|||	                                determine the order of the message
|||	                                ports in the global list of message
|||	                                ports and thus, the speed in which a
|||	                                particular message port can be found.
|||
|||	    signal                      Specifies the bit in the signal word
|||	                                that is set when a message arrives at
|||	                                this port. If zero is supplied a new
|||	                                signal will be allocated. (See the
|||	                                description of AllocSignal() for
|||	                                information about allocating signal
|||	                                bits.)
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the new message port or
|||	    an error code if an error occurs.
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
|||	    When you no longer need a message port, use DeleteMsgPort() to
|||	    delete it.
|||
|||	    You can use FindMsgPort() to find a message port by name. When
|||	    creating message ports, you should assign unique names whenever
|||	    possible.
|||
|||	    The Kernel may change the priority of a message port to help
|||	    optimize throughput.
|||
|||	  See Also
|||
|||	    CreateMsg(), DeleteMsg(), DeleteMsgPort(), SendMsg(),
|||	    SendSmallMsg()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/createuniquemsgport
|||	CreateUniqueMsgPort - Create a message port with a unique name.
|||
|||	  Synopsis
|||
|||	    Item CreateUniqueMsgPort( const char *name, uint8 pri, uint32 signal )
|||
|||	  Description
|||
|||	    This convenience procedure creates a message port item. It also
|||	    creates a message queue for the port in system RAM.  You can use
|||	    this procedure in place of CreateItem() to create the item.
|||
|||	    This function works much like CreateMsgPort(), except that it
|||	    guarantees that no other message port item of the same name
|||	    already exists. And once this port created, no other
|||	    port of the same name will be allowed to be created.
|||
|||	  Arguments
|||
|||	    name                        The name of the message port (see
|||	                                `Notes').
|||
|||	    pri                         The priority of the message port. The
|||	                                priority has no effect on the way
|||	                                message ports operate, but it does
|||	                                determine the order of the message
|||	                                ports in the global list of message
|||	                                ports and thus, the speed in which a
|||	                                particular message port can be found.
|||
|||	    signal                      Specifies the bit in the signal word
|||	                                that is set when a message arrives at
|||	                                this port. If zero is supplied a new
|||	                                signal will be allocated. (See the
|||	                                description of AllocSignal() for
|||	                                information about allocating signal
|||	                                bits.)
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the new message port or
|||	    an error code if an error occurs. If a port of the same name
|||	    already existed when this call was made, the ER_Kr_UniqueItemExists
|||	    error will be returned.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V24.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    When you no longer need a message port, use DeleteMsgPort() to
|||	    delete it.
|||
|||	    You can use FindMsgPort() to find a message port by name.
|||
|||	    The Kernel may change the priority of a message port to help
|||	    optimize throughput.
|||
|||	  See Also
|||
|||	    CreateMsg(), DeleteMsg(), DeleteMsgPort(), SendMsg(),
|||	    SendSmallMsg()
|||
**/

Item
internalCreateMsgPort(mp,tagpt)
MsgPort *mp;
TagArg *tagpt;
{
	/* Create a message Port */
	Item ret;

	ret = TagProcessor(mp, tagpt, icmp_c, 0);
	if (ret < 0) return ret;

	if (mp->mp_Signal == 0)
	{
		mp->mp_Signal = internalAllocSignal(0);
		if (!mp->mp_Signal)
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_NoSigs);

		mp->mp.n_Flags |= MSGPORT_SIGNAL_ALLOCATED;
	}
	else
	{
		ret = ValidateSignal(mp->mp_Signal);
		if (ret < 0) return ret;
	}

	InitList(&mp->mp_Msgs,0);
	/* Add to list of public ports */

	TailInsertNode(KernelBase->kb_MsgPorts,(Node *)mp);
	return  mp->mp.n_Item;
}

/**
|||	AUTODOC PUBLIC spg/kernel/sendmsg
|||	SendMsg - Send a message.
|||
|||	  Synopsis
|||
|||	    Err SendMsg( Item mp, Item msg, const void *dataptr, int32 datasize )
|||
|||	  Description
|||
|||	    This procedure sends a message to the specified message port.  (To
|||	    send a small message without having to cast the data to (void *)
|||	    and (int32) use SendSmallMsg().
|||
|||	    The message is queued on the message port's list of messages
|||	    according to its priority.  Messages that have the same priority
|||	    are queued on first come first served basis.
|||
|||	    Whether a message is standard or buffered is determined by the
|||	    procedure you use to create the message: CreateMsg() creates a
|||	    standard message (one whose message data belongs to the sending
|||	    task; the receiving task reads from this block), while
|||	    CreateBufferedMsg creates a buffered message (one in which a
|||	    buffer for the message data is included in the message).  For
|||	    standard messages, the dataptr and datasize arguments refer to the
|||	    data block owned by the message sender.  For buffered messages,
|||	    the dataptr and datasize arguments specify the data to be copied
|||	    into the message's internal buffer before the message is sent.
|||
|||	    If the message is a buffered message, SendMsg() checks the size of
|||	    the data block to see whether it will fit in the message's buffer.
|||	    If it won't fit, SendMsg() returns an error.
|||
|||	  Arguments
|||
|||	    mp                          The item number of the message port to
|||	                                which to send the message.
|||
|||	    msg                         The item number of the message to
|||	                                send.
|||
|||	    dataptr                     A pointer to the message data, or NULL
|||	                                if there is no data to include in the
|||	                                message.  For a standard message, the
|||	                                pointer specifies a data block owned
|||	                                by the sending task, which can later
|||	                                be read by the receiving task.  For a
|||	                                buffered message, the pointer
|||	                                specifies a block of data to be copied
|||	                                into the message's internal data
|||	                                buffer.
|||
|||	    datasize                    The size of the message data, in
|||	                                bytes, or 0 if there is no data to
|||	                                include in the message.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the message was sent successfully or an
|||	    error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ARM C "swi" declaration
|||
|||	  See Also
|||
|||	    GetMsg(), GetThisMsg(), ReplyMsg(), ReplySmallMsg(),
|||	    SendSmallMsg(), WaitPort()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/sendsmallmsg
|||	SendSmallMsg - Send a small message.
|||
|||	  Synopsis
|||
|||	    Err SendSmallMsg( Item mp, Item msg, uint32 val1, uint32 val2 )
|||
|||	  Description
|||
|||	    This procedure sends a small message (a message that contains no
|||	    more than eight bytes of data) to the specified message port.  (To
|||	    send a standard or buffered message, use SendMsg().) This routine
|||	    is identical to SendMsg().  It is provided only to avoid having to
|||	    cast uint32 to the types needed by SendMsg().
|||
|||	  Arguments
|||
|||	    mp                          The item number of the message port to
|||	                                which to send the message
|||
|||	    msg                         The item number of the message to
|||	                                send.
|||
|||	    val1                        The first four bytes of message data.
|||	                                This data is put into the msg_DataPtr
|||	                                field of the message structure.
|||
|||	    val2                        The last four bytes of message data.
|||	                                This data is put into the msg_DataSize
|||	                                field of the message structure.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the message is sent successfully or an
|||	    error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ARM C "swi" declaration
|||
|||	  See Also
|||
|||	    GetMsg(), GetThisMsg(), ReplyMsg(), ReplySmallMsg(), SendMsg(),
|||	    WaitPort()
|||
**/

Err externalPutMsg(imp,imsg,dataptr,datasize)
Item imp,imsg;
void *dataptr;
int32 datasize;
{
	Task *ct = KernelBase->kb_CurrentTask;
	MsgPort *mp = (MsgPort *)CheckItem(imp,KERNELNODE,MSGPORTNODE);
	Msg *msg = (Msg *)CheckItem(imsg,KERNELNODE,MESSAGENODE);

	DBUGPM(("externalPutMsg(%lx,%lx)" ,imp,imsg));
	DBUGPM((" (%lx,%lx)\n",(long)mp,(long)msg));

	if ( (!mp) || (!msg) ) return BADITEM;

	/* msg_MsgPort becomes the TaskItem of the current user */
	/* this is set by createmsg and getmsg */

	if (ct->t.n_Item != msg->msg_MsgPort) return NOTOWNER;
	msg->msg_Result = 0;

	if (msg->msg.n_Flags & MESSAGE_PASS_BY_VALUE)
	{
	    /* Validate incoming message data ptr */
	    if (IsRamAddr((uint8 *)dataptr,datasize) == 0) {
		INFO(("MSG Send with bad data region [addr=0x%x, size=%d]\n", dataptr, datasize));
		return BADPTR;
	    }
	    /* and validate the message size */
	    if (datasize > msg->msg_DataPtrSize)
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadSize);
	}
	/* if not a small message, validate pointer */
	else if ((msg->msg.n_Flags & MESSAGE_SMALL) == 0) {
	    /* Validate incoming message data ptr */
	    if (oldIsRamAddr((uint8 *)dataptr,datasize) == 0) {
		INFO(("MSG Send with bad data region [addr=0x%x, size=%d]\n", dataptr, datasize));
		return BADPTR;
	    }
	}
	return internalPutMsg(mp,msg,dataptr,datasize);
}

Err
internalPutMsg(mp,msg,dataptr,datasize)
MsgPort *mp;
Msg *msg;
void *dataptr;
int32 datasize;
{
	/* Send a Msg to the Port, wake up the port's task */
	uint32	oldints;
	Err	ret;

	/* Have a valid Msg and a valid MsgPort */
	oldints = Disable();
	if (msg->msg.n_Flags & (MESSAGE_SENT | MESSAGE_REPLIED))
	{
		/* Msg in a bad state - already on some MsgPort */
		Enable(oldints);
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_MsgSent);
	}
	msg->msg.n_Flags |= MESSAGE_SENT;
	TailInsertNode(&mp->mp_Msgs,(Node *)msg);
	Enable(oldints);
	msg->msg_DataSize = datasize;
	if (msg->msg.n_Flags & MESSAGE_PASS_BY_VALUE)
	{
		/* copy the data */
		/* (this may take too long while interrupts disabled) */
		memcpy(msg->msg_DataPtr,dataptr,datasize);
	}
	else msg->msg_DataPtr = dataptr;
	msg->msg_MsgPort = mp->mp.n_Item;
	DBUGPM(("Putmsg calls signal\n"));

	ret = internalSignal((Task *)LookupItem(mp->mp.n_Owner),mp->mp_Signal);
	DBUGPM(("PutMsg returns\n"));
	return ret;
}

Err
superinternalPutMsg(mp,msg,dataptr,datasize)
MsgPort *mp;
Msg *msg;
void *dataptr;
int32 datasize;
{ return internalPutMsg(mp,msg,dataptr,datasize); }

/**
|||	AUTODOC PUBLIC spg/kernel/replymsg
|||	ReplyMsg - Send a reply to a message.
|||
|||	  Synopsis
|||
|||	    Err ReplyMsg( Item msg, int32 result, const void *dataptr,
|||	                  int32 datasize )
|||
|||	  Description
|||
|||	    This procedure sends a reply to a message.
|||
|||	    When replying to a message, send back the same message item you
|||	    received.  (Think of this as sending a reply in the same envelope
|||	    as the original letter.)  Note that you must include a result code
|||	    (which you provide in the result argument).
|||
|||	    The meanings of the dataptr and datasize arguments depend on the
|||	    type of the original message.  (You can find out what type of
|||	    message it is by looking at its msg.n_Flags field.  If it's is a
|||	    small message, the value of the field is MESSAGE_SMALL.  If it's a
|||	    buffered message, the value of the field is
|||	    MESSAGE_PASS_BY_VALUE.)
|||
|||	    *   If the original message was a small message, the dataptr and
|||	        datasize are put in the corresponding fields of the message as
|||	        eight bytes of data.
|||
|||	    *   If the original message was a standard message, these refer to
|||	        a data block that your task allocates for returning reply
|||	        data.  For standard messages, the sending task and the
|||	        replying task must each allocate its own memory blocks for
|||	        message data. If the message was also buffered, the data is
|||	        copied into the internal buffer of the message.
|||
|||	  Arguments
|||
|||	    msg                         The item number of the message.  Note:
|||	                                The reply message item must be same
|||	                                message item that was received.
|||
|||	    result                      A result code.  This code is placed in
|||	                                the msg_Result field of the message
|||	                                data structure before the reply is
|||	                                sent.  Note:  There are no standard
|||	                                result codes currently defined; the
|||	                                code must be a value whose meaning is
|||	                                agreed upon by the calling and
|||	                                receiving tasks.  In general, you
|||	                                should use negative values to specify
|||	                                errors and 0 to specify that no error
|||	                                occurred.
|||
|||	    dataptr                     See the `Description' section above
|||	                                for a description of this argument.
|||	                                If the message is not buffered, set
|||	                                this to 0.
|||
|||	    datasize                    See the `Description' section above
|||	                                for a description of this argument.
|||	                                If the message is not buffered, set
|||	                                this to 0.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the reply was sent successfully or an
|||	    error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    Most programs handle only one type of message per message port.
|||	    Each type of message moves information in a different way;
|||	    programs choose a particular message type to meet their specific
|||	    needs.
|||
|||	    A reply message is automatically sent to the reply port specified
|||	    by the sending task when the message is created.  For more
|||	    information about message structures, see the `Data Structures and
|||	    Variable Types' chapter.
|||
|||	  See Also
|||
|||	    GetMsg(), GetThisMsg(), ReplySmallMsg(), SendMsg(),
|||	    SendSmallMsg(), WaitPort()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/replysmallmsg
|||	ReplySmallMsg - Send a reply to a small message.
|||
|||	  Synopsis
|||
|||	    Err ReplySmallMsg( Item msg, int32 result, uint32 val1, uint32
|||	    val2)
|||
|||	  Description
|||
|||	    This procedure sends a reply to a small message.  (To reply to a
|||	    standard or buffered message, use ReplyMsg().)
|||
|||	    This procedure is identical to ReplyMsg() except that arguments
|||	    are cast as (uint32) instead of (void *) and (int32).  A small
|||	    message can return only eight bytes of data: four that you provide
|||	    in the val1 argument, and four that you provide in the val2
|||	    argument.
|||
|||	    When replying to a message, send back the same message item you
|||	    received.  (Think of this as sending a reply in the same envelope
|||	    as the original letter.)  You can include an optional result code
|||	    (which you provide in the result argument).  Note that no standard
|||	    result codes are currently defined; the result code must be a
|||	    value whose meaning is agreed upon by the sending and receiving
|||	    tasks.  For consistency, use negative numbers for errors.
|||
|||	  Arguments
|||
|||	    msg                         The item number of the message.  Note:
|||	                                The reply message item must be same
|||	                                message item that was received.
|||
|||	    result                      A result code.  This code is placed in
|||	                                the msg_Result field of the message
|||	                                data structure before the reply is
|||	                                sent.
|||
|||	    val1                        The first four bytes of data for the
|||	                                reply.  This data is put into the
|||	                                msg_DataPtr field of the message
|||	                                structure.
|||
|||	    val2                        The last four bytes of data for the
|||	                                reply.  This data is put into the
|||	                                msg_DataSize field of the message
|||	                                structure.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the reply was sent successfully or an
|||	    error code if there was an error.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ARM C "swi" declaration
|||
|||	  Note
|||
|||	    A reply message automatically goes to the reply port specified by
|||	    the sending task when the original message was sent.  For more
|||	    information about message structures, see the `Data Structures and
|||	    Variable Types' chapter.
|||
|||	  See Also
|||
|||	    GetMsg(), GetThisMsg(), ReplyMsg(), SendMsg(), SendSmallMsg(),
|||	    WaitPort()
|||
**/

Err
internalReplyMsg(Msg *msg, int32 result, void *dataptr, int32 datasize)
{
    MsgPort *mp;
    uint32   oldints;
    int32    err;

    mp = (MsgPort *)LookupItem(msg->msg_ReplyPort);
    DBUGRM(("ReplyMsg(), reply port $%x\n",mp));
    if (!mp)
    {
        /* bad reply port */
        return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_NoReplyPort);
    }

    oldints = Disable();
    if (msg->msg.n_Flags & (MESSAGE_SENT|MESSAGE_REPLIED))
    {
        /* Msg in a bad state */
        Enable(oldints);
#ifdef DEVELOPMENT
        printf("ReplyMsg(): message is already marked as 'sent' or 'received'");
#endif
        return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_MsgSent);
    }
    msg->msg.n_Flags |= (MESSAGE_REPLIED|MESSAGE_SENT);
    TailInsertNode(&mp->mp_Msgs,(Node *)msg);
    Enable(oldints);

    if (msg->msg.n_Flags & MESSAGE_PASS_BY_VALUE)
        memcpy(msg->msg_DataPtr,dataptr,datasize);
    else
        msg->msg_DataPtr = dataptr;

    msg->msg_DataSize = datasize;
    msg->msg_MsgPort  = mp->mp.n_Item;
    msg->msg_Result   = result;

    DBUGRM(("ReplyMsg() signalling target task\n"));

    err = internalSignal((Task *)LookupItem(mp->mp.n_Owner),mp->mp_Signal);

    DBUGRM(("ReplyMsg(), returning with %d\n",err));

    return err;
}

Err
externalReplyMsg(Item imsg, int32 result, void *dataptr, int32 datasize)
{
    Msg *msg;
    Task *t;

    DBUGRM(("ReplyMsg($%x,$%x,$%x,$%x)\n",imsg,msg,dataptr,datasize));

    msg = (Msg *)CheckItem(imsg,KERNELNODE,MESSAGENODE);
    if (!msg)
    {
        /* not a valid message item */
        return BADITEM;
    }

    t = (Task *)CheckItem(msg->msg_MsgPort, KERNELNODE, TASKNODE);

    if (!t || !IsSameTaskContext(CURRENTTASK, t))
    {
	/* message not got or bad priv */
	return BADPRIV;
    }

    /* We should check if this task really has the msg. This would
     * require another field in the msg block indicating which task
     * has received this message
     */

    if (msg->msg.n_Flags & MESSAGE_PASS_BY_VALUE)
    {
        /* Validate incoming message data size */
        if (datasize > msg->msg_DataPtrSize)
        {
#ifdef DEVELOPMENT
            printf("ReplyMsg(): bad datasize (was %d, maximum is %d)\n",datasize,msg->msg_DataPtrSize);
#endif
            return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadSize);
        }

        /* Validate incoming message data ptr */
        if (IsRamAddr((uint8 *)dataptr,datasize) == 0)
        {
#ifdef DEVELOPMENT
            printf("ReplyMsg(): bad dataptr (was $%x)\n", dataptr);
#endif
            return BADPTR;
        }
    }
    else if ((msg->msg.n_Flags & MESSAGE_SMALL) == 0)
    {
        /* validate the dataptr/size if not a small message */

        /* Validate incoming message data ptr */
        if (oldIsRamAddr((uint8 *)dataptr,datasize) == 0)
        {
#ifdef DEVELOPMENT
            printf("ReplyMsg(): bad dataptr (was $%x)\n", dataptr);
#endif
            return BADPTR;
        }
    }

    return internalReplyMsg(msg,result,dataptr,datasize);
}

/**
|||	AUTODOC PUBLIC spg/kernel/deletemsg
|||	DeleteMsg - Delete a message.
|||
|||	  Synopsis
|||
|||	    Err DeleteMsg( Item it )
|||
|||	  Description
|||
|||	    This macro deletes the specified message and any resources
|||	    (including the internal data buffer for a buffered message) that
|||	    were allocated for it.
|||
|||	  Arguments
|||
|||	    it                          The item number of the message to be
|||	                                deleted.
|||
|||	  Return Value
|||
|||	    The macro returns 0 if successful or an error code if an error
|||	    occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in msgport.h V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ANSI C Macro definition
|||
|||	  See Also
|||
|||	    CreateMsg()
|||
**/

Err
internalDeleteMsg(msg,ct)
Msg *msg;
Task *ct;
{
	uint32 oldints;
	DBUGDEL(("DeleteMsg(%lx,%lx)\n",msg,(uint32)ct));
	oldints = Disable();
	/* Is this Msg currently in use? */
	if (msg->msg.n_Flags & MESSAGE_SENT)
	{	/* Currently on someones MsgPort */
		REMOVENODE((Node *)msg);
	}
	/* mark it sent anyway; so that any further reply/send on it fails */
	msg->msg.n_Flags |= MESSAGE_SENT;
	Enable(oldints);

	if (msg->msg_Waiters)
	{
	    List *l = KernelBase->kb_Tasks;
	    Node *n;

            /* If there are folks waiting on this message, notify them that
             * the message just went *poof*
             */

	    for (n = FIRSTNODE(l); ISNODE(l,n); n = NEXTNODE(n))
	    {
		Task *t = Task_Addr(n);

		if (t->t_WaitItem == msg->msg.n_Item)
		    internalSignal(t, SIGF_ABORT);
	    }
	}

	return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/deletemsgport
|||	DeleteMsgPort - Delete a message port.
|||
|||	  Synopsis
|||
|||	    Err DeleteMsgPort( Item it )
|||
|||	  Description
|||
|||	    This macro deletes the specified message port.
|||
|||	  Arguments
|||
|||	    it                          The item number of the message port to
|||	                                be deleted.
|||
|||	  Return Value
|||
|||	    The macro returns 0 if successful or an error code if an error
|||	    occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in msgport.h V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ANSI C Macro Definition
|||
|||	  See Also
|||
|||	    CreateMsg(), CreateMsgPort()
|||
**/

Err
internalDeleteMsgPort(mp,ct)
MsgPort *mp;
Task *ct;
{
	Msg *msg;
	uint32 oldints;
	Task *t;

	DBUGDEL(("DeleteMsgPort(%d,%lx)\n",mp,(uint32)ct));

	REMOVENODE((Node *)mp);

	oldints = Disable();

	/* We need the Flags to be preserved so call RemHead */
	/* instead of GetMsg() */
	DBUGDEL(("draining msg list\n"));
	while ((msg = (Msg *)RemHead(&mp->mp_Msgs)) != (Msg *)0)
	{	/* Pull all messages off and process */
		DBUGDEL(("msgid = %d\n",msg->msg.n_Item));
		if (msg->msg.n_Flags & MESSAGE_REPLIED)
		{
			/* This may not be correct */
			/*internalDeleteMsg(msg,ct);*/
			msg->msg.n_Flags &= ~(MESSAGE_SENT|MESSAGE_REPLIED);
			/* Just let it hang around */
			/* But mark it as free floating */
		}
		else if (msg->msg.n_Flags & MESSAGE_SENT)
		{
			msg->msg.n_Flags &= ~(MESSAGE_SENT|MESSAGE_REPLIED);
			/* send back to owner */
			externalReplyMsg(msg->msg.n_Item,(int32)BADITEM,
						(void *)0,(int32)0);
		}
		else
		{
			/* Msg should NOT be here */
#ifdef DEVELOPMENT
			printf("error, bad message on MsgPort\n");
			while (1);
#else
			Panic(1,"bad msg on port\n");
#endif

		}
	}
	/* Msg port cleaned off */

	/* Still need to do: What about all the msgs created with */
	/* this port as a reply port? */

	Enable(oldints);

	t  = (Task *)LookupItem(mp->mp.n_Owner);
	if (t)
	{
#ifdef DEVELOPMENT
	    if ((mp->mp_Signal & t->t_AllocatedSigs) != mp->mp_Signal)
	    {
	        printf("WARNING: signal bits freed before the message port was deleted!\n");
	        printf("         (port name '%s', port bits $%08lx, task bits $%08lx)\n",
                       (mp->mp.n_Name ? mp->mp.n_Name : "<null>"), mp->mp_Signal,t->t_AllocatedSigs);
	    }
#endif

	    if (mp->mp.n_Flags & MSGPORT_SIGNAL_ALLOCATED)
                internalFreeSignal(mp->mp_Signal,t);
        }

	return 0;
}

static int32
icm_c(msg, p, tag, arg)
Msg *msg;
void *p;
uint32 tag;
uint32 arg;
{
  switch (tag)
  {
    case  CREATEMSG_TAG_REPLYPORT: msg->msg_ReplyPort = (Item)arg;
			 break;
    case  CREATEMSG_TAG_MSG_IS_SMALL:
				   msg->msg.n_Flags |= MESSAGE_SMALL;
			 break;
    case  CREATEMSG_TAG_DATA_SIZE:
				msg->msg_DataPtrSize = (long)arg;
				msg->msg_DataPtr = msg+1;
				msg->msg.n_Flags |= MESSAGE_PASS_BY_VALUE;
			 break;

    default:	return BADTAG;


  }
  return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/createmsg
|||	CreateMsg - Create a standard message.
|||
|||	  Synopsis
|||
|||	    Item CreateMsg( const char *name, uint8 pri, Item mp )
|||
|||	  Description
|||
|||	    One of the ways tasks communicate is by sending messages to each
|||	    other.  This procedure creates an item for a standard message (a
|||	    message where any data to be communicated to the receiving task is
|||	    contained in a data block allocated by the sending task).  You can
|||	    use this procedure in place of CreateItem() to create the item.
|||
|||	    To create a buffered message (a message that includes an internal
|||	    buffer for sending data to the receiving task), use
|||	    CreateBufferedMsg().  To create a small message (a message that
|||	    can contain up to eight bytes of data), use CreateSmallMsg().
|||
|||	  Arguments
|||
|||	    name                        The name of the message (see `Notes').
|||
|||	    pri                         The priority of the message.  This
|||	                                determines the position of the message
|||	                                in the receiving task's message queue
|||	                                and thus, how soon it is likely to be
|||	                                handled (see `Notes').  The priority
|||	                                can be a value from 11 to 199 (0 to 10
|||	                                and 200 to 255 can only be assigned by
|||	                                the system software).  A larger number
|||	                                specifies a higher priority.
|||
|||	    mp                          The item number of the message port at
|||	                                which to receive the reply.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the message or an error
|||	    code (a negative value) if an error occurs.
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
|||	    The same message item can be reused for any number of messages.
|||	    When you are finished with a message item, use DeleteMsg() to
|||	    delete it.
|||
|||	    You can use FindNamedItem() to find a message item by name.  When
|||	    creating messages, you should assign unique names whenever
|||	    possible.
|||
|||	  See Also
|||
|||	    CreateMsgPort(), CreateBufferedMsg(), CreateSmallMsg(),
|||	    DeleteMsg(), DeleteMsgPort(), SendMsg()
|||
**/

Item
externalCreateMsg(mdummy,tagpt)
Msg *mdummy;
TagArg *tagpt;
{
	Task *ct = KernelBase->kb_CurrentTask;
	Msg  *msg;
	Item ret;

	DBUG(("eCreateMsg(%lx,%lx)\n",(long)mdummy,(long)tagpt));

	if (mdummy != (Msg *)-1)
	    return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadSize);

	{
	    TagArg tagreturn;
	    int32  size = sizeof(Msg);

	    ret= TagProcessorSearch(&tagreturn, tagpt, CREATEMSG_TAG_DATA_SIZE);
	    if (ret < 0) return ret;

	    if (ret)
	    {
		size += (int32)tagreturn.ta_Arg;
		if (size < sizeof(Msg))	return BADTAGVAL;
	    }

	    msg= (Msg *)AllocateSizedNode((Folio *)KernelBase,MESSAGENODE,size);
	    if (msg == NULL) return NOMEM;
	}

	ret = TagProcessor(msg, tagpt, icm_c, 0);
	if (ret < 0) goto err;

	/* validation */

	/* a message can't be both small and pass by value */
	if ((msg->msg.n_Flags & (MESSAGE_SMALL|MESSAGE_PASS_BY_VALUE)) ==
	    (MESSAGE_SMALL|MESSAGE_PASS_BY_VALUE))
	{
	    ret = BADTAGVAL;
	    goto err;
	}

	if (msg->msg_ReplyPort)
	{
	    MsgPort *mp;

	    mp= (MsgPort *)CheckItem(msg->msg_ReplyPort,KERNELNODE,MSGPORTNODE);
	    if (!mp)
	    {
		ret = BADITEM;
		goto err;
	    }
	    if (ct->t.n_Item != mp->mp.n_Owner)
	    {
		ret = NOTOWNER;
		goto err;
	    }
	}
	else {
 	    ret = MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_ReplyPortNeeded);
	    goto err;
	}

	/* the current task is the creator */
	msg->msg_MsgPort = ct->t.n_Item;

	return msg->msg.n_Item;
err:
	FreeNode((Folio *)KernelBase, msg);
	return ret;
}

int
OKMsg(msg)
Msg *msg;
{
    Task *ct = KernelBase->kb_CurrentTask;
    MsgPort *mp;
    if (ct->t.n_Item == msg->msg.n_Owner) return 1;
    /* Don't own this msg, do we own the msg port it is on? */
    mp = (MsgPort *)CheckItem(msg->msg_MsgPort,KERNELNODE,MSGPORTNODE);
    if (mp == 0)	return 0;	/* not on any port, can't get */
    if (mp->mp.n_Owner == ct->t.n_Item) return 1; /* ct owns msgport, ok to get */
    return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/getthismsg
|||	GetThisMsg - Get a specific message.
|||
|||	  Synopsis
|||
|||	    Item GetThisMsg( Item message )
|||
|||	  Description
|||
|||	    This procedure gets a specific message and removes it from the
|||	    message queue that contains it, if any.
|||
|||	    A task that is receiving messages can use GetThisMsg() to get an
|||	    incoming message.  Unlike GetMsg(), which gets the first message
|||	    in a specific message queue, GetThisMsg() can get any message from
|||	    any of the task's message queues.
|||
|||	    A task that has sent a message can use GetThisMsg() to get it
|||	    back.  If the receiving task hasn't already taken the message from
|||	    its message queue, GetThisMsg() removes it from the queue.  If the
|||	    message is not on any MsgPort a zero is returned.
|||
|||	  Arguments
|||
|||	    message                     The item number of the message to get.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the message or an error
|||	    code (a negative value) if an error occurred.  Possible error
|||	    codes include:
|||
|||	    BADITEM                     The message argument does not specify
|||	                                a message.
|||
|||	    BADPRIV                     The calling task is not allowed to get
|||	                                this message.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ANSI C Prototype
|||
|||	  Caveats
|||
|||	    Prior to V21, if the message was not on any message port this
|||	    procedure still returned the item number of the message.  In
|||	    V21 and beyond, if the message is not on any port, the procedure
|||	    returns an error.
|||
|||	  See Also
|||
|||	    CreateMsg(), GetMsg(), ReplyMsg(), ReplySmallMsg(), SendMsg(),
|||	    SendSmallMsg(), WaitPort()
|||
**/

Item
internalGetThisMsg(imsg)
Item imsg;
{
	/* Get this Msg from any MsgPort */
	Msg *msg = (Msg *)CheckItem(imsg,KERNELNODE,MESSAGENODE);
	uint32 oldints;
	DBUGGM(("GetMsg msg =%lx\n",(long)imsg));
	if (!msg)	return BADITEM;
	oldints = Disable();
	if (OKMsg(msg))
	{
	    if (msg->msg.n_Flags & (MESSAGE_SENT|MESSAGE_REPLIED))
	    {
	    	REMOVENODE((Node *)msg);
	    	msg->msg.n_Flags &= ~(MESSAGE_SENT|MESSAGE_REPLIED);
	    	msg->msg_MsgPort = CURRENTTASK->t.n_Item;;
	    }
	    else imsg = -1;
	}
	else	imsg = BADPRIV;
	Enable(oldints);
	return imsg;
}

/**
|||	AUTODOC PUBLIC spg/kernel/getmsg
|||	GetMsg - Get a message from a message port.
|||
|||	  Synopsis
|||
|||	    Item GetMsg( Item mp )
|||
|||	  Description
|||
|||	    This procedure gets the first message in the message queue for the
|||	    specified message port and removes the message from the queue.
|||
|||	  Arguments
|||
|||	    mp                          The item number of the message port
|||	                                from which to get the message.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the first message in the
|||	    message queue or an error code (a negative value) if an error
|||	    occurs.  Possible error codes include the following:
|||
|||	    0                           The queue is empty.
|||
|||	    BADITEM                     The mp argument does not specify a
|||	                                message port.
|||
|||	    NOTOWNER                    The message port specified by the mp
|||	                                argument is not owned by this task.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ARM C "swi" declaration
|||
|||	  See Also
|||
|||	    DeleteMsg(), GetThisMsg(), ReplyMsg(), ReplySmallMsg(), SendMsg(),
|||	    SendSmallMsg(), WaitPort()
|||
**/

Item
internalGetMsg(imp)
Item imp;
{
	Item ret;
	/* Get the next Msg from the MsgPort */
	MsgPort *mp = (MsgPort *)CheckItem(imp,KERNELNODE,MSGPORTNODE);
	Task *ct = KernelBase->kb_CurrentTask;
	uint32 oldints;
	Msg *msg;
	DBUGGM(("GetMsg mp=%lx ",(long)imp));
	if (!mp)	ret = BADITEM;
	else
	{
	    if (ct->t.n_Item != mp->mp.n_Owner) ret = NOTOWNER;
	    else
	    {
	        oldints = Disable();
	        msg = (Msg *)RemHead(&mp->mp_Msgs);
	        if (msg)
	        {
	            msg->msg.n_Flags &= ~(MESSAGE_SENT|MESSAGE_REPLIED);
	            msg->msg_MsgPort = CURRENTTASK->t.n_Item;;
	        }
	        Enable(oldints);
	        if (msg)	ret = msg->msg.n_Item;
	        else		ret = 0;
	    }
	}
	DBUGGM(("ret= %lx\n",(long)ret));
	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/waitport
|||	WaitPort - Wait for a message to arrive.
|||
|||	  Synopsis
|||
|||	    Item WaitPort( Item mp, Item msg )
|||
|||	  Description
|||
|||	    The procedure puts the calling task into wait state until a
|||	    message is received at the specified message port.  When a task is
|||	    in wait state, it uses no CPU time.
|||
|||	    The task can wait for a specific message by providing the item
|||	    number of the message
|||
|||	    in the message argument.  To wait for any incoming message, the
|||	    task uses 0 as the
|||
|||	    value of the message argument.
|||
|||	    Note:  If the desired message is already in the message queue for
|||	    the specified message port, the procedure returns immediately.  If
|||	    the message never arrives, the procedure may never return.
|||
|||	    When the message arrives, the task is moved from the wait queue to
|||	    the ready queue, the item number of the message is returned as the
|||	    result, and the message is removed from the message port.
|||
|||	  Arguments
|||
|||	    mp                          The item number of the message port to
|||	                                check for incoming messages.
|||
|||	    msg                         The item number of the message to wait
|||	                                for.  If this argument is 0, control
|||	                                is returned to the task when any
|||	                                message arrives at the specified
|||	                                message port.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the incoming message or
|||	    an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    msgport.h                   ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    GetMsg(), ReplyMsg(), SendMsg()
|||
**/

Item internalWaitPort(MsgPort *port, Msg *msg)
{
    DBUGWP(("internalWaitPort: port $%x, msg $%x\n",port,msg));

    if (msg)
    {
        /* we want to wait for a particular message to arrive */
        while (TRUE)
        {
	    Item it;

            /* is the message on the port? */
            if (msg->msg_MsgPort == port->mp.n_Item)
                return internalGetThisMsg(msg->msg.n_Item);

            /* the message is not on this port, wait for it... */

            /* tell the world we're waiting for this message */
            CURRENTTASK->t_WaitItem = msg->msg.n_Item;
            msg->msg_Waiters++;

	    it = msg->msg.n_Item;

            if (internalWait(port->mp_Signal) & SIGF_ABORT)
            {
                CURRENTTASK->t_WaitItem = -1;
		if (CURRENTTASK->t_Flags & TASK_EXITING)
		{
		    msg->msg_Waiters--;
		    return ABORTED;
		}
                /* someone deleted the message! */
                return BADITEM;
            }

            /* we're done waiting, clear this... */
            msg->msg_Waiters--;
            CURRENTTASK->t_WaitItem = -1;
        }
    }

    /* we want any message that comes into the port */
    while (TRUE)
    {
        if (IsEmptyList(&port->mp_Msgs) == 0)
        {
            /* there are messages on this port */
            return internalGetMsg(port->mp.n_Item);
        }

        /* no messages on the port, wait for some... */
        if (internalWait(port->mp_Signal) & SIGF_ABORT)
	    return ABORTED;
    }
}


/*****************************************************************************/


Item externalWaitPort(Item portItem, Item msgItem)
{
Msg     *msg;
MsgPort *port;

    DBUGWP(("externalWaitPort: portItem $%x, msgItem $%x\n",portItem,msgItem));

    /* check the message port */
    port = (MsgPort *)CheckItem(portItem,KERNELNODE,MSGPORTNODE);
    if (!port)
        return BADITEM;

    /* can only wait if we're the owner of the port */
    if (port->mp.n_Owner != CURRENTTASK->t.n_Item)
        return NOTOWNER;

    /* check the message item */
    if (msgItem != 0)
    {
        msg = (Msg *)CheckItem(msgItem,KERNELNODE,MESSAGENODE);
        if (!msg)
            return BADITEM;
    }
    else
    {
        msg = NULL;
    }

    return internalWaitPort(port,msg);
}


/*****************************************************************************/


/* WaitPort() used to be a user-mode vector. This is so that old code still
 * works, and ends up just doing a SWI into the new code
 */
Item oldWaitPort(Item portItem, Item msgItem)
{
    return WaitPort(portItem,msgItem);
}
