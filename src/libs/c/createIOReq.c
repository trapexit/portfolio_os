/* $Id: createIOReq.c,v 1.5 1994/09/01 21:58:52 vertex Exp $ */
/* file createIOReq.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "io.h"



Item
CreateIOReq(const char *name,uint8 pri,Item dev,Item mp)
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
	ta[i].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	ta[i].ta_Arg = (void *)dev;
	i++;
	if (mp > 0)
	{
	    ta[i].ta_Tag = CREATEIOREQ_TAG_REPLYPORT;
	    ta[i].ta_Arg = (void *)mp;
	    i++;
	}
	ta[i].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,IOREQNODE),ta);
}
