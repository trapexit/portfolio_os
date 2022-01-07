/* $Id: calloc.c,v 1.5 1994/03/18 23:34:07 sdas Exp $ */
/* file: calloc.c */

#include "types.h"
#include "stdlib.h"
#include "strings.h"

void *
calloc(nelem, elsize)
size_t nelem, elsize;
{
	size_t size = nelem*elsize;
	void *p = malloc((int32)size);
	if (p)
	{
		memset(p,0,size);
	}
	return p;
}

