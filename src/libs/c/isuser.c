/* $Id: isuser.c,v 1.3 1994/05/12 00:17:14 limes Exp $ */
#include "clib.h"

int
isUser(void)
{
	return (getPSR() & 3) == 0;
}

