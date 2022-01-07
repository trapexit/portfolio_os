/* $Id: convertascii2shiftjis.c,v 1.1 1994/08/24 20:22:54 vertex Exp $ */

#include "types.h"
#include "jstring_lib.h"


/****************************************************************************/


int32 ConvertASCII2ShiftJIS(const char *string, char *result,
                            uint32 resultSize, uint8 filler)
{
int32 ret;

    CallFolioRet(JStringBase, CONVERTASCII2SHIFTJIS, (string,result,resultSize,filler), ret, (int32));

    return ret;
}
