/* $Id: cplusruntime.c,v 1.2 1994/11/14 23:16:54 vertex Exp $ */

#include "mem.h"


/*****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *cpluslib_version;
static void *cplulib_version = &cpluslib_version;
#endif


/*****************************************************************************/


/* This is some magic code needed for the C++ support */


void *__nw__FUi(unsigned int x)
{
    if (x != 0L)
	return malloc(x);
    else
	return 0L;
}

void __dl__FPv(void *x)
{
    free(x);
}
