/* $Id: converthalfkana2fullkana.c,v 1.1 1994/11/04 18:39:25 vertex Exp $ */

#include "types.h"
#include "jstring_lib.h"


/****************************************************************************/


int32 ConvertHalfKana2FullKana(const char *string, char *result,
                               uint32 resultSize, uint8 filler)
{
int32 ret;

    CallFolioRet(JStringBase, CONVERTHALFKANA2FULLKANA, (string,result,resultSize,filler), ret, (int32));

    return ret;
}
