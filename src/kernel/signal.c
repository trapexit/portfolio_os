/* $Id: signal.c,v 1.44 1995/02/14 01:04:11 limes Exp $ */

/* file: signal.c */

extern void VBDelay(int);

#define DBUG(x)	 /*printf x*/
#define INFO(x) printf x

#ifdef MASTERDEBUG
#define DBUGWAIT(x)	if (CheckDebug(KernelBase,1)) printf x
#define DBUGSIGNAL(x)	if (CheckDebug(KernelBase,2)) printf x
#else
#define DBUGWAIT(x)
#define DBUGSIGNAL(x)
#endif

#include "types.h"

#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "listmacros.h"
#include "task.h"
#include "kernel.h"
#include "operror.h"
#include "stdio.h"
#include "mem.h"

extern void Panic(int halt,char *);

Task *Land(Task *);

#ifdef MULTIT
#include "semaphore.h"
extern Semaphore *SwiSemaphore;
#endif

#define ILLEGAL_SIG		0x80000000
#define SUPER_SIGS		0x000000FF
#define RESERVED_SIGBITS	(ILLEGAL_SIG|SUPER_SIGS)

#include "internalf.h"

void
Switch()
{
    /* let higher priority tasks run */
    Task *t = CURRENTTASK;
    List *ReadyQ = KernelBase->kb_TaskReadyQ;
    Task *nt;
    uint32 oldints;
    oldints = Disable();
    nt = (Task *)FIRSTNODE(ReadyQ);
    if (t->t.n_Priority  <= nt->t.n_Priority)
    {
	TailInsertNode(ReadyQ,(Node *)t);
	if (ULand(t) == 0)
	{
	    	/* get top task on list */
	    	t = (Task *)RemHead(ReadyQ);
	    	ULaunch(t);	/* reeneables interrupts, does not return */
	    	/* ULaunch is like a goto */
	}
	/* back to original caller */
    }
    Enable(oldints);
}

/**
|||	AUTODOC PUBLIC spg/kernel/waitsignal
|||	WaitSignal - Wait until a signal is sent.
|||
|||	  Synopsis
|||
|||	    int32 WaitSignal( uint32 sigMask )
|||
|||	  Description
|||
|||	    This procedure puts the calling task into wait state until any of
|||	    the signal(s) specified in the sigMask have been received.  When a
|||	    task is in wait state, it uses no CPU time.
|||
|||	    When WaitSignal() returns, bits set in the result indicate which
|||	    of the signal(s) the task was waiting for were received since the
|||	    last call to WaitSignal().  (The SIGF_ABORTED bit is also set if
|||	    that signal was received, even if it is not in the signal mask.)
|||	    If the task was not waiting for certain signals, the bits for
|||	    those signals remain set in the task's signal word, and all other
|||	    bits in the signal word are cleared.
|||
|||	    See AllocSignal for a description of the implementation of
|||	    signals.
|||
|||	  Arguments
|||
|||	    sigMask                     A mask in which bits are set to
|||	                                specify the signals the task wants to
|||	                                wait for.
|||
|||	  Return Value
|||
|||	    The procedure returns a mask that specifies which of the signal(s)
|||	    a task was waiting for have been received or an error code (a
|||	    negative value) if an error occurs.  Possible error codes include:
|||
|||	    ILLEGALSIGNAL               One or more of the signal bits in the
|||	                                sigMask argument was not allocated by
|||	                                the task.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    task.h                      ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    Because it is possible for tasks to send signals in error, it is
|||	    up to tasks to confirm that the actual event occurred when they
|||	    receive a signal.
|||
|||	    For example, if you were waiting for SIGF_IODONE and the return
|||	    value from WaitSignal indicated that the signal was sent, you
|||	    should still call CheckIO using the IOReq to make sure it is
|||	    actually done.  If it was not done you should go back to
|||	    WaitSignal.
|||
|||	  See Also
|||
|||	    SendSignal(), WaitSignal()
|||
**/

int32
internalWait(bits)
int32 bits;
{
	/* Called via swi */
	Task *t = CURRENTTASK;
	Task *nt;
	uint32 oldints;
	uint32 res;
	uint8 flgs;
	if (bits & (~t->t_AllocatedSigs)) {
		return ILLEGALSIGNAL;
	}
	bits |= SIGF_ABORT; /* we can always be aborted */
	oldints = Disable();
	res = bits & t->t_SigBits;

	DBUGWAIT(("Wait(%lx): enter res=%lx task=%lx\n",bits,res,t));
	if (res)
	{
		/* Clear the bits we just pinged off of */
		t->t_SigBits &= ~res;
		Enable(oldints);
		return res;
	}
	/* Have to put current task to sleep */
	t->t_WaitBits = bits;


	/* Priority unimportant in WaitQ */
	ADDTAIL(KernelBase->kb_TaskWaitQ,(Node *)t);
	/* Save context */
	DBUGWAIT(("Wait calling Land\n"));
	/* While we are in supervisor mode */
	/* there is no task switching of the current task */
	/* although things can be removed from the WaitQ */
	/* and put on the ReadyQ, and the ReadyQ may get */
	/* shuffled */
	if (ULand(t))
	{
	    /* process has returned in supervisor mode*/
	    /* but using the users stack */
	    DBUGWAIT(("Wait: Uland returned\n"));
	    DBUGWAIT(("isUser=%d\n",isUser()));
	    res = bits & t->t_SigBits;	/* recompute */
	    t->t_SigBits &= ~res;	/* clear bits returning */
#ifdef MULTIT
	    internalLockSemaphore(SwiSemaphore,1);
#endif

	    Enable(oldints);
	    return res;
	}
	/* undo what CLand does because we are now in the wait q */
	/* not the readyQ */
	flgs =  t->t.n_Flags;
	flgs |= TASK_WAITING;
	flgs &= ~TASK_READY;
	t->t.n_Flags = flgs;

#ifdef MULTIT
	internalUnlockSemaphore(SwiSemaphore);
#endif

	/* let pending interrupts in for just a sec */
	Enable(oldints);
	oldints = Disable();

	nt = t;
	/* Get next task */
	t = (Task *)RemHead(KernelBase->kb_TaskReadyQ);
	if (t)	{
	    /* This does not return */
	    ULaunch(t);	/* reeneables interrupts */
	}
	/* Can never get here */
#ifdef DEVELOPMENT
	Enable(oldints);
	printf("Error, where did the idle task go?\n");
	while (1);
#else
	Panic(1,"Idle Task missing\n");
#endif

	return 1;	/* to keep compiler happy */
}

/**
|||	AUTODOC PUBLIC spg/kernel/allocsignal
|||	AllocSignal - Allocate signals
|||
|||	  Synopsis
|||
|||	    int32 AllocSignal( uint32 sigMask )
|||
|||	  Description
|||
|||	    One of the ways tasks communicate is by sending signals to each
|||	    other.  Signals are 1-bit flags that indicate that a particular
|||	    event has occurred.
|||
|||	    Tasks that send and receive signals must agree on which signal
|||	    bits to use and the meanings of the signals.  Except for system
|||	    signals, there are no conventions for
|||
|||	    the meanings of individual signal bits; it is up to software
|||	    developers to define their meanings.
|||
|||	    You allocate bits for new signals by calling AllocSignal().  To
|||	    define one signal at a time -- by far the most common case -- you
|||	    call AllocSignal() with 0 as the argument:
|||
|||	    theSignal = AllocSignal( 0 )
|||
|||	    This allocates the next unused bit in the signal word.  In the
|||	    return value, the bit that was allocated is set.  If the
|||	    allocation fails (which happens if all the non-reserved bits in
|||	    the signal word are already allocated), the procedure returns 0.
|||
|||	    In rare cases, you may need to define more than one signal with a
|||	    single call.  You do this by creating a uint32 value and setting
|||	    any bits you want to allocate for new signals, then calling
|||	    AllocSignal() with this value as the argument.  If all the signals
|||	    are successfully allocated, the bits set in the return value are
|||	    the same as the bits that were set in the argument.
|||
|||	    Signals are implemented as follows:
|||
|||	    *   Each task has a 32-bit signal mask that specifies the signals
|||	        it understands.  Tasks allocate bits for new signals by
|||	        calling AllocSignal().  The bits are numbered from 1 (the
|||	        least significant bit) to 32 (the most significant bit).  Bits
|||	        1 through 8 are reserved for system signals (signals sent by
|||	        the Kernel to all tasks); remaining bits can be allocated for
|||	        other signals.  Note: Bit 32 is also reserved for the system.
|||	        It is set when the Kernel returns an error code to a task
|||	        instead of signals.  For example, trying to allocate a system
|||	        signal or signal number 32.
|||
|||	    *   A task calls SendSignal() to send one or more signals to
|||	        another task.  Each bit set in the signalWord argument
|||	        specifies a signal to send.  Normally, only one signal is sent
|||	        at a time.
|||
|||	    *   When SendSignal() is called, the Kernel gets the incoming
|||	        signal word and ORs it into the received signal mask of the
|||	        target task.  If the task was in the wait queue, it sends the
|||	        received signals with the WaitSignalMask.  If there are any
|||	        bits set in the target, the task is moved from the wait queue
|||	        to the ready queue.
|||
|||	    *   If the SIGF_ABORTED system signal is sent to the task, the
|||	        corresponding bit in the task's signal word is automatically
|||	        set.  This signal cannot be masked.
|||
|||	    *   A task gets incoming signals by calling WaitSignal().  If any
|||	        bits are set in the task's signal-word register, WaitSignal()
|||	        returns immediately.  If no bits are set in the task's
|||	        signal-word register, the task remains in wait state until a
|||	        signal arrives that matches one of the signals the task is
|||	        waiting for.
|||
|||	    The following system signals are currently defined:
|||
|||	    SIGF_MEMLOW                 Informs the task that there is little
|||	                                memory left in the system-wide free
|||	                                memory pool.  When a task receives
|||	                                this signal, it should call
|||	                                ScavengeMem() to return unused memory
|||	                                pages to the system-wide free memory
|||	                                pool. This capability is currently not
|||	                                implemented, so the kernel never sends
|||	                                this signal.
|||
|||	    SIGF_MEMGONE                Informs the task that there is no more
|||	                                memory in the system-wide free memory
|||	                                pool.  When a task receives this
|||	                                signal, it must call ScavengeMem()
|||	                                immediately. This capability is
|||	                                currently not implemented, so the
|||	                                kernel never sends this signal.
|||
|||	    SIGF_ABORT                  Informs the task that the current
|||	                                operation has been aborted.
|||
|||	    SIGF_IODONE                 Informs the task that an asynchronous
|||	                                I/O request is complete.
|||
|||	    SIGF_DEADTASK               Informs the task that one of its child
|||	                                tasks or threads has been deleted.
|||	                                Note:  This signal does not specify
|||	                                which task was deleted.  To find this
|||	                                out, the parent task must check to see
|||	                                which of its child tasks still exist.
|||
|||	    SIGF_SEMAPHORE              Informs a task waiting to lock a
|||	                                semaphore that it can do so.
|||	                                Note:  This signal is for system
|||	                                internal use only.
|||
|||	  Arguments
|||
|||	    signalMask                  An uint32 value in which the bits to
|||	                                be allocated are set, or 0 to allocate
|||	                                the next available bit.  You should
|||	                                use 0 whenever possible (see `Notes').
|||
|||	  Return Value
|||
|||	    The procedure returns a int32 value with bits set for any bits
|||	    that were allocated or 0 if not all of the requested bits could be
|||	    allocated.  It returns ILLEGALSIGNAL if the signalMask specified a
|||	    reserved signal.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    task.h                      ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    Use FreeSignal() to deallocate signals.
|||
|||	    Future versions of Portfolio may define additional system signals.
|||	    To help ensure that the signal bits you allocate in current
|||	    applications do not conflict with future system signals, you
|||	    should always use 0 as the value of the signalMask argument when
|||	    allocating signals.  If you must allocate specific signal bits
|||	    (which is strongly discouraged), use bits 17-31.
|||
|||	  See Also
|||
|||	    FreeSignal(), GetCurrentSignals(), SendSignal(), WaitSignal()
|||
**/

int32
internalAllocSignal(sig)
int32 sig;
{
	struct Task *t = KernelBase->kb_CurrentTask;
	uint32 res;
	uint32 cursigs = t->t_AllocatedSigs;
	if (sig)
	{	/* requesting a particular signal */
		if (sig & RESERVED_SIGBITS)	return ILLEGALSIGNAL;
		if (sig & cursigs) return 0;
		t->t_AllocatedSigs = sig | cursigs;
		ClearSignals(t, sig);	/* clear the allocated signal */
		return sig;
	}
	/* get the first free one there is */

	/* From Martin Taillefer */
	/* side affect, bits are allocated right to left instead */
	/* of left to right */
	cursigs = ~cursigs;
	res = ~(cursigs&(cursigs-1));	/* get another bit */
	if ((int32)res < 0)
	{
	    res = 0;	/* no bits available */
	}
	else
	{
	    t->t_AllocatedSigs = res;
	    res = res & cursigs;
	}

	ClearSignals(t, res);	/* clear the allocated signal */
	return res;
}


/**
|||	AUTODOC PUBLIC spg/kernel/freesignal
|||	FreeSignal - Free signals.
|||
|||	  Synopsis
|||
|||	    Err FreeSignal( uint32 sigMask )
|||
|||	  Description
|||
|||	    This procedure frees one or more signal bits allocated by
|||	    AllocSignal().  The freed bits can then be reallocated.
|||
|||	    For information about signals, see the description of the
|||	    AllocSignal() procedure and the `Communicating Among Tasks'
|||	    chapter in the 3DO Portfolio Programmer's Guide.
|||
|||	  Arguments
|||
|||	    sigMask                     A 32-bit value in which any signal
|||	                                bits to deallocate are set.  The bits
|||	                                are numbered from 1 (the least
|||	                                significant bit) to 32 (the most
|||	                                significant bit).  Bits 1 through 8
|||	                                and bit 32 cannot be freed.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the signal(s) were freed successfully
|||	    or an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    task.h                      ANSI C Macro definition
|||
|||	  See Also
|||
|||	    AllocSignal(), WaitSignal(), SendSignal()
|||
**/

Err
internalFreeSignal(int32 sigs,Task *t)
{
	uint32 cursigs = t->t_AllocatedSigs;
	if (sigs & RESERVED_SIGBITS)	return ILLEGALSIGNAL;
	if ((~cursigs) & sigs)	return ILLEGALSIGNAL;
	cursigs &= ~sigs;
	t->t_AllocatedSigs = cursigs;
	ClearSignals(t, sigs);		/* clear the freed signals */
    	return 0;
}

Err externalFreeSignal(int32 sigs) {
	struct Task *t = KernelBase->kb_CurrentTask;

	return internalFreeSignal(sigs,t);
}



Err
internalSignal(t,bits)
Task *t;
int32 bits;
{
	/* Callable from interrupts */
	uint32 oldints;
	DBUGSIGNAL(("Internal Signal(%lx,%lx) ct=%lx\n",(uint32)t,bits,(uint32)CURRENTTASK));
	oldints = Disable();
	if (bits & (~t->t_AllocatedSigs))
	{
	    Enable(oldints);
	    return ILLEGALSIGNAL;
	}
	t->t_SigBits |= bits;
	/* This only affects tasks on the WaitQ */
	if (t->t.n_Flags & TASK_WAITING)
	{
		uint32 res;
		res = t->t_SigBits & t->t_WaitBits;
		if (res)
		{	/* Got what we are looking for */

		    /* if we're here with no stack, we're a Zombie getting a signal */
		    /* its best just to ignore things like that. */

		    if(t->t_SuperStackBase) {
			Task *me = CURRENTTASK;
			REMOVENODE((Node *)t);
			t->t.n_Flags |= TASK_READY;
			t->t.n_Flags &= ~TASK_WAITING;
			TailInsertNode(KernelBase->kb_TaskReadyQ,(Node *)t);
			if (me && (me->t.n_Priority < t->t.n_Priority))
			{
				/* We must retire current task */
				KernelBase->kb_PleaseReschedule = 1;
			}
		    }
		}
	}
	Enable(oldints);
        return 0;
}

Err
ValidateSignal(bits)
int32 bits;
{
    if (bits & SUPER_SIGS)
    {
	if ((CURRENTTASK->t.n_Flags & TASK_SUPER) == 0)
	{
	    INFO(("Bad priv for that signal\n"));
	    return BADPRIV;
	}
    }
    if (bits & ILLEGAL_SIG) return ILLEGALSIGNAL;

    return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/sendsignal
|||	SendSignal - Send a signal to another task.
|||
|||	  Synopsis
|||
|||	    Err SendSignal( Item task, uint32 sigMask )
|||
|||	  Description
|||
|||	    This procedure sends one or more signals to the specified task.
|||	    See the description of AllocSignal() for more information about
|||	    signals.
|||
|||	    It is an error for a user task to send a system signal or a signal
|||	    that has not been allocated by the receiving task.
|||
|||	  Arguments
|||
|||	    task                        The item number of the task to send
|||	                                signals to. If this parameter is 0,
|||	                                then the signals are sent to the
|||	                                calling task. This is sometimes useful
|||	                                to set initial conditions.
|||
|||	    sigMask                     The signals to send.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.  Possible error codes include:
|||
|||	    BADPRIV                     The task attempted to send a system
|||	                                signal.
|||
|||	    ILLEGALSIGNAL               The task attempted to send a signal to
|||	                                a task that was not allocated by that
|||	                                task, or bit 32 in the sigMask
|||	                                argument (which is reserved by the
|||	                                system software) was set.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    task.h                      ARM C "swi" declaration
|||
|||	  See Also
|||
|||	    AllocSignal(), FreeSignal(), GetCurrentSignals(), WaitSignal()
|||
**/

Err
externalSignal(it,bits)
Item it;
int32 bits;
{
    /* NOT Callable from interrupts with it == 0 */

    Task *t;
    Err  ret;

    if (it == 0)
    {
	t = CURRENTTASK;
        DBUGSIGNAL(("External Signal(%lx,%lx) ct=%lx\n",(uint32)t,bits,(uint32)CURRENTTASK));
    }
    else
    {
	t = (Task *)CheckItem(it,KERNELNODE,TASKNODE);
        DBUGSIGNAL(("External Signal(%lx,%lx) ct=%lx\n",(uint32)t,bits,(uint32)CURRENTTASK));
	if (!t)	return BADITEM;
    }

    ret = ValidateSignal(bits);

    if (ret >= 0)
	ret = internalSignal(t,bits);

    return ret;
}
