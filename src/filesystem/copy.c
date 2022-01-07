/*****

$Id: copy.c,v 1.4 1994/02/18 01:54:55 limes Exp $

$Log: copy.c,v $
 * Revision 1.4  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.3  1993/07/28  02:30:50  dplatt
 * Zero out IOReqs;  do delete-item cleanup
 *
 * Revision 1.2  1993/06/14  01:00:23  dplatt
 * Dragon beta release
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
  cp.c - Copy an input file to an output file (usually on /nvram).
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

#define fail(foo,verb,noun) { if (foo < 0) { PrintError(0,verb,noun,foo);  return (int) foo; } }

int main (int32 argc, char **argv)
{
  Item oldFileItem, newFileItem;
  Item oldReqItem, newReqItem;
  int32 bytes, blocks, addBlocks, index;
  IOReq *oldReq, *newReq;
  IOInfo oldInfo, newInfo;
  FileStatus oldStatus, newStatus;
  void *buffer;
  int32 bufferSize;
  int32 oldChunks, newChunks;
  Err err;
  if (argc != 3) {
    printf("Usage: cp inputfile outputfile\n");
    return -1;
  }
  oldFileItem = OpenDiskFile(argv[1]);
  fail(oldFileItem,"open",argv[1]);
  oldReqItem = CreateIOReq(NULL, 0, oldFileItem, 0);
  fail(oldReqItem,"create IOReq for",argv[1]);
  oldReq = (IOReq *) LookupItem(oldReqItem);
  memset(&oldInfo, 0, sizeof oldInfo);
  oldInfo.ioi_Command = CMD_STATUS;
  oldInfo.ioi_Recv.iob_Buffer = &oldStatus;
  oldInfo.ioi_Recv.iob_Len = sizeof oldStatus;
  DoIO(oldReqItem, &oldInfo);
  fail(oldReq->io_Error,"get status of",argv[1]);
  bytes = oldStatus.fs_ByteCount;
  printf("Existing file has %d bytes\n", bytes);
  (void) DeleteFile(argv[2]); /* trash it if it's not busy */
  err = CreateFile(argv[2]);  /* create a new one */
  fail(err,"create",argv[2]);
  newFileItem = OpenDiskFile(argv[2]);
  fail(newFileItem,"open",argv[2]);
  newReqItem = CreateIOReq(NULL, 0, newFileItem, 0);
  fail(newReqItem,"create IOReq for",argv[2]);
  newReq = (IOReq *) LookupItem(newReqItem);
  memset(&newInfo, 0, sizeof newInfo);
  newInfo.ioi_Command = CMD_STATUS;
  newInfo.ioi_Recv.iob_Buffer = &newStatus;
  newInfo.ioi_Recv.iob_Len = sizeof newStatus;
  DoIO(newReqItem, &newInfo);
  fail(newReq->io_Error,"get status of",argv[2]);
  blocks = (bytes + newStatus.fs.ds_DeviceBlockSize - 1) / newStatus.fs.ds_DeviceBlockSize;
  printf("%d blocks needed, %d already available\n", blocks,
	 newStatus.fs.ds_DeviceBlockCount);
  addBlocks = blocks - newStatus.fs.ds_DeviceBlockCount;
  if (addBlocks > 0) {
    newInfo.ioi_Command = FILECMD_ALLOCBLOCKS;
    newInfo.ioi_Recv.iob_Buffer = NULL;
    newInfo.ioi_Recv.iob_Len = 0;
    newInfo.ioi_Offset = addBlocks;
    err = DoIO(newReqItem, &newInfo);
    fail(err,"allocate blocks for",argv[2]);
    fail(newReq->io_Error,"allocate blocks for",argv[2]);
    printf("%d blocks allocated\n", addBlocks);
  }
  bufferSize = 0;
  do {
    bufferSize += oldStatus.fs.ds_DeviceBlockSize;
  } while (bufferSize % newStatus.fs.ds_DeviceBlockSize != 0);  /* cheapo LCM */
  buffer = ALLOCMEM(bufferSize, MEMTYPE_DMA);
  if (!buffer) {
    printf("Could not allocate a %d-byte buffer!\n", bufferSize);
    return -1;
  }
  oldChunks = bufferSize / oldStatus.fs.ds_DeviceBlockSize;
  oldInfo.ioi_Command = CMD_READ;
  oldInfo.ioi_Recv.iob_Buffer = buffer;
  oldInfo.ioi_Recv.iob_Len = bufferSize;
  oldInfo.ioi_Offset = 0;
  newChunks = bufferSize / newStatus.fs.ds_DeviceBlockSize;
  newInfo.ioi_Command = CMD_WRITE;
  newInfo.ioi_Send.iob_Buffer = buffer;
  newInfo.ioi_Send.iob_Len = bufferSize;
  newInfo.ioi_Offset = 0;
  index = 0;
  printf("Copying...");
  while (index < bytes) {
    DoIO(oldReqItem, &oldInfo);
    fail(oldReq->io_Error,"read",argv[1]);
    if (bytes - index < bufferSize) {
      newInfo.ioi_Send.iob_Len = (blocks - newInfo.ioi_Offset) *
	newStatus.fs.ds_DeviceBlockSize;
    }
    err = DoIO(newReqItem, &newInfo);
    fail(err,"write",argv[2]);
    fail(newReq->io_Error,"write",argv[2]);
    oldInfo.ioi_Offset += oldChunks;
    newInfo.ioi_Offset += newChunks;
    index += bufferSize;
  }
  printf(" setting EOF");
  newInfo.ioi_Command = FILECMD_SETEOF;
  newInfo.ioi_Send.iob_Buffer = NULL;
  newInfo.ioi_Send.iob_Len = 0;
  newInfo.ioi_Offset = bytes;
  err = DoIO(newReqItem, &newInfo);
  fail(err,"set size of",argv[2]);
  fail(newReq->io_Error,"set size of",argv[2]);
  printf(" done.\n");
  CloseDiskFile(oldFileItem);
  CloseDiskFile(newFileItem);
  return 0;
}
