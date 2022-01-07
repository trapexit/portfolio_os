/* $Id: CreateItem.c,v 1.5 1994/04/26 02:33:58 limes Exp $ */
#include "types.h"
#include "item.h"
#include "super.h"

Item
SuperCreateItem(a,b)
long a;
TagArg *b;
{
	return SuperCreateSizedItem(a,b,0);
}
