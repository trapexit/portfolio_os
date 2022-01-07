/* $Id: formatdate.c,v 1.1 1994/04/25 23:22:52 vertex Exp $ */

#include "types.h"
#include "international_lib.h"


/****************************************************************************/


int32 intlFormatDate(Item locItem, DateSpec spec,
                     const GregorianDate *date,
                     unichar *result, uint32 resultSize)
{
int32 ret;

    CallFolioRet(InternationalBase, INTLFORMATDATE, (locItem,spec,date,result,resultSize), ret, (int32));

    return ret;
}
