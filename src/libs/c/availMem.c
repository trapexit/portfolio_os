/*	$Id: availMem.c,v 1.12 1994/12/09 23:01:41 vertex Exp $
**
**	Return information on available memory (struct MemInfo).
**
**	Copyright 1993 by The 3DO Company Inc.
*/
#include "types.h"
#include "mem.h"

#define	D(x)	;

/**
|||	AUTODOC PUBLIC spg/kernel/availmem
|||	AvailMem - Get information about available memory.
|||
|||	  Synopsis
|||
|||	    void AvailMem(MemInfo *minfo, uint32 memflags);
|||
|||	  Description
|||
|||	    This procedure returns information about the amount of memory that
|||	    is currently available.  You can get information about a
|||	    particular kind of memory by setting the corresponding flags, such
|||	    as MEMTYPE_VRAM or MEMTYPE_DRAM, in the flags argument.  To get
|||	    information about all memory that is available to the CPU, use
|||	    MEMTYPE_ANY as the value of the flags argument.
|||
|||	    The information about available memory is returned in a MemInfo
|||	    structure:
|||
|||	    Example 15-1                Memory Information Structure for
|||	                                AvailMem()
|||
|||	    typedef struct MemInfo
|||
|||	    {
|||
|||	                                uint32 minfo_SysFree;
|||
|||	                                uint32 minfo_SysLargest;
|||
|||	                                uint32 minfo_TaskFree;
|||
|||	                                uint32 minfo_TaskLargest;
|||
|||	    }                                   MemInfo;
|||
|||
|||	    The fields contain the following information:
|||
|||	    minfo_SysFree               The amount of memory of the specified
|||	                                memory type in the system-wide free
|||	                                memory pool, in bytes.  The pool
|||	                                contains only full pages of memory.
|||
|||	    minfo_SysLargest            The size, in bytes, of the largest
|||	                                series of contiguous pages of the
|||	                                specified memory type in the
|||	                                system-wide free memory pool.
|||
|||	    minfo_TaskFree              The amount of memory of the specified
|||	                                type in the task's free memory pool,
|||	                                in bytes.
|||
|||	    minfo_TaskLargest           The size, in bytes, of the largest
|||	                                contiguous block of the specified
|||	                                memory type that can be allocated from
|||	                                the task's free memory pool.
|||
|||	  Arguments
|||
|||	    minfo                       A pointer to the MemInfo structure
|||	                                used to return the result.
|||
|||	    memflags                    Flags that specify the type of memory
|||	                                to get information about.  These flags
|||	                                can include MEMTYPE_ANY, MEMTYPE_VRAM,
|||	                                MEMTYPE_DRAM, MEMTYPE_BANKSELECT,
|||	                                MEMTYPE_BANK1, MEMTYPE_BANK2,
|||	                                MEMTYPE_DMA, MEMTYPE_CEL,
|||	                                MEMTYPE_AUDIO, and MEMTYPE_DSP.  For
|||	                                information about these flags, see the
|||	                                description of AllocMem().
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       ANSI C Macro
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Caveats
|||
|||	    When you call AvailMem(), you must request information about only
|||	    one memory type.  Attempting to find out about more than one
|||	    memory type may produce unexpected results.
|||
|||	    If you pass in a garbage minfo pointer, sparks may fly.
|||
|||	    The information returned by AvailMem() is inherently flawed,
|||	    since you are existing in a multitasking environment. Memory
|||	    can be allocated or freed asynchronous to the operation of the
|||	    task calling AvailMem().
|||
|||	  See Also
|||
|||	    AllocMem(), free(), FreeMem(), malloc()
|||
**/

void
availMem( MemInfo *minfo, uint32 flags )
{
uint32	liveflags;

uint32	size;
uint32	largest;

List	*l;
MemList	*ml;


    liveflags = flags &
	 (MEMTYPE_VRAM_BANK1 | MEMTYPE_VRAM_BANK2 |MEMTYPE_DMA  |MEMTYPE_CEL|
	  MEMTYPE_DRAM|MEMTYPE_AUDIO|MEMTYPE_DSP);

    l = KernelBase->kb_MemHdrList;	/* Safe only if noone adds RAM!! */
    {
	int32 ones;			/* One bits this pass */
	int32 runs;			/* Run of continuously free pages */
	int32 longest_run;
	MemHdr *m;

	size = 0;
	largest = 0;
	longest_run = 0;

	for(m=(MemHdr *)FIRSTNODE(l);ISNODE(l,m); m = (MemHdr *)NEXTNODE(m))
	{
	    ulong *p	= m->memh_FreePageBits;
	    int bits	= m->memh_FreePageBitsSize;
	    ulong v;	/* Longword temporary */

       	    if ( (liveflags & m->memh_Types) != liveflags) {
		continue;
		}

	    runs = 0;
	    ones = 0;	/* Count 1 bits in this memheader */
	    while (bits--)
	    {
		D(printf("\nBits %d/m $%lx\n", bits, m));
		v = *p++;	/* 32 bits at a time */

		while( v )
		{
		    if (v & 1) {
			ones++;
			runs++;
		    } else {
		        if( runs > longest_run ) {
				longest_run = runs;
	        		largest = (longest_run*m->memh_PageSize);
				}
			runs = 0;
		    }
		    v >>= 1;
		}
	    }

	    if( runs > longest_run ) {
		longest_run = runs;
	        largest = (longest_run*m->memh_PageSize);
		}

            size   += (ones*m->memh_PageSize);

	    D(debug("Runs %d/longest %d/Ones %d/Size +=%d\n",
		   runs, longest_run, ones, (ones*m->memh_PageSize) ));
	}
        minfo->minfo_SysFree    = size;
        minfo->minfo_SysLargest = largest;
    }


    l = KernelBase->kb_CurrentTask->t_FreeMemoryLists;
    largest	= 0;
    size	= 0;
    for(ml=(MemList *)FIRSTNODE(l); ISNODE(l,ml); ml=(MemList *)NEXTNODE(ml))
    {
	D(debug("MemList %s @%lx. Types %lx\n",
			ml->meml_n.n_Name, (uint32)ml, ml->meml_Types));

        if ( (liveflags & ml->meml_Types) != liveflags )
		continue;	/* Don't pass go, don't collect $200... */

	    {
	    List *mn = ml->meml_l;	/* Free nodes */
	    Node *n;
            if (ml->meml_Sema4) LockItem(ml->meml_Sema4,TRUE);
		for( n = FIRSTNODE(mn); ISNODE(mn,n); n = NEXTNODE(n) )
		{
		    D(debug("Node %lx, size %d\n", n, n->n_Size));
		    if (largest < n->n_Size)
			largest = n->n_Size;
		    size += n->n_Size;
		}
	    if (ml->meml_Sema4) UnlockItem(ml->meml_Sema4);
	    }
    }
    minfo->minfo_TaskFree	= size;
    minfo->minfo_TaskLargest	= largest;
}

