/* $Id: free.c,v 1.5 1994/03/18 23:37:32 sdas Exp $ */
/* file: free.c */

#include "types.h"
#include "stdlib.h"
#include "mem.h"

/*#define DEBUG*/

void
free(ptr)
void *ptr;
{
#ifdef DEBUG
extern void kprintf(const char *, ...);
	Task *ct = KernelBase->kb_CurrentTask;
	kprintf("free(%lx\n",(ulong)ptr);
	kprintf("CurrentTask=%lx\n",(ulong)ct);
	kprintf("memlist=%lx\n",ct->t_FreeMemoryLists);
#endif
	if (ptr)
	{
		long *p = (long *)ptr;
		p--;	/* back up to the hidden size */
		FREEMEM((void *)p,(int)*p);
	}
}
