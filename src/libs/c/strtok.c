/* $Id: strtok.c,v 1.1 1994/09/06 16:56:56 vertex Exp $ */

#include "types.h"
#include "string.h"


/*****************************************************************************/


char *strtok(char *str, const char *seps)
{
static char *last;
char *ptr;

    if (str == NULL)
    {
        /* pick up where we left off... */
	str = last;
	if (str == NULL)
	    return NULL;
    }

    /* skip separator characters */
    while (*str && strchr(seps, *str))
	str++;

    /* no more tokens */
    if (*str == 0)
	return NULL;

    ptr = strpbrk(str, seps);
    if (ptr)
	*ptr++ = 0;

    last = ptr;

    return str;
}
