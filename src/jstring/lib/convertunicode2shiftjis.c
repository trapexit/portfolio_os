/* $Id: convertunicode2shiftjis.c,v 1.1 1994/08/24 20:22:54 vertex Exp $ */

#include "types.h"
#include "jstring_lib.h"


/****************************************************************************/


int32 ConvertUniCode2ShiftJIS(const unichar *string, char *result,
                              uint32 resultSize, uint8 filler)
{
int32 ret;

    CallFolioRet(JStringBase, CONVERTUNICODE2SHIFTJIS, (string,result,resultSize,filler), ret, (int32));

    return ret;
}
