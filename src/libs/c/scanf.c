/* $Id: scanf.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
#include "types.h"
#include "debug.h"

extern void exit(int);

int sscanf(void)
{
	kprintf("unsupported call to sscanf\n");
	exit(0);
	return 1;
}
