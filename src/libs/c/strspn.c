/* $Id: strspn.c,v 1.1 1994/09/06 17:26:49 vertex Exp $ */

#include "types.h"
#include "string.h"


/*****************************************************************************/


size_t strspn(const char *str, const char *set)
{
const char *ptr;

    ptr = str;
    while (*ptr)
    {
        if (strchr(set,*ptr) == NULL)
            break;

        ptr++;
    }

    return (ptr - str);
}
