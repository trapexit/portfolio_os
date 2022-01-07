/* $Id: convertshiftjis2unicode.c,v 1.1 1994/08/24 20:22:54 vertex Exp $ */

#include "types.h"
#include "jstring_lib.h"


/****************************************************************************/


int32 ConvertShiftJIS2UniCode(const char *string, unichar *result,
                              uint32 resultSize, uint8 filler)
{
int32 ret;

    CallFolioRet(JStringBase, CONVERTSHIFTJIS2UNICODE, (string,result,resultSize,filler), ret, (int32));

    return ret;
}
