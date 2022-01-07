/* $Id: createNamedItem.c,v 1.3 1994/05/04 20:42:11 vertex Exp $ */
/* file createNamedItem.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"
#include "super.h"
#include "createNamedItem.h"


Item
SuperCreateNamedItemVA(int32 ctype, const char *name, uint8 pri, uint32 extraTags, ...)
{
TagArg tags[3];

    tags[0].ta_Tag = TAG_ITEM_PRI;
    tags[0].ta_Arg = (void *)pri;

    if (name)
    {
        tags[1].ta_Tag = TAG_ITEM_NAME;
        tags[1].ta_Arg = (void *)name;
    }
    else
    {
        tags[1].ta_Tag = TAG_NOP;
    }

    tags[2].ta_Tag = TAG_JUMP;
    tags[2].ta_Arg = (void *)&extraTags;

    return SuperCreateSizedItem(ctype,tags,0);
}
