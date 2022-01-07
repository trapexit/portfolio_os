/* $Id: allocmem.c,v 1.82 1994/11/18 02:32:26 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "listmacros.h"
#include "task.h"
#include "folio.h"
#include "kernel.h"
#include "mem.h"
#include "string.h"
#include "internalf.h"
#include "item.h"
#include "semaphore.h"
#include "debug.h"
#include "stdio.h"

/* #define TRASHMEM */

#define TYPES_OF_MEM (MEMTYPE_DMA | MEMTYPE_CEL | MEMTYPE_DRAM | \
		      MEMTYPE_AUDIO | MEMTYPE_DSP | \
		      MEMTYPE_VRAM_BANK1 | MEMTYPE_VRAM_BANK2 )

#ifdef TRASHMEM
int                     trashflag = 1;
#endif

extern bool             isUser (void);
extern void             Panic (int halt, const char *fmt,...);

#define INFO(x)		{ if (isUser()==0) printf x ; else kprintf x ; }

/* MEM_ALLOC_ALIGN_SIZE must be at least MINFREE.
 * All allocations are in multiples of, and aligned
 * on multiples of, MEM_ALLOC_ALIGN_SIZE.
 */
#define MINFREE                 (sizeof(NamelessNode))
#define	MEM_ALLOC_ALIGN_LOG2	4
#define	MEM_ALLOC_ALIGN_SIZE	(1<<MEM_ALLOC_ALIGN_LOG2)
#define	MEM_ALLOC_ALIGN_MASK	(MEM_ALLOC_ALIGN_SIZE-1)

#define ROUND_ALLOC_SIZE(x) {x += MEM_ALLOC_ALIGN_MASK; x &= ~MEM_ALLOC_ALIGN_MASK;}

#define VRAM_PAGE_SIZE	2048
#define VRAM_PAGE_MASK	(VRAM_PAGE_SIZE-1)


/*****************************************************************************/


typedef struct MemTrack
{
    MinNode  mt_Link;
    void    *mt_Addr;
    int32    mt_Size;
} MemTrack;


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/kernel/universalinsertnode
|||	UniversalInsertNode - Insert a node into a list.
|||
|||	  Synopsis
|||
|||	    void UniversalInsertNode( List *l, Node *n, bool (*f)(Node *n,
|||	    Node *m) )
|||
|||	  Description
|||
|||	    Every node in a list has a priority (a value from 0 to 255 that is
|||	    stored in the n_Priority field of the node structure).  When a new
|||	    node is inserted with InsertNodeFromHead() or
|||	    InsertNodeFromTail(), the position at which it is added is
|||	    determined by its priority.  The UniversalInsertNode() procedure
|||	    allows tasks to arrange list nodes according to values other than
|||	    priority.
|||
|||	    UniversalInsertNode() uses a comparison function provided by the
|||	    calling task to determine where to insert a new node.  It compares
|||	    the node to be inserted with nodes already in the list, beginning
|||	    with the first node.  If the comparison function returns TRUE, the
|||	    new node is inserted immediately before the node to which it was
|||	    compared.  If the comparison function never returns TRUE, the new
|||	    node becomes the last node in the list.  The comparison function,
|||	    whose arguments are pointers to two nodes, can use any data in the
|||	    nodes for the comparison.
|||
|||	  Arguments
|||
|||	    l                           A pointer to the list into which to
|||	                                insert the node.
|||
|||	    n                           A pointer to the node to insert.  This
|||	                                same pointer is passed as the first
|||	                                argument to the comparison function.
|||
|||	    f                           A comparison function provided by the
|||	                                calling task that returns TRUE if the
|||	                                node to be inserted (pointed to by the
|||	                                first argument to the function) should
|||	                                be inserted immediately before the
|||	                                node to which it is compared (pointed
|||	                                to by the second argument to the
|||	                                function).
|||
|||	    m                           A pointer to the node in the list to
|||	                                which to compare the node to insert.
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
|||	  Notes
|||
|||	    A node can be included only in one list.
|||
|||	  Caveats
|||
|||	    GIGO (`garbage in, garbage out')
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    RemHead(), RemNode(), RemTail()
|||
**/

void
UniversalInsertNode (List *l, Node *n, bool (*f) (Node *n, Node *m))
{
	/* Insert node based on function comparison of node */
	Node                   *p;

	for (p = FIRSTNODE (l); ISNODE (l, p); p = NEXTNODE (p))
	{
		if ((*f) (n, p))
		{
			/* put right here */
			if (p == FIRSTNODE (l))
			{
				ADDHEAD (l, n);
			}
			else
			{
				n->n_Next = p;
				n->n_Prev = p->n_Prev;
				p->n_Prev->n_Next = n;
				p->n_Prev = n;
			}
			return;
		}
	}
	ADDTAIL (l, n);
}

static bool
comparef (Node *m, Node *n)
{
	return (uint32) m < (uint32) n;
}

/*
 * Coallesc:
 * check previous and next
 * nodes to see if we can merge
 * the data into one node
 */
static void
Coallesc (List *l, Node *n)
{
	Node                   *p;
	Node                   *m;

	/* does end of p match start of node n? */
	p = n->n_Prev;
	if (ISNODEB (l, p))
	{
		m = (Node *) ((uint32) p + p->n_Size);
		if (m == n)
		{
			REMOVENODE (n);
			p->n_Size += n->n_Size;
			n = p;		/* New node n */
		}
	}

	/* does end of node n match with start of node p? */
	p = n->n_Next;
	if (ISNODE (l, p))
	{

		m = (Node *) ((uint32) n + n->n_Size);
		if (m == p)
		{
			REMOVENODE (p);
			n->n_Size += p->n_Size;
		}
	}
}

void
AddMem (MemList *ml, NamelessNode *n, int32 size)
{
	List                   *l;

	n->n_SubsysType = KERNELNODE;
	n->n_Type       = MEMFREENODE;
	n->n_Priority   = 0;
	n->n_Flags      = 0;
	if (ml->meml_Sema4)
		LockSemaphore (ml->meml_Sema4, SEM_WAIT);

	if (size < 0)
	{
	MemTrack *mt;

	    ScanList(ml->meml_AlignedTrackSize,mt,MemTrack)
	    {
	        if (mt->mt_Addr == n)
	        {
	            size = mt->mt_Size;
	            ROUND_ALLOC_SIZE(size);
	            REMOVENODE((Node *)mt);
	            FreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,mt,sizeof(MemTrack));
	            break;
	        }
	    }
	}

	n->n_Size = size;		/* Size of entire node */
	l = ml->meml_l;
	UniversalInsertNode (l, (Node *) n, comparef);
	Coallesc (l, (Node *) n);
	if (ml->meml_Sema4)
		UnlockSemaphore (ml->meml_Sema4);
}

MemList *
MatchMemHdr (List *l, MemHdr *mh)
{
	Node                   *n;
	MemList                *memlist;

	if (!l)
		return 0;		/* only check a valid list */

	for (n = FIRSTNODE (l); ISNODE (l, n); n = NEXTNODE (n))
	{
		memlist = (MemList *) n;
		if (memlist->meml_MemHdr == mh)
			return (MemList *) n;
	}
	return 0;
}

MemList *
FindMemList (List *l, uint8 *q)
{
	Node                   *n;
	MemList                *memlist;
	MemHdr                 *mh;

	for (n = FIRSTNODE (l); ISNODE (l, n); n = NEXTNODE (n))
	{
		memlist = (MemList *) n;
		mh = memlist->meml_MemHdr;
		if ((mh->memh_MemBase <= q) &&
		    (mh->memh_MemTop > q))
			return memlist;
	}
	return 0;
}

void
TrashMem (uint32 *p, uint32 val, int32 cnt)
{
#ifdef TRASHMEM
	if (trashflag)
	{
		if (KernelBase->kb_CPUFlags & KB_NODBGR)
			return;
		while (cnt > 0)
		{
			*p++ = val;
			cnt -= 4;
		}
	}
#endif
}

/**
|||	AUTODOC PUBLIC spg/kernel/freememtomemlist
|||	FreeMemToMemList - Free a private block of memory.
|||
|||	  Synopsis
|||
|||	    void FreeMemToMemList( struct MemList *ml, void *p, int32 size )
|||
|||	  Description
|||
|||	    This procedure frees a private block of memory that was previously
|||	    allocated by a call to AllocMemFromMemList().  You can also use it
|||	    to add memory to a private memory list created by AllocMemList().
|||
|||	    The block of memory is expected to begin and end on nicely
|||	    aligned bounds; the alignment size can be obtained by
|||	    calling GetMemAllocAlignment() . If the start of the block
|||	    is not properly aligned, the leading partial chunk is
|||	    discarded; if the end of the block is not aligned, it is
|||	    extended to include the entire block.
|||
|||	    Note:  The memory you free to a memory list must be either memory
|||	    that was previously allocated from the list or (when adding new
|||	    memory to the list) memory of the type specified (by the p
|||	    argument of AllocMemList() ) when the memory list was created.
|||
|||	  Arguments
|||
|||	    ml                          A pointer to the memory list to which
|||	                                to free the memory.
|||
|||	    p                           A pointer to the memory to free. This
|||	                                value may be NULL in which case this
|||	                                function just returns.
|||
|||	    size                        The number of bytes to free.  This
|||	                                must be the same size that was passed
|||	                                to AllocMemFromMemList() to allocate
|||	                                the block.
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
|||	  Caveat
|||
|||	    If you pass a block of memory to FreeMemToMemList that was
|||	    not directly received from AllocMemFromMemList, you MUST
|||	    bear in mind that the end of the block will be *EXTENDED*
|||	    to the next alignment boundrey, not truncated back. Call
|||	    GetMemAllocAlignment() to get the block size.
|||
|||	  See Also
|||
|||	    AllocMemFromMemList(), AllocMemList(), FreeMemList(),
|||	    GetMemAllocAlignment().
|||
**/

void
FreeMemToMemList (MemList *ml, void *q, int32 size)
{
uint32 p;

    if ((size == 0) || (q == NULL))
        return;

    if (size < 0)
    {
        if ((uint32)q & MEM_ALLOC_ALIGN_MASK)
        {
            p     = (uint32)q - sizeof(int32);
            size  = *(int32 *)p + sizeof(uint32);
            ROUND_ALLOC_SIZE(size);
        }
        else
        {
            p = (uint32)q;
        }
    }
    else
    {
        p = ((uint32)q + MEM_ALLOC_ALIGN_MASK) & ~MEM_ALLOC_ALIGN_MASK;

        /* The rounding up of size here must match the rounding up of
         * the size in AllocMemFromMemList: code calls AllocMemFromMemList
         * with an arbitrary size, and gets back a pointer; later, that
         * pointer and the same arbitrary size are passed into here. If this
         * code is changed to truncate size down instead of up, every such
         * alloc/free cycle of non-aligned sizes results in one allocation
         * chunk (from the end of the allocation) being irretrievably lost,
         * not to mention horrible memory fragmentation.
         *
         * Anyone who calls FreeMemToMemList with a pointer they did not get
         * from AllocMemFromMemList or with some size other than what they
         * gave AllocMemFromMemList to get the pointer, *MUST* know absolutely
         * that the size will be rounded up. See the above AUTODOC comment
         * about
         */
        size -=  p - (uint32)q;
        ROUND_ALLOC_SIZE(size);
    }

#ifdef TRASHMEM
    if (trashflag)
            TrashMem((uint32 *)p, (uint32)0xdeaddead, size);
#endif

    AddMem(ml, (NamelessNode *)p, size);
}

/**
|||	AUTODOC PUBLIC spg/kernel/freemem
|||	FreeMem - Freememory from AllocMem().
|||
|||	  Synopsis
|||
|||	    void FreeMem( void *p, int32 size )
|||
|||	  Description
|||
|||	    This macro frees memory that was previously allocated by a call to
|||	    AllocMem().  The size argument specifies the number of bytes to
|||	    free.  The freed memory is automatically returned to the memory
|||	    list from which is was allocated.
|||
|||	  Arguments
|||
|||	    p                           A pointer to the memory block to free.
|||	                                This value may be NULL in which
|||	                                case this function just returns.
|||
|||	    size                        The size of the block to free, in
|||	                                bytes.  This must be the same size
|||	                                that was specified when the block was
|||	                                allocated.
|||
|||	  Implementation
|||
|||	    Macro implemented in mem.h V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       ANSI C Macro definition
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by
|||	    compiling your entire project with the MEMDEBUG value
|||	    defined on the compiler's command-line. Refer to the
|||	    CreateMemDebug() function for more details.
|||
|||	  See Also
|||
|||	    AllocMem()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/freememtomemlists
|||	FreeMemToMemLists - Return memory to the free pool.
|||
|||	  Synopsis
|||
|||	    void FreeMemToMemLists( List *l, void *p, int32 size )
|||
|||	  Description
|||
|||	    The procedure returns a block of memory that was allocated with
|||	    AllocMemFromMemLists() to the specified free memory pool (a list
|||	    of memory lists).
|||
|||	    Note:  Unless you are trying to move memory from one memory pool
|||	    to another, you should always free memory to the same pool that
|||	    you obtained it from.
|||
|||	  Arguments
|||
|||	    l                           A pointer to the memory pool (a list
|||	                                of memory lists) to which to return
|||	                                the memory block.
|||
|||	    p                           A pointer to the memory to free. This
|||	                                value may be NULL in which case this
|||	                                function just returns.
|||
|||	    size                        Number of bytes to free.  This must be
|||	                                the same size that was passed to
|||	                                AllocMemFromMemLists() to allocate the
|||	                                block.
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
|||	    You can enable memory debugging in your application by
|||	    compiling your entire project with the MEMDEBUG value
|||	    defined on the compiler's command-line. Refer to the
|||	    CreateMemDebug() function for more details.
|||
|||	  See Also
|||
|||	    AllocMem(), AllocMemFromMemLists(), FreeMem()
|||
**/

void
FreeMemToMemLists (List *l, void *q, int32 size)
{
	FreeMemToMemList (FindMemList(l, (char *)q), q, size);
}

void                   *
FindML (List *l, uint32 types)
{
	Node                   *n;
	MemList                *ml;
	uint32                  mt;

	types &= TYPES_OF_MEM;
	for (n = FIRSTNODE (l); ISNODE (l, n); n = NEXTNODE (n))
	{
		ml = (MemList *) n;
		mt = ml->meml_Types;
		mt &= types;		/* mask out bits we need */
		if (types == mt)
			break;		/* a match? */
	}
	if (!(ISNODE (l, n)))
		n = 0;
	return (MemList *) n;
}

/**
|||	AUTODOC PUBLIC spg/kernel/getpagesize
|||	GetPageSize - Get the number of bytes in a memory page.
|||
|||	  Synopsis
|||
|||	    int32 GetPageSize( uint32 memflags )
|||
|||	  Description
|||
|||	    This procedure gets the number of bytes in a page of the specified
|||	    type of memory.
|||
|||	  Arguments
|||
|||	    memflags                    Flags that specify the type of memory
|||	                                whose page size to get.  These flags
|||	                                can include MEMTYPE_ANY, MEMTYPE_VRAM,
|||	                                MEMTYPE_DRAM, MEMTYPE_BANKSELECT,
|||	                                MEMTYPE_BANK1, MEMTYPE_BANK2,
|||	                                MEMTYPE_DMA, MEMTYPE_CEL,
|||	                                MEMTYPE_AUDIO, MEMTYPE_DSP, and
|||	                                MEMTYPE_SYSTEMPAGESIZE.  For
|||	                                information about these flags, see the
|||	                                description of AllocMem().
|||
|||	  Return Value
|||
|||	    The procedure returns the size, in bytes, of a page of the
|||	    specified type of RAM.  If there is no memory of the specified
|||	    type, the procedure returns 0.
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
|||	    If you do not specify the type of memory you're asking about, the
|||	    page size you get may be for memory you're not interested in.  The
|||	    page size for a particular piece of memory may be not what you
|||	    expected if the amount of memory requested causes the allocator to
|||	    jump to a different type of memory that can satisfy your request,
|||	    but which may have a different page size.  To ensure that you get
|||	    the actual page size of the memory you're interested, call
|||	    GetMemType() to get the memory flags for the memory, then call
|||	    GetPageSize() with those flags as the memflags argument.
|||
|||	  See Also
|||
|||	    GetMemType()
|||
**/

int32
GetPageSize (uint32 flags)
{
	MemList                *ml;
	MemHdr                 *mh;
	int32                   pagesize;

	ml = (MemList *) FindML (KernelBase->kb_MemFreeLists, flags);
	if (ml == 0)
		return 0;
	mh = ml->meml_MemHdr;
	pagesize = mh->memh_PageSize;
	if ((flags & MEMTYPE_VRAM) && !(flags & MEMTYPE_SYSTEMPAGESIZE))
		pagesize = mh->memh_VRAMPageSize;
	return pagesize;
}

/**
|||	AUTODOC PUBLIC spg/kernel/getmemallocalignment
|||	GetMemAllocAlignment - Get the memory allocation alignment size
|||
|||	  Synopsis
|||
|||	    int32 GetMemAllocAlignment( uint32 memflags )
|||
|||	  Description
|||
|||	    This function returns the alignment guarantted by AllocMem
|||	    and friends, and required by FreeMem and friends.
|||
|||	  Arguments
|||
|||	    memflags                    Similar to the usage in
|||	                                GetPageSize(); different
|||	                                allocators may be in use for
|||	                                different kinds of memory,
|||	                                which may have different
|||	                                alignment guarantees and restrictions.
|||
|||	  Return Value
|||
|||	    The return value is the alignment modulus for the memory
|||	    specified; for example, if AllocMem guaranees (and FreeMem
|||	    requires) long-word alignment, it returns "4". If no
|||	    alignment is guaranteed or required, it returns "1".
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V22.
|||
|||	  Associated Files
|||
|||	    mem.h                       ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveat
|||
|||	    Note that malloc() does not necessarily provide same
|||	    alignment guarantee as AllocMem, as malloc() reserves some
|||	    space at the front of the block it gets from AllocMem to
|||	    contain the size of the block allocated.
|||
|||	  See Also
|||
|||	    GetMemType(), AllocMem(), FreeMem(), malloc(), free()
|||
**/

/*
 * At the moment, we have only one allocator, and
 * it guaranttees and requires MEM_ALLOC_ALIGN_SIZE-byte alignment.
 */
int32
GetMemAllocAlignment (uint32 flags)
{
	return MEM_ALLOC_ALIGN_SIZE;
}


/*****************************************************************************/


void                   *
internalAllocMemFromMemList (MemList *ml, int32 reqsize, int32 usersize,
			     uint32 flags)
{
	MemHdr                 *mh;
	int32                   pagesize;
	uint32                  pagemask;
	int32                  *p;
	Node                   *n;

/* assumptions:
 *	reqsize > 0
 *	reqsize = multiple of MEM_ALLOC_ALIGN_SIZE
 *	reqsize includes room to store size when MEMTYPE_TRACKSIZE
 *	ml = ptr to valid memlist hdr for this memtype
 *	flags = type bits
 */


	mh = ml->meml_MemHdr;

	pagesize = mh->memh_PageSize;
	if ((flags & MEMTYPE_VRAM) && !(flags & MEMTYPE_SYSTEMPAGESIZE))
		pagesize = mh->memh_VRAMPageSize;

#ifdef DEVELOPMENT
	if ((reqsize == 0) || (reqsize & MEM_ALLOC_ALIGN_MASK))
	{
		INFO (("allocmem error, bad request size\n"));
		while (1);
	}
#endif

	pagemask = (int32) pagesize - 1;

	/* weed out the impossible */
	if (flags & MEMTYPE_INPAGE)
	{
		if (reqsize > pagesize)
		{
			return 0;
		}
	}

	/* Supervisor allocations are always from the top (for now) */
	if (isUser () == 0)
		flags |= MEMTYPE_FROMTOP;

	/*
	 * For SuperMemory Sema4 will always be zero,
	 * is this true for mt?
	 */
	if (ml->meml_Sema4)
		LockSemaphore (ml->meml_Sema4, SEM_WAIT);

	p = 0;

	/*
	 * Now search through the memlist and try to satisfy the
	 * request
	 */
	if (flags & MEMTYPE_FROMTOP)
	{
		for (n = LASTNODE (ml->meml_l);
		     ISNODEB (ml->meml_l, n);
		     n = PREVNODE (n))
		{

			if ((uint32) n & MEM_ALLOC_ALIGN_MASK)
			{
#ifdef DEVELOPMENT
				INFO (("allocmem error, bad node address\n"));
				while (1);
#else
				if (isUser () == 0)
					Panic (1, "AllocMem error, bad node address\n");
#endif
			}
#ifdef DEVELOPMENT
			if (n->n_Size & MEM_ALLOC_ALIGN_MASK)
			{
				INFO (("allocmem error, bad node size\n"));
				while (1);
			}
#endif
			if (n->n_Size >= reqsize)
			{
				uint32                  ntop, nend;
				uint32			ptop, pend;
				int32                   leftover;	/* must be signed */

				ntop = (uint32) n;
				nend = ntop + n->n_Size;
				pend = nend;
				ptop = pend - reqsize;

				/*
				 * If STARTPAGE is specified, and we
				 * are not at a page break, roll us
				 * back to start at a page break.
				 *
				 * If INPAGE is specified, and we cross a
				 * page break, roll us back until we no
				 * longer cross it -- in other words,
				 * until we end at a page break.
				 *
				 * Note that if we have done the STARTPAGE
				 * correction, we need not check
				 * whether we are INPAGE or not.
				 */
				if ((flags & MEMTYPE_STARTPAGE) &&
				    ((ptop & pagemask) != 0))
				{
					ptop &= ~pagemask;
					pend = ptop + reqsize;
				}
				else if ((flags & MEMTYPE_INPAGE) &&
					 ((ptop & ~pagemask) !=
					  ((pend - 1) & ~pagemask)))
				{
					pend &= ~pagemask;
					ptop = pend - reqsize;
				}
				else
				{

					/*
					 * no alignment fixups;
					 * allocating from end of node.
					 * If we have a leading free
					 * area, adjust the size of the
					 * free node; otherwise, dust
					 * it.
					 */
					leftover = ptop - ntop;
					if (leftover > 0)
						n->n_Size = leftover;
					else
						REMOVENODE (n);
					p = (int32 *) ptop;
					break;
				}

				/*
				 * If our allocation requirements
				 * pushed us up beyond the front of the
				 * node, reject this allocation.
				 */
				if (ptop < ntop)
					continue;

				/*
				 * If we have a leading free area,
				 * adjust the size of the free node;
				 * otherwise, dust it.
				 */
				leftover = ptop - ntop;
				if (leftover > 0)
					n->n_Size = leftover;
				else
					REMOVENODE (n);

				/*
				 * If we have a trailing free area, add
				 * it back into the free list.
				 */
				leftover = nend - pend;
				if (leftover > 0)
					AddMem (ml, (NamelessNode *) pend, (int32) leftover);

				p = (int32 *) ptop;
				break;
			}
		}
	}
	else
	{
		for (n = FIRSTNODE (ml->meml_l);
		     ISNODE (ml->meml_l, n);
		     n = NEXTNODE (n))
		{

			if ((uint32) n & MEM_ALLOC_ALIGN_MASK)
			{
#ifdef DEVELOPMENT
				INFO (("allocmem error, bad node address\n"));
				while (1);
#else
				if (isUser () == 0)
					Panic (1, "AllocMem error, bad node address\n");
#endif
			}
			if (n->n_Size & MEM_ALLOC_ALIGN_MASK)
			{
#ifdef DEVELOPMENT
				INFO (("allocmem error, bad node size\n"));
				while (1);
#else
				if (isUser () == 0)
					Panic (1, "AllocMem error, bad node size\n");
#endif
			}
			if (n->n_Size >= reqsize)
			{
				uint32                  ntop, nend;
				uint32			ptop, pend;
				int32                   leftover;	/* must be signed */

				ntop = (uint32) n;
				nend = ntop + n->n_Size;
				ptop = ntop;
				pend = ptop + reqsize;

				/*
				 * If STARTPAGE is specified and we are
				 * not at a page break, or If INPAGE is
				 * specified and we cross a page break,
				 * roll us forward to the next page
				 * bound.
				 */
				if (((flags & MEMTYPE_STARTPAGE) &&
				     ((ptop & pagemask) != 0)) ||
				    ((flags & MEMTYPE_INPAGE) &&
				     ((ptop & ~pagemask) != ((pend - 1) & ~pagemask))))
				{
					ptop = (ptop | pagemask) + 1;
					pend = ptop + reqsize;

					/*
					 * If our allocation
					 * requirements pushed us up
					 * beyond the front of the
					 * node, reject this
					 * allocation.
					 */
					if (pend > nend)
						continue;

					/*
					 * If we have a leading free
					 * area, adjust the size of the
					 * free node; otherwise, dust
					 * it.
					 */
					leftover = ptop - ntop;
					if (leftover > 0)
						n->n_Size = leftover;
					else
						REMOVENODE (n);
				}
				else
				{

					/*
					 * Allocating from start of
					 * node: delete the old node.
					 */
					REMOVENODE (n);
				}

				/*
				 * If we have a trailing free area, add
				 * it back into the free list.
				 */
				leftover = nend - pend;
				if (leftover > 0)
					AddMem (ml, (NamelessNode *) pend, (int32) leftover);

				p = (int32 *) ptop;
				break;
			}
		}
	}

	if (p)
	{
	    if (flags & MEMTYPE_TRACKSIZE)
	    {
	        if (flags & MEMTYPE_STARTPAGE)
	        {
	            if (!ml->meml_AlignedTrackSize)
	            {
	                ml->meml_AlignedTrackSize = (List *)AllocMemFromMemLists(CURRENTTASK->t_FreeMemoryLists,sizeof(List),MEMTYPE_ANY);
                        if (ml->meml_AlignedTrackSize)
                            InitList(ml->meml_AlignedTrackSize,"Aligned Allocs");
                    }

                    if (ml->meml_AlignedTrackSize)
	            {
	            MemTrack *mt;

	                mt = (MemTrack *)AllocMemFromMemLists(CURRENTTASK->t_FreeMemoryLists,sizeof(MemTrack),MEMTYPE_ANY);
	                if (mt)
	                {
	                    mt->mt_Addr = p;
	                    mt->mt_Size = usersize;
	                    ADDHEAD(ml->meml_AlignedTrackSize,(Node *)mt);
	                }
	                else
	                {
	                    FreeMemToMemList(ml,p,reqsize);
	                    p = NULL;
	                }
	            }
	            else
	            {
	                FreeMemToMemList(ml,p,reqsize);
	                p = NULL;
	            }
	        }
	        else
	        {
	            *p = usersize;
	            p  = (int32 *)((uint32)p + sizeof(int32));
	            reqsize -= sizeof(int32);
	        }
	    }
	}

	if (ml->meml_Sema4)
		UnlockSemaphore (ml->meml_Sema4);

	if (p)
	{
		if (flags & MEMTYPE_FILL)
			memset ((uint8 *) p,
				(int) (flags & MEMTYPE_FILLMASK),
				reqsize);
		else
		{
#ifdef TRASHMEM
			if (trashflag)
				TrashMem ((uint32 *) p,
					  (uint32) 0x0bad0bad,
					  reqsize);
#endif
			*p = (int32) (reqsize);
		}
	}

	return p;
}

/**
|||	AUTODOC PUBLIC spg/kernel/allocmemfrommemlist
|||	AllocMemFromMemList - Allocate memory from a private memory pool.
|||
|||	  Synopsis
|||
|||	    void *AllocMemFromMemList( struct MemList *ml, int32 size, uint32
|||	    memflags)
|||
|||	  Description
|||
|||	    A task can do its own memory management by creating one or more
|||	    private memory lists.  Use this procedure to allocate a memory
|||	    block from a private memory list.
|||
|||	    Note: Most applications do not need to do their own memory
|||	    management.  When you use standard memory allocation procedures
|||	    like AllocMem(), the details of memory management are handled for
|||	    you by the Kernel.
|||
|||	    To create a private memory list, use the AllocMemList() procedure.
|||
|||	  Arguments
|||
|||	    ml                          A pointer to the private memory list
|||	                                from which to allocate the memory
|||	                                block.
|||
|||	    size                        The size of the memory block to
|||	                                allocate, in bytes.
|||
|||	    memflags                    Flags that specify the type of memory
|||	                                to allocate.  These flags include
|||	                                MEMTYPE_ANY, MEMTYPE_VRAM,
|||	                                MEMTYPE_DRAM, MEMTYPE_BANKSELECT,
|||	                                MEMTYPE_BANK1, MEMTYPE_BANK2,
|||	                                MEMTYPE_DMA, MEMTYPE_CEL,
|||	                                MEMTYPE_AUDIO, MEMTYPE_DSP,
|||	                                MEMTYPE_FILL, MEMTYPE_INPAGE,
|||	                                MEMTYPE_STARTPAGE, and MEMTYPE_MYPOOL.
|||	                                For information about these flags, see
|||	                                the description of AllocMem().
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to the allocated memory or NULL if
|||	    the memory couldn't be allocated.
|||
|||	    Note:  If there is not enough memory in the specified memory list
|||	    to satisfy the request, the Kernel returns 0.
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
|||	    To free a memory block allocated with AllocMemFromMemList(), use
|||	    the FreeMemToMemList() procedure.
|||
|||	  See Also
|||
|||	    AllocMemList(), FreeMemToMemList()
|||
**/

void *
AllocMemFromMemList (MemList *ml, int32 usersize, uint32 flags)
{
	int32                   reqsize;

        reqsize = usersize;
        if (reqsize <= 0)
            return NULL;

        if (flags & MEMTYPE_TRACKSIZE)
        {
            if ((flags & MEMTYPE_STARTPAGE) == 0)
                reqsize += sizeof(int32);
        }

        ROUND_ALLOC_SIZE(reqsize);

	return internalAllocMemFromMemList (ml, reqsize, usersize, flags);
}

/**
|||	AUTODOC PUBLIC spg/kernel/allocmem
|||	AllocMem - Allocate a memory block of a specific type.
|||
|||	  Synopsis
|||
|||	    void *AllocMem( int32 s, uint32 t )
|||
|||	  Description
|||
|||	    This macro allocates a memory block of a specific type.
|||
|||	  Arguments
|||
|||	    s                           The size of the memory block to
|||	                                allocate, in bytes.
|||
|||	    t                           Flags that specify the type,
|||	                                alignment, and fill characteristics of
|||	                                memory to allocate.
|||
|||	    One of the following flags must be set:
|||
|||	    MEMTYPE_ANY                 Any memory can be allocated.
|||
|||	    MEMTYPE_VRAM                Allocate only video random-access
|||	                                memory (VRAM).
|||
|||	    MEMTYPE_DRAM                Allocate only dynamic random-access
|||	                                memory (DRAM).
|||
|||	    If a block of VRAM must come from a specific VRAM bank, the
|||	    following flag must be set:
|||
|||	    MEMTYPE_BANKSELECT          Allocate VRAM from a specific VRAM
|||	                                bank.  In addition, one of the
|||	                                following two VRAM bank selection
|||	                                flags must be set:
|||
|||	    MEMTYPE_BANK1               Allocate only memory from VRAM bank 1.
|||
|||	    MEMTYPE_BANK2               Allocate only memory from VRAM bank 2.
|||
|||	    The following flags are provided for compatibility with future
|||	    hardware.  You can set them in addition to the preceding flags.
|||
|||	    MEMTYPE_DMA                 Allocate only memory that is
|||	                                accessible via direct memory access
|||	                                (DMA).  Currently, all memory is
|||	                                accessible via DMA, but this may not
|||	                                be true in future hardware.  Set this
|||	                                flag if you know the memory must be
|||	                                accessible via DMA.
|||
|||	    MEMTYPE_CEL                 Allocate only memory that is
|||	                                accessible to the cel engine.
|||	                                Currently, all memory is accessible to
|||	                                the cel engine, but this may not be
|||	                                true in future hardware.  Set this
|||	                                flag if you know the memory will be
|||	                                used for graphics.
|||
|||	    MEMTYPE_AUDIO               Allocate only memory that can be used
|||	                                for audio data (such as digitized
|||	                                sound).  Currently, all memory can be
|||	                                used for audio, but this may not be
|||	                                true in future hardware.  Set this
|||	                                flag if you know the memory will be
|||	                                used for audio data.
|||
|||	    MEMTYPE_DSP                 Allocate only memory that is
|||	                                accessible to the digital signal
|||	                                processor (DSP).  Currently, all
|||	                                memory is accessible to the DSP, but
|||	                                this may not be true in future
|||	                                hardware.  Set this flag if you know
|||	                                the memory must be accessible to the
|||	                                DSP.
|||
|||	    The following flags specify alignment, fill, and other allocation
|||	    characteristics:
|||
|||	    MEMTYPE_FILL                Set every byte in the memory block to
|||	                                the value of the lower eight bits of
|||	                                the flags.  If this flag is not set,
|||	                                the previous contents of the memory
|||	                                block are not changed.  (Using 0 as
|||	                                the fill value can be useful for
|||	                                debugging: Any memory that is
|||	                                inadvertently changed can easily be
|||	                                detected.)
|||
|||	    MEMTYPE_INPAGE              Allocate a memory block that does not
|||	                                cross page boundaries.
|||
|||	    MEMTYPE_STARTPAGE           Allocate a memory block that starts on
|||	                                a page boundary.
|||
|||	    MEMTYPE_MYPOOL              Specifies that the memory block must
|||	                                be allocated only from the task's free
|||	                                memory pool.  This means that if there
|||	                                is not sufficient memory in the task's
|||	                                pool, the Kernel must not allocate
|||	                                additional memory from the system-wide
|||	                                free memory pool (see `Notes').
|||
|||	    MEMTYPE_SYSTEMPAGESIZE      Use the system's protection page size
|||	                                for that type of memory and not the
|||	                                special feature page size.  This is
|||	                                necessary for MEMTYPE_VRAM to
|||	                                distinguish between allocations on
|||	                                VRAM page boundaries for graphics
|||	                                operations (normally 2 K) and task
|||	                                page sizes (normally 16 K).
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to the memory block that was
|||	    allocated or NULL if the memory couldn't be allocated.
|||
|||	  Implementation
|||
|||	    Macro implemented in mem.h V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       C Macro Definition
|||
|||	  Notes
|||
|||	    Use FreeMem() to free a block of memory that was allocated with
|||	    AllocMem().
|||
|||	    A memory block allocated with AllocMem() will never overwrite the
|||	    stack.
|||
|||	    If there is insufficient memory in a task's free memory pool to
|||	    allocate the requested memory, the Kernel automatically transfers
|||	    the necessary pages of additional memory from the system-wide free
|||	    memory pool to the task's free memory pool.  The only exceptions
|||	    are (1) when there is not enough memory in both pools together to
|||	    satisfy the request, or (2) when the MEMTYPE_MYPOOL memory flag --
|||	    which specifies that the memory block must be allocated only from
|||	    the task's free memory pool -- is set.
|||
|||	    You can enable memory debugging in your application by
|||	    compiling your entire project with the MEMDEBUG value
|||	    defined on the compiler's command-line. Refer to the
|||	    CreateMemDebug() function for more details.
|||
|||	  See Also
|||
|||	    AllocMemBlocks(), AllocMemFromMemList(), AllocMemList(),
|||	    ControlMem(), FreeMem(), FreeMemList(), FreeMemToMemList(),
|||	    malloc(), ScavengeMem(), USER_ALLOCMEM(), USER_FREEMEM()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/allocmemfrommemlists
|||	AllocMemFromMemLists - Allocate a memory block from a list of memory lists.
|||
|||	  Synopsis
|||
|||	    void *AllocMemFromMemLists( List *l, int32 size, uint32 memflags)
|||
|||	  Description
|||
|||	    This procedure allocates a memory block of the specified type from
|||	    a list of free memory lists.  Although it used most often by the
|||	    Kernel, it can also be called by user tasks that need to do their
|||	    own memory management.
|||
|||	    Note:  Most applications do not need to do their own memory
|||	    management.  When you use standard memory-allocation procedures
|||	    like AllocMem(), the details of memory management are handled for
|||	    you by the Kernel.
|||
|||	    The `free memory pools' in Portfolio are implemented as lists of
|||	    memory lists:
|||
|||	    *   The system-wide free memory pool is the list of memory lists
|||	        that contain the free memory pages owned by the system.
|||
|||	    *   A task's free memory pool is a list of memory lists that have
|||	        been allocated for the task.  These include (1) the two memory
|||	        lists -- one for VRAM and one for DRAM -- that are allocated
|||	        automatically for a task when the task is created.
|||
|||	    AllocMemFromMemLists() allocates memory directly from a particular
|||	    memory pool.  This is in contrast to AllocMemFromMemList(), which
|||	    allocates memory from a specific memory list, typically one that
|||	    was created by the task for doing its own memory management.
|||
|||	    Note:  Tasks can only allocate memory from memory lists that
|||	    belong to them.  This means that user tasks cannot use
|||	    AllocMemFromMemLists() to allocate memory directly from the
|||	    system-wide free memory pool, because the memory in that pool
|||	    belongs to the system.
|||
|||	    If a task requests more memory from its free memory pool than is
|||	    available, the Kernel automatically allocates the necessary
|||	    additional memory pages from the system-wide free memory pool if
|||	    they are available.  The task gets ownership of the additional
|||	    memory and, when the memory is later freed, it is added to the
|||	    task's free memory pool.
|||
|||	  Arguments
|||
|||	    l                           A pointer to the memory pool from
|||	                                which to allocate the block.
|||
|||	    size                        The size of the memory block to
|||	                                allocate, in bytes.
|||
|||	    memflags                    Flags that specify the type of memory
|||	                                to allocate.  These flags can include
|||	                                MEMTYPE_ANY, MEMTYPE_VRAM,
|||	                                MEMTYPE_DRAM, MEMTYPE_BANKSELECT,
|||	                                MEMTYPE_BANK1, MEMTYPE_BANK2,
|||	                                MEMTYPE_DMA, MEMTYPE_CEL,
|||	                                MEMTYPE_AUDIO, MEMTYPE_DSP,
|||	                                MEMTYPE_FILL, MEMTYPE_INPAGE,
|||	                                MEMTYPE_STARTPAGE, and MEMTYPE_MYPOOL
|||	                                .  For information about these flags,
|||	                                see the description of AllocMem().
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to memory block that was allocated
|||	    or NULL if the memory couldn't be allocated.
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
|||	    To free a memory block allocated with AllocMemFromMemLists(), use
|||	    the FreeMemToMemLists() procedure.
|||
|||	    You can enable memory debugging in your application by
|||	    compiling your entire project with the MEMDEBUG value
|||	    defined on the compiler's command-line. Refer to the
|||	    CreateMemDebug() function for more details.
|||
|||	  See Also
|||
|||	    FreeMemToMemLists()
|||
**/

void *
AllocMemFromMemLists (List *l, int32 usersize, uint32 flags)
{
	MemList                *ml;
	int32                  *p;
	uint32                  ptypes;
	uint32                  lcltype;
	int32                   reqsize;

	reqsize = usersize;
	if (reqsize <= 0)
	    return NULL;

        if (flags & MEMTYPE_TRACKSIZE)
        {
            if ((flags & MEMTYPE_STARTPAGE) == 0)
                reqsize += sizeof(int32);
        }

	ROUND_ALLOC_SIZE(reqsize);

	p = 0;				/* memlist may be empty ... */

	/*
	 * Search through the memlists for one that can give us the
	 * right kind of memory.
	 */

	lcltype = flags & TYPES_OF_MEM;

	for (ml = (MemList *) FIRSTNODE (l);
	     ISNODE (l, ml);
	     ml = (MemList *) NEXTNODE (ml))
	{
		/*
		 * If this list does not provide the right kind of
		 * memory, ignore it.
		 */

		if ((lcltype & ml->meml_Types) != lcltype)
			continue;

		/* Try to allocate from the free list. */

		p = (int32 *) internalAllocMemFromMemList (ml, reqsize, usersize, flags);
		if (p)
			break;

		/*
		 * If the MEMTYPE_MYPOOL option is turned on, do
		 * nothing more than checking free lists.
		 */

		if (flags & MEMTYPE_MYPOOL)
			continue;

		/*
		 * We are about to try to get additional pages. Before
		 * allocating them, return any pages to the system that
		 * we can.
		 */

		ScavengeMem ();

		/*
		 * We need a page of memory... and it can't just be any
		 * kind of memory, it has to be the kind that goes in
		 * this list. And, if we are in user mode, it has to be
		 * memory from a header that we have a memory list for.
		 */

		ptypes = ml->meml_Types & TYPES_OF_MEM;

		if (isUser () == 0)
		{
			p = (int32 *) internalAllocMemBlocks (reqsize, ptypes);
		}
		else
		{
			ptypes |= MEMTYPE_TASKMEM;
			p = (int32 *) AllocMemBlocks (reqsize, ptypes);
		}

		/*
		 * If we got some pages, put them on the proper free
		 * lists. Since we asked nicely, this should leave them
		 * on the current list's free list, but allowing for a
		 * graceful failure is easy.
		 */

		if (p)
		{
			FreeMemToMemList (ml, (void *) p, *p);
			p = (int32 *) internalAllocMemFromMemList (ml, reqsize, usersize, flags);
			if (p)
				break;	/* should never fail. */
		}

		/*
		 * If we are in user mode, and we still could not get
		 * memory, ask the system to scavenge its memory lists
		 * and try again.
		 */

		if (isUser () == 1)
		{
			SystemScavengeMem ();
			p = (int32 *) AllocMemBlocks (reqsize, ptypes);
			if (!p)
				continue;
			FreeMemToMemList (ml, (void *) p, *p);
			p = (int32 *) internalAllocMemFromMemList (ml, reqsize, usersize,flags);
			if (p)
				break;	/* should never fail. */
		}
	}

	return p;
}

/**
|||	AUTODOC PUBLIC spg/kernel/getmemtracksize
|||	GetMemTrackSize - Get the size of a block of memory allocated with
|||	                  MEMTYPE_TRACKSIZE.
|||
|||	  Synopsis
|||
|||	    int32 GetMemTrackSize( const void *p )
|||
|||	  Description
|||
|||	    This function returns the size that was used to allocate a
|||	    block of memory. The block of memory must have been allocated
|||	    using the MEMTYPE_TRACKSIZE flag, otherwise this function will
|||	    return garbage.
|||
|||	  Arguments
|||
|||	    p                           Pointer obtained from a system
|||	                                memory allocation routine. The
|||	                                block of memory must have been
|||	                                allocated using the MEMTYPE_TRACKSIZE
|||	                                flag, otherwise the value returned
|||	                                by this function will be random.
|||
|||	  Return Value
|||
|||	    The function returns the size, in bytes, of the memory block.
|||	    This size corresponds to the size provided to the system memory
|||	    allocation routine when the block was first allocated.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    mem.h                       ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveats
|||
|||	    This function will not return the correct value for allocations
|||	    made using MEMTYPE_STARTPAGE and using a private MemList
|||	    structure. Allocations done with AllocMem() will always work,
|||	    since they use the task's memory lists and not any private
|||	    MemList.
|||
|||	  See Also
|||
|||	    AllocMem(), FreeMem()
|||
**/

int32 GetMemTrackSize(const void *p)
{
int32    *size;
MemList  *ml;
MemTrack *mt;

    if ((uint32)p & MEM_ALLOC_ALIGN_MASK)
    {
        size = (int32 *)((uint32)p - sizeof(int32));
        return *size;
    }

    ScanList(CURRENTTASK->t_FreeMemoryLists,ml,MemList)
    {
        if (ml->meml_Sema4)
            LockSemaphore(ml->meml_Sema4,SEM_WAIT);

        ScanList(ml->meml_AlignedTrackSize,mt,MemTrack)
        {
            if (mt->mt_Addr == p)
            {
                if (ml->meml_Sema4)
                    UnlockSemaphore(ml->meml_Sema4);

                return mt->mt_Size;
            }
        }

        if (ml->meml_Sema4)
            UnlockSemaphore(ml->meml_Sema4);
    }

    return 0;
}


/*****************************************************************************/


/*
 * SystemScavengeMem gets called via SWI,
 * to scavenge supervisor memory
 */
int32
externalSystemScavengeMem (void)
{
	return ScavengeMem ();
}

/**
|||	AUTODOC PUBLIC spg/kernel/scavengemem
|||	ScavengeMem - Return task's free memory pages to the system memory  pool.
|||
|||	  Synopsis
|||
|||	    int32 ScavengeMem( void )
|||
|||	  Description
|||
|||	    This procedure finds pages of memory in the task's free memory
|||	    pool from which no memory is allocated and returns those pages to
|||	    the system-wide memory pool.
|||
|||	    Tasks should always call ScavengeMem() when they receive a
|||	    SIGF_MEMLOW signal from the Kernel.  The Kernel sends this signal
|||	    when very little memory is available in the system-wide memory
|||	    pool.  Although tasks should normally wait for a SIGF_MEMLOW
|||	    signal before calling ScavengeMem(), because it is often
|||	    expensive, they can call ScavengeMem() at other times if they want
|||	    to make unused memory available to other tasks.
|||
|||	    If there is not enough memory in a task's free-memory pool to
|||	    satisfy a memory-allocation request, AllocMem() tries to get
|||	    reclaim the necessary memory by calling ScavengeMem().
|||
|||	  Return Value
|||
|||	    The procedure returns the amount of memory that was returned to
|||	    the system-wide memory pool, in bytes.  If no memory was returned,
|||	    the procedure returns 0.
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
|||	  See Also
|||
|||	    AllocMem(), AllocMemFromMemLists(), FreeMem(), FreeMemToMemLists()
|||
**/

int32
ScavengeMem (void)
{
	char                   *p;
	List                   *FreeMemLists;
	int32                   pagesize;
	int32                   ret = 0, i;
	uint32                  flags;


	/* Go through Task t's free memlist and free any unused blocks */
	/* back to the system */

	if (isUser () == 0)
		FreeMemLists = KernelBase->kb_MemFreeLists;
	else if (KernelBase->kb_CurrentTask)
		FreeMemLists = KernelBase->kb_CurrentTask->t_FreeMemoryLists;
	else
		return 0;		/* nothing to scavenge from */

	for (i = 0; i < 2; i++)
	{
		flags = (i == 0) ? MEMTYPE_DRAM : MEMTYPE_VRAM | MEMTYPE_SYSTEMPAGESIZE;
		pagesize = GetPageSize (flags);

		if (pagesize == 0)
		{
#ifdef DEVELOPMENT
			kprintf ("SC internal error, GetPageSize is 0\n");
			while (1);
#else
			if (isUser () == 0)
				Panic (1, "SC internal error, GetPageSize is 0\n");
#endif
		}
		do
		{
			p = (char *) AllocMemFromMemLists
				(FreeMemLists, pagesize,
				 (uint32) (MEMTYPE_STARTPAGE |
					   MEMTYPE_MYPOOL |
					   MEMTYPE_INPAGE |
					   flags));
			if (p == 0)
				break;	/* done with this list */
			ret += pagesize;
			if (isUser () == 0)
				internalControlMem (0, p, 4, MEMC_SC_GIVE, 0);
			else
				ControlMem ((void *) p, 4, MEMC_GIVE, 0);
		} while (p);
	}
	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/malloc
|||	malloc - Allocate memory.
|||
|||	  Synopsis
|||
|||	    void *malloc( int32 size )
|||
|||	  Description
|||
|||	    This procedure allocates a memory block.  It is identical to the
|||	    malloc() procedure in the standard C library.  The memory is
|||	    guaranteed only to be memory that is accessible to the CPU; you
|||	    cannot specify the memory type (such as VRAM), alignment (such as
|||	    a memory block that begins on a page boundary), or any other
|||	    memory characteristics.
|||
|||	    Note:  You should only use malloc() when porting existing C
|||	    programs to Portfolio.  If you are writing programs specifically
|||	    for Portfolio, use AllocMem(), which allows you to specify the
|||	    type of memory to allocate, such as VRAM or memory that begins on
|||	    a page boundary.
|||
|||	  Arguments
|||
|||	    size                        Size of the memory block to allocate,
|||	                                in bytes.
|||
|||	  Return Value
|||
|||	    The procedure returns a pointer to the memory that was allocated
|||	    or NULL if the memory couldn't be allocated.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    stdlib.h                    ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by
|||	    compiling your entire project with the MEMDEBUG value
|||	    defined on the compiler's command-line. Refer to the
|||	    CreateMemDebug() function for more details.
|||
|||	  See Also
|||
|||	    AllocMem(), availMem(), free()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/free
|||	free - Free memory from malloc().
|||
|||	  Synopsis
|||
|||	    void free( void *p )
|||
|||	  Description
|||
|||	    The procedure frees a memory block that was allocated by a call to
|||	    malloc().  It is identical to the free() procedure in standard C
|||	    library.  The freed memory is automatically returned to the memory
|||	    list from which is was allocated.
|||
|||	    Note:  You should only use malloc() and free() when porting
|||	    existing C programs to Portfolio.  If you are writing programs
|||	    specifically for Portfolio, use AllocMem() and FreeMem(), which
|||	    allow you to specify the type of memory to allocate, such as VRAM
|||	    or memory that begins on a page boundary.
|||
|||	  Arguments
|||
|||	    p                           A pointer to the memory to be freed.
|||	                                This pointer may be NULL, in which case
|||	                                this function just returns.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    stdlib.h                    ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    You can enable memory debugging in your application by
|||	    compiling your entire project with the MEMDEBUG value
|||	    defined on the compiler's command-line. Refer to the
|||	    CreateMemDebug() function for more details.
|||
|||	  Caveats
|||
|||	    Trusts that p is a result of a previous malloc() call.
|||
|||	  See Also
|||
|||	    FreeMem(), FreeMemToMemList(), FreeMemToMemLists(), malloc(),
|||	    USER_FREEMEM()
|||
**/

void
InitVMEM (int32 memsize)
{
void *p;

#ifdef	TRASHMEM
    trashflag = 0;
#endif

    /* The upper 4K of either banks of VRAM cannot be used for VRAM purposes
     * due to Opera HW problems. To avoid all confusion, these are just
     * permanently taken out of the memory list here. If you're clever,
     * maybe you could find a way to make this memory usable by the kernel
     * to store other things, while still preventing VRAM data to be stored
     * there.
     */

    p = AllocMemFromMemLists(KernelBase->kb_MemFreeLists,4096,MEMTYPE_VRAM_BANK1 | MEMTYPE_FROMTOP);
    p = AllocMemFromMemLists(KernelBase->kb_MemFreeLists,4096,MEMTYPE_VRAM_BANK2 | MEMTYPE_FROMTOP);
    if (GETBANKBITS(p) & MEMTYPE_BANK1)
    {
        /* This'll happen if you ask for VRAM from bank 2 when there
         * isn't any bank 2...
         */
        FreeMemToMemLists(KernelBase->kb_MemFreeLists,p,4096);
    }

#ifdef	TRASHMEM
    trashflag = 1;
#endif
}
