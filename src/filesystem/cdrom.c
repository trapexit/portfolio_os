/*****

$Id: cdrom.c,v 1.6 1994/02/18 01:54:55 limes Exp $

$Log: cdrom.c,v $
 * Revision 1.6  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.5  1993/12/01  00:04:51  limes
 * Fix some compile warnings.
 *
 * Revision 1.4  1993/07/28  02:31:05  dplatt
 * Clear IOInfo
 *
 * Revision 1.3  1993/06/15  00:55:14  dplatt
 * Ensure that OpenItem calls are in place
 *
 * Revision 1.2  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.1  1993/04/22  21:03:15  dplatt
 * New features, timeout support, bug fixes
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  cdrom.c - set CD-ROM default operating modes
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
#include "filesystem.h"
#include "filesystemdefs.h"
#include "discdata.h"
#include "cdrom.h"
#include "operror.h"
#include "time.h"

#include "filefunctions.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"
#include "stdio.h"

int errlim = 0;

extern Item SuperCreateSizedItem(int32 itemType, void *whatever, int32 size);

/* #define DEBUG */

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

#define BUFSIZE 64*1024
#define READSIZE 2048
#define READINCR 1
#define READMINBLOCK 150

uint32 beq(void *s, void *d, int32 n)
{
  char *s1 = (char *) s;
  char *d1 = (char *) d;
  while (--n >= 0) {
    if (*s1++ != *d1++) return 0;
  }
  return 1;
}

int main(int argc, char **argv)
{
  Item ioReqItem;
  IOReq *ioReq;
  Item cdRom = 0;
  int32 err;
  IOInfo ioInfo;
  union CDROMCommandOptions options;
  CDROM *theDevice;
  int argnum;
  static TagArg ioReqTags[2] =
    {
      CREATEIOREQ_TAG_DEVICE,       0,
      TAG_END,			0
      };
  options.asLongword = 0;
  for (argnum = 1; argnum < argc; argnum++) {
    if (strcmp(argv[argnum], "-ds") == 0) {
      options.asFields.speed = CDROM_DOUBLE_SPEED;
    } else if (strcmp(argv[argnum], "-ss") == 0) {
      options.asFields.speed = CDROM_SINGLE_SPEED;
    } else if (strcmp(argv[argnum], "-circ") == 0) {
      options.asFields.errorRecovery = CDROM_CIRC_RETRIES_ONLY;
    } else if (strncmp(argv[argnum], "-r", 2) == 0) {
      options.asFields.retryShift = 0;
      if (strlen(argv[argnum]) > 2) {
	options.asFields.retryShift = (int)strtol(argv[argnum]+2, NULL, 0);
      }
      if (options.asFields.errorRecovery == CDROM_Option_Unspecified) {
	options.asFields.errorRecovery = CDROM_DEFAULT_RECOVERY;
      }
    } else if (strcmp(argv[argnum], "-h") == 0) {
      options.asFields.readAhead = CDROM_READAHEAD_ENABLED;
    } else if (strcmp(argv[argnum], "-nh") == 0) {
      options.asFields.readAhead = CDROM_READAHEAD_DISABLED;
    } 
  }
  cdRom = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE), "CD-ROM"), 0);
  if (cdRom < 0) return 0;
  theDevice = (CDROM *) LookupItem(cdRom);
  ioReqTags[0].ta_Arg = (void *) cdRom;
  ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    printf("Can't allocate an IOReq for CD-ROM device");
    return 0;
  }
  ioReq = (IOReq *) LookupItem(ioReqItem);
  memset(&ioInfo, 0, sizeof ioInfo);
  ioInfo.ioi_Command = CDROMCMD_SETDEFAULTS;
  ioInfo.ioi_CmdOptions = options.asLongword;
  err = DoIO(ioReqItem, &ioInfo);
  if (err < 0 || (err = ioReq->io_Error) < 0) {
    PrintError(0,"set CD-ROM defaults",0,err);
  }
  return (int) err;
}
