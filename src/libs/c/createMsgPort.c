/* $Id: createMsgPort.c,v 1.3 1994/09/02 03:32:28 stan Exp $ */
/* file createMsgPort.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"

Item
CreateMsgPort(const char *name,uint8 pri,uint32 signal)
{
	TagArg ta[5];
	int i;
	ta[0].ta_Tag = TAG_ITEM_PRI;
	ta[0].ta_Arg = (void *)pri;
	i = 1;
	if (name)
	{
	    ta[i].ta_Tag = TAG_ITEM_NAME;
	    ta[i].ta_Arg = (void *)name;
	    i++;
	}
	/* Item dependent options */
	if (signal)
	{
	    ta[i].ta_Tag = CREATEPORT_TAG_SIGNAL;
	    ta[i].ta_Arg = (void *)signal;
	    i++;
	}
	ta[i].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,MSGPORTNODE),ta);
}
