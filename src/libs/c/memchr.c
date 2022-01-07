/* $Id: memchr.c,v 1.1 1994/09/06 17:48:10 vertex Exp $ */

#include "types.h"
#include "string.h"


/*****************************************************************************/


void *memchr(const void *mem, int searchChar, size_t numBytes)
{
const char *p = (char *)mem;

    while (numBytes--)
    {
        if (*p == searchChar)
            return (void *)p;

        p++;
    }

    return NULL;
}
