/* $Id: convertstring.c,v 1.1 1994/04/25 23:22:52 vertex Exp $ */

#include "types.h"
#include "international_lib.h"


/****************************************************************************/


int32 intlConvertString(Item locItem, const unichar *string, unichar *result,
                        uint32 resultSize, uint32 flags)
{
int32 ret;

    CallFolioRet(InternationalBase, INTLCONVERTSTRING, (locItem,string,result,resultSize,flags), ret, (int32));

    return ret;
}
