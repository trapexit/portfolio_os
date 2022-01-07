/*****

$Id: DirectoryAccess.c,v 1.12 1994/02/18 01:54:55 limes Exp $

$Log: DirectoryAccess.c,v $
 * Revision 1.12  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.11  1993/07/28  02:30:50  dplatt
 * Zero out IOReqs;  do delete-item cleanup
 *
 * Revision 1.10  1993/04/22  21:03:15  dplatt
 * New features, timeout support, bug fixes
 *
 * Revision 1.9  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.8  1993/02/11  19:39:37  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.7  1992/12/08  05:59:52  dplatt
 * Magenta changes
 *
 * Revision 1.6  1992/10/24  00:40:56  dplatt
 * Bluebird changes and bug fixes
 *
 * Revision 1.5  1992/10/20  06:02:33  dplatt
 * Blue changes
 *
 * Revision 1.4  1992/10/16  01:22:24  dplatt
 * First cut at bluebird release changes
 *
 * Revision 1.3  1992/10/01  23:36:21  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.2  1992/09/11  22:37:38  dplatt
 * New compiler
 *
 * Revision 1.1  1992/09/11  00:41:56  dplatt
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
  DirectoryAccess.c - library code for accessing a directory.
*/

#include "types.h"
#include "item.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "mem.h"
#include "device.h"
#include "driver.h"
#include "msgport.h"
#include "io.h"
#include "filesystem.h"
#include "operror.h"

#include "directory.h"

#include "filefunctions.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "stdio.h"
#include "strings.h"

/* #define DEBUG */

Directory *OpenDirectoryItem(Item openFileItem)
{
  Directory *dir;
  FileStatus fileStatus;
  IOInfo theInfo;
  Item ioReqItem;
  int32 err;
  TagArg ioReqTags[2];
#ifdef DEBUG
  printf("Creating directory-walker for open-file item %d\n", openFileItem);
#endif
  dir = (Directory *) ALLOCMEM(sizeof (Directory), MEMTYPE_FILL);
  if (!dir) {
    printf("Couldn't allocate!\n");
    goto returnNull;
  }
#ifdef DEBUG
  printf("User memory block allocated at %x\n", dir);
#endif
  dir->dir_OpenFileItem = openFileItem;
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) openFileItem;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    printf("Couldn't allocate I/O request\n");
    goto releaseDirectory;
  }
  dir->dir_IOReqItem = ioReqItem;
  dir->dir_IOReq = (IOReq *) LookupItem(ioReqItem);
  memset(&theInfo, 0, sizeof theInfo);
  theInfo.ioi_Command = CMD_STATUS;
  theInfo.ioi_Flags = IO_QUICK;
  theInfo.ioi_Send.iob_Buffer = NULL;
  theInfo.ioi_Send.iob_Len = 0;
  theInfo.ioi_Recv.iob_Buffer = (void *) &fileStatus;
  theInfo.ioi_Recv.iob_Len = sizeof fileStatus;
  if ((err = DoIO(ioReqItem, &theInfo)) != 0) {
    printf("I/O error %x getting status\n", err);
    goto releaseIOReq;
  }
  if (!(fileStatus.fs.ds_DeviceFlagWord & FILE_IS_DIRECTORY)) {
    printf("Flags %lx means not a directory\n",
	    fileStatus.fs.ds_DeviceFlagWord);
    goto releaseIOReq;
  }
  dir->dir_Flags = fileStatus.fs.ds_DeviceFlagWord;
  dir->dir_BlockSize = fileStatus.fs.ds_DeviceBlockSize;
  dir->dir_BlockCount = fileStatus.fs.ds_DeviceBlockCount;
  dir->dir_BlockNumber = 0;
  dir->dir_BlockOffset = 0;
  dir->dir_EntryNum = 0;
#ifdef DEBUG
  printf("Directory open, flags %lx, %d blocks of %d bytes\n", dir->dir_Flags,
	  dir->dir_BlockCount, dir->dir_BlockSize);
#endif
  if ((fileStatus.fs.ds_DeviceFlagWord & FILE_SUPPORTS_DIRSCAN)) {
    dir->dir_BlockBuf = (char *) NULL;
  } else {
    dir->dir_BlockBuf = (char *) ALLOCMEM((int32) dir->dir_BlockSize, MEMTYPE_FILL);
    if (!dir->dir_BlockBuf) {
#ifdef DEBUG
      printf("Couldn't allocate a %d-byte buffer\n", dir->dir_BlockSize);
#endif
      goto releaseIOReq;
    }
  }
  return dir;
 releaseIOReq:
  DeleteItem(ioReqItem);
 releaseDirectory:
  FREEMEM(dir, sizeof (Directory));
 returnNull:
  return (Directory *) NULL;
}

Directory *OpenDirectoryPath(char *thePath)
{
  Item openFileItem;
  openFileItem = OpenDiskFile(thePath);
  if (openFileItem < 0) {
#ifdef DEBUG
    PrintError(0,"open",thePath,openFileItem);
#endif
    return (Directory *) NULL;
  }
  return OpenDirectoryItem(openFileItem);
}

int32 ReadDirectory (Directory *dir, DirectoryEntry *de)
{
  IOInfo theInfo;
  int32 err;
  DirectoryRecord *dr;
  DirectoryHeader *dh;
  if (dir->dir_Flags & FILE_SUPPORTS_DIRSCAN) {
    memset(&theInfo, 0, sizeof theInfo);
    theInfo.ioi_Command = FILECMD_READDIR;
    theInfo.ioi_Flags = IO_QUICK;
    theInfo.ioi_Offset = (int32) ++dir->dir_EntryNum;
    theInfo.ioi_Recv.iob_Buffer = (void *) de;
    theInfo.ioi_Recv.iob_Len = sizeof (DirectoryEntry);
    strcpy(de->de_FileName, "## KABONG ##");
#ifdef DEBUG
    printf("Read directory entry %d\n", dir->dir_EntryNum);
#endif
    err = DoIO(dir->dir_IOReqItem, &theInfo);
    if (err == 0) {
      err = dir->dir_IOReq->io_Error;
    }
    return err;
  }
  while (dir->dir_BlockOffset == 0) {
#ifdef DEBUG
    printf("Read directory block %d\n", dir->dir_BlockNumber);
#endif
    if (dir->dir_BlockNumber < 0 ||
	dir->dir_BlockNumber >= dir->dir_BlockCount) {
#ifdef DEBUG
      printf("Past end of directory\n");
#endif
      return -1;
    }
    memset(&theInfo, 0, sizeof theInfo);
    theInfo.ioi_Command = CMD_READ;
    theInfo.ioi_Offset = (int32) dir->dir_BlockNumber;
    theInfo.ioi_Recv.iob_Buffer = (void *) dir->dir_BlockBuf;
    theInfo.ioi_Recv.iob_Len = (int32) dir->dir_BlockSize;
    err = DoIO(dir->dir_IOReqItem, &theInfo);
    if (err != 0 || (err = dir->dir_IOReq->io_Error) != 0) {
#ifdef DEBUG
      PrintError(0,"read directory",0,(Item) err);	/* XXX- do we have the dir name handy? */
#endif
      return err;
    }
    dh = (DirectoryHeader *) dir->dir_BlockBuf;
    dir->dir_BlockOffset = dh->dh_FirstEntryOffset;
    dir->dir_BlockNumber = dh->dh_NextBlock;
#ifdef DEBUG
    printf("Block offset is %d, next block is %d\n", dir->dir_BlockOffset,
	   dir->dir_BlockNumber);
#endif
  }
  dr = (DirectoryRecord *) (dir->dir_BlockBuf + dir->dir_BlockOffset);
  de->de_Flags = dr->dir_Flags;
  de->de_UniqueIdentifier = dr->dir_UniqueIdentifier;
  de->de_Type = dr->dir_Type;
  de->de_BlockSize = dr->dir_BlockSize;
  de->de_ByteCount = dr->dir_ByteCount;
  de->de_BlockCount = dr->dir_BlockCount;
  de->de_Burst = dr->dir_Burst;
  de->de_Gap = dr->dir_Gap;
  de->de_AvatarCount = dr->dir_LastAvatarIndex + 1;
  strncpy(de->de_FileName, dr->dir_FileName, FILESYSTEM_MAX_NAME_LEN);
  if (dr->dir_Flags & DIRECTORY_LAST_IN_BLOCK) {
    dir->dir_BlockOffset = 0;
    if (dr->dir_Flags & DIRECTORY_LAST_IN_DIR) {
      dir->dir_BlockNumber = -1;
    }
  } else {
    dir->dir_BlockOffset += sizeof (DirectoryRecord) +
      (sizeof (ulong) *	dr->dir_LastAvatarIndex);
  }
  return 0;
}

void CloseDirectory (Directory *dir) {
  if (dir->dir_BlockBuf) {
    FREEMEM(dir->dir_BlockBuf, (int32) dir->dir_BlockSize);
  }
  DeleteItem(dir->dir_IOReqItem);
  CloseDiskFile(dir->dir_OpenFileItem);
  FREEMEM(dir, sizeof (Directory));
  return;
}
