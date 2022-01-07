/* $Id: sendio.c,v 1.110.1.4 1995/01/09 23:40:25 sdas Exp $ */
/* file: sendio.c */

#include "types.h"
#include "list.h"
#include "listmacros.h"
#include "item.h"
#include "msgport.h"
#include "io.h"
#include "kernelnodes.h"
#include "task.h"
#include "kernel.h"
#include "device.h"
#include "driver.h"
#include "operror.h"
#include "stdio.h"
#include "semaphore.h"
#include "setjmp.h"
#include "sherryvers.h"
#include "internalf.h"

#define DBUG(x)	/*printf x*/
#define MBUG(x)	/*printf x*/
#define DBUGCIO(x)	/* printf x */
#define DBUGWIO(x)	/* printf x */
#define DBUGWAIO(x) /* printf x */

extern Node *FindNode(List *list,Node *node);

#ifdef MASTERDEBUG
#define DBUGSIO(x)	if (CheckDebug(KernelBase,24)) printf x
#else
#define DBUGSIO(x)
#endif

#define CHECKIO(ior) (ior->io_Flags & IO_DONE)

Err
SetIOReqOwner(ior,newOwner)
IOReq *ior;
Item newOwner;
{
    if (CHECKIO(ior) == 0)
	return MAKEKERR(ER_SEVER,ER_C_STND,ER_IONotDone);
    if (ior->io_MsgItem != ior->io.n_Owner)
	return NOSUPPORT;	/* can't have a msg there */
    ior->io_MsgItem = newOwner;
    return 0;
}

static int32
icior_c(ior, p, tag, arg)
IOReq  *ior;
void   *p;
uint32 tag;
uint32 arg;
{
	switch (tag)
	{
		case CREATEIOREQ_TAG_REPLYPORT:
			*(Item *)p = arg;
			break;
		case CREATEIOREQ_TAG_DEVICE:
			break;
		default:
			return BADTAG;
	}
	return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/createioreq
|||	CreateIOReq - Create an I/O request.
|||
|||	  Synopsis
|||
|||	    Item CreateIOReq( const char *name, uint8 pri, Item dev, Item mp )
|||
|||	  Description
|||
|||	    This convenience procedure creates an I/O request item.
|||
|||	    When you create an I/O request, you must decide how the device
|||	    will notify you when an I/O operation completes. There are two
|||	    choices:
|||
|||	      - Notification by signal
|||
|||	      - Notification by message
|||
|||	    With notification by signal, the device will send your task the
|||	    SIGF_IODONE signal whenever an I/O operation completes. This is
|||	    a low overhead mechanism, which is also low on information. When
|||	    you get the signal, all you know is that an I/O operation has
|||	    completed. You do not know which operation has completed. This
|||	    has to be determined by looking at the state of all outstanding
|||	    I/O requests.
|||
|||	    Notification by message involve very slightly more overhead, but
|||	    provides much more information. When you create the I/O request,
|||	    you indicate a message port. Whenever an I/O operation completes,
|||	    the device will send a message to that message port. The message
|||	    will contain the following information:
|||
|||	      - msg_Result
|||	        Contains the io_Error value from the I/O request. This
|||	        indicates the state of the I/O operation, whether it worked
|||	        or failed.
|||
|||	      - msg_DataPtr
|||	        Contains the item number of the I/O request that completed.
|||
|||	      - msg_DataSize
|||	        Contains the value of the ioi_User field taken from the
|||	        IOInfo structure used when initiating the I/O operation.
|||
|||	  Arguments
|||
|||	    name                        The name of the I/O request (see
|||	                                `Notes').
|||
|||	    pri                         The priority of the I/O request.  For
|||	                                some device drivers, this value
|||	                                determines the order in which I/O
|||	                                requests are processed.  When in doubt
|||	                                about what value to use, use 0.
|||
|||	    dev                         The item number of the device to which
|||	                                to send the I/O request.
|||
|||	    mp                          If a task wants to receive a message
|||	                                when an I/O request is finished, this
|||	                                argument must be the item number of
|||	                                the message port to receive the
|||	                                message.  If the task wants to receive
|||	                                a signal when an I/O request is
|||	                                finished instead of a message, this
|||	                                argument must be 0.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the new I/O request or
|||	    one of the following error codes if an error occurs:
|||
|||	    BADITEM                     The mp argument was not zero but did
|||	                                not specify a valid message port.
|||
|||	    ER_Kr_ItemNotOpen           The device specified by the dev
|||	                                argument is not open.
|||
|||	    NOMEM                       There was not enough memory to
|||	                                complete the operation.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    io.h                        ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    When you no longer need an I/O request, use DeleteIOReq() to
|||	    delete it.
|||
|||	    You can use FindNamedItem() to find a I/O request by name.  When
|||	    creating I/O requests, you should assign unique names whenever
|||	    possible.
|||
|||	    The Kernel may change the priority of an I/O request to help
|||	    optimize throughput.
|||
|||	  See Also
|||
|||	    DeleteIOReq()
|||
**/

Item
internalCreateIOReq(iordummy,tagpt)
IOReq *iordummy;
TagArg *tagpt;
{
	/* Create an IOReq Item */
	Task   *ct = KernelBase->kb_CurrentTask;
	TagArg tagreturn;
	Item   ret;
	Item   devItem;
	Device *dev;
	Item   replyport = 0;
	Msg    *msg = 0;
	IOReq  *ior = iordummy;

	DBUGCIO(("iCreateIOReq(%lx,%lx)\n",(long)ior,(long)tagpt));

	tagreturn.ta_Arg = (void *)0;
	ret = TagProcessorSearch(&tagreturn, tagpt, CREATEIOREQ_TAG_DEVICE);
	if ((int32)ret < 0) return ret;
	devItem = (Item)tagreturn.ta_Arg;

	dev = (Device *)CheckItem(devItem,KERNELNODE,DEVICENODE);
	DBUGCIO(("Validate CreateIOReq dev=%lx\n", dev));
	if (!dev) return BADITEM;

	if (FindItemSlot(ct,devItem | ITEM_WAS_OPENED) < 0)
	{
		DBUGCIO(("WARNING: Device not open!!!\n"));
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemNotOpen);
	}

	if (ior == (IOReq *)-1)
	{
	    DBUGCIO(("ioreqsize = %d\n",(int)dev->dev_IOReqSize));
	    ior = (IOReq *)AllocateSizedNode(
			(Folio *)KernelBase,IOREQNODE,dev->dev_IOReqSize);
	    DBUGCIO(("ior from AllocateSizeNode=%lx\n",(uint32)ior));
	    if (!ior) return NOMEM;
	    ret = TagProcessor(ior, tagpt, icior_c, &replyport);
	    if (ret < 0) goto err;
	}
	else
	{
	    /* sanity check here */
	    if (dev->dev_IOReqSize > ior->io.n_Size)
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadSize);
	}

	ior->io_Dev = dev;

	DBUGCIO(("Validate CreateIOReq replyport=%lx\n",replyport));
	if (replyport)
	{
	    TagArg IOMsg[5];
	    IOMsg[0].ta_Tag = TAG_ITEM_PRI;
	    IOMsg[0].ta_Arg = (void *)ior->io.n_Priority;
	    IOMsg[1].ta_Tag = CREATEMSG_TAG_REPLYPORT;
	    IOMsg[1].ta_Arg = (void *)replyport;
	    IOMsg[2].ta_Tag = CREATEMSG_TAG_MSG_IS_SMALL;
	    IOMsg[3].ta_Tag = TAG_ITEM_NAME;
	    IOMsg[3].ta_Arg = (void *)ior->io.n_Name;
	    IOMsg[4].ta_Tag = TAG_END;
	    if (ior->io.n_Name == 0) IOMsg[3].ta_Tag = TAG_END;

	    if (internalDiscOsVersion(0) <= DiscOs_1_4)
	    {
	        /* For old titles from Digital Pictures (like Night Trap), we
                 * must not use small messages.
	         */
	        IOMsg[2].ta_Tag = TAG_NOP;
	    }

	    ret = externalCreateMsg((Msg *)-1,IOMsg);
	    DBUGCIO(("msgItem=%lx\n",ret));
	    if ((int32)ret < 0) goto err;
	    msg=(Msg *)LookupItem(ret);

	    msg->msg.n_Owner = ct->t.n_Item;
	    msg->msg_DataPtr = ior;	/* put pointer to IOReq into message */
	    ior->io_MsgItem = msg->msg.n_Item;
	}
	else
	    ior->io_MsgItem = ct->t.n_Item;
	DBUGCIO(("internalCreateIO: devcreate=%lx\n",(long)dev->dev_CreateIOReq));
	if (dev->dev_CreateIOReq)
	{
	    ret = (*dev->dev_CreateIOReq)(ior);
	    if ((int32)ret < 0) goto err;
	}
	ior->io_Flags |= (IO_DONE|IO_QUICK);
	/* This list is not searched */
	ADDTAIL(&dev->dev_IOReqs,(Node *)(&ior->io_Link));
	return ior->io.n_Item;

err:
	if (msg)
	{
	    internalDeleteMsg(msg, ct);
	    FreeNode((Folio *)KernelBase, msg);
	}
	if (ior != iordummy)
	{
	    /* an ior was allocated by this routine */
	    FreeNode((Folio *)KernelBase, ior);
	}
	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/abortio
|||	AbortIO - Abort an I/O operation.
|||
|||	  Synopsis
|||
|||	    Err AbortIO( Item ior )
|||
|||	  Description
|||
|||	    This procedure aborts an I/O operation.  If the I/O operation has
|||	    already completed, calling AbortIO() has no effect.  If it is not
|||	    complete, it will be aborted.
|||
|||	    A task should call WaitIO() immediately after calling AbortIO().
|||	    When WaitIO() returns, the task knows that the I/O operation is no
|||	    longer active.  It can then confirm that the I/O operation was
|||	    aborted before it was finished by checking the io_Error field of
|||	    the IOReq structure.  If the operation aborted, the value of this
|||	    field is ER_Aborted.
|||
|||	  Arguments
|||
|||	    ior                         The item number of the I/O request to
|||	                                abort.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the I/O operation was successfully
|||	    aborted or an error code (a negative value) if an error occurs.
|||	    Possible error codes include:
|||
|||	    BADITEM                     The ior argument is not an IOReq.
|||
|||	    NOTOWNER                    The ior argument is an IOReq but the
|||	                                calling task is not its owner.
|||
|||	  Implementation
|||
|||	    SWI implemented in the kernel folio V20.
|||
|||	  Associated Files
|||
|||	    io.h                        ANSI C Prototype
|||
|||	  See Also
|||
|||	    CheckIO(), CreateIOReq(), DeleteIOReq(), DoIO(), SendIO(),
|||	    WaitIO()
|||
**/

void
internalAbortIO(iorP)
struct IOReq *iorP;
{
	ulong oldints;
	void (*abrt)();
	abrt = iorP->io_Dev->dev_Driver->drv_AbortIO;
	oldints = Disable();
	if ( (iorP->io_Flags & IO_DONE) == 0 ) (*abrt)(iorP);
	Enable(oldints);
}

Err
externalAbortIO(iori)
Item iori;
{
	IOReq *ior = (IOReq *)CheckItem(iori,KERNELNODE,IOREQNODE);
	Task *ct = CURRENTTASK;

	if (ior == 0)	return BADITEM;
	if (ior->io.n_Owner != ct->t.n_Item)	return NOTOWNER;
	internalAbortIO(ior);
	return 0;
}


int32
internalWaitAbortedIO(IOReq *ior)
{
	uint32 oldints;

	DBUGWAIO(("WaitAbortedIO entered, ior=%lx\n",ior));
	if (ior->io_Flags & IO_QUICK)return 0;
	ClearSignals(CURRENTTASK, SIGF_ONESHOT);
	oldints = Disable();
	if (CHECKIO(ior) == 0)
	{
	     ior->io_SigItem = CURRENTTASK->t.n_Item;
	     /* We can't wake up below getting an SIGF_ABORT */
	     /* We have to wait for the io to comlete        */
	     while (!(internalWait(SIGF_ONESHOT) & SIGF_ONESHOT));
	     ior->io_SigItem = 0;
	}
	Enable(oldints);
	return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/deleteioreq
|||	DeleteIOReq - Delete an I/O request.
|||
|||	  Synopsis
|||
|||	    Err DeleteIOReq( Item item )
|||
|||	  Description
|||
|||	    This macro deletes an I/O request item.  You can use this macro in
|||	    place of DeleteItem() to delete the item. If there was any
|||	    outstanding I/O with this IOReq, it will be aborted first.
|||
|||	  Arguments
|||
|||	    item                        The item number of the I/O request to
|||	                                delete.
|||
|||	  Return Value
|||
|||	    The macro returns 0 if the I/O request was successfully deleted or
|||	    an error code (a negative value) if an error occurs.
|||
|||	  Implementation
|||
|||	    Macro implemented in io.h V20.
|||
|||	  Associated Files
|||
|||	    io.h                        ANSI C Macro Definition
|||
|||	  See Also
|||
|||	    CreateIOReq()
|||
**/

int32
internalDeleteIOReq(ior, t)
IOReq *ior;
Task *t;
{
	Device *dev;
	Msg *msg;

	DBUG(("internalDeleteIOreq(%lx,%lx)\n",ior,t));
	if ( (ior->io_Flags & IO_DONE) == 0)
	{
		internalAbortIO(ior);
		internalWaitAbortedIO(ior); 	/* need to wait till it is done! */
		/* Now the IO is done for sure */
	}

	dev = ior->io_Dev;
	if (dev->dev_DeleteIOReq)
	{
		(*dev->dev_DeleteIOReq)(ior);
	}

	REMOVENODE((Node *)&ior->io_Link);
	msg = (Msg *)LookupItem(ior->io_MsgItem);
	if (msg->msg.n_Type == MESSAGENODE)
	{
		DBUG(("Calling internalDeleteMsg\n"));

		internalDeleteMsg(msg,t);
		FreeNode((Folio *)KernelBase,msg);
	}

	if (ior->io_Flags & IO_WAITING)
	{
	    Task *task;

	    /* if the owner of this IO req is currently waiting for it to
	     * return, we must inform him that the IO req have been zapped.
	     */
	    task = (Task *)LookupItem(ior->io.n_Owner);
	    if (task)
                internalSignal(task,SIGF_ABORT);
	}

	return 0;
}

/**
|||	AUTODOC PRIVATE spg/kernel/completeio
|||	CompleteIO - Completes an I/O operation
|||
|||	  Synopsis
|||
|||	     void CompleteIO( IOReq *ior )
|||
|||	  Description
|||
|||	     The task using this swi must be privileged.
|||	     This procedure causes the system to do the proper
|||	     endaction for this iorequest.
|||	     1) Send a message back
|||	     2) Send a signal
|||	     3) Call the callback hook.
|||	     After calling this routine, the device task should
|||	     recheck pending work queues since if the IORequest
|||	     was completed via the callback mechanism there may be
|||	     new work that now needs to be done.
|||
|||	  Arguments
|||
|||	     ior                        Pointer to the IORequest to send back
|||	                                to the user.
|||
|||	  Implementation
|||
|||	     SWI implemented in kernel folio V21.
|||
|||	  Associated Files
|||
|||	     io.h
|||
|||	  See Also
|||
|||	     CheckIO(), CreateIOReq(), DeleteIOReq(), DoIO(),
|||	     SendIO(), WaitIO()
|||
**/

void
internalCompleteIO(ior)
IOReq *ior;
{
again:
    ior->io_Flags |= IO_DONE;
    if (ior->io_CallBack)
    {
	IOReq *newior = (*ior->io_CallBack)(ior);
	if (newior)
	{
	    int32 ret;
	    newior->io_Flags |= IO_INTERNAL;
	    ret = internalSendIO(newior);
	    newior->io_Flags &= ~IO_INTERNAL;
	    if (ret)
	    {	/* Done with this one too! */
		ior = newior;
		goto again;
	    }
	}
    }
    else if ((ior->io_Flags & IO_QUICK) == 0)
    {
	Msg *msg = (Msg *)LookupItem(ior->io_MsgItem);
	Task *t;

	if (msg->msg.n_Type == MESSAGENODE)
	{
	    if (internalDiscOsVersion(0) <= DiscOs_1_4)
	    {
	        /* For old titles from Digital Pictures (like NightTrap), we
	         * must return the result field and data size fields set to 0.
	         */
                internalReplyMsg(msg,0,(void *)ior->io.n_Item,0);
            }
            else
            {
                internalReplyMsg(msg,ior->io_Error,(void *)ior->io.n_Item,ior->io_Info.ioi_User);
            }
	}
	else
	{
	    t = (Task *)LookupItem(ior->io.n_Owner);
	    internalSignal(t,SIGF_IODONE);
	}

	if (ior->io_SigItem)
	{
	    t = (Task *)CheckItem(ior->io_SigItem, KERNELNODE, TASKNODE);
	    if (t) internalSignal(t, SIGF_ONESHOT);
	}
    }
}

void
externalCompleteIO(ior)
IOReq *ior;
{
	/* This task must be privileged to make this call */
	Task *ct = KernelBase->kb_CurrentTask;
	if (ct->t.n_Flags & TASK_SUPER) internalCompleteIO(ior);
	/* ignore otherwise */
}

/**
|||	AUTODOC PUBLIC spg/kernel/waitio
|||	WaitIO - Wait for an I/O request to complete.
|||
|||	  Synopsis
|||
|||	    Err WaitIO( Item ior )
|||
|||	  Description
|||
|||	    The procedure puts the calling task into wait state until the
|||	    specified I/O request completes.  When a task is in wait state, it
|||	    uses no CPU time.
|||
|||	    Note:  If the I/O request has already completed, the procedure
|||	    returns immediately.  If the I/O request never completes and it is
|||	    not aborted, the procedure never returns.
|||
|||	    Starting with kernel folio V24, this function will automatically
|||	    sample the io_Error field of the IO request, and return this to
|||	    you. It is therefore no longer necessary to have code such as:
|||
|||	        err = WaitIO(iorItem);
|||	        if (err >= 0)
|||	            err = ior->io_Error;
|||
|||	    You can now just look at the return value of WaitIO().
|||
|||	  Arguments
|||
|||	    ior                         The item number of the I/O request to
|||	                                wait for.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the I/O request was successful or an
|||	    error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||	    Became a folio call in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    io.h                        ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    AbortIO(), CheckIO(), DoIO(), SendIO()
|||
**/

Err
internalWaitIO(IOReq *ior)
{
    Msg     *msg;
    Err      result;
    MsgPort *port;

    DBUGWIO(("internalWaitIO: ior $%x\n",ior));

    /* done? */
    if (ior->io_Flags & IO_QUICK)
        return ior->io_Error;

    msg = (Message *)LookupItem(ior->io_MsgItem);

    /* io_MsgItem is either a task or a message */
    if (msg->msg.n_Type == MESSAGENODE)
    {
        DBUGWIO(("internalWaitIO: message-based IO req\n"));

        /* Message-based notification. Wait for the message */

        /* Get the port the message will come back to */
        if ((CHECKIO(ior) == 0) || (msg->msg_MsgPort == msg->msg_ReplyPort))
        {
            /* We can safely wait here, it is
             *      a) not done
             *  or  b) on this message port (already done)
             */
            port = (MsgPort *)LookupItem(msg->msg_ReplyPort);
            if (port)
                result = internalWaitPort(port,msg);
            else
                result = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_NoReplyPort);

            if (result < 0)
                return result;
        }

        /* It is done and not on the msgport, therefore it must have already
         * been removed
         */
    }
    else
    {
	uint32 oldints;

        DBUGWIO(("internalWaitIO: signal-based IO req\n"));

        /* signal-based IO, wait for the signal */

        /* tell the world we're waiting on this IO */
        CURRENTTASK->t_WaitItem = ior->io.n_Item;
	oldints = Disable();
	/* The following is not an atomic operation */
	/* and is not interrupt-safe */
        ior->io_Flags |= IO_WAITING;
	Enable(oldints);

        while (CHECKIO(ior) == 0)
        {
            if (internalWait(SIGF_IODONE) & SIGF_ABORT)
            {
                CURRENTTASK->t_WaitItem = -1;
		if (CURRENTTASK->t_Flags & TASK_EXITING)
		{
		    oldints = Disable();
		    ior->io_Flags &= ~IO_WAITING;
		    Enable(oldints);
		    return ABORTED;
		}
                /* someone deleted the IO request! */
                return BADITEM;
            }
        }

	/* we're done waiting, clear IO_WAITING bit */
	/* We don't expect io_Flags being modified */
	/* from interrupt during the operation below */
        ior->io_Flags &= ~IO_WAITING;
        CURRENTTASK->t_WaitItem = -1;
    }

    return ior->io_Error;
}


/*****************************************************************************/


Err
externalWaitIO(Item iorItem)
{
    IOReq *ior;

    DBUGWIO(("externalWaitIO: iorItem $%x\n",iorItem));

    /* check the IOReq item */
    ior = (IOReq *)CheckItem(iorItem,KERNELNODE,IOREQNODE);
    if (!ior)
        return BADITEM;

    /* if this is quick, there's no need to go any further... */
    if (ior->io_Flags & IO_QUICK)
        return ior->io_Error;

    /* can only wait if we're the owner */
    if (ior->io.n_Owner != CURRENTTASK->t.n_Item)
        return NOTOWNER;

    return internalWaitIO(ior);
}


/**
|||	AUTODOC PUBLIC spg/kernel/sendio
|||	SendIO - Request synchronous I/O.
|||
|||	  Synopsis
|||
|||	    Err SendIO( Item ior, const IOInfo *ioiP )
|||
|||	  Description
|||
|||	    This procedure sends an I/O request that is to be executed
|||	    asynchronously.  Because the request is asynchronous, control
|||	    returns to the calling task as soon as the request is sent (unlike
|||	    synchronous I/O, where control doesn't return until after the
|||	    request has been completed).
|||
|||	    Call this procedure after creating the necessary IOReq item (for
|||	    the ior argument, created by calling CreateIOReq() ) and an IOInfo
|||	    structure (for the ioiP argument).  The IOReq item specifies the
|||	    device to which to send the request and the reply port, if any,
|||	    while the IOInfo structure includes the I/O command to be executed
|||	    and a variety of other information.  For descriptions of the IOReq
|||	    and IOInfo data structures, see the `Data Structures and Variable
|||	    Types' chapter.
|||
|||	    To request `quick I/O,' set the IO_QUICK flag in the ioi_Flags
|||	    field of the IOInfo structure.  Quick I/O works as follows: The
|||	    Kernel tries to perform the I/O operation immediately; if it is
|||	    successful, it sends back the resulting IOReq item immediately
|||	    without setting any signal bits or sending a message.  If quick
|||	    I/O was successful, the IO_QUICK bit in the io_Flags field of the
|||	    IOReq is set.  If quick I/O was not successful, the Kernel
|||	    performs normal asynchronous I/O and notifies the task with a
|||	    signal or message when the I/O request is complete.
|||
|||	    The IOInfo structure must be fully initialized before calling
|||	    this function. You can use the ioi_User field of the IOInfo
|||	    structure to contain whatever you want. This is a useful
|||	    place to store a pointer to contextual data that needs to be
|||	    associated with the I/O request. If you use message-based
|||	    notification for your I/O requests, the msg_DataSize field of
|||	    the notification messages will contain the value of ioi_User
|||	    from the IOInfo structure.
|||
|||	  Arguments
|||
|||	    ior                         The item number of the IOReq structure
|||	                                for the request.  This structure is
|||	                                normally created by calling
|||	                                CreateIOReq().
|||
|||	    ioiP                        A pointer to an IOInfo structure.
|||
|||	  Return Value
|||
|||	    The procedure returns 1 if the I/O was completed immediately or 0
|||	    if the I/O request is still in progress (the task will be notified
|||	    with either a signal or message when the request is complete,
|||	    depending on what you specified when you called CreateIOReq()).
|||	    It returns an error code (a negative value) if an error occurs.
|||	    Possible error codes include:
|||
|||	    BADITEM                     The ior argument does not specify an
|||	                                IOReq.
|||
|||	    NOTOWNER                    The I/O request specified by the ior
|||	                                argument is not owned by this task.
|||
|||	    ER_IONotDone                The I/O request is already in
|||	                                progress.
|||
|||	    BADPTR                      A pointer is invalid: Either the
|||	                                IOInfo structure specified by the ioiP
|||	                                argument is not entirely within the
|||	                                task's memory, the IOInfo receive
|||	                                buffer (specified by the ioi_Recv
|||	                                field in the IOInfo structure) is not
|||	                                entirely within the task's memory, or
|||	                                the IOInfo send buffer (specified by
|||	                                the ioi_Send field in the IOInfo
|||	                                structure) is not in legal memory.
|||
|||	    BADIOARG                    One or more reserved I/O flags are set
|||	                                (either reserved flags in the
|||	                                ioi_Flags field of the IOInfo
|||	                                structure or any of the flags in the
|||	                                ioi_Flags2 field of the IOInfo
|||	                                structure).
|||
|||	    BADUNIT                     The unit specified by the ioi_Unit
|||	                                field of the IOInfo structure is not
|||	                                supported by this device.
|||
|||	    If quick I/O occurs, the IO_QUICK flag is set in the io_Flags
|||	    field of the IOReq structure.
|||
|||	    If the ior and ioiP arguments were valid but an error occurred
|||	    during the I/O operation, an error code is returned in the
|||	    io_Error field of the IOReq structure.  If SendIO() returns 0 and
|||	    a error occurs during I/O, the IOReq is returned as if it were
|||	    completed, and it contains the error code in io_Error of the IOReq
|||	    structure.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    io.h                        ARM C "swi" declaration
|||
|||	  Caveats
|||
|||	    SendIO() returns only BADITEM, NOTOWNER, and ER_IONotDone.  All
|||	    other errors codes are returned in the io_Error field of the
|||	    IOReq.
|||
|||	  See Also
|||
|||	    AbortIO(), CheckIO(), DoIO(), WaitIO()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/doio
|||	DoIO - Perform synchronous I/O.
|||
|||	  Synopsis
|||
|||	    Err DoIO( Item ior, const IOInfo *ioiP )
|||
|||	  Description
|||
|||	    This procedure is the most efficient way to perform synchronous
|||	    I/O (I/O in which the task waits for the I/O request to complete
|||	    before continuing).  It automatically requests quick I/O (see
|||	    below), sends the I/O request, and then puts the calling task into
|||	    wait state until the request is complete.
|||
|||	    DoIO() automatically sets the IO_QUICK flag in the IOInfo
|||	    structure.  This flag specifies `quick I/O,' which works as
|||	    follows: The Kernel tries to perform the I/O operation immediately
|||	    and, if it is successful, it sends back the resulting IOReq
|||	    structure immediately and returns 1 as the result code.  The
|||	    calling task can then get the necessary information from the
|||	    IOReq.  If quick I/O is not successful, the Kernel then performs
|||	    normal asynchronous I/O and notifies the task with a signal or
|||	    message when the I/O request is complete.  This wakes up the task
|||	    and DoIO then returns.
|||
|||	    Starting with kernel folio V24, this function will automatically
|||	    sample the io_Error field of the IO request, and return this to
|||	    you. It is therefore no longer necessary to have code such as:
|||
|||	        err = DoIO(iorItem,&ioinfo);
|||	        if (err >= 0)
|||	            err = ior->io_Error;
|||
|||	    You can now just look at the return value of DoIO().
|||
|||	  Arguments
|||
|||	    ior                         The item number of the I/O request to
|||	                                use
|||
|||	    ioiP                        A pointer to the IOInfo structure
|||	                                containing the I/O command to be
|||	                                executed, input and/or output data,
|||	                                and other information.  For more
|||	                                information about this structure, see
|||	                                the `Data Structures and Variable
|||	                                Types' chapter.
|||
|||	  Return Value
|||
|||	    The procedure returns when the I/O was successful (which means
|||	    that the IOReq structure has already been returned and its
|||	    contents can be examined).
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||	    Became a SWI in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    io.h                        ANSI C Prototype
|||
|||	  See Also
|||
|||	    AbortIO(), CheckIO(), CreateIOReq(), DeleteIOReq(), SendIO(),
|||	    WaitIO()
|||
**/

int32
internalSendIO(ior)
IOReq *ior;
{
	Driver *drvr;
	Device *dev;
	int32 ret;
	IOInfo *ioi = &ior->io_Info;

	DBUGSIO(("internalSendio(%lx,%lx)\n",(uint32)ior,(uint32)ioi));

	dev = ior->io_Dev;
	drvr = dev->dev_Driver;

	DBUGSIO(("cmd=%d dev=%s\n",ior->io_Info.ioi_Command,dev->dev.n_Name));

	ior->io_Error = 0;
	ior->io_Actual = 0;
	ior->io_Flags &= ~(IO_DONE|IO_QUICK);
	if (ioi->ioi_Flags & IO_QUICK) ior->io_Flags |= IO_QUICK;
	ret = (*drvr->drv_DispatchIO)(ior);
	DBUGSIO(("Dispatch returns: %d\n",(int)ret));

	/* ret == 0, if deferred */
	/* ret == 1, if done */
	return ret;
}

static int32
StartIO(IOReq *ior, IOInfo *ioi, bool wait)
{
	Task *ct = KernelBase->kb_CurrentTask;
	Device *dev;
	jmp_buf jb,*old_co;
	uint32 ret;

	DBUGSIO(("StartIO(%lx,%lx) %lx\n",ior,(uint32)ioi));
	DBUGSIO(("Owner=%lx ct=%lx\n",(uint32)ior->io.n_Owner,ct->t.n_Item));

	if (!ior) {
		ret =  BADITEM;
		goto send_exit;
	}

	ior->io.n_ReservedP = 0;

	if (ior->io.n_Owner != ct->t.n_Item) {
		ret = NOTOWNER;
		goto send_exit;
	}
	if ( (ior->io_Flags & IO_DONE) == 0) {
		ret = MAKEKERR(ER_SEVER,ER_C_STND,ER_IONotDone);
		goto send_exit;
	}
	dev = ior->io_Dev;	/* guaranteed to always be good */

	/* should we always do this? */
	/*ior->io.n_Priority = ct->t.n_Priority;*/

	/* catch memory errors */
	old_co = KernelBase->kb_CatchDataAborts;
	KernelBase->kb_CatchDataAborts = &jb;
	if (setjmp(jb))	{
	    	KernelBase->kb_CatchDataAborts = old_co;
		ret =  BADPTR;
		goto send_exit;
	}
	/* copy the information from the users ioinfo to the */
	/* ioinfo in the ioreq */

	ior->io_Info = *ioi;

	KernelBase->kb_CatchDataAborts = old_co;

	if (wait)
	    ior->io_Info.ioi_Flags |= IO_QUICK;

	/* make sure flags2 field is clear for this release */
	if (ior->io_Info.ioi_Flags2 != 0) {
#ifndef ROMBUILD
		printf("Error: non-null ioi_Flags2 field value: %d\n",
			(int)ior->io_Info.ioi_Flags2);
#endif
	    	ret =  BADIOARG;
		goto send_exit;
	}

	if ((ior->io_Info.ioi_Flags & ~IO_QUICK) != 0) {
#ifndef ROMBUILD
		printf("Error: unknown ioi_Flags set\n");
#endif
	    	ret =  BADIOARG;
		goto send_exit;
	}

	if (ior->io_Info.ioi_Unit > dev->dev_MaxUnitNum) {
		ret = BADUNIT;
		goto send_exit;
	}
	ior->io_CallBack = 0;

	/* preliminary validation of recv pointers */
	/* data will be dma'd , sent by supercode here so we must */
	/* make sure this task owns the memory that is going to be */
	/* changed */

	if (ior->io_Info.ioi_Recv.iob_Len)
	{
	    if ( ValidateMem(ct,(uint8 *)ior->io_Info.ioi_Recv.iob_Buffer,
				ior->io_Info.ioi_Recv.iob_Len) < 0)
	    {
#ifdef DEVELOPMENT
		Task *t = (Task *)LookupItem(ior->io.n_Owner);
		printf("ValidateMem returns bad in sendio\n");
		printf("Recvptr=%lx Recvlen=%ld\n",
				(int32)ior->io_Info.ioi_Recv.iob_Buffer,
				ior->io_Info.ioi_Recv.iob_Len);
		printf("device=%s\n",dev->dev.n_Name);
		printf("Owner=%s\n",t->t.n_Name);
#endif
	    	ret = BADPTR;
		goto send_exit;
	    }
	}
	if (ior->io_Info.ioi_Send.iob_Len)
	{
	    if ( IsRamAddr((uint8 *)ior->io_Info.ioi_Send.iob_Buffer,
				ior->io_Info.ioi_Send.iob_Len) == 0)
	    {
#ifdef DEVELOPMENT
		Task *t = (Task *)LookupItem(ior->io.n_Owner);
		printf("IsRam returns bad read addr in sendio\n");
		printf("Sendptr=%lx sendlen=%ld\n",(int32)ior->io_Info.ioi_Send.iob_Buffer,
				ior->io_Info.ioi_Send.iob_Len);
		printf("device=%s\n",dev->dev.n_Name);
		printf("Owner=%s\n",t->t.n_Name);
#endif
	    	ret =  BADPTR;
		goto send_exit;
	    }
	}
	return internalSendIO(ior);

send_exit:
	return ret;
}

/* called in supervisor mode */
Err
internalDoIO(IOReq *ior)
{
    Err ret;

    ret = internalSendIO(ior);

    /* Wait only if the IO was deferred (done asynchronously) */
    if (ret == 0)
        ret = internalWaitIO(ior);

    if (ret >= 0)
        ret = ior->io_Error;

    return ret;
}

/* called in supervisor mode */
Err
externalDoIO(Item iorItem, IOInfo *ioInfo)
{
    Err    ret;
    IOReq *ior;

    ior = (IOReq *)LookupItem(iorItem);
    ret = StartIO(ior, ioInfo, TRUE);

    /* Wait only if the IO was deferred (done asynchronously) */
    if (ret == 0)
        ret = internalWaitIO(ior);

    if (ret >= 0)
        ret = ior->io_Error;

    return ret;
}

/* called in supervisor mode */
int32
externalSendIO(Item iorItem, IOInfo *ioInfo)
{
    return StartIO((IOReq *)LookupItem(iorItem), ioInfo, FALSE);
}

/**
|||	AUTODOC PUBLIC spg/kernel/checkio
|||	CheckIO - Check for I/O request completion
|||
|||	  Synopsis
|||
|||	    int32 CheckIO( Item ior )
|||
|||	  Description
|||
|||	    This procedure checks to see if an I/O request has completed.
|||
|||	  Arguments
|||
|||	    ior                         The item number of the I/O request to
|||	                                be checked.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the I/O is not complete.
|||	    It returns >0 if it is complete. It returns BADITEM if ior is
|||	    bad.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||	    Became a folio call in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    io.h                        ANSI C Prototype
|||
|||	  See Also
|||
|||	    AbortIO(), CreateIOReq(), DeleteIOReq(), DoIO(), SendIO(),
|||	    WaitIO()
|||
**/

/* user-mode vector */
int32
externalCheckIO(Item iorItem)
{
IOReq *ior;

#ifdef DEVELOPMENT
    ior = (IOReq *)CheckItem(iorItem,KERNELNODE,IOREQNODE);
#else
    ior = (IOReq *)LookupItem(iorItem);
#endif

    if (!ior)
        return BADITEM;

    return CHECKIO(ior);
}

extern Semaphore *DevSemaphore;

void
AbortIOReqs(t,p,len)
Task *t;
uint8 *p;
int32 len;
{
	Device *d;
	List *l = KernelBase->kb_Devices;

	MBUG(("AbortIOReqs(t=%lx p=%lx l=%d\n",t,p,len));

	internalLockSemaphore(DevSemaphore,SEM_WAIT);

rescan:
	for (d = (Device *)FIRSTNODE(l); ISNODE(l,d); d = (Device *)NEXTNODE(d))
	{
	    uint32 oldints;
	    List *iol = &d->dev_IOReqs;
	    Node *n;
	    IOReq *ior;
	    oldints =  Disable();
	    for (n = (Node *)FIRSTNODE(iol); ISNODE(iol,n); n = NEXTNODE(n) )
	    {
		IOInfo *ioi;
		ior = IOReq_Addr(n);

		/* if t == 0, abort IOReq (using mem<p,p+len>) for everybody */
		if (t)
		{
		    /* Abort IOReq (using mem<p,p+len>) for the whole */
		    /* task family, as it shares the same memory and  */
		    /* any memory permission change should affect all */
		    /* members of the task family equally             */
		    Task *it = (Task *)LookupItem(ior->io.n_Owner);
		    if (!IsSameTaskContext(t, it)) continue;
		}
		if  (ior->io_Flags & IO_DONE)	continue;
		/* We only need to check the writes */
		ioi = &ior->io_Info;
		if (ioi->ioi_Recv.iob_Len)
		{
		    /* recv buffer start later? */
		    if ((int)p >=
			  ioi->ioi_Recv.iob_Len + (int)ioi->ioi_Recv.iob_Buffer)
			continue;
		    if ((int)p + len <= (int)ioi->ioi_Recv.iob_Buffer)
			continue;

                    Enable(oldints);

		    /* We found an IOReq to be aborted */
		    internalAbortIO(ior);	/* Abort the IOR */
		    internalWaitAbortedIO(ior);	/* Wait for the IOR to finish */
		    goto rescan;
		}
	    }
	    Enable(oldints);
	}

	internalUnlockSemaphore(DevSemaphore);
}

Node *
FindNode(List *list,Node *node) {
    Node *n;

    for (n = (Node *)FIRSTNODE(list); ISNODE(list,n); n = NEXTNODE(n) ) {
	if( (uint32)node == (uint32)n) return n;
    }
    return 0;
}


