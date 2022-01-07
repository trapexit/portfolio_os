/*****

$Id: FileFolio.c,v 1.44 1994/11/11 22:26:09 shawn Exp $

$Log: FileFolio.c,v $
 * Revision 1.44  1994/11/11  22:26:09  shawn
 * Added comments only.
 *
 * Revision 1.43  1994/11/11  22:13:10  shawn
 * Every tag has preference to Current running task. Also
 * the decrement is done in the case of the tag. If no tag
 * is specified the task that is being created will inherit
 * from the parent task. This is now compatible to the old
 * behavior. In case tag is provided we first set cur/prog
 * dir to curtask, then if tag is valid, they are selectively
 * replaced by the tag arg.
 *
 * Revision 1.42  1994/09/29  21:14:11  shawn
 * We must first process the tags, then expect to have
 * CurrentDrirectory and ProgramDirectory have values.
 * As a result of this the UseCount of CurrentDirectory
 * and ProgramDirectory of the creating task do not get
 * decremend. Therefore, all places that we ever cd to will
 * have increasing UseCount increments and they never get
 * decremented. One effect of this is that these directory
 * entries never get out of the fileinfo cache. A more
 * serious problem is that, if user cd's to /nvram and performs
 * any filesystem operations which require child task creation
 * like $c/ls of anything. Then, the /nvram filesystem can never
 * be unmounted until a system reboot.
 *
 * Revision 1.41  1994/07/26  21:06:22  dplatt
 * Add a cache-I/O-busy counter.
 *
 * Revision 1.40  1994/07/14  01:23:33  limes
 * Now that banners don't print on the screen,
 * we can print FS Cache status in non-DEV builds.
 *
 * Revision 1.39  1994/07/09  03:33:34  limes
 * Back out delta 1.38 (which made FS Cache Enabled message
 * appear all the time, unfortunately including on the screen
 * when we built roms).
 *
 * Revision 1.38  1994/06/28  02:14:46  limes
 * Change the "FS cache {en,dis}abled" message so it always
 * comes out, even in rom builds (where it may appear on the
 * serial port but never appears on the screen).
 *
 * Revision 1.37  1994/06/10  22:32:36  shawn
 * Phase II changes for rom over cd.
 *
 * Revision 1.36  1994/06/10  20:49:29  dplatt
 * Use new kernel routines to acquire directory buffer memory, and
 * directory cache page.
 *
 * Revision 1.35  1994/06/06  22:59:00  dplatt
 * Fix file use accounting problem, due to missing code to decrement
 * a parent directory's use count when deleting a File node within
 * the directory.
 *
 * Revision 1.34  1994/05/16  23:56:47  shawn
 * preliminary changes for rom over cd.
 *
 * Revision 1.33  1994/03/25  23:05:05  dplatt
 * Add vectors for Martin's new code management routines.
 *
 * Revision 1.32  1994/02/04  05:31:56  limes
 * No need to print copyright, print_vinfo takes care of this.
 *
 * Revision 1.31  1994/01/21  00:29:21  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.31  1994/01/19  20:54:05  dplatt
 * Change zero-use-count accounting strategy
 *
 * Revision 1.30  1993/12/16  00:45:40  dplatt
 * Add hook to call filesystem-specific routine when a file is closed.
 *
 * Revision 1.29  1993/11/24  06:57:13  limes
 * Quiet down the debug messages.
 *
 * Revision 1.28  1993/09/01  23:14:07  dplatt
 * Scavenge File items with zero use count... both on-the-fly (flush
 * LRU if more than 16 exist) and when a task exits (flush all).
 *
 * Revision 1.27  1993/07/28  02:30:50  dplatt
 * Zero out IOReqs;  do delete-item cleanup
 *
 * Revision 1.26  1993/07/20  07:04:19  dplatt
 * Directory cache
 *
 * Revision 1.25  1993/07/14  01:44:46  dplatt
 * Print standard copyright notice.
 *
 * Revision 1.24  1993/07/11  21:25:24  dplatt
 * Set-item-owner hook should return Err, not Item.
 *
 * Revision 1.23  1993/07/11  21:17:10  dplatt
 * Add a set-item-owner hook, return 0, thus permitting file items to have
 * their ownership changed during mount processing.
 *
 * Revision 1.22  1993/07/03  00:31:06  dplatt
 * Change label mechanisms to keep the lawyers happy
 *
 * Revision 1.21  1993/06/29  17:32:40  dplatt
 * Size reduction
 *
 * Revision 1.20  1993/06/14  01:37:57  dplatt
 * Move startup message.
 *
 * Revision 1.19  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.18  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.17  1993/05/08  01:08:14  dplatt
 * Add flat-file-system/NVRAM support, and recover from RCS bobble
 *
 * Revision 1.16  1993/04/26  20:12:55  dplatt
 * Quiet coldstart;  CD-ROM retry limit increased to 10
 *
 * Revision 1.15  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.14  1993/02/12  21:05:55  dplatt
 * More error-text-node stuff, and driver/folio renaming.
 *
 * Revision 1.13  1993/02/12  20:02:30  dplatt
 * Fix error-table tag
 *
 * Revision 1.12  1993/02/11  19:39:37  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.11  1993/02/09  01:47:20  dplatt
 * Reorganize and update for developer release
 *
 * Revision 1.10  1992/12/22  09:38:50  dplatt
 * Fix CD-ROM aborts and timer trouble
 *
 * Revision 1.9  1992/12/08  05:59:04  dplatt
 * Magenta changes
 *
 * Revision 1.8  1992/11/06  01:12:39  dplatt
 * Make LoadProgram() into a library routine
 *
 * Revision 1.7  1992/10/27  01:35:41  dplatt
 * Oops, tested on UGO not on Blue
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
 * Revision 1.2  1992/09/11  22:36:57  dplatt
 * New compiler
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
  FileFolio.c - code to initialize the file folio
*/

#define SUPER 

#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "folio.h"
#include "io.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "discdata.h"
#include "super.h"
#include "operror.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"

#undef DEBUG

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

extern void OpenFileSWI(void);
extern void CloseOpenFileSWI(void);
extern void UnimplementedSWI(void);
extern void FileDaemonInternals(void);
extern void MountFileSystemSWI(void);
extern void OpenFileInDirSWI(void);
extern void MountMacFileSystemSWI(void);
extern void ChangeDirectorySWI(void);
extern void GetDirectorySWI(void);
extern void CreateFileSWI(void);
extern void DeleteFileSWI(void);
extern void CreateAliasSWI(void);
extern void LoadOverlaySWI(void);
extern void DismountFileSystemSWI(void);
extern void CreateLinkSWI(void);

extern void OpenDiskStream(void);
extern void SeekDiskStream(void);
extern void CloseDiskStream(void);
extern void ReadDiskStream(void);

extern void LoadProgram(void);
extern void LoadProgramPrio(void);
extern void LoadCode(void);
extern void UnloadCode(void);
extern void ExecuteAsSubroutine(void);
extern void ExecuteAsThread(void);

extern void OpenDirectoryItem(void);
extern void OpenDirectoryPath(void);
extern void ReadDirectory(void);
extern void CloseDirectory(void);

void *(*FileSWIFunctions[])() = {
  (void *(*)()) CreateLinkSWI,	       /* 14 */
  (void *(*)()) DismountFileSystemSWI, /* 13 */
  (void *(*)()) LoadOverlaySWI,        /* 12 */
  (void *(*)()) CreateAliasSWI,        /* 11 */
  (void *(*)()) DeleteFileSWI,         /* 10 */
  (void *(*)()) CreateFileSWI,         /* 9 */
  (void *(*)()) GetDirectorySWI,       /* 8 */
  (void *(*)()) ChangeDirectorySWI,    /* 7 */
  (void *(*)()) MountMacFileSystemSWI, /* 6 */
  (void *(*)()) OpenFileInDirSWI,      /* 5 */
  (void *(*)()) MountFileSystemSWI,    /* 4 */
  (void *(*)()) UnimplementedSWI,      /* 3 */
  (void *(*)()) FileDaemonInternals,   /* 2 */
  (void *(*)()) CloseOpenFileSWI,      /* 1 */
  (void *(*)()) OpenFileSWI,           /* 0 */
};

void *(*FileUserFunctions[])() = {

  (void *(*)()) ExecuteAsThread,       /* -14 */
  (void *(*)()) ExecuteAsSubroutine,   /* -13 */
  (void *(*)()) UnloadCode,            /* -12 */
  (void *(*)()) LoadCode,              /* -11 */

  (void *(*)()) CloseDirectory,        /* -10 */
  (void *(*)()) ReadDirectory,         /* -9 */
  (void *(*)()) OpenDirectoryPath,     /* -8 */
  (void *(*)()) OpenDirectoryItem,     /* -7 */

  (void *(*)()) LoadProgramPrio,       /* -6 */
  (void *(*)()) LoadProgram,           /* -5 */

  (void *(*)()) CloseDiskStream,       /* -4 */
  (void *(*)()) SeekDiskStream,        /* -3 */
  (void *(*)()) ReadDiskStream,        /* -2 */
  (void *(*)()) OpenDiskStream,        /* -1 */
};

#define NUM_FILESWIFUNCS ((sizeof FileSWIFunctions) / sizeof (int32))
#define NUM_FILEUSERFUNCS ((sizeof FileUserFunctions) / sizeof (int32))

int32 FileFolioCreateTaskHook(Task *t, TagArg *tags);

Item CreateFileItem(ItemNode *theNode, uint8 nodeType, TagArg *args);
Err SetFileItemOwner(ItemNode *n, Item newOwner, struct Task *t);
int32 DeleteFileItem(Item theItem, Task *theTask);
void DeleteFileTask(Task *theTask);

long InitFileFolio(FileFolio *folio);

extern void MySysErr(int32 err);
extern void TrimAllZeroUseFiles(void);
extern void preferFirst(void);
extern void hideFirst(void);

void *GetDirectoryCache(int32 *size);  /* special kernel routine */

#ifdef GIVEDURINGDELETE
extern void GiveDaemon(void *foo);
#endif

const NodeData FileFolioNodeData[] = {
  { 0,                          0 },
  { sizeof (FileSystem),        NODE_ITEMVALID + NODE_NAMEVALID },
  { 0 /* File size varies */,   NODE_ITEMVALID + NODE_NAMEVALID },
  { sizeof (Alias),             NODE_ITEMVALID + NODE_NAMEVALID },
};

const TagArg fileFolioTags[] = {
  { CREATEFOLIO_TAG_DATASIZE,	  (void *) sizeof (FileFolio) },
  { CREATEFOLIO_TAG_NSWIS,	  (void *) NUM_FILESWIFUNCS}, 
  { CREATEFOLIO_TAG_NUSERVECS,	  (void *) NUM_FILEUSERFUNCS },
  { CREATEFOLIO_TAG_SWIS,	  (void *) FileSWIFunctions },
  { CREATEFOLIO_TAG_USERFUNCS,	  (void *) FileUserFunctions },
  { TAG_ITEM_NAME,		  (void *) "File" },
  { CREATEFOLIO_TAG_INIT,	  (void *) ((long)InitFileFolio) },
  { TAG_ITEM_PRI,		  (void *) 0 },
  { CREATEFOLIO_TAG_NODEDATABASE, (void *) FileFolioNodeData },
  { CREATEFOLIO_TAG_MAXNODETYPE,  (void *) FILEALIASNODE },
  { CREATEFOLIO_TAG_ITEM,         (void *) FILEFOLIO },
  { CREATEFOLIO_TAG_TASKDATA,     (void *) sizeof (FileFolioTaskData) },
  { 0,				  (void *) 0 },
};

const char *fileFolioErrorTable[] = {
  "Zorro!",
  "No such file",
  "Not a directory",
  "No such filesystem",
  "Illegal filename",
  "Medium I/O error",
  "Volume offline",
  "Hardware error",
  "Bad parameter",
  "Filesystem full",
  "Filesystem damaged",
  "File[system] busy",
  "Duplicate filename",
  "Read-only file[system]",
  "Duplicate link",
  "Circular link",
  };

#define ffErrSize (sizeof fileFolioErrorTable / sizeof (char *))

#ifdef PRODUCTION
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define oqprintf(x) /* x */
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#define	INFO(x)		Superkprintf x

const TagArg fileFolioErrorTags[] = {
  { TAG_ITEM_NAME,                (void *) "File errors" },
  { ERRTEXT_TAG_OBJID,            (void *) ((ER_FOLI<<ERR_IDSIZE) + ER_FSYS) },
  { ERRTEXT_TAG_MAXERR,           (void *) ffErrSize },
  { ERRTEXT_TAG_TABLE,            (void *) fileFolioErrorTable },
  { ERRTEXT_TAG_MAXSTR,           (void *) 32 },
  { TAG_END,                      (void *) 0 }
};

#ifdef DOCACHE

char *fsCacheBase;
int32 fsCacheSize;
int32 fsCacheBusy;
IoCacheEntry *fsCacheEntryArray;
int32 fsCacheEntrySize;
List fsCacheList;

void InitFileCache(void);

#endif

FileFolioTaskData *SetupFileFolioTaskDataFor(Task *theTask)
{
  FileFolioTaskData *ffPrivate;
  int32 folioIndex;
  folioIndex = fileFolio->ff.f_TaskDataIndex;
  ffPrivate = (FileFolioTaskData *) theTask->t_FolioData[folioIndex];
  if (ffPrivate == NULL) {
    DBUG(("Create file-folio private data for task 0x%x at 0x%x\n",
	   theTask->t.n_Item, theTask));
    ffPrivate = (FileFolioTaskData *) ALLOCMEM(sizeof (FileFolioTaskData),
					      MEMTYPE_ANY | MEMTYPE_FILL);
    if (!ffPrivate) {
      qprintf(("Could not create file-folio private data for task %d\n",
		   theTask->t.n_Item));
      return (FileFolioTaskData *) NULL;
    }
    ffPrivate->fftd_ErrorCode = 0;
    ffPrivate->fftd_CurrentDirectory = fileFolio->ff_Root;
    fileFolio->ff_Root->fi_UseCount ++;
    ffPrivate->fftd_ProgramDirectory = (File *) NULL;
    ffPrivate->fftd_LinkDirectory = (File *) NULL;
    ffPrivate->fftd_LinkPartPath = (char *) NULL;
    InitList(&ffPrivate->fftd_AliasList, NULL);
    DBUG(("Allocating private space at %lx for task %x\n",
		 ffPrivate, theTask));
    theTask->t_FolioData[folioIndex] = ffPrivate;
#ifdef DEBUG
    qprintf(("Assigned.\n"));
#endif
  }
  return ffPrivate;
}

FileFolioTaskData *SetupFileFolioTaskData()
{
  return (SetupFileFolioTaskDataFor(KernelBase->kb_CurrentTask));
}

long
InitFileFolio (FileFolio *folio)
{
  Item errorTableItem;
/*
  Save the folio address in static memory for later use
*/
  fileFolio = folio;
  folio->ff_Root = (File *) NULL;
  InitList(&folio->ff_Devices, "Devices");
  InitList(&folio->ff_Filesystems, "Filesystems");
  InitList(&folio->ff_Files, "Files");
  InitList(&folio->ff_OpenFiles, "OpenFiles");
  folio->ff.f_ItemRoutines->ir_Create =
    (Item (*)(void *, uint8, void *)) CreateFileItem;
  folio->ff.f_ItemRoutines->ir_Delete = (int32 (*)())DeleteFileItem;
  folio->ff.f_ItemRoutines->ir_SetOwner = SetFileItemOwner;
  folio->ff.f_FolioDeleteTask = (void (*)()) DeleteFileTask;
  folio->ff.f_FolioCreateTask = (int32 (*)()) FileFolioCreateTaskHook;
  folio->ff_Daemon.ffd_Task = (Task *) NULL;
  folio->ff_Daemon.ffd_QueuedSignal = 0;
  folio->ff_Daemon.ffd_WaitingSignal = 0;
  folio->ff_Daemon.ffd_RescheduleSignal = 0;
  folio->ff_NextUniqueID = -1;
  folio->ff_OpensSinceCleanup = 0;
  errorTableItem = SuperCreateItem(MKNODEID(KERNELNODE,ERRORTEXTNODE),
				   (TagArg *) fileFolioErrorTags);
  if (errorTableItem < 0) {
    qprintf(("Can't register file-folio error table, 0x%x!\n", errorTableItem));
    MySysErr(errorTableItem);
  }
#ifdef DOCACHE
  InitFileCache();
#endif
  oqprintf(("File-folio init ends\n"));
  folio->ff_LinkFuncs[FSLK_PREFER_FIRST] = (void *(*)()) preferFirst;
  folio->ff_LinkFuncs[FSLK_HIDE_FIRST] = (void *(*)()) hideFirst;
  return folio->ff.fn.n_Item;
}

#ifdef DOCACHE
void InitFileCache()
{
  int32 entryCount;
  int32 i;
  DBUG(("Initializing filesystem cache\n"));
  fsCacheSize = GetPageSize(MEMTYPE_DRAM);
  DBUG(("DRAM page size is %d bytes\n", fsCacheSize));
  entryCount = fsCacheSize / FILESYSTEM_DEFAULT_BLOCKSIZE;
  fsCacheSize = entryCount * FILESYSTEM_DEFAULT_BLOCKSIZE;
  fsCacheBase = (char *) GetDirectoryCache(&fsCacheSize);
  if (!fsCacheBase) {
    INFO(("FS cache disabled\n"));
    return;
  }
  DBUG(("Filesystem cache base is 0x%x\n", fsCacheBase));
  fsCacheEntrySize = entryCount * sizeof (IoCacheEntry);
  fsCacheEntryArray = (IoCacheEntry *) AllocMem(fsCacheEntrySize, MEMTYPE_FILL);
  if (!fsCacheEntryArray) {
    DBUG(("Could not allocate %d-byte cache table\n", fsCacheEntrySize));
    FreeMem(fsCacheBase, fsCacheSize);
    fsCacheBase = NULL;
    return;
  }
  DBUG(("Cache table is %d bytes at 0x%x\n", fsCacheEntrySize, fsCacheEntryArray));
  InitList(&fsCacheList, "f/s cache");
  for (i = 0; i < entryCount; i++) {
    fsCacheEntryArray[i].ioce_CachedBlock = fsCacheBase + i * FILESYSTEM_DEFAULT_BLOCKSIZE;
    AddTail(&fsCacheList, (Node *) &fsCacheEntryArray[i]);
    DBUG(("Entry %d buffer is at 0x%x\n", i, fsCacheEntryArray[i].ioce_CachedBlock));
  }
  INFO(("FS cache enabled\n"));
}

#endif

/*
 *	Every tag has preference to Current running task. If no tag
 *	is specified the task that is being created will inherit
 *	from the parent task. In case tag is provided we first set
 *	cur/prog dir to curtask, then if tag is valid, curdir and
 *	progdir are selectively	replaced by the appropriate tag args.
 */
int32 FileFolioCreateTaskHook(Task *t, TagArg *tags)
{
  FileFolioTaskData *ffPrivate, *ownerPrivate;
  File *directoryFile;
  DBUG(("File-folio create-task hook for task 0x%x\n", t));
  ffPrivate = SetupFileFolioTaskDataFor(t);
  if (!ffPrivate) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_NoMem);
  }
  if (CURRENTTASK != NULL) {
    ownerPrivate = (FileFolioTaskData *) CURRENTTASK->t_FolioData[fileFolio->ff.f_TaskDataIndex];
    if (ownerPrivate) {
      if (ownerPrivate->fftd_ProgramDirectory) {
	if (ffPrivate->fftd_ProgramDirectory) {
	  ffPrivate->fftd_ProgramDirectory->fi_UseCount --;
	}
	ffPrivate->fftd_ProgramDirectory = ownerPrivate->fftd_ProgramDirectory;
	ffPrivate->fftd_ProgramDirectory->fi_UseCount ++;
      }
      if (ownerPrivate->fftd_CurrentDirectory) {
	if (ffPrivate->fftd_CurrentDirectory) {
	  ffPrivate->fftd_CurrentDirectory->fi_UseCount --;
	}
	ffPrivate->fftd_CurrentDirectory = ownerPrivate->fftd_CurrentDirectory;
	ffPrivate->fftd_CurrentDirectory->fi_UseCount ++;
      }
    }
  }
  if (!tags) {
    DBUG(("No tags.\n"));
    return t->t.n_Item;
  }
  while ((tags->ta_Tag & 0x000000FF) != 0) {
    DBUG(("Examine tag 0x%x\n", tags->ta_Tag));
    if (((tags->ta_Tag >> 16) & 0x0000FFFF) == FILEFOLIO) {
      switch (tags->ta_Tag & 0x000000FF) {
      case FILETASK_TAG_CURRENTDIRECTORY:
	DBUG(("CURRENT_DIRECTORY tag 0x%x\n", tags->ta_Arg));
	if (tags->ta_Arg == 0) {
	  break;
	}
	directoryFile = (File *) CheckItem((Item) tags->ta_Arg,
					   FILEFOLIO,
					   FILENODE);
	if (directoryFile) {
	  if (directoryFile->fi_Flags & FILE_IS_DIRECTORY) {
	    if (ffPrivate->fftd_CurrentDirectory) {
	      ffPrivate->fftd_CurrentDirectory->fi_UseCount --;
	    }
	    ffPrivate->fftd_CurrentDirectory = directoryFile;
	    directoryFile->fi_UseCount ++;
	    DBUG(("Setting current directory to %s\n", directoryFile->fi_FileName));
	  } else {
	    DBUG(("Not a directory item\n"));
	    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
	  }
	} else {
	  DBUG(("Bad item\n"));
	  return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
	}
	break;
      case FILETASK_TAG_PROGRAMDIRECTORY:
	DBUG(("PROGRAM_DIRECTORY tag 0x%x\n", tags->ta_Arg));
	if (tags->ta_Arg == 0) {
	  break;
	}
	directoryFile = (File *) CheckItem((Item) tags->ta_Arg,
					   FILEFOLIO,
					   FILENODE);
	if (directoryFile) {
	  if (directoryFile->fi_Flags & FILE_IS_DIRECTORY) {
	    if (ffPrivate->fftd_ProgramDirectory) {
	      ffPrivate->fftd_ProgramDirectory->fi_UseCount --;
	    }
	    ffPrivate->fftd_ProgramDirectory = directoryFile;
	    directoryFile->fi_UseCount ++;
	    DBUG(("Setting program directory to %s\n", directoryFile->fi_FileName));
	  } else {
	    DBUG(("Not a directory item\n"));
	    return MakeFErr(ER_SEVER,ER_C_NSTND,ER_Fs_NotADirectory);
	  }
	} else {
	  DBUG(("Bad item\n"));
	  return MakeFErr(ER_SEVER,ER_C_STND,ER_BadItem);
	}
	break;
      default:
	DBUG(("Unknown file-folio tag\n"));
	break; /* maybe should return an error code here */
      }
    } else {
      DBUG(("Not a filesystem tag, ignored.\n"));
    }
    tags ++;
  }
  DBUG(("File folio create-task hook done\n"));
  return t->t.n_Item;
}

static Err FileTagProcessor(Item *n, void *dataP, uint32 tagc, uint32 taga)
{
  return 0;
}

Item CreateFileItem(ItemNode *theNode, uint8 nodeType, TagArg *args)
{
  if (theNode == (ItemNode *) NULL ||
      theNode == (ItemNode *) -1) {
    return MakeFErr(ER_SEVER,ER_C_STND,ER_BadSubType);
  }
  switch (nodeType) {
  case FILESYSTEMNODE:
    theNode->n_Name = (uchar *) ((FileSystem *)theNode)->fs_FileSystemName;
    DBUG(("Created filesystem node %s\n",theNode->n_Name));
    break;
  case FILENODE:
    if (theNode->n_Size < sizeof (File)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadSubType);
    }
    theNode->n_Name = (uchar *) ((File *) theNode)->fi_FileName;
    DBUG(("Created file node %s\n",theNode->n_Name));
    break;
  case FILEALIASNODE:
    if (theNode->n_Size < sizeof (Alias)) {
      return MakeFErr(ER_SEVER,ER_C_STND,ER_BadSubType);
    }
    TagProcessor(theNode, args, FileTagProcessor, NULL);
    break;
  }
  return theNode->n_Item;
}

int32 DeleteFileItem(Item theItem, Task *theTask)
{
  Node *theNode;
  File *parent;
  int32 useCount;
  HighLevelDisk *disk;
  theNode = (Node *) LookupItem(theItem);
  if (!theNode) {
    return 0;
  }
  switch (theNode->n_Type) {
  case FILESYSTEMNODE:
#ifdef DEBUG
    qprintf(("Delete filesystem node '%s' at 0x%lx for task %x\n",
	     theNode->n_Name, theNode, theTask->t.n_Item));
#endif
    break;
  case FILENODE:
    DBUG(("Delete File node '%s' at %lx for task %x\n",
	   theNode->n_Name, theNode, theTask->t.n_Item));
    useCount = (int32) ((File *)theNode)->fi_UseCount;
    if (useCount > 0) {
      DBUG(("Nope!  Use count = %d\n", ((File *)theNode)->fi_UseCount));
#ifdef GIVEDURINGDELETE
      GiveDaemon(theNode);
#endif
      return -1;
    } else if (useCount < 0) {
      qprintf(("Gaah! File %s use count = %d\n",
		   ((File *)theNode)->fi_FileName,
		   ((File *)theNode)->fi_UseCount));
#ifdef GIVEDURINGDELETE
      GiveDaemon(theNode);
#endif
      return -1;
    } else {
      disk = ((File *)theNode)->fi_FileSystem->fs_Device;
      if (disk->hdi_CloseFile) {
	DBUG(("Calling close-file hook for /%s\n",
	       ((File *)theNode)->fi_FileSystem->fs.n_Name));
	(*disk->hdi_CloseFile)((File *) theNode);
      }
      parent = ((File *)theNode)->fi_ParentDirectory;
      RemNode(theNode);
      if (parent) {
	parent->fi_UseCount --;
      }
#ifdef DEBUG
      qprintf(("Unlinked from files list\n"));
#endif
    }
    break;
  case FILEALIASNODE:
    DBUG(("Unlinking and deleting alias node '%s' for task 0x%lx\n",
	  theNode->n_Name, theTask->t.n_Item));
    RemNode(theNode);
    break;
  default:
    qprintf(("Deleting filesystem node type %d, at %lx for task %x\n",
		 theNode->n_Type, theNode, theTask->t.n_Item));
    break;
  }
  return 0;
}

Err SetFileItemOwner(ItemNode *n, Item newOwner, struct Task *t)
{
  return 0;
}


void DeleteFileTask(Task *theTask)
{
  FileFolioTaskData *ffPrivate;
  ffPrivate = (FileFolioTaskData *) theTask->t_FolioData[fileFolio->ff.f_TaskDataIndex];
  if (ffPrivate) {
    DBUG(("Tearing down private-data block 0x%x for task 0x%x\n",
	   ffPrivate, theTask->t.n_Item));
    theTask->t_FolioData[fileFolio->ff.f_TaskDataIndex] = NULL;
    if (ffPrivate->fftd_CurrentDirectory) {
      ffPrivate->fftd_CurrentDirectory->fi_UseCount --;
#ifdef DEBUG
      qprintf(("Use-count for current-directory %s is now %d\n",
		   ffPrivate->fftd_CurrentDirectory->fi_FileName,
		   ffPrivate->fftd_CurrentDirectory->fi_UseCount));
#endif
    }
    if (ffPrivate->fftd_ProgramDirectory) {
      ffPrivate->fftd_ProgramDirectory->fi_UseCount --;
#ifdef DEBUG
      qprintf(("Use-count for current-program %s is now %d\n",
		   ffPrivate->fftd_ProgramDirectory->fi_FileName,
		   ffPrivate->fftd_ProgramDirectory->fi_UseCount));
#endif
    }
#ifdef DEBUG
    qprintf(("Releasing private-data block %x for task %x\n",
		 ffPrivate, theTask));
#endif
    FREEMEM(ffPrivate, sizeof (FileFolioTaskData));
#ifdef DEBUG
    qprintf(("Released.\n"));
#endif
  }
  if (theTask->t_ThreadTask == (Task *) NULL) {
    TrimAllZeroUseFiles(); /* flush all zero-use files when killing a task */
  }
}

void UnimplementedSWI(void)
{
  return;
}

/*
  Funky stuff to cut down on stdlib size, per dale
*/

int stdout;
int
putc(int a)
{
    kprintf("%c",a);
    return a;
}
