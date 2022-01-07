/* $Id: utils.c,v 1.1 1994/05/06 22:48:27 vertex Exp $ */

#include "types.h"
#include "utils.h"


/*****************************************************************************/


/* convert a 32-bit number to UniCode numerals, return length */
uint32 ConvUnsigned(uint32 num, unichar *result)
{
char   flip[16];
uint8  i;
uint32 len;

    i = 0;
    do
    {
        flip[i++] = (char)(num % 10) + '0';
        num = num / 10;
    }
    while (num);

    len = i;

    do
    {
        i--;
        *result++ = flip[i];
    }
    while (i);

    *result = 0;

    return (len);
}
