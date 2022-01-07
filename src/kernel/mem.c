/* $Id: mem.c,v 1.120 1994/11/18 02:33:32 vertex Exp $ */

/* Memory management routines for Operator */
/* List management routines for Operator */

/* Basic low level functions */
/* Add a piece of memory to the FreeMemList */

/*#define CLISTS*/

#include "types.h"
#include "ctype.h"
#include "aif.h"

#define ONEMEG		(1024*1024)

#include "nodes.h"
#include "kernelnodes.h"
#include "item.h"

#define MINFREE (sizeof(Node))

#include "folio.h"
#include "list.h"
#include "listmacros.h"
#include "task.h"

#include "msgport.h"
#include "semaphore.h"
#include "interrupts.h"
#include "mem.h"
#include "strings.h"

#include "kernel.h"
#include "io.h"
#include "inthard.h"
#include "stdio.h"

#include "internalf.h"
#include "debug.h"

#ifdef	DEVELOPMENT
#define	INFO(x)		printf x
#else
#define	INFO(x)		/* printf x */
#endif

#define	FIDBUG(x)	/* if (!isUser()) prinf x; else kprintf x */

extern void             TailInsertNode (List *l, Node *n);
extern void             InitVMEM (int32 memsize);

extern void             Panic (int halt, char *fmt, ...);

extern AIFHeader        __my_AIFHeader;

/**
|||	AUTODOC PUBLIC spg/kernel/findnamednode
|||	FindNamedNode - Find a node by name.
|||
|||	  Synopsis
|||
|||	    Node *FindNamedNode( const List *l, const char *name )
|||
|||	  Description
|||
|||	    This procedure searches a list for a node with the specified name.
|||	    The search is not case-sensitive (that is, the Kernel does not
|||	    distinguish uppercase and lowercase letters in node names).
|||
|||	  Arguments
|||
|||	    l                           A pointer to the list to search.
|||
|||	    name                        The name of the node to find.
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to the node structure or NULL if
|||	    the named node couldn't be found.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    list.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    FirstNode(), LastNode(), NextNode(), PrevNode()
|||
**/

Node *
FindNamedNode (const List *l, const char *s)
{
	Node                   *n;
	char		       *name;

	for (n = FIRSTNODE (l); ISNODE (l, n); n = NEXTNODE (n))
	{
		name = n->n_Name;
		if (s == name)		/* trivially equal */
			return n;	/* (usually both NULL) */
		if ((s != 0) && (name != 0))
		{
			if (strcasecmp (s, name) == 0)	/* Match */
				return n;
		}
	}
	return 0;
}

/**
|||	AUTODOC PUBLIC spg/kernel/setnodepri
|||	SetNodePri - Change the priority of a list node.
|||
|||	  Synopsis
|||
|||	    uint8 SetNodePri( Node *n, uint8 newpri )
|||
|||	  Description
|||
|||	    This procedure changes the priority of a node in a list.  The
|||	    Kernel arranges lists by priority, with higher-priority nodes
|||	    coming before lower-priority nodes.  When
|||
|||	    the priority of a node changes, the Kernel automatically
|||	    rearranges the list to reflect the new priority.  The node is
|||	    moved immediately before the first node whose priority is lower.
|||
|||	  Arguments
|||
|||	    n                           A pointer to the node whose priority
|||	                                to change.
|||
|||	    newpri                      The new priority for the node.  This
|||	                                can be a value from 0 to 255.
|||
|||	  Return Value
|||
|||	    The procedure returns the previous priority of the node.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    list.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveats
|||
|||	    GIGO (`garbage in, garbage out')
|||
|||	  See Also
|||
|||	    InsertNodeFromHead(), InsertNodeFromTail(), UniversalInsertNode()
|||
**/

uint8
SetNodePri (Node *n, uint8 pri)
{
	List                   *l;
	Node                   *lastnode;
	uint8                  *p;
	uint8                   oldpri = n->n_Priority;

	/* Find the list anchor */
	lastnode = NEXTNODE (n);
	while (lastnode->n_Next)
		lastnode = NEXTNODE (lastnode);
	/* lastnode now points to the tail.links */
	p = (uint8 *) lastnode;
	/* claw our way back */
	p -= sizeof (Link *) + sizeof (Node);
	l = (List *) p;
	REMOVENODE (n);
	n->n_Priority = (uint8) pri;
	TailInsertNode (l, n);
	return oldpri;
}

/* Each subsysgroup will have its own Node Allocator */
/* All fields initialized to zero except for: */
/* subsystype, type, size, flags */
void                   *
AllocateSizedNode (Folio *f, uint8 thistype, int32 size)
{
	Node                   *ret;
	NodeData               *nd = f->f_NodeDB;
	int32                   nsize;
	uint8			flags;
	ItemNode	       *in;

	if ((thistype <= 0) ||
	    (thistype > f->f_MaxNodeType) ||
	    (size < 0))
		return 0;

	nd += thistype;
	nsize = nd->size;
	if (size != 0)
	{				/* alternate size */

		/* only allow alternate size if its big enough and  */
		if (size < nsize)
			return 0;

		/* the size isn't locked down */
		if (nsize && (nd->flags & NODE_SIZELOCKED))
			return 0;

		nsize = size;
	}

	/* nsizes of 0 mean variable length */
	/* so we return -1 to signal that */
	if (nsize == 0)
		return (void *) -1;

	/* last sanity check */
	if (nsize < sizeof (List))
		return 0;

	ret = (Node *) ALLOCMEM (nsize, MEMTYPE_ANY | MEMTYPE_FILL);
	if (ret == (Node *)0)
		return 0;

	flags = nd->flags;

	ret->n_SubsysType = (char) (f->fn.n_Item);
	ret->n_Type = thistype;
	ret->n_Size = nsize;
	ret->n_Flags = (flags & 0xf0);
	if ((flags & NODE_ITEMVALID) == 0)
		return (void *)ret;

	in = (ItemNode *) ret;
	in->n_Item = GetItem (in);
	if ((int32) (in->n_Item) >= 0)
		return (void *)ret;

	FREEMEM (ret, nsize);
	return 0;
}

void                   *
AllocateNode (Folio * f, uchar thistype)
{
	return AllocateSizedNode (f, thistype, 0);
}

void
FreeNode (f, n)
	Folio                  *f;
	void                   *n;
{
	ItemNode               *N;

	if (n == 0)
		return;
	N = (ItemNode *) n;
	if (N->n_Flags & NODE_ITEMVALID)
	{
		FreeItem (N->n_Item);
	}

	if (N->n_Flags & NODE_NAMEVALID)
	    FreeString(N->n_Name);

	FREEMEM (N, N->n_Size);
}

#define	VRAMSetSize	2
uint32                  VRAMpagebits_1[VRAMSetSize];
uint32                  VRAMkernelbits_1[VRAMSetSize];
uint32                  VRAMpagebits_2[VRAMSetSize];
uint32                  VRAMkernelbits_2[VRAMSetSize];

#define	DRAMSetSize	2
uint32                  DRAMpagebits[DRAMSetSize];
uint32                  DRAMkernelbits[DRAMSetSize];

static struct MemHdr    DRAMHdr;
static struct MemList   DRAMFreeMem;
static struct List      DRAMFreeList;

static struct MemHdr    VRAMHdr_1;
static struct MemList   VRAMFreeMem_1;
static struct List      VRAMFreeList_1;

static struct MemHdr    VRAMHdr_2;
static struct MemList   VRAMFreeMem_2;
static struct List      VRAMFreeList_2;

int                     kernel_reserve;

uint32                  dram_size;
uint8                  *vram_start;
uint32                  vram_size;
bool                    two_vram_banks;

uint32		       *RamDiskAddr;
int32			RamDiskSize;

/*
 * FindRamSize(): compute "vram_size" and "dram_size" based on the
 * contents of the Madam System Control Register.
 *
 *             |  x8 == 0   |  x8 == 0  |  x8 == 1
 *  MSYS       | MSYS DRAM  | MSYS DRAM | MSYS
 *  2:0  VRAM  | 6:5  SET0  | 4:3 SET1  | 6:3  DRAM
 *  ---  ----  | ---- ----  | ---- ---- | ---- ----
 *             |  00  0 MB  |  00 0 MB  |
 *  001  1 MB  |  01  1 MB  |  01 1 MB  | 0101 2 MB
 *  010  2 MB  |  10  4 MB  |  10 4 MB  | 1010 8 MB
 *                          |  11 16MB  |
 *
 * Combinations not shown in the above tables are not supported and
 * this routine is not guaranteed to return anything sensible (even
 * though it usually will do something vaguely rational).
 *
 * Note: only 16 MB address space is available for system memory, so
 * the actual amount of DRAM available is reduced so that the sum of
 * the DRAM and VRAM is no larger than 16 MB.
 */

void
FindRamSize (void)
{
	uint32	bits = *MSYSBits;

	/*
	 * This may look complex, but it only takes about eight ARM
	 * instructions to evaluate. Basicly, take each DRAM set size
	 * field, and double it to get the left-shift count. Then
	 * shift "a quarter megabyte" left by the count, casting off
	 * anything less than a megabyte; this is easiest to do if you
	 * shift "1" left by the count, then right by two. We turn it
	 * into megabytes later.
	 */
	dram_size =
		(((1<<((bits >> (DRAMSIZE_SET0SHIFT-1))&(DRAMSIZE_SETMASK<<1)))>>2) +
		 ((1<<((bits >> (DRAMSIZE_SET1SHIFT-1))&(DRAMSIZE_SETMASK<<1)))>>2));

	/*
	 * Only two VRAM sizes are defined. I'd guess that this gets
	 * more complex if we start seeing more VRAM sizes.
	 */
	vram_size = ((bits & VRAMSIZE_MASK) >> VRAMSIZE_SHIFT);

	/*
	 * Opera only provides 16 Megabytes of physical address space
	 * to contain both the VRAM and DRAM, and the VRAM is above
	 * the DRAM. If the calculations above give more than 16M of
	 * total memory, trim DRAM back so the total is exactly 16M.
	 */
	if ((dram_size + vram_size) > 16)
		dram_size = 16 - vram_size;

	/*
	 * The rest of the system wants to deal with byte counts, not
	 * numbers of megabytes, so be nice to it.
	 */
	dram_size <<= 20;
	vram_size <<= 20;

	/*
	 * As mentioned above, VRAM starts where DRAM ends; note the
	 * actual VRAM starting address for future consumption.
	 */
	vram_start = (uint8 *) dram_size;

        if (vram_size > ONEMEG)
            two_vram_banks = TRUE;
        else
            two_vram_banks = FALSE;
}

void
internalInitMemList (MemList *ml, MemHdr *mh, char *name)
{
	ml->meml_Types = mh->memh_Types;
	ml->meml_MemHdr = mh;
	ml->meml_n.n_Name = (uchar *) name;
	ml->meml_n.n_SubsysType = KERNELNODE;
	ml->meml_n.n_Type = MEMLISTNODE;
	ml->meml_n.n_Flags = NODE_NAMEVALID;
	ml->meml_n.n_Size = sizeof (MemList);
	ml->meml_n.n_Priority = 100;	/* default */
	if (ml->meml_l) InitList (ml->meml_l, name);
}

/**
|||	AUTODOC PUBLIC spg/kernel/allocmemlist
|||	AllocMemList - Create a private memory list.
|||
|||	  Synopsis
|||
|||	    MemList *AllocMemList( const void *p, const char *name )
|||
|||	  Description
|||
|||	    A task can do its own memory management by creating one or more
|||	    private memory lists.  You use this procedure to allocate a
|||	    private memory list.
|||
|||	    A memory list you create with AllocMemList() is initially empty.
|||	    To add memory to the list, use the FreeMemToMemList() procedure.
|||
|||	    Note: A single memory list can store either DRAM or VRAM, but not
|||	    both.  The p argument points to an example of the memory the task
|||	    wants to manage with this MemList.
|||
|||	  Arguments
|||
|||	    p                           A pointer to a memory address whose
|||	                                memory type (either DRAM or VRAM) is
|||	                                the type you want to store in the
|||	                                list.  You control actual type of
|||	                                memory in the list by controlling what
|||	                                you free into the list.
|||
|||	    name                        The name of the memory list.
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to the MemList structure that is
|||	    created or NULL if an error occurs.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    To deallocate a memory list created with AllocMemList(), use
|||	    FreeMemList().
|||
|||	  See Also
|||
|||	    AllocMemFromMemList(), FreeMemList(), FreeMemToMemList()
|||
**/

/* only called by user code */
MemList *AllocMemList (const void *p, const char *name)
{
	MemList                *ml;
	MemHdr                 *mh;
	List                   *fml;
	List                   *l;

	fml = KernelBase->kb_CurrentTask->t_FreeMemoryLists;

	ml = (MemList *) AllocMemFromMemLists (fml, sizeof (*ml), MEMTYPE_FILL | MEMTYPE_ANY);
	if (ml)
	{
		l = (List *) AllocMemFromMemLists (fml, sizeof (*l), MEMTYPE_FILL | MEMTYPE_ANY);
		if (!l)
		{
			FreeMemToMemLists (fml, ml, sizeof (*ml));
			ml = 0;
		}
		else
		{
			ml->meml_l = l;
			mh = internalFindMH ((void *)p);
			internalInitMemList (ml, mh, (char *)name);
		}
	}
	return ml;
}

/**
|||	AUTODOC PUBLIC spg/kernel/freememlist
|||	FreeMemList - Free a memory list.
|||
|||	  Synopsis
|||
|||	    void FreeMemList( struct MemList *ml )
|||
|||	  Description
|||
|||	    This procedure frees a memory list that was allocated by a call to
|||	    AllocMemList().
|||
|||	  Arguments
|||
|||	    ml                          A pointer to the memory list to free.
|||	                                This value may be NULL in which case
|||	                                this function just returns.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveats
|||
|||	    If the list is not empty, any memory it contains is lost.
|||
|||	  See Also
|||
|||	    AllocMemList()
|||
**/

void
FreeMemList (ml)
	MemList                *ml;
{
	List                   *fml = KernelBase->kb_CurrentTask->t_FreeMemoryLists;

        if (!ml)
            return;

	FreeMemToMemLists (fml, ml->meml_l, sizeof (*ml->meml_l));
	FreeMemToMemLists (fml, ml->meml_AlignedTrackSize, sizeof (*ml->meml_AlignedTrackSize));
	FreeMemToMemLists (fml, ml, sizeof (*ml));
}

void
InitMemHdr (mh)
	MemHdr                 *mh;
{
	mh->memh_n.n_SubsysType = KERNELNODE;
	mh->memh_n.n_Type = MEMHDRNODE;
	mh->memh_n.n_Flags = NODE_NAMEVALID;
	mh->memh_n.n_Size = sizeof (MemHdr);
	mh->memh_PageSize = (int32) 1 << mh->memh_PageShift;
	mh->memh_PageMask = (uint32) (mh->memh_PageSize - 1);

	mh->memh_VRAMPageSize = (int32) 1 << mh->memh_VRAMPageShift;
	mh->memh_VRAMPageMask = (uint32) (mh->memh_VRAMPageSize - 1);
}

extern void	w64copy(uint32 src, uint32 dst, uint32 len);
extern void	w64zero(uint32 buf, uint32 len);

void
InitMem ()
{
    /* Grab rest of memory and add it to Kernel Free */
    /* memory pool */
    MemHdr                 *mh;
    MemList                *ml;
    uint32                  pagemask;

    if (dram_size == 0)
	Panic(1, "unsupported dram configuration\n");

/********************
 * initialize DRAM memory list
 */

    mh = &DRAMHdr;

    mh->memh_n.n_Name = (uchar *) "DRAM memory";
    mh->memh_n.n_Priority = 101;
    mh->memh_Types = MEMTYPE_AUDIO | MEMTYPE_DSP | MEMTYPE_CEL | MEMTYPE_DMA | MEMTYPE_DRAM;
    mh->memh_FreePageBits = DRAMpagebits;
    mh->memh_MemBase = (char *) 0;
    mh->memh_MemTop = (char *) dram_size;
    mh->memh_FreePageBitsSize = DRAMSetSize;
    mh->memh_PageShift = FindMSB ((dram_size - 1) / 64);
    mh->memh_VRAMPageShift = mh->memh_PageShift;

    InitMemHdr (mh);

    pagemask = mh->memh_PageMask;

    ml = &DRAMFreeMem;
    ml->meml_OwnBits = DRAMkernelbits;
    ml->meml_l = &DRAMFreeList;
    ml->meml_OwnBitsSize = mh->memh_FreePageBitsSize;

    internalInitMemList (ml, mh, "DRAM Free List");

    ml->meml_n.n_Priority = 101;

    ADDHEAD(KernelBase->kb_MemHdrList, (Node *) mh);
    ADDHEAD(KernelBase->kb_MemFreeLists, (Node *) ml);

/********************
 * initialize VRAM bank 1 memory list
 */

    mh = &VRAMHdr_1;

    mh->memh_n.n_Name = "VRAM Bank 1 memory";
    mh->memh_n.n_Priority = 100;
    mh->memh_Types = MEMTYPE_AUDIO | MEMTYPE_DSP | MEMTYPE_VRAM_BANK1 | MEMTYPE_DMA | MEMTYPE_CEL;
    mh->memh_FreePageBits = VRAMpagebits_1;
    mh->memh_MemBase = vram_start;
    mh->memh_MemTop = vram_start + ONEMEG;
    mh->memh_FreePageBitsSize = VRAMSetSize;
    mh->memh_PageShift = FindMSB ((ONEMEG - 1) / 64);
    mh->memh_VRAMPageShift = 11;

    if (vram_size > ONEMEG)
    {
        mh->memh_FreePageBitsSize = VRAMSetSize / 2;
        mh->memh_PageShift = FindMSB ((ONEMEG - 1) / 32);
    }

    InitMemHdr (mh);

    ml = &VRAMFreeMem_1;
    ml->meml_OwnBits = VRAMkernelbits_1;
    ml->meml_l = &VRAMFreeList_1;
    ml->meml_OwnBitsSize = mh->memh_FreePageBitsSize;

    internalInitMemList (ml, mh, "VRAM Bank 1 Free List");

    ADDTAIL(KernelBase->kb_MemHdrList, (Node *) mh);
    ADDTAIL(KernelBase->kb_MemFreeLists, (Node *) ml);

    if (vram_size > ONEMEG)
    {
        /********************
         * initialize VRAM bank 2 memory list
         */

        mh = &VRAMHdr_2;

        mh->memh_n.n_Name = "VRAM Bank 2 memory";
        mh->memh_n.n_Priority = 99;
        mh->memh_Types = MEMTYPE_AUDIO | MEMTYPE_DSP | MEMTYPE_VRAM_BANK2 | MEMTYPE_DMA | MEMTYPE_CEL;
        mh->memh_FreePageBits = VRAMpagebits_2;
        mh->memh_MemBase = vram_start + ONEMEG;
        mh->memh_MemTop = vram_start + ONEMEG + ONEMEG;
        mh->memh_FreePageBitsSize = VRAMSetSize / 2;
        mh->memh_PageShift = FindMSB ((ONEMEG - 1) / 32);
        mh->memh_VRAMPageShift = 11;

        InitMemHdr (mh);

        ml = &VRAMFreeMem_2;
        ml->meml_OwnBits = VRAMkernelbits_2;
        ml->meml_l = &VRAMFreeList_2;
        ml->meml_OwnBitsSize = mh->memh_FreePageBitsSize;

        internalInitMemList (ml, mh, "VRAM Bank 2 Free List");

        ADDTAIL(KernelBase->kb_MemHdrList, (Node *) mh);
        ADDTAIL(KernelBase->kb_MemFreeLists, (Node *) ml);
    }

/********************
 * The operator and filesystem have been placed where they belong for
 * systems with 2M of DRAM. Find them, and if necessary, move them to
 * where they should be. The final result is that we know where the
 * DRAM reserved for the kernel -- "kernel_reserve" -- really is; and
 * getting the operator and filesystem modules moved is a side effect.
 */

#define	DRAM_2M_PAGEBITS	15
#define	DRAM_2M_PAGESIZE	(1<<DRAM_2M_PAGEBITS)
#define	DRAM_2M_PAGEMASK	(DRAM_2M_PAGESIZE-1)
#undef	SHOWMOVES

    {
	extern AIFHeader	__my_AIFHeader;
	AIFHeader	       *aif_sh = &__my_AIFHeader;
	AIFHeader	       *aif_op = 0;
	AIFHeader	       *aif_fs = 0;
	uint32			tpm = DRAM_2M_PAGEMASK & pagemask;

#ifdef	SHOWMOVES
	printf("pagemask is %lx (on a 2M system, would be %lx); searching with %lx\n",
		pagemask, DRAM_2M_PAGEMASK, tpm);
#endif
	/* using sherry AIF, find operator */
	aif_op = FindImage(aif_sh, tpm, "Operator");
	if (aif_op == (AIFHeader *)0)
	    Panic(1, "sherry: unable to locate Operator");
	/* using operator AIF, find filesystem */
	aif_fs = FindImage(aif_op, tpm, "FileSystem");
	if (aif_fs == (AIFHeader *)0)
	    Panic(1, "sherry: unable to locate File System");
	/* using filesystem AIF, find end of kernel reserve */
	kernel_reserve = (int) FindImage(aif_fs, tpm, NULL);
	if (kernel_reserve == 0)
	    Panic(1, "sherry: unable to decode FileSystem's AIF header");

#ifdef	SHOWMOVES
	printf("module load map:\n");
	printf("%06lX  sherry\n", aif_sh);
	printf("%06lX  operator\n", aif_op);
	printf("%06lX  file system\n", aif_fs);
	printf("%06lX  end of kernel reserve\n", kernel_reserve);
#endif

	if (pagemask &~ DRAM_2M_PAGEMASK) {
	    uint32	op_src = (uint32)aif_op;
	    uint32	fs_src = (uint32)aif_fs;
	    uint32	op_sz = fs_src - op_src;
	    uint32	fs_sz = kernel_reserve - fs_src;
	    uint32	op_dst = (op_src + pagemask) &~ pagemask;
	    uint32	fs_dst = (fs_src + (op_dst - op_src) + pagemask) &~ pagemask;
/*
 * Move the filesystem to the correct place first, as
 * it is at the higher address ... and the correct place
 * for the operator may overlap the original place for
 * the file system.
 */
	    if (fs_dst != fs_src) {
#ifdef	SHOWMOVES
		printf("moving fs [%x..%x] => %x\n",
		       fs_src, fs_src+fs_sz-1, fs_dst);
#endif
		w64copy(fs_src, fs_dst, fs_sz);
		if ((fs_src + fs_sz) <= fs_dst)
		    w64zero(fs_src, fs_sz);
		else
		    w64zero(fs_src, fs_dst-fs_src);
		aif_fs = (AIFHeader *)fs_dst;
	    }
/*
 * Whether or not the filesystem moved, we need to
 * recalculate kernel_reserve based on the (possibly
 * changed) location of the filesystem and the (definitely
 * different) current correct pagemask.
 */
	    kernel_reserve = (int) FindImage(aif_fs, pagemask, NULL);
	    if (kernel_reserve == 0)
		Panic(1, "sherry: unable to decode FileSystem's AIF header");
/*
 * Now we can move the operator up, if needed.
 */
	    if (op_dst != op_src) {
#ifdef	SHOWMOVES
		printf("moving op [%x..%x] => %x\n",
		       op_src, op_src+op_sz-1, op_dst);
#endif
		w64copy(op_src, op_dst, op_sz);
		if ((op_src + op_sz) <= op_dst)
		    w64zero(op_src, op_sz);
		else
		    w64zero(op_src, op_dst-op_src);
		aif_op = (AIFHeader *)op_dst;
	    }
#ifdef	SHOWMOVES
	    printf("corrected module load map:\n");
	    printf("%06lX  sherry\n", aif_sh);
	    printf("%06lX  operator\n", aif_op);
	    printf("%06lX  file system\n", aif_fs);
	    printf("%06lX  end of kernel reserve\n", kernel_reserve);
#endif
	}
    }

/********************
 * regularize initial conditions for titles
 * by pre-clearing memory.
 */

    w64zero(kernel_reserve, dram_size - kernel_reserve);
#if 0
/* XXX- some VRAM pages should not be cleared */
/* XXX- but which ones???? */
    w64zero(vram_start, vram_size);
#endif

/********************
 * build the free lists,
 * leaving out the kernel reserve dram
 * and some bits of vram.
 */

    internalControlMem ((Task *) 0,
			(char *)kernel_reserve,
			dram_size - kernel_reserve,
			MEMC_GIVE, 0);

    internalControlMem ((Task *) 0,
			(char *) vram_start,
			ONEMEG,
			MEMC_GIVE, 0);

    if (vram_size > ONEMEG)
    {
        internalControlMem ((Task *) 0,
                            (char *) vram_start + ONEMEG,
                            ONEMEG,
                            MEMC_GIVE, 0);
    }

    KernelBase->kb_MemEnd = (uint32) mh->memh_MemTop;

    InitVMEM (vram_size);
}

/* handle strings up to 253 bytes in length */
/* eventually rewrite this to use a pool of memory with granularity */
/* of two bytes. All accesses to this memory will be byte at a time */
char                   *
AllocateString (n)
	char                   *n;
{
	int32			nsize;
	char                   *newP;

	nsize = strlen (n) + 2;		/* space for cnt and null */
	/* printf("AllocateString(%s) nsize=%d\n",n,nsize); */
	if (nsize > 255)
		return 0;
	newP = (char *) AllocMemFromMemLists (KernelBase->kb_MemFreeLists, nsize, MEMTYPE_ANY);
	/* printf("new=%lx\n",newP); */
	if (newP)
	{
		*newP++ = (uchar) nsize;
		strcpy (newP, n);
	}
	return newP;
}

void
FreeString (n)
	char                   *n;
{
	if (n)
	{
		n--;
		FREEMEM (n, *n);
	}
}

bool
IsLegalName (name)
	char                   *name;
{
	char                    c;

	if (name == 0)
		return 0;
	while ((c = *name++) != '\0')
	{
#ifdef undef
		if (c == ' ')
			continue;
		if (isalnum (c))
			continue;
#endif
		if (isprint (c))
			continue;	/* all the rest for now */
		return 0;
	}
	return 1;
}

char                   *
AllocateName (n)
	char                   *n;
{
	if (IsLegalName (n))
		return AllocateString (n);
	return 0;
}

/**
|||	AUTODOC PRIVATE spg/kernel/superisramaddr
|||	SuperIsRamAddr - Decide whether a region is in memory
|||
|||	  Synopsis
|||
|||	    int32 SuperIsRamAddr( char *p, int32 size )
|||
|||	  Description
|||
|||	    This function considers the described block of the address
|||	    space in relation to the known locations of RAM in the
|||	    system, and returns "true" if the block is entirely
|||	    contained within RAM, and "false" otherwise.
|||
|||	    The low 512 (0x200) bytes of the address space have been
|||	    declared to be "not memory" for all reasonable purposes;
|||	    this function will therefore return "false" for any block
|||	    of memory that uses any portion of low memory.
|||
|||	  Arguments
|||
|||	    p                           A pointer to the start of the block
|||
|||	    size                        The number of bytes in the block
|||
|||	  Return Value
|||
|||	    SuperIsRamAddr returns a nonzero value if the block is
|||	    entirely contained in memory, or zero if any part of the
|||	    block is outside memory or within the "low memory" area.
|||
|||	  Implementation
|||
|||	    Supervisor-mode folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    super.h                     ANSI C Prototype for SuperIsRamAddr
|||
|||	    kernellib.lib               ARM Link Library
|||
|||	  See Also
|||
|||	    IsRamAddr()
**/

/**
|||	AUTODOC PUBLIC spg/kernel/ismemreadable
|||	IsMemReadable - Decide whether a region of memory is fully
|||	                readable by the current task.
|||
|||	  Synopsis
|||
|||	    bool IsMemReadable( const void *p, int32 size )
|||
|||	  Description
|||
|||	    This function considers the described block of the address
|||	    space in relation to the known locations of RAM in the
|||	    system, and returns TRUE if the block is entirely
|||	    contained within RAM, and FALSE otherwise.
|||
|||	    The low 512 (0x200) bytes of the address space have been
|||	    declared to be "not memory" for all reasonable purposes;
|||	    this function will therefore return FALSE for any block
|||	    of memory that uses any portion of low memory.
|||
|||	  Arguments
|||
|||	    p                           A pointer to the start of the block
|||
|||	    size                        The number of bytes in the block
|||
|||	  Return Value
|||
|||	    This function returns TRUE if the block is entirely
|||	    readable by the current task, or FALSE if any part of the block
|||	    is not.
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
|||	    IsMemWritable()
**/


/* called directly by SuperIsRamAddress(), IsRamAddress(), and IsMemReadable() */
int32
IsRamAddr (void *p, uint32 size)
{
	uint32 end;

	/* all ram is contiguous, vram is always at the end */

	/*
	 * if we do not return true for all zero-length regions,
	 * applications crash.
	 */
	if (size == 0)
		return 1;		/* anything ok by definition */

	/* check for valid ram addr */
	if ((uint32)p < (uint32)0x200)
		return 0;		/* p is not in range [0x200, 2^32> */

	end = (uint32)p + size;
	if ((end < (uint32)p) || (end > KernelBase->kb_MemEnd))
		return 0;		/* p+size is not in range [p,MemEnd] */

	return 1;
}

/*
 * oldIsRamAddr does not disallow "low" memory.
 * This is needed for the message checking.
 */

int32
oldIsRamAddr (void *p, uint32 size)
{
	uint32 end;

	/* all ram is contiguous, vram is always at the end */

	/* must return true or apps crash */
	if (size == 0)
		return 1;		/* anything ok by definition */

	/* check for valid ram addr */
	end = (uint32)p + size;
	if ((end < (uint32)p) || (end > KernelBase->kb_MemEnd))
		return 0;		/* p+size is not in range [p,MemEnd] */

	return 1;
}

/**
|||	AUTODOC PRIVATE spg/kernel/findimage
|||	FindImage - locate next binary image in memory
|||
|||	  Synopsis
|||
|||	    AIFHeader *FindImage(AIFHeader *curr, uint32 pagemask, char *iname)
|||
|||	  Description
|||
|||	    Given the address of the start of a program image in
|||	    memory, this function returns the address where the next
|||	    program image with a given name could have been loaded. If no
|||	    address is specified, then the start of the current program
|||	    is used. If a pagemask is provided, it is used in the page
|||	    boundry calculations; otherwise, the proper pagemask is
|||	    retrieved for the memory in question. If no name is provided,
|||	    the next image address is returned.
|||
|||	  Arguments
|||
|||	    curr                        The image we want to skip to
|||	                                the end of. If a null is
|||	                                passed, then the image into
|||	                                which this function is linked
|||	                                will be used.
|||
|||	    pagemask                    The page mask (one less than
|||	                                the page size) to be used to
|||	                                find page boundrys. If a zero
|||	                                is passed, the proper pagemask
|||	                                for the memory being examined
|||	                                is retrieved.
|||
|||	    name                        The name of the image being
|||	                                searched for. If name is null,
|||	                                the address of the first image
|||	                                found after curr is returned.
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to the next probable
|||	    location after the specified AIF header that might be
|||	    the AIF header for the image name provided; if FindImage
|||	    can not find any sensible header in the memory byond the
|||	    end of the current image, it returns null pointer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V21.
|||
|||	  Associated Files
|||
|||	    aif.h                       ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveats
|||
|||	    It is conceivable that this routine could be fooled if the
|||	    ZeroInit segment was so small that the signature added to
|||	    the image loaded from disk went past the end of the
|||	    ZeroInit segment and lapped into the next page.
**/

/**
|||	AUTODOC PRIVATE spg/kernel/nextimage
|||	nextImage - locate next binary image in memory
|||
|||	  Synopsis
|||
|||	    AIFHeader *nextImage(AIFHeader *curr, uint32 pagemask)
|||
|||	  Description
|||
|||	    Given the address of the start of a program image in
|||	    memory, this function returns the address where the next
|||	    program image could have been loaded. If no address is
|||	    specified, then the start of the current program is used.
|||	    If a pagemask is provided, it is used in the page boundry
|||	    calculations; otherwise, the proper pagemask is retrieved
|||	    for the memory in question.
|||
|||	  Arguments
|||
|||	    curr                        The image we want to skip to
|||	                                the end of. If a null is
|||	                                passed, then the image into
|||	                                which this function is linked
|||	                                will be used.
|||
|||	    pagemask                    The page mask (one less than
|||	                                the page size) to be used to
|||	                                find page boundrys. If a zero
|||	                                is passed, the proper pagemask
|||	                                for the memory being examined
|||	                                is retrieved.
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to the next probable
|||	    location after the specified AIF header that might be
|||	    another AIF header; if nextImage can not make sense out of
|||	    the first header, it returns a null pointer.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V21.
|||
|||	  Associated Files
|||
|||	    aif.h                       ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveats
|||
|||	    It is conceivable that this routine could be fooled if the
|||	    ZeroInit segment was so small that the signature added to
|||	    the image loaded from disk went past the end of the
|||	    ZeroInit segment and lapped into the next page.
**/

AIFHeader *
FindImage(AIFHeader *aifp, uint32 pagemask, char *aifname)
{

	uint32	newaif;
	uint32	imagesize;
	uint32	pagesize;
	_3DOBinHeader *_3do;

	if (!aifp) aifp = &__my_AIFHeader;
	if (!pagemask) pagemask = GetPageSize(GetMemType((void *)aifp)) - 1;
	pagesize = pagemask + 1;

FIDBUG(("FindImage: size=%lx mask=%lx aifp=%lx'\n", pagesize, pagemask, aifp));

	newaif = (uint32)aifp;
	_3do = (_3DOBinHeader *)
	    ((unsigned)aifp +
	     ((unsigned)&__my_3DOBinHeader) -
	     ((unsigned)&__my_AIFHeader));
{
unsigned ro = aifp->aif_ImageROsize;
unsigned rw = aifp->aif_ImageRWsize;
unsigned zs = aifp->aif_ZeroInitSize;
unsigned st = _3do->_3DO_Stack;
if (_3do == &__my_3DOBinHeader) st = 0;	/* sherry stack BELOW not ABOVE */
FIDBUG(("FindImage: aifp=%lx 3do=%lx\n", aifp, _3do));
FIDBUG(("FindImage: sizes=%lx/%lx/%lx/%lx\n", ro, rw, zs, st));
	imagesize = ro + rw + zs + st;
}
	if (imagesize > (16<<20))	newaif += pagesize;
	else				newaif += imagesize;
FIDBUG(("FindImage: imagesize=%lx newaif=%lx\n", imagesize, newaif));
	newaif = (newaif + pagemask) & ~pagemask;

	if (aifname == NULL)
	{
FIDBUG(("FindImage: null aifname; imagesize=%ldk\n", imagesize>>10));
	    if (imagesize > (16<<20))
		return (AIFHeader *)NULL;
	    else
		return (AIFHeader *)newaif;
	}

	while (newaif < (uint32)(VRAMHdr_1.memh_MemTop)) // !!!
	{
	    _3do = (_3DOBinHeader *)(newaif + sizeof(AIFHeader));

FIDBUG(("FindImage: check for %s at %lx ...\n",aifname,newaif));
	    if (!strncasecmp(_3do->_3DO_Name, aifname, sizeof(_3do->_3DO_Name)))
		return (AIFHeader *)newaif;
	    newaif += pagesize;
	}
FIDBUG(("FindImage: %s not found!\n", aifname));
	return (AIFHeader *)NULL;
}

#ifdef	MEM_DEBUG
void
put_pagebits(char *name, int size, uint32 *words)
{
  uint32	m;
  uint32	b;
  printf("%5s:", name);
  while (size-->0)
  {
    m = *words++;
    for (b=1; b; b<<=1)
      printf("%c", (b & m) ? '*' : '.');
  }
  printf("\n");
}

void
put_memhdr(MemHdr *mh)
{
  printf("Memory Header '%s' covers 0x%lx to 0x%lx\n",
	 mh->memh_n.n_Name,
	 mh->memh_MemBase, mh->memh_MemTop);
  put_pagebits("free", mh->memh_FreePageBitsSize, mh->memh_FreePageBits);
}

void
put_memlist(MemList *ml)
{
  Node *n;

  put_memhdr(ml->meml_MemHdr);
  printf("Memory List '%s':\n", ml->meml_n.n_Name);
  put_pagebits("own", ml->meml_OwnBitsSize, ml->meml_OwnBits);
  put_pagebits("write", ml->meml_OwnBitsSize, ml->meml_WriteBits);
  for (n = FIRSTNODE(ml->meml_l);
       ISNODE(ml->meml_l, n);
       n = NEXTNODE(n))
  {
    printf("node: 0x%lx (%d)\n", n, n->n_Size);
  }
}
#endif
