/* $Id: convertshiftjis2ascii.c,v 1.1 1994/08/24 20:22:54 vertex Exp $ */

#include "types.h"
#include "jstring_lib.h"


/****************************************************************************/


int32 ConvertShiftJIS2ASCII(const char *string, char *result,
                            uint32 resultSize, uint8 filler)
{
int32 ret;

    CallFolioRet(JStringBase, CONVERTSHIFTJIS2ASCII, (string,result,resultSize,filler), ret, (int32));

    return ret;
}
