/* $Id: bigmalloc.c,v 1.5 1994/05/03 16:49:57 limes Exp $ */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <operror.h>

/*
 * This program serves two purposes.
 * First, to validate that "very large"
 * allocations do not kill the system;
 * second, to report how much total
 * memory was allocated (and the size of
 * the blocks allocated).
 */

int
main ()
{
    uint32                  tsize;
    uint32                  asize;
    uint32                  tbit;
    char                   *p;

    tsize = 0;
    printf ("bigmalloc: %8s %7s %7s\n", "blk", "bsz", "tsz");
    while (1)
    {
	asize = 0;
	for (tbit = 1 << 30; tbit > 0; tbit >>= 1)
	{
	    if ((p = (char *) AllocMem (asize + tbit, 0)) != (char *) 0)
	    {
		FreeMem (p, asize + tbit);
		asize += tbit;
	    }
	}
	while ((asize > 0) && (AllocMem (asize, 0) == 0))
	    asize--;
	if (asize < 1)
	    break;
	tsize += asize;
	printf ("bigmalloc: 0x%06lx %7d %7d\n",
		p, asize, tsize);
    }

    return 0;
}
