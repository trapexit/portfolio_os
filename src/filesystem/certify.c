/*****

$Id: certify.c,v 1.3 1993/04/22 21:03:15 dplatt Exp $

$Log: certify.c,v $
 * Revision 1.3  1993/04/22  21:03:15  dplatt
 * New features, timeout support, bug fixes
 *
 * Revision 1.2  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.1  1993/01/05  20:57:47  dplatt
 * CES changes redux
 *
 * Revision 1.8  1993/01/04  02:19:26  dplatt
 * CES Changes
 *
 * Revision 1.7  1992/12/22  07:58:10  dplatt
 * Magneto 3 changes and CD-ROM support
 *
 * Revision 1.6  1992/12/08  05:59:52  dplatt
 * Magenta changes
 *
 * Revision 1.5  1992/10/27  01:35:41  dplatt
 * Oops, tested on UGO not on Blue
 *
 * Revision 1.4  1992/10/24  00:40:56  dplatt
 * Bluebird changes and bug fixes
 *
 * Revision 1.3  1992/10/16  01:22:24  dplatt
 * First cut at bluebird release changes
 *
 * Revision 1.2  1992/10/01  23:36:21  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:42:27  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  certify.c - simple CD-ROM certification program
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

#include "filefunctions.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"
#include "stdio.h"

extern void FileDaemonStartup(void);

extern TagArg fileFolioTags[];
extern TagArg fileDriverTags[];

extern Item SuperCreateSizedItem(int32 itemType, void *whatever, int32 size);

/* #define DEBUG */

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

#define BUFSIZE 512*1024
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
  uint32 i;
  int passnum;
  int readincr, readsize;
  DeviceStatus status;
  char *buf1, *buf2;
  IOInfo ioInfo;
  union CDROMCommandOptions options;
  int32 argnum;
  static TagArg ioReqTags[2] =
    {
      CREATEIOREQ_TAG_DEVICE,       0,
      TAG_END,			0
      };
  options.asLongword = 0;
  for (argnum = 1; argnum < argc; argnum++) {
    if (strcmp(argv[argnum], "-ds") == 0) {
      options.asFields.speed = CDROM_DOUBLE_SPEED;
      printf("Double-speed operation requested\n");
    } else if (strcmp(argv[argnum], "-ss") == 0) {
      options.asFields.speed = CDROM_SINGLE_SPEED;
      printf("Normal-speed operation requested\n");
    } else if (strcmp(argv[argnum], "-circ") == 0) {
      options.asFields.errorRecovery = CDROM_CIRC_RETRIES_ONLY;
      printf("CIRC error recovery only, no LERC\n");
    } else if (strncmp(argv[argnum], "-r", 2) == 0) {
      options.asFields.retryShift = 0;
      printf("Retry limit set to 0\n");
      if (options.asFields.errorRecovery == CDROM_Option_Unspecified) {
	options.asFields.errorRecovery = CDROM_DEFAULT_RECOVERY;
      }
    } else if (strcmp(argv[argnum], "-h") == 0) {
      options.asFields.readAhead = CDROM_READAHEAD_ENABLED;
      printf("Readahead/hover enabled\n");
    } else if (strcmp(argv[argnum], "-nh") == 0) {
      options.asFields.readAhead = CDROM_READAHEAD_DISABLED;
      printf("Readahead/hover disabled\n");
    }
  }
  cdRom = FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE), "CD-ROM");
  printf("CD-ROM device is item 0x%x\n", cdRom);
  if (cdRom < 0) return 0;
  ioReqTags[0].ta_Arg = (void *) cdRom;
  ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    printf("Can't allocate an IOReq for CD-ROM device");
    return 0;
  }
  ioReq = (IOReq *) LookupItem(ioReqItem);
  buf1 = (char *) ALLOCMEM(BUFSIZE, MEMTYPE_DMA);
  buf2 = (char *) ALLOCMEM(BUFSIZE, MEMTYPE_DMA);
  if (!buf1 || !buf2) {
    printf("Can't allocate buffers\n");
    return 0;
  }
  ioInfo.ioi_Send.iob_Buffer = NULL;
  ioInfo.ioi_Send.iob_Len = 0;
  ioInfo.ioi_Recv.iob_Buffer = &status;
  ioInfo.ioi_Recv.iob_Len = sizeof status;
  ioInfo.ioi_Unit = 0;
  ioInfo.ioi_Command = CMD_STATUS;
  ioInfo.ioi_CmdOptions = options.asLongword;
  err = DoIO(ioReqItem, &ioInfo);
  if (err < 0 || (err = ioReq->io_Error) < 0) {
    printf("Error getting status: ");
    PrintfSysErr(err);
    return 0;
  }
  for (readincr = BUFSIZE / READSIZE, readsize = BUFSIZE, passnum = 1;
       readincr >= 1;
       readincr /= 2, readsize /= 2, passnum += 1) {
    printf("Pass %d - read and compare %d block(s) at a time\n",
	    passnum, readincr);
    for (i = READMINBLOCK;
	 i < status.ds_DeviceBlockCount - readincr - 3;
	 i += readincr) {
      ioInfo.ioi_Recv.iob_Buffer = (void *) buf1;
      ioInfo.ioi_Recv.iob_Len = readsize;
      ioInfo.ioi_Offset = i;
      ioInfo.ioi_Command = CMD_READ;
      err = DoIO(ioReqItem, &ioInfo);
      if (err < 0 || (err = ioReq->io_Error) < 0) {
	printf("Error reading block %d first time: ", i);
	PrintfSysErr(err);
      } else if (ioReq->io_Actual != readsize) {
	printf("Only got %d bytes for block %d\n", ioReq->io_Actual, i);
      } else {
	ioInfo.ioi_Recv.iob_Buffer = (void *) buf2;
	ioInfo.ioi_Recv.iob_Len = readsize;
	ioInfo.ioi_Offset = i;
	ioInfo.ioi_Command = CMD_READ;
	err = DoIO(ioReqItem, &ioInfo);
	if (err < 0 || (err = ioReq->io_Error) < 0) {
	  printf("Error reading block %d second time: ", i);
	  PrintfSysErr(err);
	} else if (ioReq->io_Actual != readsize) {
	  printf("Only got %d bytes for block %d\n", ioReq->io_Actual, i);
	} else {
	  if (!beq(buf1, buf2, readsize)) {
	    printf("Compare mismatch on block %d!\n");
	  }
	}
      }
    }
  }
  printf("Done\n");
  return 0;
}
