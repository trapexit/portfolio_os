/* $Id: isuser.c,v 1.3 1994/05/11 23:05:13 limes Exp $ */
#include "types.h"
#include "internalf.h"

bool
isUser(void)
{
	return (getPSR() & 3) == 0;
}

