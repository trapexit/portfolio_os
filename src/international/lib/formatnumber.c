/* $Id: formatnumber.c,v 1.2 1994/11/05 00:48:40 vertex Exp $ */

#include "types.h"
#include "international_lib.h"


/****************************************************************************/


int32 intlFormatNumber(Item locItem, const NumericSpec *spec,
                       uint32 whole, uint32 frac, bool negative, bool doFrac,
                       unichar *result, uint32 resultSize)
{
int32 ret;

    CallFolioRet(InternationalBase, INTLFORMATNUMBER, (locItem,spec,whole,frac,negative,doFrac,result,resultSize), ret, (int32));

    return ret;
}
