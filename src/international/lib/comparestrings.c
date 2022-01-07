/* $Id: comparestrings.c,v 1.1 1994/04/25 23:22:52 vertex Exp $ */

#include "types.h"
#include "international_lib.h"


/****************************************************************************/


int32 intlCompareStrings(Item locItem, const unichar *string1, const unichar *string2)
{
int32 ret;

    CallFolioRet(InternationalBase, INTLCOMPARESTRINGS, (locItem,string1,string2), ret, (int32));

    return ret;
}
