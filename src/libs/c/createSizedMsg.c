/* $Id: createSizedMsg.c,v 1.6 1994/09/01 21:58:52 vertex Exp $ */
/* file createSizedMsg.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "msgport.h"


Item
CreateSizedMsg(const char *name,uint8 pri,Item mp,uint32 datasize)
{
	TagArg ta[5];

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

	if(datasize) {
		ta[3].ta_Tag = CREATEMSG_TAG_DATA_SIZE;
		ta[3].ta_Arg = (void *)datasize;
	}
	else ta[3].ta_Tag = TAG_NOP;

	ta[4].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,MESSAGENODE),ta);
}

