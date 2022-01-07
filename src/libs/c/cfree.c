/* $Id: cfree.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
/* file: malloc.c */

#include "types.h"
#include "mem.h"

int
cfree(ptr)
char *ptr;
{
	free((void *)ptr);
	return 0;
}
