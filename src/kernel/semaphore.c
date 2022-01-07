/* $Id: semaphore.c,v 1.46 1994/11/17 20:56:46 vertex Exp $ */
/*  file: semaphore.c */

#define DBUG(x)	/*printf x*/
#define DBUGCS(x)	/*printf x*/


#ifdef MASTERDEBUG
#define DBUGLS(x)	if (CheckDebug(KernelBase,7)) printf x
#define DBUGUS(x)	if (CheckDebug(KernelBase,6)) printf x
#else
#define DBUGLS(x)
#define DBUGUS(x)
#endif


#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "listmacros.h"
#include "folio.h"
#include "task.h"

#include "mem.h"
#include "kernel.h"

#include "semaphore.h"
#include "operror.h"

#include "internalf.h"


/**
|||	AUTODOC PUBLIC spg/kernel/createsemaphore
|||	CreateSemaphore - Create a semaphore.
|||
|||	  Synopsis
|||
|||	    Item CreateSemaphore( const char *name, uint8 pri )
|||
|||	  Description
|||
|||	    This convenience procedure creates a semaphore item with the
|||	    specified name and priority.  You can use this procedure in place
|||	    of CreateItem() to create the semaphore.
|||
|||	    For information about semaphores, which are used to control access
|||	    to shared resources, see the `Sharing System Resources' chapter in
|||	    the 3DO Portfolio Programmer's Guide.
|||
|||	  Arguments
|||
|||	    name                        The name of the semaphore (see
|||	                                `Notes').
|||
|||	    pri                         The priority of the semaphore.  Use 0
|||	                                for now.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the semaphore or an error
|||	    code if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    semaphore.h                 ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    When you no longer need a semaphore, use DeleteSemaphore() to
|||	    delete it.
|||
|||	    You can use FindSemaphore() to find a semaphore by name. When
|||	    creating semaphores, you should assign unique names whenever
|||	    possible.
|||
|||	  See Also
|||
|||	    DeleteSemaphore(), LockSemaphore(), UnlockSemaphore()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/createuniquesemaphore
|||	CreateUniqueSemaphore - Create a semaphore with a unique name.
|||
|||	  Synopsis
|||
|||	    Item CreateUniqueSemaphore( const char *name, uint8 pri )
|||
|||	  Description
|||
|||	    This convenience procedure creates a semaphore item with the
|||	    specified name and priority.  You can use this procedure in place
|||	    of CreateItem() to create the semaphore.
|||
|||	    This function works much like CreateSemaphore(), except that it
|||	    guarantees that no other semaphore item of the same name
|||	    already exists. And once this semaphore created, no other
|||	    semaphore of the same name will be allowed to be created.
|||
|||	    For information about semaphores, which are used to control access
|||	    to shared resources, see the `Sharing System Resources' chapter in
|||	    the 3DO Portfolio Programmer's Guide.
|||
|||	  Arguments
|||
|||	    name                        The name of the semaphore (see
|||	                                `Notes').
|||
|||	    pri                         The priority of the semaphore.  Use 0
|||	                                for now.
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the semaphore or an error
|||	    code if an error occurs. If a semaphore of the same name already
|||	    existed when this call was made, the ER_Kr_UniqueItemExists error
|||	    will be returned.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V24.
|||
|||	  Associated Files
|||
|||	    semaphore.h                 ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    When you no longer need a semaphore, use DeleteSemaphore() to
|||	    delete it.
|||
|||	    You can use FindSemaphore() to find a semaphore by name.
|||
|||	  See Also
|||
|||	    DeleteSemaphore(), LockSemaphore(), UnlockSemaphore()
|||
**/

Item
internalCreateSemaphore(s, a)
Semaphore *s;
TagArg *a;
{
    int32 ret;

    DBUGCS(("CreateSemaphore\n"));

    ret = TagProcessor(s, a, NULL,0);

    if (ret >= 0)
    {
    	InitList(&s->sem_TaskWaitingList,"Semaphore WaitQ");
#ifdef MULTIT
    	s->sem_Owner = (Task *)-1;
#else
    	s->sem_Owner = -1;
#endif
    	TailInsertNode(KernelBase->kb_Semaphores,(Node *)s);
    	DBUGCS(("returning: %d\n",s->s.n_Item));
    	ret = s->s.n_Item;
    }

    return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/locksemaphore
|||	LockSemaphore - Lock a semaphore.
|||
|||	  Synopsis
|||
|||	    int32 LockSemaphore( Item s, uint32 flags )
|||
|||	  Description
|||
|||	    This procedure locks a semaphore.  For information about
|||	    semaphores, which are used to control access to shared resources,
|||	    see the `Sharing System Resources' chapter in the 3DO Portfolio
|||	    Programmer's Guide.
|||
|||	  Arguments
|||
|||	    s                           The number of the semaphore to be
|||	                                locked.
|||
|||	    flags                       Semaphore flags.
|||
|||	    Only one flag is currently defined:
|||
|||	    SEM_WAIT                    If the item to be locked is already
|||	                                locked, put the calling task into wait
|||	                                state until the item is available.
|||
|||	  Return Value
|||
|||	    The procedure returns 1 if the semaphore was successfully locked.
|||	    If the semaphore is already locked and the SEM_WAIT flag is not
|||	    set (which indicates that the calling task does not want to wait
|||	    until the semaphore is available), the procedure returns 0.  If an
|||	    error occurs, the procedure returns an error code.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	    This is the same SWI as LockItem().
|||
|||	  Associated Files
|||
|||	    semaphore.h                 ARM C "swi" declaration
|||
|||	  Caveats
|||
|||	    There are currently no asynchronous locks or shared read locks.
|||
|||	  See Also
|||
|||	    FindSemaphore(), UnlockSemaphore()
|||
**/

int32
internalLockSemaphore(s,wait)
Semaphore *s;
int32 wait;
{
	/* Try to Lock the semaphore */
	/* if the semaphore is successfully locked return TRUE */
	/* if unsuccessful and wait is TRUE then Wait for */
	/* the Semaphore to become free */
	/* if wait is false and the semaphore is busy then */
	/* return FALSE */
	DBUGLS(("LockSem: s=%lx t=%lx\n",(uint32)s,KernelBase->kb_CurrentTask));
#ifdef MULTIT
    {
	uint32 oldints;
	uint32 ctr;
	Task *ct = KernelBase->kb_CurrentTask;
	oldints = Disable();
	ctr = s->sem_bit;
	if (ctr == 0)
	{   /* unlocked, simple */
	    s->sem_bit = 1;
	    s->sem_Owner = ct;
	    Enable(oldints);
	    return 1;
	}
	/* already locked, is it us? */
	if (s->sem_Owner == ct)
	{
	    s->sem_NestCnt++;
	    Enable(oldints);
	    return 1;
	}
	/* locked by someone else, do we want to wait? */
	if (wait == 0)
	{
	    Enable(oldints);
	    return 0;
	}
	{
	    uint32 waitret;
	    /* must q our selves up */
	    /* and wait */
	    SemaphoreWaitNode p;
	    p.swn.n_Type = SEMA4WAIT;
	    p.swn.n_SubsysType = KERNELNODE;
	    p.swn_Task = KernelBase->kb_CurrentTask;
	    ClearSignals(CURRENTTASK,SIGF_ONESHOT);
	    ADDTAIL(&s->sem_TaskWaitingList,(Node *)&p);
	    waitret = internalWait(SIG_SEMAPHORE);
	    Enable(oldints);
	    if (waitret & SIGF_ABORT)
	    {
		if (ct->t_Flags & TASK_EXITING)
		    REMOVENODE((Node *)&p);
	        return BADITEM;
	    }
	    return 1;
	}
    }

#else
    {
	int32	was_owned;
	Task	*ct = KernelBase->kb_CurrentTask;
	uint32	waitret;
	SemaphoreWaitNode p;

	/* Is the semaphore currently locked ? */
	was_owned = armswap(&s->sem_bit,1);
	DBUGLS(("was_owned=%d\n",was_owned));

	if (!was_owned)
	{
		/* The semaphore is currently not locked by anyone */
		s->sem_NestCnt = 1;
		s->sem_Owner = ct->t.n_Item;
		return 1;
	}
	/* The semaphore is currently locked by some task */

	/* Are we the one? */
	if (ct->t.n_Item == s->sem_Owner)
	{
		/* Yup, just bump the nest count and return */
		s->sem_NestCnt++;
		return 1;
	}
	/* No, it is some other task */
	/* Do we wait for the semaphore to be unlocked ? */
	if (!wait) return 0;		/* No waiting */

	/* Yes, we queue our selves up  and wait */
	p.swn.n_Type = SEMA4WAIT;
	p.swn.n_SubsysType = KERNELNODE;
	p.swn_Task = KernelBase->kb_CurrentTask;
	ClearSignals(CURRENTTASK,SIGF_ONESHOT);
	ADDTAIL(&s->sem_TaskWaitingList,(Node *)&p);
	waitret = internalWait(SIGF_ONESHOT);
	if (waitret & SIGF_ABORT)
	{
	    if (ct->t_Flags & TASK_EXITING)
		REMOVENODE((Node *)&p);
	    return BADITEM;
	}
	/* sem_Owner and sem_NestCnt are already set for us */
	/* the signaling task */
	return 1;
    }
#endif
}

/**
|||	AUTODOC PUBLIC spg/kernel/lockitem
|||	LockItem - Lock an item.
|||
|||	  Synopsis
|||
|||	    int32 LockItem( Item s, uint32 flags )
|||
|||	  Description
|||
|||	    This procedure locks an item.
|||
|||	    Note: Currently, semaphores are the only items that can be locked.
|||	    You can lock semaphores by calling LockSemaphore().
|||
|||	  Arguments
|||
|||	    s                           The number of the item to be locked.
|||
|||	    flags                       Semaphore flags.
|||
|||	    Only one flag is currently defined:
|||
|||	    SEM_WAIT                    If the item to be locked is already
|||	                                locked, put the calling task into wait
|||	                                state until the item is available.
|||
|||	  Return Value
|||
|||	    The procedure returns 1 if the item was successfully locked.  If
|||	    the item is already locked and the SEM_WAIT flag is not set (which
|||	    indicates that the calling task does not want to wait until the
|||	    item is available), the procedure returns 0.  If an error occurs,
|||	    the procedure returns an error code.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	    This is the same SWI as LockSemaphore()
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  Caveats
|||
|||	    Other items should be lockable with some reasonable semantic
|||	    interpretation.
|||
|||	  See Also
|||
|||	    DeleteItem(), UnlockItem()
|||
**/

int32
externalLockSemaphore(is,wait)
Item is;
int32 wait;
{
	Semaphore *s = (Semaphore *)CheckItem(is,KERNELNODE,SEMA4NODE);
	if (!s)	return BADITEM;
	if (wait & 0xfffffffe) return MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadLockArg);
	return internalLockSemaphore(s,wait);
}

int32
internalUnlockSemaphore(s)
Semaphore *s;
{
	SemaphoreWaitNode *swn;
	DBUGUS(("UlockSem: s=%lx t=%lx\n",(uint32)s,KernelBase->kb_CurrentTask));
#ifdef MULTIT
    {
	uint32 oldints;
	uint32 ctr;

	oldints = Disable();
	ctr = s->sem_bit;
	ctr--;
	s->sem_bit = ctr;
	if (ctr == 0)
	{   /* No one else wants it, return quick */
	    s->sem_Owner = (Task *)-1;
	    Enable(oldints);
	    return 0;
	}
	ctr = s->sem_NestCnt;
	if (ctr)
	{   /* nested locks, unnest one and return */
	    ctr--;
	    s->sem_NestCnt = ctr;
	    Enable(oldints);
	    return 0;
	}
	swn = (SemaphoreWaitNode *)RemHead(&s->sem_TaskWaitingList);
	internalSignal(swn->swn_Task,SIGF_ONESHOT);
	s->sem_Owner = swn->swn_Task;
	return 0;
    }
#else
	s->sem_NestCnt--;
	if (s->sem_NestCnt)
	{
		/* don't unlock until complete unwound */
		return 0;
	}
	swn = (SemaphoreWaitNode *)RemHead(&s->sem_TaskWaitingList);
	if (swn)
	{
		DBUGUS(("Send sig:%lx to %d\n",swn->swn_sig,swn->swn_Task));
		internalSignal(swn->swn_Task, SIGF_ONESHOT);
		s->sem_Owner = swn->swn_Task->t.n_Item;
		s->sem_NestCnt = 1;
	}
	else
	{
		DBUGUS(("just clear sem_bit\n"));
		s->sem_bit = 0;
		s->sem_Owner = -1;
		/* s->sem_NestCnt = 0; */
	}
	return 0;
#endif
}

/**
|||	AUTODOC PUBLIC spg/kernel/unlockitem
|||	UnlockItem - Unlock a locked item.
|||
|||	  Synopsis
|||
|||	    Err UnlockItem( Item s )
|||
|||	  Description
|||
|||	    This procedure unlocks the specified item.
|||
|||	    Note:  Currently, semaphores are the only items that can be
|||	    locked.  You can also unlock semaphores by calling
|||	    UnlockSemaphore().
|||
|||	  Arguments
|||
|||	    s                           The item number of the item to be
|||	                                unlocked.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.  Possible error codes include:
|||
|||	    NOTOWNER                    The item specified by the s argument
|||	                                is not owned by the task.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	    This is the same SWI as UnlockSemaphore().
|||
|||	  Associated Files
|||
|||	    item.h                      ARM C "swi" declaration
|||
|||	  See Also
|||
|||	    DeleteItem(), LockItem(), UnlockSemaphore()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/unlocksemaphore
|||	UnlockSemaphore - Unlock a semaphore.
|||
|||	  Synopsis
|||
|||	    Err UnlockSemaphore( Item s )
|||
|||	  Description
|||
|||	    This is an alternate entry point for UnlockItem().  This procedure
|||	    unlocks the specified semaphore.  For information about
|||	    semaphores, which are used to control access to shared resources,
|||	    see the `Sharing System Resources' chapter in the 3DO Portfolio
|||	    Programmer's Guide.
|||
|||	  Arguments
|||
|||	    s                           The item number of the semaphore to
|||	                                unlock.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if successful or an error code (a negative
|||	    value) if an error occurs.  Possible error codes include:
|||
|||	    NOTOWNER                    The semaphore specified by the s
|||	                                argument is not owned by the task.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	    This is the same SWI as UnlockItem().
|||
|||	  Associated Files
|||
|||	    semaphore.h                 ARM C "swi" declaration
|||
|||	  See Also
|||
|||	    LockSemaphore(), LockItem(), UnlockItem()
|||
**/

int32
externalUnlockSemaphore(is)
Item is;
{
	Semaphore *s = (Semaphore *)CheckItem(is,KERNELNODE,SEMA4NODE);
	if (!s) return BADITEM;
	if (KernelBase->kb_CurrentTask->t.n_Item != s->sem_Owner)
	{
		DBUGUS(("Not Owner\n"));
		return NOTOWNER;
	}
	return internalUnlockSemaphore(s);
}
/**
|||	AUTODOC PUBLIC spg/kernel/deletesemaphore
|||	DeleteSemaphore - Delete a semaphore
|||
|||	  Synopsis
|||
|||	    Err DeleteSemaphore( Item s )
|||
|||	  Description
|||
|||	    This macro deletes a semaphore.
|||
|||	    For information about semaphores, which are used to control access
|||	    to shared resources, see the `Sharing System Resources' chapter in
|||	    the 3DO Portfolio Programmer's Guide.
|||
|||	  Arguments
|||
|||	    s                           The item number of the semaphore to be
|||	                                deleted.
|||
|||	  Return Value
|||
|||	    The macro returns 0 if successful or an error code if an error
|||	    occurred.
|||
|||	  Implementation
|||
|||	    Macro implemented in semaphore.h V20.
|||
|||	  Associated Files
|||
|||	    semaphore.h                 ANSI C Macro definition
|||
|||	  Notes
|||
|||	    As with all items, the item number of a deleted semaphore is not
|||	    reused.  If a task specifies the item number of a deleted
|||	    semaphore, the Kernel informs the task that the item no longer
|||	    exists.
|||
|||	  See Also
|||
|||	    CreateSemaphore()
|||
**/

int32
internalDeleteSemaphore(s,t)
Semaphore *s;
Task *t;
{
	SemaphoreWaitNode *n;
	DBUG(("DeleteSemaphore(%d) s=%lx\n",s->s.n_Item,(uint32)s));
	/* Only the creator or superuser can kill a semaphore */
	/* remove all tasks waiting for this semaphore */
	/* and send an error signal to them */
	while ((n = (SemaphoreWaitNode *)RemHead(&s->sem_TaskWaitingList)) !=
	       (SemaphoreWaitNode *)0) {
		internalSignal(n->swn_Task,SIGF_ABORT);
	}
	REMOVENODE((Node *)s);
	return 0;
}


Err
internalSetSemaphoreOwner(Semaphore *s, Item newOwner)
{
    if (s->sem_Owner == (Item)-1)
    {
        /* Only allow semaphores to be transfered ownership when they are
         * not locked by anyone.
         *
         * Note that we could also transfer the ownership if the semaphore
         * is locked by the current thread... Maybe later.
         */
        return 0;
    }

    return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);
}
