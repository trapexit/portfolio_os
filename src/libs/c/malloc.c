/* $Id: malloc.c,v 1.5 1994/03/18 23:38:50 sdas Exp $ */
/* file: malloc.c */

#include "types.h"
#include "stdlib.h"
#include "mem.h"

void *
malloc(size)
long size;
{
	long *ret;
	ret = (long *)ALLOCMEM(size+4,MEMTYPE_ANY);	/* any type of memory */
	/* AllocMem stashes the realsize in the first long word of */
	/* this allocation, very convenient! */
	if (ret)	return (void *)(ret+1);
	else		return 0;
}
