/* $Id: createUniqueMsgPort.c,v 1.1 1994/09/14 17:13:07 vertex Exp $ */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"


/*****************************************************************************/


Item
CreateUniqueMsgPort(const char *name,uint8 pri,uint32 signal)
{
TagArg ta[5];
int32  i;

    ta[0].ta_Tag = TAG_ITEM_PRI;
    ta[0].ta_Arg = (void *)pri;

    ta[1].ta_Tag = TAG_ITEM_UNIQUE_NAME;
    ta[1].ta_Arg = 0;

    i = 2;
    if (name)
    {
        ta[i].ta_Tag = TAG_ITEM_NAME;
        ta[i].ta_Arg = (void *)name;
        i++;
    }

    if (signal)
    {
        ta[i].ta_Tag = CREATEPORT_TAG_SIGNAL;
        ta[i].ta_Arg = (void *)signal;
        i++;
    }
    ta[i].ta_Tag = TAG_END;

    return CreateItem(MKNODEID(KERNELNODE,MSGPORTNODE),ta);
}
