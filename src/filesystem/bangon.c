/*****

$Id: bangon.c,v 1.11 1994/09/22 20:05:02 dplatt Exp $

$Log: bangon.c,v $
 * Revision 1.11  1994/09/22  20:05:02  dplatt
 * Add -random option to torture-test the mechanism.
 *
 * Revision 1.10  1993/12/01  00:04:34  limes
 * Fix some compile warnings.
 *
 * Revision 1.9  1993/07/28  02:30:50  dplatt
 * Zero out IOReqs;  do delete-item cleanup
 *
 * Revision 1.8  1993/07/15  04:36:32  dplatt
 * Add -log option;  print device flag byte.
 *
 * Revision 1.7  1993/07/11  21:16:52  dplatt
 * Add a -loop option to repeat tests forever.
 *
 * Revision 1.6  1993/06/15  00:55:14  dplatt
 * Ensure that OpenItem calls are in place
 *
 * Revision 1.5  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.4  1993/03/24  23:41:16  dplatt
 * New drive, timeouts, multiple drives, new xbus features, etc.
 *
 * Revision 1.3  1993/03/17  18:13:52  dplatt
 * Use new timer-device semantics
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
  bangon.c - test program for driving the CD-ROM.
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
#include "stdlib.h"

extern void FileDaemonStartup(void);

extern TagArg fileFolioTags[];
extern TagArg fileDriverTags[];

int errlim = 0;

extern Item SuperCreateSizedItem(int32 itemType, void *whatever, int32 size);

/* #define DEBUG */

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

int32 blocksize = CDROM_M1_D;

#define BUFSIZE 64*blocksize

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
  Item ioReqItem, timerDeviceItem, timerReqItem;
  IOReq *ioReq;
  Item cdRom = 0;
  int32 err;
  uint32 i;
  int passnum;
  int32 readincr, readsize;
  DeviceStatus status;
  char *buf1, *buf2;
  IOInfo ioInfo;
  IOInfo timerInfo;
  union CDROMCommandOptions options;
  CDROM *theDevice, *myCopy;
  int trace = 0, traceNum, traceStep, xorit = 0, xorsum, delayit=0;
  int32 j;
  int argnum, c, traces;
  int dumpDev = 0;
  int log = 0;
  int loop = 0;
  int redbook = 0;
  int random = 0;
  const struct timeval cdromDelay = {0, 200 * 1000};
  static TagArg ioReqTags[2] =
    {
      CREATEIOREQ_TAG_DEVICE,       0,
      TAG_END,			0
      };
  static TagArg timerReqTags[2] =
    {
      CREATEIOREQ_TAG_DEVICE,       0,
      TAG_END,			0
      };
  printf("Bangon: hello world!\n");
  memset(&ioInfo, 0, sizeof ioInfo);
  memset(&timerInfo, 0, sizeof timerInfo);
  options.asLongword = 0;
  for (argnum = 1; argnum < argc; argnum++) {
    if (strncmp(argv[argnum], "-t", 2) == 0) {
      trace = 1;
      traceStep = 75;
      if (strlen(argv[argnum]) > 2) {
	traceStep = 0;
	i = 2;
	while ((c = *(argv[argnum]+i)) != '\0') {
	  traceStep = traceStep * 10 + c - '0';
	  i++;
	}
      }
      printf("Progress trace enabled every %d blocks\n", traceStep);
    } else if (strcmp(argv[argnum], "-random") == 0) {
      random = 1;
      printf("Random seeks\n");
    } else if (strcmp(argv[argnum], "-x") == 0) {
      xorit = 1;
      printf("Checksumming enabled\n");
    } else if (strcmp(argv[argnum], "-d") == 0) {
      delayit = 1;
      printf("200-millisecond block delay enabled\n");
    } else if (strcmp(argv[argnum], "-ds") == 0) {
      options.asFields.speed = CDROM_DOUBLE_SPEED;
      printf("Double-speed operation requested\n");
    } else if (strcmp(argv[argnum], "-ss") == 0) {
      options.asFields.speed = CDROM_SINGLE_SPEED;
      printf("Normal-speed operation requested\n");
    } else if (strcmp(argv[argnum], "-stat") == 0) {
      dumpDev = 1;
      printf("Print device status\n");
    } else if (strcmp(argv[argnum], "-log") == 0) {
      log = 1;
      printf("Print device log\n");
    } else if (strcmp(argv[argnum], "-loop") == 0) {
      loop = 1;
      printf("Loop forever\n");
    } else if (strcmp(argv[argnum], "-redbook") == 0) {
      redbook = 1;
      blocksize = CDROM_DA;
      options.asFields.blockLength = CDROM_DA;
      options.asFields.densityCode = CDROM_DIGITAL_AUDIO;
      printf("Red book mode, %d bytes per block\n", blocksize);
    } else if (strcmp(argv[argnum], "-cd+g") == 0) {
      redbook = 1;
      blocksize = CDROM_DA_PLUS_SUBCODE;
      options.asFields.blockLength = CDROM_DA_PLUS_SUBCODE;
      options.asFields.densityCode = CDROM_DIGITAL_AUDIO;
      printf("CD+G (Red Book plus subcode) mode, %d bytes per block\n", blocksize);
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
    } else if (strcmp(argv[argnum], "-e") == 0) {
      errlim = 20;
      printf("Exit after 20 errors\n");
    }
  }
  cdRom = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE), "CD-ROM"), 0);
  printf("CD-ROM device is item 0x%x\n", cdRom);
  if (cdRom < 0) return 0;
  theDevice = (CDROM *) LookupItem(cdRom);
  if (dumpDev) {
    myCopy = (CDROM *) malloc(sizeof(CDROM));
    memcpy(myCopy, theDevice, sizeof (CDROM));
    printf("cdrom_PushParameters = 0x%x\n", myCopy->cdrom_PushParameters);
    printf("cdrom_LastErrorCode = 0x%x\n", myCopy->cdrom_LastErrorCode);
    printf("cdrom_VendorType = 0x%x\n", myCopy->cdrom_VendorType);
    printf("cdrom_Holdoff_MS = 0x%x\n", myCopy->cdrom_Holdoff_MS);
    printf("cdrom_MediumBlockCount = 0x%x\n", myCopy->cdrom_MediumBlockCount);
    printf("cdrom_NextBlockOffset = 0x%x\n", myCopy->cdrom_NextBlockOffset);
    printf("cdrom_BlocksPerDataXfer = 0x%x\n", myCopy->cdrom_BlocksPerDataXfer);
    printf("cdrom_TrackToProbe = 0x%x\n", myCopy->cdrom_TrackToProbe);
    printf("cdrom_ReadRemaining = 0x%x\n", myCopy->cdrom_ReadRemaining);
    printf("cdrom_RetrysPermitted = 0x%x\n", myCopy->cdrom_RetrysPermitted);
    printf("cdrom_Level0 = 0x%x\n", myCopy->cdrom_Level0);
    printf("cdrom_Level1 = 0x%x\n", myCopy->cdrom_Level1);
    printf("cdrom_Level2 = 0x%x\n", myCopy->cdrom_Level2);
    printf("cdrom_MustReadError = 0x%x\n", myCopy->cdrom_MustReadError);
    printf("cdrom_DoingReadError = 0x%x\n", myCopy->cdrom_DoingReadError);
    printf("cdrom_DiscAvailable = 0x%x\n", myCopy->cdrom_DiscAvailable);
    printf("cdrom_DiscInfoAvailable = 0x%x\n", myCopy->cdrom_DiscInfoAvailable);
    printf("cdrom_Initialized = 0x%x\n", myCopy->cdrom_Initialized);
    printf("cdrom_DoReadahead = 0x%x\n", myCopy->cdrom_DoReadahead);
    printf("cdrom_ReadaheadActive = 0x%x\n", myCopy->cdrom_ReadaheadActive);
    printf("cdrom_MustFlush = 0x%x\n", myCopy->cdrom_MustFlush);
    printf("cdrom_StatusByte = 0x%x\n", myCopy->cdrom_StatusByte);
#ifdef CDLOG
    if (log) {
      int i;
      for (i = 0 ; i < CDLOG; i++) {
	printf("Log %d from %d: ", myCopy->cdrom_Log[i].index,
	       myCopy->cdrom_Log[i].actor);
	printf("levels %d/%d/%d, ",
	       myCopy->cdrom_Log[i].level0,
	       myCopy->cdrom_Log[i].level1,
	       myCopy->cdrom_Log[i].level2);
	printf("RA do %d, running %d\n",
	       myCopy->cdrom_Log[i].doRA,
	       myCopy->cdrom_Log[i].raActive);
      }
    }
#endif
    return 0;
  }
  ioReqTags[0].ta_Arg = (void *) cdRom;
  ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    printf("Can't allocate an IOReq for CD-ROM device");
    return 0;
  }
  ioReq = (IOReq *) LookupItem(ioReqItem);
  timerDeviceItem = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
					   "timer"), (void *) NULL);
  if (timerDeviceItem < 0) {
    printf("Error opening timer device\n");
    return 0;
  } 
  timerReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  timerReqTags[0].ta_Arg = (void *) timerDeviceItem;
  timerReqTags[1].ta_Tag = TAG_END;
  timerReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), timerReqTags);
  if (timerReqItem < 0) {
    printf("Error creating IOReq node\n");
    return 0;
  }
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
    printf("Device flags word contains 0x%x\n", status.ds_DeviceFlagWord);
    return 0;
  }
  printf("Device block count is %d\n", status.ds_DeviceBlockCount);
  printf("Device flags word contains 0x%x\n", status.ds_DeviceFlagWord);
 loophere:
  printf("Pass 1 - read volume once through\n");
  traceNum = 0;
  traces = 0;
  for (i = READMINBLOCK; i < status.ds_DeviceBlockCount; i += READINCR) {
    if (delayit) {
      timerInfo.ioi_Unit = 1;
      timerInfo.ioi_Command = TIMERCMD_DELAY;
      timerInfo.ioi_Send.iob_Buffer = (void *) &cdromDelay;
      timerInfo.ioi_Send.iob_Len = sizeof cdromDelay;
      timerInfo.ioi_Recv.iob_Buffer = NULL;
      timerInfo.ioi_Recv.iob_Len = 0;
      DoIO(timerReqItem, &timerInfo);
    }
    if (trace && ++traceNum >= traceStep) {
      printf(".");
      if (++traces >= 75) {
	traces = 0;
	printf("\n");
      }
      traceNum = 0;
    }
    ioInfo.ioi_Recv.iob_Buffer = (void *) buf1;
    ioInfo.ioi_Recv.iob_Len = blocksize;
    if (READINCR == 1 && random) {
      do {
	ioInfo.ioi_Offset = urand() % (status.ds_DeviceBlockCount - 4);
      } while (ioInfo.ioi_Offset < 150);
    } else {
      ioInfo.ioi_Offset = i;
    }
    ioInfo.ioi_Command = CMD_READ;
    err = DoIO(ioReqItem, &ioInfo);
    if (err < 0 || (err = ioReq->io_Error) < 0) {
      printf("Block %d: ", i);
      PrintfSysErr(err);
      if (errlim && ! --errlim) {
	goto done;
      }
    } else if (ioReq->io_Actual != blocksize) {
      printf("Block %d: only got %d bytes\n", i, ioReq->io_Actual);
      if (errlim && ! --errlim) {
	goto done;
      }
    }
    if (xorit) {
      xorsum = 0;
      j = blocksize / 4;
      while (--j >= 0) {
	xorsum ^= ((int *) buf1)[j];
      }
      printf("%d 0x%x\n", i, xorsum);
    }
  }
    if (trace) {
      printf("\n");
    }
  if (redbook) {
    goto endrun;
  }
  traceNum = 0;
  for (readincr = 1, readsize = blocksize, passnum = 2;
       readincr <= BUFSIZE / blocksize;
       readincr *= 2, readsize *= 2, passnum += 1) {
    traces = 0;
    printf("Pass %d - read and compare %d block(s) at a time\n",
	   passnum, readincr);
    for (i = READMINBLOCK;
	 i < status.ds_DeviceBlockCount - readincr - 1;
	 i += readincr) {
      if (trace && ++traceNum >= traceStep) {
	printf(".");
	if (++traces >= 75) {
	  traces = 0;
	  printf("\n");
	}
	traceNum = 0;
      }
      ioInfo.ioi_Recv.iob_Buffer = (void *) buf1;
      ioInfo.ioi_Recv.iob_Len = readsize;
      ioInfo.ioi_Offset = i;
      ioInfo.ioi_Command = CMD_READ;
      err = DoIO(ioReqItem, &ioInfo);
      if (err < 0 || (err = ioReq->io_Error) < 0) {
	printf("Block %d first time: ", i);
	PrintfSysErr(err);
	if (errlim && ! --errlim) {
	  goto done;
	}
      } else if (ioReq->io_Actual != readsize) {
	printf("Block %d 1st time: only got %d bytes\n", i, ioReq->io_Actual);
	if (errlim && ! --errlim) {
	  goto done;
	}
      } else {
	ioInfo.ioi_Recv.iob_Buffer = (void *) buf2;
	ioInfo.ioi_Recv.iob_Len = readsize;
	ioInfo.ioi_Offset = i;
	ioInfo.ioi_Command = CMD_READ;
	err = DoIO(ioReqItem, &ioInfo);
	if (err < 0 || (err = ioReq->io_Error) < 0) {
	  printf("Block %d second time: ", i);
	  PrintfSysErr(err);
	  if (errlim && ! --errlim) {
	    goto done;
	  }
	} else if (ioReq->io_Actual != readsize) {
	  printf("Block %d 2nd time: only got %d bytes\n", i, ioReq->io_Actual);
	  if (errlim && ! --errlim) {
	    goto done;
	  }
	} else {
	  if (!beq(buf1, buf2, readsize)) {
	    printf("Compare mismatch on block %d!\n", i);
	    if (errlim && ! --errlim) {
	      goto done;
	    }
	  }
	}
      }
    }
    if (trace) {
      printf("\n");
    }
  }
 endrun:
  if (loop) {
    goto loophere;
  }
 done: ;
  printf("Done\n");
  return 0;
}
