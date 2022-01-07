/*****

$Id: delete.c,v 1.3 1994/02/18 17:43:07 limes Exp $

$Log: delete.c,v $
 * Revision 1.3  1994/02/18  17:43:07  limes
 * include "operror.h" to get PrintError macro
 *
 * Revision 1.2  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.1  1993/05/10  19:42:48  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.de_
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  delete.c - delete a file (usually on the NVRAM filesystem)
*/

#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "operror.h"

#undef SUPER

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"
#include "stdio.h"

int main (int32 argc, char **argv)
{
  int i;
  Item newFileItem;
  if (argc < 2) {
    printf("Usage: delete file [file2 ...]\n");
  } else {
    for (i = 1; i < argc; i ++) {
      printf("Deleting %s\n", argv[i]);
      newFileItem = DeleteFile(argv[i]);
      if (newFileItem < 0) {
	PrintError(0,"delete",argv[i],(uint32) newFileItem);
      }
    }
  }
  return 0;
}
