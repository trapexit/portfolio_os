/* $Id: createMsg.c,v 1.10 1994/09/01 21:58:52 vertex Exp $ */
/* file createMsg.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"

Item
CreateMsg(const char *name,uint8 pri,Item mp)
{
	TagArg ta[4];

	ta[0].ta_Tag = TAG_ITEM_PRI;
	ta[0].ta_Arg = (void *)pri;

	if (name)
	{
	    ta[1].ta_Tag = TAG_ITEM_NAME;
	    ta[1].ta_Arg = (void *)name;
	}
	else ta[1].ta_Tag = TAG_NOP;

	/* Item dependent options */
	ta[2].ta_Tag = CREATEMSG_TAG_REPLYPORT;
	ta[2].ta_Arg = (void *)mp;

	ta[3].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,MESSAGENODE),ta);
}

