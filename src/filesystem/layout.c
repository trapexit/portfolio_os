/*****

$Id: layout.c,v 1.10 1994/07/15 21:35:58 dplatt Exp $

$Log: layout.c,v $
 * Revision 1.10  1994/07/15  21:35:58  dplatt
 * When writing the image file on a Mac, use the special magick option
 * which bypasses the file cache and goes directly to the disk.
 * The cache is a loss when writing the image file, as very little of
 * it is ever read back (only the catapult page), and the Mac's
 * cache-flush code is piggishly slow.
 *
 * Revision 1.9  1993/07/03  00:31:06  dplatt
 * Change label mechanisms to keep the lawyers happy
 *
 * Revision 1.8  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.7  1993/03/04  22:42:13  dplatt
 * Improve support for unused-block preinitialization
 *
 * Revision 1.6  1993/02/10  00:24:40  dplatt
 * Minor Mac-oriented changes
 *
 * Revision 1.5  1993/02/09  01:47:20  dplatt
 * Reorganize and update for developer release
 *
 * Revision 1.3  1992/12/08  05:59:52  dplatt
 * Magenta changes
 *
 * Revision 1.2  1992/10/01  23:36:21  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:42:33  dplatt
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
  maker.c - stub code to build a minimal Opera filesystem.
  Most of this is bogus scaffolding for the debug/simulation phase of
  development, and will need to be redone.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef applec

# include <Quickdraw.h>
# include <OSUtils.h>
# include <Files.h>
# include <ioctl.h>
# include <Events.h>
# include <stdarg.h>

# define Boolean operaBoolean

# define __swi(foo) /* foo */

#define FASTWRITE

#else

# include <malloc.h>
# include <sys/types.h>
# include <sys/time.h>
# include <sys/timeb.h>
# include <unistd.h>

#endif

typedef unsigned char uchar;
typedef unsigned int uint32;
typedef unsigned char uint8;
typedef long int int32;

#include "discdata.h"

#include "strings.h"

#include "tcl.h"

#ifdef applec
# undef FREERETURNSRESULT
#else
#ifdef __GNUC__
# undef FREERETURNSRESULT
#else
# define FREERETURNSRESULT
#endif
#endif

/* #define DEBUG */

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

typedef struct Buffer {
  int32      size;
  int32      used;
  void     *contents;
} Buffer;

Tcl_Interp *interp;
Tcl_CmdBuf buffer;
int32 quitFlag = 0;

#ifdef applec
char *initCmd = "set environment Mac ; source layout.tcl";
#else
char *initCmd = "set environment Unix ; source layout.tcl";
#endif

static int32 maxAvatars = 32;

extern int32 TclGetOpenFile(Tcl_Interp *, char *, FILE ***);

#undef NOBUFFER

#ifdef applec

char	**environ = NULL;

#endif


#ifdef applec

void
Feedback(char *format, ...)
	{
	va_list		varg;
	
	va_start(varg, format);
	
	vfprintf(stderr, format, varg);
	
	va_end(varg);
	}

RotateCursor(phase)
long	phase;
	{
	extern pascal void ROTATECURSOR(int phase);
	
	ROTATECURSOR(phase);
	}

check_environment_set_of_globals(name, value)
char	*name;
char	*value;
	{
	}

CheckCmdPeriod()
{
KeyMap	mykeys;

	GetKeys(mykeys);
	return (mykeys[1] == 0x00808000);
	}


#endif

#ifdef TCL_MEM_DEBUG
# define malloc(x) ckalloc(x)
# define free(x) ckfree(x)
#endif

static char *ckcalloc(int count, long size)
{
  char *block;
  long int idx, totsize;
  totsize = count * size;
  block = (char *) malloc(totsize);
  if (!block) return block;
  while (--totsize >= 0) {
  	block[totsize] = 0;
  }
  return block;
}

static char *ckrealloc(void *block, long size, long oldsize)
{
  char *newblock;
  newblock = (char *) malloc(size);
  if (!newblock) return newblock;
  if (size < oldsize) {
    memcpy(newblock, block, size);
  } else {
    memcpy(newblock, block, oldsize);
  }
  ckfree(block);
  return newblock;
}

int32 FindTclFile (interp, token, theFilePtr)
     Tcl_Interp *interp;
     char *token;
     FILE **theFilePtr;
{
  FILE **theFile;
  if (TclGetOpenFile(interp, token, &theFile) != TCL_OK) {
    return TCL_ERROR;
  }
  *theFilePtr = *theFile;
  return TCL_OK;
}

int32
cmdAvatarLimit(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 avatarLimit;

    if (argc != 2) {
      Tcl_SetResult(interp, "Illegal avatarlimit command", TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &avatarLimit) != TCL_OK) {
      return TCL_ERROR;
    }
    if (avatarLimit < 1 || avatarLimit > 256) {
      Tcl_SetResult(interp, "Avatar limit must be between 1 and 256",
		    TCL_STATIC);
      return TCL_ERROR;
    }
    maxAvatars = avatarLimit;
    return TCL_OK;
}

	/* ARGSUSED */
int32
cmdNewFile(clientData, interp, argc, argv)
     ClientData *clientData;
     Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    int32 i;
    DirectoryRecord *dirRecord;
    char addressBuf[256];
    Buffer *buffer;

    if (argc < 2) {
      Tcl_SetResult(interp, "Usage: newfile NAME", TCL_STATIC);
      return TCL_ERROR;
    }

    buffer = (Buffer *) calloc(1, sizeof (Buffer));

    if (!buffer) {
      Tcl_SetResult(interp, "Could not allocate buffer record", TCL_STATIC);
      return TCL_ERROR;
    }

    dirRecord = (DirectoryRecord *) calloc(1, sizeof (DirectoryRecord));

    if (!dirRecord) {
      free((void *) buffer);
      Tcl_SetResult(interp, "Could not allocate directory record", TCL_STATIC);
      return TCL_ERROR;
    }

    buffer->size = sizeof (DirectoryRecord);
    buffer->used = sizeof (DirectoryRecord) - sizeof (long); /* 0 avatars */
    buffer->contents = dirRecord;
     
    dirRecord->dir_BlockSize = DISC_BLOCK_SIZE;
    dirRecord->dir_Type = 0x3f3f3f3f;
    strcpy(dirRecord->dir_FileName, argv[1]);
    dirRecord->dir_LastAvatarIndex = -1;
    sprintf(addressBuf, "0x%lX", (long) buffer);
    Tcl_SetResult(interp, addressBuf, TCL_VOLATILE);
    return TCL_OK;
}

	/* ARGSUSED */
int32
cmdDeleteFile(clientData, interp, argc, argv)
     ClientData *clientData;
     Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    int32 i;
    DirectoryRecord *dirRecord;
    char addressBuf[256];
    Buffer *buffer;

    if (argc != 2) {
      Tcl_SetResult(interp, "Usage: deletefile NAME", TCL_STATIC);
      return TCL_ERROR;
    }

#ifdef applec
    remove(argv[1]);
#else
    unlink(argv[1]);
#endif
    return TCL_OK;
}

int32
cmdNewLabel(clientData, interp, argc, argv)
     ClientData *clientData;
     Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
  int32 avatarIndex, avatarBlock;
  DiscLabel *discLabel;
  char addressBuf[256];
  Buffer *buffer;
  int32 blockCount, uniqueIdentifier, blockSize;
  
  if (argc < 4) {
    Tcl_SetResult(interp,
		  "Usage: newlabel DISCNAME BLOCKCOUNT UNIQUEID [BLOCKSIZE]",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  
  buffer = (Buffer *) calloc(1, sizeof (Buffer));
  
  if (Tcl_ExprLong(interp, argv[2], &blockCount) != TCL_OK) {
    return TCL_ERROR;
  }

  if (Tcl_ExprLong(interp, argv[3], &uniqueIdentifier) != TCL_OK) {
    return TCL_ERROR;
  }
  
  if (!buffer) {
    Tcl_SetResult(interp, "Could not allocate buffer record", TCL_STATIC);
    return TCL_ERROR;
  }

  if (argc >= 5) {
    if (Tcl_ExprLong(interp, argv[4], &blockSize) != TCL_OK) {
      return TCL_ERROR;
    }
  } else {
    blockSize = DISC_BLOCK_SIZE;
  }
    
  
  discLabel = (DiscLabel *) calloc(1, sizeof (DiscLabel));
  
  if (!discLabel) {
    free((void *) buffer);
    Tcl_SetResult(interp, "Could not allocate label record", TCL_STATIC);
    return TCL_ERROR;
  }
  
  buffer->size = sizeof (DiscLabel);
  buffer->used = sizeof (DiscLabel);
  buffer->contents = discLabel;
  
  discLabel->dl_RecordType = 1;
  memset(discLabel->dl_VolumeSyncBytes, VOLUME_SYNC_BYTE, sizeof discLabel->dl_VolumeSyncBytes);
  discLabel->dl_VolumeStructureVersion = 1;
  discLabel->dl_VolumeFlags = 0;
  discLabel->dl_VolumeBlockCount = blockCount;
  discLabel->dl_VolumeUniqueIdentifier = uniqueIdentifier;
  strcpy((char *) discLabel->dl_VolumeCommentary, "");
  strcpy((char *) discLabel->dl_VolumeIdentifier, argv[1]);
  discLabel->dl_VolumeBlockSize = blockSize;
  
  sprintf(addressBuf, "0x%lX", (long) buffer);
  Tcl_SetResult(interp, addressBuf, TCL_VOLATILE);
  return TCL_OK;
}

int32 cmdSetRoot(clientData, interp, argc, argv)
     ClientData *clientData;
     Tcl_Interp *interp;
     int32 argc;
     char *argv[];
{
  int32 avatarLimit, blockCount, avatarIndex, avatarBlock, rootID;
  int32 blockSize;
  DiscLabel *discLabel;
  int32 bufferAddress;
  char addressBuf[256];
  
  if (argc < 6) {
    Tcl_SetResult(interp, "Usage: setroot LABEL BLOCKCOUNT BLOCKSIZE UNIQUEID AVATAR+",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  
  if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
    return TCL_ERROR;
  }

  discLabel = (DiscLabel *) ((Buffer *) bufferAddress)->contents;
  
/***   discLabel->dl_VolumeBlockSize = DISC_BLOCK_SIZE; ***/
  
  if (Tcl_ExprLong(interp, argv[2], &blockCount) != TCL_OK) {
    return TCL_ERROR;
  }

  if (Tcl_ExprLong(interp, argv[3], &blockSize) != TCL_OK) {
    return TCL_ERROR;
  }

  if (Tcl_ExprLong(interp, argv[4], &rootID) != TCL_OK) {
    return TCL_ERROR;
  }

  discLabel->dl_RootDirectoryBlockCount = blockCount;

  avatarLimit = argc - 6;

  if (avatarLimit > ROOT_HIGHEST_AVATAR) {
    sprintf(addressBuf, "More than %ld avatars specified",
	    ROOT_HIGHEST_AVATAR + 1);
    Tcl_SetResult(interp, addressBuf, TCL_VOLATILE);
    return TCL_ERROR;
  }

  discLabel->dl_RootDirectoryLastAvatarIndex = avatarLimit;
  discLabel->dl_RootUniqueIdentifier = rootID;
  discLabel->dl_RootDirectoryBlockSize = blockSize;

  for (avatarIndex = 0; avatarIndex <= avatarLimit; avatarIndex++) {
    if (Tcl_ExprLong(interp, argv[avatarIndex+5], &avatarBlock) != TCL_OK) {
      return TCL_ERROR;
    }
    discLabel->dl_RootDirectoryAvatarList[avatarIndex] = avatarBlock;
  }

  return TCL_OK;
}

int32
cmdSetFile(clientData, interp, argc, argv)
     ClientData *clientData;
     Tcl_Interp *interp;
     int32 argc;
     char *argv[];
{
  int32 i;
  int32 bufferAddress;
  DirectoryRecord *dirRecord;
  int32 theVal;
  char oops[256];
  
  if (argc < 4 || (argc % 2) != 0) {
    Tcl_SetResult(interp, "Illegal setfile command", TCL_STATIC);
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
    return TCL_ERROR;
  }
  if (bufferAddress == 0) {
    Tcl_SetResult(interp, "Null address", TCL_STATIC);
    return TCL_ERROR;
  }
  dirRecord = (DirectoryRecord *) ((Buffer *) bufferAddress)->contents;
  for (i = 2; i < argc; i += 2) {
    if (strcmp(argv[i], "-name") == 0) {
      strcpy(dirRecord->dir_FileName, argv[i+1]);
    } else if (strcmp(argv[i], "-type") == 0) {
      if (Tcl_ExprLong(interp, argv[i+1], &theVal) != TCL_OK) {
	if (strlen(argv[i+1]) <= sizeof dirRecord->dir_Type) {
	  memcpy((char *) &dirRecord->dir_Type, "    ", 
		 sizeof dirRecord->dir_Type);
	  memcpy((char *) &dirRecord->dir_Type, argv[i+1],
		 strlen(argv[i+1]));
	} else {
	  memcpy((char *) &dirRecord->dir_Type, argv[i+1],
		 sizeof dirRecord->dir_Type);
	}
      } else {
	dirRecord->dir_Type = theVal;
      }
    } else if (strcmp(argv[i], "-blocksize") == 0) {
      if (Tcl_ExprLong(interp, argv[i+1], &theVal) != TCL_OK) {
	return TCL_ERROR;
      }
      dirRecord->dir_BlockSize = theVal; 
    } else if (strcmp(argv[i], "-blockcount") == 0) {
      if (Tcl_ExprLong(interp, argv[i+1], &theVal) != TCL_OK) {
	return TCL_ERROR;
      }
      dirRecord->dir_BlockCount = theVal;
    } else if (strcmp(argv[i], "-bytecount") == 0) {
      if (Tcl_ExprLong(interp, argv[i+1], &theVal) != TCL_OK) {
	return TCL_ERROR;
      }
      dirRecord->dir_ByteCount = theVal;
    } else if (strcmp(argv[i], "-flags") == 0) {
      if (Tcl_ExprLong(interp, argv[i+1], &theVal) != TCL_OK) {
	return TCL_ERROR;
      }
      dirRecord->dir_Flags = theVal;
    } else if (strcmp(argv[i], "-uniqueidentifier") == 0) {
      if (Tcl_ExprLong(interp, argv[i+1], &theVal) != TCL_OK) {
	return TCL_ERROR;
      }
      dirRecord->dir_UniqueIdentifier = theVal;
    } else if (strcmp(argv[i], "-burst") == 0) {
      if (Tcl_ExprLong(interp, argv[i+1], &theVal) != TCL_OK) {
	return TCL_ERROR;
      }
      dirRecord->dir_Burst = theVal;
    } else if (strcmp(argv[i], "-gap") == 0) {
      if (Tcl_ExprLong(interp, argv[i+1], &theVal) != TCL_OK) {
	return TCL_ERROR;
      }
      dirRecord->dir_Gap = theVal;
    } else {
      sprintf(oops, "Unknown setfile option %s", argv[i]);
      Tcl_SetResult(interp, oops, TCL_VOLATILE);
      return TCL_ERROR;
    }
  }
  return TCL_OK;
}

int32
cmdGetFile(clientData, interp, argc, argv)
ClientData *clientData;
Tcl_Interp *interp;
int32 argc;
char *argv[];
{
  int32 i;
  int32 bufferAddress;
  DirectoryRecord *dirRecord;
  int32 theVal;
  int32 avatarIndex, lastAvatarIndex;
  char oops[256], avatars[2048];
  
  if (argc != 3) {
    Tcl_SetResult(interp, "Illegal getfile command", TCL_STATIC);
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
    return TCL_ERROR;
  }
  if (bufferAddress == 0) {
    Tcl_SetResult(interp, "Null address", TCL_STATIC);
    return TCL_ERROR;
  }
  dirRecord = (DirectoryRecord *) ((Buffer *) bufferAddress)->contents;
  if (strcmp(argv[2], "-name") == 0) {
    Tcl_SetResult(interp, dirRecord->dir_FileName, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-type") == 0) {
    sprintf(oops, "0x%x", dirRecord->dir_Type);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-blocksize") == 0) {
    sprintf(oops, "%d", dirRecord->dir_BlockSize);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-blockcount") == 0) {
    sprintf(oops, "%d", dirRecord->dir_BlockCount);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-bytecount") == 0) {
    sprintf(oops, "%d", dirRecord->dir_ByteCount);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-flags") == 0) {
    sprintf(oops, "0x%x", dirRecord->dir_Flags);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-uniqueidentifier") == 0) {
    sprintf(oops, "0x%x", dirRecord->dir_UniqueIdentifier);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-burst") == 0) {
    sprintf(oops, "%d", dirRecord->dir_Burst);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-gap") == 0) {
    sprintf(oops, "%d", dirRecord->dir_Gap);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-numavatars") == 0) {
    sprintf(oops, "%d", dirRecord->dir_LastAvatarIndex + 1);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
  } else if (strcmp(argv[2], "-avatars") == 0) {
    strcpy(avatars, "");
    lastAvatarIndex = (int32) dirRecord->dir_LastAvatarIndex;
    for (avatarIndex = 0;
	 avatarIndex <= lastAvatarIndex;
	 avatarIndex++) {
      sprintf(oops, (avatarIndex == 0) ? "%d" : " %d",
	      dirRecord->dir_AvatarList[avatarIndex]);
      strcat(avatars, oops);
    }
    Tcl_SetResult(interp, avatars, TCL_VOLATILE);
  } else {
    sprintf(oops, "Unknown getfile option %s", argv[2]);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
    return TCL_ERROR;
  }
  return TCL_OK;
}

int32
cmdNameOf(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
  int32 bufferAddress;
  DirectoryRecord *dirRecord;
  
  if (argc != 2) {
    Tcl_SetResult(interp, "Illegal nameof command", TCL_STATIC);
    return TCL_ERROR;
  }
  
  if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
    return TCL_ERROR;
  }
  if (bufferAddress == 0) {
    Tcl_SetResult(interp, "Null address", TCL_STATIC);
    return TCL_ERROR;
  }
  dirRecord = (DirectoryRecord *) ((Buffer *) bufferAddress)->contents;
  Tcl_SetResult(interp, dirRecord->dir_FileName, TCL_VOLATILE);
  return TCL_OK;
}

int32
cmdSortNamesOf(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
  struct sorter {
    char *arg;
    DirectoryRecord *dirRecord;
  } tempSort, *sortArray;
  int32 i, j, swap;
  int32 stringLen;
  int32 bufferAddress;
  char *result;
  if (argc < 2) {
    Tcl_SetResult(interp, "Usage: sortnamesof FILE1 FILE2 ...", TCL_STATIC);
    return TCL_ERROR;
  }
  sortArray = (struct sorter *) malloc((argc - 1) * sizeof (struct sorter));
  if (!sortArray) {
    Tcl_SetResult(interp, "Can't get workspace for sortnamesof!", TCL_STATIC);
    return TCL_ERROR;
  }
  stringLen = 0;
  for (i = 0 ; i < argc - 1; i++) {
    stringLen += strlen(argv[i+1]);
    sortArray[i].arg = argv[i+1];
    if (Tcl_ExprLong(interp, sortArray[i].arg, &bufferAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    sortArray[i].dirRecord = (DirectoryRecord *) ((Buffer *) bufferAddress)->contents;
  }
  result = malloc(stringLen + argc + 10);
  if (!result) {
    Tcl_SetResult(interp, "Can't get result string for sortnamesof!", TCL_STATIC);
    free((char *) sortArray);
    return TCL_ERROR;
  }
  for (i = argc - 3; i >= 0; i--) {
    swap = i;
    for (j = argc - 2; j > i; j--) {
      if (strcmp(sortArray[j].dirRecord->dir_FileName,
		 sortArray[swap].dirRecord->dir_FileName) < 0) {
	swap = j;
      }
    }
    if (swap != i) {
      tempSort = sortArray[i];
      sortArray[i] = sortArray[swap];
      sortArray[swap] = tempSort;
    }
  }
  strcpy(result, "{");
  strcat(result, sortArray[0].arg);
  for (i = 1 ; i < argc - 1; i++) {
    strcat(result, " ");
    strcat(result, sortArray[i].arg);
  }
  strcat(result, "}");
  Tcl_SetResult(interp, result, TCL_DYNAMIC);
  free((char *) sortArray);
  return TCL_OK;
}

int32
cmdSetAvatars(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
  long bufferAddress, avatar;
  Buffer *buffer;
  DirectoryRecord *dirRecord;
  long numAvatars, i, recordSize;
  char oops[256];
  
  if (argc < 3) {
    Tcl_SetResult(interp, "Illegal setavatars command", TCL_STATIC);
    return TCL_ERROR;
  }
  numAvatars = argc - 2;
  if (numAvatars > maxAvatars) {
    sprintf(oops, "More than %d avatars specified", maxAvatars);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
    return TCL_ERROR;
  }
  if (bufferAddress == 0) {
    Tcl_SetResult(interp, "Null address", TCL_STATIC);
    return TCL_ERROR;
  }
  buffer = (Buffer *) bufferAddress;
  dirRecord = (DirectoryRecord *) buffer->contents;
  recordSize = sizeof (DirectoryRecord) + sizeof (long) * (numAvatars - 1);
  dirRecord = (DirectoryRecord *) realloc(dirRecord, recordSize);
  if (!dirRecord) {
    Tcl_SetResult(interp,
		  "Could not reallocated directory record to add avatars",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  buffer->contents = dirRecord;
  buffer->size = buffer->used = recordSize;
  for (i = 0; i < numAvatars; i++) {
    if (Tcl_ExprLong(interp, argv[i+2], &avatar) != TCL_OK) {
      return TCL_ERROR;
    }
    dirRecord->dir_AvatarList[i] = avatar;
  }
  dirRecord->dir_LastAvatarIndex = numAvatars - 1;
  return TCL_OK;
}

int32
cmdEcho(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    int32 i;

    for (i = 1; ; i++) {
	if (argv[i] == NULL) {
	    if (i != argc) {
		echoError:
		sprintf(interp->result,
		    "argument list wasn't properly NULL-terminated in \"%s\" command",
		    argv[0]);
	    }
	    break;
	}
	if (i >= argc) {
	    goto echoError;
	}
	fputs(argv[i], stdout);
	if (i < (argc-1)) {
	    printf(" ");
	}
    }
    printf("\n");
#ifdef applec
	fflush(stdout);
#endif
    return TCL_OK;
}

int32
cmdNewBuffer(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 bufferSize;
    Buffer *b;
    char *contents;

    if (argc != 2) {
      Tcl_SetResult(interp, "Illegal newbuffer command", TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &bufferSize) != TCL_OK) {
      return TCL_ERROR;
    }
    if ((b = (Buffer *) calloc(1, sizeof (Buffer))) == NULL) {
      Tcl_SetResult(interp, "Can't allocate buffer header", TCL_STATIC);
      return TCL_ERROR;
    }
    if ((contents = (char *) calloc(1, bufferSize)) == NULL) {
      sprintf(oops, "Can't allocate a %d-byte buffer", bufferSize);
      Tcl_SetResult(interp, oops, TCL_VOLATILE);
      return TCL_ERROR;
    }
    b->size = bufferSize;
    b->contents = contents;
    sprintf(oops, "0x%lX", (long) b);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
    return TCL_OK;
}

int32
cmdKillBuffer(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 bufferAddress;
    Buffer *b;

    if (argc != 2) {
      Tcl_SetResult(interp, "Usage: killbuffer BUFFER", TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    b = (Buffer *) bufferAddress;
#ifndef FREERETURNSRESULT
    free(b->contents);
    free(b);
#else
    if (!free(b->contents)) {
      sprintf(oops, "Could not free contents region of buffer 0x%lx", b);
      Tcl_SetResult(interp, oops, TCL_VOLATILE);
      Tcl_UnixError(interp);
      return TCL_ERROR;
    }
    if (!free(b)) {
      sprintf(oops, "Could not free buffer 0x%lx", b);
      Tcl_SetResult(interp, oops, TCL_VOLATILE);
      Tcl_UnixError(interp);
      return TCL_ERROR;
    }
#endif
    return TCL_OK;
}

int32
cmdReallocBuffer(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 bufferAddress;
    int32 bufferSize;
    Buffer *b;
    char *contents;

    if (argc != 3) {
      Tcl_SetResult(interp, "Illegal reallocbuffer command", TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[2], &bufferSize) != TCL_OK) {
      return TCL_ERROR;
    }
    b = (Buffer *) bufferAddress;
    if ((contents = (char *) realloc(b->contents, bufferSize)) == NULL) {
      sprintf(oops, "Can't reallocate a %d-byte buffer to %d bytes",
	      b->size, bufferSize);
      Tcl_SetResult(interp, oops, TCL_VOLATILE);
      return TCL_ERROR;
    }
    b->contents = contents;
    while (b->size < bufferSize) {
#ifdef NOBUFFER
	  b->size++;
#else
      contents[b->size++] = '\0';
#endif
    }
    sprintf(oops, "0x%lX", (long) b);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
    return TCL_OK;
}

int32
cmdBufferRemaining(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 bufferAddress;
    int32 bufferSize;
    Buffer *b;
    char *contents;

    if (argc != 2) {
      Tcl_SetResult(interp, "Usage: bufferremaining BUFFER", TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    b = (Buffer *) bufferAddress;
    sprintf(oops, "%d", b->size - b->used);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
    return TCL_OK;
}

int32
cmdBufferUsed(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 bufferAddress;
    int32 bufferSize;
    Buffer *b;
    char *contents;

    if (argc != 2) {
      Tcl_SetResult(interp, "Usage: bufferused BUFFER", TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    b = (Buffer *) bufferAddress;
    sprintf(oops, "%d", b->used);
    Tcl_SetResult(interp, oops, TCL_VOLATILE);
    return TCL_OK;
}

int32
cmdStuffBytes(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 bufferAddress, dataAddress, dataSize, dataIndex;
    Buffer *dest, *source;

    if (argc < 3 || argc > 4) {
      Tcl_SetResult(interp,
		    "Usage: stuffbytes DESTBUFFER SOURCEBUFFER [BYTECOUNT]",
		    TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    dest = (Buffer *) bufferAddress;
    if (Tcl_ExprLong(interp, argv[2], &dataAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    source = (Buffer *) dataAddress;
    if (argc == 4) {
      if (Tcl_ExprLong(interp, argv[3], &dataSize) != TCL_OK) {
	return TCL_ERROR;
      }
    } else {
      dataSize = source->used;
    }
    if (dataSize < 0) {
      Tcl_SetResult(interp, "Negative stuffbytes count", TCL_STATIC);
      return TCL_ERROR;
    }
    if (dest->size - dest->used < dataSize) {
      sprintf(oops, "Need %ld bytes in buffer 0x%lx, only %ld available",
	      dataSize, bufferAddress, dest->size - dest->used);
      Tcl_SetResult(interp, oops, TCL_VOLATILE);
      return TCL_ERROR;
    }
    dataIndex = 0;
    while (--dataSize >= 0) {
#ifdef NOBUFFER
      dest->used++;
#else
      ((char *) dest->contents)[dest->used++] =
	   ((char *) source->contents)[dataIndex++];
#endif
    }
    return TCL_OK;
}

int32
cmdStuffZeroes(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 bufferAddress, dataSize, dataIndex, byteVal, useString, useInt;
    char *stringP;
    int32 stringLen, stringIndex;
    Buffer *dest, *source;

    if (argc < 3 || argc > 4) {
      Tcl_SetResult(interp,
		    "Usage: stuffzeroes BUFFER BYTECOUNT [BYTEVAL]",
		    TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    dest = (Buffer *) bufferAddress;
    if (Tcl_ExprLong(interp, argv[2], &dataSize) != TCL_OK) {
      return TCL_ERROR;
    }
    if (dataSize < 0) {
      Tcl_SetResult(interp, "Negative stuffzeroes count", TCL_STATIC);
      return TCL_ERROR;
    }
    byteVal = 0x0000;
    stringP = (char *) &byteVal;
    stringLen = 1;
    if (argc == 4) {
      if (Tcl_ExprLong(interp, argv[3], &byteVal) != TCL_OK) {
	stringP = argv[3];
	stringLen = strlen(stringP);
      } else {
	if ((byteVal & 0xFFFFFF00)) {
	  stringLen = 4;
	} else {
	  stringP += 3;
	}
      }
    }
    if (dest->size - dest->used < dataSize) {
      sprintf(oops, "Need %ld bytes in buffer 0x%lx, only %ld available",
	      dataSize, bufferAddress, dest->size - dest->used);
      Tcl_SetResult(interp, oops, TCL_VOLATILE);
      return TCL_ERROR;
    }
    dataIndex = 0;
    stringIndex = 0;
    while (--dataSize >= 0) {
#ifdef NOBUFFER
      dest->used++;
#else
      ((char *) dest->contents)[dest->used++] = stringP[stringIndex];
      if (stringLen > 1) {
	stringIndex = (stringIndex + 1) % stringLen;
      }
#endif
    }
    return TCL_OK;
}

int32
cmdStuffLongword(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
    char oops[256];
    int32 bufferAddress, dataOffset, dataValue;
    Buffer *dest, *source;

    if (argc != 4) {
      Tcl_SetResult(interp,
		    "Usage: stufflongword BUFFER OFFSET VALUE",
		    TCL_STATIC);
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[1], &bufferAddress) != TCL_OK) {
      return TCL_ERROR;
    }
    dest = (Buffer *) bufferAddress;
    if (Tcl_ExprLong(interp, argv[2], &dataOffset) != TCL_OK) {
      return TCL_ERROR;
    }
    if (Tcl_ExprLong(interp, argv[3], &dataValue) != TCL_OK) {
      return TCL_ERROR;
    }
    if (dataOffset < 0 || (dataOffset % 4) != 0 ||
	dataOffset > dest->used + 4) {
      Tcl_SetResult(interp, "Illegal stufflongword offset", TCL_STATIC);
      return TCL_ERROR;
    }
    memcpy(dataOffset + (char *) dest->contents, (char *) &dataValue, 4);
    return TCL_OK;
}

int
cmdSetType(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
  char oops[256];
  long int byteCount, hasBytes;
  short macRefNum;
  FILE *theFile;
#ifdef applec
  HParamBlockRec pb;
  FCBPBRec fpb;
  OSErr osErr;
  char errbuf[256];
  Str255 theName;
#endif
  if (argc != 4) {
    Tcl_SetResult(interp, "Usage: settype FILE FILETYPE CREATORCODE",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  if (FindTclFile(interp, argv[1], &theFile) != TCL_OK) {
    return TCL_ERROR;
  }
#ifdef applec
  	if (ioctl(fileno(theFile), FIOREFNUM, (long *) &macRefNum) == -1) {
      Tcl_SetResult(interp, "ioctl error", TCL_STATIC);
      Tcl_UnixError(interp);
      return TCL_ERROR;
	}
	fpb.ioNamePtr = theName;
	fpb.ioRefNum = macRefNum;
	fpb.ioVRefNum = 0;
	fpb.ioFCBIndx = 0;
	osErr = PBGetFCBInfo(&fpb, 0);
#ifdef DEBUG
	fprintf(stderr, "Getting FCB info\n");
#endif
	if (osErr == noErr) {
 		pb.fileParam.ioNamePtr = theName;
		pb.fileParam.ioVRefNum = fpb.ioFCBVRefNum;
		pb.fileParam.ioDirID = fpb.ioNamePtr;
		pb.fileParam.ioDirID = fpb.ioFCBParID;
		pb.fileParam.ioFDirIndex = 0;
#ifdef DEBUG		
		fprintf(stderr, "Getting file info\n");
#endif
		osErr = PBHGetFInfo(&pb, 0);
		if (osErr == noErr) {
			pb.fileParam.ioFlFndrInfo.fdType = 0x494D4147;
			pb.fileParam.ioFlFndrInfo.fdCreator = 0x71746F70;
#ifdef DEBUG
			fprintf(stderr, "Setting file info\n");
#endif
 			pb.fileParam.ioNamePtr = theName;
			pb.fileParam.ioVRefNum = fpb.ioFCBVRefNum;
			pb.fileParam.ioDirID = fpb.ioNamePtr;
			pb.fileParam.ioDirID = fpb.ioFCBParID;
			pb.fileParam.ioFDirIndex = 0;
			osErr = PBHSetFInfo(&pb, 0);
		}
	}
	if (osErr != noErr) {
		sprintf(errbuf, "O/S error %d setting type", osErr);
		Tcl_SetResult(interp, errbuf, TCL_VOLATILE);
		return TCL_ERROR;
	}
#endif
  return TCL_OK;
}

int
cmdAllocPhysical(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
  char oops[256];
  long int byteCount, hasBytes;
  FILE *theFile;
#ifdef applec
  short macRefNum;
  ParamBlockRec pb;
  OSErr osErr;
  char errbuf[256];
#endif
  if (argc != 3) {
    Tcl_SetResult(interp, "Usage: allocphysical FILE BYTECOUNT",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  if (FindTclFile(interp, argv[1], &theFile) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[2], &byteCount) != TCL_OK) {
    return TCL_ERROR;
  }
#ifdef applec
  fseek(theFile, 0L, SEEK_END);
  hasBytes = ftell(theFile);
  if (hasBytes < byteCount) {
  	if (ioctl(fileno(theFile), FIOREFNUM, (long *) &macRefNum) == -1) {
      Tcl_SetResult(interp, "ioctl error", TCL_STATIC);
      Tcl_UnixError(interp);
      return TCL_ERROR;
	}
	pb.ioParam.ioRefNum = macRefNum;
	pb.ioParam.ioReqCount = byteCount;
	osErr = PBAllocContig(&pb, 0);
	if (osErr != noErr) {
		osErr = PBAllocate(&pb, 0);
	}
	if (osErr != noErr) {
		sprintf(errbuf, "O/S error %d allocating space", osErr);
		Tcl_SetResult(interp, errbuf, TCL_VOLATILE);
		return TCL_ERROR;
	}
  }
#endif
  return TCL_OK;
}

int
cmdSetEOF(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int argc;
    char *argv[];
{
  char oops[256];
  long int byteCount, hasBytes;
  short macRefNum;
  FILE *theFile;
  if (argc != 3) {
    Tcl_SetResult(interp, "Usage: seteof FILE BYTECOUNT",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  if (FindTclFile(interp, argv[1], &theFile) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[2], &byteCount) != TCL_OK) {
    return TCL_ERROR;
  }
#ifdef applec
  fseek(theFile, 0L, SEEK_END);
  hasBytes = ftell(theFile);
  if (hasBytes < byteCount) {
  	if (ioctl(fileno(theFile), FIOREFNUM, (long *) &macRefNum) == -1) {
      Tcl_SetResult(interp, "ioctl error", TCL_STATIC);
      Tcl_UnixError(interp);
      return TCL_ERROR;
	}
	if (SetEOF(macRefNum, byteCount) != noErr) {
      Tcl_SetResult(interp, "SetEOF error", TCL_STATIC);
      return TCL_ERROR;
	}
  }
#endif
  return TCL_OK;
}

int32
cmdWriteBuffer(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
  char oops[256];
  int32 bufferAddress, fileAddress, dataSize, offset;
  Buffer *b;
  char *data;
  FILE *theFile;
#ifdef applec
  short macRefNum;
  ParamBlockRec pb;
  OSErr osErr;
  char errbuf[256];
#endif
  if (argc != 4) {
    Tcl_SetResult(interp, "Usage: writebuffer FILE OFFSET BUFFER",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  if (FindTclFile(interp, argv[1], &theFile) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[2], &offset) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[3], &bufferAddress) != TCL_OK) {
    return TCL_ERROR;
  }
  b = (Buffer *) bufferAddress;
  fseek(theFile, offset, SEEK_SET);
#ifdef applec
  	if (ioctl(fileno(theFile), FIOREFNUM, (long *) &macRefNum) == -1) {
      Tcl_SetResult(interp, "ioctl error", TCL_STATIC);
      Tcl_UnixError(interp);
      return TCL_ERROR;
	}
#ifdef FASTWRITE
  memset(&pb, 0, sizeof pb);
  pb.ioParam.ioRefNum = macRefNum;
  pb.ioParam.ioBuffer = b->contents;
  pb.ioParam.ioReqCount = b->used;
  pb.ioParam.ioPosOffset = offset;
  pb.ioParam.ioPosMode = fsFromStart | 0x20; /* set the don't-cache bit */
  osErr = PBWrite(&pb, false);
#else
  osErr = SetFPos(macRefNum, fsFromStart, offset);
  if (osErr != noErr) {
    Tcl_SetResult(interp, "SetFPos error", TCL_STATIC);
    errno = osErr;
    Tcl_UnixError(interp);
    return TCL_ERROR;
  }
  dataSize = b->used;
  osErr = FSWrite(macRefNum, &dataSize, b->contents);
#endif
  if (osErr != noErr) {
    Tcl_SetResult(interp, "FSWrite error", TCL_STATIC);
    errno = osErr;
    Tcl_UnixError(interp);
    return TCL_ERROR;
  }
  return TCL_OK;
#else
  if (fwrite(b->contents, b->used, 1, theFile) == 1) {
    return TCL_OK;
  }
  Tcl_SetResult(interp, "Unix I/O error", TCL_STATIC);
  Tcl_UnixError(interp);
  return TCL_ERROR;
#endif
}

int32
cmdReadBuffer(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
  char oops[256];
  int32 bufferAddress, fileAddress, dataSize, offset;
  Buffer *b;
  char *data;
  FILE *theFile;
#ifdef applec
  short macRefNum;
  ParamBlockRec pb;
  OSErr osErr;
  char errbuf[256];
#endif
  if (argc != 4) {
    Tcl_SetResult(interp, "Usage: readbuffer FILE OFFSET BUFFER",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  if (FindTclFile(interp, argv[1], &theFile) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[2], &offset) != TCL_OK) {
    return TCL_ERROR;
  }
  if (Tcl_ExprLong(interp, argv[3], &bufferAddress) != TCL_OK) {
    return TCL_ERROR;
  }
  b = (Buffer *) bufferAddress;
  b->used = 0;
#ifdef applec
  	if (ioctl(fileno(theFile), FIOREFNUM, (long *) &macRefNum) == -1) {
      Tcl_SetResult(interp, "ioctl error", TCL_STATIC);
      Tcl_UnixError(interp);
      return TCL_ERROR;
	}
  osErr = SetFPos(macRefNum, fsFromStart, offset);
  if (osErr != noErr) {
    Tcl_SetResult(interp, "SetFPos error", TCL_STATIC);
    errno = osErr;
    Tcl_UnixError(interp);
    return TCL_ERROR;
  }
  dataSize = b->size;
  osErr = FSRead(macRefNum, &dataSize, b->contents);
  if (osErr != noErr) {
    Tcl_SetResult(interp, "FSRead error", TCL_STATIC);
    errno = osErr;
    Tcl_UnixError(interp);
    return TCL_ERROR;
  }
  b->used = b->size;
  return TCL_OK;
#else
  fseek(theFile, offset, SEEK_SET);
  if (fread(b->contents, 1, b->size, theFile) == b->size) {
    b->used = b->size;
    return TCL_OK;
  }
  sprintf(oops, "Read error %d, got %d\n", errno, b->size);
  Tcl_SetResult(interp, oops, TCL_VOLATILE);
  Tcl_UnixError(interp);
  return TCL_ERROR;
#endif
}

int32
cmdRandom(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
  char oops[256];
  static int32 randomInitialized = 0;
#ifdef applec
  unsigned long int timestamp_now;
#else
  static struct timeb timestamp_now;
#endif
  if (argc != 1) {
    Tcl_SetResult(interp, "Usage: random",
		  TCL_STATIC);
    return TCL_ERROR;
  }
  if (!randomInitialized) {
#ifdef applec
    GetDateTime(&timestamp_now);
	srand(timestamp_now);
	(void) rand();
	(void) rand();
#else
    (void) ftime(&timestamp_now);
    (void) initstate(0, (char *) &timestamp_now, sizeof timestamp_now);
    (void) random();
    (void) random();
    (void) random();
#endif
    randomInitialized = 1;
  }
#ifdef applec
  sprintf(oops, "%ld", rand() ^ ((long) rand()) << 15);
#else
  sprintf(oops, "%ld", random());
#endif
  Tcl_SetResult(interp, oops, TCL_VOLATILE);
  return TCL_OK;
}

int32
cmdBusy(clientData, interp, argc, argv)
    ClientData *clientData;
    Tcl_Interp *interp;
    int32 argc;
    char *argv[];
{
  char oops[256];
  static int32 randomInitialized = 0;
#ifdef applec
  unsigned long int timestamp_now;
#else
  static struct timeb timestamp_now;
#endif
  if (argc != 1) {
    Tcl_SetResult(interp, "Usage: busy",
		  TCL_STATIC);
    return TCL_ERROR;
  }
#ifdef applec
	RotateCursor(32);
#endif
	return TCL_OK;
}


int32
main(argc, argv, envp)
int32	argc;
char	*argv[];
char	*envp[];
{
    char line[1000], *cmd;
    int32 result, gotPartial;

    DBUG(("Layout program startup\n"));

#ifdef macintosh
	{
	int		i;
	char	*ptr;
	
	for (i=0; envp[i] != NULL ; i++)
		{
		for (ptr = envp[i] ; *ptr++ ; )
			;
		*ptr = '=';
		}
	
	environ = envp;
	}
#endif

    interp = Tcl_CreateInterp();
    DBUG(("Interpreter created\n"));

    Tcl_CreateCommand(interp, "echo", (Tcl_CmdProc *) cmdEcho, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "newfile", (Tcl_CmdProc *) cmdNewFile, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "deletefile", (Tcl_CmdProc *) cmdDeleteFile, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "newlabel", (Tcl_CmdProc *) cmdNewLabel, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "setroot", (Tcl_CmdProc *) cmdSetRoot, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "nameof", (Tcl_CmdProc *) cmdNameOf, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "setavatars", (Tcl_CmdProc *) cmdSetAvatars, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "newbuffer", (Tcl_CmdProc *) cmdNewBuffer, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "killbuffer", (Tcl_CmdProc *) cmdKillBuffer, (ClientData) 0,
	    (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "reallocbuffer", (Tcl_CmdProc *) cmdReallocBuffer,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "bufferremaining", (Tcl_CmdProc *) cmdBufferRemaining,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "bufferused", (Tcl_CmdProc *) cmdBufferUsed,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "avatarlimit", (Tcl_CmdProc *) cmdAvatarLimit,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "stuffbytes", (Tcl_CmdProc *) cmdStuffBytes,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "stuffzeroes", (Tcl_CmdProc *) cmdStuffZeroes,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "stufflongword", (Tcl_CmdProc *) cmdStuffLongword,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "setfile", (Tcl_CmdProc *) cmdSetFile,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "getfile", (Tcl_CmdProc *) cmdGetFile,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "readbuffer", (Tcl_CmdProc *) cmdReadBuffer,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "writebuffer", (Tcl_CmdProc *) cmdWriteBuffer,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "allocphysical", (Tcl_CmdProc *) cmdAllocPhysical,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "settype", (Tcl_CmdProc *) cmdSetType,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "seteof", (Tcl_CmdProc *) cmdSetEOF,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "random", (Tcl_CmdProc *) cmdRandom,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "busy", (Tcl_CmdProc *) cmdBusy,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "sortnamesof", (Tcl_CmdProc *) cmdSortNamesOf,
		      (ClientData) 0, (Tcl_CmdDeleteProc *) NULL);

    DBUG(("Commands registered\n"));

    buffer = Tcl_CreateCmdBuf();
    DBUG(("Command buffer created, running init command\n"));

    result = Tcl_Eval(interp, initCmd, 0, (char **) NULL);

    if (result != TCL_OK) {
	printf("%s\n", interp->result);
	exit(1);
    }

    DBUG(("Entering command loop\n"));

    gotPartial = 0;
    while (1) {
#ifdef applec
	RotateCursor(32);
#else applec
	clearerr(stdin);
#endif
	if (fgets(line, 1000, stdin) == NULL) {
	    if (!gotPartial) {
		exit(0);
	    }
	    line[0] = 0;
	}
	cmd = Tcl_AssembleCmd(buffer, line);
	if (cmd == NULL) {
	    gotPartial = 1;
	    continue;
	}

	gotPartial = 0;
	result = Tcl_RecordAndEval(interp, cmd, 0);
	if (result == TCL_OK) {
#ifdef ECHO_ALL_RESULTS
	    if (*interp->result != 0) {
		printf("%s\n", interp->result);
	    }
#endif
	    if (quitFlag) {
		Tcl_DeleteInterp(interp);
		Tcl_DeleteCmdBuf(buffer);
#ifdef TCL_MEM_DEBUG
		Tcl_DumpActiveMemory(stdout);
#endif
		exit(0);
	    }
	} else {
	    if (result == TCL_ERROR) {
		printf("Error");
	    } else {
		printf("Error %d", result);
	    }
	    if (*interp->result != 0) {
		printf(": %s\n", interp->result);
	    } else {
		printf("\n");
	    }
	}
    }
}
