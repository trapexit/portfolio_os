/* $Id: createDev.c,v 1.7 1994/09/01 22:10:55 vertex Exp $ */
/* file createDev.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "device.h"


Item
CreateDevice(const char *name, uint8 pri,Item driver)
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
	ta[2].ta_Tag = CREATEDEVICE_TAG_DRVR;
	ta[2].ta_Arg = (void *)driver;
	ta[3].ta_Tag = TAG_END;
	return CreateItem(MKNODEID(KERNELNODE,DEVICENODE),ta);
}
