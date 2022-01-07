/* $Id: atoi.c,v 1.3 1994/04/06 16:49:58 sdas Exp $ */
#include "types.h"
#include "stdlib.h"

int32
atoi(const char *nsptr)
{
	return (int) strtol(nsptr, (char **)0, 0);
}

