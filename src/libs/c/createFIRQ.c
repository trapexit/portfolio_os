/* $Id: createFIRQ.c,v 1.5 1994/09/01 22:10:55 vertex Exp $ */
/* file createFIRQ.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "interrupts.h"

Item
CreateFIRQ(const char *name,uint8 pri,int32 (*code)(),int32 num)
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
	ta[2].ta_Tag = CREATEFIRQ_TAG_CODE;
	ta[2].ta_Arg = (void *)((long)code);

	ta[3].ta_Tag = CREATEFIRQ_TAG_NUM;
	ta[3].ta_Arg = (void *)num;

	ta[4].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,FIRQNODE),ta);
}
