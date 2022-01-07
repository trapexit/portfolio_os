/* $Id: strpbrk.c,v 1.1 1994/09/06 16:57:11 vertex Exp $ */

#include "types.h"
#include "string.h"


/*****************************************************************************/


char *strpbrk(const char *str, const char *breaks)
{
const char *ptr;

    while (*str)
    {
	ptr = breaks;
	while (*ptr)
        {
	    if (*ptr == *str)
		return (char *)str;

	    ptr++;
	}
	str++;
    }

    return NULL;
}
