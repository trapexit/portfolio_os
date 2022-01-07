/*****

$Id: lmdump.c,v 1.5 1994/04/21 18:09:21 shawn Exp $

$Log: lmdump.c,v $
 * Revision 1.5  1994/04/21  18:09:21  shawn
 * Bug fix. Send.Len should be Recv.Len and the whole thing
 * out the loop and added DEBUG stuff.
 *
 * Revision 1.4  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.3  1993/07/28  02:30:50  dplatt
 * Zero out IOReqs;  do delete-item cleanup
 *
 * Revision 1.2  1993/07/03  00:31:06  dplatt
 * Change label mechanisms to keep the lawyers happy
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
  lmdump.c - dump/interpret a linked-memory-block filesystem
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

/* #define	DEBUG */
#ifdef	DEBUG
#define DBUG(x)	printf x
#else	/* DEBUG */
#define DBUG(x)	/* x */
#endif	/* DEBUG */

int main(int argc, char **argv)
{
  Item ioReqItem;
  IOReq *ioReq;
  Item dev = 0;
  int32 err;
  IOInfo ioInfo;
  int32 unit, offset;
  int32 halt, probe, limit, prev;
  DeviceStatus devStatus;
  DiscLabel discLabel;
  struct {
    LinkedMemFileEntry entry;
    uchar slack [64];
    } theEntry;
  int32 anchorOffset, freeOffset;
  int32 blockSize, blockCount, blockRound, labelBlocks, blockBlocks;
  if (argc != 4) {
    printf("Usage: lmdump DEVICENAME UNIT OFFSET\n");
    return 0;
  }
  dev = OpenNamedDevice(argv[1], NULL);
  fail(dev,"open",argv[1]);
  ioReqItem = CreateIOReq(NULL, 50, dev, 0);
  if (ioReqItem < 0) {
    return (int) ioReqItem;
  }
  unit = strtol(argv[2], NULL, 0);
  offset = strtol(argv[3], NULL, 0);
  ioReq = (IOReq *) LookupItem(ioReqItem);
  memset(&ioInfo, 0, sizeof ioInfo);
  ioInfo.ioi_Recv.iob_Buffer = &devStatus;
  ioInfo.ioi_Recv.iob_Len = sizeof devStatus;
  ioInfo.ioi_Unit = (uint8) unit;
  ioInfo.ioi_Command = CMD_STATUS;
  err = DoIO(ioReqItem, &ioInfo);
  fail(err,"get status of",argv[1]);
  fail(ioReq->io_Error,"get status of",argv[1]);
  blockSize = devStatus.ds_DeviceBlockSize;
  blockCount = devStatus.ds_DeviceBlockCount;
  printf("Device %s unit %d has %d blocks of %d bytes each\n",
	 argv[1], unit, blockCount, blockSize);
  blockRound = blockSize - 1;
  labelBlocks = (sizeof discLabel + blockRound) / blockSize;
  anchorOffset = labelBlocks;
  blockBlocks = (sizeof (LinkedMemBlock) + blockRound) / blockSize;
  freeOffset = anchorOffset + blockBlocks;
  ioInfo.ioi_Recv.iob_Buffer = &discLabel;
  ioInfo.ioi_Recv.iob_Len = labelBlocks * blockSize;
  ioInfo.ioi_Offset = offset;
  ioInfo.ioi_Unit = (uint8) unit;
  ioInfo.ioi_Flags = 0;
  ioInfo.ioi_Command = CMD_READ;
  ioInfo.ioi_CmdOptions = 0;
  err = DoIO(ioReqItem, &ioInfo);
  fail(err,"read file system header",argv[1]);
  fail(ioReq->io_Error,"read file system header",argv[1]);
  if (discLabel.dl_RecordType != 1 ||
      discLabel.dl_VolumeSyncBytes[0] != VOLUME_SYNC_BYTE ||
      discLabel.dl_VolumeSyncBytes[1] != VOLUME_SYNC_BYTE ||
      discLabel.dl_VolumeSyncBytes[2] != VOLUME_SYNC_BYTE ||
      discLabel.dl_VolumeSyncBytes[3] != VOLUME_SYNC_BYTE ||
      discLabel.dl_VolumeSyncBytes[4] != VOLUME_SYNC_BYTE ||
      discLabel.dl_VolumeStructureVersion != VOLUME_STRUCTURE_LINKED_MEM) {
    printf("Not an Opera flat linked-memory filesystem\n");
    return -1;
  }
  printf("Volume name:          %s\n", discLabel.dl_VolumeIdentifier);
  printf("Volume block size:    %d\n", discLabel.dl_VolumeBlockSize);
  printf("Volume block count:   %d\n", discLabel.dl_VolumeBlockCount);
  printf("Volume root:          %d\n", discLabel.dl_RootDirectoryAvatarList[0]);
  probe = discLabel.dl_RootDirectoryAvatarList[0];
  halt = probe;
  limit = 128;
  prev = -1;
  ioInfo.ioi_Recv.iob_Buffer = &theEntry;
  ioInfo.ioi_Recv.iob_Len =
    ((sizeof (LinkedMemFileEntry) + discLabel.dl_VolumeBlockSize - 1) /
     discLabel.dl_VolumeBlockSize) * discLabel.dl_VolumeBlockSize;

  do {
    ioInfo.ioi_Offset = probe + offset;
    err = DoIO(ioReqItem, &ioInfo);
    DBUG(("Reading entry at offset %d\n", probe));
    fail(err,"read entry err",0);
    fail(ioReq->io_Error,"read entry",0);
    printf("Entry [%d] %d [%d] size %d blocks is ",
	   theEntry.entry.lmfe.lmb_BlinkOffset,
	   probe,
	   theEntry.entry.lmfe.lmb_FlinkOffset,
	   theEntry.entry.lmfe.lmb_BlockCount);
    switch (theEntry.entry.lmfe.lmb_Fingerprint) {
    case FINGERPRINT_ANCHORBLOCK:
      printf("ANCHOR\n");
      break;
    case FINGERPRINT_FREEBLOCK:
      printf("FREE\n");
      break;
    case FINGERPRINT_FILEBLOCK:
      printf("FILE %s, %d bytes\n",
	     theEntry.entry.lmfe_FileName,
	     theEntry.entry.lmfe_ByteCount);
      break;
    default:
      printf("BOGUS, type 0x%x\n", theEntry.entry.lmfe.lmb_Fingerprint);
      break;
    }
    if (prev > 0 && prev != theEntry.entry.lmfe.lmb_BlinkOffset) {
      printf("ERROR!  Backlink is broken!\n");
    }
    prev = probe;
    probe = theEntry.entry.lmfe.lmb_FlinkOffset;
  } while (probe != halt && --limit > 0);
  return 0;
}
