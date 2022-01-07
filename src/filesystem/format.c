/*****

$Id: format.c,v 1.6 1994/11/22 00:03:24 shawn Exp $

$Log: format.c,v $
 * Revision 1.6  1994/11/22  00:03:24  shawn
 * disallow format of a mounted filesystem. Also, the volume
 * name should be slashfree.
 *
 * Revision 1.5  1994/06/08  00:19:09  shawn
 * use defines that is shared between utilities.
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
  format.c - format a linked-memory-block filesystem
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

#define fail(err,verb,noun) { PrintError(0,verb,noun,err); return (int) err; }
#define	MAX_FILE_NAME_LEN	32

char	*sub_slash(char *cp);
char	*add_slash(char *cp);

int main(int argc, char **argv)
{
  Item		ioReqItem;
  IOReq		*ioReq;
  Item		dev = 0;
  int32		err;
  IOInfo	ioInfo;
  int32		unit, offset;
  DeviceStatus	devStatus;
  DiscLabel	discLabel;
  LinkedMemBlock	anchorBlock, freeBlock;
  int32		anchorOffset, freeOffset;
  int32		blockSize, blockCount, blockRound, labelBlocks, blockBlocks;
  char		*mounted;

  if (argc != 5) {
    printf("Usage: format DEVICENAME UNIT OFFSET FSNAME\n");
    return 0;
  }
  dev = OpenNamedDevice(argv[1], NULL);
  if (dev < 0) {
    fail(dev,"open",argv[1]);
  }

  mounted = add_slash(argv[4]);
  if (OpenDiskFile(mounted) > 0) {
    printf(" %s is mounted, failed to format\n", mounted);
    return 0;
  }
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
  if (err < 0 || (err = ioReq->io_Error) < 0) {
    fail(err,"get status of",argv[1]);
  }
  blockSize = devStatus.ds_DeviceBlockSize;
  blockCount = devStatus.ds_DeviceBlockCount;
  printf("Device %s unit %d has %d blocks of %d bytes each\n",
	 argv[1], unit, blockCount, blockSize);
  blockRound = blockSize - 1;
  labelBlocks = (sizeof discLabel + blockRound) / blockSize;
  printf("Label requires %d blocks\n", labelBlocks);
  anchorOffset = labelBlocks;
  blockBlocks = (sizeof (LinkedMemBlock) + blockRound) / blockSize;
  printf("Linked-memory structures require %d blocks each\n", blockBlocks);
  freeOffset = anchorOffset + blockBlocks;
  discLabel.dl_RecordType = 1;
  memset(discLabel.dl_VolumeSyncBytes, VOLUME_SYNC_BYTE, sizeof discLabel.dl_VolumeSyncBytes);
  discLabel.dl_VolumeStructureVersion = VOLUME_STRUCTURE_LINKED_MEM;
  discLabel.dl_VolumeFlags = 0;
  strncpy(discLabel.dl_VolumeCommentary, "formatted discdata", VOLUME_COM_LEN);
  strncpy(discLabel.dl_VolumeIdentifier, sub_slash(argv[4]), VOLUME_ID_LEN);
  discLabel.dl_VolumeUniqueIdentifier = -1;
  discLabel.dl_VolumeBlockSize = blockSize;
  discLabel.dl_VolumeBlockCount = blockCount - offset;
  discLabel.dl_RootUniqueIdentifier = -2;
  discLabel.dl_RootDirectoryBlockCount = 0;
  discLabel.dl_RootDirectoryBlockSize = blockSize;
  discLabel.dl_RootDirectoryLastAvatarIndex = 0;
  discLabel.dl_RootDirectoryAvatarList[0] = anchorOffset;
  anchorBlock.lmb_Fingerprint = FINGERPRINT_ANCHORBLOCK;
  anchorBlock.lmb_FlinkOffset = freeOffset;
  anchorBlock.lmb_BlinkOffset = freeOffset;
  anchorBlock.lmb_BlockCount = blockBlocks;
  anchorBlock.lmb_HeaderBlockCount = blockBlocks;
  freeBlock.lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
  freeBlock.lmb_FlinkOffset = anchorOffset;
  freeBlock.lmb_BlinkOffset = anchorOffset;
  freeBlock.lmb_BlockCount = blockCount - labelBlocks - blockBlocks;
  freeBlock.lmb_HeaderBlockCount = blockBlocks;
  ioInfo.ioi_Send.iob_Buffer = &anchorBlock;
  ioInfo.ioi_Send.iob_Len = blockBlocks * blockSize;
  ioInfo.ioi_Recv.iob_Buffer = NULL;
  ioInfo.ioi_Recv.iob_Len = 0;
  ioInfo.ioi_Unit = (uint8) unit;
  ioInfo.ioi_Flags = 0;
  ioInfo.ioi_Command = CMD_WRITE;
  ioInfo.ioi_CmdOptions = 0;
  ioInfo.ioi_Offset = anchorOffset + offset;
  printf("Writing anchor to absolute block %d\n", ioInfo.ioi_Offset);
  err = DoIO(ioReqItem, &ioInfo);
  if (err < 0 || (err = ioReq->io_Error) < 0) {
    fail(err,"write anchor",0);
  }
  ioInfo.ioi_Send.iob_Buffer = &freeBlock;
  ioInfo.ioi_Offset = freeOffset + offset;
  printf("Writing free-space to absolute block %d\n", ioInfo.ioi_Offset);
  err = DoIO(ioReqItem, &ioInfo);
  if (err < 0 || (err = ioReq->io_Error) < 0) {
    fail(err,"write free-space",0);
  }
  ioInfo.ioi_Send.iob_Buffer = &discLabel;
  ioInfo.ioi_Send.iob_Len = labelBlocks * blockSize;
  ioInfo.ioi_Offset = offset;
  printf("Writing label to absolute block %d\n", ioInfo.ioi_Offset);
  err = DoIO(ioReqItem, &ioInfo);
  if (err < 0 || (err = ioReq->io_Error) < 0) {
    fail(err,"write label",0);
  }
  printf("Filesystem initialization complete.\n");
  return 0;
}


/*
 * unslash the path
 */
char *
sub_slash(char *cp)
{

 if (*cp != '/')
   return(cp);

 while (*cp++ == '/')
   ;
 return(cp - 1);
}


/*
 * slash the path
 */
char *
add_slash(char *cp)
{
 static	char	mdirnm[MAX_FILE_NAME_LEN + 1];

 if (*cp != '/') {
  strcpy(mdirnm, "/");
  strncat(mdirnm, cp, MAX_FILE_NAME_LEN - 1);
  return(mdirnm);
 } else
  return(cp);
}


