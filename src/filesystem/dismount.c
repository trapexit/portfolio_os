/*****

$Id: dismount.c,v 1.2 1994/02/18 01:54:55 limes Exp $

$Log: dismount.c,v $
 * Revision 1.2  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.1  1993/05/31  03:33:05  dplatt
 * Tweaks, and filesystem dismounting
 *

 *****/

/*
  Copyright The 3DO Company Inc., 1993
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  dismount.c - mount a filesystem
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
  int32 err;
  if (argc != 2) {
    printf("Usage: dismount FILESYSTEM\n");
    return 0;
  }
  err = DismountFileSystem(argv[1]);
  fail(err,"dismount",argv[1]);
  return (int) err;
}
