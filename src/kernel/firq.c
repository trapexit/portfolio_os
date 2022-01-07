
/* $Id: firq.c,v 1.64 1994/09/19 19:28:02 vertex Exp $ */

#include "types.h"

#include "nodes.h"
#include "kernelnodes.h"
#include "item.h"

#include "folio.h"
#include "list.h"
#include "listmacros.h"
#include "task.h"

#include "interrupts.h"
#include "timer.h"
#include "mem.h"
#include "strings.h"

#include "kernel.h"

#include "internalf.h"
#include "operror.h"

#include "inthard.h"
#include "setjmp.h"
#include "stdio.h"

#include "clio.h"

#include "sysinfo.h"
#include "super.h"

#define	DBUGFIRQINIT(x)		/* printf x */
#define	DBUGHL(x)		/* printf x */
#define	DBUGHLINIT(x)		/* printf x */

/*
 * SUPPORT_ACS: adds support for Asynchronous Call Services Works, but
 * not used in this release.
 */
#undef	SUPPORT_ACS

/*
 * SUPPORT_VBLACS: adds support for automatic ACS at top of frame
 * Works, but not used in this release. You must also turn on
 * SUPPORT_ACS for this.
 */
#ifdef	SUPPORT_ACS
#undef	SUPPORT_VBLACS
#endif	/* SUPPORT_ACS */

/*
 * SUPPORT_HLINTS: adds support for FIRQs that trigger at any specific
 * horizontal line. Still in development, they appear now #ifdef'd out
 * so the HLINT development does not diverge from the real sources.
 */
#define	SUPPORT_HLINTS
#define	ACTIVATE_HLINTS

/*
 * TEST_HLINTS: enables the hlint quick-test (chalkmark) code. You
 * must also turn on SUPPORT_HLINTS for this.
 */
#ifdef	SUPPORT_HLINTS
#undef	TEST_HLINTS
#endif	/* SUPPORT_HLINTS */

#ifdef	SUPPORT_HLINTS

#define	DEFAULT_HLINT_LINE	5			/* "line 4" */

volatile FirqNodeP      hlint_nextup = 0;		/* next HLInt handler to run */

void                    internalRegisterHLInt (FirqNode *firq);
Item                    internalCreateHLInt (FirqNode *firq, TagArg *tagpt);
Item                    internalCreateHLIntVA (FirqNode *firq, uint32 tags,...);
Item                    internalAllocHLInt (char *name, int pri, int line, void (*code) (void));
void                    runHLInts (void);

#endif	/* SUPPORT_HLINTS */


#ifdef	TEST_HLINTS
vuint32                 hlint_entries = 0;		/* hlint call counter */
vuint32                 hlint_req_line = 0;		/* last value sent to VInt0 */
vuint32                 hlint_tests = 15;		/* test enables */
vuint32                *hlint_wall05 = 0;		/* chalk marks for line 05 handler */
vuint32                *hlint_wall06 = 0;		/* chalk marks for line 06 handler */
vuint32                *hlint_wall40 = 0;		/* chalk marks for line 40 handler */
vuint32                *hlint_wallF0 = 0;		/* chalk marks for line F0 handler */

void                    HLIntMarkWall05 (FirqNode *firq);
void                    HLIntMarkWall06 (FirqNode *firq);
void                    HLIntMarkWall40 (FirqNode *firq);
void                    HLIntMarkWallF0 (FirqNode *firq);
void                    setHLIntTest (void);

#endif	/* TEST_HLINTS */

extern void             kernelVBL (void);		/* VSync, maybe watchdog, maybe top-of-frame ACS list */
extern void             cblank (void);

extern void             FirqHandler (void);		/* general purpose */

extern void		InitConioFirqs (void);

extern Timer           *quantaclock;
extern void             Panic (int halt, char *);

#ifdef	SUPPORT_VBLACS
List                    kernelVBLlist;
List                   *kernelVBLlistp = 0;
#endif	/* SUPPORT_VBLACS */

#ifdef	SUPPORT_ACS
List                    ACSlist;
extern List            *ACSlistp;
extern int              ACS_Enable;
#endif	/* SUPPORT_ACS */

uint32
FirqInterruptControl (int32 n, int32 cmd)
{
	uint32                  msk;
	uint32                  was;
	Clio                   *clio = (Clio *) CLIO;
	ClioInterrupts         *p;

	if (n < 0)
		goto error;
	if (n < 32)
		p = &clio->ClioInt0;
	else
	{
		if (n >= INT_MAX)
			goto error;
		p = &clio->ClioInt1;
		n -= 32;
	}
	msk = (uint32) 1 << n;
	switch (cmd)
	{
	case FIRQ_ENABLE:
		was = p->SetIntEnBits;
		p->SetIntEnBits = msk;
		break;
	case FIRQ_DISABLE:
		was = p->ClrIntEnBits;
		p->ClrIntEnBits = msk;
		break;
	case FIRQ_CLEAR:
		was = p->ClrIntBits;
		p->ClrIntBits = msk;
		break;
	case FIRQ_SET:
		was = p->SetIntBits;
		p->SetIntBits = msk;
		break;
	default:
		goto error;
	}
	was &= msk;
	return was;

error:
#ifdef DEVELOPMENT
	printf ("Error, bad command to FirqInterruptControl(%d,%d)\n", (int) n, (int) cmd);
	while (1);
#else
	Panic (1, "Bad command to FirqInterruptControl\n");
#endif
	return -1;
}

Item
internalCreateFirqVA (FirqNode *firq, uint32 args,...)
{
	return internalCreateFirq (firq, (TagArg *) &args);
}

Item
AllocFirq (char *name, int pri, int num, void (*code) ())
{
	FirqNode               *firq;
	Item                    firqI;

	firq = (FirqNode *) AllocateNode ((Folio *) KernelBase, FIRQNODE);
	if (firq == (FirqNode *) 0)
		return NOMEM;

	firqI = internalCreateFirqVA (firq,
				      TAG_ITEM_NAME, name,
				      TAG_ITEM_PRI, pri,
				      CREATEFIRQ_TAG_NUM, num,
				      CREATEFIRQ_TAG_CODE, code,
				      TAG_END);
	if (firqI < 0)
		FreeNode ((Folio *) KernelBase, firq);

	return firqI;
}

#ifdef	SUPPORT_HLINTS
static char            *kernel_HLInt = "scan line interrupt manager";
#endif

static char            *kernel_default = "kernel default";
static char            *kernel_quanta = "kernel quanta";
static char            *bad_bits = "BadBits";
static char            *SoftInt = "SoftInt";

extern int              BadBitHandler (void);
extern void             SoftIntHandler (void);

void
firq_err (char *n, Item i)
{
	PrintError ("start_firq", "create firq", n, i);
#ifdef DEVELOPMENT
	printf ("start_firq: error creating:%s : %lx\n", n, i);
	while (1);
#else
	Panic (1, "start_firq: error on create\n");
#endif

}

#ifdef	SUPPORT_ACS
void
ACS_err (char *n, Item i)
{
	PrintError ("start_ACS", "create ACS", n, i);
#ifdef DEVELOPMENT
	printf ("start_ACS: error creating:%s : %lx\n", n, i);
	while (1);
#else
	Panic (1, "start_ACS: error on create\n");
#endif
}
#endif	/* SUPPORT_ACS */

void
start_firq (void)
{
	Item                    firqI;

#ifdef	SUPPORT_ACS
	InitList (&ACSlist, "ACS queue");
	ACSlistp = &ACSlist;
	ACS_Enable = 1;
#endif	/* SUPPORT_ACS */

#ifdef	SUPPORT_VBLACS
	InitList (&kernelVBLlist, "VBL ACS queue");
	kernelVBLlistp = &kernelVBLlist;
#endif	/* SUPPORT_VBLACS */

#ifdef	ACTIVATE_HLINTS
	/* set up kernel horizontal-line interrupt */
	/* used to provide "any line" interrupt service. */
	firqI = AllocFirq (kernel_HLInt, 250, INT_V1, runHLInts);
	if (firqI < 0)
		firq_err (kernel_HLInt, firqI);
#endif	/* ACTIVATE_HLINTS */

#ifdef	TEST_HLINTS
	setHLIntTest ();
#endif	/* TEST_HLINTS */

	/* set up kernel default vblank interrupt */
	/* different routine used for WatchDog */
	/* as long as you aren't running on a development machine. */
	/* On a development machine, we let sleeping dogs lie. */
	firqI = AllocFirq (kernel_default, 0, INT_V1, kernelVBL);
	if (firqI < 0)
		firq_err (kernel_default, firqI);

	/* set up round robin quanta task interrupt */
	firqI = AllocFirq (kernel_quanta, 250, quantaclock->tm_IntNum, cblank);
	if (firqI < 0)
		firq_err (kernel_quanta, firqI);

	/* set up badbits handler */
	firqI = AllocFirq (bad_bits, 250, INT_BADBITS, (void (*) ()) BadBitHandler);
	if (firqI < 0)
		firq_err (bad_bits, firqI);

	/* set up software interrupt handler */
	firqI = AllocFirq (SoftInt, 250, INT_Spare, SoftIntHandler);
	if (firqI < 0)
		firq_err (SoftInt, firqI);

	/* Now jump to list handler */
	InstallArmVector (0x1c, FirqHandler);
	ControlTimer (quantaclock, TIMER_INTEN, 0);
	EnableInterrupt ((int32) INT_BADBITS);
	EnableInterrupt ((int32) INT_Spare);		/* SoftwareInterrupt */

	/* console I/O may have firqs it wants, but
	 * it has already been initialized. Ich. */
	InitConioFirqs();
}

static int32
icf_c (FirqNode *firq, void *p, uint32 tag, uint32 arg)
{
	switch (tag)
	{
	case CREATEFIRQ_TAG_DATA:
		firq->firq_Data = (int) arg;
		break;
	case CREATEFIRQ_TAG_CODE:
		firq->firq_F = Make_Func (int32, arg);
		break;
	case CREATEFIRQ_TAG_FLAGS:
		firq->firq.n_Flags |= (uchar) arg;
		break;
	case CREATEFIRQ_TAG_NUM:
		firq->firq_Num = (int) arg;
		break;
	default:
		return BADTAG;
	}
	return 0;
}

Item
internalCreateFirq (FirqNode *firq, TagArg *tagpt)
{
	int32                   num;
	Node                   *n;
	Task                   *t = KernelBase->kb_CurrentTask;
	Item                    ret;
	uint32                  oldints;

	if (t && ((t->t.n_Flags & TASK_SUPER) == 0))
		return BADPRIV;

	firq->firq_Num = -1;
	ret = TagProcessor (firq, tagpt, icf_c, 0);

	if (firq->firq.n_Name == 0)
		return BADTAGVAL;

	if (firq->firq_F == 0)
	{
		ret = BADTAGVAL;
		goto done;
	}
	num = firq->firq_Num;
	if ((num < 0) | (num > 47))
	{
		ret = BADTAGVAL;
		goto done;
	}

	n = KernelBase->kb_InterruptHandlers[num];

	if (n == 0)
	{
		DBUGFIRQINIT (("CreateFirq(%d): first server for %d at pri %d is 0x%lx (%s)\n", __LINE__,
			       num, firq->firq.n_Priority, firq->firq_F, firq->firq.n_Name));
		KernelBase->kb_InterruptHandlers[num] = (Node *) firq;
	}
	else
	{
#ifdef	ACTIVATE_HLINTS
/*
 * Only the first INT_V1 handler is allowed to be a FIRQ -- it is the
 * HBL comparator manager. All others are turned into HLINTs on the
 * default line to be used for VBL service.
 */
		if (num == INT_V1)
		{
			firq->firq_Num = DEFAULT_HLINT_LINE;	/* HLInts use firq_num as line number */
			internalRegisterHLInt (firq);
			return firq->firq.n_Item;
		}
#endif	/* ACTIVATE_HLINTS */
		if (n->n_Type != LISTNODE)
		{					/* must allocate a list */
			List                   *l;

			l = (List *) AllocateNode ((Folio *) KernelBase, LISTNODE);
			if (l == (List *) 0)
			{
				ret = NOMEM;
				goto done;
			}
			InitList (l, 0);

			oldints = Disable ();
			KernelBase->kb_InterruptHandlers[num] = (Node *) l;
			/* Put old node on new list */
			TailInsertNode (l, n);
			n = (Node *) l;
			Enable (oldints);
		}
		oldints = Disable ();
		TailInsertNode ((List *) n, (Node *) firq);
		Enable (oldints);
	}
	ret = firq->firq.n_Item;

done:
	return ret;
}

int32
internalDeleteFirq (FirqNode *f, Task *t)
{
	uint32                  oldints;
	Node                   *n;

	n = KernelBase->kb_InterruptHandlers[f->firq_Num];

	oldints = Disable ();
	if (n == (Node *) f)
	{
		/* simple */
		KernelBase->kb_InterruptHandlers[f->firq_Num] = 0;
		/* Make sure interrupts are disabled for this num */
		DisableInterrupt (f->firq_Num);
	}
	else
	{						/* a list of em */
		REMOVENODE ((Node *) f);
		/* Was this the last one on the list? */
		if (ISEMPTYLIST ((List *) n))
			DisableInterrupt (f->firq_Num);
	}
	/* We do not attempt to reclaim empty lists at this time */
	Enable (oldints);

	return 0;
}

/*
 * dispatch control to FIRQ handlers
 */

void
cfirq (void)
{
	uint32                  interrupts;
	uint32                  firstint;
	int32                   i;
	List                   *l;
	Node                   *n;
	FirqNode               *fn;
	ClioInterrupts         *ip0;
	ClioInterrupts         *ip1;
	ClioInterrupts         *ip;

	/* read pending interrupts */

	ip0 = &(((Clio *) CLIO)->ClioInt0);
	ip1 = &(((Clio *) CLIO)->ClioInt1);
	while (1)
	{
		ip = ip0;
		interrupts = ip->SetIntBits & ip->SetIntEnBits;

		if (interrupts == 0)
			break;
		firstint = interrupts & ~(interrupts - 1);
		if (firstint >> 16)
			i = 24;
		else
			i = 8;
		if (firstint >> i)
			i += 4;
		else
			i -= 4;
		if (firstint >> i)
			i += 2;
		else
			i -= 2;
		if (firstint >> i)
			i += 1;
		else
			i -= 1;
		if (firstint >> i)
			i += 1;
		i -= 1;

		/* Do we need to check the second word? */
		if (i == INT_SecondPri)
		{
			/* get the second word of interrupts */
			ip = ip1;
			interrupts = ip->SetIntBits & ip->SetIntEnBits;
			if (interrupts == 0)
			{
				ip0->ClrIntBits = firstint;
				continue;
			}
			firstint = interrupts & ~(interrupts - 1);
			if (firstint >> 16)
				i = 24;
			else
				i = 8;
			if (firstint >> i)
				i += 4;
			else
				i -= 4;
			if (firstint >> i)
				i += 2;
			else
				i -= 2;
			if (firstint >> i)
				i += 1;
			else
				i -= 1;
			if (firstint >> i)
				i += 1;
			i += 31;
		}
		/* now have the first bit# i */
		if ((n = KernelBase->kb_InterruptHandlers[i]) != (Node *) 0)
		{
			ip->ClrIntBits = firstint;
			if (n->n_Type == FIRQNODE)
			{
				fn = (FirqNode *) n;
				(*fn->firq_F) (fn);
			}
			else
			{
				l = (List *) n;
				for (n = FIRSTNODE (l); ISNODE (l, n);
				     n = NEXTNODE (n))
				{
					fn = (FirqNode *) n;
					(*fn->firq_F) (fn);
				}
			}
		}
		else
		{
			/* serious panic here: */

/*
 * we have an interrupt and no one there to deal with it
 */
			/* Squash it */
			DisableInterrupt (i);
#ifdef DEVELOPMENT
			EnableIrq ();
			printf ("Panic: no interrupt handler for int:%d\n", (int) i);
			printf ("interrupts=%lx\n", interrupts);
			printf ("shutting down, ...now..\n");
			while (1);
#else
			Panic (1, "No interrupt handler");
#endif

		}
	}
}

/* returns whether we should start a task swap */
Task                   *
roundrobin (void)
{
	List                   *l;
	Task                   *t;
	Task                   *ct;

	/* Do this preprocessing and then check one more time */
	/* for another interrupt */
	if (KernelBase->kb_PleaseReschedule == 0)
		return 0;
#ifdef FORBID
	if (KernelBase->kb_Forbid)
		return 0;
#endif
	l = KernelBase->kb_TaskReadyQ;
	t = (Task *) FIRSTNODE (l);
	if (!ISNODE (KernelBase->kb_TaskReadyQ, t))
		return 0;
	ct = KernelBase->kb_CurrentTask;
	if (!ct)
		return 0;
	if (ct->t.n_Priority > t->t.n_Priority)
		return 0;
	return ct;
}

void
RunACS (void)
{
#ifdef	SUPPORT_ACS
	FirqNode               *firq;
	int32                   (*func) (FirqNode *);
	uint32                  bits;

	bits = Disable ();
	while ((firq = (FirqNode *) RemHead (ACSlistp)) != (FirqNode *) 0)
	{
		func = firq->firq_F;
		firq->firq_Num = 0;
		Enable (bits & ~0x40);
		(void) (*func) (firq);
		Disable ();
	}
#endif	/* SUPPORT_ACS */
}

Item
internalCreateACS (FirqNode *firq, TagArg *tagpt)
{
#ifdef	SUPPORT_ACS
	Task                   *t = KernelBase->kb_CurrentTask;

	if (t && ((t->t.n_Flags & TASK_SUPER) == 0))
		return BADPRIV;

	(void) TagProcessor (firq, tagpt, icf_c, 0);

	if (firq->firq.n_Name == 0)
		return BADTAGVAL;

	if (firq->firq_F != 0)
		return firq->firq.n_Item;

	return BADTAGVAL;
#else	/* SUPPORT_ACS */
	return NOSUPPORT;
#endif	/* SUPPORT_ACS */
}

Item
internalCreateACSVA (FirqNode *firq, uint32 args,...)
{
	return internalCreateACS (firq, (TagArg *) &args);
}

/**
|||	AUTODOC PRIVATE spg/kernel/allocacs
|||	AllocACS                   Allocate Item for Async Call Service
|||
|||	  SYNOPSIS
|||
|||	    Item AllocACS(char *name, int pri, void (*code)())
|||
|||	  DESCRIPTION
|||
|||	    AllocACS ...
|||
|||	  PARAMETERS
|||
|||	    name - the name to be used for the item
|||	    pri - the priority of the ACS; higher numbered ACS nodes
|||	      will be serviced before lower numbered ACS nodes, and
|||	      in the future, sufficiently low priority ACS calls will
|||	      be preempted by sufficiently high ACS calls.
|||	    code - a pointer to the function to be called.
|||
|||	  RETURNS
|||
|||	    AllocACS returns an item number that can be used by
|||	    subsequent PendACS calls.
|||
|||	  IMPLEMENTATION
|||	  ASSOCIATED FILES
|||	  COMMENTS
|||
|||	    AllocACS should be called once during initialization; the
|||	    item number it returns can be used over and over.
|||
|||	  SEE ALSO
|||
|||	    PendACS
**/

Item
internalAllocACS (char *name, int pri, int32 (*code) ())
{
#ifdef	SUPPORT_ACS
	FirqNode               *firq;
	Item                    firqI;

	firq = (FirqNode *) AllocateNode ((Folio *) KernelBase, FIRQNODE);
	if (firq == 0)
		return NOMEM;

	firqI = internalCreateACSVA (firq,
				     TAG_ITEM_NAME, name,
				     TAG_ITEM_PRI, pri,
				     CREATEFIRQ_TAG_NUM, 0,
				     CREATEFIRQ_TAG_CODE, code,
				     TAG_END);
	if (firqI < 0)
		FreeNode ((Folio *) KernelBase, firq);
	return firqI;
#else	/* SUPPORT_ACS */
	return NOSUPPORT;
#endif	/* SUPPORT_ACS */
}

/**
|||	AUTODOC PRIVATE spg/kernel/pendacs
|||	PendACS                    Request Async Call Service
|||
|||	  SYNOPSIS
|||
|||	    int32 PendACS(Item req)
|||
|||	  DESCRIPTION
|||
|||	    PendACS requests that the function described within the
|||	    item be called with FIRQs enabled, after other FIRQ
|||	    services have executed but before the interrupted code is
|||	    resumed.
|||
|||	  PARAMETERS
|||
|||	    req - the item number to place on the queue
|||
|||	  RETURNS
|||	  IMPLEMENTATION
|||	  ASSOCIATED FILES
|||	  COMMENTS
|||	  SEE ALSO
|||
|||	    AllocACS
**/

void
internalPendACSNode (FirqNode *firq)
{
#ifdef	SUPPORT_ACS
	int32                   oldints;

	oldints = Disable ();
	if (firq->firq_Num == 0)
	{
		firq->firq_Num++;
		TailInsertNode (ACSlistp, (Node *) firq);
	}
	Enable (oldints);
#endif	/* SUPPORT_ACS */
}

int32
internalPendACS (Item req)
{
#ifdef	SUPPORT_ACS
	FirqNode               *firq;

	firq = (FirqNode *) LookupItem (req);
	if (firq == (FirqNode *) 0)
		return BADITEM;
	internalPendACSNode (firq);
	return 0;
#else	/* SUPPORT_ACS */
	return NOSUPPORT;
#endif	/* SUPPORT_ACS */
}

/**
***	AUTODOC PRIVATE spg/kernel/registerperiodicvblacs
***	RegisterPeriodicVBLACS - Request ACS all future Vertical Blanks
***
***	  SYNOPSIS
***
***	    int	RegisterPeriodicVBLACS(Item it)
***
***	  DESCRIPTION
***
***	    RegisterVBLACS takes an item previously allocated with the
***	    AllocACS call and registers it in the list of ACS calls that are
***	    to be made at all future vertical blanking intervals.
***
***	  PARAMETERS
***
***	    req - the item number to place on the queue
***
***	  RETURNS
***
***	    zero if the registration was successful, or -1 if the periodic
***	    list was full.
***
***	  COMMENTS
***
***	    At this time, the number of ACS's that can be run in this manner
***	    is limited to 64 by a #define in the kernel source. If this
***	    service starts getting heavy use, a more dynamic data structure
***	    will be used.
***
***	    There is no way to "unregister."
***
***	    If the periodic list was full or if "unregistration" is desired,
***	    RegisterSingleVBLACS() may be used to get the same functionality
***	    by having the function "rearm" itself. To unregister, of course,
***	    have the function simply not rearm.
***
***	    All VBL ACSs, Periodic and Single, are run in priority order.
***	    Calls with the same priority order are run in the order presented
***	    to the system, except that Single calls are run before Periodic
***	    calls of the same priority.
***
***	  SEE ALSO
***
***	    RegisterSingleVBLACS(), AllocACS()
**/

#ifdef	SUPPORT_VBLACS
#define	MAX_VBL_ACS	64
uint32                  kernelVBLcount = 0;
Item                    kernelVBLarray[MAX_VBL_ACS];
#endif	/* SUPPORT_VBLACS */

int
internalRegisterPeriodicVBLACS (Item it)
{
#ifdef	SUPPORT_VBLACS
	if (kernelVBLcount >= MAX_VBL_ACS)
		return NOMEM;
	kernelVBLarray[kernelVBLcount] = it;
	kernelVBLcount++;
	return 0;
#else	/* SUPPORT_ACS */
	return NOSUPPORT;
#endif	/* SUPPORT_ACS */
}

/**
***	AUTODOC PRIVATE spg/kernel/registersinglevblacs
***	RegisterSingleVBLACS - Request ACS at Next Vertical Blank
***
***	  SYNOPSIS
***
***	    int RegisterSingleVBLACS(Item it)
***
***	  DESCRIPTION
***
***	    RegisterVBLACS takes an item previously allocated with the
***	    AllocACS call and registers it in the list of ACS calls that are
***	    to be made at the next vertical blanking interval.
***
***	  PARAMETERS
***
***	    req - the item number to place on the queue
***
***	  RETURNS
***
***	    zero for success, or "-1" if the item number was invalid.
***
***	  COMMENTS
***
***	    If a function is to be called every vertical blanking period, it
***	    is more efficient to use RegisterPeriodicVBLACS() than to use
***	    RegisterSingleVBLACS() with the function rearming itself every
***	    call.
***
***	    All VBL ACSs, Periodic and Single, are run in priority order.
***	    Calls with the same priority order are run in the order presented
***	    to the system, except that Single calls are run before Periodic
***	    calls of the same priority.
***
***	  SEE ALSO
***
***	    RegisterPeriodicVBLACS(), AllocACS()
**/

int
internalRegisterSingleVBLACS (Item it)
{
#ifdef	SUPPORT_VBLACS
	Node                   *n;

	n = (Node *) LookupItem (it);
	if (n == (Node *) 0)
		return BADITEM;
	TailInsertNode (kernelVBLlistp, n);
	return 0;
#else	/* SUPPORT_VBLACS */
	return NOSUPPORT;
#endif	/* SUPPORT_VBLACS */
}

void
kernelVBL (void)
{
	Clio                   *clio;

#ifdef	SUPPORT_VBLACS
	uint32                  i;
	List                   *l;
	FirqNode               *firq;
#endif	/* SUPPORT_VBLACS */

	/* acknowledge the interrupt */
	clio = (Clio *) CLIO;
	clio->ClioInt0.ClrIntBits = 2;

	/* if not running with the debugger, reset the watchdog */
	if (KernelBase->kb_CPUFlags & KB_NODBGR)
		SuperSetSysInfo(SYSINFO_TAG_WATCHDOG, (void *)SYSINFO_WDOGENABLE, 0);

#ifdef	SUPPORT_VBLACS
	/* pend one-shot VBL ACSs */
	if ((l = kernelVBLlistp) != (List *) 0)
		while ((firq = (FirqNode *) RemHead (l)) != (FirqNode *) 0)
			(void) internalPendACSNode (firq);

	/* pend periodic VBL ACSs */
	for (i = 0; i < kernelVBLcount; ++i)
		(void) internalPendACS (kernelVBLarray[i]);
#endif							/* SUPPORT_VBLACS */
}

#ifdef	SUPPORT_HLINTS

/*
 * sorting by ascending HLINT_ORDER sorts by ascending line and
 * descending pri within line.
 */
#define	HLINT_ORDER(line,pri)	(((line) << 8) - (pri))
#define	HLINT_BEAM_ORDER(beam)	HLINT_ORDER((beam)&VCNT_MASK, 0)
#define	HLINT_FIRQ_ORDER(fn)	HLINT_ORDER((fn)->firq_Num, (fn)->firq.n_Priority)

/**
|||	AUTODOC PRIVATE spg/kernel/registerhlint
|||	RegisterHLInt - request firq service on a specific line
|||
|||	  Synopsis
|||
|||         void RegisterHLInt( FirqNode *firq )
|||
|||	  Description
|||
|||         Similar to AllocHLInt() but used when a firq node is
|||         already in hand with all the fields filled in. This entry
|||         point is provided for the conveinence of the FIRQ creation
|||         code to speed the redirection of older INT_V1 FIRQ
|||         handlers into using the new mechanism.
|||
|||	  Arguments
|||
|||         firq                        A pointer to the
|||                                     already-constructed FIRQ node,
|||                                     with the scan line number
|||                                     substituted for the FIRQ index
|||	                                number.
|||
|||	  Implementation
|||	  Associated Files
|||	  Notes
|||	  Caveats
|||
|||         Interrupt latencies in the system may result in the
|||         routine being called later than expected. If your code has
|||         a hard completion-time, you must read the horizontal line
|||         counter yourself.
|||	  See Also
|||
|||         AllocHLInt(), CreateHLInt()
**/

void
internalRegisterHLInt (FirqNode *firq)
{
	uint32                  line;
	FirqNode               *sent;
	FirqNode               *prev;
	FirqNode               *next;
	uint32                  oldints;

	DBUGHLINIT (("RegisterHLInt(%d): firq=0x%lx\n", __LINE__, firq));

	line = firq->firq_Num;

#ifdef	HLINT_MIN_HLINE
	if (line < HLINT_MIN_HLINE)
		firq->firq_Num = line = HLINT_MIN_HLINE;
#endif	/* HLINT_MIN_HLINE */

#ifdef	HLINT_MIN_HLINE
	if (line > HLINT_MAX_HLINE)
		firq->firq_Num = line = HLINT_MAX_HLINE;
#endif	/* HLINT_MIN_HLINE */

	DBUGHLINIT (("RegisterHLInt(%d): line=%d pri=%d\n", __LINE__, line, firq->firq.n_Priority));

	sent = (FirqNode *) hlint_nextup;
	DBUGHLINIT (("RegisterHLInt(%d): sent=0x%lx\n", __LINE__, sent));
	if (sent == (FirqNode *) 0)
	{
		NEXTNODE (firq) = PREVNODE (firq) = (Node *) firq;

		oldints = Disable ();
		hlint_nextup = firq;
#ifdef	TEST_HLINTS
		hlint_req_line = line;
#endif	/* TEST_HLINTS */
		((Clio *) CLIO)->VInt1 = line;
		sent = firq;
		Enable (oldints);
		DBUGHLINIT (("RegisterHLInt(%d): registered the first HLINT at 0x%lx (%s)\n", __LINE__, firq, firq->firq.n_Name));
	}
	else
	{
		prev = sent;
		next = (FirqNode *) NEXTNODE (sent);
		DBUGHLINIT (("RegisterHLInt(%d): next=0x%lx\n", __LINE__, next));
		if (next != sent)
		{
			uint32                  sorder = HLINT_FIRQ_ORDER (prev);
			uint32                  order = HLINT_FIRQ_ORDER (firq);
			uint32                  dist = order - sorder;

			DBUGHLINIT (("RegisterHLInt(%d): sorder=0x%06lx order=0x%06lx dist=%d\n", __LINE__, sorder, order, dist));
			while (1)
			{
				uint32                  norder = HLINT_FIRQ_ORDER (next);
				uint32                  ndist = norder - sorder;

				DBUGHLINIT (("RegisterHLInt(%d): norder=0x%06lx ndist=0x%06lx\n", __LINE__, norder, ndist));
				if (ndist > dist)
					break;
				prev = next;
				next = (FirqNode *) NEXTNODE (next);
				DBUGHLINIT (("RegisterHLInt(%d): next=0x%lx\n", __LINE__, next));
				if (next == sent)
					break;
			}
		}
		DBUGHLINIT (("RegisterHLInt(%d): linking 0x%lx<=>0x%lx<=>0x%lx\n", __LINE__, prev, firq, next));
		oldints = Disable ();
		NEXTNODE (firq) = (Node *) next;
		NEXTNODE (prev) = (Node *) firq;
		PREVNODE (firq) = (Node *) prev;
		PREVNODE (next) = (Node *) firq;

		if ((next == hlint_nextup) &&
		    (firq->firq_Num == next->firq_Num) &&
		    (firq->firq.n_Priority > next->firq.n_Priority))
		{
			hlint_nextup = firq;
			Enable (oldints);
			DBUGHLINIT (("RegisterHLInt(%d): pulled sentinal to 0x%lx\n", __LINE__, firq));
		}
		else
		{
			Enable (oldints);
		}

		DBUGHLINIT (("RegisterHLInt(%d): registered another HLINT at 0x%lx (%s)\n", __LINE__, firq, firq->firq.n_Name));
	}

	DBUGHL (("HLInt: call 0x%lx on line %d at pri %d for '%s'\n",
		 firq->firq_F, firq->firq_Num, firq->firq.n_Priority, firq->firq.n_Name));

#ifdef	TEST_HLINTS
	DBUGHLINIT (("RegisterHLInt(%d): HLInt chain now looks like:\n", __LINE__));
	firq = sent;
	do
	{
		DBUGHLINIT (("    0x%06lx: call 0x%06lx at line %d (pri %d) for %s; next=0x%lx\n",
			     firq, firq->firq_F,
			     firq->firq_Num,
			     firq->firq.n_Priority,
			     firq->firq.n_Name,
			     NEXTNODE (firq)));
	} while ((firq = (FirqNode *) NEXTNODE (firq)) != sent);
#endif	/* TEST_HLINTS */
}

/**
|||	AUTODOC PRIVATE spg/kernel/createhlint
|||	CreateHLInt - request firq service on a specific line
|||
|||	  Synopsis
|||
|||         Item CreateHLInt( FirqNode *firq, TagArg *tags )
|||
|||	  Description
|||
|||         Similar to AllocHLInt() but used with a firq node has been
|||         allocated but initialized, and a TagArgs array contains
|||         the definition of the interrupt.
|||
|||	  Arguments
|||
|||         firq                        A pointer to the preallocated
|||	                                FIRQ node which will be used
|||	                                to track this interrupt handler.
|||
|||         tags                        The TagArg array that contains
|||	                                configuration information and
|||	                                parameters for the
|||	                                initialization of the FIRQ
|||	                                node.
|||
|||	  Implementation
|||	  Associated Files
|||	  Notes
|||	  Caveats
|||	  See Also
**/

Item
internalCreateHLInt (FirqNode *firq, TagArg *tagpt)
{
	Task                   *t = KernelBase->kb_CurrentTask;
	Item                    ret;

	if (((uint32)firq) & 0x3)
	{
		DBUGHL(("internalCreateHLInt: firq ptr is 0x%08X\n", firq));
		return BADPTR;
	}

	if (t && ((t->t.n_Flags & TASK_SUPER) == 0))
		return BADPRIV;

	firq->firq_Num = -1;
	ret = TagProcessor (firq, tagpt, icf_c, 0);

	if (firq->firq.n_Name == 0)
		return BADTAGVAL;

	if (firq->firq_F == 0)
	{
		ret = BADTAGVAL;
		goto done;
	}
	internalRegisterHLInt (firq);

	ret = firq->firq.n_Item;

done:
	return ret;
}

int32
internalDeleteHLInt (FirqNode *f, Task *t)
{
	Clio                   *clio;
	uint32                  oldints;

	oldints = Disable ();
	clio = (Clio *) CLIO;
	if (f == hlint_nextup)
	{						/* removing next HLINT to be run */
		hlint_nextup = (FirqNode *) NEXTNODE (f);
		if (f == hlint_nextup)
		{					/* removing only HLINT */
			clio->VInt1 = ~0;		/* turn off comparator */
			clio->ClioInt0.ClrIntEnBits = 2;/* disable V1 ints */
			clio->ClioInt0.ClrIntBits = 2;	/* ack pending V1 ints */
			hlint_nextup = (volatile FirqNodeP) 0;
		}
		else
		{
			int	line;
			line = (int) hlint_nextup->firq_Num;
#ifdef	TEST_HLINTS
			hlint_req_line = line;		/* note requested line */
#endif							/* TEST_HLINTS */
			clio->VInt1 = line;
			REMOVENODE ((Node *) f);
		}
	}
	else
	{						/* removing HLINT from elsewhere */
		REMOVENODE ((Node *) f);
	}
	Enable (oldints);
	return 0;
}

Item
internalCreateHLIntVA (FirqNode *firq, uint32 tags,...)
{
	if (((uint32)firq) & 0x3)
	{
		DBUGHL(("internalCreateHLInt:VA firq ptr is 0x%08X\n", firq));
		return BADPTR;
	}

	return internalCreateHLInt (firq, (TagArg *) &tags);
}

/**
|||	AUTODOC PRIVATE spg/kernel/allochlint
|||	AllocHLInt - request firq service on a specific line
|||
|||	  Synopsis
|||
|||	    Item AllocHLInt( char *name, int pri, int line, void (*code)() )
|||
|||	  Description
|||
|||         This function registers a FIRQ handler that is to be
|||         triggered by the horizontal line number comparator at the
|||         specified line.
|||
|||	  Arguments
|||
|||	    name                        the null teriminated string to
|||	                                be used as the name of the node.
|||
|||	    pri                         a priority value for the
|||	                                interrupt; if two handlers are
|||	                                registered to the same line,
|||	                                the higher priority one is
|||	                                called first.
|||
|||	    line                        the scan line on which the
|||	                                routine is to be called.
|||
|||	    code                        a pointer to the function to
|||	                                be called when the specified
|||	                                scan line is reached.
|||
|||	  Implementation
|||	  Associated Files
|||	  Notes
|||	  Caveats
|||
|||         Interrupt latencies in the system may result in the
|||         routine being called later than expected. If your code has
|||         a hard completion-time, you must read the horizontal line
|||         counter yourself.
|||
|||	  See Also
|||
|||         CreateHLInt(), RegisterHLInt()
**/

Item
internalAllocHLInt (char *name, int pri, int line, void (*code) ())
{
	FirqNode               *firq;
	Item                    firqI;

	firq = (FirqNode *) AllocateNode ((Folio *) KernelBase, FIRQNODE);
	if (firq == (FirqNode *) 0)
		return NOMEM;

	if (((uint32)firq) & 0x3)
	{
		DBUGHL(("internalAllocHLINT: firq ptr is 0x%08X\n", firq));
		return BADPTR;
	}

	firqI = internalCreateHLIntVA (firq,
				       TAG_ITEM_NAME, name,
				       TAG_ITEM_PRI, pri,
				       CREATEFIRQ_TAG_NUM, line,
				       CREATEFIRQ_TAG_CODE, code,
				       TAG_END);
	if (firqI < 0)
		FreeNode ((Folio *) KernelBase, firq);

	return firqI;
}

/* XXX- TODO: hoist loop invarients */
/* XXX- TODO: delete repeated assignments */
/* XXX- TODO: eliminate trivial temps */
/* XXX- TODO: general optimization and tuning */
/* XXX- TODO: downcode to assembly? */
void
runHLInts (void)
{
	Clio                   *clio;			/* ptr to CLIO hardware */
	uint32                  beam;			/* raw CLIO beam-position data */
	FirqNode               *sent;			/* sentinal node for runs with same order */
	FirqNode               *here;			/* hlint firqnode being examined */
	FirqNode               *next;			/* cache point for next node to examine */
	int32                   (*func) (FirqNode *p);	/* cache point for firq's function to call */
	uint32                  line;			/* cache point for next requested line */
	uint32                  sord;			/* order of the sentinal node */
	uint32                  oord;			/* order of just-executed firqnode */
	uint32                  nord;			/* order of next-to-execute firqnode */
	uint32                  bord;			/* order of beam position */
	uint32                  ndist;			/* order distance from just-run firq to next-to-run firq. */
	uint32                  bdist;			/* order distance from just-run firq to beam position */

#ifdef	TEST_HLINTS
	hlint_entries++;				/* make hlint run call visible in memory */
#endif	/* TEST_HLINTS */

	here = hlint_nextup;
	if ((sent = here) == (FirqNode *) 0)		/* if nothing to do, bail out. */
	{
		return;
	}

	sent = here;					/* next node is initial sentinal */
	sord = HLINT_FIRQ_ORDER (sent);			/* track sentinal order */

	clio = (Clio *) CLIO;

	do
	{
		clio->VInt1 = ~0;			/* disable the comparator */
		clio->ClioInt0.ClrIntBits = 2;		/* ack the int */

		do
		{
			next = (FirqNode *) NEXTNODE (here);
			oord = HLINT_FIRQ_ORDER (here);
			if (oord != sord)
			{				/* when the order changes, */
				sent = here;		/* update the sentinel information. */
				sord = oord;
			}
			func = here->firq_F;
			(void) (*func) (0);		/* run the service */
			here = next;
			nord = HLINT_FIRQ_ORDER (here);

			/*
			 * if we are back at the sentinel, all the
			 * nodes on the list were at exactly the same
			 * order. prevent the infinite loop.
			 */

			if (here == sent)
				break;

			/*
			 * compare the orders of the node just
			 * serviced, the next node to service, and the
			 * beam. if the beam is "farther" from the
			 * node just serviced than the next node to
			 * service, then service the next node.
			 *
			 * NOTE: usage of unsigned here is important;
			 * if the beam or the service list has wrapped
			 * around, we want the comparisons to do the
			 * right thing.
			 */
			beam = *VCNT;			/* loop if "here" overdue */
			bord = HLINT_BEAM_ORDER (beam);
			ndist = nord - oord;
			bdist = bord - oord;
		} while (ndist <= bdist);

		line = here->firq_Num;
		hlint_nextup = here;			/* note next to run */
#ifdef	TEST_HLINTS
		hlint_req_line = line;			/* note requested line */
#endif	/* TEST_HLINTS */
		clio->VInt1 = line;			/* turn on the comparator */

		if (here == sent)
			break;

		/*
		 * It is possible that between the time we looked at
		 * the beam position and when we set the comparator
		 * for the next interrupt, the beam counter
		 * incremented into (or past!) the line we wanted to
		 * be interrupted at. This should be rare, but would
		 * cause us to skip an entire display field if it
		 * happened. Repeating the check that we did above
		 * closes this window; if we loop back this time, we
		 * must remember to turn of the comparator and ack the
		 * interrupt just in case it happened.
		 */
		beam = *VCNT;				/* loop if "here" overdue */
		bord = HLINT_BEAM_ORDER (beam);
		ndist = nord - oord;
		bdist = bord - oord;
	} while (ndist <= bdist);
}
#endif	/* SUPPORT_HLINTS */


#ifdef	TEST_HLINTS
/************************************************************************
 *	hlint test code
 */

void
HLIntMarkWall (vuint32 *marks)
{
	marks[*VCNT & VCNT_MASK] ++;
}

void
HLIntMarkWall05 (FirqNode *firq)
{
	HLIntMarkWall (hlint_wall05);
}

void
HLIntMarkWall06 (FirqNode *firq)
{
	HLIntMarkWall (hlint_wall06);
}

void
HLIntMarkWall40 (FirqNode *firq)
{
	HLIntMarkWall (hlint_wall40);
}

void
HLIntMarkWallF0 (FirqNode *firq)
{
	HLIntMarkWall (hlint_wallF0);
}

void
setHLIntTest (void)
{
	if (hlint_tests)
	{
		int	size = 4 * (VCNT_MASK + 1);
		if (hlint_tests & 1)
		{
			hlint_wall05 = (uint32 *) AllocMem(size, MEMTYPE_FILL);
			internalAllocHLInt ("line 0x05 chalk", 100, 0x05, HLIntMarkWall05);
		}
		if (hlint_tests & 2)
		{
			hlint_wall06 = (uint32 *) AllocMem(size, MEMTYPE_FILL);
			internalAllocHLInt ("line 0x06 chalk", 100, 0x06, HLIntMarkWall06);
		}
		if (hlint_tests & 4)
		{
			hlint_wall40 = (uint32 *) AllocMem(size, MEMTYPE_FILL);
			internalAllocHLInt ("line 0x40 chalk", 100, 0x40, HLIntMarkWall40);
		}
		if (hlint_tests & 8)
		{
			hlint_wallF0 = (uint32 *) AllocMem(size, MEMTYPE_FILL);
			internalAllocHLInt ("line 0xF0 chalk", 100, 0xF0, HLIntMarkWallF0);
		}
	}
}
#endif	/* TEST_HLINTS */
