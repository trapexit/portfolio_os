/* $Id: CountBits.c,v 1.7 1994/09/10 02:52:22 vertex Exp $ */

#include "types.h"

/**
|||	AUTODOC PUBLIC spg/clib/countbits
|||	CountBits: count the number of bits set in a word.
|||
|||	  Synopsis
|||
|||	    int CountBits(uint32 mask);
|||
|||	  Description
|||
|||	    CountBits examines the longword passed in as a parameter and
|||	    returns the number of bits that are turned on in the word, using
|||	    an algorithm derived from a note in MIT's "HAKMEM".
|||
|||	  Arguments
|||
|||	    mask                        The data word to count the bits of.
|||
|||	  Return Value
|||
|||	    The number of bits that were turned on in the supplied value.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V21.
|||
|||	  Associated Files
|||
|||	    string.h                    ANSI C Prototype
|||
|||	    clib.lib                    ARM Link library
|||
|||	  See Also
|||
|||	    FindMSB(), FindLSB()
**/

/*
 * Algorithm derived from a note in HAKMEM, and reproduced here from
 * memory; cleverness is the success of [someone?] at MIT, any
 * problems are the fault of my memory-seive.
 *
 * The basis for this algorithm is a "partitioned add" where, at any
 * time, each [aligned] group of 32/n bits contains an integer in the
 * range [0..n] representing the number of bits set in the
 * corresponding bitfield in the original word. Thus, the first line
 * adds all the odd bits and all the even bits together to make bit
 * pairs; then the second adds those bit pairs to make nibbles, and
 * so on. The number of steps total is proportional to the log of the
 * size of the original word in bits, and the time taken is constant.
 *
 * I've trimmed the final constant down to eliminate one ARM
 * instruction; we know that the values being added in the final step
 * are in the range [0..16]; along the same lines (to highlight the
 * ranges involved) I've trimmed the other constants where possible.
 */

int
CountBits (uint32 word)
{
	uint32		mask;
	mask = 0x55555555;
	word = (mask & word) + (mask & (word >> 1));
	mask = 0x33333333;
	word = (mask & word) + (mask & (word >> 2));
	mask = 0x07070707;
	word = (mask & word) + (mask & (word >> 4));
	mask = 0x000F000F;
	word = (mask & word) + (mask & (word >> 8));
	mask = 0x0000001F;
	return (int) ((mask & word) + (word >> 16));
}
