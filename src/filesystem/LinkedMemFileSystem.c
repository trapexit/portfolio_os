/*****

$Id: LinkedMemFileSystem.c,v 1.16 1994/05/06 21:59:56 shawn Exp $

$Log: LinkedMemFileSystem.c,v $
 * Revision 1.16  1994/05/06  21:59:56  shawn
 * perform boundery check for FSSTAT only.
 *
 * Revision 1.15  1994/04/19  18:05:07  shawn
 * Set the file device is working on to the currently open file.
 *
 * Revision 1.14  1994/04/07  21:48:53  dplatt
 * Fix the busyness race condition here, too.
 *
 * Revision 1.13  1994/03/31  00:45:59  dplatt
 * Writes which go past the previous end-of-file will now automagically
 * set the end-of-file to include all bytes actually written.
 *
 * Revision 1.12  1994/03/24  01:50:06  dplatt
 * Add support for kernel events, hot-mount/hot-dismount.  Minor
 * security enhancements and rearrangements of various things.
 *
 * Revision 1.11  1994/02/09  21:34:04  shawn
 * do not too much into the user address space.
 *
 * Revision 1.10  1994/02/04  22:10:41  shawn
 * Changes to support FSSTAT.
 *
 * Revision 1.9  1993/12/16  00:48:20  dplatt
 * Add null hook for OpenEntry
 *
 * Revision 1.8  1993/06/29  19:43:06  dplatt
 * Don't permit attempts to write data past the physical EOF.  For that
 * matter, don't allow reads past physical EOF either.
 *
 * Revision 1.7  1993/06/24  03:02:06  dplatt
 * Fix nvram problems
 *
 * Revision 1.6  1993/06/15  20:33:46  dplatt
 * use string.h rather than strings.h
 *
 * Revision 1.5  1993/06/15  20:30:15  dplatt
 * Remove extraneous extern for strcasecmp
 *
 * Revision 1.4  1993/06/15  04:08:43  dplatt
 * strncasecmp becomes strcasecmp
 *
 * Revision 1.3  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.2  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
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
  LinkedMemFileSystem.c - support code for flat linked-memory-block
  file systems.
*/

#define SUPER

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
#include "filesystemdefs.h"
#include "discdata.h"
#include "directory.h"
#include "operror.h"

#include "super.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "string.h"

/* #define DEBUG */

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x)  /* x */
#endif

#ifdef PRODUCTION
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

extern int32 CloseOpenFile(OpenFile *theOpenFile);
extern Node *AllocFileNode(int32 theSize, int32 theType);
extern void FreeFileNode (void *it);

int32 QueueLinkedMemDiskRequest(LinkedMemDisk *theDevice, IOReq *theRequest);
void ScheduleLinkedMemDiskIo(LinkedMemDisk *theDevice);
void StartLinkedMemDiskIo (LinkedMemDisk *theDevice);
void AbortLinkedMemDiskIo (IOReq *theRequest);
void LinkedMemDoStat(LinkedMemFileEntry *fep, FileSystemStat *fsp,
		     uint32 curblk);
IOReq *LinkedMemDiskEndAction(IOReq *theRequest);
Err LinkedMemFiniteStateMachine(LinkedMemDisk *theDevice, FileIOReq *userReq);

int32 QueueLinkedMemDiskRequest(LinkedMemDisk *theDevice, IOReq *theRequest)
{
  uint32 interrupts;
  File *theFile;
#ifdef DEBUG
  qprintf(("LinkedMemDisk I/O scheduler\n"));
#endif
  switch (theRequest->io_Info.ioi_Command) {
  case CMD_READ:
    theFile = ((OpenFile *) theRequest->io_Dev)->ofi_File;
    if (theRequest->io_Info.ioi_Recv.iob_Len % theFile->fi_BlockSize != 0 ||
	theRequest->io_Info.ioi_Recv.iob_Len > theFile->fi_BlockSize *
	(theFile->fi_BlockCount - theRequest->io_Info.ioi_Offset)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case CMD_WRITE:
    theFile = ((OpenFile *) theRequest->io_Dev)->ofi_File;
    if (theRequest->io_Info.ioi_Send.iob_Len % theFile->fi_BlockSize != 0 ||
	theRequest->io_Info.ioi_Send.iob_Len > theFile->fi_BlockSize *
	(theFile->fi_BlockCount - theRequest->io_Info.ioi_Offset)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case FILECMD_OPENENTRY:
    SuperCompleteIO(theRequest);
    return 0;
  case FILECMD_ADDENTRY:
  case FILECMD_DELETEENTRY:
  case FILECMD_ALLOCBLOCKS:
  case FILECMD_SETEOF:
  case FILECMD_SETTYPE:
    break;
  case FILECMD_READDIR:
  case FILECMD_READENTRY:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case FILECMD_FSSTAT:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    if (theRequest->io_Info.ioi_Recv.iob_Len < sizeof(FileSystemStat)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
    }
    break;
  default:
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  interrupts = Disable();
  InsertNodeFromTail(&theDevice->lmd.hdi_RequestsToDo, (Node *) theRequest);
  theRequest->io_Flags &= ~IO_QUICK;
  Enable(interrupts);
  return 0;
}

void ScheduleLinkedMemDiskIo(LinkedMemDisk *theDevice)
{
  FileIOReq *theRequest;
  if (theDevice->lmd.hdi_DeviceBusy) {
    return;
  }
  while (!ISEMPTYLIST(&theDevice->lmd.hdi_RequestsToDo)) {
    theRequest = (FileIOReq *) FIRSTNODE(&theDevice->lmd.hdi_RequestsToDo);
    RemNode((Node *) theRequest);
    AddTail(&theDevice->lmd.hdi_RequestsRunning, (Node *) theRequest);
  }
}

Err PrepareLinkedMemDiskIO(LinkedMemDisk *theDevice, FileIOReq *theRequest)
{
  IOReq *rawRequest;
  File *theFile;
  int32 avatar, absoluteOffset, absoluteBlock;
  Err err;
  int32 len;
  theFile = ((OpenFile *) theRequest->fio.io_Dev)->ofi_File;
  rawRequest = theDevice->lmd_RawDeviceRequest;
  rawRequest->io_CallBack = theDevice->lmd.hdi_EndAction;
  rawRequest->io_Info = theRequest->fio.io_Info;
  rawRequest->io_Info.ioi_Unit = theDevice->lmd.hdi_RawDeviceUnit;
  rawRequest->io_Info.ioi_User = (ulong) theRequest;
  rawRequest->io_Info.ioi_CmdOptions = 0; /* use defaults */
  rawRequest->io_Info.ioi_Flags = 0;
  switch (theRequest->fio.io_Info.ioi_Command) {
  case CMD_READ:
  case CMD_WRITE:
    avatar = theFile->fi_AvatarList[0];
    absoluteOffset = theRequest->fio.io_Info.ioi_Offset;
    absoluteBlock = ((avatar & DRIVER_BLOCK_MASK) + absoluteOffset) *
      theFile->fi_FileSystem->fs_DeviceBlocksPerFilesystemBlock +
	theDevice->lmd_FileHeaderBlockSize;
    theRequest->fio_AvatarIndex = 0;
    theRequest->fio_AbsoluteBlockNumber = absoluteBlock;
    theRequest->fio_BlockCount = theRequest->fio.io_Info.ioi_Recv.iob_Len /
      theFile->fi_BlockSize;
    theRequest->fio_DevBlocksPerFileBlock =
      theFile->fi_FileSystem->fs_DeviceBlocksPerFilesystemBlock;
    rawRequest->io_Info.ioi_Command = theRequest->fio.io_Info.ioi_Command;
    rawRequest->io_Info.ioi_Offset =
      (int32) theRequest->fio_AbsoluteBlockNumber +
	theDevice->lmd_RawDeviceBlockOffset;
    theDevice->lmd_CurrentFileActingOn = theFile;
    if (theRequest->fio.io_Info.ioi_Command == CMD_READ) {
      theDevice->lmd_FSM = LMD_Idle;
    } else {
      theDevice->lmd_FSM = LMD_ExtendEOF;
    }
    DBUG(("User requests offset %d, fs block %d, device block %d\n",
	  absoluteOffset, absoluteBlock, rawRequest->io_Info.ioi_Offset));
    err = 0;
    break;
  case FILECMD_ADDENTRY:
    theDevice->lmd_CurrentFileActingOn = theFile;
    theDevice->lmd_DesiredSize =
      (sizeof (LinkedMemFileEntry) +
       theFile->fi_FileSystem->fs_VolumeBlockSize - 1) /
	   theFile->fi_FileSystem->fs_VolumeBlockSize;
    theDevice->lmd_FSM = LMD_Initialization;
    err = LinkedMemFiniteStateMachine(theDevice, theRequest);
    break;
  case FILECMD_FSSTAT:
    theDevice->lmd_CurrentFileActingOn = theFile;
    theDevice->lmd_FSM = LMD_Initialization;
    err = LinkedMemFiniteStateMachine(theDevice, theRequest);
    break;
  case FILECMD_ALLOCBLOCKS:
    theDevice->lmd_CurrentFileActingOn = theFile;
    theDevice->lmd_DesiredSize = theFile->fi_BlockCount +
      theRequest->fio.io_Info.ioi_Offset +
	theDevice->lmd_FileHeaderBlockSize;
    DBUG(("AllocBlocks:  desire a chunk of %d blocks\n", theDevice->lmd_DesiredSize));
    theDevice->lmd_FSM = LMD_Initialization;
    theDevice->lmd_ThisBlockCursor = theFile->fi_AvatarList[0];
    err = LinkedMemFiniteStateMachine(theDevice, theRequest);
    break;
  case FILECMD_READENTRY:
  case FILECMD_DELETEENTRY:
    memset(theDevice->lmd_CopyBuffer, 0, sizeof theDevice->lmd_CopyBuffer);
    len = theRequest->fio.io_Info.ioi_Send.iob_Len;
    if (len > FILESYSTEM_MAX_NAME_LEN) {
      len = FILESYSTEM_MAX_NAME_LEN;
    }
    strncpy(theDevice->lmd_CopyBuffer,
	    (char *) theRequest->fio.io_Info.ioi_Send.iob_Buffer, len);
    /* drop through */
  case FILECMD_READDIR:
    theDevice->lmd_CurrentFileActingOn = theFile;
    theDevice->lmd_FSM = LMD_InitScan;
    err = LinkedMemFiniteStateMachine(theDevice, theRequest);
    break;
  case FILECMD_SETEOF:
    DBUG(("Want to set EOF to %d\n", theRequest->fio.io_Info.ioi_Offset));
    if (theRequest->fio.io_Info.ioi_Offset >
	theFile->fi_BlockCount * theFile->fi_BlockSize) {
      DBUG(("Too big!\n"));
      err = MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
    } else {
      DBUG(("OK, run FSM\n"));
      theDevice->lmd_FSM = LMD_ReadToSetEOF;
      theDevice->lmd_ThisBlockCursor = theFile->fi_AvatarList[0];
      err = LinkedMemFiniteStateMachine(theDevice, theRequest);
    }
    break;
  case FILECMD_SETTYPE:
    DBUG(("Want to set type to %d\n", theRequest->fio.io_Info.ioi_Offset));
    theDevice->lmd_FSM = LMD_ReadToSetType;
    theDevice->lmd_ThisBlockCursor = theFile->fi_AvatarList[0];
    err = LinkedMemFiniteStateMachine(theDevice, theRequest);
    break;
  default:
    err = MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  return err;
}
    
void StartLinkedMemDiskIo (LinkedMemDisk *theDevice)
{
  FileIOReq *theRequest;
  IOReq *rawRequest;
  int32 err;
  uint32 interrupts;
  do {
    interrupts = Disable();
    if (theDevice->lmd.hdi_DeviceBusy ||
	ISEMPTYLIST(&theDevice->lmd.hdi_RequestsRunning)) {
      Enable(interrupts);
      return;
    }
    theDevice->lmd.hdi_DeviceBusy = TRUE;
    theRequest = (FileIOReq *) FIRSTNODE(&theDevice->lmd.hdi_RequestsRunning);
    Enable(interrupts);
    err = PrepareLinkedMemDiskIO (theDevice, theRequest);
    if (err < 0) {
      interrupts = Disable();
      theDevice->lmd.hdi_DeviceBusy = FALSE;
      RemNode((Node *) theRequest);
      Enable(interrupts);
      theRequest->fio.io_Error = err;
      SuperCompleteIO((IOReq *) theRequest);
    } else {
      rawRequest = theDevice->lmd_RawDeviceRequest;
      DBUG(("Raw I/O offset %d, send [0x%lx,%d]",
		   rawRequest->io_Info.ioi_Offset,		 
		   rawRequest->io_Info.ioi_Send.iob_Buffer,
		   rawRequest->io_Info.ioi_Send.iob_Len));		 
      DBUG((" recv [0x%lx,%d]\n",
		   rawRequest->io_Info.ioi_Recv.iob_Buffer,
		   rawRequest->io_Info.ioi_Recv.iob_Len));
      err = SuperinternalSendIO(rawRequest);
      if (err < 0) {
	qprintf(("Error %d from SuperinternalSendIO!\n", err));
      } else {
	DBUG(("Sent\n"));
      }
    }
  } while (!theDevice->lmd.hdi_DeviceBusy);
  return;
}

void AbortLinkedMemDiskIo(IOReq *theRequest)
{
  LinkedMemDisk *theDevice;
  OpenFile *theOpenFile;
  theOpenFile = (OpenFile *) theRequest->io_Dev;
  theDevice = (LinkedMemDisk *)theOpenFile->ofi_File->fi_FileSystem->fs_Device;
/*
  If this is the request that the device is currently servicing, then
  simply abort the lower-level I/O request - the endaction code will
  perform the cleanup for both levels.  If this request is not yet being
  serviced, dequeue and kill it immediately.
*/
  if (theDevice->lmd.hdi_DeviceBusy &&
   theDevice->lmd_RawDeviceRequest->io_Info.ioi_User == (ulong) theRequest) {
    SuperinternalAbortIO(theDevice->lmd_RawDeviceRequest);
  } else {
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
    RemNode((Node *) theRequest);
    SuperCompleteIO(theRequest);
  }
  return;
}

IOReq *LinkedMemDiskEndAction(IOReq *rawRequest)
{
  FileIOReq *userRequest;
  OpenFile *theOpenFile;
  LinkedMemDisk *theDevice;
  File *theFile;
  int32 bytesRead;
  Err err;
  userRequest = (FileIOReq *) rawRequest->io_Info.ioi_User;
  theOpenFile = (OpenFile *) userRequest->fio.io_Dev;
  theFile = theOpenFile->ofi_File;
  theDevice = (LinkedMemDisk *) theFile->fi_FileSystem->fs_Device;
  DBUG(("LMD endaction routine, state %d, error 0x%x, actual %d\n",
	theDevice->lmd_FSM,
	rawRequest->io_Error,
	rawRequest->io_Actual));
  switch (theDevice->lmd_FSM) {
  case LMD_Idle:
    bytesRead = rawRequest->io_Actual;
    if ((rawRequest->io_Error & 0x0000001F) == ((ER_C_STND << ERR_CLASHIFT) +
						(ER_Aborted << ERR_ERRSHIFT))) {
      userRequest->fio.io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
    } else {      
      userRequest->fio.io_Error = rawRequest->io_Error;
    }
    userRequest->fio.io_Actual += bytesRead;
    break;
  case LMD_Done:
    theDevice->lmd_FSM = LMD_Idle;
    break;
  default: /* FSM is still running, prod it */
    err = rawRequest->io_Error;
    if (err < 0) {
      DBUG(("Aborting FSM due to I/O error!\n"));
      userRequest->fio.io_Error = err;
      theDevice->lmd_FSM = LMD_Idle;
      break;
    }
    err = LinkedMemFiniteStateMachine(theDevice, userRequest);
    if (err > 0) {
      DBUG(("Followon raw I/O offset %d, send [0x%lx,%d]",
	     rawRequest->io_Info.ioi_Offset,		 
	     rawRequest->io_Info.ioi_Send.iob_Buffer,
	     rawRequest->io_Info.ioi_Send.iob_Len));		 

      DBUG((" recv [0x%lx,%d]\n",
	     rawRequest->io_Info.ioi_Recv.iob_Buffer,
	     rawRequest->io_Info.ioi_Recv.iob_Len));
      return rawRequest; /* keep running */
    }
    DBUG(("Terminate FSM with error 0x%x\n", err));
    userRequest->fio.io_Error = err;
  }
  theDevice->lmd.hdi_DeviceBusy = FALSE;
/*
  That request is done (successfully or with errors from which we cannot
  recover).  Signal completion and check for a follow-on I/O from an
  upper-level driver.
*/
 wrapup:
  RemNode((Node *) userRequest);
  SuperCompleteIO((IOReq *) userRequest);
  if (ISEMPTYLIST(&theDevice->lmd.hdi_RequestsRunning)) {
/*
  Either we've completed the last scheduled I/O, or we've set aside
  some pending requests because of a higher-priority boink.  In either
  case, reschedule to find something to do.
  
  If the filesystem daemon is alive, signal it to reschedule...
  otherwise, do the scheduling here at interrupt level (which is not
  generally a thing we want to do, due to the amount of time it requires)
*/
    if (fileFolio->ff_Daemon.ffd_Task != (Task *) NULL) {
      SuperinternalSignal(fileFolio->ff_Daemon.ffd_Task,
			  fileFolio->ff_Daemon.ffd_RescheduleSignal);
      return (IOReq *) NULL;
    }
    (*theDevice->lmd.hdi_ScheduleIO)((HighLevelDisk *) theDevice);
    if (ISEMPTYLIST(&theDevice->lmd.hdi_RequestsRunning)) {
      return (IOReq *) NULL;
    }
  }
  userRequest = (FileIOReq *) FIRSTNODE(&theDevice->lmd.hdi_RequestsRunning);
  err = PrepareLinkedMemDiskIO (theDevice, userRequest);
  if (err < 0) {
    userRequest->fio.io_Error = err;
    goto wrapup;
  }
  theDevice->lmd.hdi_DeviceBusy = TRUE;
  rawRequest->io_Actual = 0; /* debug */
  return rawRequest;
}

Err LinkedMemFiniteStateMachine(LinkedMemDisk *theDevice, FileIOReq *userReq)
{
  IOReq *raw;
  File *theFile;
  enum LinkedMemDiskFSM nextState;
  enum {
    noAction,
    readThis,
    readOther,
    writeThis,
    writeOther,
    readData,
    writeData} action;
  Err returnVal;
  LinkedMemFileEntry tempBlock;
  FileSystemStat *fstp = NULL;
  DirectoryEntry de;
  int32 deSize;
  uint32 newEOF;

  DBUG(("LMD FSM, state %d\n", theDevice->lmd_FSM));
  theFile = theDevice->lmd_CurrentFileActingOn;
  nextState = LMD_Fault;
  action = noAction;  
  returnVal = 1;
  raw = theDevice->lmd_RawDeviceRequest;
  switch (theDevice->lmd_FSM) {
  case LMD_Idle:
  case LMD_Done:
    nextState = LMD_Idle;
    returnVal = 0;
    break;
  case LMD_Fault:
    returnVal = 0;
    break;
  case LMD_FsStat:
    if (theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset <
    	theDevice->lmd_ThisBlockCursor) {
    	action = noAction;
	/* this is the last block */
    	fstp = (FileSystemStat *) userReq->fio.io_Info.ioi_Recv.iob_Buffer;
	LinkedMemDoStat(&theDevice->lmd_ThisBlock, fstp, 
    			theDevice->lmd_ThisBlockCursor);
	fstp->fst_BitMap |= (FSSTAT_BLOCKSIZE | FSSTAT_MAXFILESIZE |
			     FSSTAT_FREE | FSSTAT_USED);
    	nextState = LMD_Done;
    } else {	/* more traversing is needed */
	LinkedMemDoStat(&theDevice->lmd_ThisBlock,
    		(FileSystemStat *) userReq->fio.io_Info.ioi_Recv.iob_Buffer,
    			theDevice->lmd_ThisBlockCursor);
    	theDevice->lmd_ThisBlockCursor = 
    	theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset;
    	action = readThis;
    	nextState = LMD_FsStat;
    }
    break;

  case LMD_Initialization:
    theDevice->lmd_MergeBlockCursor = 0;
    if (userReq->fio.io_Info.ioi_Command == FILECMD_ADDENTRY) {
      goto TryNewBlock;
    }

    if (userReq->fio.io_Info.ioi_Command == FILECMD_FSSTAT) {
	fstp = (FileSystemStat *) userReq->fio.io_Info.ioi_Recv.iob_Buffer;
    	memset(fstp, 0, sizeof(FileSystemStat));
        fstp->fst_BlockSize = theFile->fi_FileSystem->fs_VolumeBlockSize;
        /* fstp->fst_Size = theFile->fi_FileSystem->fs_VolumeBlockCount; */
        fstp->fst_Used = HOWMANY(sizeof(DiscLabel), fstp->fst_BlockSize);
    	theDevice->lmd_ThisBlockCursor =
      	theFile->fi_FileSystem->fs_RootDirectory->fi_AvatarList[0];
    	action = readThis;
    	nextState = LMD_FsStat;
    	break;
    }

    action = readThis;
    nextState = LMD_CheckIsThisLast;
    break;
  case LMD_CheckIsThisLast:
    if (theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset < theDevice->lmd_ThisBlockCursor) {
      goto TryNewBlock;
    }
    theDevice->lmd_OtherBlockCursor = theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset;
    action = readOther;
    nextState = LMD_CheckSuccessor;
    break;
  case LMD_CheckSuccessor:
    if (theDevice->lmd_OtherBlock.lmfe.lmb_Fingerprint != FINGERPRINT_FREEBLOCK ||
	theDevice->lmd_OtherBlock.lmfe.lmb_BlockCount < theDevice->lmd_DesiredSize -
	theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount) {
      goto TryNewBlock;
    }
    theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount +=
      theDevice->lmd_OtherBlock.lmfe.lmb_BlockCount;
    theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset =
      theDevice->lmd_OtherBlock.lmfe.lmb_FlinkOffset;
    action = writeThis;
    nextState = LMD_CutTheSlack;
    break;
  case LMD_CutTheSlack:
    if (userReq->fio.io_Info.ioi_Command == FILECMD_ADDENTRY) {
      memcpy(&theDevice->lmd_ThisBlock, &theDevice->lmd_OtherBlock,
	     sizeof theDevice->lmd_ThisBlock);
      theDevice->lmd_ThisBlockCursor = theDevice->lmd_OtherBlockCursor;
    }
    if (theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount - theDevice->lmd_DesiredSize <
	sizeof (LinkedMemFileEntry) + LINKED_MEM_SLACK) {
      theDevice->lmd_OtherBlockCursor = theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset;
      action = readOther;
      nextState = LMD_FixOldBackLink;
    } else {
      DBUG(("Trim entry at %d to %d blocks\n", theDevice->lmd_ThisBlockCursor,
	    theDevice->lmd_DesiredSize));
      theDevice->lmd_OtherBlockCursor = theDevice->lmd_ThisBlockCursor +
	theDevice->lmd_DesiredSize;
      theDevice->lmd_OtherBlock.lmfe.lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
      theDevice->lmd_OtherBlock.lmfe.lmb_HeaderBlockCount =
	HOWMANY(sizeof(LinkedMemBlock),
	theFile->fi_FileSystem->fs_VolumeBlockSize);
      theDevice->lmd_OtherBlock.lmfe.lmb_FlinkOffset =
	theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset;
      theDevice->lmd_OtherBlock.lmfe.lmb_BlinkOffset =
	theDevice->lmd_ThisBlockCursor;
      theDevice->lmd_OtherBlock.lmfe.lmb_BlockCount =
	theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount - theDevice->lmd_DesiredSize;
      DBUG(("New successor at %d will have %d blocks\n", theDevice->lmd_OtherBlockCursor,
	    theDevice->lmd_OtherBlock.lmfe.lmb_BlockCount));
      action = writeOther;
      nextState = LMD_CutOffExcess;
    }
    break;
  case LMD_CutOffExcess:
    theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount = theDevice->lmd_DesiredSize;
    theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset = theDevice->lmd_OtherBlockCursor;
    action = writeThis;
    nextState = LMD_GetOldBackLink;
    break;
  case LMD_GetOldBackLink:
    theDevice->lmd_OtherBlockCursor = theDevice->lmd_OtherBlock.lmfe.lmb_FlinkOffset;
    action = readOther;
    nextState = LMD_FixOldBackLink;
    break;
  case LMD_FixOldBackLink:
    if (theDevice->lmd_OtherBlockCursor == theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset) {
      theDevice->lmd_OtherBlock.lmfe.lmb_BlinkOffset = theDevice->lmd_ThisBlockCursor;
    } else {
      theDevice->lmd_OtherBlock.lmfe.lmb_BlinkOffset = theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset;
    }
    action = writeOther;
    nextState = LMD_SuccessfulChomp;
    break;
  case LMD_SuccessfulChomp:
    theFile->fi_BlockCount = theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount -
      theDevice->lmd_FileHeaderBlockSize;
    if (theDevice->lmd_MergeBlockCursor != 0) {
      theDevice->lmd_ThisBlockCursor = theDevice->lmd_MergeBlockCursor;
      DBUG(("Need to do a merger!\n"));
      goto FetchHeader;
    }
    nextState = LMD_Done;
    returnVal = 0; /* done, success */
    break;
  case LMD_TryNewBlock:
  TryNewBlock: ;
    theDevice->lmd_OtherBlockCursor =
      theFile->fi_FileSystem->fs_RootDirectory->fi_AvatarList[0];
    action = readOther;
    nextState = LMD_ExamineNewBlock;
    break;
  case LMD_ExamineNewBlock:
    DBUG(("Examine block at %d\n", theDevice->lmd_OtherBlockCursor));
    if (theDevice->lmd_OtherBlock.lmfe.lmb_Fingerprint != FINGERPRINT_FREEBLOCK ||
	theDevice->lmd_OtherBlock.lmfe.lmb_BlockCount < theDevice->lmd_DesiredSize) {
      DBUG(("Wanted %d, was %d type 0x%x\n", theDevice->lmd_DesiredSize,
	    theDevice->lmd_OtherBlock.lmfe.lmb_BlockCount,
	    theDevice->lmd_OtherBlock.lmfe.lmb_Fingerprint));
      theDevice->lmd_OtherBlockCursor = theDevice->lmd_OtherBlock.lmfe.lmb_FlinkOffset;
      DBUG(("Advance to %d\n", theDevice->lmd_OtherBlockCursor));
      if (theDevice->lmd_OtherBlockCursor ==
	  theFile->fi_FileSystem->fs_RootDirectory->fi_AvatarList[0]) {
	returnVal = MakeFErr(ER_SEVER,ER_C_STND,ER_Fs_NoSpace);
	DBUG(("Wraparound, EOF\n"));
      } else {
	action = readOther;
	nextState = LMD_ExamineNewBlock;
	DBUG(("Do another read\n"));
      }
    } else {
      DBUG(("Acquiring\n"));
      theDevice->lmd_OtherBlock.lmfe.lmb_Fingerprint = FINGERPRINT_FILEBLOCK;
      theDevice->lmd_OtherBlock.lmfe.lmb_HeaderBlockCount =
	theDevice->lmd_FileHeaderBlockSize;
      switch (userReq->fio.io_Info.ioi_Command) {
      case FILECMD_ADDENTRY:
	theDevice->lmd_OtherBlock.lmfe_ByteCount = 0;
	theDevice->lmd_OtherBlock.lmfe_Type = 0x20202020;
	theDevice->lmd_OtherBlock.lmfe_UniqueIdentifier = 0;
	strncpy(theDevice->lmd_OtherBlock.lmfe_FileName,
		(char *) userReq->fio.io_Info.ioi_Send.iob_Buffer,
		sizeof theDevice->lmd_OtherBlock.lmfe_FileName);
	DBUG(("Add entry, filename is %s\n", theDevice->lmd_OtherBlock.lmfe_FileName));
	action = writeOther;
	nextState = LMD_CutTheSlack;
	break;
      case FILECMD_ALLOCBLOCKS:
	theDevice->lmd_OtherBlock.lmfe_ByteCount =
	  theDevice->lmd_ThisBlock.lmfe_ByteCount;
	theDevice->lmd_OtherBlock.lmfe_Type =
	  theDevice->lmd_ThisBlock.lmfe_Type;
	theDevice->lmd_OtherBlock.lmfe_UniqueIdentifier =
	  theDevice->lmd_ThisBlock.lmfe_UniqueIdentifier;
	strncpy(theDevice->lmd_OtherBlock.lmfe_FileName,
		theDevice->lmd_ThisBlock.lmfe_FileName,
		sizeof theDevice->lmd_OtherBlock.lmfe_FileName);
	nextState = LMD_ReadToCopy;
	theDevice->lmd_ContentOffset =
	  theDevice->lmd_OtherBlock.lmfe.lmb_HeaderBlockCount;
	theDevice->lmd_BlocksToCopy =
	  (theDevice->lmd_OtherBlock.lmfe_ByteCount +
	   theDevice->lmd_BlockSize - 1) / theDevice->lmd_BlockSize;
	if (theDevice->lmd_BlocksToCopy > 0) {
	  nextState = LMD_ReadToCopy;
	} else {
	  nextState = LMD_CopyDone;
	}
	action = writeOther;
	theDevice->lmd_BlocksToRead = 0;
      }
    }
    break;
  case LMD_ReadToCopy:
    theDevice->lmd_ContentOffset += theDevice->lmd_BlocksToRead;
    theDevice->lmd_BlocksToRead = theDevice->lmd_BlocksToCopy;
    if (theDevice->lmd_BlocksToRead > theDevice->lmd_CopyBlockSize) {
      theDevice->lmd_BlocksToRead = theDevice->lmd_CopyBlockSize;
    }
    action = readData;
    nextState = LMD_WriteCopiedData;
    break;
  case LMD_WriteCopiedData:
    action = writeData;
    theDevice->lmd_BlocksToCopy -= theDevice->lmd_BlocksToRead;
    if (theDevice->lmd_BlocksToCopy <= 0) {
      nextState = LMD_CopyDone;
    } else {
      nextState = LMD_ReadToCopy;
    }
    break;
  case LMD_CopyDone:
    memcpy(&tempBlock, &theDevice->lmd_ThisBlock, sizeof tempBlock);
    memcpy(&theDevice->lmd_ThisBlock, &theDevice->lmd_OtherBlock, sizeof theDevice->lmd_ThisBlock);
    memcpy(&theDevice->lmd_OtherBlock, &tempBlock, sizeof theDevice->lmd_OtherBlock);
    theDevice->lmd_OtherBlock.lmfe.lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
    theDevice->lmd_OtherBlock.lmfe.lmb_HeaderBlockCount =
	HOWMANY(sizeof(LinkedMemBlock),
	theFile->fi_FileSystem->fs_VolumeBlockSize);
    theDevice->lmd_MergeBlockCursor = theDevice->lmd_ThisBlockCursor;
    theDevice->lmd_ThisBlockCursor = theDevice->lmd_OtherBlockCursor;
    theDevice->lmd_OtherBlockCursor = theDevice->lmd_MergeBlockCursor;
    theFile->fi_AvatarList[0] = theDevice->lmd_ThisBlockCursor;
    theFile->fi_BlockCount = theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount -
      theDevice->lmd_FileHeaderBlockSize;
    DBUG(("File avatar[0] set to %d\n", theFile->fi_AvatarList[0]));
    action = writeOther;
    nextState = LMD_CutTheSlack;
    break;
  case LMD_FetchHeader:
  FetchHeader: ;
    action = readThis;
    nextState = LMD_MarkItFree;
    break;
  case LMD_MarkItFree:
  MarkItFree:
    theDevice->lmd_CurrentEntryIndex = theDevice->lmd_CurrentEntryOffset = 0;
    theDevice->lmd_ThisBlock.lmfe.lmb_Fingerprint = FINGERPRINT_FREEBLOCK;
    theDevice->lmd_ThisBlock.lmfe.lmb_HeaderBlockCount =
	HOWMANY(sizeof(LinkedMemBlock),
	theFile->fi_FileSystem->fs_VolumeBlockSize);
    action = writeThis;
    nextState = LMD_BackUpOne;
    break;
  case LMD_BackUpOne:
    if (theDevice->lmd_ThisBlock.lmfe.lmb_Fingerprint == FINGERPRINT_FREEBLOCK) {
      theDevice->lmd_ThisBlockCursor = theDevice->lmd_ThisBlock.lmfe.lmb_BlinkOffset;
      nextState = LMD_BackUpOne;
    } else {
      theDevice->lmd_ThisBlockCursor = theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset;
      nextState = LMD_ScanAhead;
    }
    action = readThis;
    break;
  case LMD_ScanAhead:
    theDevice->lmd_OtherBlockCursor = theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset;
    action = readOther;
    nextState = LMD_AttemptMerge;
    break;
  case LMD_AttemptMerge:
    if (theDevice->lmd_OtherBlock.lmfe.lmb_Fingerprint == FINGERPRINT_FREEBLOCK) {
      theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount += theDevice->lmd_OtherBlock.lmfe.lmb_BlockCount;
      theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset = theDevice->lmd_OtherBlock.lmfe.lmb_FlinkOffset;
      theDevice->lmd_OtherBlockCursor = theDevice->lmd_OtherBlock.lmfe.lmb_FlinkOffset;
      action = readOther;
      nextState = LMD_AttemptMerge;
    } else {
      theDevice->lmd_OtherBlock.lmfe.lmb_BlinkOffset = theDevice->lmd_ThisBlockCursor;
      action = writeOther;
      nextState = LMD_FixFlink;
    }
    break;
  case LMD_FixFlink:
    action = writeThis;
    nextState = LMD_DoneDeleting;
    break;
  case LMD_DoneDeleting:
    returnVal = 0;
    break;
  case LMD_InitScan:
    theDevice->lmd_ThisBlockIndex = theDevice->lmd_CurrentEntryIndex;
    theDevice->lmd_ThisBlockCursor = theDevice->lmd_CurrentEntryOffset;
    if (theDevice->lmd_ThisBlockCursor <= 0 ||
	theDevice->lmd_ThisBlockIndex <= 0) {
      theDevice->lmd_ThisBlockCursor = theFile->fi_FileSystem->fs_RootDirectory->fi_AvatarList[0];
      theDevice->lmd_ThisBlockIndex = 0;
    }
    theDevice->lmd_HaltCursor = theDevice->lmd_ThisBlockCursor;
    action = readThis;
    nextState = LMD_ExamineEntry;
    break;
  case LMD_ExamineEntry:
    switch (theDevice->lmd_ThisBlock.lmfe.lmb_Fingerprint) {
    case FINGERPRINT_ANCHORBLOCK:
      DBUG(("Found anchor block\n"));
      theDevice->lmd_ThisBlockIndex = 0;
      break;
    case FINGERPRINT_FILEBLOCK:
      if (theDevice->lmd_CurrentEntryOffset != theDevice->lmd_ThisBlockCursor) {
	theDevice->lmd_ThisBlockIndex ++;
	theDevice->lmd_CurrentEntryIndex = theDevice->lmd_ThisBlockIndex;
	theDevice->lmd_CurrentEntryOffset = theDevice->lmd_ThisBlockCursor;
      }
      DBUG(("Now at entry index %d file %s\n",
	    theDevice->lmd_CurrentEntryIndex,
	    theDevice->lmd_ThisBlock.lmfe_FileName));
      DBUG(("  want entry index %d file %s\n",
	    userReq->fio.io_Info.ioi_Offset,
	    theDevice->lmd_CopyBuffer));
      if (userReq->fio.io_Info.ioi_Command == FILECMD_DELETEENTRY &&
	  strcasecmp(theDevice->lmd_ThisBlock.lmfe_FileName,
		      theDevice->lmd_CopyBuffer) == 0) {
	DBUG(("Found proper index/entry!\n"));
	goto MarkItFree;
      } else if ((userReq->fio.io_Info.ioi_Command == FILECMD_READDIR &&
	   theDevice->lmd_ThisBlockIndex == userReq->fio.io_Info.ioi_Offset) ||
	  (userReq->fio.io_Info.ioi_Command == FILECMD_READENTRY &&
	   strcasecmp(theDevice->lmd_ThisBlock.lmfe_FileName,
		       theDevice->lmd_CopyBuffer) == 0)) {
	returnVal = 0;
	DBUG(("Found proper index/entry!\n"));
	strncpy(de.de_FileName, theDevice->lmd_ThisBlock.lmfe_FileName,
		FILESYSTEM_MAX_NAME_LEN);
	de.de_BlockSize = theFile->fi_BlockSize;
	DBUG(("Entry block size is %d\n", theFile->fi_BlockSize));
	de.de_Burst = -1;
	de.de_Gap = -1;
	de.de_AvatarCount = 1;
	de.de_Type = theDevice->lmd_ThisBlock.lmfe_Type;
	de.de_UniqueIdentifier = theDevice->lmd_ThisBlock.lmfe_UniqueIdentifier;
	de.de_ByteCount = theDevice->lmd_ThisBlock.lmfe_ByteCount;
	de.de_BlockCount = theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount -
	  theDevice->lmd_ThisBlock.lmfe.lmb_HeaderBlockCount;
	de.de_Location = theDevice->lmd_ThisBlockCursor;
	DBUG(("Total blocks %d, header blocks %d, content blocks %d\n",
	      theDevice->lmd_ThisBlock.lmfe.lmb_BlockCount,
	      theDevice->lmd_ThisBlock.lmfe.lmb_HeaderBlockCount,
	      de.de_BlockCount));
	de.de_Flags = 0;
	deSize = userReq->fio.io_Info.ioi_Recv.iob_Len;
	if (deSize > sizeof (DirectoryEntry)) {
	  deSize = sizeof (DirectoryEntry);
	}
	memcpy(userReq->fio.io_Info.ioi_Recv.iob_Buffer, &de, deSize);
	userReq->fio.io_Actual = deSize;
      } else {
	DBUG(("At index %d, want index %d, keep going\n", theDevice->lmd_ThisBlockIndex,  userReq->fio.io_Info.ioi_Offset));
      }
      break;
    default:
      break;
    }
    if (returnVal != 0) {
      theDevice->lmd_ThisBlockCursor = theDevice->lmd_ThisBlock.lmfe.lmb_FlinkOffset;
      DBUG(("Advance cursor to %d\n", theDevice->lmd_ThisBlockCursor));
      if (theDevice->lmd_ThisBlockCursor == theDevice->lmd_HaltCursor) {
	returnVal = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
	nextState = LMD_Done;
      } else {
	action = readThis;
	nextState = LMD_ExamineEntry;
      }
    }
    break;
  case LMD_ReadToSetEOF:
    theFile->fi_ByteCount = userReq->fio.io_Info.ioi_Offset;
    action = readThis;
    nextState = LMD_WriteWithNewEOF;
    break;
  case LMD_WriteWithNewEOF:
    theDevice->lmd_ThisBlock.lmfe_ByteCount = theFile->fi_ByteCount;
    DBUG(("Setting EOF to %d\n", theDevice->lmd_ThisBlock.lmfe_ByteCount));
    action = writeThis;
    nextState = LMD_Done;
    break;
  case LMD_ReadToSetType:
    action = readThis;
    nextState = LMD_WriteWithNewType;
    break;
  case LMD_WriteWithNewType:
    theDevice->lmd_ThisBlock.lmfe_Type = userReq->fio.io_Info.ioi_Offset;
    action = writeThis;
    nextState = LMD_Done;
    break;
  case LMD_ExtendEOF:
    newEOF = userReq->fio.io_Info.ioi_Offset * theFile->fi_BlockSize +
      userReq->fio.io_Info.ioi_Send.iob_Len;
    if (newEOF > theFile->fi_ByteCount) {
      DBUG(("Need to extend EOF after write\n"));
      theFile->fi_ByteCount = newEOF;
      theDevice->lmd_ThisBlockCursor = theFile->fi_AvatarList[0];
      action = readThis;
      nextState = LMD_WriteWithNewEOF;
    } else {
      nextState = LMD_Done;
    }
  }
  DBUG(("LMD FSM, action %d, nextstate %d, return 0x%x\n", action, nextState, returnVal));
  switch (action) {
  case noAction:
    break;
  case readThis:
    raw->io_Info.ioi_Send.iob_Buffer = NULL;
    raw->io_Info.ioi_Send.iob_Len = 0;
    raw->io_Info.ioi_Recv.iob_Buffer = &theDevice->lmd_ThisBlock;
    raw->io_Info.ioi_Recv.iob_Len = sizeof theDevice->lmd_ThisBlock;
    raw->io_Info.ioi_Command = CMD_READ;
    raw->io_Info.ioi_Offset = theDevice->lmd_ThisBlockCursor;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case readOther:
    raw->io_Info.ioi_Send.iob_Buffer = NULL;
    raw->io_Info.ioi_Send.iob_Len = 0;
    raw->io_Info.ioi_Recv.iob_Buffer = &theDevice->lmd_OtherBlock;
    raw->io_Info.ioi_Recv.iob_Len = sizeof theDevice->lmd_OtherBlock;
    raw->io_Info.ioi_Command = CMD_READ;
    raw->io_Info.ioi_Offset = theDevice->lmd_OtherBlockCursor;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case writeThis:
    raw->io_Info.ioi_Recv.iob_Buffer = NULL;
    raw->io_Info.ioi_Recv.iob_Len = 0;
    raw->io_Info.ioi_Send.iob_Buffer = &theDevice->lmd_ThisBlock;
    raw->io_Info.ioi_Send.iob_Len = sizeof theDevice->lmd_ThisBlock;
    raw->io_Info.ioi_Command = CMD_WRITE;
    raw->io_Info.ioi_Offset = theDevice->lmd_ThisBlockCursor;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case writeOther:
    raw->io_Info.ioi_Recv.iob_Buffer = NULL;
    raw->io_Info.ioi_Recv.iob_Len = 0;
    raw->io_Info.ioi_Send.iob_Buffer = &theDevice->lmd_OtherBlock;
    raw->io_Info.ioi_Send.iob_Len = sizeof theDevice->lmd_OtherBlock;
    raw->io_Info.ioi_Command = CMD_WRITE;
    raw->io_Info.ioi_Offset = theDevice->lmd_OtherBlockCursor;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case readData:
    raw->io_Info.ioi_Send.iob_Buffer = NULL;
    raw->io_Info.ioi_Send.iob_Len = 0;
    raw->io_Info.ioi_Recv.iob_Buffer = theDevice->lmd_CopyBuffer;
    raw->io_Info.ioi_Recv.iob_Len = theDevice->lmd_BlocksToRead *
      theDevice->lmd_BlockSize;
    raw->io_Info.ioi_Command = CMD_READ;
    raw->io_Info.ioi_Offset = theDevice->lmd_ThisBlockCursor + theDevice->lmd_ContentOffset;
;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  case writeData:
    raw->io_Info.ioi_Send.iob_Buffer = theDevice->lmd_CopyBuffer;
    raw->io_Info.ioi_Send.iob_Len = theDevice->lmd_BlocksToRead *
      theDevice->lmd_BlockSize;
    raw->io_Info.ioi_Recv.iob_Buffer = NULL;
    raw->io_Info.ioi_Recv.iob_Len = 0;
    raw->io_Info.ioi_Command = CMD_WRITE;
    raw->io_Info.ioi_Offset = theDevice->lmd_OtherBlockCursor + theDevice->lmd_ContentOffset;
;
    raw->io_Info.ioi_Flags = 0;
    raw->io_Info.ioi_CmdOptions = 0;
    break;
  }
  theDevice->lmd_FSM = nextState;
  return returnVal;
}


/*
 *	provide filesystem statistics and info
 */
void
LinkedMemDoStat(LinkedMemFileEntry *fep, FileSystemStat *fsp, uint32 curblk)
{
	uint32 realsz, fhdr;

	switch(fep->lmfe.lmb_Fingerprint) {
	case FINGERPRINT_FILEBLOCK:
	case FINGERPRINT_ANCHORBLOCK:
		fsp->fst_Used += fep->lmfe.lmb_BlockCount;
		break;
	case FINGERPRINT_FREEBLOCK:
		
		fhdr = HOWMANY(sizeof(LinkedMemFileEntry), fsp->fst_BlockSize);
		realsz = (fep->lmfe.lmb_BlockCount <= fhdr)? 0:
			  fep->lmfe.lmb_BlockCount - fhdr;
		fsp->fst_Free += realsz;
		if (realsz > fsp->fst_MaxFileSize)
			fsp->fst_MaxFileSize = realsz;
		break;
	default:	/* FS corruption, we should panic */
		break;
	}
}
