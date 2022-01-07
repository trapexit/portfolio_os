/* $Id: createDrvr.c,v 1.4 1994/09/01 22:10:55 vertex Exp $ */
/* file createDev.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "driver.h"

Item
CreateDriver(const char *name,uint8 pri,Item (*init)())
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
	ta[2].ta_Tag = CREATEDRIVER_TAG_INIT;
	ta[2].ta_Arg = (void *)((long)init);
	ta[3].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),ta);
}
