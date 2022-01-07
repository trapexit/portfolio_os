/* $Id: createSema4.c,v 1.4 1994/09/01 22:02:02 vertex Exp $ */
/* file createSema4.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "semaphore.h"

Item
CreateSemaphore(const char *name,uint8 pri)
{
	TagArg ta[3];

	ta[0].ta_Tag = TAG_ITEM_PRI;
	ta[0].ta_Arg = (void *)pri;
	if (name)
	{
	    ta[1].ta_Tag = TAG_ITEM_NAME;
	    ta[1].ta_Arg = (void *)name;
	}
	else ta[1].ta_Tag = TAG_NOP;

	/* Item dependent options */
	ta[2].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,SEMA4NODE),ta);
}
