/* $Id: finditem.c,v 1.4 1994/02/09 01:22:45 limes Exp $ */
#include "types.h"
#include "nodes.h"
#include "item.h"
#include "super.h"

Item SuperFindNamedItem(cntype, name)
int32 cntype;
char *name;
{
	TagArg	tags[2];
	tags[0].ta_Tag = TAG_ITEM_NAME;
	tags[0].ta_Arg = (void *)name;
	tags[1].ta_Tag = 0;
	return SuperFindItem(cntype,tags);
}
