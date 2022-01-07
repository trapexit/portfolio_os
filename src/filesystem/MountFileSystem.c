/*****

$Id: MountFileSystem.c,v 1.41 1994/09/22 19:39:54 dplatt Exp $

$Log: MountFileSystem.c,v $
 * Revision 1.41  1994/09/22  19:39:54  dplatt
 * Print times-entered-catapult at dismount time.
 *
 * Revision 1.40  1994/09/09  23:00:31  dplatt
 * Whoops... changed the logic, didn't change the name of the function
 * I called.  Mounts worked only on device which did IO_QUICK.
 *
 * Revision 1.39  1994/09/09  03:10:06  dplatt
 * Use new SuperInternalDoIO() call.
 *
 * Revision 1.38  1994/07/26  21:07:46  dplatt
 * Load Catapult file index at mount time, if the Catapult file is
 * present.  Make sure that the Mount call sets up file-folio private
 * data, so that this routine can call Open and other internal routines.
 *
 * Revision 1.37  1994/06/06  23:10:59  dplatt
 * Change debug code around a bit.
 *
 * Revision 1.36  1994/05/13  00:42:22  cramer
 * fix compilation warnings.
 *
 * Revision 1.35  1994/04/28  00:13:54  dplatt
 * Clean up debug stuff.  Report a "not supported" error, rather than
 * "no such filesystem", if an attempt is made to mount a device which
 * says it can't support filesystems.
 *
 * Revision 1.34  1994/04/27  23:32:25  shawn
 * mount/dismount allowed by priv tasks only.
 *
 * Revision 1.33  1994/04/22  19:42:40  dplatt
 * Construct mount point name, based on device name, unit, and offset.
 *
 * Revision 1.32  1994/04/07  00:14:31  shawn
 * ifdef'ed FS_DIRSEMA
 *
 * Revision 1.31  1994/03/26  00:50:32  dplatt
 * Network devices do not yet say that they are filesystem-
 * capable.  Try BARF mount before rejecting.
 *
 * Revision 1.30  1994/03/25  01:34:47  dplatt
 * Debug stuff...
 *
 * Revision 1.29  1994/03/24  01:48:43  dplatt
 * Add support for kernel events, hot-mount/hot-dismount.  Minor
 * security enhancements and rearrangements of various things.
 *
 * Revision 1.28  1993/12/17  00:44:08  dplatt
 * Remove extraneous debug message
 *
 * Revision 1.27  1993/12/16  00:51:48  dplatt
 * Add hook to BARF filesystem mounter, if compiled in.
 * Set null CloseFile hooks.
 *
 * Revision 1.26  1993/11/24  06:57:24  limes
 * Quiet down the debug messages.
 *
 * Revision 1.25  1993/09/01  23:14:07  dplatt
 * Scavenge File items with zero use count... both on-the-fly (flush
 * LRU if more than 16 exist) and when a task exits (flush all).
 *
 * Revision 1.24  1993/08/30  18:53:17  dplatt
 * Make the DismountFileSystem SWI work properly... use
 * SuperInternalDeleteItem (to override ownership rules), and clean up
 * the filesystem-type-specific stuff (ioreqs, buffers) and the
 * HighLevelDisk device item.
 *
 * Revision 1.23  1993/07/20  07:04:19  dplatt
 * Directory cache
 *
 * Revision 1.22  1993/07/11  21:24:46  dplatt
 * Report errors during SetItemOwner
 *
 * Revision 1.22  1993/07/11  21:24:46  dplatt
 * Report errors during SetItemOwner
 *
 * Revision 1.21  1993/07/03  04:28:56  dplatt
 * Add a label-check bypass for Dale to use tonight.
 *
 * Revision 1.20  1993/07/03  00:31:06  dplatt
 * Change label mechanisms to keep the lawyers happy
 *
 * Revision 1.19  1993/06/15  20:15:34  dplatt
 * Don't cache file info for Macintosh /remote files
 *
 * Revision 1.18  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.17  1993/05/31  03:33:05  dplatt
 * Tweaks, and filesystem dismounting
 *
 * Revision 1.16  1993/05/08  01:08:14  dplatt
 * Add flat-file-system/NVRAM support, and recover from RCS bobble
 *
 * Revision 1.15  1993/04/26  20:13:35  dplatt
 * Quiet coldstart
 *
 * Revision 1.14  1993/04/23  21:49:56  dplatt
 * CD-rom enhancements, cleanup
 *
 * Revision 1.13  1993/04/22  21:03:15  dplatt
 * New features, timeout support, bug fixes
 *
 * Revision 1.12  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.11  1993/02/21  03:33:46  dplatt
 * Report volume block size at mount time during debug
 *
 * Revision 1.10  1993/02/11  19:39:37  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.9  1993/02/09  01:47:20  dplatt
 * Reorganize and update for developer release
 *
 * Revision 1.8  1993/01/05  20:57:47  dplatt
 * CES changes redux
 *
 * Revision 1.7  1992/12/22  07:58:10  dplatt
 * Magneto 3 changes and CD-ROM support
 *
 * Revision 1.6  1992/12/08  05:59:52  dplatt
 * Magenta changes
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
 * Revision 1.1  1992/09/11  00:42:30  dplatt
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
  InitFileSystem.c - stub code to initialize the filesystem at boot time.
  Most of this is bogus scaffolding for the debug/simulation phase of
  development, and will need to be redone.
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
#include "semaphore.h"
#include "operror.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "discdata.h"
#include "event.h"

#include "super.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"
#include "ctype.h"

#undef DEBUG
#undef DEBUG2
#undef TRACEDISMOUNT

#ifdef PRODUCTION
# undef TRACEDISMOUNT
#endif

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

#ifdef PRODUCTION
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define oqprintf(x) /* x */
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#ifndef PRODUCTION
extern void DumpFileList(void);
#endif

#ifdef BARFFS
extern Item MountBarfFilesystem(Device *theDevice, int32 unit,
				uint32 blockOffset);
#endif

extern OpenFile *OpenPath (File *startingDirectory, char *path);
extern Node *AllocFileNode(int32 theSize, int32 theType);
extern void FreeFileNode (void *it);

extern IoCacheEntry *AddCacheEntry(OpenFile *theOpenFile, int32 blockNum,
				   void *buffer,
				   uint8 priority, uint32 format,
				   uint32 bytes);

extern FileFolioTaskData *SetupFileFolioTaskData(void);

extern int32 internalDeleteItem(Item,struct Task *);

int32 QueueOptimizedDiskRequest(OptimizedDisk *theDevice, IOReq *theRequest);
void ScheduleOptimizedDiskIo(OptimizedDisk *theDevice);
void StartOptimizedDiskIo (OptimizedDisk *theDevice);
IOReq *OptimizedDiskEndAction(IOReq *theRequest);
IOReq *CatapultEndAction(IOReq *theRequest);
void AbortOptimizedDiskIo(IOReq *userRequest);

int32 QueueLinkedMemDiskRequest(LinkedMemDisk *theDevice, IOReq *theRequest);
void ScheduleLinkedMemDiskIo(LinkedMemDisk *theDevice);
void StartLinkedMemDiskIo (LinkedMemDisk *theDevice);
IOReq *LinkedMemDiskEndAction(IOReq *theRequest);
void AbortLinkedMemDiskIo(IOReq *userRequest);

const TagArg filesystemArgs[] = {
  { FILESYSTEM_TAG_PRI,     (void *)1, },
  { FILESYSTEM_TAG_END,     0, },
};

const TagArg fsDevArgs[] = {
  { TAG_ITEM_PRI,            (void *)1, },
  { CREATEDEVICE_TAG_DRVR,   NULL },
  { TAG_ITEM_NAME,           "filesystem" },
  { TAG_END,                 0, },
};

extern int32 fsCacheBusy;

extern void MySysErr(int32 err);

static void Nuke(void *foo) {
  if (foo) {
    SuperDeleteItem(((ItemNode *)foo)->n_Item);
    DBUG2(("Released.\n"));
  }
}

void GiveDaemon(void *foo) {
  Err err;
  DBUG(("Give item at 0x%x to daemon\n", foo));
  if (foo && fileFolio->ff_Daemon.ffd_Task) {
    DBUG(("Transfer item 0x%x to task 0x%x\n",
	   ((ItemNode *)foo)->n_Item,
	   fileFolio->ff_Daemon.ffd_Task->t.n_Item));
    err = SuperSetItemOwner(((ItemNode *)foo)->n_Item,
			    fileFolio->ff_Daemon.ffd_Task->t.n_Item);
    DBUG(("SuperSetItemOwner returns 0x%x\n", err));
#ifndef PRODUCTION    
    if (err < 0) {
      DBUG0(("SuperSetItemOwner failed on item 0x%x\n",
	     ((ItemNode *)foo)->n_Item));
      MySysErr(err);
    }
#endif
  }
}

Err FilesystemEvent(FileSystem *fs, uint32 eventCode)
{
  struct {
    EventFrameHeader    efh;
    FilesystemEventData fsed;
  } event;
  Err err;
  DBUG(("Reporting event\n"));
  memset(&event, 0, sizeof event);
  event.efh.ef_ByteCount = sizeof event;
  event.efh.ef_EventNumber = (uint8) eventCode;
  event.fsed.fsed_FilesystemItem = fs->fs.n_Item;
  memcpy(event.fsed.fsed_Name, fs->fs_FileSystemName,
	 sizeof event.fsed.fsed_Name);
  err = SuperReportEvent(&event);
#ifdef DEBUG
  if (err < 0) {
    MySysErr(err);
  }
  DBUG(("Event done, returned 0x%X\n", err));
#endif
  return err;
}

void CatDotted(char *string, uint32 num)
{
  uint32 digits, residue, len;
  len = strlen(string);
  digits = 0;
  residue = num;
  do {
    digits ++;
    residue /= 10;
  } while (residue != 0);
  string[len+digits+1] = '\0';
  string[len] = '.';
  residue = num;
  do {
    string[len+digits] = '0' + (char) (residue % 10);
    digits --;
    residue /= 10;
  } while (residue != 0);
}

Item InitFileSystem(Device *theDevice, int32 unit, uint32 blockOffset)
{
  FileSystem *fileSystem = NULL;
  OptimizedDisk *fsDevice = NULL;
  LinkedMemDisk *lmDevice = NULL;
  HighLevelDisk *hld = NULL;
  Item fsDeviceItem, filesystemItem, rootFileItem, lmDeviceItem;
  Item ioReqItem;
#ifdef SEMAPHORE_LMD
  Item semItem;
  Semaphore *sem = NULL;
#endif
  File *fsRoot = NULL;
  OpenFile *catapult = NULL;
  IoCacheEntry *catapultPage = NULL;
  DiscLabel *discLabel = NULL;
  FileIOReq **optBuffer = NULL;
  DeviceStatus devStatus;
  IOReq *rawIOReq = NULL;
  int32 fileSize;
  int32 avatarIndex, avatarBlock;
  int32 errno, gotLabel;
  int32 discLabelSize = 0;
  int32 deviceBlockSize;
  Err err = 0;
#ifdef DEBUG2
  int32 i;
  int32 j;
  uint8 *foo, c;
#endif
  TagArg deviceArgBlock[4];
  TagArg ioReqTags[2];
/*
  Create the filesystem device which resides on this physical
  device.
*/
  DBUG(("Looking for filesystem on %s unit %d offset %d\n",
	theDevice->dev.n_Name, unit, blockOffset));
/*****
  Here we need to create an IOReq for the raw device.
 *****/
  DBUG(("Creating IOReq\n"));
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) theDevice->dev.n_Item;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    DBUG(("Can't allocate an internal IOReq for raw device"));
    err = ioReqItem;
    goto nuke;
  }
  rawIOReq = (IOReq *) LookupItem(ioReqItem);
  rawIOReq->io_Info.ioi_Unit = (uint8) unit;
  DBUG(("IOReq is item 0x%X at 0x%X\n", ioReqItem, rawIOReq));
/*
  Query the raw device to determine its block size and block count,
  and record these in the optimized-device structure.
*/
  DBUG(("Getting device status\n"));
  rawIOReq->io_Info.ioi_Command = CMD_STATUS;
  rawIOReq->io_Info.ioi_Flags = IO_QUICK;
  rawIOReq->io_Info.ioi_Unit = (uint8) unit;
  rawIOReq->io_Info.ioi_Offset = 0;
  rawIOReq->io_Info.ioi_Send.iob_Buffer = NULL;
  rawIOReq->io_Info.ioi_Send.iob_Len = 0;
  rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) &devStatus;
  rawIOReq->io_Info.ioi_Recv.iob_Len = sizeof devStatus;
  devStatus.ds_DeviceBlockSize = FILESYSTEM_DEFAULT_BLOCKSIZE;
  devStatus.ds_DeviceBlockCount = 0;
  errno = SuperInternalDoIO(rawIOReq);
  if (errno < 0 || (errno = rawIOReq->io_Error) < 0) {
    DBUG(("I/O error 0x%x getting device status\n", errno));
#ifdef DEBUG
    MySysErr(errno);
#endif
    goto nuke;
  }
  deviceBlockSize = devStatus.ds_DeviceBlockSize;
  DBUG(("Got device status\n"));
  DBUG(("Device says block size is %d, block count is %d\n",
	  devStatus.ds_DeviceBlockSize,
	  devStatus.ds_DeviceBlockCount));
#ifdef BARFFS
  if (devStatus.ds_FamilyCode == DS_DEVTYPE_NETWORK) {
    oqprintf(("It's a network device, go barf on it\n"));
    SuperDeleteItem(ioReqItem);
    return MountBarfFilesystem(theDevice, unit, blockOffset);
  }
#endif
  if (!(devStatus.ds_DeviceUsageFlags & DS_USAGE_FILESYSTEM)) {
    oqprintf(("Device cannot support filesystem\n"));
    err = MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
    goto nuke;
  }
  err = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFileSystem);
  if (devStatus.ds_DeviceBlockCount <= DISC_LABEL_OFFSET) {
    oqprintf(("Device too small to hold a filesystem\n"));
    goto nuke;
  }
  if (blockOffset == 0 && devStatus.ds_FamilyCode == DS_DEVTYPE_CDROM) {
    blockOffset = 150;
    DBUG(("Device is a CD-ROM, setting starting offset to 150\n"));
  }
  discLabelSize = (int32) ((sizeof (DiscLabel) + deviceBlockSize - 1) /
			   deviceBlockSize) * deviceBlockSize;
  DBUG(("Allocating disc-label space, %d bytes\n", discLabelSize));
  discLabel = (DiscLabel *) AllocMem(discLabelSize, MEMTYPE_FILL);
  if (!discLabel) {
    DBUG(("Could not allocate disc label\n"));
    err = NOMEM;
    goto nuke;
  }
  DBUG(("Buffer allocated at 0x%x\n", discLabel));
/*
  Read in the disc label, from any available avatar, and confirm that
  it's what we expected.  Bail if not.
*/
  gotLabel = 0;
  avatarBlock = 0;
  for (avatarIndex = -1;
       avatarIndex <= DISC_LABEL_HIGHEST_AVATAR && !gotLabel;
       avatarIndex++) {
    DBUG(("Probing for label at block %d", avatarBlock));
/*
   Unit, null send buffer already set up from CMD_STATUS
*/
    rawIOReq->io_Info.ioi_Command = CMD_READ;
    rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) discLabel;
    rawIOReq->io_Info.ioi_Recv.iob_Len = discLabelSize;
    rawIOReq->io_Info.ioi_Flags = IO_QUICK;
    rawIOReq->io_Info.ioi_Offset = (int32) avatarBlock + blockOffset;
    errno = SuperInternalDoIO(rawIOReq);   
    if (errno < 0 || (errno = rawIOReq->io_Error) < 0) {
     DBUG((" [couldn't read]\n"));
    } else {
      DBUG((" [got %d]", rawIOReq->io_Actual));
      if (discLabel->dl_RecordType != 1 ||
#ifndef FORDALE
	  discLabel->dl_VolumeSyncBytes[0] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_VolumeSyncBytes[1] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_VolumeSyncBytes[2] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_VolumeSyncBytes[3] != VOLUME_SYNC_BYTE ||
	  discLabel->dl_VolumeSyncBytes[4] != VOLUME_SYNC_BYTE ||
#endif
	  (discLabel->dl_VolumeStructureVersion != VOLUME_STRUCTURE_OPERA_READONLY &&
	   discLabel->dl_VolumeStructureVersion != VOLUME_STRUCTURE_LINKED_MEM) ||
	  discLabel->dl_RootDirectoryLastAvatarIndex > ROOT_HIGHEST_AVATAR)
	{
	  DBUG((" [not valid]\n"));
#ifdef DEBUG2
	  foo = (uint8 *) discLabel;
	  for (i = 0; i < 20; i++) {
	    for (j = 0; j < 16; j++) {
	      qprintf(("%02x ", *(foo+j)));
	    }
	    qprintf(("  "));
	    for (j = 0; j < 16; j++) {
	      c = *foo++;
	      if (!isprint(c)) c = '.';
	      qprintf(("%c", c));
	    }
	    qprintf(("\n"));
	  }
#endif
	}
      else {
	gotLabel = 1;
	DBUG((" [OK]\n"));
      }
    }
    if (avatarIndex < 0) {
      avatarBlock = DISC_LABEL_OFFSET;
    } else {
      avatarBlock += DISC_LABEL_AVATAR_DELTA;
    }
  }
  if (gotLabel) {
    DBUG(("Label '%s' validated, ID %d\n",
	   discLabel->dl_VolumeIdentifier,
	   discLabel->dl_VolumeUniqueIdentifier));
  } else {
    DBUG(("Could not find a valid label on %s\n",
	  theDevice->dev.n_Name));
    err = -1;
    goto nuke;
  }
  if (FindNamedNode(&fileFolio->ff_Filesystems,
		    discLabel->dl_VolumeIdentifier) != (Node *) NULL) {
    qprintf(("Duplicate filesystem name %s, cannot mount\n",
	     discLabel->dl_VolumeIdentifier));
    err = MakeFErr(ER_SEVER,ER_C_NSTND,ER_FS_DuplicateFile);

    goto nuke;
  }
/*
  Label found.
*/

  DBUG(("Creating optimization buffer\n"));
  optBuffer = (FileIOReq **) AllocMem(DEVICE_SORT_COUNT * sizeof(FileIOReq *),
				       MEMTYPE_FILL);
  if (!optBuffer) {
    goto nuke;
  }
/*
  Create a filesystem which resides on the device
*/
  DBUG(("Creating filesystem node\n"));
  filesystemItem = SuperCreateItem(MKNODEID(FILEFOLIO,FILESYSTEMNODE),
				   (TagArg *) filesystemArgs);
  fileSystem = (FileSystem *) LookupItem(filesystemItem);
  if (!fileSystem) {
    DBUG(("Could not create filesystem\n"));
    err = filesystemItem;
    goto nuke;
  }
  switch (discLabel->dl_VolumeStructureVersion) {
  case VOLUME_STRUCTURE_OPERA_READONLY:
    DBUG(("Creating optimized-disk device\n"));
    memcpy(deviceArgBlock, fsDevArgs, sizeof deviceArgBlock);
    deviceArgBlock[1].ta_Arg = (void *) fileDriver->drv.n_Item;
    fsDeviceItem = SuperCreateSizedItem(MKNODEID(KERNELNODE,DEVICENODE),
					deviceArgBlock,
					sizeof (OptimizedDisk));
    DBUG(("Got item 0x%x\n", fsDeviceItem));
    fsDevice = (OptimizedDisk *) LookupItem(fsDeviceItem);
    if (!fsDevice) {
      DBUG(("Could not allocate optimized-disk device\n"));
      MySysErr(fsDeviceItem);
      err = fsDeviceItem;
      goto nuke;
    }
    DBUG(("Initializing optimized-disk device\n"));
    hld = (HighLevelDisk *) fsDevice;
    fsDevice->odi.hdi_DeviceType = FILE_DEVICE_OPTIMIZED_DISK;
    fsDevice->odi.hdi_RawDeviceItem = theDevice->dev.n_Item;
    fsDevice->odi.hdi_RawDeviceUnit = (uint8) unit;
    fsDevice->odi_BlockSize  = devStatus.ds_DeviceBlockSize;
    fsDevice->odi_BlockCount = devStatus.ds_DeviceBlockCount;
    fsDevice->odi_RawDeviceBlockOffset = blockOffset;
    fsDevice->odi_RawDeviceRequest = rawIOReq;
    fsDevice->odi_DeferredPriority = 0;
    fsDevice->odi.hdi_QueueRequest = (FileDriverQueueit) QueueOptimizedDiskRequest;
    fsDevice->odi.hdi_ScheduleIO = (FileDriverHook) ScheduleOptimizedDiskIo;
    fsDevice->odi.hdi_StartIO = (FileDriverHook) StartOptimizedDiskIo;
    fsDevice->odi.hdi_EndAction = (FileDriverEA) OptimizedDiskEndAction;
    fsDevice->odi.hdi_AbortIO = (FileDriverAbort) AbortOptimizedDiskIo;
    fsDevice->odi.hdi_CloseFile = (FileDriverClose) NULL;
    fsDevice->odi_RequestSort = optBuffer;
    fsDevice->odi_RequestSortLimit = DEVICE_SORT_COUNT;
    fsDevice->odi_NextBlockAvailable = 0;
    fsDevice->odi_CatapultFile = (OpenFile *) NULL;
    fsDevice->odi_CatapultPage = (IoCacheEntry *) NULL;
    fsDevice->odi_CatapultPhase = CATAPULT_NONE;
#ifdef DOCACHE
    if (fsDevice->odi_BlockSize == FILESYSTEM_DEFAULT_BLOCKSIZE) {
      DBUG(("Filesystem directory caching enabled\n"));
      fileSystem->fs_Flags = FILESYSTEM_IS_READONLY |
	FILESYSTEM_CACHEWORTHY;
    } else {
      fileSystem->fs_Flags = FILESYSTEM_IS_READONLY;
    }
#else
    fileSystem->fs_Flags = FILESYSTEM_IS_READONLY;
#endif
    break;
  case VOLUME_STRUCTURE_LINKED_MEM:
    DBUG(("Creating linked-memory-disk device\n"));
    memcpy(deviceArgBlock, fsDevArgs, sizeof deviceArgBlock);
    deviceArgBlock[1].ta_Arg = (void *) fileDriver->drv.n_Item;
    lmDeviceItem = SuperCreateSizedItem(MKNODEID(KERNELNODE,DEVICENODE),
					deviceArgBlock,
					sizeof (LinkedMemDisk));
    DBUG(("Got item 0x%x\n", lmDeviceItem));
    lmDevice = (LinkedMemDisk *) LookupItem(lmDeviceItem);
    if (!lmDevice) {
      DBUG(("Could not allocate optimized-disk device\n"));
      MySysErr(lmDeviceItem);
      err = lmDeviceItem;
      goto nuke;
    }
    DBUG(("Initializing linked-memory-disk device\n"));
    hld = (HighLevelDisk *) lmDevice;
    lmDevice->lmd.hdi_DeviceType = FILE_DEVICE_LINKED_STORAGE;
    lmDevice->lmd.hdi_RawDeviceItem = theDevice->dev.n_Item;
    lmDevice->lmd.hdi_RawDeviceUnit = (uint8) unit;
    lmDevice->lmd_BlockSize  = devStatus.ds_DeviceBlockSize;
    lmDevice->lmd_BlockCount = devStatus.ds_DeviceBlockCount;
    lmDevice->lmd_RawDeviceBlockOffset = blockOffset;
    lmDevice->lmd_RawDeviceRequest = rawIOReq;
    lmDevice->lmd_CopyBlockSize = sizeof lmDevice->lmd_CopyBuffer / lmDevice->lmd_BlockSize;
    lmDevice->lmd_FileHeaderBlockSize =
      (sizeof (LinkedMemFileEntry) + lmDevice->lmd_BlockSize - 1) /
	lmDevice->lmd_BlockSize;
    lmDevice->lmd_FileHeaderByteSize = lmDevice->lmd_FileHeaderBlockSize * lmDevice->lmd_BlockSize;
    lmDevice->lmd_FSM = LMD_Idle;
    lmDevice->lmd.hdi_QueueRequest = (FileDriverQueueit) QueueLinkedMemDiskRequest;
    lmDevice->lmd.hdi_ScheduleIO = (FileDriverHook) ScheduleLinkedMemDiskIo;
    lmDevice->lmd.hdi_StartIO = (FileDriverHook) StartLinkedMemDiskIo;
    lmDevice->lmd.hdi_EndAction = (FileDriverEA) LinkedMemDiskEndAction;
    lmDevice->lmd.hdi_AbortIO = (FileDriverAbort) AbortLinkedMemDiskIo;
    lmDevice->lmd.hdi_CloseFile = (FileDriverClose) NULL;
    fileSystem->fs_Flags = 0;
#ifdef SEMAPHORE_LMD
    semItem = SuperCreateItem(MKNODEID(KERNELNODE,SEMA4NODE), NULL);
    if (semItem < 0) {
      err = semItem;
      DBUG(("Cannot create semaphore\n"));
      goto nuke;
    }
    sem = (Semaphore *) LookupItem(semItem);
    lmDevice->lmd_Semaphore = sem;
#endif
    break;
  }
  hld->hdi_DeviceBusy = 0;
  hld->hdi_RequestPriority = 0;
  hld->hdi_RunningPriority = 0;
  InitList(&hld->hdi_RequestsToDo, "To do");
  InitList(&hld->hdi_RequestsRunning, "Running");
  InitList(&hld->hdi_RequestsDeferred, "Deferred");
  fileSystem->fs_Device = hld;
/*
  Create a file descriptor which will frame the root directory.

  If we ever need the label itself again, we should access it through an
  ordinary file access path, via the DiscLabel file in the root
  directory (gotta make sure that the disc-builder creates a descriptor
  which frames the label properly!)

*/
  DBUG(("Creating root-descriptor file node\n"));
  fileSize = (int32) (sizeof(File) + sizeof (ulong) *
		    (discLabel->dl_RootDirectoryLastAvatarIndex + 1));
  rootFileItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
				      (void *) NULL, fileSize);
  fsRoot = (File *) LookupItem(rootFileItem);
  if (!fsRoot) {
    DBUG(("Could not create root file node\n"));
    goto nuke;
  }
  AddHead(&fileFolio->ff_Files, (Node *) fsRoot);
/*
  Set up the root file descriptor and fill in the actual locations of the
  root directory.
*/
  DBUG(("Initializing root directory\n"));
  fsRoot->fi_FileSystem = fileSystem;
  fsRoot->fi_ParentDirectory = fileFolio->ff_Root;
  fsRoot->fi_Type = FILE_TYPE_DIRECTORY;
  fileFolio->ff_Root->fi_UseCount ++;
  switch (hld->hdi_DeviceType) {
  case FILE_DEVICE_OPTIMIZED_DISK:
    fsRoot->fi_Flags = FILE_IS_DIRECTORY | FILE_IS_READONLY |
      FILE_IS_FOR_FILESYSTEM;
#ifdef DOCACHE
    if (fileSystem->fs_Flags & FILESYSTEM_CACHEWORTHY) {
      fsRoot->fi_Flags |= FILE_BLOCKS_CACHED;
    }
#endif
    break;
  case FILE_DEVICE_LINKED_STORAGE:
    fsRoot->fi_Flags = FILE_IS_DIRECTORY |
      FILE_IS_FOR_FILESYSTEM | FILE_SUPPORTS_DIRSCAN | FILE_SUPPORTS_ENTRY;
    break;
  }
  fsRoot->fi_UseCount = 1; /* lock it in */
  fsRoot->fi_Burst = 1;
  fsRoot->fi_Gap = 0;
  fsRoot->fi_BlockSize = discLabel->dl_RootDirectoryBlockSize;
  fsRoot->fi_FileSystemBlocksPerFileBlock =
    discLabel->dl_RootDirectoryBlockSize / discLabel->dl_VolumeBlockSize;
  fsRoot->fi_BlockCount = discLabel->dl_RootDirectoryBlockCount;
  fsRoot->fi_ByteCount = fsRoot->fi_BlockSize * fsRoot->fi_BlockCount;
  fsRoot->fi_UniqueIdentifier = discLabel->dl_VolumeUniqueIdentifier;
  fsRoot->fi_LastAvatarIndex = discLabel->dl_RootDirectoryLastAvatarIndex;
  strncpy((char *) fsRoot->fi_FileName,
	  (char *) discLabel->dl_VolumeIdentifier,
	  FILESYSTEM_MAX_NAME_LEN);
#ifdef	FS_DIRSEMA
  InitDirSema(fsRoot, 1);
#endif	/* FS_DIRSEMA */
  fileSystem->fs_VolumeBlockSize = discLabel->dl_VolumeBlockSize;
  fileSystem->fs_VolumeBlockCount = discLabel->dl_VolumeBlockCount;
  fileSystem->fs_VolumeUniqueIdentifier =
    discLabel->dl_VolumeUniqueIdentifier;
  fileSystem->fs_DeviceBlocksPerFilesystemBlock =
    (int32) (fileSystem->fs_VolumeBlockSize / deviceBlockSize);
  strncpy((char *) fileSystem->fs_FileSystemName,
	  (char *) discLabel->dl_VolumeIdentifier,
	  FILESYSTEM_MAX_NAME_LEN);
  strcpy(fileSystem->fs_MountPointName, "on.");
  strcat(fileSystem->fs_MountPointName, theDevice->dev.n_Name);
  CatDotted(fileSystem->fs_MountPointName, (uint32) unit);
  CatDotted(fileSystem->fs_MountPointName, (uint32) blockOffset);
  DBUG(("Volume is %d blocks of %d bytes\n",
	       fileSystem->fs_VolumeBlockCount,
	       fileSystem->fs_VolumeBlockSize));
  DBUG(("Root directory (%d blocks of %d bytes) at offset(s) ",
	       fsRoot->fi_BlockCount, fsRoot->fi_BlockSize));
  for (avatarIndex = (int32) fsRoot->fi_LastAvatarIndex;
       avatarIndex >= 0;
       avatarIndex --) {
    fsRoot->fi_AvatarList[avatarIndex] =
      discLabel->dl_RootDirectoryAvatarList[avatarIndex];
    DBUG((" %d", fsRoot->fi_AvatarList[avatarIndex]));
  }
  DBUG(("\n"));
  fileSystem->fs_RootDirectory = fsRoot;
  if (devStatus.ds_DeviceUsageFlags & DS_USAGE_READONLY) {
    fileSystem->fs_Flags |= FILESYSTEM_IS_READONLY;
    fsRoot->fi_Flags |= FILE_IS_READONLY;
  }
  AddTail(&fileFolio->ff_Filesystems, (Node *) fileSystem);
  qprintf(("Filesystem /%s is mounted\n", fileSystem->fs_FileSystemName));
  GiveDaemon(rawIOReq);
  GiveDaemon(fsRoot);
  GiveDaemon(fileSystem);
  GiveDaemon(fsDevice);
  GiveDaemon(lmDevice);
#ifdef SEMAPHORE_LMD
  GiveDaemon(sem);
#endif
  if (hld->hdi_DeviceType == FILE_DEVICE_OPTIMIZED_DISK) {
#ifndef NOTDEF
    catapult = OpenPath(fsRoot, "Catapult");
    if (catapult) {
      DBUG(("Hey, there's a catapult file!\n"));
      fsDevice->odi_CatapultFile = catapult;
      fsDevice->odi_CatapultPage = catapultPage =
	AddCacheEntry(catapult, 0, discLabel /* dummy ptr here... dcp */,
		      255, CACHE_CATAPULT_INDEX,
		      catapult->ofi_File->fi_BlockSize);
      if (catapult->ofi_File->fi_FileSystemBlocksPerFileBlock != 1) {
	DBUG(("Catapult file has weird blocking!  Can't use\n"));
	fsDevice->odi_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
      } else if (catapultPage) {
	fsCacheBusy ++;
	fsDevice->odi_CatapultPhase = CATAPULT_READING;
	/* majority of IOReq still set up from label read */
	rawIOReq->io_Info.ioi_Recv.iob_Buffer =
	  catapultPage->ioce_CachedBlock;
	rawIOReq->io_Info.ioi_Recv.iob_Len =
	  catapult->ofi_File->fi_BlockSize;
	rawIOReq->io_Info.ioi_Flags = 0;
	rawIOReq->io_Info.ioi_Offset = (int32) blockOffset +
	  catapult->ofi_File->fi_AvatarList[0] *
	    fileSystem->fs_DeviceBlocksPerFilesystemBlock;
	rawIOReq->io_Info.ioi_User = (uint32) fileSystem;
	rawIOReq->io_CallBack = CatapultEndAction;
	DBUG(("Starting read of catapult page 0\n"));
	fsDevice->odi.hdi_DeviceBusy = TRUE;
	fsDevice->odi_NextBlockAvailable = 
	  catapult->ofi_File->fi_AvatarList[0] + 1;
	err = SuperinternalSendIO(rawIOReq);
	if (err < 0) {
	  DBUG(("Catapult SendIO error 0x%X\n", err));
	  MySysErr(err);
	}
      } else {
	DBUG(("Could not get cache page for catapult\n"));
	fsDevice->odi_CatapultPhase = CATAPULT_MUST_SHUT_DOWN;
      }
    } else {
      DBUG(("No catapult file\n"));
    }
#endif
  }
  (void) FilesystemEvent(fileSystem, EVENTNUM_FilesystemMounted);
  FreeMem(discLabel, discLabelSize);
  return filesystemItem;
 nuke:
  DBUG2(("Releasing IOReq\n"));
  Nuke(rawIOReq);
  DBUG2(("Releasing root\n"));
  Nuke(fsRoot);
  DBUG2(("Releasing filesystem\n"));
  Nuke(fileSystem);
  DBUG2(("Releasing fsdevice\n"));
  Nuke(fsDevice);
  DBUG2(("Releasing lmDevice\n"));
  Nuke(lmDevice);
#ifdef SEMAPHORE_LMD
  DBUG2(("Releasing semaphore\n"));
  Nuke(sem);
#endif
  if (discLabel) {
    DBUG2(("Releasing label\n"));
    FreeMem(discLabel, discLabelSize);
  }
  if (optBuffer) {
    DBUG2(("Releasing opt buffer\n"));
    FreeMem(optBuffer, DEVICE_SORT_COUNT * sizeof(FileIOReq *));
  }
  DBUG2(("Returning error code 0x%x\n", err));
  return err;
}

Err SuperDismountFileSystem(char *name)
{
  FileSystem *fs;
  File *theFile, *nextFile;
  Task *owner;
  HighLevelDisk *hdi;
  OptimizedDisk *odi;
  MacDisk *mdi;
  LinkedMemDisk *lmd;
  int32 keepGoing, inUse;
  Err err;
  if (name[0] == '/') {
    name ++;
  }
  fs = (FileSystem *) FindNamedNode(&fileFolio->ff_Filesystems, name);
  if (!fs) {
    DBUG(("No such filesystem\n"));
    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFileSystem);
  }
#ifdef TRACEDISMOUNT
  DumpFileList();
#endif
  hdi = fs->fs_Device;
  switch (hdi->hdi_DeviceType) {
  case FILE_DEVICE_OPTIMIZED_DISK:
    odi = (OptimizedDisk *) hdi;
    DBUG0(("TotCatapultStreamedHits is %d\n", odi->odi_TotCatapultStreamedHits));
    DBUG0(("TotCatapultNonstreamedHits is %d\n", odi->odi_TotCatapultNonstreamedHits));
    DBUG0(("TotCatapultSeeksAvoided is %d\n", odi->odi_TotCatapultSeeksAvoided));
    DBUG0(("TotCatapultTimesEntered is %d\n", odi->odi_TotCatapultTimesEntered));
    DBUG0(("TotCatapultDeclined is %d\n", odi->odi_TotCatapultDeclined));
    DBUG0(("TotCatapultMisses is %d\n", odi->odi_TotCatapultMisses));
    DBUG0(("TotCatapultNonstreamedMisses is %d\n", odi->odi_TotCatapultNonstreamedMisses));
    if (odi->odi_CatapultFile) {
      DBUG(("Closing catapult\n"));
      (void) SuperCloseItem(odi->odi_CatapultFile->ofi.dev.n_Item);
      (void) SuperDeleteItem(odi->odi_CatapultFile->ofi.dev.n_Item);
      odi->odi_CatapultFile = NULL;
      odi->odi_CatapultPage = NULL;
      odi->odi_CatapultPhase = CATAPULT_NONE;
    }
    break;
  default:
    break;
  }
  keepGoing = TRUE;
  do {
    keepGoing = FALSE;
    inUse = 0;
    theFile = (File *) FirstNode(&fileFolio->ff_Files);
    while (IsNode(&fileFolio->ff_Files,theFile)) {
      nextFile = (File *) NextNode(theFile);
      if (theFile->fi_FileSystem == fs) {
	if (theFile->fi_UseCount == 0) {
#ifdef TRACEDISMOUNT
	  DBUG0(("Dismount: closing out file %s\n", theFile->fi_FileName));
#endif
	  owner = (Task *) LookupItem(theFile->fi.n_Owner);
	  if (!owner) {
	    DBUG(("Can't find File owner\n"));
	    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
	  }
#ifdef	FS_DIRSEMA
	  DelDirSema(theFile);
#endif	/* FS_DIRSEMA */
	  err = SuperInternalDeleteItem(theFile->fi.n_Item);
	  if (err < 0) {
	    DBUG(("DeleteItem failed\n"));
	    return err;
	  }
	  keepGoing = TRUE;
	} else {
	  DBUG(("Dismount: file %s has use count of %d\n",
		 theFile->fi_FileName, theFile->fi_UseCount));
	  inUse ++;
	}
      }
      theFile = nextFile;
    }
  } while (keepGoing);
#ifdef TRACEDISMOUNT
  DBUG0(("All possible files have been released\n"));
  DumpFileList();
#endif
  if (inUse > 1) {
    DBUG(("Filesystem still busy\n"));
    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
  }
  if (fs->fs_RootDirectory->fi_UseCount > 1) {
#ifdef TRACEDISMOUNT
    DBUG0(("Filesystem root directory has use count of %d\n",
	   fs->fs_RootDirectory->fi_UseCount));
#endif
    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
  }
  fs->fs_RootDirectory->fi_UseCount = 0;
#ifdef	FS_DIRSEMA
  DelDirSema(fs->fs_RootDirectory);
#endif	/* FS_DIRSEMA */
  DBUG(("Deleting filesystem root node\n"));
  err = SuperInternalDeleteItem(fs->fs_RootDirectory->fi.n_Item);
  if (err < 0) {
    DBUG(("Root deletion failed\n"));
    return err;
  }
/*
   At this point we've crossed the Rubicon... the root directory has
   been deleted.  We can no longer permit accesses to this filesystem.
   If one of the subsequent deletions fails, that's too bad... we'll
   have zombie items hanging around.
*/
  DBUG(("Unlinking filesystem\n"));
  RemNode((Node *) fs);
  switch (hdi->hdi_DeviceType) {
  case FILE_DEVICE_OPTIMIZED_DISK:
    odi = (OptimizedDisk *) hdi;
    err = SuperInternalDeleteItem(odi->odi_RawDeviceRequest->io.n_Item);
    if (err < 0) {
      DBUG(("Delete of internal IOReq failed\n"));
    }
    if (odi->odi_RequestSort) {
      FreeMem(odi->odi_RequestSort, DEVICE_SORT_COUNT * sizeof (FileIOReq *));
    }
    break;
  case FILE_DEVICE_MAC_DISK:
    mdi = (MacDisk *) hdi;
    err = SuperInternalDeleteItem(mdi->mdi_RawDeviceRequest->io.n_Item);
    if (err < 0) {
      DBUG(("Delete of internal IOReq failed\n"));
    }
    break;
  case FILE_DEVICE_LINKED_STORAGE:
    lmd = (LinkedMemDisk *) hdi;
    err = SuperInternalDeleteItem(lmd->lmd_RawDeviceRequest->io.n_Item);
    if (err < 0) {
      DBUG(("Delete of internal IOReq failed\n"));
    }
    break;
  }
  DBUG(("Deleting high-level-disk structure\n"));
  err = SuperInternalDeleteItem(hdi->hdi.dev.n_Item);
  if (err < 0) {
    DBUG(("High-level-disk deletion failed\n"));
  }
  DBUG(("Reporting event, just before we delete!\n"));
  (void) FilesystemEvent(fs, EVENTNUM_FilesystemDismounted);
  err = SuperInternalDeleteItem(fs->fs.n_Item);
  if (err < 0) {
    DBUG(("Filesystem deletion failed\n"));
    return err;
  }
  return 0;
}

/**********************
 SWI handler for MountFileSystem call
 **********************/

Item MountFileSystemSWI(Item deviceItem, int32 unit, uint32 blockOffset)
{
  Device *theDevice;
  Item fsItem;
  DBUG(("MountFileSystem SWI task Flag: 0x%x\n", CURRENTTASK->t.n_Flags));
  if (!(CURRENTTASK->t.n_Flags & TASK_SUPER)) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NotPrivileged);
  }
  (void) SetupFileFolioTaskData();
  theDevice = (Device *) CheckItem(deviceItem, KERNELNODE, DEVICENODE);
  if (!theDevice) {
    DBUG(("Item %d is not a device!\n", theDevice));
    return -1;
  }
  fsItem = InitFileSystem(theDevice, unit, blockOffset);
  DBUG2(("MountFileSystem SWI exiting, code 0x%x\n", fsItem));
  return fsItem;
}

/**********************
 SWI handler for DismountFileSystem call
 **********************/

Err DismountFileSystemSWI(char *name)
{
  DBUG(("Dismount SWI task Flag: 0x%x\n", CURRENTTASK->t.n_Flags));
  if (!(CURRENTTASK->t.n_Flags & TASK_SUPER)) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NotPrivileged);
  }
  (void) SetupFileFolioTaskData();
  return SuperDismountFileSystem(name);
}

#ifdef NOTDEF

/*
  Given a filesystem pointer, create a filesystem-folio task data
  structure, and initialize it so that the task's current working
  directory points to the base of the filesystem.
*/

FileFolioTaskData *InitFileTaskData(FileSystem *fs)
{
  FileFolioTaskData *fftd;
  fftd = (FileFolioTaskData *) AllocFileNode(sizeof (FileFolioTaskData),
					     FSPRIVDATANODE);
  if (fftd == (FileFolioTaskData *) NULL) {
    return fftd;
  }
  fftd->fftd_CurrentDirectory = fs->fs_RootDirectory;
  return fftd;
}

#endif

