/* $Id: getcharattrs.c,v 1.1 1994/04/25 23:22:52 vertex Exp $ */

#include "types.h"
#include "international_lib.h"


/****************************************************************************/


uint32 intlGetCharAttrs(Item locItem, unichar character)
{
uint32 ret;

    CallFolioRet(InternationalBase, INTLGETCHARATTRS, (locItem,character), ret, (uint32));

    return ret;
}
