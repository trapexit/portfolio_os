/* $Id: task.c,v 1.166.1.4 1995/02/02 21:26:50 limes Exp $ */


#define DBUG(x)	 /*printf x*/
#define DBUGCT(x) /*if (KernelBase->kb_Flags & KB_TASK_DBG)*/  /*printf x*/
#define DBUGKILL(x)	/*{ printf x ; }*/
extern void Panic(int halt, char *);

extern int dram_size;
extern int kernel_reserve;

#include "types.h"

#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "listmacros.h"
#include "task.h"
#include "folio.h"
#include "kernel.h"
#include "mem.h"
#include "msgport.h"
#include "semaphore.h"
#include "io.h"
#include "interrupts.h"
#include "aif.h"
#include "rsa.h"
#include "strings.h"
#include "timer.h"
#include "operror.h"
#include "inthard.h"
#include "stdio.h"
#include "internalf.h"
#include "debug.h"

extern void Panic(int halt,char *);

Task *Land(Task *);
extern Timer	*quantaclock;
extern int32	SuperStackSize;	/*found in startup.s */
extern char	*SuperStack;

extern int32 AllocPages(MemHdr *,Task *,int32,int32);
extern void AddMem(MemList *,Node *,int32);

extern _3DOBinHeader	__my_3DOBinHeader;

Item
internalFindTask(name)
char *name;
{
	List	*l = KernelBase->kb_Tasks;
	Node	*n;
	Task	*t;
	Item	it;

	DBUG(("FindTask(%lx):%s\n",name,name));
	DBUG(("ct=%lx name=%s\n",CURRENTTASK,CURRENTTASK->t.n_Name));

	if (name == 0)	return CURRENTTASK->t.n_Item;

	for (n=FIRSTNODE(l); ISNODE(l,n); n = NEXTNODE(n)) {

		/* Get task pointer (t) from the tasks link pointer (n) */
		t = Task_Addr(n);

		/* Note: Both t->t.n_Name and name are non-NULL */
		if (strcasecmp(name, t->t.n_Name) == 0)
			return t->t.n_Item;
	}

	it = MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
	return it;
}


void
UnlockSemaphores(t)
Task *t;
{
	Node *n; Semaphore *s;
	DBUGKILL(("Unlocking semaphores for %lx\n",(uint32)t));
	for (n = FIRSTNODE(KernelBase->kb_Semaphores);
		ISNODE(KernelBase->kb_Semaphores,n);
		n = NEXTNODE(n) )
	{
		s = (Semaphore *)n;
#ifdef MULTIT
		while (s->sem_Owner == t)
#else
		while (s->sem_Owner == t->t.n_Item)
#endif
		{
			/* We will Unlock it now */
			internalUnlockSemaphore(s);
		}
	}
}

void FreeUserMem(Task *);

void
FreeTask(t)
Task *t;
{
	DBUGKILL(("FreeTask: enter thread=%lx\n",(uint32)t->t_ThreadTask));
	if (t->t_FolioData)
	{
	    int32 i;

	    for (i = 0; i < KernelBase->kb_FolioTaskDataCnt; i++)
	    {
		void *tfd = t->t_FolioData[i];
		if (tfd)
		{
		    struct Folio **DataFolios = KernelBase->kb_DataFolios;
		    void (*fd)() = DataFolios[i]->f_FolioDeleteTask;
		    if (fd)(*fd)(t);
		}
	    }
	    FREEMEM(t->t_FolioData,KernelBase->kb_FolioTaskDataSize);
	}
	if (t->t_ThreadTask == 0)
	{	/* only for main (nonthread) tasks */
		DBUGKILL(("non thread task\n"));
		FreeUserMem(t);
		/*DBUGKILL(("Free Mem Protect Bits\n"));*/
		/*FREEMEM(t->t_MemProtectBits,sizeof(pd_set));*/
#ifdef ARM600
		FreePageTable(t);
#endif
	}
	/* Don't free the stack we are presently using! */
	if ((t != CURRENTTASK) && CURRENTTASK)
	{
	    if(t->t_SuperStackBase) {	/* Only free stack if they currently have one */
		DBUGKILL(("Freeing super stack at %lx\n",(uint32)t->t_SuperStackBase));
		FREEMEM(t->t_SuperStackBase,t->t_SuperStackSize);
	        t->t_SuperStackBase = 0;
	    }
	}
	/* KillTask only happens from swi mode, the code that returns
	   from swi land checks for a CurrentTask==0. If it finds it
	   it deals with this left over super stack */
	DBUGKILL(("Freeing ResourceTable(%lx size=%d)\n",(uint32)t->t_ResourceTable,(int)(t->t_ResourceCnt * sizeof(Item))));
	FREEMEM(t->t_ResourceTable,t->t_ResourceCnt * sizeof(Item));

	DBUGKILL(("FreeString\n"));
	FreeString(t->t.n_Name);
	t->t.n_Name = NULL;	/* so we won't free it twice */

	/* clear the shared task bit */
	if (t->t_Flags & TASK_DATADISCOK)
	{
	    /* this really is wrong! */
	    /* clear the global bit */
	    /* this really needs to be a counter here */
	    __my_3DOBinHeader._3DO_Flags &= ~_3DO_DATADISCOK;
	}

#if 0
	{
	Item fi;
	/* Send the FileSystem Daemon the 'low memory' signal */
	fi=internalFindTask("Aahz");
	internalSignal( (Task *)LookupItem(fi),SIGF_MEMLOW);
	}
#endif

	DBUGKILL(("Call ScavengeMem\n"));
	ScavengeMem();	/* force page returns in system space */

	DBUGKILL(("Return from FreeTask\n"));
}

void
FreeResources(t)
Task *t;
{
	Item *ip;
	int32 i;
	ip = t->t_ResourceTable;
	DBUGKILL(("FreeResources(%lx)\n",(uint32)t));
	ip += t->t_ResourceCnt;	/* go to last entry */
	for (i = 0; i < t->t_ResourceCnt; i++)
	{
	    Item it;
	    it = *--ip;
	    if (it >= 0)
	    {
		if (it & ITEM_WAS_OPENED)
		{
		    it &= ~ITEM_WAS_OPENED;
		    DBUGKILL(("closing item %lx\n",it));
		    internalCloseItem(it,t);
		}
		else
		    internalDeleteItem(it,t);
	    }
	}
	DBUGKILL(("Exiting FreeResources\n"));
}

int32
internalKill(t,ct)
Task *t;
Task *ct;
{
	uint32 oldints;

	DBUGKILL(("Kill: t=%lx(%s), ct=%lx(%s)\n", (uint32)t, t->t.n_Name,
						   (uint32)ct, ct->t.n_Name));

	t->t_Flags |= TASK_EXITING;

        if (t != ct)
        {
	    t->t_ExitStatus = KILLED;
	    if (t->t_psr & 0xf)
	    {
		/* User-pc of the task can be found at the bottom of */
		/* its supervisor stack after it last did a SWI call */

		if (t->t_SuperStackBase)
		{
		    uint32 *upc_addr;
		    upc_addr = (uint32 *)((char *)t->t_SuperStackBase +
				          t->t_SuperStackSize - 8);
		    *upc_addr = (uint32)ForceKill;
		}
		else
		{
		    t->t_sp = t->t_Usersp;
		    t->t_regs[10] = (uint32)t->t_StackBase+128;
		    t->t_psr= 0x10;
		    t->t_pc = (uint32)ForceKill;
		}
	    }
	    else
		t->t_pc = (uint32)ForceKill;

	    t->t_Killer = ct;

	    /*
	     * INLINE AND COMBINE, all in one critical section:
	     *	ClearSignals(ct, SIGF_ONESHOT);
	     *	internalSignal(t, SIGF_ABORT);
	     *	internalChangeTaskPri(t, 200);
	     */
	    oldints = Disable();
	    ct->t_SigBits &= ~SIGF_ONESHOT;
	    REMOVENODE((Node *)t);
	    t->t_SigBits |= SIGF_ABORT;
	    t->t.n_Flags |= TASK_READY;
	    t->t.n_Flags &= ~TASK_WAITING;
	    t->t.n_Priority = 200;
	    TailInsertNode(KernelBase->kb_TaskReadyQ,(Node *)t);
	    Enable(oldints);

	    /*
	     * Wait here until victim falls over.
	     * XXX- what do we want to do if we get a SIGF_ABORT?
	     */
	    while (!(internalWait(SIGF_ONESHOT) & SIGF_ONESHOT));
	    return 1;
        }

	FreeResources(t);
	DBUGKILL(("Now Unlock Semaphores\n"));
	/* Semaphores are done separately since we do not */
	/* dymanically track them in the tasks ResourceTable */
	/*Pause();*/
	/* Actually, since the process should have opened the */
	/* KernelFolio before accessing any Semaphores, the process */
	/* of closing the FolioLibrary should take care of thise */
	/* so we should move this to CloseKernelFolio */
	UnlockSemaphores(t);
	DBUGKILL(("After Unlock Semaphores\n"));
	DBUGKILL(("CurrentTask=%lx\n",(uint32)CURRENTTASK));
	/*Pause();*/

	if (t->t_Killer)
	    internalSignal(t->t_Killer, SIGF_ONESHOT);
	else
	    t->t_Killer = ct;

	if (t->t_ExitMessage)
	{
	    /* Send a message to the owner task about the child dying */
	    Msg *msg = (Msg *)CheckItem(t->t_ExitMessage,KERNELNODE,MESSAGENODE);
	    if (msg)
		internalReplyMsg(msg, t->t_ExitStatus, (void *)t->t.n_Item,
							t->t_Killer->t.n_Item);
	}
	else
	{
	    /* send a signal to the creator that his child is now deceased */
	    internalSignal((Task *)LookupItem(t->t.n_Owner),SIGF_DEADTASK);
	}

	/* Remove the task from the list of tasks in the system */
	oldints = Disable();
	REMOVENODE((Node *)&(t->t_TasksLinkNode));
	Enable(oldints);

	/* Save context and tidy up */
	DBUGKILL(("Kill: Suicide t=%lx\n",(uint32)t));
	FreeTask(t);
#ifdef FORBID
	KernelBase->kb_Forbid = 0;
#endif
	KernelBase->kb_PleaseReschedule = 1;
	CURRENTTASK = 0;
	/* The return value is never passed to the */
	/* caller since the caller has been eliminated */
	/* However the return value is used by the kernel */
	/* to tell him to remove this resource from the */
	/* the tasks resource table */
	/* In this case that resource table no int32er exists */
	return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/yield
|||	Yield - Give up the CPU to a task of equal priority.
|||
|||	  Synopsis
|||
|||	    void Yield( void )
|||
|||	  Description
|||
|||	    In Portfolio, high-priority tasks always have precedence over
|||	    lower priority tasks. Whenever a high priority task becomes
|||	    ready to execute, it will instantly interrupt lower priority
|||	    tasks and start running. The lower priority tasks do not get
|||	    to finish their time quantum, and just get put into the system's
|||	    ready queue for future scheduling.
|||
|||	    If there are a number of tasks of equal priority which are all
|||	    ready to run, the kernel does round-robin scheduling of these
|||	    tasks. This is a process by which each task is allowed to run
|||	    for a fixed amount of time before the CPU is taken away from it,
|||	    and given to another task of the same priority. The amount of time
|||	    a task is allowed to run before being preempted is called the
|||	    task's "quantum".
|||
|||	    The purpose of the Yield() function is to let a task voluntarily
|||	    give up the remaining time of its quantum. Since the time quantum
|||	    is only an issue when the kernel does round-robin scheduling, it
|||	    means that Yield() actually only does something when there are
|||	    multiple ready tasks at the same priority. However, since the
|||	    yielding task does not know exactly which task, if any, is going
|||	    to run next, Yeild() should not be used for implicit communication
|||	    amongst tasks. The way to cooperate amongst tasks is using signals,
|||	    messages, and semaphores.
|||
|||	    In short, if there are higher-priority tasks in the system, the
|||	    current task will only run if the higher-priority tasks are all in
|||	    the wait queue. If there are lower-priority tasks, these will
|||	    only run if the current task is in the wait queue. And if there
|||	    are other tasks of the same priority, the kernel automatically
|||	    cycles through all the tasks, spending a quantum of time on each
|||	    task, unless a task calls Yield(), which will cut short its
|||	    quantum.
|||
|||	    If there are no other ready tasks of the same priority as the
|||	    task that calls Yield(), then that task will keep running as
|||	    if nothing happened.
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
|||	    WaitSignal()
**/

void
internalYield()
{
	/* Give up CPU if another task of equal priority is waiting */
	/* Note, in theory there cannot be another task of higher */
	/* priority waiting */
	/* This is simple now */
	KernelBase->kb_PleaseReschedule = 1;
}

int32
internalChangeTaskPri(Task *t,uint8 newpri)
{
	Task *me = CURRENTTASK;
	uint8 oldpri;

	if ((t->t.n_Flags & TASK_SUPER) == 0) /* no checks on super tasks */
	{
	    if ( (newpri > 199) || (newpri < 10) )
		 return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadPriority);
        }

	/* CurrentTask cannot change from an interrupt! */
	/* But only if single threaded kernel! */
	oldpri = me->t.n_Priority;

	if (me == t)
	{
	    /* Changing priority of current running task, simple! */
	    me->t.n_Priority = newpri;
	    /* a lower newpri may preempt us */
	    if (newpri < oldpri) KernelBase->kb_PleaseReschedule = 1;
	}
	else
	{
	    /* Now must disable */
	    uint32 oldints;
	    oldints = Disable();
	    if (t->t.n_Flags & TASK_WAITING)
	    {
		/* on the unsorted WaitQ, just update priority */
		t->t.n_Priority = newpri;
	    }
	    else	/* On the ReadyQ */
	    {
	    	REMOVENODE((Node *)t);
	    	t->t.n_Priority = newpri;
	    	TailInsertNode(KernelBase->kb_TaskReadyQ,(Node *)t);
		/* A higher newpri may preempt us */
	        if (newpri > oldpri)
		    KernelBase->kb_PleaseReschedule = 1;
	    }
	    Enable(oldints);
	}
	return oldpri;
}

void
internalSetExitStatus(int32 status)
{
	CURRENTTASK->t_ExitStatus = status;
}

/**
|||	AUTODOC PRIVATE spg/kernel/SetExitStatus
|||	SetExitStatus - Set exit status for a task or thread.
|||
|||	  Synopsis
|||
|||	    Err SetExitStatus( int32 status )
|||
|||	  Description
|||
|||	    This procedure sets the exit status of the calling task or thread
|||	    process.
|||	    This status is returned to the parent process, if the calling
|||	    process was created with CREATETASK_TAG_MSGFROMCHILD tag.
|||
|||	  Arguments
|||
|||	    status                      The status to be returned to the
|||	                                parent of the calling task or thread.
|||	                                Negative status is reserved for
|||	                                system use.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the status was valid or an error code
|||	    if error occurs.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    task.h                      ANSI C Prototype
|||
|||	  Notes
|||
|||	    When a task or thread returns or calls exit(), the status
|||	    parameter is used by the system code to set the exit status
|||	    using SetExitStatus(). A task need not call SetExitStatus().
|||	    A system error-code will be set by the system in case
|||	    of fatal errors.
|||
|||	  See Also
|||
|||	    exit()
|||
**/

Err
externalSetExitStatus(int32 status)
{
	Err ret = 0;
	if (status < 0)
	{
	    status =  MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadExitStatus);
	    ret = status;
	}
	internalSetExitStatus(status);
	return ret;
}

void
KillSelf(void)
{
	DeleteItem(CURRENTTASK->t.n_Item);
}

/**
|||	AUTODOC PUBLIC spg/kernel/exit
|||	exit - Exit from a task or thread.
|||
|||	  Synopsis
|||
|||	    void exit( int status )
|||
|||	  Description
|||
|||	    This procedure deletes the calling task or thread process.
|||	    If the CREATETASK_TAG_MSGFROMCHILD tag was set when creating the
|||	    calling task, then the status is sent to the parent process through
|||	    a message.
|||
|||	  Arguments
|||
|||	    status                      The status to be returned to the
|||	                                parent of the calling task or thread.
|||	                                Negative status is reserved for
|||	                                system use.
|||
|||	  Return Value
|||
|||	    This procedure never returns.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    stdlib.h                    ANSI C Prototype
|||
|||	  Notes
|||
|||	    When tasks (including threads) have finished their work, they
|||	    should call exit() to die gracefully.  The system will call exit()
|||	    on behalf of the task that returns from the top-level routine.
|||	    exit() does necessary clean-up work when a task is finished,
|||	    including the cleanup required for a thread created by the
|||	    CreateThread() library routine.
|||
|||	  See Also
|||
|||	    CreateThread(), DeleteItem(), DeleteThread()
|||
**/

void
ExitTask(int32 status)
{
	/* Set exit status and commit suicide */
	/* We are in user-mode now */
	/* Commit suicide - we are in user-mode now */
	Task	*ct = CURRENTTASK;

	SetExitStatus(status);
	if (ct->t_ThreadTask && (ct->t_Flags & TASK_ALLOCATED_SP)) {
		List	*l = ct->t_FreeMemoryLists;
		Node	*n;
		MemList	*ml;

		/* Prevent others from using the stack memory being */
		/* freed below, before this thread item gets deleted */
		for (n = FIRSTNODE(l); ISNODE(l,n); n = NEXTNODE(n))
		{
			ml = (MemList *)n;
			if (ml->meml_Sema4) LockItem(ml->meml_Sema4,TRUE);
		}
		FreeMemToMemLists(ct->t_FreeMemoryLists, ct->t_StackBase, ct->t_StackSize);
	}

	DeleteItem(ct->t.n_Item);
}

int32
AbortCurrentTask(uint32 *sp)
{
	/* Abort the current task; runs only in abort mode */
	/* sp points the abort stack frame [r0-r12,r15] */
	Task	*ct = CURRENTTASK;

	if ((int32)ct->t_ExitStatus >= 0)
	{
	    internalSetExitStatus(ABORTED);
	    if (ct->t_ThreadTask && (ct->t_Flags & TASK_ALLOCATED_SP))
	    {
		/* it is a thread that needs to clean up its stack; so, change */
		/* his pc on the abort-stack to resume at ForceKill */
		sp[13] = (uint32)ForceKill;
		return 1;
	    }
	}
	internalKill(ct, ct);
	return 0;
}

struct TaskInfo
{
	AIFHeader *aifHdr;
	int32	filesize;
	char	*cmdstr;
	int32	cmdstrsize;
	uint32	exitmsgport;
	uint8	super;
	uint8	useronly;
};

static int32
ict_c(t, p, tag, arg)
Task *t;
struct TaskInfo *p;
uint32 tag;
uint32 arg;
{
    DBUGCT(("ict_c task=%lx tag=%lx arg=%lx\n",t,tag,arg));
    switch (tag)
    {
	case CREATETASK_TAG_MAXQ: t->t_MaxUSecs = arg;
				break;
	case CREATETASK_TAG_PC: t->t_pc = arg & 0x03FFFFFC;
				break;
	case CREATETASK_TAG_STACKSIZE: t->t_StackSize = (int32)arg;
				break;
	case CREATETASK_TAG_ARGC: t->t_regs[0] = arg;
				break;
	case CREATETASK_TAG_ARGP: t->t_regs[1] = arg;
				break;
	case CREATETASK_TAG_BASE: t->t_regs[9] = arg;
				break;
	case CREATETASK_TAG_SP: t->t_sp = (uint32 *) arg;
				break;
	case CREATETASK_TAG_ALLOCDTHREADSP:
				t->t_Flags |= TASK_ALLOCATED_SP;
				break;
	case CREATETASK_TAG_MSGFROMCHILD:
				p->exitmsgport = arg;
				break;
	case CREATETASK_TAG_AIF: p->aifHdr = (AIFHeader *)arg;
				if (arg & 3) return BADPTR;
				if (IsRamAddr((void *)arg,512) == 0) return BADPTR;
				break;
	case CREATETASK_TAG_IMAGESZ: p->filesize = (int32)arg;
				break;
	case CREATETASK_TAG_CMDSTR:
	{
				int32 cmdstrsize;
				p->cmdstr = (char *)arg;
				if (IsRamAddr((void *)arg,1024) == 0) return BADPTR;
				cmdstrsize = strlen((char *)arg)+1;
				/* make int32 words */
				cmdstrsize += 3;
				cmdstrsize &= ~3;
				p->cmdstrsize = cmdstrsize;
	}
				break;
	case CREATETASK_TAG_SUPER:
				p->super = TRUE;
				break;

	case CREATETASK_TAG_USERONLY:
				p->useronly = TRUE;
				break;

	case CREATETASK_TAG_RSA:	/* treated as a NOP */
	default:
		DBUGCT(("parsing tagargs for ct: tag=%lx\n",tag));
		if (tag < 0x10000)
		{
		    DBUGCT(("bad tag arg:%d\n",(int32)tag));
		    return BADTAG;
		}
		/* ignore the tag */
		break;
    }
    return 0;
}

Item
internalCreateTask(t,tagpt)
Task *t;
TagArg *tagpt;
{
	/* Normally called via swi */
	uint32 *mpb;	/* Memory Bits */
	MemHdr *dmh = (MemHdr *)FIRSTNODE(KernelBase->kb_MemHdrList);
	MemList *ml = 0, *dml = 0, *vml2 = 0;
	Task *ct;
	uchar *ustack=0;
	uint32 oldints;
	int32 usermem_needed;
	int32 size;
	uint8 *top=0;
	int32 imagesize = 0;
	int32 leftover = 0;
	int32 gavemem = 0;
	char isthread = 0;
	char RSAok = 0;
	AIFHeader *aifHdr;
	Item ret;
	MsgPort *mp = NULL;

	struct TaskInfo tinfo;
	/* Allocate a Task Control Block */

	MemHdr *vmh1, *vmh2;	/* VRAM MemHdr */
	MemHdr *defaultmh;

	vmh1 = (MemHdr *)FindML(KernelBase->kb_MemHdrList,MEMTYPE_VRAM_BANK1);
	if (!vmh1)
	{
#ifdef DEVELOPMENT
	    printf("Could not find VRAM bank 1 list in CreateTask\n");
	    while (1);
#else
	    Panic(1,"No VRAM list in CT\n");
#endif

	}
	if (vmh1 == dmh) dmh = 0;

	vmh2 = (MemHdr *)FindML(KernelBase->kb_MemHdrList,MEMTYPE_VRAM_BANK2);

	DBUGCT(("internalCreateTask: Disabled=%lx\n",Disabled()));
	/* set up defaults */
	t->t_StackSize = 256;
	ct = CURRENTTASK;
	if (ct == 0)
		DBUGCT(("Parent task is NULL - Creating Operator Task"));

	if (ct)
	{
		/* Inherit some things from creator */
		t->t.n_Priority = ct->t.n_Priority;
	}
	t->t_MaxUSecs = 15000;	/* 15 msecs default */
	/* regs0 and regs1 are a dont care */
	t->t_regs[9] = (int32)KernelBase;	/* default initial Base ptr */

	DBUGCT(("Process tag args:%lx\n",(uint32)tagpt));

	memset(&tinfo,0,sizeof(tinfo));

	ret = TagProcessor(t, tagpt, ict_c, &tinfo);
	if (ret < 0)	return ret;

	if (ct)
	{
	    Task *tt = ct->t_ThreadTask;

	    if (t->t_sp)
	    {
		isthread = 1;
		/* if the current task is a thread, then make the current */
		/* thread's owner-task the owner-task for the child thread */
		t->t_ThreadTask = (tt) ? tt : ct;
	    }
	    else
	    {
		/* We are trying to create a new task */
		/* Threads cannot create tasks */
		if (tt)
		{
		    ret = MAKEKERR(ER_SEVER,ER_C_NSTND, ER_Kr_ThreadError);
		    goto err;
		}
	    }
	}

	aifHdr = tinfo.aifHdr;

	/*
	   A: if ct is SUPER and TAG_SUPER is sent the nt is SUPER
	   B: if ct is SUPER and TAG_SUPER is not  then nt is NOT SUPER
	*/
	if (ct == 0) 	t->t.n_Flags |= TASK_SUPER;	/* operator starting */
	else
	{
	    if (tinfo.super)	/* requesting SUPER? */
	    {
		DBUGCT(("requesting SUPER ct->flags=%lx\n",ct->t.n_Flags));
	    	if (ct->t.n_Flags & TASK_SUPER)
			t->t.n_Flags |= TASK_SUPER;
		else
		{
		    ret = MAKEKERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged);
		    goto err;
		}
	    }
	}

	/* requesting user only ?  (no super stack) */
	if (tinfo.useronly) {
		/*
		 * if its not the operator and not a super task,
		 * don't let them.
		 */
		if(ct && ((ct->t.n_Flags & TASK_SUPER) == 0)) {
		    ret = MAKEKERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged);
		    goto err;
		}
	}
	if (t->t_StackSize < tinfo.cmdstrsize) t->t_StackSize = tinfo.cmdstrsize + 256;
	/* validation */
	DBUGCT(("Now validate\n"));
	DBUGCT(("TaskNode at %lx item=%lx\n",(uint32)t,t->t.n_Item));
	DBUGCT(("name=%s sp=%lx ssize=%d\n",t->t.n_Name,(uint32)t->t_sp,t->t_StackSize));
	DBUGCT(("pc=%lx\n",(uint32)t->t_pc));

	if (tinfo.cmdstr)	DBUGCT(("cmdstr=%s\n",tinfo.cmdstr));

	if (!t->t.n_Name) return BADTAGVAL;

	if ( ct && ((ct->t.n_Flags & TASK_SUPER) == 0))
	{
		if ( (t->t.n_Priority < 10) || (t->t.n_Priority > 199) )
		{
		    DBUGCT(("BadPrioritytag=%ld\n",t->t.n_Priority));
		    ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadPriority);
		    goto err;
		}
	}
	if ( ( t->t_MaxUSecs < 5000)
	    || (t->t_MaxUSecs > SimpleTicksToUsecs(62500)) )
	{
#ifdef undef
		/* use default for now */
		t->t_MaxUSecs = 15000;
#endif
		/* Nope, error them out now */
		ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadQuanta);
		goto err;
	}
	/*
	 * allow threads that do nothing through with 128 bytes
	 * if they do anything, we'll get them via stack limit check
	 */
	if ( t->t_StackSize < ((isthread) ? 128 : 256) )	{
	    ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadStackSize);
	    goto err;
	}

	if (aifHdr)
	{
	    if (isthread)
	    {
		ret = NOSUPPORT;
		goto err;
	    }
	    /* override any set pc */
	    DBUGCT(("Is AifTask, aifHdr = %lx\n",(uint32)aifHdr));
	    t->t_pc = (int)aifHdr;
	}

	if (IsRamAddr((void *)t->t_pc,16) == 0)
	{
	    ret = BADPTR;
	    goto err;
	}

	t->t_AllocatedSigs = 0x000000FF;	/* preallocate 8 signals */
	t->t_FreeResourceTableSlot = -1;
	/* Allocate Supervisor stack */
	ret = NOMEM;	/* assume out of memory if we goto abort */
	if(tinfo.useronly == 0) {
	    t->t_SuperStackSize = SuperStackSize;
	    t->t_SuperStackBase = (uint32 *)ALLOCMEM(t->t_SuperStackSize,MEMTYPE_ANY);
	    DBUGCT(("sstackbase=%lx\n",(uint32)t->t_SuperStackBase));
	    if (!t->t_SuperStackBase) goto abort;
	    t->t_ssp = t->t_SuperStackBase+(t->t_SuperStackSize/4) - 1;
	    DBUGCT(("ssp=%lx\n",(uint32)t->t_ssp));
	    t->t_ssl = t->t_SuperStackBase+128;
	}

	/* Allocate per task data for other folios */
	DBUGCT(("Allocate per task data for other folios ds=%d\n", \
			KernelBase->kb_FolioTaskDataSize));
	if (KernelBase->kb_FolioTaskDataSize)
	{
	    Folio *f;
	    t->t_FolioData = (void **)ALLOCMEM(KernelBase->kb_FolioTaskDataSize, \
				MEMTYPE_FILL);
	    DBUGCT(("pertaskdata=%lx\n",(ulong)t->t_FolioData));
	    if (!t->t_FolioData) goto abort;
	    /* paw through the list of Folios */
	    for (f = (Folio *)FIRSTNODE(KernelBase->kb_FolioList);
			ISNODE(KernelBase->kb_FolioList,f);
			    f = (Folio *)NEXTNODE(f) )
	    {
		DBUGCT(("Folio=%lx\n",(ulong)f));
		if (f == 0)
		{
#ifdef DEVELOPMENT
			printf("CreateTask panic:f=0 KB=%lx\n",(long)KernelBase);
			while (1);
#else
		Panic(1,"CreateTask error\n");
#endif

		}
		if (f->f_FolioCreateTask)
		{
		    DBUGCT(("Calling FolioCreateTask folio=%s at:%lx\n", \
				f->fn.n_Name,f->f_FolioCreateTask));
		    if ( (*f->f_FolioCreateTask)((struct Task *)t,tagpt) < 0)
									goto abort;
		}
	    }
	}

	/* Allocate Resource Table */

	DBUGCT(("Allocate resource table\n"));
	if (IncreaseResourceTable(t,6) < 0) goto abort;

	VERIFYMEM("CreateTask:Before check for isthread\n");
	if (isthread) {
	    MemList *n;
		if (ct == 0)
			DBUGCT(("Using NULL task pointer to set bits!"));
		DBUGCT(("Is a thread!:%s\n",t->t.n_Name));
		/*t->t_MemProtectBits = ct->t_MemProtectBits;*/
		t->t_FreeMemoryLists = ct->t_FreeMemoryLists;
		t->t_StackBase = t->t_sp - (t->t_StackSize/sizeof(int32));
		t->t_regs[10] = (uint32)t->t_StackBase+128;	/* set stack limit register */
		t->t_PageTable = ct->t_PageTable;
		/* Create Semaphores to control access to the MemLists */
	    for (n = (MemList *)FIRSTNODE(t->t_FreeMemoryLists);
			ISNODE(t->t_FreeMemoryLists,n);
				n = (MemList *)NEXTNODE(n) )
	    {
		if (n->meml_Sema4 == 0)
		{
		    TagArg st[2];
		    st[0].ta_Tag = TAG_ITEM_NAME;
		    st[0].ta_Arg = n->meml_n.n_Name;
		    st[1].ta_Tag = 0;

		    n->meml_Sema4 =
			internalCreateSizedItem(MKNODEID(KERNELNODE,SEMA4NODE),st,0);
		    if (n->meml_Sema4 < 0) goto abort;
		}
	    }
	}
	else {	/* not a thread, a full process */
	/* Allocate a List for the Memory Free Lists */
		t->t_FreeMemoryLists = (List *)AllocateNode((Folio *)KernelBase,LISTNODE);
		if (!t->t_FreeMemoryLists)	goto abort;

		InitList(t->t_FreeMemoryLists,"List of Free Mem Lists");
    /* Need to allocate a MemListNode for each available memtype */
	/* Allocate VRAM MemList */
		ml = (MemList *)AllocateNode((Folio *)KernelBase,MEMLISTNODE);
		if (!ml)	goto abort;

		internalInitMemList(ml,vmh1,"task vram bank 1 ml");
		ADDTAIL(t->t_FreeMemoryLists,(Node *)ml);

	/* Allocate Memory Own Bits */
		ml->meml_OwnBitsSize = vmh1->memh_FreePageBitsSize;
		mpb = (uint32 *)ALLOCMEM( \
		    ml->meml_OwnBitsSize*(int32)sizeof(pd_mask), \
					MEMTYPE_ANY|MEMTYPE_FILL);
		if (!mpb)	goto abort;
		ml->meml_OwnBits = mpb;

	/* Allocate Memory Protect Bits */
		mpb = (uint32 *)ALLOCMEM( \
		    ml->meml_OwnBitsSize*(int32)sizeof(pd_mask), \
			MEMTYPE_ANY|MEMTYPE_FILL);
		if (!mpb)	goto abort;
		ml->meml_WriteBits = mpb;

                if (vmh2)
                {
                    /* Allocate VRAM bank 2 MemList */
                    vml2 = (MemList *)AllocateNode((Folio *)KernelBase,MEMLISTNODE);
                    if (!vml2)        goto abort;

                    internalInitMemList(vml2,vmh2,"task vram bank 2 ml");
                    ADDTAIL(t->t_FreeMemoryLists,(Node *)vml2);

                    /* Allocate Memory Own Bits */
                    vml2->meml_OwnBitsSize = vmh2->memh_FreePageBitsSize;
                    mpb = (uint32 *)ALLOCMEM( \
                        vml2->meml_OwnBitsSize*(int32)sizeof(pd_mask), \
                                            MEMTYPE_ANY|MEMTYPE_FILL);
                    if (!mpb)       goto abort;
                    vml2->meml_OwnBits = mpb;

                    /* Allocate Memory Protect Bits */
                    mpb = (uint32 *)ALLOCMEM( \
                        vml2->meml_OwnBitsSize*(int32)sizeof(pd_mask), \
                            MEMTYPE_ANY|MEMTYPE_FILL);
                    if (!mpb)       goto abort;
                    vml2->meml_WriteBits = mpb;
                }

		/* add DRAM memlist if dram memhdr found */
		if (dmh)
		{
		    uint32 *mpb;
		    dml = (MemList *)AllocateNode((Folio *)KernelBase,MEMLISTNODE);
		    if (!dml)	goto abort;
		    internalInitMemList(dml,dmh,"task dram ml");
		    ADDHEAD(t->t_FreeMemoryLists,(Node *)dml);
	/* Allocate Memory Own Bits */
		    dml->meml_OwnBitsSize = dmh->memh_FreePageBitsSize;
		    mpb = (uint32 *)ALLOCMEM( \
			  dml->meml_OwnBitsSize*(int32)sizeof(pd_mask), \
				MEMTYPE_ANY|MEMTYPE_FILL);
		    if (!mpb)	goto abort;
		    dml->meml_OwnBits = mpb;

	/* Allocate Memory Protect Bits */
		    mpb = (uint32 *)ALLOCMEM( \
			  dml->meml_OwnBitsSize*(int32)sizeof(pd_mask), \
				MEMTYPE_ANY|MEMTYPE_FILL);
		    if (!mpb)	goto abort;
		    dml->meml_WriteBits = mpb;
#ifdef undef
		    if (ct == 0)
		    {	/* operator task only */
			memset(dml->meml_WriteBits,-1,
				  dml->meml_OwnBitsSize*(int32)sizeof(pd_mask));
		    }
#endif

		}

#ifdef ARM600
		if (KernelBase->kb_CPUFlags & KB_ARM600)
		{
			mpb = AllocatePageTable();
			DBUGCT(("PageTable at : %lx\n",(uint32)mpb));
			if (!mpb)	goto abort;
			t->t_PageTable = mpb;
			ComputePageTable(t);
		}
#endif
		    /* set default MemHdr now */
		defaultmh = internalFindMH((char *)t->t_pc);
		if (aifHdr)
		{
		    /* Make sure we can read the aifheader */
		    if (IsRamAddr(aifHdr,sizeof(AIFHeader)) == 0)
		    {
		        ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_BadAIFHeader);
			goto abort;
		    }
		    DBUGCT(("RO=%d RW=%d ZI=%d\n",aifHdr->aif_ImageROsize,\
					aifHdr->aif_ImageRWsize,\
					(int)aifHdr->aif_ZeroInitSize));
		    imagesize = aifHdr->aif_ImageROsize +
					aifHdr->aif_ImageRWsize +
					(~15 &(aifHdr->aif_ZeroInitSize+15));
			/* round up to 16, piece of shit arm sys spec */
		    if ((aifHdr->aif_ImageROsize < 128) ||
			(aifHdr->aif_ImageRWsize < 0) ||
			(aifHdr->aif_ZeroInitSize < 0) ||
			(imagesize == 0))
		    {
		        ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_BadAIFHeader);
			goto abort;
		    }
		    DBUGCT(("Give the startup image to the new task\n"));
		    DBUGCT(("ct=%lx mh=%lx\n",(long)ct,(long)defaultmh));
		    DBUGCT(("dml=%lx\n",dml));
		    if (imagesize < tinfo.filesize) imagesize = tinfo.filesize;
		    if (ct)
		    {
			/* Must do this in two passes now */
			/* write protect this piece of memory for all tasks */
		    	ret = internalControlMem(ct,(char *)aifHdr,
				imagesize,MEMC_NOWRITE,0);
			if (ret == 0)
		    	    ret = internalControlMem(ct,(char *)aifHdr,
				imagesize,MEMC_GIVE,t->t.n_Item);
		    }
		    else /* operator special */
		    {
		    	ret = internalControlMem(ct,(char *)aifHdr,
				 (int32)kernel_reserve - (int32)aifHdr,
						 MEMC_GIVE, t->t.n_Item);
		    }
		    if (ret < 0) goto abort;
		    ret = NOMEM; /* assume out of memory if we goto abort */
		    gavemem = imagesize;

		    DBUGCT(("Process expanded 3do header\n"));
		    /* process expanded 3do aif header (_3DOBinHeader) */
		    if (aifHdr->aif_WorkSpace & AIF_3DOHEADER)
		    {
			/* override all passed in information */
			_3DOBinHeader *_3do = (_3DOBinHeader *)(aifHdr + 1);
		    	if (IsRamAddr(_3do,sizeof(_3DOBinHeader)) == 0)
			{
			    ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_BadAIFHeader);
			    goto abort;
			}
			if (_3do->_3DO_Stack)
			{
			    t->t_StackSize = _3do->_3DO_Stack;
			    /* Revalidate stack setting */
			    if (t->t_StackSize < tinfo.cmdstrsize)
				t->t_StackSize = tinfo.cmdstrsize + 256;
	    		    if ( t->t_StackSize < ((isthread) ? 128 : 256) )
			    {
				ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadStackSize);
			    	goto abort;
			    }
			}
			if (_3do->_3DO_MaxUSecs) t->t_MaxUSecs = _3do->_3DO_MaxUSecs;
			if ( ( t->t_MaxUSecs < 5000)
	    		    || (t->t_MaxUSecs > SimpleTicksToUsecs(62500)) )
			{
			    ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadQuanta);
			    goto abort;
			}

			t->t.n_Version = _3do->_3DO_Item.n_Version;
			t->t.n_Revision = _3do->_3DO_Item.n_Revision;
			if (_3do->_3DO_Flags & _3DO_DATADISCOK)
			{
			    __my_3DOBinHeader._3DO_Flags |= _3DO_DATADISCOK;
			    t->t_Flags |= TASK_DATADISCOK;
			}

			RSAok = 0;
			if (_3do->_3DO_SignatureLen)
			{
			    if (IsRamAddr(aifHdr,tinfo.filesize) == 0)
			    {
				ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_BadAIFHeader);
				goto abort;
			    }
			    /* Reality check the tinfo.filesize */
			    if (tinfo.filesize != _3do->_3DO_SignatureLen + _3do->_3DO_Signature)
			    {
				ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_RSAFail);
                                goto abort;
			    }
			    RSAok = (char)RSACheck((uchar *)aifHdr,tinfo.filesize);
			    if (RSAok == FALSE)
			    {
				DBUGCT(("RSACheck failed\n"));
				ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_RSAFail);
				goto abort;
			    }
			    if (_3do->_3DO_Flags & _3DO_PRIVILEGE) {
				if(RSAok == 1) {
				    t->t.n_Flags |= TASK_SUPER;
				}
				else {
				    ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_RSAFail);
				    goto abort;
				}
			    }
			}
			if (_3do->_3DO_Item.n_Priority)
			{
			    t->t.n_Priority =
				    _3do->_3DO_Item.n_Priority;
			    /*
			     * SUPER TASKS and 3DO SIGNED TASKS can
			     * have any priority they want
			     */
			    if (( (t->t.n_Flags & TASK_SUPER) == 0) && (RSAok != 1)) {

			    /* Revalidate priority */
			      if ( (t->t.n_Priority < 10) || (t->t.n_Priority > 199) )
			      {
		    		DBUGCT(("BadPriority=%ld\n",t->t.n_Priority));
		    		ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadPriority);
		    		goto abort;
			      }
			    }
			}
		    }

	/* determine if this task is privileged */
	/* must be done after memory taken away from current task */
	/* rules:
	   C: if aif hdr RSA/MD5s then examine the
	      the WorkSpace flags in the decrypted aif hdr for AIF_PRIVILEGE
	      if set then nt is SUPER if not then nt is NOT SUPER
	*/
#ifdef OBSOLETE_AIF
		    DBUGCT(("aif_MD4DataSize=%ld\n",aifHdr->aif_MD4DataSize));
		    if (aifHdr->aif_MD4DataSize)
		    {
			bool ok;
			/* trigger RSA/MD5 processing */
			ok = RSACheck(aifHdr,tinfo.filesize);
			if (ok == FALSE)
			{
			    DBUGCT(("RSACheck failed\n"));
			    ret = MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_RSAFail);
			    goto abort;
			}
			if (aifHdr->aif_WorkSpace & AIF_PRIVILEGE)
					t->t.n_Flags |= TASK_SUPER;
		    }
#endif
		}

		DBUGCT(("Allocate a stack\n"));
		/* Allocate a stack */
		/* Round stack up to next int32 word */
		usermem_needed = (int)t->t_StackSize;
		usermem_needed += 3;
		usermem_needed &= ~3;
		t->t_StackSize = usermem_needed;

		/* add to this the List for the users free memory pool */
		usermem_needed += sizeof(List) * 2;
		usermem_needed += sizeof(List) * 2;

		if (vmh2)
		    usermem_needed += sizeof(List) * 2;

		/* need at least a node of 32bytes for seed (16 will do) */
		usermem_needed += 32;

		/*printf("aifHdr=%lx filesize=%d\n",aifHdr,tinfo.filesize);*/
		if (aifHdr && tinfo.filesize)
		{   /* try to use left over memory */
		    int32 imagepage;
		    DBUGCT(("filesize=%d imagesize=%d\n",tinfo.filesize,imagesize));
		    /* correct for iafhdr not on page boundary (operator) */
		    imagesize += (uint32)aifHdr &
					 (uint32)defaultmh->memh_PageMask;
		    if (ct == 0)
		    {	/* operator special stuff */
			DBUGCT(("Stupid Operator tricks\n"));
			/* recompute these since kernel lies to us */
			/* shrink operator to fit into pages it uses */
			imagesize = aifHdr->aif_ImageROsize +
			    aifHdr->aif_ImageRWsize +
			    (~15 &(aifHdr->aif_ZeroInitSize+15));
			/* round up to full pages */
		    	imagepage = imagesize + defaultmh->memh_PageMask;
		    	imagepage &= ~defaultmh->memh_PageMask;
		    }

		    /* make sure imagesize is rounded up to 16 byte boundary */
		    imagesize += 15;
		    imagesize &= ~15;

		    imagepage = imagesize + defaultmh->memh_PageMask;
		    imagepage &= ~defaultmh->memh_PageMask;

		    leftover = imagepage - imagesize;
		    DBUGCT(("leftover=%d usermem_needed=%d\n",leftover,usermem_needed));
		    if (leftover > usermem_needed)
		    {
			ustack = (uint8 *)aifHdr + imagesize;
			ustack -= (uint32)aifHdr & (uint32)defaultmh->memh_PageMask;
			/*DBUGCT(("b4 bzero: dml=%lx\n",dml));*/
			bzero(ustack,(int)leftover);
			/*DBUGCT(("after bzero: dml=%lx\n",dml));*/
			top = ustack + leftover;
			DBUGCT(("Setting new top to:%lx\n",top));
			leftover = 0;
		    }
		}

		DBUGCT(("b4 check ustack=%lx mh=%lx\n",(long)ustack,(long)defaultmh));
		DBUGCT(("dml=%lx\n",dml));
		if (ustack == 0)
		{
		    /* round up to page boundary */
		    usermem_needed += (int)defaultmh->memh_PageMask;
		    size = usermem_needed >> defaultmh->memh_PageShift;
		    ustack = (uint8 *)(AllocPages(defaultmh,t,size,0)*defaultmh->memh_PageSize);
		    DBUGCT(("ustack=%lx size=%d pages\n",ustack,size));
		    if (ustack==0) goto abort;
		    /* may be a different mtype though */
		    defaultmh = internalFindMH(ustack);
		    bzero(ustack,(int)(size*defaultmh->memh_PageSize));
		    top = ustack + size*defaultmh->memh_PageSize;
		}

		/* Now have size Pages for this stack, hack out a List Node */
		DBUGCT(("ustack=%lx top=%lx\n",(uint32)ustack,(uint32)top));

		/* allocate and initialize VRAM bank 1 pool */
		top -= sizeof(List);
		ml->meml_l = (List *)top;
		InitList(ml->meml_l,"Task VRAM Bank 1 MemList");
                top -= sizeof(List);
		ml->meml_AlignedTrackSize = (List *)top;
		InitList(ml->meml_AlignedTrackSize,"Task Aligned Tracked VRAM Allocs");

                if (vml2)
                {
                    /* allocate and initialize VRAM bank 2 pool */
                    top -= sizeof(List);
                    vml2->meml_l = (List *)top;
                    InitList(vml2->meml_l,"Task VRAM Bank 2 MemList");
                    top -= sizeof(List);
                    vml2->meml_AlignedTrackSize = (List *)top;
                    InitList(vml2->meml_AlignedTrackSize,"Task Aligned Tracked VRAM Allocs");
                }

		/* allocate and initialize DRAM pool */
		top -= sizeof(List);
		dml->meml_l = (List *)top;
		InitList(dml->meml_l,"Task DRAM MemList");
		top -= sizeof(List);
		dml->meml_AlignedTrackSize = (List *)top;
		InitList(dml->meml_AlignedTrackSize,"Task Aligned Tracked DRAM Allocs");

		DBUGCT(("dml=%lx top=%lx\n",dml,top));

		top -= sizeof(int32);
		if (tinfo.cmdstr)
		{
			top -= tinfo.cmdstrsize;
			DBUGCT(("Copying cmdstr\n"));
			strcpy(top,tinfo.cmdstr);
		}
		else
		{
			uint32 *p = (uint32 *)top;
			*p = 0;
		}
		t->t_sp = (uint32 *)top;	/* initial sp */
		top -= t->t_StackSize;

		{
		/*
		 * Our memory allocator prefers to deal with free nodes
		 * that are always multiples of 16 bytes long (and on
		 * 16 byte bounds).  So we extend the base of our stack
		 * down until it is 16-byte aligned, so the memory
		 * below it can be properly placed in the freelist.
		 */
		    int32	misa;
		    misa = (int32)top & 15;
		    if (misa) {
			t->t_StackSize += misa;
			top -= misa;
		    }
		}

		t->t_StackBase = (uint32 *)top;
		TrashMem(t->t_StackBase,0xDEADCAFE,t->t_StackSize);

		size = top - ustack;

		/* Which memlist gets the leftover memory? */

		if (defaultmh == dmh)
		{
			DBUGCT(("Adding leftover mem to %s\n",dml->meml_n.n_Name));
			ml = dml;
			AddMem(dml,(Node *)ustack,size);
		}
		else /* must figure it out */
		{
			MemHdr *mh = internalFindMH(ustack);
			DBUGCT(("initing memlist metype=%lx\n",mh->memh_Types));
			if (mh->memh_Types & MEMTYPE_VRAM)
			    AddMem(ml,(Node *)ustack,size);
			else
			    AddMem(dml,(Node *)ustack,size);
		}

	        t->t_regs[10] = (uint32)t->t_StackBase+128;	/* set stack limit register */
	}

	if (tinfo.exitmsgport)
	{
	    /* Set up an exit-message for the child */
	    TagArg  ta[5];

	    ta[0].ta_Tag = TAG_ITEM_NAME;
	    ta[0].ta_Arg = (void *)"Exit Msg";
	    ta[1].ta_Tag = TAG_ITEM_PRI;
	    ta[1].ta_Arg = (void *)t->t.n_Priority;
	    ta[2].ta_Tag = CREATEMSG_TAG_REPLYPORT;
	    ta[2].ta_Arg = (void *)tinfo.exitmsgport;
	    ta[3].ta_Tag = CREATEMSG_TAG_MSG_IS_SMALL;
	    ta[4].ta_Tag = TAG_END;

	    ret = internalCreateSizedItem(MKNODEID(KERNELNODE,MESSAGENODE),ta,0);
	    if (ret < 0) goto abort;

	    t->t_ExitMessage = ret;
	}

	VERIFYMEM("before Init regs: bad node\n");

	DBUGCT(("Initialize stackbase to %lx\n",t->t_StackBase));

	t->t_MaxTicks = USec2Ticks(t->t_MaxUSecs) - 1;

	t->t_lk = (uint32)ForceExit; /* in case user does an rts */
	t->t_psr = 0x10;	/* Really the saved Program status register */

	/* Hack for AIF crap */
	/* Because the jerks eat my startup registers! */
	t->t_regs[5] = t->t_regs[0];
	t->t_regs[6] = t->t_regs[1];
	t->t_regs[7] = t->t_regs[9];

	DBUGCT(("Disabled=%lx\n",Disabled()));
	oldints = Disable();

	/* Add the new task to the list of tasks in the system */
	ADDTAIL(KernelBase->kb_Tasks, (Node *)&(t->t_TasksLinkNode));

	if (ct == 0) ULaunch(t);	/* Operator starting up */
	/* Now we must add it to the ready Q */
	t->t.n_Flags |= TASK_READY;
	/* Is the new task going to bump current task? */
	TailInsertNode(KernelBase->kb_TaskReadyQ,(Node *)t);

        /* There was a bug here until version 1.133 of this file. If
         * the bug fix has to be yanked for any reason, than kludge
         * code must be added in the operator to let the Flopon game
         * work. The kludge code was in the operator until version
         * 1.133 of operator.c
         */
	if (ct->t.n_Priority < t->t.n_Priority)
		KernelBase->kb_PleaseReschedule = 1;

	Enable(oldints);
	return t->t.n_Item;

abort:
	/* problem some where */
	/* The Free routine is defined to zero check */
	DBUGCT(("Aborting CreateTask t=%lx\n",t));
	if (mp)
	    FreeNode((Folio *)KernelBase, (ItemNode *)mp);
	if (gavemem)
	{
	    /* give memory back to caller */
	    internalControlMem(t,(char *)aifHdr,gavemem,MEMC_GIVE,ct->t.n_Item);
	}
	if (t)	FreeTask(t);
err:
	DBUGCT(("CreateTask ret=%lx\n",ret));
	return ret;
}

Item
internalCreateTaskVA(Task *t,uint32 args, ...)
{
	return internalCreateTask(t, (TagArg *)&args);
}
void
CLand(t)
Task *t;
{
	uint8 flags;
	uint32 i;
	struct timeval *tv;

	/*KernelBase->kb_PleaseReschedule = 0;*/
	/* paw through the pertask table and call SAVE */
	if ((i = t->t_FolioContext) != 0)
	{
	    do
	    {
		int32 j = (int32)ffs((unsigned)i) - 1;
		i &= ~((uint32)1<<j);
		(*KernelBase->kb_DataFolios[j]->f_FolioSaveTask)(t);
	    }
	    while (i);
	}
	/* update this guys elapsed time */
	tv = &t->t_ElapsedTime;
	i = ReadTimer(quantaclock->tm_ID);
	if (i == 0xffff)
	    tv->tv_usec += t->t_MaxUSecs;
	else
	{
		i = t->t_MaxTicks  - i;
		tv->tv_usec += SimpleTicksToUsecs(i);
	}
	if (tv->tv_usec > 1000000)
	{
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}
#ifdef FORBID
	t->t_Forbid = KernelBase->kb_Forbid;
#endif
	flags = t->t.n_Flags & ~TASK_RUNNING;
	t->t.n_Flags = TASK_READY | flags;
}

Task *
ULand(t)
Task *t;
{
	CLand(t);
	return Land(t);
}

extern int32 Arm600;

void
CLaunch(t)
Task *t;
{
	uint8 flags;
	uint32 i;

	CURRENTTASK = t;
	KernelBase->kb_ElapsedQuanta = 0;

#ifdef DEVELOPMENT
	KernelBase->kb_NumTaskSwitches++;
	t->t_NumTaskLaunch++;
#endif

#ifdef FORBID
	KernelBase->kb_Forbid = t->t_Forbid;
#endif
	KernelBase->kb_PleaseReschedule = 0;
	flags = t->t.n_Flags | TASK_RUNNING;
	t->t.n_Flags = flags & (~(TASK_READY|TASK_WAITING));
	LoadFenceBits(t);
	if ((i = t->t_FolioContext) != 0)
	{
	    do
	    {
		int32 j = (int32)ffs((unsigned)i)-1;
		i &= ~((uint32)1<<j);
		(*KernelBase->kb_DataFolios[j]->f_FolioRestoreTask)(t);
	    }
	    while (i);
	}
	/* restart quanta ticker */
	LoadTimer((int32)quantaclock->tm_ID,t->t_MaxTicks,t->t_MaxTicks);
	ControlTimer(quantaclock,(int32)TIMER_DECREMENT,0);
}

void
ULaunch(t)
Task *t;
{
	CLaunch(t);
	Launch(t);
}

void
USwap(oldtask,newtask)
Task *oldtask,*newtask;
{
	CLand(oldtask);
	CLaunch(newtask);
	if (Land(oldtask))	return;
	Launch(newtask);
}

int32
cblank(void)
/* round robin schedule for same priority tasks */
{
    Task *t = CURRENTTASK;
    Task *nt;
    struct timeval *tv;
    List *ReadyQ = KernelBase->kb_TaskReadyQ;

    /* Is there a task waiting? */
    if (!t) return 0;

    /* This tasks quanta timed out */
    nt = (Task *)FIRSTNODE(ReadyQ);
    if (ISNODE(ReadyQ,nt))	/* another task? */
    {
    	/* If current tasks priority is the same or */
    	/* less than the task on the readyQ, do reschedule */
    	if (t->t.n_Priority <= nt->t.n_Priority)
    	    KernelBase->kb_PleaseReschedule = 1;
    }
    /* need to restart the timer */
    /* restart quanta ticker */
    if (KernelBase->kb_PleaseReschedule == 0)
    {
        tv = &t->t_ElapsedTime;
        tv->tv_usec += t->t_MaxUSecs;
        if (tv->tv_usec >= 1000000)
        {
            tv->tv_usec -= 1000000;
            tv->tv_sec++;
        }
        LoadTimer((int32)quantaclock->tm_ID,t->t_MaxTicks,t->t_MaxTicks);
        ControlTimer(quantaclock,(int32)TIMER_DECREMENT,0);
    }
    return 0;
}

#ifdef FORBID
void
internalForbid(void)
{
	KernelBase->kb_Forbid++;
}
void
internalPermit(void)
{
	KernelBase->kb_Forbid--;
	/* Need to check to see if tasks need to be recomputed */
}
#endif


/* Returns TRUE if both task pointers refer to either a task or threads
 * which are sharing the same address space. So basically, are both of
 * these threads of the same task, or is one a thread of the other
 */
bool IsSameTaskContext(Task *t1, Task *t2)
{
    if (t1->t_FreeMemoryLists == t2->t_FreeMemoryLists)
        return TRUE;

    return FALSE;
}

static char refmday[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

#define	XPRINTF(f)	do { printf f; } while (0)

static char	npfmt[] = "%-16.16s ($%06X)";
static char	vfmt[] = " %s%02d.%03d";
static char	dfmt[] = " %02d/%02d/%02d";
static char	tfmt[] = " %02d:%02d:%02d";

extern char	copyright[];

void
internalPrint3DOHeader(_3DOBinHeader *p3do, char *whatstr, char *copystr)
{
	uint32                  v, r;
	char                   *n;

	n = p3do->_3DO_Name;
	if (!n || !*n) n = "Anonymous";
	XPRINTF ((npfmt, n, (char *)p3do-sizeof(AIFHeader)));
	v = p3do->_3DO_Item.n_Version;
	r = p3do->_3DO_Item.n_Revision;
	if (v || r) XPRINTF ((vfmt, "v", v, r));
	v = p3do->_3DO_OS_Version;
	r = p3do->_3DO_OS_Revision;
	if (v || r) XPRINTF ((vfmt, "os", v, r));

	{

		/* secs is the number of seconds since 01/01/93 00:00:00 GMT;
		 * convert to mm/dd/yy hh:mm:ss and display
		 */

		int32                   hour, min, sec;
		int32                   leap, year, month, day;
		extern int		timezone;

		char	mday[12];

		for (month = 0; month < 12; ++month)
			mday[month] = refmday[month];

		sec = p3do->_3DO_Time;
		sec -= timezone;
		min = sec / 60;
		hour = min / 60;
		day = hour / 24;

		leap = day / (3 * 365 + 366);
		day -= leap * (3 * 365 + 366);
		year = day / 365;
		if (year == 4) --year;		/* last day of the leapyear */
		day -= year * 365;

		if (!(year&3)) mday[1] ++;
		for (month = 0; (month < 12) && (day >= mday[month]); month++)
			day -= mday[month];

		year = (leap*4 + year + 93) % 100;
		day++;
		month++;
		hour %= 24;
		min %= 60;
		sec %= 60;

		XPRINTF ((dfmt, year, month, day));
		XPRINTF ((tfmt, hour, min, sec));
	}
	XPRINTF ((" %s\n", whatstr ? whatstr : ""));
	if (copystr && *copystr && ((copystr == copyright) || strcmp(copystr, copyright)))
		XPRINTF (("%s\n", copystr));
}

Err
internalSetTaskOwner(Task *t, Item newowner)
{
	Task	*pt = t->t_ThreadTask;
	Item	towner;
	Item	ti;

	if (pt)
	{
		/* Trying to change ownership of a thread */

		Task	*nt = (Task *)LookupItem(newowner);

		/* Don't allow thread to chown to another task or its thread */
		if ((pt != nt) && (pt != nt->t_ThreadTask))
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);

		/* Don't allow thread to chown to itself */
		if (t == nt)
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);
	}

	towner = newowner;
	ti = t->t.n_Item;
	do
	{
		/* Check for cyclic ownership relation */

		towner = ((Task *)LookupItem(towner))->t.n_Owner;

		if (towner == ti)
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);
	}
	while (towner);

	return 0;
}
