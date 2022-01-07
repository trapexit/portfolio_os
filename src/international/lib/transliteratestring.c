/* $Id: transliteratestring.c,v 1.1 1994/04/25 23:22:52 vertex Exp $ */

#include "types.h"
#include "international_lib.h"


/****************************************************************************/


int32 intlTransliterateString(const void *string, CharacterSets stringSet,
                              void *result, CharacterSets resultSet,
                              uint32 resultSize,
                              uint8 unknownFiller)
{
int32 ret;

    CallFolioRet(InternationalBase, INTLTRANSLITERATESTRING, (string,stringSet,result,resultSet,resultSize,unknownFiller), ret, (int32));

    return ret;
}
