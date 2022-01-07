/* $Id: dov.c,v 1.1 1994/07/26 23:59:38 limes Exp $ */

#include <types.h>
#include <sherryvers.h>
#include <clib.h>		/* for isUser() */

/*
 * If we are not supervisor, and we are trying to set the
 * version, we must dive through the swi. Otherwise, we can
 * just call through the supervector.
 */

uint32
DiscOsVersion(uint32 set)
{
    return (set && isUser()) ? swiDiscOsVersion(set) : SuperDiscOsVersion(set);
}
