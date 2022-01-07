/*****

$Id: FileDriver.c,v 1.36 1995/02/24 01:20:00 dplatt Exp $

$Log: FileDriver.c,v $
 * Revision 1.36  1995/02/24  01:20:00  dplatt
 * Filesystem should be marked busy, not not-busy, when the next page
 * of the Catapult file index is being loaded.  Otherwise there's
 * a timing window during which a subsequent user IOReq could be
 * handed off to the underlying device using an IOReq still in use for
 * the Catapult index page.
 *
 * Revision 1.35  1994/09/22  19:40:12  dplatt
 * More performance tracking in Catapult logic.
 *
 * Revision 1.34  1994/07/26  21:05:35  dplatt
 * Remove (make conditional) most support for interleaved files... this
 * is going away, as it's never been used and we need the code space.
 *
 * Implement first cut of the Catapult driver changes.
 *
 * Revision 1.33  1994/06/01  17:48:12  dplatt
 * Did RemNode() of wrong node when killing requests due to offline
 * filesystem.  Results in system lockup if CD-ROM removed - DIPIR
 * probably would not work correctly thereafter, and power-cycling
 * system would be required to un-hang and reboot.  Oops.  Thanks for
 * catching this, Dale!
 *
 * Revision 1.32  1994/05/13  00:42:22  cramer
 * fix compilation warnings.
 *
 * Revision 1.31  1994/04/07  03:19:46  dplatt
 * Avoid race condition which was killing Madden and had also caused
 * problems for Spence.  StartOptimizedDiskIO must now check busy,
 * instead of just setting it.
 *
 * Revision 1.30  1994/04/05  18:55:33  shawn
 * Added directory semaphore.
 *
 * Revision 1.29  1994/03/24  01:48:43  dplatt
 * Add support for kernel events, hot-mount/hot-dismount.  Minor
 * security enhancements and rearrangements of various things.
 *
 * Revision 1.28  1994/02/09  21:30:40  shawn
 * do not write too much into user address space.
 *
 * Revision 1.27  1994/02/07  18:19:21  shawn
 * Changes to suppport FSSTAT.
 *
 * Revision 1.26  1994/02/04  23:26:14  dplatt
 * Switch over to standardized error codes for device errors and offline,
 * and add device-offline flag handling.
 *
 * Revision 1.25  1994/01/21  00:29:21  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.25  1994/01/19  20:31:52  dplatt
 * Change printf to DBUG, to eliminate device-busy message which
 * might crop up now that the scheduler algorithm has been tweaked.
 *
 * Revision 1.24  1993/12/16  00:42:55  dplatt
 * Add support for OpenEntry command, fix bug in CMD_STATUS handling
 *
 * Revision 1.23  1993/11/07  00:51:47  dplatt
 * Initiating a high-priority read, then aborting it, will cause the
 * next read to hang if it's submitted at a lower priority under
 * certain conditions.
 *
 * Revision 1.22  1993/07/09  18:03:24  dplatt
 * If a file's blocksize is larger than the blocksize of the filesystem
 * (e.g. a directory with a blocksize of 2048, on a ROM filesystem of
 * blocksize 1 or 4), then reads to any offset other than 0 go to the
 * wrong place in the file.  Need to multiply user-specified offset by
 * the (fileblocksize/filesystemblocksize) correction factor when doing
 * avatar lookup.
 *
 * Revision 1.21  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.20  1993/05/31  03:42:18  dplatt
 * Dragon changes
 *
 * Revision 1.19  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.18  1993/05/08  01:08:14  dplatt
 * Add flat-file-system/NVRAM support, and recover from RCS bobble
 *
 * Revision 1.17  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.16  1993/02/21  03:33:16  dplatt
 * Two-level debugging.
 *
 * Revision 1.15  1993/02/12  21:05:55  dplatt
 * More error-text-node stuff, and driver/folio renaming.
 *
 * Revision 1.14  1993/02/12  01:50:31  dplatt
 * Set name of root to "" rather than NULL, to work with Dale's new kernel.
 *
 * Revision 1.13  1993/02/11  19:39:37  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.12  1993/02/10  00:27:26  dplatt
 * Change to new IOReq queueing mechanism and save big money!
 *
 * Revision 1.11  1993/02/09  01:47:20  dplatt
 * Reorganize and update for developer release
 *
 * Revision 1.10  1993/01/05  20:57:47  dplatt
 * CES changes redux
 *
 * Revision 1.9  1993/01/04  05:48:38  dplatt
 * Fix crash in file daemon
 *
 * Revision 1.8  1992/12/22  07:58:10  dplatt
 * Magneto 3 changes and CD-ROM support
 *
 * Revision 1.7  1992/12/08  05:59:52  dplatt
 * Magenta changes
 *
 * Revision 1.6  1992/10/27  01:35:41  dplatt
 * Oops, tested on UGO not on Blue
 *
 * Revision 1.5  1992/10/24  00:40:56  dplatt
 * Bluebird changes and bug fixes
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
 * Revision 1.1  1992/09/11  00:42:26  dplatt
 * Initial revision
 *
 * Revision 1.2  1992/09/10  18:42:12  dplatt
 * Initial update
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  FileDriver.c - implements the driver which handles OpenFile "devices"
  and OptimizedDisk "devices".

  The driver receives IORequests issued against OpenFile "devices".  It
  queues these requests against the OptimizedDisk device associated with
  the actual (raw, physical) device on which the filesystem resides.
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
#include "msgport.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "super.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "directory.h"
#include "discdata.h"
#include "operror.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"

#undef DEBUG
#undef DEBUG2
#undef INTERLEAVED_FILES

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifdef PRODUCTION
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

#ifdef PRODUCTION
#define MySysErr(x) /* x */
#else
extern void MySysErr(Err err);
#endif

Driver *fileDriver;

extern char *fsCacheBase;
extern int32 fsCacheBusy;

int32 FileDriverInit(struct Driver *me);
int32 FileDriverDispatch(IOReq *theRequest);

void FileDriverAbortIo(IOReq *theRequest);
int32  FileDriverDoit(IOReq *theRequest);
int32  FileDriverStatus(IOReq *theRequest);
int32  FileDriverGetPath(IOReq *theRequest);

void ScheduleOptimizedDiskIo(OptimizedDisk *theDevice);
void StartOptimizedDiskIo (OptimizedDisk *theDevice);
IOReq *OptimizedDiskEndAction(IOReq *theRequest);
IOReq *CatapultEndAction(IOReq *theRequest);
void AbortOptimizedDiskIo(IOReq *userRequest);

static uchar HighestPriority(List *);

extern int32 internalGetPath(File *theFile, char *theBuffer, int32 bufLen);
extern FileFolioTaskData *SetupFileFolioTaskData(void);

const TagArg fileDriverTags[] = {
  { TAG_ITEM_NAME, 	        (void *) "File" },
  { CREATEDRIVER_TAG_ABORTIO,   (void *) (int32) FileDriverAbortIo },
  { CREATEDRIVER_TAG_MAXCMDS,   (void *) 12 },
  { CREATEDRIVER_TAG_DISPATCH,  (void *) (int32) FileDriverDispatch },
  { CREATEDRIVER_TAG_INIT,      (void *) (int32) FileDriverInit },
  { TAG_END,                     NULL }
};

int32 FileDriverInit(struct Driver *me)
{
  int32 i;
  File *root;
  DBUG(("File driver init, driver at %x!\n", me));
  i = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
			   (void *) NULL, sizeof (File));
  if (i < 0) {
    DBUG(("Couldn't create root file (%d)\n", i));
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  DBUG(("Root-of-the-world file created as item %d\n", i));
  root = (File *) LookupItem(i);
  fileFolio->ff_Root = root;
  root->fi_FileName[0] = '\0';
  root->fi_FileSystem = (FileSystem *) NULL;
  root->fi_ParentDirectory = root;
  root->fi_UniqueIdentifier = 0;
  root->fi_Type = FILE_TYPE_DIRECTORY;
  root->fi_Flags = FILE_IS_DIRECTORY | FILE_IS_READONLY |
                   FILE_IS_FOR_FILESYSTEM | FILE_SUPPORTS_DIRSCAN;
  root->fi_UseCount = 1;
  root->fi_BlockSize = FILESYSTEM_DEFAULT_BLOCKSIZE;
  root->fi_ByteCount = 0;
  root->fi_BlockCount = 0;
  root->fi_Burst = -1;
  root->fi_Gap = -1;
  root->fi_LastAvatarIndex = -1;
  root->fi.n_Name = root->fi_FileName;
#ifdef	FS_DIRSEMA
  InitDirSema(root, 0);
#endif	/* FS_DIRSEMA */
  DBUG(("Root-of-the-world file initialized\n"));
  AddTail(&fileFolio->ff_Files, (Node *) root);
  DBUG(("Root-of-the-world file queued\n"));
  DBUG(("File driver initialization complete\n"));
  return me->drv.n_Item;
}

void FileDriverAbortIo(IOReq *theRequest)
{
  HighLevelDisk *theDevice;
  OpenFile *theOpenFile;
  theOpenFile = (OpenFile *) theRequest->io_Dev;
  theDevice = theOpenFile->ofi_File->fi_FileSystem->fs_Device;
  (*theDevice->hdi_AbortIO)(theRequest);
  return;
}

int32 FileDriverDispatch (IOReq *theRequest)
{
  FileFolioTaskData *ffPrivate;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  switch (theRequest->io_Info.ioi_Command) {
  case CMD_READ:
  case CMD_WRITE:
  case FILECMD_READDIR:
  case FILECMD_READENTRY:
  case FILECMD_ADDENTRY:
  case FILECMD_DELETEENTRY:
  case FILECMD_ALLOCBLOCKS:
  case FILECMD_SETEOF:
  case FILECMD_SETTYPE:
  case FILECMD_OPENENTRY:
  case FILECMD_FSSTAT:
    return FileDriverDoit(theRequest);
  case CMD_STATUS:
    return FileDriverStatus(theRequest);
  case FILECMD_GETPATH:
    return FileDriverGetPath(theRequest);
  default:
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
}

int32 RootFileIO(IOReq *theRequest)
{
  OpenFile *theOpenFile;
  File *rootDirectory;
  FileSystem *theFS;
  DirectoryEntry de;
  int32 deSize;
  int32 num;
  theOpenFile = (OpenFile *) theRequest->io_Dev;
  switch (theRequest->io_Info.ioi_Command) {
  case FILECMD_READDIR:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    num = (int32) theRequest->io_Info.ioi_Offset;
    if (num <= 0) {
      return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
    }
    theFS = (FileSystem *) FIRSTNODE(&fileFolio->ff_Filesystems);
    while (--num > 0) {
      if (!ISNODE(&fileFolio->ff_Filesystems,theFS)) {
	return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
      }
      theFS = (FileSystem *) NEXTNODE(theFS);
    }
    if (!ISNODE(&fileFolio->ff_Filesystems,theFS)) {
      return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
    }
    rootDirectory = theFS->fs_RootDirectory;
    strncpy(de.de_FileName, theFS->fs.n_Name,
	    FILESYSTEM_MAX_NAME_LEN);
    de.de_UniqueIdentifier = theFS->fs_RootDirectory->fi_UniqueIdentifier;
    de.de_Type = rootDirectory->fi_Type;
    de.de_BlockSize = rootDirectory->fi_BlockSize;
    de.de_Burst = rootDirectory->fi_Burst;
    de.de_Gap = rootDirectory->fi_Gap;
    de.de_AvatarCount = rootDirectory->fi_LastAvatarIndex + 1;
    de.de_ByteCount = rootDirectory->fi_ByteCount;
    de.de_BlockCount = rootDirectory->fi_BlockCount;
    de.de_Flags = rootDirectory->fi_Flags;
    de.de_Location = rootDirectory->fi_AvatarList[0];
    deSize = theRequest->io_Info.ioi_Recv.iob_Len;
    if (deSize > sizeof (DirectoryEntry)) {
      deSize = sizeof (DirectoryEntry);
    }
    memcpy(theRequest->io_Info.ioi_Recv.iob_Buffer, &de, deSize);
    theRequest->io_Actual = deSize;
    return 0;
  default:
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }  
}

int32 FileDriverDoit(IOReq *theRequest)
{
  HighLevelDisk *theDevice;
  OpenFile *theOpenFile;
  FileSystem *theFileSystem;
  int32 err;
  theOpenFile = (OpenFile *) theRequest->io_Dev;
  theFileSystem = theOpenFile->ofi_File->fi_FileSystem;
  if (!theFileSystem) {
    err = RootFileIO(theRequest);
    theRequest->io_Error = err;
    SuperCompleteIO(theRequest);
    return err;
  }
  if (theFileSystem->fs_Flags & FILESYSTEM_IS_OFFLINE) {
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
    SuperCompleteIO(theRequest);
    return MakeFErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
  }    
  theDevice = theFileSystem->fs_Device;
#ifdef DEBUG
  Superkprintf("Action request for ioreq %lx openfile %lx device %lx\n",
	       theRequest, theOpenFile, theDevice);
#endif
  if ((err = (*theDevice->hdi_QueueRequest)(theDevice, theRequest)) != 0) {
    DBUG(("Request rejected by device, code 0x%x\n", err));
    theRequest->io_Error = err;
    SuperCompleteIO(theRequest);
    return err;
  }
  if (theDevice->hdi_DeviceBusy) {
#ifdef DEBUG
    Superkprintf("Device busy, holding off\n");
#endif
  } else {
    if (fileFolio->ff_Daemon.ffd_Task != (Task *) NULL) {
/*
  If this request is the only one on the to-do list, signal the daemon
  to schedule I/O for this device after an appropriate holdoff interval.

  TBD: device-specific decisions about whether to hold off.  Very fast
  devices (e.g. ramdisks) or those with low differential latency should
  not be held off.
*/

#ifdef OPTWAKEUP
      if ((IOReq *) FIRSTNODE(&theDevice->hdi_RequestsToDo) == theRequest &&
	  (IOReq *) LASTNODE(&theDevice->hdi_RequestsToDo) == theRequest) {
	DBUG2(("Requesting daemon schedule\n"));
	SuperinternalSignal(fileFolio->ff_Daemon.ffd_Task,
			    fileFolio->ff_Daemon.ffd_QueuedSignal);
      }
#else
	DBUG2(("Requesting daemon schedule\n"));
	SuperinternalSignal(fileFolio->ff_Daemon.ffd_Task,
			    fileFolio->ff_Daemon.ffd_QueuedSignal);
#endif
    } else {
#ifdef DEBUG2
      Superkprintf("Scheduling I/O\n");
#endif
      (*theDevice->hdi_ScheduleIO)(theDevice);
#ifdef DEBUG2
      Superkprintf("Starting I/O\n");
#endif
      (*theDevice->hdi_StartIO)(theDevice);
    }
  }
  return 0;
}

int32 FileDriverStatus(IOReq *theRequest)
{
  FileStatus status;
  OpenFile *theOpenFile;
  File *theFile;
#ifdef DEBUG
  Superkprintf("File-driver status call via request at %d\n", theRequest);
#endif
  theOpenFile = (OpenFile *) theRequest->io_Dev;
  theFile = theOpenFile->ofi_File;
  memset(&status, 0, sizeof status);
  status.fs.ds_DriverIdentity = DI_FILE;
  status.fs.ds_FamilyCode = DS_DEVTYPE_FILE;
  status.fs.ds_MaximumStatusSize = sizeof status;
  status.fs.ds_DeviceBlockSize = theFile->fi_BlockSize;
  status.fs.ds_DeviceBlockCount = theFile->fi_BlockCount;
  status.fs.ds_DeviceFlagWord = theFile->fi_Flags;
  if (theFile->fi_FileSystem->fs_Flags & FILESYSTEM_IS_OFFLINE) {
    status.fs.ds_DeviceUsageFlags = DS_USAGE_OFFLINE;
  }
  if (theFile->fi_Flags & FILE_IS_READONLY) {
    status.fs.ds_DeviceUsageFlags |= DS_USAGE_READONLY;
  }
  status.fs_ByteCount = theFile->fi_ByteCount;
  DBUG(("Status on %s: %d bytes, %d blocks\n", theFile->fi_FileName,
	       status.fs_ByteCount, status.fs.ds_DeviceBlockCount));
  memcpy(theRequest->io_Info.ioi_Recv.iob_Buffer,
	 &status,
	 (int32) ((theRequest->io_Info.ioi_Recv.iob_Len < sizeof status) ?
		theRequest->io_Info.ioi_Recv.iob_Len :  sizeof status));
  SuperCompleteIO(theRequest);
#ifdef DEBUG
  Superkprintf("Status call completed\n");
#endif
  return 0;
}

int32 FileDriverGetPath(IOReq *theRequest)
{
  OpenFile *theOpenFile;
  File *theFile;
  int32 err;
  theOpenFile = (OpenFile *) theRequest->io_Dev;
  theFile = theOpenFile->ofi_File;
  err = internalGetPath(theFile,
			(char *) theRequest->io_Info.ioi_Recv.iob_Buffer,
			(int32) theRequest->io_Info.ioi_Recv.iob_Len);
  SuperCompleteIO(theRequest);
  return err;
}

int32 QueueOptimizedDiskRequest(OptimizedDisk *theDevice, IOReq *theRequest)
{
  uint32 interrupts;
  File *theFile;
  switch (theRequest->io_Info.ioi_Command) {
  case CMD_READ:
    break;
  case FILECMD_OPENENTRY:
    SuperCompleteIO(theRequest);
    return 0;
  case FILECMD_FSSTAT:
    if (theRequest->io_Info.ioi_Recv.iob_Buffer == NULL) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    if (theRequest->io_Info.ioi_Recv.iob_Len < sizeof(FileSystemStat)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
    }
    /*
     *	no fs info for this filesystem
     */
    memset(theRequest->io_Info.ioi_Recv.iob_Buffer,
	   0, sizeof(FileSystemStat));
    SuperCompleteIO(theRequest);
    return 0;
  default:
    DBUG(("I/O command %d rejected\n", theRequest->io_Info.ioi_Command));
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  theFile = ((OpenFile *) theRequest->io_Dev)->ofi_File;
  if (theRequest->io_Info.ioi_Recv.iob_Len % theFile->fi_BlockSize != 0) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
  }
  interrupts = Disable();
  AddTail(&theDevice->odi.hdi_RequestsToDo, (Node *) theRequest);
  if (theRequest->io.n_Priority > theDevice->odi.hdi_RequestPriority) {
    theDevice->odi.hdi_RequestPriority = theRequest->io.n_Priority;
  }
  if (theDevice->odi.hdi_DeviceBusy) {
    if (theDevice->odi.hdi_RequestPriority >
	theDevice->odi.hdi_RunningPriority) {
      theDevice->odi.hdi.dev.n_Flags |= DEVICE_BOINK; /* force defer/resched */
    }
  }
  theRequest->io_Flags &= ~IO_QUICK;
  Enable(interrupts);
#ifdef DEBUG
  Superkprintf("Request %x queued on to-do list\n", theRequest);
#endif
  return 0;
}

/*
  The ScheduleOptimizedIo routine is not reentrant for any one device (although
  it is reentrant across multiple devices).  If interrupted, the
  interrupt routine must not call ScheduleIo either directly or
  indirectly.  If it looks as if the "no SendIO calls at interrupt
  time" rule is ever to be relaxed, all calls to this routine should
  be guarded with disable-interrupt/enable-interrupt pairs.
*/

void ScheduleOptimizedDiskIo(OptimizedDisk *theDevice)
{
  FileIOReq **requests;
  FileIOReq *theRequest, *nextRequest;
  IOReq *rawRequest;
  File *theFile;
  int32 numRequests, topPriority;
  uint32 requestLimit;
  int32 delta, leastDelta = 0, avatar, whereNow;
  int32 streaming;
  int32 nextAbove, nextBelow, phase, passes;
  int32 reqIndex, reqLimit;
  uint32 lowBlock, highBlock;
#ifdef INTERLEAVED_FILES
  uint32 burstModulus, blocksRequested;
#endif
  uint32 absoluteBlock = 0, absoluteOffset;
  int32 avatarIndex, closestAvatar = 0;
  int32 flawLevel, leastFlawLevel, i, j;
  int32 devBlocksPerFilesystemBlock;
  int32 fileSystemBlocksPerFileBlock;
  CatapultPage *catapultPage;
  File *catapultFile;
  List *scheduleList;
  SchedulerSweepDirection sweep;
  DBUG(("Scheduling disk I/O for device 0x%lX\n", theDevice));
  if (theDevice->odi.hdi_DeviceBusy) {
    DBUG(("Can't schedule I/Os, device is busy\n"));
    return;
  }
  rawRequest = theDevice->odi_RawDeviceRequest;
  switch (theDevice->odi_CatapultPhase) {
  case CATAPULT_NONE:
  case CATAPULT_MUST_SHUT_DOWN:
    DBUG(("No catapult, or must shut down\n"));
    break;
  case CATAPULT_MUST_VERIFY:
    DBUG(("Got end-action on catapult file index read\n"));
    fsCacheBusy --; /* this is never changed at interrupt time */
    catapultPage = (CatapultPage *) rawRequest->io_Info.ioi_Recv.iob_Buffer;
    if (rawRequest->io_Error < 0 ||
	rawRequest->io_Actual < rawRequest->io_Info.ioi_Recv.iob_Len ||
	catapultPage->cp_MBZ != 0 ||
	catapultPage->cp_Fingerprint != FILE_TYPE_CATAPULT) {
      DBUG(("Catapult page not good\n"));
      DBUG(("  raw medium offset %d\n", rawRequest->io_Info.ioi_Offset));
      DBUG(("  io_Error = 0x%X ", rawRequest->io_Error));
      MySysErr(rawRequest->io_Error);
      DBUG(("  io_Actual %d, wanted %d\n",
	     rawRequest->io_Actual, rawRequest->io_Info.ioi_Recv.iob_Len));
      DBUG(("  mbz field %d\n", catapultPage->cp_MBZ));
      DBUG(("  fingerprint 0x%X, wanted 0x%X\n",
	     catapultPage->cp_Fingerprint, FILE_TYPE_CATAPULT));
      theDevice->odi_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
    } else {
      DBUG(("Catapult page validated\n"));
      theDevice->odi_CatapultPhase = CATAPULT_AVAILABLE;
      theDevice->odi_CatapultNextIndex = 0;
    }
    break;
  case CATAPULT_READING:
    DBUG(("Catapult read still in progress\n"));
    break;
  default:
    DBUG(("Catapult phase %d\n", theDevice->odi_CatapultPhase));
    catapultFile = theDevice->odi_CatapultFile->ofi_File;
    if (!fsCacheBase /* cache killed */ ||
	theDevice->odi_CatapultPage->ioce_CacheFormat != CACHE_CATAPULT_INDEX ||
	theDevice->odi_CatapultPage->ioce_FilesystemUniqueIdentifier !=
	catapultFile->fi_FileSystem->fs_VolumeUniqueIdentifier ||
	theDevice->odi_CatapultPage->ioce_FileUniqueIdentifier !=
	catapultFile->fi_UniqueIdentifier) {
      DBUG(("Catapult page preempted\n"));
      theDevice->odi_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
    }
    break;
  }
  if (theDevice->odi_CatapultPhase == CATAPULT_MUST_SHUT_DOWN) {
    DBUG0(("Shutting down catapult\n"));
    theDevice->odi_CatapultPage = NULL;
    (void) SuperCloseItem(theDevice->odi_CatapultFile->ofi.dev.n_Item);
    (void) SuperDeleteItem(theDevice->odi_CatapultFile->ofi.dev.n_Item);
    theDevice->odi_CatapultFile = NULL;
    theDevice->odi_CatapultPhase = CATAPULT_NONE;
    DBUG0(("TotCatapultStreamedHits is %d\n", theDevice->odi_TotCatapultStreamedHits));
    DBUG0(("TotCatapultNonstreamedHits is %d\n", theDevice->odi_TotCatapultNonstreamedHits));
    DBUG0(("TotCatapultSeeksAvoided is %d\n", theDevice->odi_TotCatapultSeeksAvoided));
    DBUG0(("TotCatapultTimesEntered is %d\n", theDevice->odi_TotCatapultTimesEntered));
    DBUG0(("TotCatapultDeclined is %d\n", theDevice->odi_TotCatapultDeclined));
    DBUG0(("TotCatapultMisses is %d\n", theDevice->odi_TotCatapultMisses));
    DBUG0(("TotCatapultNonstreamedMisses is %d\n", theDevice->odi_TotCatapultNonstreamedMisses));
  }
  topPriority = 0;
  theDevice->odi.hdi_RequestPriority =
    HighestPriority(&theDevice->odi.hdi_RequestsToDo);
  theDevice->odi_DeferredPriority =
    HighestPriority(&theDevice->odi.hdi_RequestsDeferred);
#ifdef DEBUG2
  Superkprintf("Request priority %d, deferred priority %d\n",
	       theDevice->odi.hdi_RequestPriority,
	       theDevice->odi_DeferredPriority);
#endif
  if (ISEMPTYLIST(&theDevice->odi.hdi_RequestsDeferred) ||
      theDevice->odi.hdi_RequestPriority > theDevice->odi_DeferredPriority) {
    scheduleList = &theDevice->odi.hdi_RequestsToDo;
    topPriority = theDevice->odi.hdi_RequestPriority;
#ifdef DEBUG2
    Superkprintf("Taking to-do list\n");
#endif
  } else {
    scheduleList = &theDevice->odi.hdi_RequestsDeferred;
    topPriority = theDevice->odi_DeferredPriority;
#ifdef DEBUG2
    Superkprintf("Taking deferred-priority list\n");
#endif
  }
  if (ISEMPTYLIST(scheduleList)) {
#ifdef NOTDEF
    Superkprintf("Schedule list at %x is empty... AAGH!\n", scheduleList);
    Superkprintf("First node %x, last node %x\n", FIRSTNODE(scheduleList),
		 LASTNODE(scheduleList));
#endif
    DBUG(("Nothing to schedule\n"));
    return;
  }
  theRequest = (FileIOReq *) FIRSTNODE(scheduleList);
  numRequests = 0;
  requests = theDevice->odi_RequestSort;
  requestLimit = theDevice->odi_RequestSortLimit;
  while (numRequests < requestLimit &&
	 ISNODE(scheduleList, theRequest)) {
    nextRequest = (FileIOReq *) NEXTNODE(theRequest);
#ifdef DEBUG2
    Superkprintf("Examining request %x\n", theRequest);
#endif
    if (theRequest->fio.io.n_Priority == topPriority) {
      RemNode((Node *) theRequest);
      requests[numRequests++] = theRequest;
#ifdef DEBUG2
      Superkprintf("Took request %x\n", theRequest);
#endif
      if (theDevice->odi_CatapultPhase == CATAPULT_AVAILABLE) {
	break; /* take only one at a time while catapulting */
      }
    }
    theRequest = nextRequest;
  }
#ifdef DEBUG2
  Superkprintf("%d requests picked off\n", numRequests);
#endif
  lowBlock = 0x7FFFFFFF /* BIGGEST_POSITIVE_INTEGER */;
  highBlock = 0;
  whereNow = theDevice->odi_NextBlockAvailable;
  DBUG(("Scheduler: next block under arm is %d\n", whereNow));
  for (i = 0; i < numRequests; i++) {
#ifdef DEBUG
    Superkprintf("Checking request %d\n", i);
#endif
    theRequest = requests[i];
    theFile = ((OpenFile *) theRequest->fio.io_Dev)->ofi_File;
    theRequest->fio_BlockCount = theRequest->fio.io_Info.ioi_Recv.iob_Len /
      theFile->fi_BlockSize;
    leastFlawLevel = DRIVER_FLAW_MASK + 1;
    avatarIndex = (int32) theFile->fi_LastAvatarIndex;
#ifdef INTERLEAVED_FILES
/*
  Noninterleaved-file reads are pretty easy to handle.  Interleaved ones
  are a bit trickier.  In either case, leave blockBurst holding the number
  of filesystem blocks to be transferred during the first read.

  Thought for the future... perhaps all files with nonzero burst sizes
  should be read in chunks which don't exceed their burst size, even if the
  gap is zero.  This would cause big reads of large files to be
  interruptable by higher-priority reads on other files.
*/
#ifdef ALWAYSBURST
    burstModulus = theRequest->fio.io_Info.ioi_Offset % theFile->fi_Burst;
    absoluteOffset = (theRequest->fio.io_Info.ioi_Offset /
		      theFile->fi_Burst) *
			(theFile->fi_Burst + theFile->fi_Gap) + burstModulus;
    theRequest->fio_BlockBurst = theFile->fi_Burst - burstModulus;
    blocksRequested = theRequest->fio.io_Info.ioi_Recv.iob_Len /
      theFile->fi_BlockSize;
    if (theRequest->fio_BlockBurst > blocksRequested) {
      theRequest->fio_BlockBurst = blocksRequested;
    }
    theRequest->fio_Flags |= FIO_INTERLEAVE_IO;
#else
    if (theFile->fi_Gap == 0) {
      absoluteOffset = theRequest->fio.io_Info.ioi_Offset;
      theRequest->fio_BlockBurst = theRequest->fio_BlockCount;
      theRequest->fio_Flags &= FIO_INTERLEAVE_IO;
    } else {
      burstModulus = theRequest->fio.io_Info.ioi_Offset % theFile->fi_Burst;
      absoluteOffset = (theRequest->fio.io_Info.ioi_Offset /
			theFile->fi_Burst) *
			(theFile->fi_Burst + theFile->fi_Gap) + burstModulus;
      theRequest->fio_BlockBurst = theFile->fi_Burst - burstModulus;
      blocksRequested = theRequest->fio.io_Info.ioi_Recv.iob_Len /
	theFile->fi_BlockSize;
      if (theRequest->fio_BlockBurst > blocksRequested) {
	theRequest->fio_BlockBurst = blocksRequested;
      }
      theRequest->fio_Flags |= FIO_INTERLEAVE_IO;
    }
#endif
#else
    absoluteOffset = theRequest->fio.io_Info.ioi_Offset;
    theRequest->fio_BlockBurst = theRequest->fio_BlockCount;
#endif
    devBlocksPerFilesystemBlock =
      theFile->fi_FileSystem->fs_DeviceBlocksPerFilesystemBlock;
    fileSystemBlocksPerFileBlock =
      theFile->fi_FileSystemBlocksPerFileBlock;
    do {
      avatar = theFile->fi_AvatarList[avatarIndex];
      flawLevel = (int32) ((avatar >> DRIVER_FLAW_SHIFT) & DRIVER_FLAW_MASK);
      avatar = (absoluteOffset * fileSystemBlocksPerFileBlock +
		(avatar & DRIVER_BLOCK_MASK)) * devBlocksPerFilesystemBlock;
      if (flawLevel < leastFlawLevel) {
	leastFlawLevel = flawLevel;
	closestAvatar = avatarIndex;
        absoluteBlock = avatar;
	leastDelta = avatar - whereNow;
	if (leastDelta < 0) {
	  leastDelta = - leastDelta;
	}
      } else if (flawLevel == leastFlawLevel) {
	delta = avatar - whereNow;
	if (delta < 0) {
	  delta = - delta;
	}
	if (delta < leastDelta) {
	  leastDelta = delta;
	  closestAvatar = avatarIndex;
	  absoluteBlock = avatar;
	}
      }
    } while (--avatarIndex >= 0);
    theRequest->fio_DevBlocksPerFileBlock = devBlocksPerFilesystemBlock;
    if (leastDelta == 0) {
      streaming = TRUE;
    } else {
      streaming = FALSE;
    }
    if (theDevice->odi_CatapultPhase == CATAPULT_AVAILABLE) {
      int32 cIndex;
      int32 cCount;
      int32 cIncr, cSign;
      CatapultPage *page;
      File *catapult;
      int32 indexIntoRun;
      int32 fsBlocksNeeded;
      int32 fileFsBlockOffset;
      int32 catapultBase, catapultBlocks;
      int32 err;
      int32 hitFile, hitRun, hitExactly, usedHit, usedIndex;
      DBUG(("Attempting catapult scan, least delta is %d\n", leastDelta));
      page = (CatapultPage *) theDevice->odi_CatapultPage->ioce_CachedBlock;
      cIndex = theDevice->odi_CatapultNextIndex;
      cIncr = 1;
      cSign = 1;
      indexIntoRun = usedIndex = 0;
      cCount = page->cp_Entries;
      hitFile = hitRun = hitExactly = usedHit = FALSE;
      fsBlocksNeeded = theRequest->fio_BlockBurst *
	fileSystemBlocksPerFileBlock;
      fileFsBlockOffset = theRequest->fio.io_Info.ioi_Offset *
	fileSystemBlocksPerFileBlock;
      catapultBase = theDevice->odi_CatapultFile->ofi_File->fi_AvatarList[0];
      catapultBlocks = theDevice->odi_CatapultFile->ofi_File->fi_BlockCount;
      DBUG(("Need %d filesystem blocks, from %d fs blocks into file\n",
	     fsBlocksNeeded, fileFsBlockOffset));
      DBUG(("Page has %d entries\n", cCount));
      while (cCount > 0 && !hitExactly) {
	if (cIndex >= 0 && cIndex <= page->cp_Entries) {
	  cCount --;
	  DBUG(("Examining index %d id 0x%X", cIndex,
		 page->cpe[cIndex].cpe_FileIdentifier));
	  DBUG((" file-offset %d run length %d catapult offset %d\n",
		 page->cpe[cIndex].cpe_FileBlockOffset,
		 page->cpe[cIndex].cpe_RunLength,
		 page->cpe[cIndex].cpe_RunOffset));
	  if (page->cpe[cIndex].cpe_FileIdentifier ==
	      theFile->fi_UniqueIdentifier) {
	    DBUG(("  It's the right file\n"));
	    hitFile = TRUE;
	    indexIntoRun = fileFsBlockOffset -
	      page->cpe[cIndex].cpe_FileBlockOffset;
	    if (indexIntoRun >= 0 &&
		indexIntoRun + fsBlocksNeeded <= page->cpe[cIndex].cpe_RunLength) {
/*
 Following calcs assume catapult file blocksize is same as
 filesystem block size... this is checked in the mount/open code.
*/
	      DBUG(("  It's a suitable run."));
	      avatar = catapultBase +
		page->cpe[cIndex].cpe_RunOffset + indexIntoRun;
	      DBUG(("  Its location in the filesystem is block %d\n", avatar));
	      if (!hitRun && !streaming) {
		theDevice->odi_CatapultMisses = 0;
		theDevice->odi_CatapultHits ++;
		DBUG(("Successive hit count is %d\n", theDevice->odi_CatapultHits));
		hitRun = TRUE;
	      }	      
	      delta = avatar - whereNow;
	      DBUG(("  Offset from arm position is %d blocks\n", delta));
	      if (delta < 0) {
		delta = -delta / theDevice->odi_CatapultHits + 8 /* discourage back-seeks */;
	      } else if (delta == 0) {
		hitExactly = TRUE;
	      } else {
		delta = (delta / theDevice->odi_CatapultHits) + 1;
	      }
	      DBUG(("  Weighted delta is %d\n", delta));
	      if (delta < leastDelta) {
		DBUG(("  Use it!\n"));
		leastDelta = delta;
		closestAvatar = -1; /* flag as being from catapult file */
		absoluteBlock = avatar;
		usedHit = TRUE;
		usedIndex = cIndex;
	      }
	    }
	  }
	}
	cIndex += cIncr * cSign;
	cIncr ++;
	cSign = -cSign;
      }
      if (usedHit) {
	theDevice->odi_CatapultNextIndex = usedIndex;
#ifndef PRODUCTION
	if (hitExactly) {
	  theDevice->odi_TotCatapultStreamedHits ++;
	  if (indexIntoRun == 0) {
	    theDevice->odi_TotCatapultSeeksAvoided ++;
	  }
	} else {
	  theDevice->odi_TotCatapultNonstreamedHits ++;
	}
	if (whereNow < catapultBase ||
	    whereNow >= catapultBase + catapultBlocks) {
	  theDevice->odi_TotCatapultTimesEntered ++;
	}
#endif
      } else if (hitRun) {
#ifndef PRODUCTION
	DBUG(("Catapult declined\n"));
	theDevice->odi_TotCatapultDeclined ++;
#endif
      } else {
	DBUG(("Catapult missed %s, %d+%d\n", theFile->fi_FileName,
	       fileFsBlockOffset, fsBlocksNeeded));
	theDevice->odi_CatapultHits = 0;
	theDevice->odi_TotCatapultMisses ++;
	if (!streaming) {
	  theDevice->odi_CatapultMisses ++;
	  theDevice->odi_TotCatapultNonstreamedMisses ++;
	  DBUG(("Miss count is %d\n", theDevice->odi_CatapultMisses));
	  if (theDevice->odi_CatapultMisses > page->cp_Entries -
	      theDevice->odi_CatapultNextIndex) {
	    DBUG0(("Page exhausted\n"));
	    if (page->cp_NextPage < 0 ||
		theDevice->odi_CatapultMisses > (page->cp_Entries >> 1) + 1) {
	      DBUG0(("Exhausted or stale catapult\n"));
	      theDevice->odi_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
	    } else {
	      DBUG0(("Defer I/O, load next catapult page\n"));
	      RemNode((Node *) theRequest);
	      AddHead(&theDevice->odi.hdi_RequestsDeferred, (Node *) theRequest);
	      fsCacheBusy ++;
	      catapult = theDevice->odi_CatapultFile->ofi_File;
	      theDevice->odi_CatapultPhase = CATAPULT_READING;
	      rawRequest->io_Info.ioi_Recv.iob_Buffer = page;
	      rawRequest->io_Info.ioi_Recv.iob_Len =
		catapult->fi_BlockSize;
	      rawRequest->io_Info.ioi_Flags = 0;
	      rawRequest->io_Info.ioi_Offset = 
		theDevice->odi_RawDeviceBlockOffset +
		  (catapult->fi_AvatarList[0] + page->cp_NextPage) *
		    catapult->fi_FileSystem->fs_DeviceBlocksPerFilesystemBlock;
	      rawRequest->io_Info.ioi_User =
		(uint32) catapult->fi_FileSystem;
	      rawRequest->io_CallBack = CatapultEndAction;
	      DBUG(("Starting read of catapult page %d\n",
		     page->cp_NextPage));
	      theDevice->odi.hdi_DeviceBusy = TRUE;
	      theDevice->odi_NextBlockAvailable = 
		catapult->fi_AvatarList[0] + page->cp_NextPage + 1;
	      err = SuperinternalSendIO(rawRequest);
	      if (err < 0) {
		DBUG(("Catapult SendIO error 0x%X\n", err));
		MySysErr(err);
	      }
	      return;
	    }
	  }
	}
      }
    }
    theRequest->fio_AvatarIndex = closestAvatar;
    theRequest->fio_AbsoluteBlockNumber = absoluteBlock;
    if (absoluteBlock < lowBlock) {
      lowBlock = absoluteBlock;
    }
    if (absoluteBlock > highBlock) {
      highBlock = absoluteBlock;
    }
  }
  if ((whereNow - lowBlock) < (highBlock - whereNow)) {
    sweep = BottomIsCloser;
  } else {
    sweep = TopIsCloser;
  }
/*
  N.B. the following loop may appear to take "i" one iteration too far.
  "Trust me, I know what I'm doing."

  Yes, it's a double-loop exchange sort.  Should this code ever turn out
  to be of significant CPU impact, it should be rewritten as a quickersort
  or a heapsort or something else with O(n log n) expected-time behavior.

  First - sort into ascending block order.
*/
  DBUG2(("Sorting requests\n"));
  nextBelow = -1;
  nextAbove = requestLimit;
  for (i = 0; i < numRequests; i++) {
    theRequest = requests[i];
    for (j = i+1; j < numRequests; j++) {
      nextRequest = requests[j];
      if (theRequest->fio_AbsoluteBlockNumber >
	  nextRequest->fio_AbsoluteBlockNumber) {
	requests[j] = theRequest;
	requests[i] = theRequest = nextRequest;
      }
    }
    if (theRequest->fio_AbsoluteBlockNumber < whereNow) {
      nextBelow = i;
    } else if (nextAbove == requestLimit) {
      nextAbove = i;
    }
  }
/*
  Next - reverse the order of the blocks lying below the current head
  position, so that they'll be handled in a downwards sweep

  TBD: some deep magic to handle overlapping I/O requests for interleaved
  files.  When we detect such a request, we should force those particular
  requests to be handled in the "upwards" direction, even if both requests
  are below our current head-position.
*/
  DBUG2(("Reversing requests below waterline\n"));
  reqIndex = 0;
  reqLimit = nextBelow;
  while (reqIndex < reqLimit) {
    theRequest = requests[reqIndex];
    requests[reqIndex] = requests[reqLimit];
    requests[reqLimit] = theRequest;
    reqIndex ++;
    reqLimit --;
  }
  DBUG2(("Queueing requests\n"));
/*
  Run through the two segments of the table and queue up the requests.
*/
  if (sweep == BottomIsCloser) {
    phase = 0;
  } else {
    phase = 1;
  }
  passes = 2;
  do {
    switch (phase) {
    case 0: /* Do the requests lying below the head position */
      DBUG2(("Doing requests below waterline\n"));
      reqIndex = 0;
      reqLimit = nextBelow;
      phase = 1;
      break;
    case 1: /* Do the requests lying above the head position */
      DBUG2(("Doing requests above waterline\n"));
      reqIndex = nextAbove;
      reqLimit = (int32) numRequests - 1;
      phase = 0;
      break;
    }
    while (reqIndex <= reqLimit) {
      DBUG2(("Queued request at %x\n", requests[reqIndex]));
      AddTail(&theDevice->odi.hdi_RequestsRunning,
	      (Node *) requests[reqIndex]);
      reqIndex++;
    }
  } while (--passes);
  theDevice->odi.hdi_RunningPriority = (uchar) topPriority;
  theDevice->odi.hdi_RequestPriority =
    HighestPriority(&theDevice->odi.hdi_RequestsToDo);
  theDevice->odi_DeferredPriority =
    HighestPriority(&theDevice->odi.hdi_RequestsDeferred);
  DBUG2(("I/O scheduling complete\n"));
}

void StartOptimizedDiskIo (OptimizedDisk *theDevice)
{
  FileIOReq *theRequest;
  IOReq *rawRequest;
  File *theFile;
  int32 err;
  if (theDevice->odi.hdi_DeviceBusy) {
    return;
  }
  theDevice->odi.hdi_DeviceBusy = TRUE;
 doit:
  if (ISEMPTYLIST(&theDevice->odi.hdi_RequestsRunning)) {
    DBUG2(("No I/O to start, bailing out\n"));
    theDevice->odi.hdi_DeviceBusy = FALSE;
    return;
  }
  theRequest = (FileIOReq *) FIRSTNODE(&theDevice->odi.hdi_RequestsRunning);
#ifdef DEBUG2
  Superkprintf("Starting up I/O request %x\n", theRequest);
#endif
  theFile = ((OpenFile *) theRequest->fio.io_Dev)->ofi_File;
  if (theFile->fi_FileSystem->fs_Flags & FILESYSTEM_IS_OFFLINE) {
    DBUG2(("Filesystem offline, killing request\n"));
    RemNode((Node *) theRequest);
    theRequest->fio.io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
    SuperCompleteIO((IOReq *) theRequest);
    SuperinternalSignal(fileFolio->ff_Daemon.ffd_Task,
			fileFolio->ff_Daemon.ffd_RescheduleSignal);
    goto doit;
  }
  rawRequest = theDevice->odi_RawDeviceRequest;
  rawRequest->io_CallBack = theDevice->odi.hdi_EndAction;
  rawRequest->io_Info = theRequest->fio.io_Info;
  /***
    Pick up the absolute block number in the request, and use it for
    this transfer.  I/O transfers to and from interleaved files
    must be broken down into smaller chunks... we calculated the
    first-burst size during the scheduling process.
    ***/
  rawRequest->io_Info.ioi_Offset =
    (int32) theRequest->fio_AbsoluteBlockNumber +
      theDevice->odi_RawDeviceBlockOffset;
  if (theRequest->fio_Flags & FIO_INTERLEAVE_IO) {
    rawRequest->io_Info.ioi_Recv.iob_Len =
      (int32) ((OpenFile *) theRequest->fio.io_Dev)->ofi_File->fi_BlockSize *
	theRequest->fio_BlockBurst;
  }
  rawRequest->io_Info.ioi_User = (ulong) theRequest;
  rawRequest->io_Info.ioi_Flags = IO_QUICK;
  rawRequest->io_Info.ioi_Unit = theDevice->odi.hdi_RawDeviceUnit;
  rawRequest->io_Info.ioi_CmdOptions = 0; /* use defaults */
#ifdef DEBUG
  Superkprintf("Raw I/O request for offset %d, buf %lx, bytes %d, endaction %lx\n",
	       rawRequest->io_Info.ioi_Offset,		 
	       rawRequest->io_Info.ioi_Recv.iob_Buffer,
	       rawRequest->io_Info.ioi_Recv.iob_Len,
	       rawRequest->io_CallBack);		 
#endif
  DBUG(("Request block burst %d blocks-per %d\n", 
	 theRequest->fio_BlockBurst,
	 theFile->fi_FileSystemBlocksPerFileBlock));
  theDevice->odi_NextBlockAvailable = theRequest->fio_AbsoluteBlockNumber +
    theRequest->fio_BlockBurst * theFile->fi_FileSystemBlocksPerFileBlock;
  DBUG(("Read FS block %d, bytes %d, next block avail %d\n",
	 theRequest->fio_AbsoluteBlockNumber,
	 rawRequest->io_Info.ioi_Recv.iob_Len,
	 theDevice->odi_NextBlockAvailable));
  err = SuperinternalSendIO(rawRequest);
  if (err < 0) {
    qprintf(("Error %d from SuperinternalSendIO to device!\n", err));
#ifndef PRODUCTION
    MySysErr(rawRequest->io_Error);
#endif
  } 
  return; /* add error-checking to previous call! */
}

void AbortOptimizedDiskIo(IOReq *theRequest)
{
  uint32 interrupts;
  OptimizedDisk *theDevice;
  OpenFile *theOpenFile;
  theOpenFile = (OpenFile *) theRequest->io_Dev;
  theDevice = (OptimizedDisk *)theOpenFile->ofi_File->fi_FileSystem->fs_Device;
  interrupts = Disable(); /*** DISABLE ***/
/*
  If this is the request that the device is currently servicing, then
  simply abort the lower-level I/O request - the endaction code will
  perform the cleanup for both levels.  If this request is not yet being
  serviced, dequeue and kill it immediately.
*/
  if (theDevice->odi.hdi_DeviceBusy &&
   theDevice->odi_RawDeviceRequest->io_Info.ioi_User == (ulong) theRequest) {
    SuperinternalAbortIO(theDevice->odi_RawDeviceRequest);
  } else {
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
    RemNode((Node *) theRequest);
    SuperCompleteIO(theRequest);
  }
  Enable(interrupts);
  return;
}

IOReq *CatapultEndAction(IOReq *rawRequest)
{
  FileSystem *theFileSystem;
  OptimizedDisk *theDevice;
  theFileSystem = (FileSystem *) rawRequest->io_Info.ioi_User;
  theDevice = (OptimizedDisk *) theFileSystem->fs_Device;
  theDevice->odi_CatapultPhase = CATAPULT_MUST_VERIFY;
  theDevice->odi.hdi_DeviceBusy = FALSE;
  SuperinternalSignal(fileFolio->ff_Daemon.ffd_Task,
		      fileFolio->ff_Daemon.ffd_RescheduleSignal);
  return (IOReq *) NULL;
}

IOReq *OptimizedDiskEndAction(IOReq *rawRequest)
{
  FileIOReq *userRequest;
  OpenFile *theOpenFile;
  OptimizedDisk *theDevice;
  File *theFile;
  FileSystem *theFileSystem;
  int32 avatarIndex;
  uint32 avatar, flawLevel;
  int32 bytesRead;
  int32 significantErr;
  userRequest = (FileIOReq *) rawRequest->io_Info.ioi_User;
  theOpenFile = (OpenFile *) userRequest->fio.io_Dev;
  theFile = theOpenFile->ofi_File;
  theFileSystem = theFile->fi_FileSystem;
  theDevice = (OptimizedDisk *) theFileSystem->fs_Device;
  theDevice->odi.hdi_DeviceBusy = FALSE;
  bytesRead = rawRequest->io_Actual;
#ifdef INTERLEAVED_FILES
/*
  Check for an interleaved I/O, not yet completed, no errors yet.  If
  we find one, update the I/O request in place and continue with the
  I/O.

  TBD - handle the case in which two simultaneously-running I/O requests
  are scarfing blocks from two files which are interleaved with one
  another.  That will be grody and awful, but it probably needs to be
  done.

  Also TBD - handle boinking in the middle of an interleaved read.
*/
  if ((userRequest->fio_Flags & FIO_INTERLEAVE_IO) != 0 &&
      rawRequest->io_Error == 0) {
    userRequest->fio.io_Actual += bytesRead;
    userRequest->fio_BlockCount -= userRequest->fio_BlockBurst;
    if (userRequest->fio_BlockCount > 0) {
/*
  More to do on this request.  Move the buffer pointer past the portion
  we just read.
*/
      userRequest->fio.io_Info.ioi_Recv.iob_Buffer =
	rawRequest->io_Info.ioi_Recv.iob_Buffer =
	  ((char *) rawRequest->io_Info.ioi_Recv.iob_Buffer) + bytesRead;
/*
  Decrement total-amount-yet-to-be-read.
*/
      userRequest->fio.io_Info.ioi_Recv.iob_Len -= bytesRead;
/*
  Advance offsets.  User-request offset advances by the number of file
  blocks we just bursted in.  Device-request offset advances by enough
  to get us to the beginning of the next burst, in the device's
  frame of reference.
*/
      userRequest->fio.io_Info.ioi_Offset += userRequest->fio_BlockBurst;
      rawRequest->io_Info.ioi_Offset +=
	(int32) (userRequest->fio_DevBlocksPerFileBlock *
	       (userRequest->fio_BlockBurst + theFile->fi_Gap));
/*
  Figure out how many blocks to read in during the next iteration
*/
      if (userRequest->fio_BlockCount < theFile->fi_Burst) {
	userRequest->fio_BlockBurst = userRequest->fio_BlockCount;
      } else {
	userRequest->fio_BlockBurst = theFile->fi_Burst;
      }
/*
  Read the desired number of bytes
*/
      rawRequest->io_Info.ioi_Recv.iob_Len = theFile->fi_BlockSize *
	userRequest->fio_BlockBurst;
      return rawRequest;  /* Keep going with this I/O */
    }
  }
#endif
/*
  Pass back the completion information and dequeue the request from the
  RequestsRunning list.
*/
  RemNode((Node *) userRequest);
  if ((rawRequest->io_Error & 0x000001FF) == ((ER_C_STND << ERR_CLASHIFT) +
					      (ER_Aborted << ERR_ERRSHIFT))) {
    userRequest->fio.io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
  } else if ((rawRequest->io_Error & 0x000001FF) == ((ER_C_STND << ERR_CLASHIFT) +
						     (ER_DeviceOffline << ERR_ERRSHIFT))) {
    userRequest->fio.io_Error = rawRequest->io_Error;
    theFileSystem->fs_Flags |= FILESYSTEM_IS_OFFLINE;
  } else {      
    userRequest->fio.io_Error = rawRequest->io_Error;
/*
  Check for media errors.  If a medium error occurs on a file which
  has more than one avatar, bump up the soft-error count on this avatar
  (unless the limit has been reached) and requeue the I/O for another
  try.  For the purposes of this code, parameter errors are considered
  equivalent to medium errors, since the CD-ROM drive can return such an
  error code if a block header is damaged beyond repair.  Errors occuring
  during a catapult-file access cause the entire catapult file to be
  considered NFG - I/O will be requeued using the real avatars.
*/
    significantErr = userRequest->fio.io_Error & 0x000001FF;
    if (significantErr == ((ER_C_STND << ERR_CLASHIFT) +
			   (ER_MediaError << ERR_ERRSHIFT)) ||
	significantErr == ((ER_C_STND << ERR_CLASHIFT) +
			   (ER_ParamError << ERR_ERRSHIFT))) {
      if (theFile->fi_LastAvatarIndex != 0) {
	avatarIndex = (int32) userRequest->fio_AvatarIndex;
	if (avatarIndex < 0) {
	  theDevice->odi_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
	} else {
	  avatar = theFile->fi_AvatarList[avatarIndex];
	  flawLevel = (int32) ((avatar >> DRIVER_FLAW_SHIFT) & DRIVER_FLAW_MASK);
	  if (flawLevel < DRIVER_FLAW_SOFTLIMIT) {
	    theFile->fi_AvatarList[avatarIndex] += (1L << DRIVER_FLAW_SHIFT);
	  }
	}
	AddTail(&theDevice->odi.hdi_RequestsToDo,
		(Node *) userRequest);
	goto nextIO;
      }
    }
  }
  userRequest->fio.io_Actual += bytesRead;
/*
  That request is done (successfully or with errors from which we cannot
  recover).  Signal completion and check for a follow-on I/O from an
  upper-level driver.
*/
  SuperCompleteIO((IOReq *) userRequest);
/*
  Check to see if a boink has occurred.  If so, move any already-
  scheduled requests to the defer list.  Do likewise if we realize that
  the filesystem is offline... the scheduler will accept responsibility
  for nuking all pending requests.
*/
nextIO:
  if ((theDevice->odi.hdi.dev.n_Flags & DEVICE_BOINK) ||
      (theFileSystem->fs_Flags & FILESYSTEM_IS_OFFLINE)) {
    while (!ISEMPTYLIST(&theDevice->odi.hdi_RequestsRunning)) {
      userRequest = (FileIOReq *) RemTail(&theDevice->odi.hdi_RequestsRunning);
      AddHead(&theDevice->odi.hdi_RequestsDeferred, (Node *) userRequest);
    }
    theDevice->odi.hdi.dev.n_Flags &= ~DEVICE_BOINK;
  }
  if (ISEMPTYLIST(&theDevice->odi.hdi_RequestsRunning)) {
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
    (*theDevice->odi.hdi_ScheduleIO)((HighLevelDisk *) theDevice);
    if (ISEMPTYLIST(&theDevice->odi.hdi_RequestsRunning)) {
      return (IOReq *) NULL;
    }
  }
  userRequest = (FileIOReq *) FIRSTNODE(&theDevice->odi.hdi_RequestsRunning);
  rawRequest = theDevice->odi_RawDeviceRequest;
  rawRequest->io_CallBack = theDevice->odi.hdi_EndAction;
  rawRequest->io_Info = userRequest->fio.io_Info;
/***
  Pick up the absolute block number in the request, and use it for
  this transfer.  I/O transfers to and from interleaved files
  must be broken down into one-file-block chunks.
 ***/
  rawRequest->io_Info.ioi_Offset =
    (int32) userRequest->fio_AbsoluteBlockNumber +
      theDevice->odi_RawDeviceBlockOffset;
#ifdef INTERLEAVED_FILES
  if (userRequest->fio_Flags & FIO_INTERLEAVE_IO) {
    rawRequest->io_Info.ioi_Recv.iob_Len =
      (int32) ((OpenFile *) userRequest->fio.io_Dev)->ofi_File->fi_BlockSize;
  }
#endif
  rawRequest->io_Info.ioi_User = (ulong) userRequest;
  rawRequest->io_Info.ioi_CmdOptions = 0; /* use defaults */
  rawRequest->io_Flags = 0;
  theDevice->odi.hdi_DeviceBusy = TRUE;
  theOpenFile = (OpenFile *) userRequest->fio.io_Dev;
  theFile = theOpenFile->ofi_File;
  theDevice->odi_NextBlockAvailable = userRequest->fio_AbsoluteBlockNumber +
    userRequest->fio_BlockBurst * theFile->fi_FileSystemBlocksPerFileBlock;
  return rawRequest;
}

static uchar HighestPriority(List *theList)
{
  int32 highest = 0;
  Node *theNode;
  int32 interrupts;
  interrupts = Disable();
  for (theNode = FIRSTNODE(theList);
       ISNODE(theList, theNode);
       theNode = NEXTNODE(theNode)) {
    if (theNode->n_Priority > highest) {
      highest = theNode->n_Priority;
    }
  }
  Enable(interrupts);
  return (uchar) highest;
}
