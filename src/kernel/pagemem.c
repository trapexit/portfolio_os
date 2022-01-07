
/* $Id: pagemem.c,v 1.62.1.5 1995/01/13 20:14:11 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "list.h"
#include "task.h"
#include "folio.h"
#include "kernel.h"
#include "mem.h"
#include "internalf.h"
#include "kernelnodes.h"
#include "operror.h"
#include "stdio.h"

#ifdef MASTERDEBUG
#define DBUGAMB(x)	if (CheckDebug(KernelBase,13)) printf x
#define DBUGCM(x)	if (CheckDebug(KernelBase,20)) printf x
#else
#define DBUGAMB(x)	/* printf x */
#define DBUGCM(x)	/* printf x */
#endif

#define DBUG(x)		/*printf x*/
#define DBUGFUM(x)	/*printf x*/
#define DBUGSHARE(x)	/*printf x*/
#define DBUGC(x)	/*printf x*/
#define DBUGMH(x)	/*printf x*/
#define DBUGICM(x)	/*printf x*/

extern void AbortIOReqs(Task *,uint8 *, int32);

extern void Panic(int halt,char *);

#define MAXBITS	128
extern void checkrtn(void);
#define CHECKRTN

uint32
isBitSet(bits,n)
uint32 *bits;
int32 n;
{
#ifdef PD_IsSet
	pd_set *pbits = (pd_set *)bits;
	return PD_IsSet(n,pbits);
#else
	bits += n>>5;	/* 32 bits per int32 word */
	n &= 31;
	if (*bits & (uint32)(1<<(31-n)))	return 1;
	return 0;
#endif
}

void
SetBit(bits,n)
uint32 *bits;
int32 n;
{
#ifdef PD_IsSet
	pd_set *pbits = (pd_set *)bits;
	PD_Set(n,pbits);
#else
	bits += n>>5;
	n &= 31;
	*bits |= (uint32)1<<(31-n);
#endif
}

void
ClearBit(bits,n)
uint32 *bits;
int32 n;
{
#ifdef PD_IsSet
	pd_set *pbits = (pd_set *)bits;
	PD_Clr(n,pbits);
#else
	bits += n>>5;
	n &= 31;
	*bits &= (uint32) ~(1<<(31-n));
#endif
}

extern MemList *MatchMemHdr(List *,MemHdr *);

int32
AllocPages (mh, who, n)
	MemHdr                 *mh;
	Task                   *who;
	int32                   n;
{
	/* Allocate n contiguous pages */
	MemList                *ml;
	uint32                 *bits;
	int32                   bit;
	List                   *l;
	uint32                  startbit;
	uint32                  endbit;

	bits = mh->memh_FreePageBits;
	startbit = 0;
	endbit = (mh->memh_FreePageBitsSize * (int32) NPDBITS);

	if (who)
		l = who->t_FreeMemoryLists;
	else
		l = KernelBase->kb_MemFreeLists;

	ml = MatchMemHdr (l, mh);

	if (ml == 0)
	{
#ifdef DEVELOPMENT
		printf ("error in AllocPages, no memlist\n");
		while (1);
#else
		if (isUser () == 0)
			Panic (1, "AllocPages no memlist\n");

#endif
	}

	/*
	 * sanity check. NB: this must weed out situations where "n" is
	 * larger than endbit, or the loops below will do really bad
	 * things.
	 */

	if (n > (endbit - startbit))
	{
		return 0;
	}

	endbit -= n - 1;

	if (who)
	{		/* Task allocation */
		for (bit = startbit; bit < endbit; bit++)
		{
			if (isBitSet (bits, bit))
			{
				int32                   ok = 1;
				int32                   bit2;

				/* Can we get the next n pages? */
				for (bit2 = 0; bit2 < n; bit2++)
				{
					if (isBitSet (bits, bit + bit2) == 0)
					{
						ok = 0;
						break;
					}
				}
				if (ok)
				{
					for (bit2 = 0; bit2 < n; bit2++)
					{
						ClearBit (bits, bit + bit2);
						SetBit (ml->meml_WriteBits, bit + bit2);
						SetBit (ml->meml_OwnBits, bit + bit2);
					}

					UpdateFence (who);

					return bit + ((int32) (mh->memh_MemBase) >> mh->memh_PageShift);
				}
			}
		}
	}
	else
	{		/* supervisor mode allocation */
		bit = endbit;
		while (bit-- > startbit)
		{
			if (isBitSet (bits, bit))
			{
				int32                   ok = 1;
				int32                   bit2;

				/* Can we get the next n pages? */
				for (bit2 = n - 1; bit2 >= 0; bit2--)
				{
					if (isBitSet (bits, bit + bit2) == 0)
					{
						ok = 0;
						break;
					}
				}
				if (ok)
				{
					for (bit2 = n - 1; bit2 >= 0; bit2--)
					{
						ClearBit (bits, bit + bit2);
						SetBit (ml->meml_OwnBits, bit + bit2);
					}
					return bit + ((int32) (mh->memh_MemBase) >> mh->memh_PageShift);
				}
			}
		}
	}
	return 0;
}

extern bool isUser(void);

extern volatile int32 SaveKB;
extern void CheckR11(void);

/**
|||	AUTODOC PUBLIC spg/kernel/allocmemblocks
|||	AllocMemBlocks - Transfer pages of memory from the system-wide free memory pool.
|||
|||	  Synopsis
|||
|||	    void *AllocMemBlocks( int32 size, uint32 typebits )
|||
|||	  Description
|||
|||	    When there is insufficient memory in a task's free memory pool to
|||	    allocate a block of memory, the Kernel automatically provides
|||	    additional pages memory from the system-wide free memory pool.
|||	    Tasks can also get pages from the system-wide free memory pool by
|||	    calling AllocMemBlocks().
|||
|||	    Note: Normal applications do not need to call this procedure.  It
|||	    should only be used by applications that need additional control
|||	    over the memory allocation process.
|||
|||	    You must set MEMTYPE_TASKMEM otherwise the memory will not be
|||	    allocated to the current task.
|||
|||	    AllocMemBlocks() is different from other memory-allocation
|||	    procedures:
|||
|||	    *   The pages of memory that are transferred are not automatically
|||	        added to the task's free memory pool.  To move the memory into
|||	        its free memory pool, thereby making it available to tasks,
|||	        the task must call one of the procedures for freeing memory
|||	        (FreeMem(), FreeMemToMemList(), or FreeMemToMemLists()) with
|||	        the pointer returned by AllocMemBlocks() as the argument.
|||	        (Note that in the memory returned by AllocMemBlocks(), the
|||	        first four bytes specify the amount of memory, in bytes, that
|||	        was transferred.  You should use this value as the size to be
|||	        freed.)
|||
|||	  Arguments
|||
|||	    size                        The amount of memory to transfer, in
|||	                                bytes.  If the size is not an integer
|||	                                multiple of the page size for the type
|||	                                of memory requested, the system
|||	                                transfers the number of full pages
|||	                                needed to satisfy the request.
|||
|||	    typebits                    Flags that specify the type of memory
|||	                                to transfer.  These flags can include
|||	                                MEMTYPE_ANY, MEMTYPE_VRAM,
|||	                                MEMTYPE_DRAM, MEMTYPE_BANKSELECT,
|||	                                MEMTYPE_BANK1, MEMTYPE_BANK2,
|||	                                MEMTYPE_DMA, MEMTYPE_CEL,
|||	                                MEMTYPE_AUDIO, MEMTYPE_DSP,
|||	                                MEMTYPE_TASKMEM.
|||	                                For information about these flags, see
|||	                                the description of AllocMem().
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to the pages of memory that were
|||	    transferred or NULL if the memory couldn't be transferred.  The
|||	    first four bytes of the memory specify the amount of memory that
|||	    was transferred, in bytes.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    To return memory to the system-wide free memory pool, use
|||	    ScavengeMem() or ControlMem().  ScavengeMem() finds pages of
|||	    memory in the task's free memory pool from which no memory has
|||	    been allocated and returns those pages to the system-wide memory
|||	    pool.  You can use ControlMem() to transfer ownership of memory to
|||	    the system-wide memory pool.
|||
|||	  See Also
|||
|||	    ControlMem(), FreeMem(), FreeMemToMemList(), FreeMemToMemLists(),
|||	    ScavengeMem()
|||
**/

void                   *
internalAllocMemBlocks (size, flags)
	int32                   size;
	int32                   flags;
{
	List                   *l;
	int32                  *p;
	MemHdr                 *mh;
	Task                   *t = 0;
	int32                   newsize;
	int32                   types = flags & (MEMTYPE_VRAM_BANK1 | MEMTYPE_VRAM_BANK2 | MEMTYPE_DMA | MEMTYPE_CEL | MEMTYPE_DRAM | MEMTYPE_AUDIO | MEMTYPE_DSP);

	/*
	 * Loop through all headers, to find the ones that can provide
	 * this memory type.
	 */

	l = KernelBase->kb_MemHdrList;

	for (mh = (MemHdr *) FIRSTNODE (l);
	     ISNODE (l, (Node *) mh);
	     mh = (MemHdr *) NEXTNODE ((Node *) mh))
	{
		/*
		 * If the caller requested something that this list
		 * does not provide, ignore it.
		 */
		if ((mh->memh_Types & types) != types)
			continue;

		/*
		 * If the caller wants memory for the current task,
		 * make sure the task has a memory list that uses this
		 * header.
		 */

		if (flags & MEMTYPE_TASKMEM)
		{
			MemList                *ml;

			t = KernelBase->kb_CurrentTask;
			for (ml = (MemList *) FIRSTNODE (t->t_FreeMemoryLists);
			     ISNODE (t->t_FreeMemoryLists, ml);
			     ml = (MemList *) (NEXTNODE ((Node *) ml)))
			{
				if (ml->meml_MemHdr == mh)
					break;
			}
			if (ISNODE (t->t_FreeMemoryLists, ml) == 0)
				continue;
		}

		/*
		 * calculate number of pages needed to contain the
		 * requested allocation
		 */

		newsize = size + (int32) mh->memh_PageMask;
		newsize >>= mh->memh_PageShift;
		p = (int32 *) (AllocPages (mh, t, newsize) * mh->memh_PageSize);
		if (p)
		{
			*p = (int32) newsize   *mh->memh_PageSize;	/* Make it memfreeable */
			return (void *) p;
		}
	}
	return 0;
}

MemHdr *
internalFindMHrange(void *p,int size)
{
	Node *n;
	void *q;
	DBUGMH(("FindMH(%lx)\n",p));
	DBUGMH(("MemHdrList=%lx\n",KernelBase->kb_MemHdrList));
	DBUGMH(("Anchor=%lx\n",&((KernelBase->kb_MemHdrList)->ListAnchor.head.links.blink)));

	/* compute byte after last */
	q = (void *)( (uint32)p + size);

	for (n = FIRSTNODE(KernelBase->kb_MemHdrList);
			 ISNODE(KernelBase->kb_MemHdrList,n);
			 n = NEXTNODE(n))
	{
		MemHdr *mh = (MemHdr *)n;
		DBUGMH(("mh=%lx\n, Base=0x%x, Top=0x%x",(uint32)mh, mh->memh_MemBase, mh->memh_MemTop));
		if (mh == 0) {
#ifdef DEVELOPMENT
			while (1);
#else
			if(isUser() == 0)Panic(1,"mh=0\n");
#endif
		}
		if (p < (void *)(mh->memh_MemBase))
			continue;	/* p aint in range [MemBase,2^32> */
		if ((q < p) || (q > (void *)(mh->memh_MemTop)))
			continue;	/* p+size is not in range [p,MemTop] */
		return mh;
	}
	return 0;
}

MemHdr *
internalFindMH(void *p)
{
	return internalFindMHrange(p,1);
}

void *
FindMH(void *p)
{
	return (void *)internalFindMH(p);
}

void
UnshareAllMem(mh,ShareBits)
MemHdr *mh;
uint32 *ShareBits;
{
	Node *n;
	List *l = KernelBase->kb_Tasks;

	for (n = FIRSTNODE(l); ISNODE(l,n); n = NEXTNODE(n))
	{
		Task	*t = Task_Addr(n);
		MemList	*ml = MatchMemHdr(t->t_FreeMemoryLists,mh);
		uint32	*wbits, *sbits;
		int32	i;

		if (!ml) continue;

		wbits = ml->meml_WriteBits;
		sbits = ShareBits;
		for (i = mh->memh_FreePageBitsSize; i > 0; --i)
			*wbits++ &= ~*sbits++;
	}
	{
		/* Now Abort all IOReqs using the task's memory */
		uint32 base = (uint32)mh->memh_MemBase;
		uint32 pageshift = mh->memh_PageShift;
		uint32 endbit = (mh->memh_FreePageBitsSize * (int32) NPDBITS);
		int32  bit;

		for (bit = 0; bit < endbit; bit++)
		{
			uint32 startbit;

			if (!isBitSet(ShareBits, bit)) continue;

			startbit = bit;
			while ((++bit < endbit) && isBitSet(ShareBits, bit));
			AbortIOReqs(NULL, (uint8 *)(base+(startbit<<pageshift)),
					  ((bit-startbit)<<pageshift));
		}
	}
}

void
FreeUserMem(t)
Task *t;
{
	/* All active IOReqs for this task have been killed */
	int32 n;
	int32 pagesize;
	Node *nd;
	List *l;
	DBUGFUM(("FreeUserMem(%lx) ",(uint32)t));
	l = t->t_FreeMemoryLists;
	if (!l) return;
	while ((nd = RemHead(l)) != (Node *)0) {
		MemList *ml = (MemList *)nd;
		MemHdr *mh = ml->meml_MemHdr;
		uint32 *tbits = ml->meml_OwnBits;
		uint32 *sbits = mh->memh_FreePageBits;
		pagesize = mh->memh_PageSize;
		DBUGFUM(("< %lx %lx %lx %lx \n",tbits[0],tbits[1],tbits[2],tbits[3]));
		DBUGFUM(("> %lx %lx %lx %lx \n",sbits[0],sbits[1],sbits[2],sbits[3]));
		if (tbits)
		{
			for (n = 0; n < mh->memh_FreePageBitsSize ; n++)
			{
				uint32 q = *tbits++;
				*sbits++ |= q;
			}
			UnshareAllMem(mh,ml->meml_OwnBits);
			FREEMEM(ml->meml_OwnBits, \
				ml->meml_OwnBitsSize*(int32)sizeof(pd_mask));
		}
		if (ml->meml_WriteBits)
		    FREEMEM(ml->meml_WriteBits, \
			ml->meml_OwnBitsSize*(int32)sizeof(pd_mask));
		DBUGFUM(("Calling FreeNode ml=%lx\n",(uint32)ml));
		/* This should not be needed, but just in case */
		if (ml->meml_Sema4) internalDeleteItem(ml->meml_Sema4,t);
		FreeNode((Folio *)KernelBase,ml);
	}
	l->l.n_Name = NULL; /* prevent FreeNode() from calling FreeMem() on this pointer */
	FreeNode((Folio *)KernelBase,l);
	DBUGFUM(("Return from FreeUserMem\n"));
}

int32
ValidateMem(Task *t, uint8 *p, int32 size)
{
	/* can Task t write to memory from p -> p[size-1] ? */
	MemHdr *mh;
	MemList *ml;
	int32 bit;
	int32 pagesize;
	uint32 *pbits /*= t->t_MemProtectBits*/;	/* protect bits */

	DBUG(("ValidateMem(%lx,%lx,%d)\n",t,p,size));

	if (size <= 0)
	{
#ifdef DEVELOPMENT
		DBUG(("ValidateMem: size <= 0\n"));
#endif
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadPtr);
	}

        /* get the memlist that would contain the target address */
	ml = FindMemList(t->t_FreeMemoryLists,p);
	if (!ml)
	{
#ifdef DEVELOPMENT
	    DBUG(("Address $%x is not valid\n"));
#endif
	    return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadPtr);
	}

        /* get the associated memhdr */
	mh = ml->meml_MemHdr;

	pagesize = mh->memh_PageSize;
	pbits = ml->meml_WriteBits;

	size += (int32)p & mh->memh_PageMask;
	p -= ((int32)p & mh->memh_PageMask);

	DBUG(("mh=%lx pbits=%lx\n",mh,pbits));

	while (size > 0)
	{
		bit = ((int32)p - (int32)mh->memh_MemBase) >> mh->memh_PageShift;
		DBUG(("checking bit#%d\n",bit));
		if ( isBitSet(pbits,bit) == 0 )
		{
			DBUG(("pbits = %lx, bit#%d failed\n",(uint32)pbits,bit));
			return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadPtr);
		}
		p += pagesize;
		size -= pagesize;
	}
	return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/ismemwritable
|||	IsMemWritable - Decide whether a region of memory is fully writable
|||	                by the current task.
|||
|||	  Synopsis
|||
|||	    bool IsMemWritable( const void *p, int32 size )
|||
|||	  Description
|||
|||	    This function considers the described block of the address
|||	    space in relation to the pages of memory the current task
|||	    has write access to. This function returns TRUE If the current
|||	    task can write to the memory block, and FALSE if it cannot.
|||
|||	  Arguments
|||
|||	    p                           A pointer to the start of the block
|||
|||	    size                        The number of bytes in the block
|||
|||	  Return Value
|||
|||	    This function returns TRUE if the block can be written to by
|||	    the calling task, and FALSE if it cannot.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    mem.h                 ANSI C Prototype for IsRamAddr
|||
|||	    clib.lib              ARM Link Library
|||
|||	  See Also
|||
|||	    IsMemReadable()
**/

bool IsMemWritable(const void *p, int32 size)
{
    if (ValidateMem(CURRENTTASK,(uint8 *)p,size) == 0)
        return TRUE;

    return FALSE;
}

int32
RemoveWriteAccessTask(Task *t, uint8 *p, int32 npages, MemHdr *mh)
{
	MemList *ml;
	int32	bit;
	uint32	*Writebits;
	uint32  ret = 0;

	ml = MatchMemHdr(t->t_FreeMemoryLists,mh);

	if (!ml) return 0;

	Writebits = ml->meml_WriteBits;

	/* Compute first bit position */
	bit = ((int32)p - (int32)mh->memh_MemBase) >> mh->memh_PageShift;

	while (npages--)
	{
		ret |= isBitSet(Writebits,bit);
		ClearBit(Writebits,bit);
		bit++;
	}
	return ret;
}

int32
RemoveWriteAccessAll(uint8 *p, int32 npages, MemHdr *mh)
{
	ItemNode *n;
	List     *l = KernelBase->kb_Tasks;
	int32    ret = 0;

	/* Remove write access for all tasks */
	for (n=(ItemNode *)FIRSTNODE(l); ISNODE(l,n); n=(ItemNode *)NEXTNODE(n))
	{
		Task *t = Task_Addr(n);
		/* Remove write access to memory <p,size> for task t */
		if (!(t->t_ThreadTask))
			ret |= RemoveWriteAccessTask(t, p, npages, mh);
	}
	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/controlmem
|||	ControlMem - Control memory permissions and ownership
|||
|||	  Synopsis
|||
|||	    Err ControlMem( void *p, int32 size, int32 cmd, Item task )
|||
|||	  Description
|||
|||	    When a task allocates memory, it becomes the owner of that memory.
|||	    Other tasks cannot write to the memory unless they are given
|||	    permission by its owner.  A task can give another task permission
|||	    to write to one or more of its memory pages, revoke write
|||	    permission that was previously granted, or transfer ownership of
|||	    memory to another task or the system by calling ControlMem().
|||
|||	    Each page of memory has a control status that specifies which task
|||	    owns the memory and which tasks can write to it.  Calls to
|||	    ControlMem() change the control status for entire pages.  If the p
|||	    and size arguments (which specify the memory to change) specify
|||	    any part of a page, the changes apply to the entire page.
|||
|||	    A task can grant write permission for a page that it owns (or for
|||	    some number of contiguous pages) to any number of tasks.  To
|||	    accomplish this, the task must make a separate call to
|||	    ControlMem() for each task that is to be granted write permission.
|||
|||	    A task that calls ControlMem() must own the memory whose control
|||	    status it is changing, with one exception: A task that has write
|||	    access to memory it doesn't own can relinquish its write access
|||	    (by using MEMC_NOWRITE as the value of the cmd argument).  If a
|||	    task transfers ownership of memory it still retains write access.
|||
|||
|||	  Arguments
|||
|||	    p                           A pointer to the memory whose control
|||	                                status to change.
|||
|||	    size                        The amount of memory for which to
|||	                                change the control status, in bytes.
|||	                                If the size and p arguments specify
|||	                                any part of a page, the control status
|||	                                is changed for the entire page.
|||
|||	    cmd                         A constant that specifies the change
|||	                                to be made to the control status.;
|||	                                possible values are listed below.
|||
|||	    task                        The item-number task for which to
|||	                                change the control status or zero
|||	                                for global changes. See Notes.
|||
|||	    The possible values of "cmd" are:
|||
|||	    MEMC_OKWRITE                Grants permission to write to this
|||	                                memory to the task specified by the
|||	                                task argument.
|||
|||	    MEMC_NOWRITE                Revokes permission to write to this
|||	                                memory from the task specified by the
|||	                                task argument. If task is 0 this
|||	                                revokes write permission for all tasks.
|||
|||	    MEMC_GIVE                   If the calling task is the owner of
|||	                                the memory, this transfers ownership
|||	                                of the memory to the task specified by
|||	                                the task argument. If the specified
|||	                                task is 0, it gives the memory back to
|||	                                the system free-memory pool.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the change was successful or an error
|||	    code (a negative value) if an error occur.  Possible error codes
|||	    include:
|||
|||	    BADITEM                     The task argument does not specify a
|||	                                current task.
|||
|||	    ER_Kr_BadMemCm              The cmd argument is not one of the
|||	                                valid values.
|||
|||	    ER_BadPtr                   The p argument is not a valid pointer
|||	                                to memory.
|||
|||	  Implementation
|||
|||	    SWI implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       ARM C "swi" declaration
|||
|||	  Notes
|||
|||	    A task can use ControlMem() to prevent itself from writing to
|||	    memory that it owns.
|||
|||	    A task must own the memory for its I/O buffers.
|||
|||	    A task can use ControlMem() to return ownership of memory pages to
|||	    the system, thereby returning them to the system-wide free memory
|||	    pool.  You can do this by using NULL as the value of the task
|||	    argument.
|||
|||	    A task can use ControlMem() to unshare or write protect memory
|||	    from all other tasks. Specify 0 for the task for this to happen.
|||
|||	    We would like to support making a piece of memory writable by all
|||	    other tasks by using task==0 and MEMC_OKWRITE. But this is not
|||	    implemented yet.
|||
|||	  See Also
|||
|||	    ScavengeMem(), AllocMemBlocks()
|||
**/

int32
internalControlMem(Task *ct,uint8 *p,int32 size,int32 cmd,Item it)
{
	Task *t;
	MemHdr *mh;
	int32 pagesize;
	uint32 pagemask;
	int32 bit;
	uint32 *Ownbits,*NewOwnbits;
	uint32 *Writebits = (uint32 *)0;
	uint32 *NewWritebits = (uint32 *)0;
	int32 offset;
	uint32 t_changed = 0;
	MemList *ml;
	int vscavage = 0;
	int npages;
	int32 delta;

	t = 0;

	DBUGICM(("ICMem called (ct=0x%x, p=0x%x, sz=0x%x, cmd=0x%x, it=0x%x\n", ct, p, size, cmd, it));
	if (cmd == MEMC_SC_GIVE)
	{
		/* is this the special ScavengeMem case ? */
		cmd = MEMC_GIVE;
		vscavage = 1;
	}

	if (it)	t = (Task *)CheckItem(it,KERNELNODE,TASKNODE);
	if (t == 0)
	{
		if (it != 0)	return BADITEM;
		if (cmd == MEMC_OKWRITE)
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadMemCmd);
		/* special cases (for it == 0): */
		/* MEMC_GIVE,    return memory to system */
		/* MEMC_NOWRITE, remove write access from all other tasks */
		/* MEMC_OKWRITE, give write access to all tasks - yet to do */
	}
	if (size <= 0)	return 0;

	/* Find the proper memhdr */
	mh = internalFindMHrange((void *)p, (int)size);
	if (mh == 0)
	{
#ifdef DEVELOPMENT
		printf("No valid address range for FreePage memory\n");
#endif
		return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadPtr);
	}
	offset = (int32)mh->memh_MemBase;
	pagesize = mh->memh_PageSize;
	pagemask = pagesize - 1;

	/* round buffer start down to beginning of page */
	delta = (int32)p & pagemask;
	/* delta = number of bytes into this page */
	p -= delta;		/* align down to beginning of page */
	size += delta;
	size += pagemask;
	size &= ~pagemask;	/* round to full pages */
	npages = size >> mh->memh_PageShift;	/* how many pages? */

	/* Now get the proper MemList for this Task */
	if (ct)
	{
		ml = MatchMemHdr(ct->t_FreeMemoryLists,mh);
		Ownbits = ml->meml_OwnBits;
		Writebits = ml->meml_WriteBits;
	}
	else if (vscavage)
	{
		ml = MatchMemHdr(KernelBase->kb_MemFreeLists,mh);
		Ownbits = ml->meml_OwnBits;
		/* Writebits = 0; */	/* ml->meml_WriteBits; */
	}
	else
	{
		/* ct == 0 && vscavage == 0 */
		/* Ownbits is only zero if we are at startup and are just */
		/* initially adding memory to the system memory lists */
		Ownbits = 0;
	}


	/* pass 1: Validate all pages, can we do the operation on all pages? */
	if (ct && ((cmd != MEMC_NOWRITE) || (ct != t)))
	{
		int i = npages;

		if (Ownbits)
		{
		    /* Compute first bit position */
		    bit = ((int32)p - offset) >> mh->memh_PageShift;

		    while (i--)
		    {
			/* Do we own this memory? */
			if (isBitSet(Ownbits,bit) == 0)
			    return MAKEKERR(ER_SEVER,ER_C_STND,ER_BadPtr);
			bit++;
		    }
		}
	}

	/* OK for task ct to do this operation */

	/* pass 2: abort all IOReqs if we are losing write permission */
	if (cmd == MEMC_NOWRITE)
	{
		if (t == NULL)
		{
			/* Remove write access to mem <p,size> for all tasks */
			t_changed = (uint32)RemoveWriteAccessAll(p, npages, mh);
		}
		else
		{
			/* Remove write access to memory <p,size> for task t */
			t_changed = (uint32)RemoveWriteAccessTask(t, p, npages, mh);
		}

		AbortIOReqs(t, p, size);

		if (t_changed && t)
			UpdateFence(t);
		return 0;
	}

	if ((t == 0) && (cmd == MEMC_GIVE) && (vscavage == 0) && ct)
	{
		/* giving memory to system */
		/* Remove write access to memory <p,size> for all tasks */
		RemoveWriteAccessAll(p, npages, mh);

		AbortIOReqs(t, p, size);
	}

	/* get ptr to ownbits of target task */
	if (t)
	{
		ml = MatchMemHdr(t->t_FreeMemoryLists,mh);
		NewOwnbits = ml->meml_OwnBits;
		NewWritebits = ml->meml_WriteBits;
		DBUGICM(("NewOwnbits = 0x%x, NewWritebits = 0x%x\n", NewOwnbits, NewWritebits));
	}
	/* if t==0 , then give memory back to system */
	else	NewOwnbits = mh->memh_FreePageBits;

	/* We should be dealing with bit arrays, not single bits */
	/* Maybe borrow some X11 code */

	/* pass 3: do the operation on all pages after successful pass 2 */

	/* Compute first bit position */
	bit = ((int32)p - offset) >> mh->memh_PageShift;
	while (npages--)
	{
		switch (cmd)
		{
		    case MEMC_OKWRITE:
			SetBit(NewWritebits,bit);
			break;
		    case MEMC_GIVE:
			if (Ownbits)	ClearBit(Ownbits,bit);
			SetBit(NewOwnbits,bit);
			if (ct)	ClearBit(Writebits,bit);
			if (t)	SetBit(NewWritebits,bit);
			break;
		    default:
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadMemCmd);
		}
		bit++;
	}

	if (ct && t)
	    UpdateFence(t);

	return 0;
}

int32 internalControlSuperMem(p,size,cmd,it)
Item it;	/* Task */
uint8 *p;	/* start of mem block */
int32 size;	/* size  */
int32 cmd;
{
	return internalControlMem(0,p,size,cmd,it);
}

int32
externalControlMem(p,size,cmd,it)
Item it;	/* Task */
uint8 *p;	/* start of mem block */
int32 size;		/* size  */
int32 cmd;
{
	Task *ct = KernelBase->kb_CurrentTask;
	return internalControlMem(ct,p,size,cmd,it);
}
