/* $Id: strrchr.c,v 1.2 1994/09/06 17:20:23 vertex Exp $ */

#include "types.h"
#include "string.h"


/*****************************************************************************/


char *strrchr(const char *str, int searchChar)
{
const char *ptr;

    ptr = str;
    while (*ptr)
        ptr++;

    while (TRUE)
    {
        if (*ptr == searchChar)
            return (char *)ptr;

        if (ptr == str)
            return NULL;

        ptr--;
    }
}
