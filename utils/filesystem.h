#ifndef __H_FILESYSTEM
#define __H_FILESYSTEM

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: filesystem.h,v 1.39 1994/10/21 21:06:16 shawn Exp $
**
**  Kernel data structures for filesystem access
**
******************************************************************************/


#include "types.h"
#include "io.h"
#include "discdata.h"
#include "device.h"
#include "driver.h"
#include "folio.h"
#include "semaphore.h"

#define FILEFOLIO          3

#define FILESYSTEMNODE  1
#define FILENODE        2
#define FILEALIASNODE   3

#define FILESYSTEM_DEFAULT_BLOCKSIZE   2048

#define FILESYSTEM_TAG_END             TAG_END
#define FILESYSTEM_TAG_PRI             TAG_ITEM_LAST+1

#define FILE_TAG_END                   TAG_END
#define FILE_TAG_DATASIZE              TAG_ITEM_LAST+1

#define FILETASK_TAG_CURRENTDIRECTORY  TAG_ITEM_LAST+1
#define FILETASK_TAG_PROGRAMDIRECTORY  TAG_ITEM_LAST+2

#define DEVICE_IS_READONLY 0x00000002
#ifndef EXTERNAL_RELEASE
#define DEVICE_SORT_COUNT  1
#endif /* EXTERNAL_RELEASE */

#define ALIAS_NAME_MAX_LEN     31
#define ALIAS_VALUE_MAX_LEN    255

#define FILESYSTEM_MAX_PATH_LEN		256
#define FILESYSTEM_PART_PATH_LEN	64

/*
  Supports WRITE (0), READ (1), and STATUS (2), plus other goodies.
*/

#define FILECMD_READDIR      3
#define FILECMD_GETPATH      4
#define FILECMD_READENTRY    5
#define FILECMD_ALLOCBLOCKS  6
#define FILECMD_SETEOF       7
#define FILECMD_ADDENTRY     8
#define FILECMD_DELETEENTRY  9
#define FILECMD_SETTYPE      10
#define FILECMD_OPENENTRY    11
#define FILECMD_FSSTAT       12

#define ER_Fs_NoFile            1
#define ER_Fs_NotADirectory     2
#define ER_Fs_NoFileSystem      3
#define ER_Fs_BadName           4
#define ER_Fs_MediaError        5
#define ER_Fs_Offline           6
#define ER_Fs_DeviceError       7
#define ER_Fs_ParamError        8
#define ER_Fs_NoSpace           9
#define ER_Fs_Damaged          10
#define ER_Fs_Busy             11
#define ER_FS_DuplicateFile    12
#define ER_FS_ReadOnly         13
#define ER_Fs_DuplicateLink    14
#define ER_Fs_CircularLink     15
#define ER_Fs_LAST             16

struct HighLevelDisk;
struct File;

typedef struct HighLevelDisk HighLevelDisk;
typedef struct File File;

#ifndef EXTERNAL_RELEASE
typedef int32 (*FileDriverQueueit)
     (HighLevelDisk *theDevice, IOReq *theRequest);
typedef void (*FileDriverHook) (HighLevelDisk *theDevice);
typedef IOReq * (*FileDriverEA) (IOReq *theRequest);
typedef void (*FileDriverAbort) (IOReq *theRequest);
typedef Item (*FileDriverEntry) (File *parent, char *name);
typedef Err (*FileDriverAlloc) (IOReq *theRequest);
typedef void (*FileDriverClose) (File *theFile);
#endif /* EXTERNAL_RELEASE */

/*
  N.B. - the HighLevelDisk structure devices the header for all
  "high-level" disk devices... those on which filesystems can reside.
  All high-level device structures should start out with a HighLevelDisk
  structure or its exact equivalent.  Also, the OpenFile device structure
  must contain a DeviceType field in the same position as HighLevelDisk's
  DeviceType field.
*/

struct HighLevelDisk {
  Device             hdi;
#ifndef EXTERNAL_RELEASE
  uchar              hdi_DeviceType;
  uchar              hdi_DeviceBusy;
  uchar              hdi_RequestPriority;
  uchar              hdi_RunningPriority;
  FileDriverQueueit  hdi_QueueRequest;
  FileDriverHook     hdi_ScheduleIO;
  FileDriverHook     hdi_StartIO;
  FileDriverAbort    hdi_AbortIO;
  FileDriverEA       hdi_EndAction;
  FileDriverEntry    hdi_CreateEntry;
  FileDriverEntry    hdi_DeleteEntry;
  FileDriverAlloc    hdi_AllocSpace;
  FileDriverClose    hdi_CloseFile;
  List               hdi_RequestsToDo;
  List               hdi_RequestsRunning;
  List               hdi_RequestsDeferred;
  Item               hdi_RawDeviceItem;
  uchar              hdi_RawDeviceUnit;
  uchar              hdi_rfu[3];
#endif /* EXTERNAL_RELEASE */
};

#ifndef EXTERNAL_RELEASE
/*
  OptimizedDisk defines a high-level disk which resides on a real
  mass-storage medium, and whose performance characteristics are such
  that doing seek optimization is important.
*/

enum CatapultPhase {
  CATAPULT_NONE = 0,
  CATAPULT_AVAILABLE,
  CATAPULT_READING,
  CATAPULT_MUST_READ_FIRST,
  CATAPULT_SHOULD_READ_NEXT,
  CATAPULT_DO_READ_NEXT,
  CATAPULT_MUST_VERIFY,
  CATAPULT_MUST_SHUT_DOWN
};
#endif /* EXTERNAL_RELEASE */

typedef struct OptimizedDisk {
  HighLevelDisk        odi;
#ifndef EXTERNAL_RELEASE
  struct FileIOReq   **odi_RequestSort;
  IOReq               *odi_RawDeviceRequest;
  int32                odi_RequestSortLimit;
  uint32               odi_BlockSize;
  uint32               odi_BlockCount;
  uint32               odi_NextBlockAvailable;
  uint32               odi_RawDeviceBlockOffset;
  uchar                odi_DeferredPriority;
  struct OpenFile     *odi_CatapultFile;
  struct IoCacheEntry *odi_CatapultPage;
  enum CatapultPhase   odi_CatapultPhase;
  int32                odi_CatapultNextIndex;
  int32                odi_CatapultHits;
  int32                odi_CatapultMisses;
  int32                odi_TotCatapultStreamedHits;
  int32                odi_TotCatapultNonstreamedHits;
  int32                odi_TotCatapultTimesEntered;
  int32                odi_TotCatapultDeclined;
  int32                odi_TotCatapultSeeksAvoided;
  int32                odi_TotCatapultMisses;
  int32                odi_TotCatapultNonstreamedMisses;
#endif /* EXTERNAL_RELEASE */
} OptimizedDisk;

#ifndef EXTERNAL_RELEASE
#define DRIVER_FLAW_MASK              0x000000FF
#define DRIVER_FLAW_SHIFT             24
#define DRIVER_BLOCK_MASK             0x00FFFFFF
#define DRIVER_FLAW_SOFTLIMIT         0x000000FE
#define DRIVER_FLAW_HARDERROR         0x000000FF

#define DEVICE_BOINK                  0x80

#define MAC_MAX_NAME_LEN              256
#define MAX_ZERO_USE_FILES            16
#define CLEANUP_TIMER_LIMIT           8
#endif /* EXTERNAL_RELEASE */

#ifndef notAFileErr
# define notAFileErr                   -1302
#endif

#ifndef EXTERNAL_RELEASE
/*
  MacDisk defines a high-level "disk" which is actually mapped onto
  a Macintosh folder/file hierarchy.
*/

typedef struct MacFileInfo {
  uint32     mfi_Length;
  uint32     mfi_Info;
  uint32     mfi_NumEntries;
  uint32     rfu[5];
} MacFileInfo;

#define MAC_PATH_IS_DIRECTORY    0x00000001

typedef struct MacDirectoryEntry {
  MacFileInfo   mde_Info;
  char          mde_Name[64];
} MacDirectoryEntry;
#endif /* EXTERNAL_RELEASE */

typedef struct MacDisk {
  HighLevelDisk      mdi;
#ifndef EXTERNAL_RELEASE
  IOReq             *mdi_RawDeviceRequest;
  uchar              mdi_CurrentPathName[MAC_MAX_NAME_LEN];
  MacDirectoryEntry  mdi_DirectoryEntry;
#endif /* EXTERNAL_RELEASE */
} MacDisk;

#ifndef EXTERNAL_RELEASE
/*
  LinkedMemDisk defines a high-level "disk" which consists of a
  doubly-linked list of storage blocks in memory (RAM, ROM, NVRAM,
  or out on a gamesaver cartridge.  LinkedMemDisks have a standard
  Opera label at offset 0, with a type code of 2.  The linked list
  normally begins immediately after the label;  its offset is given
  by the zero'th avatar of the root directory.  LinkedMemDisks are,
  by definition, flat file systems and cannot contain directories.
*/

typedef struct LinkedMemBlock {
  uint32             lmb_Fingerprint; /* block fingerprint identification */
  uint32             lmb_FlinkOffset; /* 0 means first block */
  uint32             lmb_BlinkOffset; /* 0 means last block */
  uint32             lmb_BlockCount; /* includes entire header */
  uint32             lmb_HeaderBlockCount;
} LinkedMemBlock;

#define FINGERPRINT_FILEBLOCK   0xBE4F32A6
#define FINGERPRINT_FREEBLOCK   0x7AA565BD
#define FINGERPRINT_ANCHORBLOCK 0x855A02B6

#define LINKED_MEM_SLACK 32
/* the copy-buffer size must be at least FILESYSTEM_MAX_NAME_LEN! */
#define LINKED_MEM_COPY_BUFFER_SIZE 256

typedef struct LinkedMemFileEntry {
  LinkedMemBlock     lmfe;
  uint32             lmfe_ByteCount;
  uint32             lmfe_UniqueIdentifier;
  uint32             lmfe_Type;
  char               lmfe_FileName[32];
} LinkedMemFileEntry;

enum LinkedMemDiskFSM
{
  LMD_Idle,
  LMD_Done,
  LMD_Fault,
  LMD_Initialization,
  LMD_CheckIsThisLast,
  LMD_CheckSuccessor,
  LMD_CutTheSlack,
  LMD_CutOffExcess,
  LMD_GetOldBackLink,
  LMD_FixOldBackLink,
  LMD_SuccessfulChomp,
  LMD_TryNewBlock,
  LMD_ExamineNewBlock,
  LMD_ReadToCopy,
  LMD_WriteCopiedData,
  LMD_CopyDone,
  LMD_FetchHeader,
  LMD_MarkItFree,
  LMD_BackUpOne,
  LMD_ScanAhead,
  LMD_AttemptMerge,
  LMD_FixFlink,
  LMD_DoneDeleting,
  LMD_InitScan,
  LMD_ExamineEntry,
  LMD_ReadToSetEOF,
  LMD_WriteWithNewEOF,
  LMD_ReadToSetType,
  LMD_WriteWithNewType,
  LMD_FsStat,
  LMD_ExtendEOF
};
#endif /* EXTERNAL_RELEASE */

typedef struct LinkedMemDisk {
  HighLevelDisk         lmd;
#ifndef EXTERNAL_RELEASE
  IOReq                *lmd_RawDeviceRequest;
  uint32                lmd_BlockSize;
  uint32                lmd_BlockCount;
  uint32                lmd_RawDeviceBlockOffset;
  uint32                lmd_CurrentEntryOffset;
  int32                 lmd_CurrentEntryIndex;
  struct File          *lmd_CurrentFileActingOn;
  int32                 lmd_ThisBlockCursor;
  int32                 lmd_ThisBlockIndex;
  int32                 lmd_OtherBlockCursor;
  int32                 lmd_MergeBlockCursor;
  int32                 lmd_HaltCursor;
  int32                 lmd_DesiredSize;
  int32                 lmd_ContentOffset;
  int32                 lmd_BlocksToCopy;
  int32                 lmd_BlocksToRead;
  int32                 lmd_CopyBlockSize;
  int32                 lmd_FileHeaderBlockSize;
  int32                 lmd_FileHeaderByteSize;
  enum LinkedMemDiskFSM lmd_FSM;
  LinkedMemFileEntry    lmd_ThisBlock;
  LinkedMemFileEntry    lmd_OtherBlock;
  uchar                 lmd_CopyBuffer[LINKED_MEM_COPY_BUFFER_SIZE];
#endif /* EXTERNAL_RELEASE */
} LinkedMemDisk;

typedef struct FileSystem {
  ItemNode         fs;
#ifndef EXTERNAL_RELEASE
  char             fs_FileSystemName[FILESYSTEM_MAX_NAME_LEN];
  HighLevelDisk   *fs_Device;
  uint32           fs_Flags;
  uint32           fs_VolumeBlockSize;
  uint32           fs_VolumeBlockCount;
  uint32           fs_VolumeUniqueIdentifier;
  uint32           fs_RootDirectoryBlockCount;
  int32            fs_DeviceBlocksPerFilesystemBlock;
  struct File     *fs_RootDirectory;
  char             fs_MountPointName[2*FILESYSTEM_MAX_NAME_LEN];
#endif /* EXTERNAL_RELEASE */
} FileSystem;

#ifndef EXTERNAL_RELEASE
#define FILESYSTEM_IS_READONLY 0x00000002
#define FILESYSTEM_IS_IMPORTED 0x00000004
#define FILESYSTEM_IS_OFFLINE  0x00000008
#define FILESYSTEM_NEEDS_VBLS  0x00000020
#define FILESYSTEM_CACHEWORTHY 0x00000040
#define FILESYSTEM_NOT_READY   0x00000080

#define FILE_DEVICE_OPTIMIZED_DISK 1
#define FILE_DEVICE_OPENFILE       2
#define FILE_DEVICE_MAC_DISK       3
#define FILE_DEVICE_LINKED_STORAGE 4
#define FILE_DEVICE_BARF_DISK      5

typedef	struct	FSLinkInfo {
  File		*li_NextLink;
  File		*li_PreLink;
  uint32	li_FuncIndx;
} FSLinkInfo;

#define	FS_DIRSEMA
#endif /* EXTERNAL_RELEASE */
struct File {
  ItemNode         fi;
#ifndef EXTERNAL_RELEASE
  char             fi_FileName[FILESYSTEM_MAX_NAME_LEN];
  FileSystem      *fi_FileSystem;
  struct File     *fi_ParentDirectory;
  FSLinkInfo	  *fi_Linfo;
#ifdef	FS_DIRSEMA
  Item		   fi_DirSema;
#endif	/* FS_DIRSEMA */
  uint32           fi_UniqueIdentifier;
  uint32           fi_Type;
  uint32           fi_Flags;
  uint32           fi_UseCount;
  uint32           fi_BlockSize;
  uint32           fi_ByteCount;
  uint32           fi_BlockCount;
  uint32           fi_Burst;
  uint32           fi_Gap;
  uint32           fi_LastAvatarIndex;
  uint32           fi_FilesystemSpecificData;
  uint32           fi_FileSystemBlocksPerFileBlock;
  uint32           fi_AvatarList[1];
#endif /* EXTERNAL_RELEASE */
};

/*
  File flag/status bits in the low byte are specific to the filesystem.
*/

#define FILE_IS_DIRECTORY       0x00000001
#define FILE_IS_READONLY        0x00000002
#define FILE_IS_FOR_FILESYSTEM  0x00000004
#define FILE_SUPPORTS_DIRSCAN   0x00000008
#define FILE_INFO_NOT_CACHED    0x00000010
#define FILE_SUPPORTS_ENTRY     0x00000020
#define FILE_BLOCKS_CACHED      0x00000040
#define FILE_NOT_READY          0x00000080
#define FILE_HAS_LINK      	0x00000100

#define FILE_TYPE_DIRECTORY     0x2a646972
#define FILE_TYPE_LABEL         0x2a6c626c
#define FILE_TYPE_CATAPULT      0x2a7a6170

#define FILE_HIGHEST_AVATAR    255

typedef struct OpenFile {
  Device             ofi;
#ifndef EXTERNAL_RELEASE
  uchar              ofi_DeviceType;
  uchar              ofi_pad[3];
  File              *ofi_File;
  uint32             ofi_Flags;
  int32              ofi_NextBlock;
  uint32             ofi_BufferUseCount;   /* if zero, MM may scavenge */
  void              *ofi_RegisteredBuffer;
  IOReq             *ofi_InternalIOReq;
  uint32             ofi_BufferBlocks;
  uint32             ofi_BufferBlocksAvail;
  uint32             ofi_BufferBlocksFilled;
#endif /* EXTERNAL_RELEASE */
} OpenFile;

#ifndef EXTERNAL_RELEASE
#define OFILE_CONTINUOUS_IO         0x00000001
#define OFILE_CONTINUOUS_BLOCKED    0x00000002
#define OFILE_IS_SHARED             0x00000004

typedef struct BufferedFile {
  ItemNode           bfi;
  Item               bfi_OpenFile;
  void              *bfi_Buffer;
  uint32             bfi_BufferBlocks;
  uint32             bfi_BufferBlocksAvail;
  uint32             bfi_BufferBlocksFilled;
  uint32             bfi_BufferSeekBase;
  uint32             bfi_SeekNow;
} BufferedFile;

#endif /* EXTERNAL_RELEASE */
typedef struct FileIOReq {
  IOReq          fio;
#ifndef EXTERNAL_RELEASE
  uint32         fio_Flags;
  uint32         fio_BlockCount;
  uint32         fio_BlockBurst;
  uint32         fio_DevBlocksPerFileBlock;
  uint32         fio_BlockInterleave;
  uint32         fio_AbsoluteBlockNumber;
  uint32         fio_AvatarIndex;
#endif /* EXTERNAL_RELEASE */
} FileIOReq;

typedef struct FileStatus {
  DeviceStatus   fs;
  uint32         fs_ByteCount;
} FileStatus;

#ifndef EXTERNAL_RELEASE
#define FIO_CONTINUOUS_IO           0x00000001
#define FIO_INTERLEAVE_IO           0x00000002

typedef struct IoCacheEntry {
  Node           ioce;
  uint32         ioce_FilesystemUniqueIdentifier;
  uint32         ioce_FileUniqueIdentifier;
  uint32         ioce_FileBlockOffset;
  uint32         ioce_Priority;
  uint32         ioce_CacheFormat;
  uint32         ioce_CacheFirstIndex;
  uint32         ioce_CacheMiscValue;
  uint32         ioce_CachedBlockSize;
  void          *ioce_CachedBlock;
} IoCacheEntry;

#define CACHE_PRIO_HIT 16
#define CACHE_PRIO_MISS 8

#define CACHE_OPERA_DIRECTORY         1
#define CACHE_BARF_DIRECTORY          2
#define CACHE_CATAPULT_INDEX          3

typedef struct IoCache {
  Node           ioc;
  uint32         ioc_EntriesAllowed;
  uint32         ioc_EntriesPresent;
  List           ioc_CachedBlocks;
} IoCache;

typedef enum SchedulerSweepDirection {
  BottomIsCloser = 1,
  TopIsCloser = 2
} SchedulerSweepDirection;
#endif /* EXTERNAL_RELEASE */

typedef struct Alias {
  ItemNode       a;
  uchar         *a_Value;
} Alias;

#ifndef EXTERNAL_RELEASE
typedef struct CatapultPage {
  uint32         cp_MBZ;         /* must be zero */
  uint32         cp_Fingerprint; /* contains '*zap' */
  int32          cp_Entries;     /* contains # of valid entries */
  int32          cp_NextPage;    /* contains block # of next page, or -1 */
  struct {
    uint32       cpe_FileIdentifier;  /* unique ID of file */
    uint32       cpe_FileBlockOffset; /* offset of run in original file */
    uint32       cpe_RunLength;       /* # of blocks in this run */
    uint32       cpe_RunOffset;       /* offset of run in catapult file */
  } cpe[1];                    /* actually there are enough to fill page */
} CatapultPage;

#define	FSLK_TOT_FUNCS		2
#define	FSLK_PREFER_FIRST	0
#define	FSLK_HIDE_FIRST		1
#endif /* EXTERNAL_RELEASE */

typedef struct FileFolio {
  Folio          ff;
#ifndef EXTERNAL_RELEASE
  File          *ff_Root;
  List           ff_Devices;
  List           ff_Filesystems;
  List           ff_Files;
  List           ff_OpenFiles;
  int32          ff_NextUniqueID;
  int32          ff_OpensSinceCleanup;
  struct {
      Task          *ffd_Task;
      TimerDevice   *ffd_TimerDevice;
      uint32         ffd_QueuedSignal;    /* "first I/O queued for a device" */
      uint32         ffd_WaitingSignal;   /* "I'm need an I/O to wake me     */
      uint32         ffd_RescheduleSignal;/* "Did last I/O for a device      */
      uint32         ffd_CDRomSignal;     /* "Reschedule CD-ROM"             */
    } ff_Daemon;
  void		*(*ff_LinkFuncs[FSLK_TOT_FUNCS])();
#endif /* EXTERNAL_RELEASE */
} FileFolio;

#ifndef EXTERNAL_RELEASE
typedef struct FileFolioTaskData {
  File          *fftd_CurrentDirectory;
  File          *fftd_ProgramDirectory;
  File          *fftd_LinkDirectory;
  char		*fftd_LinkPartPath;
  uint32         fftd_ErrorCode;
  List           fftd_AliasList;
} FileFolioTaskData;

typedef struct FileSystemEntry {
  uint32     fse_Flags;
  char       fse_Name[FILESYSTEM_MAX_NAME_LEN];
} FileSystemEntry;
#endif /* EXTERNAL_RELEASE */

/*
 *	Filesystem status definitions (FILECMD_FSSTAT)
 */
#define	FSSTAT_CREATETIME	0x1
#define	FSSTAT_BLOCKSIZE	0x2
#define	FSSTAT_SIZE		0x4
#define	FSSTAT_MAXFILESIZE	0x8
#define	FSSTAT_FREE		0x10
#define	FSSTAT_USED		0x20

#define	FSSTAT_ISSET(bmap, bit)	(bmap & bit)
#define	FSSTAT_SET(bmap, bit)	(bmap |= bit)

#ifndef	HOWMANY
#define	HOWMANY(sz, unt)        ((sz + (unt - 1)) / unt)
#endif	/* HOWMANY */

typedef struct FileSystemStat {
  uint32     fst_BitMap;	/* field bitmap */
  uint32     fst_CreateTime;	/* filesystem creation time */
  uint32     fst_BlockSize;	/* block size of the filesystem */
  uint32     fst_Size;		/* size of the filesystem in blocks */
  uint32     fst_MaxFileSize;	/* max blocks that can be allocated */
  uint32     fst_Free;		/* total number of free blocks available */
  uint32     fst_Used;		/* total number of blocks in use */
} FileSystemStat;

#ifndef EXTERNAL_RELEASE
#ifdef	FS_DIRSEMA
void	InitDirSema(File *fp, int setowner);
void	DelDirSema(File *fp);
void	LockDirSema(File *fp, char *msg);
void	UnlockDirSema(File *fp, char *msg);
#else	/* FS_DIRSEMA */
#define LockDirSema(fp, msg)	/* no sema */
#define UnlockDirSema(fp, msg)	/* no sema */
#endif	/* FS_DIRSEMA */
#endif /* EXTERNAL_RELEASE */


/*****************************************************************************/


#endif /* __H_FILESYSTEM */
