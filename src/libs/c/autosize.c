/* $Id: autosize.c,v 1.2 1994/10/25 21:49:58 limes Exp $
 *
 * Copyright (C) 1994 by The 3DO Company
 */

#include <types.h>

/**
|||	AUTODOC PRIVATE spg/clib/autosize
|||	AutoSize: calculate true size of RAM/NVRAM component
|||	
|||	  Synopsis
|||	    uint32 AutoSize(char *base, uint32 maxsize, uint32 offset)
|||
|||	  Description
|||
|||	    This function is used to determine the precise size of a RAM-like
|||	    hardware element that is a power-of-two in size and aliases on
|||	    multiples of its size within the assigned address range.
|||
|||	    The algorithm is simple: look at power-of-two offsets from the
|||	    designated byte until one is found that is aliased with that byte;
|||	    verify aliasing by modifying the original byte and looking for the
|||	    new value in the offset location.
|||
|||	    "maxsize" should be a power of two, and "base" should be
|||	    aligned on a multiple of maxsize; "offset" should be
|||	    smaller than any anticipated part size. If any of these
|||	    conditions are violated by the caller, AutoSize will still
|||	    attempt to deduce the aliasing size as well as it can, but
|||	    results are not guaranteed to be correct.
|||	    
|||	    AutoSize guarantees that it will not access any addresses
|||	    outside the specified range, and that it will only modify
|||	    the address at the specified offset within the range (and
|||	    of course any aliased addresses that refer to the same
|||	    cell of storage).
|||
|||	  Arguments
|||	  
|||	    "base" is the base of the address range.
|||
|||	    "maxsize" is the size of the decoded address range.
|||
|||	    "offset" is the offset of the byte AutoSize can modify.
|||	  
|||	  Return Value
|||	  
|||	    AutoSize returns the (power-of-two) size in bytes of the
|||	    part as deduced by matching an alias of a byte, or the
|||	    maximum size of the address range if no aliasing is
|||	    detected.
|||
|||	  Implementation
|||	  
|||	    Convenience call implemented in clib.lib V24.
|||
|||	  Caveat
|||	  
|||	    We cannot guarantee that we can restore the value of this
|||	    byte (we may get a reset while our marker data is in the
|||	    byte) so we do not even try to restore the value.
|||
**/

uint32
AutoSize(char *base, uint32 maxsize, uint32 offset)
{
    uint32	trysize = 1;
    char	tryvalue = 0;

    maxsize -= offset;
    base += offset;
    while (trysize <= offset)
	trysize <<= 1;
    if (trysize < maxsize) {
	base[0] = tryvalue;
	do {
	    if (base[trysize] == tryvalue) {
		tryvalue ++;
		base[0] = tryvalue;
		if (base[trysize] == tryvalue) {
		    break;
		}
	    }
	} while ((trysize <<= 1) < maxsize);
    }
    return tryvalue;
}
