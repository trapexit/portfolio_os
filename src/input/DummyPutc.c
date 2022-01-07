
/*****

$Id: DummyPutc.c,v 1.2 1994/08/29 19:00:21 vertex Exp $

$Log: DummyPutc.c,v $
 * Revision 1.2  1994/08/29  19:00:21  vertex
 * Removed local definition of kprintf(), and now #include "debug.h"
 * to get the system-wide definition
 *
 * Revision 1.1  1994/05/02  21:21:15  dplatt
 * Initial revision
 *

*****/

/*
  Copyright The 3DO Company Inc., 1994, 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

#include "debug.h"

int stdout;

int
putc(int a)
{
    kprintf("%c",a);
    return a;
}

