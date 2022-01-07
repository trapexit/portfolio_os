#ifndef __H_DISCDATA
#define __H_DISCDATA

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: discdata.h,v 1.16 1994/10/25 15:57:33 vertex Exp $
**
******************************************************************************/


/*
  Define the position of the primary label on each Opera disc, the
  block offset between avatars, and the index of the last avatar
  (i.e. the avatar count minus one).  The latter figure *must* match
  the ROOT_HIGHEST_AVATAR figure from "filesystem.h", as the same
  File structure is use to read the label at boot time, and to provide
  access to the root directory.
*/

#define DISC_BLOCK_SIZE           2048
#define DISC_LABEL_OFFSET         225
#define DISC_LABEL_AVATAR_DELTA   32786
#define DISC_LABEL_HIGHEST_AVATAR 7
#define DISC_TOTAL_BLOCKS         330000

#define ROOT_HIGHEST_AVATAR       7
#define FILESYSTEM_MAX_NAME_LEN   32

#ifndef EXTERNAL_RELEASE
#define VOLUME_STRUCTURE_OPERA_READONLY    1
#define VOLUME_STRUCTURE_LINKED_MEM        2

#define VOLUME_SYNC_BYTE          0x5A
#define VOLUME_SYNC_BYTE_LEN      5
#define VOLUME_COM_LEN      	  32
#define VOLUME_ID_LEN      	  32

/*
// This disc won't necessarily cause a reboot when inserted.  This flag is
// advisory ONLY. Only by checking with cdromdipir can you be really sure.
// Place in dl_VolumeFlags.  Note: the first volume gets this flag also.
*/
#define	VOLUME_FLAGS_DATADISC	0x01

/*
  Data structures written on CD disc (Compact Disc disc?)
*/
typedef struct DiscLabel {
  uchar    dl_RecordType;                   /* Should contain 1 */
  uint8    dl_VolumeSyncBytes[VOLUME_SYNC_BYTE_LEN]; /* Synchronization byte */
  uchar    dl_VolumeStructureVersion;       /* Should contain 1 */
  uchar    dl_VolumeFlags;                  /* Should contain 0 */
  uchar    dl_VolumeCommentary[VOLUME_COM_LEN];
					    /* Random comments about volume */
  uchar    dl_VolumeIdentifier[VOLUME_ID_LEN]; /* Should contain disc name */
  uint32   dl_VolumeUniqueIdentifier;       /* Roll a billion-sided die */
  uint32   dl_VolumeBlockSize;              /* Usually contains 2048 */
  uint32   dl_VolumeBlockCount;             /* # of blocks on disc */
  uint32   dl_RootUniqueIdentifier;         /* Roll a billion-sided die */
  uint32   dl_RootDirectoryBlockCount;      /* # of blocks in root */
  uint32   dl_RootDirectoryBlockSize;       /* usually same as vol blk size */
  uint32   dl_RootDirectoryLastAvatarIndex; /* should contain 7 */
  uint32   dl_RootDirectoryAvatarList[ROOT_HIGHEST_AVATAR+1];
} DiscLabel;

typedef struct DirectoryHeader {
  int32      dh_NextBlock;
  int32      dh_PrevBlock;
  uint32     dh_Flags;
  uint32     dh_FirstFreeByte;
  uint32     dh_FirstEntryOffset;
} DirectoryHeader;

#define DIRECTORYRECORD(AVATARCOUNT) \
  uint32     dir_Flags; \
  uint32     dir_UniqueIdentifier; \
  uint32     dir_Type; \
  uint32     dir_BlockSize; \
  uint32     dir_ByteCount; \
  uint32     dir_BlockCount; \
  uint32     dir_Burst; \
  uint32     dir_Gap; \
  char       dir_FileName[FILESYSTEM_MAX_NAME_LEN]; \
  uint32     dir_LastAvatarIndex; \
  uint32     dir_AvatarList[AVATARCOUNT];

typedef struct DirectoryRecord {
  DIRECTORYRECORD(1)
} DirectoryRecord;
#endif /* EXTERNAL_RELEASE */

#define DIRECTORY_LAST_IN_DIR        0x80000000
#define DIRECTORY_LAST_IN_BLOCK      0x40000000


/*****************************************************************************/


#endif /* __H_DISCDATA */
