/*****

$Id: mount.c,v 1.3 1994/02/18 01:54:55 limes Exp $

$Log: mount.c,v $
 * Revision 1.3  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.2  1993/07/11  21:53:29  dplatt
 * Don't report bogus zero error
 *
 * Revision 1.1  1993/05/10  19:42:48  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  mount.c - mount a filesystem
*/

#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "msgport.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "discdata.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "operror.h"
#include "time.h"

#include "filefunctions.h"
#include "stdio.h"
#include "strings.h"

#define fail(err,verb,noun) if (err < 0) { PrintError(0,verb,noun,err); return (int) err; }

int main(int argc, char **argv)
{
  Item dev = 0;
  int32 err;
  int32 unit, offset;
  if (argc != 4) {
    printf("Usage: mount DEVICENAME UNIT OFFSET\n");
    return 0;
  }
  dev = OpenNamedDevice(argv[1], NULL);
  fail(dev,"open",argv[1]);
  unit = strtol(argv[2], NULL, 0);
  offset = strtol(argv[3], NULL, 0);
  err = MountFileSystem(dev, unit, offset);
  fail(err,"mount",argv[1]);
  return 0;
}
