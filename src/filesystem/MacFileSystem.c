/*****

$Id: MacFileSystem.c,v 1.27 1994/09/22 19:38:28 dplatt Exp $

$Log: MacFileSystem.c,v $
 * Revision 1.27  1994/09/22  19:38:28  dplatt
 * If a filesystem named /remote is already mounted, don't mount the
 * Mac remote folder.  Ugly sleaze to ease layout optimization testing.
 *
 * Revision 1.26  1994/05/13  00:42:22  cramer
 * fix compilation warnings.
 *
 * Revision 1.25  1994/04/22  19:43:06  dplatt
 * Construct mount point name.
 *
 * Revision 1.24  1994/04/05  18:55:33  shawn
 * Added directory semaphore.
 *
 * Revision 1.23  1994/03/24  01:48:43  dplatt
 * Add support for kernel events, hot-mount/hot-dismount.  Minor
 * security enhancements and rearrangements of various things.
 *
 * Revision 1.22  1994/02/09  21:32:46  shawn
 * do not write too mush into user address space.
 *
 * Revision 1.21  1994/02/07  18:17:25  shawn
 * Changes to support FSSTAT.
 *
 * Revision 1.20  1994/02/04  23:27:30  dplatt
 * Move postprocessing code to a subroutine so it can be used in a couple
 * of different endaction styles.
 *
 * Revision 1.19  1993/12/17  00:51:41  dplatt
 * Remove debugging msg
 *
 * Revision 1.18  1993/12/16  00:49:03  dplatt
 * Do I/Os according to priority.  Add null FILECMD_OPENENTRY hook.
 * Print debugging data if a remote read completes instantly (it shouldn't).
 *
 * Revision 1.17  1993/09/01  23:14:07  dplatt
 * Scavenge File items with zero use count... both on-the-fly (flush
 * LRU if more than 16 exist) and when a task exits (flush all).
 *
 * Revision 1.16  1993/07/11  21:17:26  dplatt
 * Interpret error messages which occur creating IOReq.
 *
 * Revision 1.15  1993/06/15  20:15:34  dplatt
 * Don't cache file info for Macintosh /remote files
 *
 * Revision 1.14  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.13  1993/05/08  01:08:14  dplatt
 * Add flat-file-system/NVRAM support, and recover from RCS bobble
 *
 * Revision 1.12  1993/04/26  20:12:55  dplatt
 * Quiet coldstart;  CD-ROM retry limit increased to 10
 *
 * Revision 1.11  1993/04/23  21:49:56  dplatt
 * CD-rom enhancements, cleanup
 *
 * Revision 1.10  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.9  1993/02/11  19:39:37  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.8  1993/02/10  00:27:26  dplatt
 * Change to new IOReq queueing mechanism and save big money!
 *
 * Revision 1.7  1993/02/09  01:47:20  dplatt
 * Reorganize and update for developer release
 *
 * Revision 1.6  1992/12/22  07:58:10  dplatt
 * Magneto 3 changes and CD-ROM support
 *
 * Revision 1.5  1992/12/08  05:59:52  dplatt
 * Magenta changes
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
 * Revision 1.1  1992/09/11  00:42:29  dplatt
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
  MacFileSystem.c - support code for Macintosh "remote" filesystems
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
#include "event.h"

#include "super.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"
/* #include <stdio.h> */

#undef DEBUG

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

#ifdef MACFS

#undef FOLLOWON

#define notAFileErr -1302

extern int32 CloseOpenFile(OpenFile *theOpenFile);
extern Node *AllocFileNode(int32 theSize, int32 theType);
extern void FreeFileNode (void *it);
extern Err FilesystemEvent(FileSystem *fs, uint32 eventCode);

static int32 QueueMacDiskRequest(MacDisk *theDevice, IOReq *theRequest);
static void ScheduleMacDiskIo(MacDisk *theDevice);
static void StartMacDiskIo (MacDisk *theDevice);
static void AbortMacDiskIo (IOReq *theRequest);
static IOReq *MacDiskEndAction(IOReq *theRequest);

static const TagArg filesystemArgs[] = {
  { FILESYSTEM_TAG_PRI,     (void *)1, },
  { FILESYSTEM_TAG_END,     0, },
};

static const TagArg fsDevArgs[] = {
  { TAG_ITEM_PRI,             (void *)1, },
  { CREATEDEVICE_TAG_DRVR,    NULL },
  { TAG_ITEM_NAME,           "filesystem" },
  { TAG_END,                  0, },
};

static Item macDeviceItem = 0;
const char macname[]="mac";
int32 stepny_fetchnik = 0;
#ifndef FOLLOWON
IOReq *postprocess = NULL;
#endif

#ifndef PRODUCTION
extern void MySysErr(Err err);
#endif

#ifdef PRODUCTION
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

static int32 ConstructMacPath(File *theTail, char *buf, int32 bufLen)
{
  int32 pathLen, entryLen, pathPoint;
  char *entryName;
  File *theDirectory, *theParent;
#ifdef NOTDEF
  pathLen = strlen(theTail->fi_FileName);
  theDirectory = theTail->fi_ParentDirectory;
  while (theDirectory != fileFolio->ff_Root) {
    theParent = theDirectory->fi_ParentDirectory;
    pathLen += strlen(theDirectory->fi_FileName) + 1;
    if (theParent == fileFolio->ff_Root  &&
	strchr(theDirectory->fi_FileName, ':') == (char *) NULL) {
      pathLen ++;
    }
    theDirectory = theParent;
  }
#else
  pathLen = 0;
  theDirectory = theTail;
  memset(buf, 0, bufLen);
  do {
    theParent = theDirectory->fi_ParentDirectory;
    pathLen += strlen(theDirectory->fi_FileName) + 1;
    theDirectory = theParent;
  } while (theDirectory != fileFolio->ff_Root);
#endif
  if (pathLen >= bufLen) {
    return -1; /* too long */
  }
  pathPoint = pathLen;
  buf[pathPoint] = '\0';
#ifdef NOTDEF
  entryLen = strlen(theTail->fi_FileName);
  pathPoint -= entryLen;
  strncpy(&buf[pathPoint], theTail->fi_FileName, entryLen);
  theDirectory = theTail->fi_ParentDirectory;
  while (theDirectory != fileFolio->ff_Root) {
    buf[--pathPoint] = ':';
    entryName = theDirectory->fi_FileName;
    entryLen = strlen(entryName);
    pathPoint -= entryLen;
    strncpy(&buf[pathPoint], entryName, entryLen);
    theDirectory = theDirectory->fi_ParentDirectory;
  }
  if (pathPoint == 1) {
    buf[--pathPoint] = ':';
  }
#else
  theDirectory = theTail;
  do {
    entryName = theDirectory->fi_FileName;
    entryLen = strlen(entryName);
    pathPoint -= entryLen;
    strncpy(&buf[pathPoint], entryName, entryLen);
    buf[--pathPoint] = ':';
    theDirectory = theDirectory->fi_ParentDirectory;
  } while (theDirectory != fileFolio->ff_Root);
#endif
#ifdef DEBUG
  qprintf(("Constructed path %s\n", buf));
#endif
  if (pathPoint != 0) {
    qprintf(("PANIC! Pathname consing off by %d\n", pathPoint));
  }
  return pathLen;
}

static void Nuke(void *foo) {
  if (foo) {
    SuperDeleteItem(((ItemNode *)foo)->n_Item);
  }
}

static FileSystem *InitMacFileSystem(char *thePath)
{
  FileSystem *fileSystem = NULL;
  MacDisk *fsDevice = NULL;
  Item fsDeviceItem, filesystemItem, rootFileItem;
  Item ioReqItem;
  File *fsRoot = NULL;
  IOReq *rawIOReq = NULL;
  MacFileInfo macFileInfo;
  int32 fileSize;
  TagArg fsDeviceArgBlock[4];
  TagArg ioReqTags[2];
  fileSystem = (FileSystem *) FIRSTNODE((&fileFolio->ff_Filesystems));
  while (fileSystem != (FileSystem *) NULL) {
    if (strcasecmp(fileSystem->fs_FileSystemName, "remote") == 0) {
      qprintf(("/remote already mounted, Mac /remote folder will not be.\n"));
      return NULL;
    }
    fileSystem = (FileSystem *) NEXTNODE((&(fileSystem->fs)));
  }
  /*   Get the Mac device */   
  if (!macDeviceItem) {
    macDeviceItem = SuperOpenItem(SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
						     (char *) macname),
				  (void *) NULL);
    if (macDeviceItem < 0) {
      qprintf(("Error %d opening: %s\n", macDeviceItem, macname));
      MySysErr(macDeviceItem);
      goto nuke;
    }
  }
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) macDeviceItem;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0)     {
    qprintf(("Error 0x%x creating ioreq\n", ioReqItem));
    MySysErr(ioReqItem);
    goto nuke;
  }
  rawIOReq = (struct IOReq *) LookupItem(ioReqItem);
/*
  Query the raw device to determine its block size and block count,
  and record these in the Mac-device structure.
*/
#ifdef DEBUG
  qprintf(("Getting info for %s\n", thePath));
#endif
  rawIOReq->io_Info.ioi_Command = MACCMD_FILEINFO;
  rawIOReq->io_Info.ioi_Flags = 0;
  rawIOReq->io_Info.ioi_Unit = 0;
  rawIOReq->io_Info.ioi_Offset = 0;
  rawIOReq->io_Info.ioi_Send.iob_Buffer = thePath;
  rawIOReq->io_Info.ioi_Send.iob_Len = strlen(thePath) + 1L;
  rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) &macFileInfo;
  rawIOReq->io_Info.ioi_Recv.iob_Len = sizeof macFileInfo;
  SuperinternalSendIO(rawIOReq);
  SuperWaitIO(rawIOReq->io.n_Item);
  if (rawIOReq->io_Error < 0 && rawIOReq->io_Error != notAFileErr) {
    qprintf(("I/O error %d getting file/folder info\n",
 		 rawIOReq->io_Error));
    goto nuke;
  }
  if (!(macFileInfo.mfi_Info & MAC_PATH_IS_DIRECTORY)) {
    qprintf(("%s is a file, not a directory!\n", thePath));
  }
#ifdef DEBUG
  qprintf(("Directory confirmed, %d entries\n",
	       macFileInfo.mfi_NumEntries));
#endif
/*
  Create the filesystem device which resides on this Mac device.
*/
#ifdef DEBUG
  qprintf(("Creating Mac-disk device\n"));
#endif
  memcpy(fsDeviceArgBlock, fsDevArgs, sizeof fsDeviceArgBlock);
  fsDeviceArgBlock[1].ta_Arg = (void *) fileDriver->drv.n_Item;
  fsDeviceItem = SuperCreateSizedItem(MKNODEID(KERNELNODE,DEVICENODE),
 				      fsDeviceArgBlock,
 				      sizeof (MacDisk));
#ifdef DEBUG
  qprintf(("Got item %d\n", fsDeviceItem));
#endif
  fsDevice = (MacDisk *) LookupItem(fsDeviceItem);
  if (!fsDevice) {
    qprintf(("Could not allocate Mac-disk device\n"));
    goto nuke;
  }
 #ifdef DEBUG
 qprintf(("Initializing Mac-disk device\n"));
#endif
/*** OBSOLETE?  fsDevice->mdi.hdi.dev_Parent = macDeviceItem; ***/
  fsDevice->mdi.hdi_DeviceType = FILE_DEVICE_MAC_DISK;
  fsDevice->mdi.hdi_RawDeviceItem = macDeviceItem;
  fsDevice->mdi.hdi_RawDeviceUnit = 0;
  fsDevice->mdi.hdi_DeviceBusy = 0;
  fsDevice->mdi.hdi_RequestPriority = 0;
  fsDevice->mdi.hdi_RunningPriority = 0;
  fsDevice->mdi_RawDeviceRequest = rawIOReq;
  fsDevice->mdi.hdi_QueueRequest = (FileDriverQueueit) QueueMacDiskRequest;
  fsDevice->mdi.hdi_ScheduleIO = (FileDriverHook) ScheduleMacDiskIo;
  fsDevice->mdi.hdi_StartIO = (FileDriverHook) StartMacDiskIo;
  fsDevice->mdi.hdi_AbortIO = (FileDriverAbort) AbortMacDiskIo;
  fsDevice->mdi.hdi_EndAction = (FileDriverEA) MacDiskEndAction;
  fsDevice->mdi.hdi_CloseFile = (FileDriverClose) NULL;
#ifdef DEBUG
  qprintf(("Initializing request lists\n"));
#endif
  InitList(&fsDevice->mdi.hdi_RequestsToDo, "To do");
  InitList(&fsDevice->mdi.hdi_RequestsRunning, "Running");
  InitList(&fsDevice->mdi.hdi_RequestsDeferred, "Deferred");
/*
   Create a filesystem which resides on the device 
*/
#ifdef DEBUG
  qprintf(("Creating filesystem node\n"));
#endif
  filesystemItem = SuperCreateItem(MKNODEID(FILEFOLIO,FILESYSTEMNODE),
 				   (TagArg *) filesystemArgs);
  fileSystem = (FileSystem *) LookupItem(filesystemItem);
  if (!fileSystem) {
    qprintf(("Could not create filesystem\n"));
    goto nuke;
  }
  fileSystem->fs_Device = (HighLevelDisk *) fsDevice;
  fileSystem->fs_Flags = (FILESYSTEM_IS_READONLY |
 			  FILESYSTEM_IS_IMPORTED);
#ifdef DEBUG
  qprintf(("Creating root-descriptor file node\n"));
#endif
  fileSize = (int32) (sizeof(File));
  rootFileItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
 				      (void *) NULL, fileSize);
  fsRoot = (File *) LookupItem(rootFileItem);
  if (!fsRoot) {
    qprintf(("Could not create root file node\n"));
    goto nuke;
  }
  AddHead(&fileFolio->ff_Files, (Node *) fsRoot);
  rawIOReq->io_Info.ioi_Send.iob_Buffer = fsDevice->mdi_CurrentPathName;
  rawIOReq->io_Info.ioi_Send.iob_Len = MAC_MAX_NAME_LEN;
/*
  Set up the root file descriptor and fill in the actual locations of the
  root directory. 
*/
#ifdef DEBUG
  qprintf(("Initializing root directory\n"));
#endif
  fsRoot->fi_FileSystem = fileSystem;
  fsRoot->fi_Type = FILE_TYPE_DIRECTORY;
  fsRoot->fi_ParentDirectory = fileFolio->ff_Root;
  fileFolio->ff_Root->fi_UseCount ++;
  fsRoot->fi_Flags = (FILE_IS_DIRECTORY |
		      FILE_IS_READONLY |
		      FILE_IS_FOR_FILESYSTEM |
		      FILE_SUPPORTS_DIRSCAN |
		      FILE_SUPPORTS_ENTRY);
  fsRoot->fi_UseCount = 1; /* lock it in */
  fsRoot->fi_Burst = 1;
  fsRoot->fi_Gap = 0;
  fsRoot->fi_BlockSize = FILESYSTEM_DEFAULT_BLOCKSIZE;
  fsRoot->fi_BlockCount = 0;
  fsRoot->fi_ByteCount = 0;
  fsRoot->fi_UniqueIdentifier = fileFolio->ff_NextUniqueID --;
  fsRoot->fi_LastAvatarIndex = -1;
  strncpy((char *) fsRoot->fi_FileName, thePath, FILESYSTEM_MAX_NAME_LEN);
#ifdef	FS_DIRSEMA
      InitDirSema(fsRoot, 1);
#endif	/* FS_DIRSEMA */
  fileSystem->fs_VolumeBlockSize = FILESYSTEM_DEFAULT_BLOCKSIZE;
  fileSystem->fs_VolumeBlockCount = -1;
  fileSystem->fs_VolumeUniqueIdentifier = fileFolio->ff_NextUniqueID --;
  fileSystem->fs_DeviceBlocksPerFilesystemBlock = 1;
  strncpy((char *) fileSystem->fs_FileSystemName, thePath,
	  FILESYSTEM_MAX_NAME_LEN);
  strcpy(fileSystem->fs_MountPointName, "on.");
  strcat(fileSystem->fs_MountPointName,
	 ((Device *) LookupItem(macDeviceItem))->dev.n_Name);
  strcat(fileSystem->fs_MountPointName, ".0.0");
  fileSystem->fs_RootDirectory = fsRoot;
  AddTail(&fileFolio->ff_Filesystems, (Node *) fileSystem);
  qprintf(("Filesystem /%s is mounted\n", fileSystem->fs_FileSystemName));
#ifndef FOLLOWON
  DBUG(("Post-processing pointer is at 0x%X\n", &postprocess));
  DBUG(("Daemon task is at 0x%X\n", fileFolio->ff_Daemon.ffd_Task));
#endif
  (void) FilesystemEvent(fileSystem, EVENTNUM_FilesystemMounted);
  return fileSystem;
 nuke:
  Nuke(rawIOReq);
  Nuke(fsRoot);
  Nuke(fileSystem);
  Nuke(fsDevice);
  return (FileSystem *) NULL;
}

/**********************
 SWI handler for MountMacFileSystem call
 **********************/

Item MountMacFileSystemSWI(char *thePath)
{
  FileSystem *theFS;
#ifdef DEBUG
  qprintf(("In the MountMacFileSystem SWI\n"));
 #endif
 theFS = InitMacFileSystem(thePath);
  if (theFS) {
    return theFS->fs.n_Item;
  } else {
    return -1;
  }
}

static void MapMacRequest(IOReq *rawRequest, FileIOReq *userRequest,
			  MacDisk *theDevice)
{
  int32 fileLen, endOffset, entryLen;
  switch (userRequest->fio.io_Info.ioi_Command) {
  case FILECMD_READDIR:
    DBUG(("Readdir for %s offset %d\n", theDevice->mdi_CurrentPathName,
		 rawRequest->io_Info.ioi_Offset));
    rawRequest->io_Info.ioi_Command = MACCMD_READDIR;
    rawRequest->io_Info.ioi_Recv.iob_Buffer =
      &theDevice->mdi_DirectoryEntry;
    rawRequest->io_Info.ioi_Recv.iob_Len =
      sizeof theDevice->mdi_DirectoryEntry;
    memset(&theDevice->mdi_DirectoryEntry, 0, sizeof theDevice->mdi_DirectoryEntry);
    strncpy(((DirectoryEntry *) userRequest->fio.io_Info.ioi_Recv.iob_Buffer)
	    ->de_FileName, "Thud!",
	    FILESYSTEM_MAX_NAME_LEN);
    theDevice->mdi_DirectoryEntry.mde_Info.mfi_Length = 0xFFFFFFFF;
    break;
  case FILECMD_READENTRY:
    rawRequest->io_Info.ioi_Command = MACCMD_FILEINFO;
    rawRequest->io_Info.ioi_Recv.iob_Buffer =
      &theDevice->mdi_DirectoryEntry.mde_Info;
    rawRequest->io_Info.ioi_Recv.iob_Len =
      sizeof theDevice->mdi_DirectoryEntry.mde_Info;
    memset(&theDevice->mdi_DirectoryEntry, 0, sizeof theDevice->mdi_DirectoryEntry);
    entryLen = strlen((char *) userRequest->fio.io_Info.ioi_Send.iob_Buffer);
    if (entryLen > userRequest->fio.io_Info.ioi_Send.iob_Len) {
      entryLen = userRequest->fio.io_Info.ioi_Send.iob_Len;
    }
    if (strlen(theDevice->mdi_CurrentPathName) + entryLen + 1 <
	MAC_MAX_NAME_LEN) {
      strcat(theDevice->mdi_CurrentPathName, ":");
      endOffset = strlen(theDevice->mdi_CurrentPathName) + entryLen;
      theDevice->mdi_CurrentPathName[endOffset] = '\0';
      strncat(theDevice->mdi_CurrentPathName,
	      (char *) userRequest->fio.io_Info.ioi_Send.iob_Buffer,
	      entryLen);
    }
    if (userRequest->fio.io_Info.ioi_Recv.iob_Len >= sizeof (DirectoryEntry) - 4) {
      strncpy(((DirectoryEntry *) userRequest->fio.io_Info.ioi_Recv.iob_Buffer)
	      ->de_FileName,
	      (char *) userRequest->fio.io_Info.ioi_Send.iob_Buffer,
	      FILESYSTEM_MAX_NAME_LEN);
    }
    theDevice->mdi_DirectoryEntry.mde_Info.mfi_Length = 0xFFFFFFFF;
    DBUG(("ReadEntry %s\n", theDevice->mdi_CurrentPathName));
    break;
  case CMD_READ:
    DBUG(("Read for %s offset %d count %d\n",
	   theDevice->mdi_CurrentPathName,
	   rawRequest->io_Info.ioi_Offset,
	   rawRequest->io_Info.ioi_Recv.iob_Len));
    rawRequest->io_Info.ioi_Offset *= FILESYSTEM_DEFAULT_BLOCKSIZE;
    fileLen = (int32) ((OpenFile *) userRequest->fio.io_Dev)->ofi_File->
      fi_ByteCount;
    endOffset = rawRequest->io_Info.ioi_Offset +
      rawRequest->io_Info.ioi_Recv.iob_Len;
    if (endOffset > fileLen) {
      rawRequest->io_Info.ioi_Recv.iob_Len =
	(int32) (fileLen - rawRequest->io_Info.ioi_Offset);
    }
    break;
  default:
#ifdef DEBUG
    qprintf(("Unsupported operation %d for %s\n",
		 userRequest->fio.io_Info.ioi_Command,
		 theDevice->mdi_CurrentPathName));
#endif
    break;
  }
}
  

/*
  The Mac I/O scheduler is pretty dumb... it just uses a simple
  priority scheme.
*/

static int32 QueueMacDiskRequest(MacDisk *theDevice, IOReq *theRequest)
{
  uint32 interrupts;
  File *theFile;
#ifdef DEBUG
  qprintf(("Mac I/O scheduler\n"));
#endif
  switch (theRequest->io_Info.ioi_Command) {
  case CMD_READ:
    theFile = ((OpenFile *) theRequest->io_Dev)->ofi_File;
    if (theRequest->io_Info.ioi_Recv.iob_Len % theFile->fi_BlockSize != 0) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);
    }
    break;
  case FILECMD_OPENENTRY:
    SuperCompleteIO(theRequest);
    return 0;
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
    /*
     *	no fs info for this filesystem
     */
    memset(theRequest->io_Info.ioi_Recv.iob_Buffer,
	   0, sizeof(FileSystemStat));
    SuperCompleteIO(theRequest);
    return 0;
  default:
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  interrupts = Disable();
  InsertNodeFromTail(&theDevice->mdi.hdi_RequestsToDo, (Node *) theRequest);
  theRequest->io_Flags &= ~IO_QUICK;
  Enable(interrupts);
  return 0;
}

static void PostprocessMacRequest(IOReq *rawRequest)
{
  FileIOReq *userRequest;
  MacDisk *theDevice;
  OpenFile *theOpenFile;
  DirectoryEntry de;
  int32 deSize;
  userRequest = (FileIOReq *) rawRequest->io_Info.ioi_User;
  theOpenFile = (OpenFile *) userRequest->fio.io_Dev;
  theDevice = (MacDisk *) theOpenFile->ofi_File->fi_FileSystem->
    fs_Device;
  DBUG(("Mac I/O postprocessor\n"));
/*
  Last-block fakeout
*/
  if (userRequest->fio.io_Info.ioi_Command == CMD_READ &&
      rawRequest->io_Info.ioi_Recv.iob_Len !=
      userRequest->fio.io_Info.ioi_Recv.iob_Len) {
    userRequest->fio.io_Actual += userRequest->fio.io_Info.ioi_Recv.iob_Len;
  } else {
    userRequest->fio.io_Actual += rawRequest->io_Actual;
  }
/*
  Pass back the completion information and dequeue the request from the
  RequestsRunning list.
*/
  userRequest->fio.io_Error = rawRequest->io_Error;
  if (/* userRequest->fio.io_Info.ioi_Command == FILECMD_READDIR && */
      userRequest->fio.io_Error == -43) {
    userRequest->fio.io_Error = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
    DBUG(("Got no-such-file on cmd %d on file '%s'\n",
	   userRequest->fio.io_Info.ioi_Command,
	   rawRequest->io_Info.ioi_Send.iob_Buffer));
  }
  RemNode((Node *) userRequest);
/*
  Postprocess info requests
*/
  switch (userRequest->fio.io_Info.ioi_Command) {
  case FILECMD_READDIR:
    strncpy(de.de_FileName, theDevice->mdi_DirectoryEntry.mde_Name,
	    FILESYSTEM_MAX_NAME_LEN);
    de.de_UniqueIdentifier = 0;
    de.de_BlockSize = FILESYSTEM_DEFAULT_BLOCKSIZE;
    de.de_Burst = -1;
    de.de_Gap = -1;
    de.de_AvatarCount = -1;
    if (theDevice->mdi_DirectoryEntry.mde_Info.mfi_Info &
	MAC_PATH_IS_DIRECTORY) {
      de.de_Type = FILE_TYPE_DIRECTORY;
      de.de_ByteCount = 0;
      de.de_BlockCount = 0;
      de.de_Flags = (FILE_IS_DIRECTORY |
		     FILE_IS_READONLY |
		     FILE_IS_FOR_FILESYSTEM |
		     FILE_SUPPORTS_DIRSCAN |
		     FILE_SUPPORTS_ENTRY);
    } else {
      de.de_Type = 0x20202020;
      de.de_ByteCount = theDevice->mdi_DirectoryEntry.mde_Info.mfi_Length;
      de.de_BlockCount = (de.de_ByteCount + FILESYSTEM_DEFAULT_BLOCKSIZE - 1) /
      FILESYSTEM_DEFAULT_BLOCKSIZE;
      de.de_Flags = FILE_IS_READONLY | FILE_INFO_NOT_CACHED;
    }
    de.de_Location = 0;
    deSize = userRequest->fio.io_Info.ioi_Recv.iob_Len;
    if (deSize > sizeof (DirectoryEntry)) {
      deSize = sizeof (DirectoryEntry);
    }
    memcpy(userRequest->fio.io_Info.ioi_Recv.iob_Buffer, &de, deSize);
    userRequest->fio.io_Actual = deSize;
    if (userRequest->fio.io_Error == notAFileErr) {
      userRequest->fio.io_Error = 0; /* suppress debugger bogosity */
    }
    break;
  case FILECMD_READENTRY:
    DBUG2(("Postprocessing read-entry for '%s'\n", rawRequest->io_Info.ioi_Send.iob_Buffer));
    de.de_UniqueIdentifier = 0;
    de.de_BlockSize = FILESYSTEM_DEFAULT_BLOCKSIZE;
    de.de_Burst = -1;
    de.de_Gap = -1;
    de.de_AvatarCount = -1;
    if (theDevice->mdi_DirectoryEntry.mde_Info.mfi_Info &
	MAC_PATH_IS_DIRECTORY) {
      DBUG2(("Got directory len %d\n",
	     theDevice->mdi_DirectoryEntry.mde_Info.mfi_Length));
      de.de_Type = FILE_TYPE_DIRECTORY;
      de.de_ByteCount = 0;
      de.de_BlockCount = 0;
      de.de_Flags = (FILE_IS_DIRECTORY |
		     FILE_IS_READONLY |
		     FILE_IS_FOR_FILESYSTEM |
		     FILE_SUPPORTS_DIRSCAN |
		     FILE_SUPPORTS_ENTRY);
    } else {
      DBUG2(("Got file len %d\n",
	     theDevice->mdi_DirectoryEntry.mde_Info.mfi_Length));
      de.de_Type = 0x20202020;
      de.de_ByteCount = theDevice->mdi_DirectoryEntry.mde_Info.mfi_Length;
      de.de_BlockCount = (de.de_ByteCount + FILESYSTEM_DEFAULT_BLOCKSIZE - 1) /
      FILESYSTEM_DEFAULT_BLOCKSIZE;
      de.de_Flags = FILE_IS_READONLY | FILE_INFO_NOT_CACHED;
    }
    de.de_Location = 0;
    deSize = userRequest->fio.io_Info.ioi_Recv.iob_Len;
    if (deSize > sizeof (DirectoryEntry)) {
      deSize = sizeof (DirectoryEntry);
    }
    memcpy(userRequest->fio.io_Info.ioi_Recv.iob_Buffer, &de, deSize);
    userRequest->fio.io_Actual = deSize;
    if (userRequest->fio.io_Error != 0) DBUG2(("Error was 0x%X\n", userRequest->fio.io_Error));
    if (userRequest->fio.io_Error == notAFileErr) {
      DBUG2(("That's a not-a-file error, setting to zero\n"));
      userRequest->fio.io_Error = 0;
    }
    break;
  case CMD_READ:
    DBUG2(("Postprocessing read for '%s' offset %d\n",
	   rawRequest->io_Info.ioi_Send.iob_Buffer,
	   rawRequest->io_Info.ioi_Offset));
    DBUG2(("Asked for %d, got %d, gave user %d\n",
	   rawRequest->io_Info.ioi_Recv.iob_Len,
	   rawRequest->io_Actual, userRequest->fio.io_Actual));
    if (rawRequest->io_Error < 0) {
      if (rawRequest->io_Error > -512) {
	DBUG2(("Mac error was %d\n", rawRequest->io_Error));
      } else {
	DBUG2(("OS error was 0x%X\n", rawRequest->io_Error));
      }
    }
    break;
  default:
    break;
  }
/*
  That request is done (successfully or with errors from which we cannot
  recover).
*/
  SuperCompleteIO((IOReq *) userRequest);
  theDevice->mdi.hdi_DeviceBusy = FALSE;
}

static void ScheduleMacDiskIo(MacDisk *theDevice)
{
  Node *theRequest;
  DBUG(("Mac I/O scheduler\n"));
#ifndef FOLLOWON
  if (postprocess) {
    DBUG(("Call postprocessor\n"));
    PostprocessMacRequest(postprocess);
    postprocess = NULL;
  }
#endif
  if (theDevice->mdi.hdi_DeviceBusy) {
    DBUG(("Busy\n"));
    return;
  }
  while (!ISEMPTYLIST(&theDevice->mdi.hdi_RequestsToDo)) {
    theRequest = FIRSTNODE(&theDevice->mdi.hdi_RequestsToDo);
    RemNode(theRequest);
    AddTail(&theDevice->mdi.hdi_RequestsRunning, theRequest);
  }
}

static void StartMacDiskIo (MacDisk *theDevice)
{
  FileIOReq *theRequest;
  IOReq *rawRequest;
  File *theFile;
  int32 err;
#ifdef DEBUG
  qprintf(("Mac I/O initiator\n"));
#endif
  if (theDevice->mdi.hdi_DeviceBusy) {
    return;
  }
  theDevice->mdi.hdi_DeviceBusy = TRUE;
  if (ISEMPTYLIST(&theDevice->mdi.hdi_RequestsRunning)) {
    DBUG(("No I/O to start, bailing out\n"));
    theDevice->mdi.hdi_DeviceBusy = FALSE;
    return;
  }
  if (stepny_fetchnik) {
#ifdef DEBUG
    qprintf(("Stepny-fetchnik: got %s\n",
		 theDevice->mdi_DirectoryEntry.mde_Name));
#endif
    stepny_fetchnik = 0;
  }
  theRequest = (FileIOReq *) FIRSTNODE(&theDevice->mdi.hdi_RequestsRunning);
  theFile = ((OpenFile *) theRequest->fio.io_Dev)->ofi_File;
  (void) ConstructMacPath(theFile, theDevice->mdi_CurrentPathName,
			  MAC_MAX_NAME_LEN);
  rawRequest = theDevice->mdi_RawDeviceRequest;
  rawRequest->io_CallBack = theDevice->mdi.hdi_EndAction;
  rawRequest->io_Error = 0;
  rawRequest->io_Flags = 0;
  rawRequest->io_Info = theRequest->fio.io_Info;
  rawRequest->io_Info.ioi_User = (ulong) theRequest;
  rawRequest->io_Info.ioi_Flags = 0;
  rawRequest->io_Info.ioi_Send.iob_Buffer = theDevice->mdi_CurrentPathName;
  rawRequest->io_Info.ioi_Send.iob_Len = MAC_MAX_NAME_LEN;
#ifdef DEBUG
  qprintf(("Mapping Mac I/O request 0x%x to 0x%x\n", theRequest, rawRequest));
#endif
  MapMacRequest(rawRequest, theRequest, theDevice);
#ifdef DEBUG
  qprintf(("Raw I/O request for offset %d, buf %lx, bytes %d,",
	       rawRequest->io_Info.ioi_Offset,		 
	       rawRequest->io_Info.ioi_Recv.iob_Buffer,
	       rawRequest->io_Info.ioi_Recv.iob_Len));		 
  qprintf((" cmd %d, endaction %lx\n",
	       rawRequest->io_Info.ioi_Command,
	       rawRequest->io_CallBack));		 
#endif
  err = SuperinternalSendIO(rawRequest);
  if (err < 0) {
    qprintf(("Error %d from SuperinternalSendIO on /remote!\n", err));
#ifndef PRODUCTION
    MySysErr(rawRequest->io_Error);
#endif
  } if (err == 1) {
    qprintf(("Suspiciously fast /remote I/O completion\n"));
    qprintf(("Raw I/O request for offset %d, buf %lx, bytes %d,",
	     rawRequest->io_Info.ioi_Offset,		 
	     rawRequest->io_Info.ioi_Recv.iob_Buffer,
	     rawRequest->io_Info.ioi_Recv.iob_Len));		 
    qprintf((" cmd %d, endaction %lx\n",
	     rawRequest->io_Info.ioi_Command,
	     rawRequest->io_CallBack));
    qprintf((" Pathname '%s'\n", rawRequest->io_Info.ioi_Send.iob_Buffer));
    qprintf((" Error is "));
    MySysErr(rawRequest->io_Error);
  } else {
    DBUG(("Sent\n"));
  }
  return;
}

static void AbortMacDiskIo(IOReq *theRequest)
{
  uint32 interrupts;
  MacDisk *theDevice;
  OpenFile *theOpenFile;
  theOpenFile = (OpenFile *) theRequest->io_Dev;
  theDevice = (MacDisk *)theOpenFile->ofi_File->fi_FileSystem->fs_Device;
  interrupts = Disable(); /*** DISABLE ***/
/*
  If this is the request that the device is currently servicing, then
  simply abort the lower-level I/O request - the endaction code will
  perform the cleanup for both levels.  If this request is not yet being
  serviced, dequeue and kill it immediately.
*/
  if (theDevice->mdi.hdi_DeviceBusy &&
   theDevice->mdi_RawDeviceRequest->io_Info.ioi_User == (ulong) theRequest) {
    SuperinternalAbortIO(theDevice->mdi_RawDeviceRequest);
  } else {
    theRequest->io_Error = MakeFErr(ER_SEVER,ER_C_STND,ER_Aborted);
    RemNode((Node *) theRequest);
    SuperCompleteIO(theRequest);
  }
  Enable(interrupts);
  return;
}

static IOReq *MacDiskEndAction(IOReq *rawRequest)
{
#ifdef FOLLOWON
  FileIOReq *userRequest;
  MacDisk *theDevice;
  OpenFile *theOpenFile;
  File *theFile;
  userRequest = (FileIOReq *) rawRequest->io_Info.ioi_User;
  theOpenFile = (OpenFile *) userRequest->fio.io_Dev;
  theDevice = (MacDisk *) theOpenFile->ofi_File->fi_FileSystem->
    fs_Device;
  PostprocessMacRequest(rawRequest);
  if (ISEMPTYLIST(&theDevice->mdi.hdi_RequestsRunning)) {
/*
  We've completed the last scheduled I/O, reschedule to find something to do.
  
  If the filesystem daemon is alive, signal it to reschedule...
  otherwise, do the scheduling here at interrupt level.
*/
    if (fileFolio->ff_Daemon.ffd_Task != (Task *) NULL) {
      SuperinternalSignal(fileFolio->ff_Daemon.ffd_Task,
			  fileFolio->ff_Daemon.ffd_RescheduleSignal);
      return (IOReq *) NULL;
    }
    (*theDevice->mdi.hdi_ScheduleIO)((HighLevelDisk *) theDevice);
    if (ISEMPTYLIST(&theDevice->mdi.hdi_RequestsRunning)) {
      return (IOReq *) NULL;
    }
  }
/***
  OK, we have something more to do... do it.
 ***/
  userRequest = (FileIOReq *) FIRSTNODE(&theDevice->mdi.hdi_RequestsRunning);
  theFile = ((OpenFile *) userRequest->fio.io_Dev)->ofi_File;
  (void) ConstructMacPath(theFile, theDevice->mdi_CurrentPathName,
			  sizeof theDevice->mdi_CurrentPathName);
  rawRequest = theDevice->mdi_RawDeviceRequest;
  rawRequest->io_CallBack = theDevice->mdi.hdi_EndAction;
  rawRequest->io_Error = 0;
  rawRequest->io_Flags = 0;
  rawRequest->io_Info = userRequest->fio.io_Info;
  rawRequest->io_Info.ioi_User = (ulong) userRequest;
  rawRequest->io_Info.ioi_Send.iob_Buffer = theDevice->mdi_CurrentPathName;
  rawRequest->io_Info.ioi_Send.iob_Len = MAC_MAX_NAME_LEN;
  MapMacRequest(rawRequest, userRequest, theDevice);
  theDevice->mdi.hdi_DeviceBusy = TRUE;
  return rawRequest;
#else
  postprocess = rawRequest;
  SuperinternalSignal(fileFolio->ff_Daemon.ffd_Task,
		      fileFolio->ff_Daemon.ffd_RescheduleSignal);
  return (IOReq *) NULL;
#endif
}

#else

/**********************
 SWI handler for MountMacFileSystem call
 **********************/

Item MountMacFileSystemSWI(char *thePath)
{
  return -1;
}

#endif
