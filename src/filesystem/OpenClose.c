/*****

$Id: OpenClose.c,v 1.64 1994/12/12 23:39:54 shawn Exp $

$Log: OpenClose.c,v $
 * Revision 1.64  1994/12/12  23:39:54  shawn
 * This should clear several of babs hangs/reboot bugs. If the first
 * alternate path fails, we should unlock its parent directory at the
 * point of failure before the new alternate is copied to whereNow, there
 * by losing the old pointer.
 *
 * Revision 1.63  1994/10/19  20:45:35  shawn
 * we now cleanup IOReq's on close time and not delete item
 * time via dev_DeleteDev. The new CleanupOnClose removes
 * the internal device IOReq before the item is closed. NOTE
 * IOReq items created by user must still be deleted by user
 * task.
 *
 * Revision 1.62  1994/10/06  21:40:33  shawn
 * If user tries to create a file with filename length of
 * 32 bytes. It is created but no blocks can be allocated to
 * it and can not be deleted either. Basically, is not found
 * again and becomes a zombie file. Let's reject it.
 *
 * Revision 1.61  1994/10/06  17:55:05  shawn
 * Let the sema4 name tag identify the file entry it belongs
 * to.
 *
 * Revision 1.60  1994/09/29  00:01:43  shawn
 * Item n is defined under both debug environments. This fixes
 * compile problems with DEBUG or SEMDEBUG and no warning
 * message for no DEBUG environment.
 *
 * Revision 1.59  1994/09/09  03:10:06  dplatt
 * Use new SuperInternalDoIO() call.
 *
 * Revision 1.58  1994/08/19  17:56:24  shawn
 * This fixes the duplicate filename creation bug.
 * There is a window, where a directory is searched for an
 * entry and then if not found, it is created. However, between
 * the search (READENTRY) and the create operation (ADDENTRY), tasks
 * are blocked. Therefore multiple tasks may perform the same search
 * and before the first one creates the file, they miss it thus create
 * the same file multiple of times. The directory lock is enabled to
 * close this gap.
 *
 * Revision 1.57  1994/08/15  17:11:38  shawn
 * Allow fsOpenForBusiness be defined without
 * caching enabled.
 *
 * Revision 1.56  1994/07/30  00:07:58  shawn
 * Hardened the error checking for CreateLinkSWI. Only
 * allowed for directory, since it is an expensive
 * operation for FindFile and should not be used
 * casualy.
 *
 * Revision 1.55  1994/07/26  21:09:05  dplatt
 * Add math for a new "Filesystem blocks per file block" variable in the
 * File structure.  Having this around simplifies the math in the driver
 * (esp. w/r/t keeping track of the "next filesystem block readable from
 * the device" value which Catapult and the avatar-seeker depend upon).
 *
 * Revision 1.54  1994/06/17  01:33:37  shawn
 * Changes to OpenFileInDir to force OpenPath to recognize
 * any link which might exist during lookup.
 *
 * Revision 1.53  1994/06/16  21:10:59  dplatt
 * Make use of the new "filesystem code is fully operational" flag maintained
 * by FileFolioMain.c.  If an attempt to find a filesystem (e.g. a name
 * search at the "/" level) is about to fail, and the fully-operational
 * flag has not been set, yield time to the daemon and then try again.
 * Keep trying (and giving the daemon a chance to do its mount operations)
 * until all of the mounting is complete - only at this point will a
 * "no such filesystem" error be reported.
 *
 * Revision 1.52  1994/06/15  18:55:51  shawn
 * usecount needs to be decremented for link stuff, if there
 * is a file found and is not selected by the selection function.
 *
 * Revision 1.51  1994/06/15  02:09:04  dplatt
 * Maintain the "Are we in the middle of an {alternative} clause?"
 * state explicitly, rather than trying to infer it from the states
 * of the punt and rescan variables.  This fixes Deborah's filesystem
 * bug involving /rom2 and /remote.
 *
 * Revision 1.50  1994/06/14  23:31:51  shawn
 * First Step towards not supporting this call.
 *
 * Revision 1.49  1994/06/14  00:57:40  limes
 * ofi_RegisteredBuffer was allocated with GetDirectoryBuffer; it
 * should always be freed with ReleaseDirectoryBuffer (not FREEMEM).
 *
 * Revision 1.48  1994/06/10  22:55:00  shawn
 * Phae II changes for rom over cd.
 *
 * Revision 1.47  1994/06/10  20:49:29  dplatt
 * Use new kernel routines to acquire directory buffer memory, and
 * directory cache page.
 *
 * Revision 1.46  1994/06/07  17:06:11  dplatt
 * Somebody's actually using a path of $boot/$boot/something.  This works
 * fine as long as $boot is a single filesystem name.  It fails if $boot
 * is a {/fs1|/fs2} alternative, because the second alternative is hit
 * while the first is still pending.
 *
 * Fix this by the following strategy:  if a naked slash (return to root of
 * namespace) appears after (outside of) an alternative, clear the
 * alternative-in-progress information and proceed normally.  This yields
 * the same basic semantics as the previous strategy... it just shortcuts
 * the alternative (since all of its branches would lead back to the root
 * anyhow).  To use this, the boot alias can be set to something like
 * /{rom|cd-rom}.  This causes the nasty $boot/$boot/foo/bar to expand to
 * /{rom|cd-rom}//{rom|cd-rom}/foo/bar, and the double-slash returns the
 * search to the root and terminates the first alternative.
 *
 * Ick.  This is the programming equivalent to brussels sprouts.
 *
 * Revision 1.45  1994/06/06  22:57:45  dplatt
 * Fix use-count accounting in the file-linking code.
 * Transfer ownership of all File items to the file daemon, not just
 * directory items.  This avoids the problem of items being transferred
 * to null ownership, if they are opened by process A, opened in
 * parallel by process B, and then process A exits.
 *
 * Revision 1.44  1994/05/17  20:17:04  shawn
 * changes for rom over cd for first phase.
 *
 * Revision 1.43  1994/05/17  00:30:16  shawn
 * preliminary changes for rom over cd.
 *
 * Revision 1.41  1994/04/22  19:42:11  dplatt
 * When looking at filesystems, compare both the name and the mount-
 * point name.
 *
 * Revision 1.40  1994/04/05  18:55:33  shawn
 * Added directory semaphore.
 *
 * Revision 1.39  1994/03/24  01:51:54  dplatt
 * DBUG0 changed back to the way it should be.
 *
 * Revision 1.38  1994/02/18  19:27:48  shawn
 * Do not allow to cd to a file.
 *
 * Revision 1.37  1994/02/07  18:23:27  shawn
 * DBUG0 redefined.
 *
 * Revision 1.36  1994/02/07  18:14:53  shawn
 * No DBUG0 messages.
 *
 * Revision 1.35  1994/01/21  00:29:21  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.35  1994/01/19  20:54:36  dplatt
 * Change zero-use-count File cleanup strategy
 *
 * Revision 1.34  1993/12/16  00:56:25  dplatt
 * Add support for BARF filesystems which require a FILECMD_OPENENTRY.
 * Make the directory cache a bit more general so it can hold BARF pages
 * without fear of xlkyhfweo.
 *
 * Revision 1.33  1993/10/25  23:18:55  limes
 * Fix a compile warning.
 *
 * Revision 1.32  1993/09/01  23:14:07  dplatt
 * Scavenge File items with zero use count... both on-the-fly (flush
 * LRU if more than 16 exist) and when a task exits (flush all).
 *
 * Revision 1.31  1993/08/10  00:32:04  dplatt
 * Don't let alias expansions loop forever;  kill after 32 tries.
 *
 * Revision 1.30  1993/07/28  02:30:50  dplatt
 * Zero out IOReqs;  do delete-item cleanup
 *
 * Revision 1.29  1993/07/20  07:04:19  dplatt
 * Directory cache
 *
 * Revision 1.28  1993/07/09  18:02:08  dplatt
 * Don't hang if bad data is detected in a directory block and the problem
 * doesn't go away after a few rereads.  Instead, just force a "file
 * not found" condition.
 *
 * Revision 1.27  1993/06/24  03:02:06  dplatt
 * Fix nvram problems
 *
 * Revision 1.26  1993/06/18  02:00:25  dplatt
 * I shouldn't have deleted those lines...
 *
 * Revision 1.25  1993/06/18  01:40:07  dplatt
 * Issue a CloseItem() before DeleteItem() when closing a disk file.
 *
 * Revision 1.24  1993/06/15  20:33:46  dplatt
 * use string.h rather than strings.h
 *
 * Revision 1.23  1993/06/15  20:30:15  dplatt
 * Remove extraneous extern for strcasecmp
 *
 * Revision 1.22  1993/06/15  20:15:34  dplatt
 * Don't cache file info for Macintosh /remote files
 *
 * Revision 1.21  1993/06/15  04:08:43  dplatt
 * strncasecmp becomes strcasecmp
 *
 * Revision 1.20  1993/06/15  00:55:14  dplatt
 * Ensure that OpenItem calls are in place
 *
 * Revision 1.19  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.18  1993/05/31  03:33:05  dplatt
 * Tweaks, and filesystem dismounting
 *
 * Revision 1.17  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.16  1993/05/08  01:08:14  dplatt
 * Add flat-file-system/NVRAM support, and recover from RCS bobble
 *
 * Revision 1.15  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.14  1993/02/12  01:59:00  dplatt
 * Suppress excess error message unless in debug mode
 *
 * Revision 1.13  1993/02/11  19:39:37  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.12  1993/02/09  01:47:20  dplatt
 * Reorganize and update for developer release
 *
 * Revision 1.11  1993/01/05  20:57:47  dplatt
 * CES changes redux
 *
 * Revision 1.10  1993/01/04  02:19:26  dplatt
 * CES Changes
 *
 * Revision 1.9  1992/12/22  07:58:10  dplatt
 * Magneto 3 changes and CD-ROM support
 *
 * Revision 1.8  1992/12/08  05:59:52  dplatt
 * Magenta changes
 *
 * Revision 1.7  1992/11/06  01:12:39  dplatt
 * Make LoadProgram() into a library routine
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
  OpenClose.c - code to open and close files.
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

int32 internalReadNextBlock(OpenFile *, int32);
int32 internalGetPath(File *theFile, char *theBuffer, int32 bufLen);
void  adjustLinkInfo(File *here, FileFolioTaskData *ftdp, File **linkFile,
		     char *afterLinkPath);

extern FileFolioTaskData *SetupFileFolioTaskData(void);
extern void GiveDaemon(void *foo);

#ifdef MACFS
extern int32 ConstructMacPath(File *theTail, char *buf, int32 bufLen);
#endif

extern void MySysErr(int32 err);

enum FindMode 
{
  FindExistingEntry,
  CreateNewEntry,
  DeleteExistingEntry,
  UpdateCurrentDirectory
};

#define	OPENFILE_TAG_NUM	6
static const TagArg openFileArgs[OPENFILE_TAG_NUM] = {
  { TAG_ITEM_PRI,            (void *)1, },
  { CREATEDEVICE_TAG_DRVR,    NULL },
  { TAG_ITEM_NAME,            NULL },
  { CREATEDEVICE_TAG_IOREQSZ, (void *) sizeof (FileIOReq) },
  { CREATEDEVICE_TAG_CLOSE,   NULL },
  { TAG_END,                  0, },
};

static const TagArg createAliasArgs[] = {
  { TAG_ITEM_PRI,            (void *)1, },
  { TAG_ITEM_NAME,            NULL },
  { TAG_END,                  0, },
};

#undef WATCHDIRECTORY
#undef PARANOID
#undef DUMPFILES
/* #define DEBUG */
/* #define SEMDEBUG */

#ifdef	SEMDEBUG
#define SEMDBUG(x) Superkprintf x
#else	/* SEMDEBUG */
#define SEMDBUG(x) /* x */
#endif	/* SEMDEBUG */

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

extern int32 fsOpenForBusiness;

#ifdef DOCACHE

extern char *fsCacheBase;
extern List fsCacheList;

IoCacheEntry *FindCache(OpenFile *theOpenFile, int32 blockNum);
IoCacheEntry *AddCacheEntry(OpenFile *theOpenFile, int32 blockNum,
			    void *buffer,
			    uint8 priority, uint32 format, uint32 bytes);
void AgeCache(void);

#endif

File	*preferFirst(File *firstFile, File *secondFile, File *firstLinkp,
		   File *secondLinkp);
File	*hideFirst(File *firstFile, File *secondFile, File *firstLinkp,
		   File *secondLinkp);

void *GetDirectoryBuffer(int32 *size);
void ReleaseDirectoryBuffer(void *buffer, int32 size);

#ifndef PRODUCTION
void DumpFileList(void)
{
  File *theFile;
  DBUG0(("Files currently in list:\n"));
  theFile = (File *) FIRSTNODE(&fileFolio->ff_Files);
  while (ISNODE(&fileFolio->ff_Files,theFile)) {
    DBUG0(("  Item %x at %x name %s", theFile->fi.n_Item, theFile,
		 theFile->fi_FileName));
    DBUG0((" use-count %d\n", theFile->fi_UseCount));
    theFile = (File *) NEXTNODE(theFile);
  }
}
#endif

void TrimSomeZeroUseFiles(void)
{
  File *theFile, *nextFile;
  int32 limit;
  limit = MAX_ZERO_USE_FILES;
#ifdef DUMPFILES
  DBUG0(("TrimSomeZeroUseFiles\n"));
  DumpFileList();
#endif
  theFile = (File *) FirstNode(&fileFolio->ff_Files);
  while (ISNODE(&fileFolio->ff_Files,theFile)) {
    nextFile = (File *) NextNode(theFile);
    DBUG(("Check File item %s\n", theFile->fi_FileName));
    if (theFile->fi_UseCount == 0 && --limit < 0) {
      DBUG(("Deleting File item for %s\n", theFile->fi_FileName));
#ifdef	FS_DIRSEMA
      DBUG(("TrimSomeZeroFiles: Calling DelDirSema for [%s], task: 0x%x\n", theFile->fi_FileName, CURRENTTASK->t.n_Item));
      DelDirSema(theFile);
#endif	/* FS_DIRSEMA */
      SuperInternalDeleteItem(theFile->fi.n_Item);
    }
    theFile = nextFile;
  }
  fileFolio->ff_OpensSinceCleanup = 0;
}

void TrimAllZeroUseFiles(void)
{
  File *theFile, *nextFile;
#ifdef DUMPFILES
  DBUG0(("TrimAllZeroUseFiles\n"));
  DumpFileList();
#endif
  theFile = (File *) FIRSTNODE(&fileFolio->ff_Files);
  while (ISNODE(&fileFolio->ff_Files,theFile)) {
    nextFile = (File *) NEXTNODE(theFile);
    if (theFile->fi_UseCount == 0) {
      DBUG(("Deleting File item for %s\n", theFile->fi_FileName));
#ifdef	FS_DIRSEMA
      DBUG(("TrimAllZeroFiles: Calling DelDirSema for [%s], task: 0x%x\n", theFile->fi_FileName, CURRENTTASK->t.n_Item));
      DelDirSema(theFile);
#endif	/* FS_DIRSEMA */
      SuperInternalDeleteItem(theFile->fi.n_Item);
    }
    theFile = nextFile;
  }
  fileFolio->ff_OpensSinceCleanup = 0;
}

static Alias *FindAlias(uchar *name)
{
  Alias *theAlias;
  Task *theTask;
  FileFolioTaskData *fftd;
  Item ownerItem;
  theTask = CURRENTTASK;
  DBUG(("Searching for alias %s\n", name));
  while (TRUE) {
    fftd = (FileFolioTaskData *) theTask->t_FolioData[fileFolio->ff.f_TaskDataIndex];
    if (fftd) {
      DBUG(("Searching alias list for task %s\n", theTask->t.n_Name));
      theAlias = (Alias *) FindNamedNode(&fftd->fftd_AliasList, name);
      if (theAlias) {
	return theAlias;
      }
    } else {
      DBUG(("Task %s has no aliases\n", theTask->t.n_Name));
    }
    ownerItem = theTask->t.n_Owner;
    if (ownerItem == 0 || ownerItem == theTask->t.n_Item) {
      DBUG(("Top of task inheritance list, search failed\n"));
      return (Alias *) NULL;
    }
    DBUG(("Look back up one level\n"));
    theTask = (Task *) LookupItem(ownerItem);
  }
}

void CleanupOnClose(Device *theDev)
{
  Err err;
  File *theFile;
  OpenFile *theOpenFile = (OpenFile *) theDev;

  if (theOpenFile == (OpenFile *) NULL) {
    return;
  }
  theFile = theOpenFile->ofi_File;
  DBUG(("CleanupOnClose: Cleaning up file %s\n", theFile->fi_FileName));
  if (theOpenFile->ofi_InternalIOReq != (IOReq *) NULL) {
    DBUG(("CleanupOnClose: Deleting IOReq for %s\n", theFile->fi_FileName));
    if ((err = SuperDeleteItem(theOpenFile->ofi_InternalIOReq->io.n_Item)) != 0) {
      DBUG(("CleanupOnClose: Failed to delete IOREQ %x\n", err));
    } else {
      theOpenFile->ofi_InternalIOReq = (IOReq *) NULL;
    }
  } else {
    DBUG(("CleanupOnClose: InternalIOReq for %s is NULL\n", theFile->fi_FileName));
  }
  return;
}


static Err CleanupOpenFile(Device *theDev)
{
  int32 bufSize;
  File *theFile;
  OpenFile *theOpenFile = (OpenFile *) theDev;
  DBUG(("CleanupOpenFile\n"));
  if (theOpenFile == (OpenFile *) NULL) {
    return 0;
  }
  theFile = theOpenFile->ofi_File;
  DBUG(("Cleaning up file %s\n", theFile->fi_FileName));
#ifdef DEBUG
  Superkprintf("Closing file %s at %lx\n", theOpenFile->ofi.dev.n_Name,
	       theOpenFile);
  DumpFileList();
#endif
/*
 *  we should have deleted the IOReq by now.
 */
  if (theOpenFile->ofi_InternalIOReq != (IOReq *) NULL) {
    DBUG(("Deleting IOReq at %lx\n", theOpenFile->ofi_InternalIOReq));
    SuperDeleteItem(theOpenFile->ofi_InternalIOReq->io.n_Item);
    theOpenFile->ofi_InternalIOReq = (IOReq *) NULL;
  }
  if (theOpenFile->ofi_RegisteredBuffer != (void *) NULL) {
/*
 * ofi_RegisteredBuffer is always allocated via
 * GetDirectoryBuffer() and freed via ReleaseDirectoryBuffer.
 */
    bufSize = (int32) theOpenFile->ofi_BufferBlocks *
      (int32) theFile->fi_BlockSize;
    DBUG(("Releasing %d-byte buffer at %lx\n", bufSize,
		 theOpenFile->ofi_RegisteredBuffer));
    ReleaseDirectoryBuffer(theOpenFile->ofi_RegisteredBuffer,
			   bufSize);
  }
  theFile->fi_UseCount --;
  DBUG(("Use count of file %s decremented to %d\n",
	 theFile->fi_FileName,
	 theFile->fi_UseCount));
  if (theFile->fi_UseCount == 0) {
    if (theFile->fi_Flags & FILE_INFO_NOT_CACHED) {
      SuperInternalDeleteItem(theFile->fi.n_Item);
    }
  }
  return 0;
}

OpenFile *OpenAFile(File *theFile)
{
  uint8 taskPrivs;
  Item openFileItem;
  OpenFile *theOpenFile;
  Err err;
  TagArg openFileArgBlock[OPENFILE_TAG_NUM];
  if (!theFile) {
    return (OpenFile *) NULL;
  }
  theFile->fi_UseCount ++;  /* prevent scavenging */
  memcpy(openFileArgBlock, openFileArgs, sizeof openFileArgBlock);
  openFileArgBlock[1].ta_Arg = (void *) fileDriver->drv.n_Item;
  openFileArgBlock[2].ta_Arg = theFile->fi.n_Name;
  openFileArgBlock[4].ta_Arg = (void *) CleanupOnClose;
#ifdef DEBUG
  Superkprintf("Creating an open-file for file '%s' at %lx\n",
	       openFileArgBlock[2].ta_Arg, theFile);
  DumpFileList();
#endif
  taskPrivs = KernelBase->kb_CurrentTask->t.n_Flags;
  KernelBase->kb_CurrentTask->t.n_Flags |= TASK_SUPER /* seize privilege */;
  openFileItem = SuperCreateSizedItem(MKNODEID(KERNELNODE,DEVICENODE),
				      openFileArgBlock,
				      sizeof (OpenFile));
  KernelBase->kb_CurrentTask->t.n_Flags =
    (KernelBase->kb_CurrentTask->t.n_Flags & ~TASK_SUPER) |
      (taskPrivs & TASK_SUPER); /* restore privilege */
  if (openFileItem < 0) {
#ifdef DEBUG
    DBUG(("Can't create open-file item (0x%x)\n", openFileItem));
    MySysErr(openFileItem);
#endif
    theFile->fi_UseCount --;
    ((FileFolioTaskData *) KernelBase->kb_CurrentTask->
      t_FolioData[fileFolio->ff.f_TaskDataIndex])->fftd_ErrorCode =
	openFileItem;
    return NULL;
  }
  theOpenFile = (OpenFile *) LookupItem(openFileItem);
  if (theOpenFile == (OpenFile *) NULL) {
    DBUG(("Could not create open-file!\n"));
    theFile->fi_UseCount --;
    return theOpenFile;
  }
  theOpenFile->ofi_File = theFile;
  theOpenFile->ofi_DeviceType = FILE_DEVICE_OPENFILE;
  theOpenFile->ofi.dev_DeleteDev = CleanupOpenFile;
#ifdef DEBUG
  DBUG(("Open-file created at %lx\n", theOpenFile));
  DumpFileList();
#endif
  DBUG(("Opening item 0x%x\n", openFileItem));
  err = SuperOpenItem(openFileItem, 0);
  DBUG(("Item 0x%x opened\n", openFileItem));
  if (err < 0) {
    DBUG(("Couldn't OpenItem() on a newly-created OpenFile!\n"));
    MySysErr(err);
  }
  if (!(theFile->fi_Flags & FILE_IS_DIRECTORY)) {
    RemNode((Node *) theFile);
    AddHead(&fileFolio->ff_Files, (Node *) theFile);
  }
  if (++fileFolio->ff_OpensSinceCleanup >= CLEANUP_TIMER_LIMIT) {
    TrimSomeZeroUseFiles();
  }
  return theOpenFile;
}

/*
  N.B.  If FindFile returns a non-null File pointer, the use count of
        the returned File will have been incremented by one to ensure that
        it is not subject to scavenging.  The caller should account for
        this, and decrement the use count at an appropriate time.

  Memo to self - the use count of "whereNow" is not bumped up by one
  within the Grand Loop.  When an item-table scavenger is implemented
  in the folio, it will be necessary either to do the bumping in this
  fashion or have some other lockout to ensure that the whereNow File
  item is not flushed during the open-the-whereNow-file process or
  elsewhen.

*/

File *
FindFile (File *startingDirectory, char *path, enum FindMode mode, 
	  File **linkFile, char *afterLinkPath)
{
  File *whereNow, *nextLevel, *wasAt, *punt;
  FileSystem *fs;
  OpenFile *openFile;
  Item fileItem, ioReqItem = (Item) -1;
  DirectoryRecord *dirRec = (DirectoryRecord *) NULL;
  FileFolioTaskData *ffPrivate;
  DirectoryEntry remoteDir;
  MacDisk *macDisk;
#ifdef PARANOID
  int32 scanTries = 0;
#endif
  int32 errcode;
  int32 triedToCreate;
  int32 expansions;
  int32 partpath = 1;
  int32 updatewd = 0;
  IOReq *rawIOReq = (IOReq *) NULL;
  char c, *fullPath, *buffer, *point, *rescan, *nameStart, *nameEnd;
  char *dirPage = (char *) NULL;
#ifdef DOCACHE
  int32 doCache, cachePass;
  IoCacheEntry *entry = (IoCacheEntry *) NULL;
#endif
  char *expandedPath, *newExpandedPath;
  int32 expandedPathLen, newExpandedPathLen;
  char fileName[FILESYSTEM_MAX_NAME_LEN+1];
  int32 blockNum, offset;
  int32 avatarIndex, nameLen, compare, bufferSize = 0, insertPoint;
  int32 fileSize;
  int32 insideAlternative;
  TagArg ioReqTags[2];
  Alias *theAlias;
  ffPrivate = (FileFolioTaskData *) KernelBase->kb_CurrentTask->
    t_FolioData[fileFolio->ff.f_TaskDataIndex];
  if (!ffPrivate) {
    DBUG(("OOPS!  FindFile entered with null private pointer\n"));
  }
  *linkFile = (File *) NULL;
  whereNow = startingDirectory;
  wasAt = punt = (File *) NULL;
  openFile = (OpenFile *) NULL;
  buffer = (char *) NULL;
  rescan = expandedPath = newExpandedPath = (char *) NULL;
  expandedPathLen = newExpandedPathLen = 0;
  triedToCreate = FALSE;
  insideAlternative = FALSE;
  fullPath = path;
  expansions = 0;
  if (mode == UpdateCurrentDirectory) {
    mode = FindExistingEntry;
    updatewd++;
  }
  DBUG(("FindFile, base dir %d, path %s\n", startingDirectory, path));
 theGrandLoop:
  while (*path) {
    DBUG(("Opening from 0x%lx, look for '%s' path %lx\n",
		 whereNow, path, path));
    c = *path;
    if (c == '/') { /* go back to root */
      DBUG(("Back to root\n"));
      whereNow = fileFolio->ff_Root;
      path ++;
      partpath = 0;
      if (punt && !insideAlternative) { /* {this|that}/foo/bar//root/... dcp */
	punt->fi_UseCount --;
	punt = NULL;
	rescan = NULL;
      }
      continue;
    }
    if (c == '{') {
      DBUG(("Alternative!\n"));
      if (rescan != NULL | mode != FindExistingEntry) {
	DBUG(("Malformed alternative-search!\n"));
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	goto trueFailure;
      }
      punt = whereNow;
      punt->fi_UseCount ++;
      path++;
      rescan = path;
      insideAlternative = TRUE;
      DBUG(("Rescan point is %s\n", rescan));
      continue;
    }
    nameLen = 0;
    nameStart = path;
    do {
      nameEnd = path;
      c = *path;
      if (c == '\0') {
	fileName[nameLen] = '\0';
	break;
      }
      if (c == '/') {
	fileName[nameLen] = '\0';
	path++;
	break;
      }
      if (c == '|') {
	DBUG(("Hit an or-delimiter\n"));
	if (punt == NULL) {
	  ffPrivate->fftd_ErrorCode =
	    MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	  goto trueFailure;
	}
	fileName[nameLen] = '\0';
	path++;
	do {
	  c = *path++;
	  if (c == '\0') {
	    ffPrivate->fftd_ErrorCode =
	      MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	    goto trueFailure;
	  }
	} while (c != '}');
	if (*path == '/') {
	  path++;
	}
	insideAlternative = FALSE;
	DBUG(("Skipped ahead to %s\n", path));
	break;
      }
      if (c == '}') {
	DBUG(("Hit end of alternative\n"));
#ifdef FIXTHISBETTER
	if (rescan == NULL) {
	  ffPrivate->fftd_ErrorCode =
	    MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	  goto trueFailure;
	}
#endif
	fileName[nameLen] = '\0';
	path++;
	if (*path == '/') {
	  path++;
	}
	rescan = NULL;
	punt->fi_UseCount --;
	punt = (File *) NULL;
	insideAlternative = FALSE;
	DBUG(("Cleared punt and rescan\n"));
	break;
      }
      if (nameLen >= FILESYSTEM_MAX_NAME_LEN) {
	DBUG(("Filename too long!\n"));
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	goto trueFailure;
      }
      fileName[nameLen++] = c;
      path++;
    } while (1);
    if (fileName[0] == '$') {
      DBUG(("Alias substitution for %s\n", fileName));
      if (++expansions >= 32) {
	DBUG(("Excessive alias expansion!\n"));
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	goto trueFailure;
      }
      theAlias = FindAlias(&fileName[1]);
      if (!theAlias) {
	DBUG(("Alias not found\n"));
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_BadName);
	goto trueFailure;
      }
      DBUG(("Found alias, value is '%s'\n", theAlias->a_Value));
      newExpandedPathLen = (nameStart - fullPath) /* prefix */ +
	strlen(theAlias->a_Value) /* substitution */ +
	  strlen(nameEnd) /* suffix */ + 1;
      DBUG(("Need %d bytes for expansion\n", newExpandedPathLen));
      newExpandedPath = (char *) AllocMem(newExpandedPathLen,
					  MEMTYPE_ANY + MEMTYPE_FILL);
      if (!newExpandedPath) {
	ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
	goto trueFailure;
      }
      insertPoint = nameStart - fullPath;
      strncpy(newExpandedPath, fullPath, insertPoint);
      strcpy(newExpandedPath + insertPoint, theAlias->a_Value);
      strcat(newExpandedPath, nameEnd);
      path = newExpandedPath + (nameStart - fullPath);
      if (rescan) {
	rescan = newExpandedPath + (rescan - fullPath);
      }
      if (expandedPath) {
	FreeMem(expandedPath, expandedPathLen);
      }
      expandedPath = fullPath = newExpandedPath;
      expandedPathLen = newExpandedPathLen;
      DBUG(("Aliased path is now '%s'\n", fullPath));
      DBUG(("Start scanning at '%s'\n", path));
      continue;
    }
    DBUG(("Searching for %s from directory 0x%lx\n",
		 fileName, whereNow));
    DBUG(("Remainder is '%s' based at 0x%lx\n",
		 path, path));
    if (strcmp(fileName, ".") == 0) {
      continue;
    }
    if (strcmp(fileName, "..") == 0) {
#ifdef NOTDEF
      if (whereNow) {
	whereNow = whereNow->fi_ParentDirectory;
      }
#else
      if (partpath && ffPrivate->fftd_LinkDirectory && updatewd) {
	if (!(*linkFile)) {
	  strcpy(afterLinkPath, ffPrivate->fftd_LinkPartPath);
	  *linkFile = ffPrivate->fftd_LinkDirectory;
	}
	adjustLinkInfo(whereNow, ffPrivate, linkFile, afterLinkPath);
      }

      whereNow = whereNow->fi_ParentDirectory;
#endif
      continue;
    }
    if (strcmp(fileName, "^") == 0) {
      if (whereNow->fi_FileSystem) {
	whereNow = whereNow->fi_FileSystem->fs_RootDirectory;
      }
      continue;
    }
    if (partpath && (startingDirectory == ffPrivate->fftd_CurrentDirectory) &&
	(startingDirectory != fileFolio->ff_Root) &&
        (*linkFile == (File *) NULL) &&
	(ffPrivate->fftd_LinkDirectory)) {
      *linkFile = ffPrivate->fftd_LinkDirectory;
      if (afterLinkPath) {
	if (ffPrivate->fftd_LinkPartPath && *(ffPrivate->fftd_LinkPartPath)) {
	  strcpy(afterLinkPath, ffPrivate->fftd_LinkPartPath);
	  strcat(afterLinkPath, "/");
	}
	strcat(afterLinkPath, fullPath);
      }
      DBUG(("FindFile: afterLinkPath [%s], [%s], [%s]\n", afterLinkPath, path, fullPath));
    }
    DBUG(("Searching for %s, %s\n", path, fullPath));
    LockDirSema(whereNow, "before cache search");
    if (strlen(fileName) >= FILESYSTEM_MAX_NAME_LEN) {
	ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_BadName);
	goto trueFailure;
    }
    nextLevel = (File *) FIRSTNODE((&fileFolio->ff_Files));
    while (ISNODE(&fileFolio->ff_Files,nextLevel) &&
	   (nextLevel->fi_ParentDirectory != whereNow ||
	    (nextLevel->fi_Flags & FILE_INFO_NOT_CACHED) ||
	    strcasecmp(fileName, nextLevel->fi_FileName) != 0)) {
      nextLevel = (File *) NEXTNODE((&(nextLevel->fi)));
    }
    if (ISNODE(&fileFolio->ff_Files,nextLevel)) { /* found it */
      DBUG(("Found in cache %s, %s, flags 0x%x\n", fileName, nextLevel->fi_FileName, nextLevel->fi_Flags));
      if ((*linkFile == (File *) NULL) &&
          (nextLevel->fi_Flags & FILE_HAS_LINK)) {
	*linkFile = nextLevel;
        DBUG(("FindFile: In Cache path %s, %s\n", path, fullPath));
	if (afterLinkPath)
		strcpy(afterLinkPath, path);
      }

      UnlockDirSema(whereNow, "found in cache");
      whereNow = nextLevel;
      DBUG(("Found %s in list at %x\n", fileName, whereNow));
      continue;
    }
    if (whereNow == fileFolio->ff_Root) {
      DBUG(("Searching filesystem list\n"));
    searchFilesystems:
      fs = (FileSystem *) FIRSTNODE((&fileFolio->ff_Filesystems));
      while (fs != (FileSystem *) NULL &&
	     (strcasecmp(fileName, fs->fs_FileSystemName) != 0 &&
	      strcasecmp(fileName, fs->fs_MountPointName) != 0)) {
	fs = (FileSystem *) NEXTNODE((&(fs->fs)));
      }
      if (!fs) {
	DBUG(("Cannot locate filesystem /%s for %s\n",fileName,fullPath));
/*
   If the daemon has not yet finished the initial series of filesystem
   mounts, yield some time to higher-priority tasks (such as the filesystem
   main thread) and then try again.  A semaphore or an explicit
   sleep would be a much cleaner way of doing this!
*/
	if (!fsOpenForBusiness) {
	  SuperSwitch();
	  goto searchFilesystems;
	}
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFileSystem);
	goto tryAlternate;
      }
      UnlockDirSema(whereNow, "found in FScache");
      whereNow = fs->fs_RootDirectory;
      DBUG(("Found filesystem /%s root at %d\n", fileName, whereNow));
      continue;
    }
    if ((whereNow->fi_Flags & FILE_IS_DIRECTORY) == 0) {
      DBUG(("%s is a file, not a directory\n", fileName));
      ffPrivate->fftd_ErrorCode =
	MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
      goto tryAlternate;
    }
    DBUG(("Path is now %lx\n", path));
    rawIOReq = NULL;
    openFile = OpenAFile(whereNow);
    if (!openFile) goto cleanup;
    if (!(whereNow->fi_Flags & FILE_SUPPORTS_DIRSCAN)) {
/*
 * ofi_RegisteredBuffer is always allocated via
 * GetDirectoryBuffer() and freed via ReleaseDirectoryBuffer.
 */
      bufferSize = (int32) whereNow->fi_BlockSize;
      DBUG(("Alloc %d bytes\n", bufferSize));
      buffer = (char *) GetDirectoryBuffer(&bufferSize);
      if (!buffer || bufferSize < (int32) whereNow->fi_BlockSize) {
	DBUG0(("Can't allocate a buffer to find %s, got %d bytes at 0x%X\n",
	       fileName, bufferSize, buffer));
	ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
	goto trueFailure;
      }
      openFile->ofi_RegisteredBuffer = buffer;
      openFile->ofi_BufferBlocks = 1;
      openFile->ofi_BufferBlocksAvail = 1;
      openFile->ofi_BufferBlocksFilled = 0;
    }
    ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
    ioReqTags[0].ta_Arg = (void *) openFile->ofi.dev.n_Item;
    ioReqTags[1].ta_Tag = TAG_END;
    ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
    if (ioReqItem < 0) {
      DBUG(("Can't allocate an internal IOReq for %s", fileName));
      ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
      goto cleanup;
    }
    openFile->ofi_InternalIOReq = rawIOReq = (IOReq *) LookupItem(ioReqItem);
  probeForIt:
    if (whereNow->fi_Flags & FILE_SUPPORTS_ENTRY) {
      DBUG(("Trying a directory probe on %s/%s\n", whereNow->fi.n_Name,fileName));
      macDisk = (MacDisk *) whereNow->fi_FileSystem->fs_Device;
      rawIOReq->io_Info.ioi_Command = FILECMD_READENTRY;
      rawIOReq->io_Info.ioi_Flags = 0;
      rawIOReq->io_Info.ioi_Send.iob_Buffer = (void *) fileName;
      rawIOReq->io_Info.ioi_Send.iob_Len = strlen(fileName);
      rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) &remoteDir;
      rawIOReq->io_Info.ioi_Recv.iob_Len = sizeof remoteDir;
      rawIOReq->io_Info.ioi_Offset = 0;
      errcode = SuperInternalDoIO(rawIOReq);
      if (errcode < 0 || (errcode = rawIOReq->io_Error) < 0) {
	DBUG(("Probe failed for %s, error 0x%x\n", fullPath, errcode));
#ifdef FALLBACK_ON_SCAN_AFTER_READ_ENTRY
	goto scanForIt;
#else
	ffPrivate->fftd_ErrorCode =
	  MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
	goto tryAlternate;
#endif
      }
      whereNow->fi_UseCount ++;
      fileItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
				      (void *) NULL, sizeof (File));
      nextLevel = (File *) LookupItem(fileItem);
      if (!nextLevel) {
	DBUG(("Can't allocate a new File object\n"));
	ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
	whereNow->fi_UseCount --;
	goto cleanup;
      }
      DBUG(("Path is now %lx\n", path));
      DBUG(("Initializing File at %lx for %s\n", nextLevel,
		   fileName));
      strncpy(nextLevel->fi_FileName, fileName,
	      FILESYSTEM_MAX_NAME_LEN);
      nextLevel->fi_FileSystem = whereNow->fi_FileSystem;
      nextLevel->fi_ParentDirectory = whereNow;
      if (remoteDir.de_UniqueIdentifier == 0) {
	nextLevel->fi_UniqueIdentifier = fileFolio->ff_NextUniqueID --;
      } else {
	nextLevel->fi_UniqueIdentifier = remoteDir.de_UniqueIdentifier;
      }
      DBUG(("Flags 0x%X, bytes %d, blocks %d\n", remoteDir.de_Flags, remoteDir.de_ByteCount, remoteDir.de_BlockCount));
      nextLevel->fi_Flags = remoteDir.de_Flags;
      nextLevel->fi_Burst = remoteDir.de_Burst;
      nextLevel->fi_Gap = remoteDir.de_Gap;
      nextLevel->fi_BlockSize = remoteDir.de_BlockSize;
      nextLevel->fi_BlockCount = remoteDir.de_BlockCount;
      nextLevel->fi_ByteCount = remoteDir.de_ByteCount;
      nextLevel->fi_LastAvatarIndex = remoteDir.de_AvatarCount - 1;
      nextLevel->fi_AvatarList[0] = remoteDir.de_Location;
      nextLevel->fi_Type = remoteDir.de_Type;
      nextLevel->fi_FileSystemBlocksPerFileBlock = 1;
      goto built;
    }
#ifdef FALLBACK_ON_SCAN_AFTER_READ_ENTRY
  scanForIt:
#endif
    if (whereNow->fi_Flags & FILE_SUPPORTS_DIRSCAN) {
      macDisk = (MacDisk *) whereNow->fi_FileSystem->fs_Device;
      rawIOReq->io_Info.ioi_Command = FILECMD_READDIR;
      rawIOReq->io_Info.ioi_Flags = 0;
      rawIOReq->io_Info.ioi_Send.iob_Buffer = (void *) NULL;
      rawIOReq->io_Info.ioi_Send.iob_Len = 0;
      rawIOReq->io_Info.ioi_Recv.iob_Buffer = (void *) &remoteDir;
      rawIOReq->io_Info.ioi_Recv.iob_Len = sizeof remoteDir;
      rawIOReq->io_Info.ioi_Offset = 1;
      while (1) {
	errcode = SuperInternalDoIO(rawIOReq);
	if (errcode < 0 || (errcode = rawIOReq->io_Error) < 0) {
	  DBUG(("Scan for %s halts on error 0x%x\n", fullPath, errcode));
	  if (errcode == -1) {
	    ffPrivate->fftd_ErrorCode =
	      MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
	  } else {
	    ffPrivate->fftd_ErrorCode = errcode;
	  }
	  goto tryAlternate;
	}
	DBUG(("Index %d is %s, flags %lx\n",
		     rawIOReq->io_Info.ioi_Offset,
		     remoteDir.de_FileName,
		     remoteDir.de_Flags));
	if (strcasecmp(fileName, remoteDir.de_FileName) == 0) {
	  whereNow->fi_UseCount ++;
	  fileItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
					  (void *) NULL, sizeof (File));
	  nextLevel = (File *) LookupItem(fileItem);
	  if (!nextLevel) {
	    DBUG(("Can't allocate a new File object\n"));
	    ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
	    whereNow->fi_UseCount --;
	    goto cleanup;
	  }
	  DBUG(("Path is now %lx\n", path));
	  DBUG(("Initializing File at %lx for %s\n", nextLevel,
		fileName));
	  strncpy(nextLevel->fi_FileName, fileName,
		  FILESYSTEM_MAX_NAME_LEN);
	  nextLevel->fi_FileSystem = whereNow->fi_FileSystem;
	  nextLevel->fi_ParentDirectory = whereNow;
	  if (remoteDir.de_UniqueIdentifier == 0) {
	    nextLevel->fi_UniqueIdentifier = fileFolio->ff_NextUniqueID --;
	  } else {
	    nextLevel->fi_UniqueIdentifier = remoteDir.de_UniqueIdentifier;
	  }
	  nextLevel->fi_Flags = remoteDir.de_Flags;
	  nextLevel->fi_Burst = remoteDir.de_Burst;
	  nextLevel->fi_Gap = remoteDir.de_Gap;
	  nextLevel->fi_BlockSize = remoteDir.de_BlockSize;
	  nextLevel->fi_BlockCount = remoteDir.de_BlockCount;
	  nextLevel->fi_ByteCount = remoteDir.de_ByteCount;
	  nextLevel->fi_LastAvatarIndex = remoteDir.de_AvatarCount - 1;
	  nextLevel->fi_AvatarList[0] = remoteDir.de_Location;
	  nextLevel->fi_Type = remoteDir.de_Type;
	  nextLevel->fi_FileSystemBlocksPerFileBlock = 1;
	  goto built;
	}
      rawIOReq->io_Info.ioi_Offset ++;
      }
    }
#ifdef DOCACHE
    if (openFile->ofi_File->fi_Flags & FILE_BLOCKS_CACHED) {
      doCache = cachePass = TRUE;
    } else {
      doCache = cachePass = FALSE;
    }
  scanLoop:
#endif
    blockNum = 0;
    openFile->ofi_NextBlock = 0;
    offset = 0;
    compare = 2;
    while (1) {
      DBUG(("Path is now %lx\n", path));
      DBUG(("OpenPath reading block %d\n", blockNum));
      while (offset == 0) {
	if (blockNum < 0 || blockNum >= whereNow->fi_BlockCount) {
	  goto doneSearch; /* fail */
	}
#ifdef PARANOID
	scanTries = 5;
      rescan:
	openFile->ofi_NextBlock = blockNum;
#endif
#ifdef DOCACHE
	entry = NULL;
	if (doCache) {
	  DBUG(("Probe for block %d in cache\n", blockNum));
	  entry = FindCache(openFile, blockNum);
	  if (entry) {
	    dirPage = (char *) entry->ioce_CachedBlock;
	    offset = (int32) ((DirectoryHeader *) dirPage)->dh_FirstEntryOffset;
	    DBUG(("Found it, data at 0x%x\n", dirPage));
	    goto scanPage;
	  }
	  DBUG(("Not found\n"));
	  if (cachePass) {
	    blockNum ++;
	    continue;
	  }
	}
#endif
	if ((errcode = internalReadNextBlock(openFile, 1)) != 0) {
#ifdef PARANOID
	  if (--scanTries > 0) {
	    DBUG(("Directory I/O error, rereading\n"));
	    offset = 0;
	    goto rescan;
	  }
#endif
	  ffPrivate->fftd_ErrorCode = errcode;
	  goto cleanup;
	}
	dirPage = buffer;
	offset = (int32) ((DirectoryHeader *) dirPage)->dh_FirstEntryOffset;
      }
#ifdef DOCACHE
    scanPage: ;
#endif
      do {
#ifdef PARANOID
	if (offset % 4 != 0) {
	  if (--scanTries > 0) {
	    DBUG(("Directory glitch, rereading\n"));
	    DBUG(("Prev block %d, next block %d\n", 
		  ((DirectoryHeader *) dirPage)->dh_PrevBlock,
		  ((DirectoryHeader *) dirPage)->dh_NextBlock));
	    DBUG(("First entry offset 0x%x\n",
		  (int32) ((DirectoryHeader *) dirPage)->dh_FirstEntryOffset));
	    offset = 0;
	    goto rescan;
	  }
	  DBUG(("Rereads exhausted\n"));
	  goto doneSearch;
	}
#endif	
	point = dirPage + offset;
	dirRec = (DirectoryRecord *) point;
	DBUG(("Path is now %lx\n", path));
	DBUG(("Look for %s at %s\n", fileName, dirRec->dir_FileName));
	compare = strcasecmp(fileName, dirRec->dir_FileName);
	if (compare == 0) {
#ifdef DOCACHE
	  if (doCache) {
	    if (entry) {
	      DBUG(("Cache hit!  Move to head of list\n"));
	      entry->ioce.n_Priority = CACHE_PRIO_HIT;
	      RemNode((Node *) entry);
	      AddHead(&fsCacheList, (Node *) entry);
	    } else {
	      DBUG(("Cache hit!  Submitting block %d to cache\n", blockNum));
	      AddCacheEntry(openFile, blockNum, dirPage, CACHE_PRIO_HIT,
			    CACHE_OPERA_DIRECTORY,
			    openFile->ofi_File->fi_BlockSize);
	    }
	  }
#endif
	  goto doneSearch;
	}
	if (dirRec->dir_Flags & DIRECTORY_LAST_IN_BLOCK) {
	  offset = 0;
#ifdef DOCACHE
	  if (cachePass) {
	    blockNum ++;
	  } else {
	    if (doCache && !entry) {
	      DBUG(("Cache miss. Submitting block %d to cache\n", blockNum));
	      AddCacheEntry(openFile, blockNum, dirPage, CACHE_PRIO_MISS,
			    CACHE_OPERA_DIRECTORY,
			    openFile->ofi_File->fi_BlockSize);
	    }
	    blockNum = openFile->ofi_NextBlock =
	      ((DirectoryHeader *) dirPage)->dh_NextBlock;
	    if (dirRec->dir_Flags & DIRECTORY_LAST_IN_DIR) {
	      goto doneSearch;
	    }
	  }
#else
	  blockNum = openFile->ofi_NextBlock =
	    ((DirectoryHeader *) dirPage)->dh_NextBlock;
	  if (dirRec->dir_Flags & DIRECTORY_LAST_IN_DIR) {
	    goto doneSearch;
	  }
#endif
	} else {
	  offset += sizeof (DirectoryRecord) +
	    (sizeof (ulong) *
	     (int32) dirRec->dir_LastAvatarIndex);
	}
      } while (offset != 0);
    }
  doneSearch:
    if (compare != 0) {
#ifdef DOCACHE
      if (cachePass) {
	DBUG(("Cache pass failed, ageing entries\n"));
	AgeCache();
	cachePass = FALSE;
	goto scanLoop;
      }
#endif
      ffPrivate->fftd_ErrorCode =
	MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NoFile);
      DBUG(("File not found: %s\n", fullPath));
      DBUG(("Error generated: %x\n", ffPrivate->fftd_ErrorCode));
      goto tryAlternate;
    }
    DBUG(("Found it!\n"));
    DBUG(("Path is now %lx\n", path));
    fileSize = sizeof(File) +
      sizeof (ulong) * (int32) dirRec->dir_LastAvatarIndex;
    whereNow->fi_UseCount ++;
    fileItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILENODE),
				    (void *) NULL, fileSize);
    nextLevel = (File *) LookupItem(fileItem);
    if (!nextLevel) {
      DBUG(("Can't allocate a new File object\n"));
      ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
      whereNow->fi_UseCount --;
      goto cleanup;
    }
    DBUG(("Path is now %lx\n", path));
    DBUG(("Initializing File at %lx for %s\n", nextLevel,
		 dirRec->dir_FileName));
    strncpy(nextLevel->fi_FileName, dirRec->dir_FileName,
	    FILESYSTEM_MAX_NAME_LEN);
    nextLevel->fi_ParentDirectory = whereNow;
    nextLevel->fi_UniqueIdentifier = dirRec->dir_UniqueIdentifier;
    nextLevel->fi_FileSystem = whereNow->fi_FileSystem;
    nextLevel->fi_Flags = dirRec->dir_Flags;
#ifdef DOCACHE
    if ((nextLevel->fi_Flags & FILE_IS_DIRECTORY) &&
	(nextLevel->fi_FileSystem->fs_Flags & FILESYSTEM_CACHEWORTHY)) {
      nextLevel->fi_Flags |= FILE_BLOCKS_CACHED;
    } else {
      nextLevel->fi_Flags &= ~FILE_BLOCKS_CACHED;
    }
#endif
    nextLevel->fi_BlockSize = dirRec->dir_BlockSize;
    nextLevel->fi_FileSystemBlocksPerFileBlock = nextLevel->fi_BlockSize /
      nextLevel->fi_FileSystem->fs_VolumeBlockSize;
    nextLevel->fi_ByteCount = dirRec->dir_ByteCount;
    nextLevel->fi_BlockCount = dirRec->dir_BlockCount;
#ifdef ALWAYSBURST
    nextLevel->fi_Burst = 1;
#else
    nextLevel->fi_Burst = dirRec->dir_Burst;
#endif
    nextLevel->fi_Gap = dirRec->dir_Gap;
    nextLevel->fi_LastAvatarIndex = dirRec->dir_LastAvatarIndex;
    for (avatarIndex = (int32) nextLevel->fi_LastAvatarIndex;
	 avatarIndex >= 0;
	 avatarIndex --) {
      nextLevel->fi_AvatarList[avatarIndex] =
	dirRec->dir_AvatarList[avatarIndex];
    }
  built:
    /*
       Kick the parent with an OPENENTRY command before adding the file
       to the list.  This gives the parent a chance to do (or start up)
       final opening/initialization of the file, and if necessary to set
       the "not ready yet" flag.
    */
    rawIOReq->io_Info.ioi_Command = FILECMD_OPENENTRY;
    rawIOReq->io_Info.ioi_Flags = 0;
    rawIOReq->io_Info.ioi_Send.iob_Buffer = 
      rawIOReq->io_Info.ioi_Recv.iob_Buffer = NULL;
    rawIOReq->io_Info.ioi_Send.iob_Len = 
      rawIOReq->io_Info.ioi_Recv.iob_Len = 
	rawIOReq->io_Info.ioi_Offset = 0;
    rawIOReq->io_Info.ioi_CmdOptions = fileItem;
    errcode = SuperInternalDoIO(rawIOReq);
    if (errcode < 0 || (errcode = rawIOReq->io_Error) < 0) {
      DBUG(("DoIO error %x doing FILECMD_OPENENTRY\n", errcode));
      ffPrivate->fftd_ErrorCode = errcode;
      goto cleanup;
    }
#ifdef	FS_DIRSEMA
    InitDirSema(nextLevel, 1);
#endif	/* FS_DIRSEMA */
    AddHead(&fileFolio->ff_Files, (Node *) nextLevel);
    UnlockDirSema(whereNow, "added to cache");
    GiveDaemon(nextLevel);  /* Transfer ownership of all File items! */
    whereNow = nextLevel;
    DBUG(("Changing levels, path is now %lx\n", path));
    (void) SuperCloseItem(openFile->ofi.dev.n_Item);
    (void) SuperDeleteItem(openFile->ofi.dev.n_Item);
    buffer = (char *) NULL;
    openFile = (OpenFile *) NULL;
    DBUG(("Path is now %lx\n", path));
  }
  if (mode == CreateNewEntry && !triedToCreate) {
    ffPrivate->fftd_ErrorCode =
      MakeFErr(ER_SEVER,ER_C_NSTND,ER_FS_DuplicateFile);
    return (File *) NULL;
  }
  if (whereNow) {
    ++ whereNow->fi_UseCount;
  }
  if (punt) {
    -- punt->fi_UseCount;
  }
  if (expandedPath) {
    FreeMem(expandedPath, expandedPathLen);
  }
  return whereNow;
 tryAlternate:
  DBUG(("Scan failure\n"));
  if (rescan) {
    DBUG(("Try rescanning from %s\n", rescan));
    path = rescan;
    do {
      c = *path++;
      if (c == '\0' || c == '{') {
	goto trueFailure;
      }
    } while (c != '|' && c != '}');
    rescan = path;
    insideAlternative = TRUE;
    DBUG(("Rescan point advanced to %s\n", rescan));
    UnlockDirSema(whereNow, "tryAlternate");
    whereNow = punt;
    goto theGrandLoop;
  }
 cleanup:
  if (mode == CreateNewEntry && *path == '\0' && openFile && rawIOReq &&
      !triedToCreate) {
    DBUG(("Try to create a new entry\n"));
    triedToCreate = TRUE;
    rawIOReq->io_Info.ioi_Command = FILECMD_ADDENTRY;
    rawIOReq->io_Info.ioi_Flags = 0;
    rawIOReq->io_Info.ioi_Send.iob_Buffer = (void *) fileName;
    rawIOReq->io_Info.ioi_Send.iob_Len = strlen(fileName);
    rawIOReq->io_Info.ioi_Recv.iob_Buffer = NULL;
    rawIOReq->io_Info.ioi_Recv.iob_Len = 0;
    rawIOReq->io_Info.ioi_Offset = 0;
    errcode = SuperInternalDoIO(rawIOReq);
    if (errcode < 0 || (errcode = rawIOReq->io_Error) < 0) {
      DBUG(("WaitIO error %x adding entry\n", errcode));
      ffPrivate->fftd_ErrorCode = errcode;
      goto trueFailure;
    }
    DBUG(("Entry created, go back and find it in the usual way\n"));
    goto probeForIt;
  }
 trueFailure:
  UnlockDirSema(whereNow, "trueFailure");
  if (punt) {
    punt->fi_UseCount --;
  }
  if (openFile) {
    (void) SuperCloseItem(openFile->ofi.dev.n_Item);
    (void) SuperDeleteItem(openFile->ofi.dev.n_Item);
  } else {
    if (buffer) {
      ReleaseDirectoryBuffer(buffer, bufferSize);
    }
  }
  if (expandedPath) {
    FreeMem(expandedPath, expandedPathLen);
  }
  return (File *) NULL;
}

OpenFile *OpenPath (File *startingDirectory, char *path)
{
  File *firstFile, *secondFile, *firstLinkp, *secondLinkp;
  File *resFile = (File *) NULL;
  OpenFile *openFile;
  char linkpath[FILESYSTEM_PART_PATH_LEN];
  uint32 findx;

  DBUG(("In OpenPath, starting directory %x\n", startingDirectory));
  if (startingDirectory == (File *) NULL) {
    startingDirectory = fileFolio->ff_Root;
  }
  firstFile = FindFile(startingDirectory, path, FindExistingEntry,
		       &firstLinkp, linkpath);
  DBUG(("OpenPath, outside find: 0x%x (%s)\n", linkpath, linkpath));
  if (firstLinkp) {	/* encountered a link */
    findx = firstLinkp->fi_Linfo->li_FuncIndx;
    if (firstLinkp->fi_Linfo->li_PreLink) {
      secondLinkp = firstLinkp;
      secondFile = firstFile;
      DBUG(("OpenPath (second): firstLinkp: 0x%x, name: %s, flags: 0x%x\n", firstLinkp, firstLinkp->fi_FileName, firstLinkp->fi_Flags));
      DBUG(("OpenPath (linfo): Next: 0x%x, Pre: 0x%x, func: %d\n", firstLinkp->fi_Linfo->li_NextLink, firstLinkp->fi_Linfo->li_PreLink, firstLinkp->fi_Linfo->li_FuncIndx));
      firstFile = FindFile(secondLinkp->fi_Linfo->li_PreLink, linkpath,
			   FindExistingEntry, &firstLinkp, NULL);
      firstLinkp = secondLinkp->fi_Linfo->li_PreLink;
    } else {
      DBUG(("OpenPath (first): firstLinkp: 0x%x, name: %s, flags: 0x%x\n", firstLinkp, firstLinkp->fi_FileName, firstLinkp->fi_Flags));
      DBUG(("OpenPath (linfo): Next: 0x%x, Pre: 0x%x, func: %d\n", firstLinkp->fi_Linfo->li_NextLink, firstLinkp->fi_Linfo->li_PreLink, firstLinkp->fi_Linfo->li_FuncIndx));
      secondFile = FindFile(firstLinkp->fi_Linfo->li_NextLink, linkpath,
			    FindExistingEntry, &secondLinkp, NULL);
      secondLinkp = firstLinkp->fi_Linfo->li_NextLink;
    }

    resFile = (File *)
	      (*fileFolio->ff_LinkFuncs[findx]) (firstFile, secondFile,
						 firstLinkp, secondLinkp);
    if (firstFile && (resFile != firstFile))
      firstFile->fi_UseCount--;
    if (secondFile && (resFile != secondFile))
      secondFile->fi_UseCount--;
  } else {		/* no link */
    resFile = firstFile;
  }

  if (!resFile) {
    return (OpenFile *) NULL;
  }
  openFile = OpenAFile(resFile);
  resFile->fi_UseCount --;
  DBUG(("Returning open-file %d\n", openFile));
  return openFile;
}

int32 internalReadNextBlock(OpenFile *theOpenFile, int32 blockCount)
{
  IOReq *theRequest;
  File *theFile;
  int32 errcode;
  theRequest = theOpenFile->ofi_InternalIOReq;
  if (!theRequest) {
    DBUG(("No internal IOReq available\n"));
    return 1;
  }
  DBUG(("internalReadNextBlock, block %d, buffer 0x%x\n",
	theOpenFile->ofi_NextBlock,
	theOpenFile->ofi_RegisteredBuffer));
  theFile = theOpenFile->ofi_File;
  theRequest->io_Info.ioi_Command = CMD_READ;
  theRequest->io_Info.ioi_Flags = 0;
  theRequest->io_Info.ioi_Offset = (int32) theOpenFile->ofi_NextBlock;
  theRequest->io_Info.ioi_Send.iob_Buffer = NULL;
  theRequest->io_Info.ioi_Send.iob_Len = 0;
  theRequest->io_Info.ioi_Recv.iob_Buffer = theOpenFile->ofi_RegisteredBuffer;
  theRequest->io_Info.ioi_Recv.iob_Len =
    (int32) (blockCount * theFile->fi_BlockSize);
  theRequest->io_Actual = -1;
  theRequest->io_CallBack = NULL;
  errcode = SuperInternalDoIO(theRequest);
  DBUG(("internalReadNextBlock read done 0x%x\n", errcode));
  if (errcode < 0 || (errcode = theRequest->io_Error) < 0) {
    DBUG(("DoIO error %x within internalReadNextBlock\n", errcode));
    return errcode;
  }
  theOpenFile->ofi_NextBlock += blockCount;
#ifdef DEBUG
  Superkprintf("internalReadNextBlock code %x, %d bytes\n",
	       theRequest->io_Error, theRequest->io_Actual);
#endif
  return errcode;
}

int32 internalGetPath(File *theFile, char *theBuffer, int32 bufLen)
{
  int32 nameLen, accumLen;
  File *whereNow;
  accumLen = 1;
  whereNow = theFile;
  while (whereNow && whereNow != fileFolio->ff_Root) {
    accumLen += strlen(whereNow->fi_FileName) + 1;
    whereNow = whereNow->fi_ParentDirectory;
  }
  if (accumLen > bufLen) {
    return -1;
  }
  theBuffer[--accumLen] = '\0';
  whereNow = theFile;
  while (whereNow && whereNow != fileFolio->ff_Root) {
    nameLen = strlen(whereNow->fi_FileName);
    accumLen -= nameLen;
    strncpy(&theBuffer[accumLen], whereNow->fi_FileName, nameLen);
    theBuffer[--accumLen] = '/';
    whereNow = whereNow->fi_ParentDirectory;
  }
  if (accumLen != 0) {
    DBUG(("Whoops, internalGetPath offset wrong by %d\n", accumLen));
  }
  return 0;
}

#ifdef DOCACHE

IoCacheEntry *FindCache(OpenFile *theOpenFile, int32 blockNum)
{
  IoCacheEntry *entry;
  uint32 filesystemIdentifier, fileIdentifier;
  if (!fsCacheBase) {
    return NULL;
  }
  fileIdentifier = theOpenFile->ofi_File->fi_UniqueIdentifier;
  filesystemIdentifier = theOpenFile->ofi_File->fi_FileSystem->fs_VolumeUniqueIdentifier;
  entry = (IoCacheEntry *) FirstNode(&fsCacheList);
  while (IsNode(&fsCacheList, entry)) {
    DBUG(("Check entry at 0x%x prio %d\n", entry, entry->ioce.n_Priority));
    if (entry->ioce_FilesystemUniqueIdentifier == filesystemIdentifier &&
	entry->ioce_FileUniqueIdentifier == fileIdentifier &&
	entry->ioce_FileBlockOffset == blockNum) {
      return entry;
    }
    entry = (IoCacheEntry *) NextNode(entry);
  }
  return NULL;
}

IoCacheEntry *AddCacheEntry(OpenFile *theOpenFile, int32 blockNum,
			    void *buffer,
			    uint8 priority, uint32 format,
			    uint32 bytes)
{
  IoCacheEntry *entry;
  if (!fsCacheBase) {
    return NULL;
  }
  if (bytes > FILESYSTEM_DEFAULT_BLOCKSIZE) {
    return NULL;
  }
  entry = (IoCacheEntry *) LastNode(&fsCacheList);
  if (entry->ioce.n_Priority > priority) {
    return NULL;
  }
  RemNode((Node *) entry);
  entry->ioce_FilesystemUniqueIdentifier =
    theOpenFile->ofi_File->fi_FileSystem->fs_VolumeUniqueIdentifier;
  entry->ioce_FileUniqueIdentifier =
    theOpenFile->ofi_File->fi_UniqueIdentifier;
  entry->ioce_FileBlockOffset = blockNum;
  entry->ioce_CacheFormat = format;
  entry->ioce_CacheMiscValue = 0;
  entry->ioce_CachedBlockSize = bytes;
  entry->ioce.n_Priority = priority;
  memcpy(entry->ioce_CachedBlock, buffer, FILESYSTEM_DEFAULT_BLOCKSIZE);
/*
   Use InsertNodeFromHead when it works
*/
  DBUG(("Insert entry 0x%x priority %d\n", entry, entry->ioce.n_Priority));
  InsertNodeFromTail(&fsCacheList, (Node *) entry);
  return entry;
}

void AgeCache(void)
{
  IoCacheEntry *entry;
  if (!fsCacheBase) {
    return;
  }
  entry = (IoCacheEntry *) FirstNode(&fsCacheList);
  while (IsNode(&fsCacheList, entry)) {
    if (entry->ioce.n_Priority == 0) {
      return;
    }
    entry->ioce.n_Priority --;
    entry = (IoCacheEntry *) NextNode(entry);
  }
}

#endif

void
adjustLinkInfo(File *here, FileFolioTaskData *ftdp, File **linkFile, char *afterLinkPath)
{
  char *cp;

  DBUG(("adjustLinkInfo: here [%s], linkFile [%s], ", here->fi_FileName, (*linkFile)->fi_FileName));
  DBUG(("afterLinkPath [%s], pwd [%s]\n", afterLinkPath, ftdp->fftd_LinkPartPath));
  if (here == ftdp->fftd_LinkDirectory) {
    *linkFile = (File *) NULL;
    *afterLinkPath = (char) 0;
    DBUG(("adjustLinkInfo: cleared pwd\n"));
    return;
  }
  cp = afterLinkPath;
  while (*cp)
    cp++;
  while((*cp != '/') && (cp != afterLinkPath))
    cp--;
  if (cp == afterLinkPath) {
    DBUG(("adjustLinkInfo: Reached to the beginning of path [%s], [%s]\n", afterLinkPath, ftdp->fftd_LinkPartPath));
  }
  DBUG(("adjustLinkInfo: cp [%c] 0x%x, ", cp, *cp, ftdp->fftd_LinkPartPath));
  *cp = (char) 0;
  DBUG(("path adjusted to [%s]\n", afterLinkPath));
  return;
}


File *
preferFirst(File *firstFile, File *secondFile, File *firstLinkp,
	    File *secondLinkp)
{
  DBUG(("preferFirst: 0x%x(%s), ", firstFile, firstFile->fi_FileName));
  DBUG(("0x%x(%s), ", secondFile, secondFile->fi_FileName));
  DBUG(("0x%x(%s), ", firstLinkp, firstLinkp->fi_FileName));
  DBUG(("0x%x(%s)\n", secondLinkp, secondLinkp->fi_FileName));
  if (firstFile)
    return firstFile;

  return secondFile;
}


File *
hideFirst(File *firstFile, File *secondFile, File *firstLinkp,
	    File *secondLinkp)
{
  DBUG(("preferFirst: 0x%x(%s), ", firstFile, firstFile->fi_FileName));
  DBUG(("0x%x(%s), ", secondFile, secondFile->fi_FileName));
  DBUG(("0x%x(%s), ", firstLinkp, firstLinkp->fi_FileName));
  DBUG(("0x%x(%s)\n", secondLinkp, secondLinkp->fi_FileName));

  return secondFile;
}


/**********************
 SWI handler for OpenFile call
 **********************/

Item OpenFileSWI(char *thePath)
{
  OpenFile *theOpenFile;
  FileFolioTaskData *ffPrivate;
  DBUG(("OpenFile %s\n", thePath));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  theOpenFile = OpenPath(ffPrivate->fftd_CurrentDirectory, thePath);
  if (theOpenFile) {
    return theOpenFile->ofi.dev.n_Item;
  } else {
    DBUG(("File not opened, error code %x\n", ffPrivate->fftd_ErrorCode));
    return (Item) ffPrivate->fftd_ErrorCode;
  }
}

/**********************
 SWI handler for OpenFileInDir call
 **********************/

Item OpenFileInDirSWI(Item baseDirItem, char *thePath)
{
#ifdef FS_OLDSWI
  Superkprintf("PANIC: OpenFileInDirSWI not Supported\n");
  return MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
#else /* FS_OLDSWI */
  File *theBaseDir;
  OpenFile *theOpenFile;
  Node *theBaseNode;
  FileFolioTaskData *ffPrivate;
  char fullPath[FILESYSTEM_MAX_PATH_LEN];
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  DBUG(("OpenFileInDir, base item %d, path %s\n", baseDirItem, thePath));
  theBaseNode = (Node *) LookupItem(baseDirItem);
  if (theBaseNode->n_SubsysType == FILEFOLIO &&
      theBaseNode->n_Type == FILENODE) {
    theBaseDir = (File *) theBaseNode;
  } else if (theBaseNode->n_SubsysType == KERNELNODE &&
	    theBaseNode->n_Type == DEVICENODE &&
	    ((Device *) theBaseNode)->dev_Driver == fileDriver &&
	    ((OpenFile *) theBaseNode)->ofi_DeviceType ==
	    FILE_DEVICE_OPENFILE) {
    theBaseDir = ((OpenFile *) theBaseNode)->ofi_File;
  } else {
    DBUG(("Item %d is neither a File nor an OpenFile\n", baseDirItem));
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
  }
  if ((theBaseDir->fi_Flags & FILE_IS_DIRECTORY) == 0) {
    DBUG(("%s is a file, not a directory\n", theBaseDir->fi_FileName));
    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
  }
  DBUG(("OpenFileInDirSWI, base path %s, path %s\n", theBaseDir->fi_FileName, thePath));

  (void) internalGetPath(theBaseDir, fullPath, (FILESYSTEM_MAX_PATH_LEN - 2 - strlen(thePath)));
  strcat(fullPath, "/");
  strcat(fullPath, thePath);
  DBUG(("fullPath: [%s]\n", fullPath));
  theOpenFile = OpenPath(fileFolio->ff_Root, fullPath);
  if (theOpenFile) {
    return theOpenFile->ofi.dev.n_Item;
  } else {
    DBUG(("File not opened, error code %x\n", ffPrivate->fftd_ErrorCode));
    return (Item) ffPrivate->fftd_ErrorCode;
  }
#endif /* FS_OLDSWI */
}

/**********************
 SWI handler for CloseFile call
 **********************/

int32 CloseOpenFileSWI(Item openFileItem)
{
  OpenFile *theOpenFile;
#ifndef PRODUCTION
  Err err;
#endif
  SetupFileFolioTaskData();
  theOpenFile = (OpenFile *) CheckItem(openFileItem, KERNELNODE, DEVICENODE);
  if (!theOpenFile ||
      theOpenFile->ofi.dev_Driver != fileDriver ||
      theOpenFile->ofi_DeviceType != FILE_DEVICE_OPENFILE) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
  }
#ifndef PRODUCTION
  err = SuperCloseItem(theOpenFile->ofi.dev.n_Item);
  DBUG(("SuperCloseItem returns 0x%x\n", err));
  if (err < 0) {
    DBUG(("SuperCloseItem failed in CloseOpenFile!\n"));
    MySysErr(err);
  }
  err = SuperDeleteItem(theOpenFile->ofi.dev.n_Item);
  DBUG(("SuperDeleteItem returns 0x%x\n", err));
  if (err < 0) {
    DBUG(("SuperDeleteItem failed in CloseOpenFile!\n"));
    MySysErr(err);
  }
  return err;
#else
  (void) SuperCloseItem(theOpenFile->ofi.dev.n_Item);
  return SuperDeleteItem(theOpenFile->ofi.dev.n_Item);
#endif
}

/**********************
 SWI handler for ChangeDirectory SWI
 **********************/

Item ChangeDirectorySWI(char *path)
{
  File *theDirectory, *linkfp;
  FileFolioTaskData *ffPrivate;
  char linkpath[FILESYSTEM_PART_PATH_LEN];
#ifdef WATCHDIRECTORY
  char foo[FILESYSTEM_MAX_PATH_LEN];
#endif
  DBUG(("ChangeDirectory to %s\n", path));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  theDirectory = FindFile(ffPrivate->fftd_CurrentDirectory, path,
			  UpdateCurrentDirectory, &linkfp, linkpath);
/********* Hey dude, check to see it's a directory please! */
  if (theDirectory) {
    if (!(theDirectory->fi_Flags & FILE_IS_DIRECTORY)) {
	return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
    }
    if (linkfp) {
      DBUG(("ChangeDirectory: LINK pwd  [%s], [%s], [%s]\n", linkfp->fi_FileName, linkpath, path));
      if (!ffPrivate->fftd_LinkDirectory) {
	if((ffPrivate->fftd_LinkPartPath = (char *)
	     AllocMem(FILESYSTEM_PART_PATH_LEN, MEMTYPE_FILL)) == NULL) {
	  ffPrivate->fftd_ErrorCode = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
    	  DBUG(("Task 0x%x failed to alloc for linkpath %s\n",
		 CURRENTTASK->t.n_Item, linkpath));
		return (Item) ffPrivate->fftd_ErrorCode;
	}
    	DBUG(("Task 0x%x alloc for linkpath %s\n",
		 CURRENTTASK->t.n_Item, linkpath));
      }
      strcpy(ffPrivate->fftd_LinkPartPath, linkpath);
      ffPrivate->fftd_LinkDirectory = linkfp;
    } else {
      if (ffPrivate->fftd_LinkDirectory) {
    	DBUG(("Task 0x%x Free for linkpath %s\n",
		 CURRENTTASK->t.n_Item, ffPrivate->fftd_LinkPartPath));
	FreeMem(ffPrivate->fftd_LinkPartPath, FILESYSTEM_PART_PATH_LEN);
	ffPrivate->fftd_LinkDirectory = (File *) NULL;
	ffPrivate->fftd_LinkPartPath = (char *) NULL;
      }
    }
    ffPrivate->fftd_CurrentDirectory->fi_UseCount --;
    ffPrivate->fftd_CurrentDirectory = theDirectory;
#ifdef WATCHDIRECTORY
    (void) internalGetPath(ffPrivate->fftd_CurrentDirectory, foo, FILESYSTEM_MAX_PATH_LEN);
    DBUG(("Task 0x%x directory changed to %s\n", CURRENTTASK->t.n_Item, foo));
#endif
    return theDirectory->fi.n_Item;
  } else {
    DBUG(("Cannot ChangeDirectory to %s\n", path));
    return (Item) ffPrivate->fftd_ErrorCode;
  }
}

/**********************
 SWI handler for GetDirectory SWI
 **********************/

Item GetDirectorySWI(char *path, int32 pathLen)
{
  FileFolioTaskData *ffPrivate;
  int32 err;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (path) {
    err = SuperValidateMem(KernelBase->kb_CurrentTask, path, pathLen);
    if (err) {
      return err;
    }
    err = internalGetPath(ffPrivate->fftd_CurrentDirectory, path, pathLen);
    if (err < 0) {
      return err;
    }
  }
  DBUG(("GetDirectorySWI: link [%s], path [%s]\n", ffPrivate->fftd_LinkDirectory->fi_FileName, ffPrivate->fftd_LinkPartPath));
  return ffPrivate->fftd_CurrentDirectory->fi.n_Item;
}

/**********************
 SWI handler for CreateFile SWI
 **********************/

Item CreateFileSWI(char *path)
{
  File *whereNow, *linkp;
  FileFolioTaskData *ffPrivate;

  DBUG(("In CreateFile\n"));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  whereNow = FindFile(ffPrivate->fftd_CurrentDirectory, path, CreateNewEntry,
		      &linkp, NULL);
  if (!whereNow) {
    return ffPrivate->fftd_ErrorCode;
  }
  whereNow->fi_UseCount --;
  return whereNow->fi.n_Item;
}

/**********************
 SWI handler for CreateAlias SWI
 **********************/

Item CreateAliasSWI(char *aliasPath, char *originalPath)
{
  FileFolioTaskData *ffPrivate;
  TagArg aliasArgs[3];
  Item aliasItem;
  Alias *alias, *oldAlias;
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (!aliasPath || !originalPath ||
      strlen(aliasPath) > ALIAS_NAME_MAX_LEN ||
      strlen(originalPath) > ALIAS_VALUE_MAX_LEN) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadName);
  }
  memcpy(aliasArgs, createAliasArgs, sizeof aliasArgs);
  aliasArgs[1].ta_Arg = aliasPath;
  aliasItem = SuperCreateSizedItem(MKNODEID(FILEFOLIO,FILEALIASNODE),
				   aliasArgs,
				   sizeof (Alias) + strlen(originalPath) + 1);
  alias = (Alias *) LookupItem(aliasItem);
  if (!alias) {
    return aliasItem;
  }
  alias->a_Value = sizeof alias->a_Value + (char *) &alias->a_Value;
  strcpy(alias->a_Value, originalPath);
  oldAlias = (Alias *) FindNamedNode(&ffPrivate->fftd_AliasList, aliasPath);
  if (oldAlias) {
    SuperDeleteItem(oldAlias->a.n_Item); /* this does a RemNode */
  }
  AddTail(&ffPrivate->fftd_AliasList, (Node *) alias);
  DBUG(("Added alias %s\n", alias->a.n_Name));
  return aliasItem;
}

/**********************
 SWI handler for DeleteFile SWI
 **********************/

Err DeleteFileSWI(char *path)
{
  File *whereNow, *linkp;
  OpenFile *parent;
  IOReq *rawIOReq;
  Item ioReqItem;
  FileFolioTaskData *ffPrivate;
  TagArg ioReqTags[2];
  Err errcode;

  DBUG(("In DeleteFile\n"));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  whereNow = FindFile(ffPrivate->fftd_CurrentDirectory, path,
		      DeleteExistingEntry, &linkp, NULL);
  if (!whereNow) {
    return ffPrivate->fftd_ErrorCode;
  }
  DBUG(("Found existing entry for %s, use count is %d\n", whereNow->fi_FileName, whereNow->fi_UseCount));
  if (whereNow->fi_UseCount > 1) {
    ffPrivate->fftd_ErrorCode =	MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_Busy);
    goto bail;
  }
  if (whereNow->fi_ParentDirectory->fi_Flags & FILE_IS_READONLY) {
    DBUG(("Can't delete, read-only directory\n"));
    ffPrivate->fftd_ErrorCode =	MakeFErr(ER_SEVER,ER_C_NSTND,ER_FS_ReadOnly);
    whereNow->fi_UseCount --;
    return ffPrivate->fftd_ErrorCode;
  }
  parent = OpenAFile(whereNow->fi_ParentDirectory);
  if (!parent) {
    goto bail;
  }
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) parent->ofi.dev.n_Item;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    (void) SuperCloseItem(parent->ofi.dev.n_Item);
    (void) SuperDeleteItem(parent->ofi.dev.n_Item);
    ffPrivate->fftd_ErrorCode = ioReqItem;
    goto bail;
  }
  rawIOReq = (IOReq *) LookupItem(ioReqItem);
  rawIOReq->io_Info.ioi_Command = FILECMD_DELETEENTRY;
  rawIOReq->io_Info.ioi_Flags = 0;
  rawIOReq->io_Info.ioi_Send.iob_Buffer = (void *) whereNow->fi_FileName;
  rawIOReq->io_Info.ioi_Send.iob_Len = strlen(whereNow->fi_FileName);
  rawIOReq->io_Info.ioi_Recv.iob_Buffer = NULL;
  rawIOReq->io_Info.ioi_Recv.iob_Len = 0;
  rawIOReq->io_Info.ioi_Offset = 0;
  errcode = SuperInternalDoIO(rawIOReq);
  if (errcode < 0 || (errcode = rawIOReq->io_Error) < 0) {
    DBUG(("WaitIO error %x deleting entry\n", errcode));
    ffPrivate->fftd_ErrorCode = errcode;
    goto wrapup;
  }
  whereNow->fi_UseCount --;
  RemNode((Node *) whereNow);
  SuperCloseItem(whereNow->fi.n_Item);
  SuperInternalDeleteItem(whereNow->fi.n_Item);
  ffPrivate->fftd_ErrorCode = 0;
 wrapup:
  SuperDeleteItem(ioReqItem);
  SuperCloseItem(parent->ofi.dev.n_Item);
  SuperDeleteItem(parent->ofi.dev.n_Item);
  return ffPrivate->fftd_ErrorCode;
 bail:
  whereNow->fi_UseCount --;
  return ffPrivate->fftd_ErrorCode;
}
 
/**********************
 SWI handler for LoadOverlay SWI
 **********************/
 
void * LoadOverlaySWI(char *path)
{
  return (void *) MakeFErr(ER_SEVER,ER_C_STND,ER_NotSupported);
}


/**********************
 SWI handler for CreateLink SWI
 **********************/
Err
CreateLinkSWI(char *firstPath, char *secondPath, uint32 funci)
{
  FileFolioTaskData *ffPrivate;
  File *firstFile, *secondFile, *linkfp;
  FSLinkInfo *linkip;
  Err err;

  firstFile = NULL;
  secondFile = NULL;
  DBUG(("CreateLinkSWI %s, %s, %d\n", firstPath, secondPath, funci));
  ffPrivate = SetupFileFolioTaskData();
  if (!ffPrivate) {
  	return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  
  if ((int) funci < 0 || funci >= FSLK_TOT_FUNCS ||
      !firstPath || !secondPath)
	return MakeFErr(ER_SEVER,ER_C_STND,ER_BadPtr);

  firstFile = FindFile(ffPrivate->fftd_CurrentDirectory, firstPath,
		       FindExistingEntry, &linkfp, NULL);
  if (!firstFile)
  	return ffPrivate->fftd_ErrorCode;
  DBUG(("LinkSWI: linkfp: 0x%x, name: %s, flags: 0x%x\n", linkfp, linkfp->fi_FileName, linkfp->fi_Flags));
  if ((firstFile->fi_Flags & FILE_HAS_LINK) || linkfp) {
    err = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_DuplicateLink);
    goto bail;
  }

  secondFile = FindFile(ffPrivate->fftd_CurrentDirectory, secondPath,
			FindExistingEntry, &linkfp, NULL);
  if (!secondFile) {
    err = ffPrivate->fftd_ErrorCode;
    goto bail;
  }
  if ((secondFile->fi_Flags & FILE_HAS_LINK) || linkfp) {
    err = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_DuplicateLink);
    goto bail;
  }

  if (firstFile == secondFile) {
    err = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_CircularLink);
    goto bail;
  }

  if (!(firstFile->fi_Flags & FILE_IS_DIRECTORY) ||
      !(secondFile->fi_Flags & FILE_IS_DIRECTORY)) {
    err = MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
    goto bail;
  }

  firstFile->fi_Linfo = (FSLinkInfo *) AllocMem(sizeof(FSLinkInfo),
						 MEMTYPE_FILL);
  if (!firstFile->fi_Linfo) {
    err = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
    goto bail;
  }

  secondFile->fi_Linfo = (FSLinkInfo *) AllocMem(sizeof(FSLinkInfo),
						 MEMTYPE_FILL);
  if (!secondFile->fi_Linfo) {
    FreeMem(firstFile->fi_Linfo, sizeof(FSLinkInfo));
    firstFile->fi_Linfo = NULL;
    err = MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
    goto bail;
  }


  /*
   * establish the link
   */
  DBUG(("LinkSWI: File %s, %s\n", firstFile->fi_FileName, secondFile->fi_FileName));
  firstFile->fi_Flags |= FILE_HAS_LINK;
  linkip = firstFile->fi_Linfo;
  linkip->li_NextLink = secondFile;
  linkip->li_PreLink = NULL;
  linkip->li_FuncIndx = funci;

  secondFile->fi_Flags |= FILE_HAS_LINK;
  linkip = secondFile->fi_Linfo;
  linkip->li_NextLink = NULL;
  linkip->li_PreLink = firstFile;
  linkip->li_FuncIndx = funci;

  return (uint32) 0;

 bail:
  if (firstFile) {
    firstFile->fi_UseCount --;
  }
  if (secondFile) {
    secondFile->fi_UseCount --;
  }
  return err;
}

#ifdef	FS_DIRSEMA
TagArg dstags[2] = {
	TAG_ITEM_NAME, NULL,
	TAG_END,	0,
};

void
InitDirSema(File *fp, int setowner)
{
	Err	err;

	if (!fp) {
    		SEMDBUG(("InitDirSema: NULL fp\n"));
		return;
	}
	if (!(fp->fi_Flags & FILE_IS_DIRECTORY)) {
    		DBUG(("InitDirSema: not a dir [%s]\n", fp->fi_FileName));
		return;
	}
	dstags[0].ta_Arg = fp->fi_FileName;
	fp->fi_DirSema =
		SuperCreateItem(MKNODEID(KERNELNODE,SEMA4NODE), dstags);

	if (fp->fi_DirSema < 0) {
		SEMDBUG(("InitDirSema: Failed to Create sema for [%s] Sema: 0x%x, Task: 0x%x\n", fp->fi_FileName, fp->fi_DirSema, CURRENTTASK->t.n_Item));
		return;
	}

	if (setowner) {
		err = SuperSetItemOwner(fp->fi_DirSema,
			  	fileFolio->ff_Daemon.ffd_Task->t.n_Item);
		if (err < 0) {
			SEMDBUG(("InitDirSema: Failed to set sema owner for [%s] at 0x%x, owner: 0x%x, ", fp->fi_FileName, fp->fi_DirSema, CURRENTTASK->t.n_Item));
			SEMDBUG(("newowner: 0x%x, err: 0x%x\n", fileFolio->ff_Daemon.ffd_Task->t.n_Item, err));
			return;
		}
		DBUG(("InitDirSema (flg: 0x%x): Created sema (setowner 0x%x) for [%s] ", fp->fi_Flags, fileFolio->ff_Daemon.ffd_Task->t.n_Item, fp->fi_FileName));
		DBUG(("at 0x%x, Task: 0x%x\n", fp->fi_DirSema, CURRENTTASK->t.n_Item));
	} else {
		DBUG(("InitDirSema: Created sema (NO setowner) for [%s] at 0x%x, Task: 0x%x\n", fp->fi_FileName, fp->fi_DirSema, CURRENTTASK->t.n_Item));
	}
	return;
}


void
DelDirSema(File *fp)
{
	Item	ret;
#if	(defined(DEBUG) || defined(SEMDEBUG))
	ItemNode *n = (ItemNode *)LookupItem(fp->fi_DirSema);
#endif	/* DEBUG || SEMDEBUG */

	if (!fp) {
    		SEMDBUG(("DelDirSema: NULL fp\n"));
		return;
	}
	if (!(fp->fi_Flags & FILE_IS_DIRECTORY)) {
    		DBUG(("DelDirSema: not a dir [%s], [0x%x]\n", fp->fi_FileName, fp->fi_Flags));
		return;
	}
	
	if ((ret = SuperInternalDeleteItem(fp->fi_DirSema)) < 0) {
		SEMDBUG(("DelDirSema: Failed to Delete sema for [%s] at 0x%x, myTask: 0x%x, ", fp->fi_FileName, fp->fi_DirSema, CURRENTTASK->t.n_Item));
		SEMDBUG(("OwnerTask: 0x%x, ret: 0x%x\n", n->n_Owner, ret));
		return;
	}
	DBUG(("DelDirSema (flg:0x%x): Deleted sema for [%s] at 0x%x, ", fp->fi_Flags, fp->fi_FileName, fp->fi_DirSema));
	DBUG(("myTask: 0x%x, OwnerTask: 0x%x, ret: 0x%x\n", CURRENTTASK->t.n_Item, n->n_Owner, ret));
	fp->fi_DirSema = (Item) NULL;
	return;
}


void
LockDirSema(File *fp, char *msg)
{
	Err err;

	if (!fp) {
    		SEMDBUG(("LockDirSema (%s): NULL fp\n", msg));
		return;
	}
	if (fp->fi_Flags & FILE_IS_DIRECTORY) {
		if ((err = SuperLockSemaphore(fp->fi_DirSema, SEM_WAIT)) != 1) {
    			SEMDBUG(("FAILED TO LOCK (%s): [%s][0x%x]", msg, fp->fi_FileName, fp->fi_DirSema));
    			SEMDBUG(("[0x%x], err: 0x%x\n", CURRENTTASK->t.n_Item, err));
		} else {
    			DBUG(("LOCKED (%s) [0x%x]: [%s]\n", msg, CURRENTTASK->t.n_Item, fp->fi_FileName));
		}
	}
}


void
UnlockDirSema(File *fp, char *msg)
{
	Err err;

	if (!fp) {
    		SEMDBUG(("UnlockDirSema (%s): NULL fp\n", msg));
		return;
	}
	if (fp->fi_Flags & FILE_IS_DIRECTORY) {
		if ((err = SuperUnlockSemaphore(fp->fi_DirSema)) != 0) {
    			SEMDBUG(("FAILED TO UNLOCK (%s): [%s][0x%x]", msg, fp->fi_FileName, fp->fi_DirSema));
    			SEMDBUG(("[0x%x], err: 0x%x\n", CURRENTTASK->t.n_Item, err));
		} else {
    			DBUG(("UNLOCKED (%s) [0x%x]: [%s]\n", msg, CURRENTTASK->t.n_Item, fp->fi_FileName));
		}
	}
}
#endif	/* FS_DIRSEMA */
