/* $Id: typeofmem.c,v 1.13 1994/09/10 02:52:22 vertex Exp $ */
/* file: typeofmem.c */

#include "types.h"
#include "mem.h"
#include "kernel.h"

extern MemHdr *FindMH(char *);

/**
|||	AUTODOC PUBLIC spg/kernel/getbankbits
|||	GetBankBits - Find out which VRAM bank contains a memory location.
|||
|||	  Synopsis
|||
|||	    uint32 GetBankBits( void *a )
|||
|||	  Description
|||
|||	    In the current 3DO hardware, 2 MB of video random-access memory
|||	    (VRAM) is divided into two banks, each containing 1MB of VRAM.
|||	    Fast VRAM memory transfers using the SPORT bus can take place only
|||	    between memory locations in the same bank.  You use this procedure
|||	    to find out which VRAM bank contains a specified memory location.
|||
|||	    When a task allocates VRAM, the memory block contains memory from
|||	    one VRAM bank or the other; it never contains memory from both
|||	    banks.  Thus, if any memory location in a block is in one VRAM
|||	    bank, the entire block is guaranteed to be in the same bank.
|||
|||	  Arguments
|||
|||	    a                           A pointer to the memory to be
|||	                                examined.
|||
|||	  Return Value
|||
|||	    The procedure returns a set of memory flags in which the following
|||	    flags can be set:
|||
|||	    MEMTYPE_BANKSELECT          Set if the memory is contained in
|||	                                VRAM.
|||
|||	    MEMTYPE_BANK1               Set if the memory is in the primary
|||	                                VRAM bank.
|||
|||	    MEMTYPE_BANK2               Set if the memory is in the secondary
|||	                                VRAM bank.
|||
|||	    If the memory is not in VRAM, the procedure returns 0.
|||
|||	  Implementation
|||
|||	    Macro implemented in mem.h V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       ANSI C Macro definition
|||
|||	  See Also
|||
|||	    GetMemType()
|||
**/

/**
|||	AUTODOC PUBLIC spg/kernel/getmemtype
|||	GetMemType - Get the type of the specified memory.
|||
|||	  Synopsis
|||
|||	    uint32 GetMemType( void *p )
|||
|||	  Description
|||
|||	    The procedure returns flags that describe the type of memory at a
|||	    specified memory location.
|||
|||	  Arguments
|||
|||	    p                           A pointer to the memory whose type to
|||	                                return.
|||
|||	  Return Value
|||
|||	    The procedure returns a set of flags that specify the type of
|||	    memory.  For information about these flags, see the description of
|||	    AllocMem().
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    mem.h                       ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    AllocMem()
|||
**/

uint32
GetMemType(void * p)
{
	MemHdr *mh;
	mh = FindMH((char *)p);
	if (mh == 0)	return 0;
	return mh->memh_Types;
}
