/* $Id: strcspn.c,v 1.1 1994/09/06 17:20:40 vertex Exp $ */

#include "types.h"
#include "string.h"


/*****************************************************************************/


size_t strcspn(const char *str, const char *set)
{
char *ptr;

    ptr = strpbrk(str, set);
    if (ptr)
	return (ptr - str);

    return strlen(str);
}
