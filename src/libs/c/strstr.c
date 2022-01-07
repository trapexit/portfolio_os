/* $Id: strstr.c,v 1.1 1994/09/06 16:57:22 vertex Exp $ */

#include "types.h"
#include "string.h"


/*****************************************************************************/


char *strstr(const char *str, const char *subString)
{
const char *s;
const char *sub;

    if (*subString == 0)
        return (char *)str;

    while (*str)
    {
        s   = str;
        sub = subString;
        while (*sub && (*s == *sub))
        {
            s++;
            sub++;
        }

        if (*sub == 0)
            return (char *)str;

	str++;
    }

    return NULL;
}
