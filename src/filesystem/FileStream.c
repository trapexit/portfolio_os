/*****

$Id: FileStream.c,v 1.14 1995/01/12 00:44:31 shawn Exp $

$Log: FileStream.c,v $
 * Revision 1.14  1995/01/12  00:44:31  shawn
 * Check for null pointers in several routines.
 * This should decrease number of lowmem aborts.
 *
 * Revision 1.13  1993/07/28  02:30:50  dplatt
 * Zero out IOReqs;  do delete-item cleanup
 *
 * Revision 1.12  1993/07/12  17:44:35  dplatt
 * Fix bug (actually fixed long ago, and the fix got lost somehow,
 * presumably in the RCS lossage) in which two SeekDiskStream() calls with
 * SEEK_CUR specifiers, without an intervening ReadDiskStream(), result in
 * unexpected behavior.  Net effect of the bug is that only the last
 * such SeekDiskStream() prior to a ReadDiskStream() has any effect... the
 * former ones are lost.
 *
 * Revision 1.11  1993/07/03  03:21:29  dplatt
 * Initial buffer-filling read should not try to read past physical
 * end of file 'cause some devices will reject the request.  Set initial
 * read size to the lesser of (buffer size, file physical size).
 *
 * Revision 1.10  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.9  1993/05/08  01:08:14  dplatt
 * Add flat-file-system/NVRAM support, and recover from RCS bobble
 *
 * Revision 1.6  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
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
 * Revision 1.1  1992/09/11  00:42:28  dplatt
 * Initial revision
 *
 * Revision 1.1  1992/09/10  19:14:52  dplatt
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
  FileStream.c - library code for reading a file in bytestream
  mode.
*/

#undef SUPER

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
#include "filestream.h"

#include "filefunctions.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"
#include "stdio.h"

/* #define DEBUG */
/* #define DEBUG1 */
/* #define DEBUG2 */
/* #define DEBUG3 */

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

#ifdef DEBUG1
#define DBUG1(x) printf x
#else
#define DBUG1(x) /* x */
#endif

#ifdef DEBUG2
#define DBUG2(x) printf x
#else
#define DBUG2(x) /* x */
#endif

extern Item Open(char *theName);

Stream *OpenDiskStream(char *theName, int32 bSize)
{
  Stream *theStream;
  Item openFileItem, ioReqItem;
  char *theBuffer;
  IOInfo theInfo;
  int32 minBlocks;
  uint32 bufferSize;
  FileStatus fileStatus;
  TagArg ioReqTags[2];
#ifdef DEBUG
  printf("Open file stream for %s\n", theName);
#endif
  theStream = (Stream *) ALLOCMEM(sizeof (Stream), MEMTYPE_FILL);
  if (!theStream) {
    goto returnNull;
  }
  openFileItem = OpenDiskFile(theName);
  if (openFileItem < 0) {
    goto releaseStream;
  }
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) openFileItem;
  ioReqTags[1].ta_Tag = TAG_END;
  ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    goto releaseOpenFile;
  }
  memset(&theInfo, 0, sizeof theInfo);
  theInfo.ioi_Command = CMD_STATUS;
  theInfo.ioi_Flags = IO_QUICK;
  theInfo.ioi_Recv.iob_Buffer = (void *) &fileStatus;
  theInfo.ioi_Recv.iob_Len = sizeof fileStatus;
  fileStatus.fs.ds_DeviceBlockSize = 2048; /* There is no sanity clause */
  fileStatus.fs.ds_DeviceBlockCount = 0;
  if (0 != DoIO(ioReqItem, &theInfo)) {
    goto releaseIoReq;
  }
  minBlocks =
    (fileStatus.fs.ds_DeviceBlockCount > FILESTREAM_BUFFER_MIN_BLOCKS) ? 
      FILESTREAM_BUFFER_MIN_BLOCKS : 1;
  if (bSize < 0) {
    bufferSize = -bSize;
    bufferSize = ((bufferSize <= fileStatus.fs.ds_DeviceBlockCount) ?
		  bufferSize : fileStatus.fs.ds_DeviceBlockCount) *
		    fileStatus.fs.ds_DeviceBlockSize;
  } else if (bSize == 0) {
    bufferSize = fileStatus.fs.ds_DeviceBlockSize * minBlocks;
  } else {
    bufferSize = bSize + fileStatus.fs.ds_DeviceBlockSize - 1;
    bufferSize -= bufferSize % fileStatus.fs.ds_DeviceBlockSize;
    if (bufferSize == 0) {
      bufferSize = fileStatus.fs.ds_DeviceBlockSize * minBlocks;
    }
  }
  if (bufferSize < FILESTREAM_BUFFER_MIN_BYTES) {
    bufferSize = FILESTREAM_BUFFER_MIN_BYTES;
  }
  theBuffer = (char *) ALLOCMEM((int32) bufferSize, MEMTYPE_DMA);
  if (!theBuffer) {
    goto releaseIoReq;
  }
#ifdef DEBUG
  printf("%d-byte buffer allocated at %lx\n", bufferSize, theBuffer);
#endif
/*
  Initialize stream structures
*/
  theStream->st_OpenFileItem = openFileItem;
  theStream->st_IOReq = (IOReq *) LookupItem(ioReqItem);
  theStream->st_Buffer = theBuffer;
  theStream->st_BufferLen = bufferSize;
  theStream->st_NextBufByteOffset = 0;
  theStream->st_BufBytesAvail = 0;
  theStream->st_FileOffset = 0;
  theStream->st_CursorPosition = 0;
  theStream->st_FileBlockSize = fileStatus.fs.ds_DeviceBlockSize;
  theStream->st_FileBlockCount = fileStatus.fs.ds_DeviceBlockCount;
  theStream->st_FileLength = fileStatus.fs_ByteCount;
  theStream->st_SeekOrigin = SEEK_NOT;
/*
  Kick off the first I/O to fill the buffer
*/
  theInfo.ioi_Command = CMD_READ;
  theInfo.ioi_Flags = 0;
  theInfo.ioi_Unit = 0;
  theInfo.ioi_Offset = 0;
  theInfo.ioi_Send.iob_Buffer = NULL;
  theInfo.ioi_Send.iob_Len = 0;
  theInfo.ioi_Recv.iob_Buffer = theBuffer;
  if (bufferSize > fileStatus.fs.ds_DeviceBlockSize * fileStatus.fs.ds_DeviceBlockCount) {
    theInfo.ioi_Recv.iob_Len = fileStatus.fs.ds_DeviceBlockSize * fileStatus.fs.ds_DeviceBlockCount;
  } else {
    theInfo.ioi_Recv.iob_Len = (int32) bufferSize;
  }
  if (0 != SendIO(ioReqItem, &theInfo)) {
    goto releaseBuffer;
  }
#ifdef DEBUG
  printf("First read initiated, %d bytes into %lx\n", bufferSize, theBuffer);
#endif
  theStream->st_IOInProgress = TRUE;
  theStream->st_HadError = FALSE;
  return theStream;
 releaseBuffer:
  FREEMEM(theBuffer, (int32) bufferSize);
 releaseIoReq:
  DeleteItem(ioReqItem);
 releaseOpenFile:
  CloseDiskFile(openFileItem);
 releaseStream:
  FREEMEM(theStream, sizeof (Stream));
 returnNull:
  return (Stream *) NULL;
}

void CloseDiskStream(Stream *theStream)
{

  if (theStream == NULL)
    return;

  if (theStream->st_IOInProgress) {
    (void) WaitIO(theStream->st_IOReq->io.n_Item);
  }
  DeleteItem(theStream->st_IOReq->io.n_Item);
  CloseDiskFile(theStream->st_OpenFileItem);
  FREEMEM(theStream->st_Buffer, (int32) theStream->st_BufferLen);
  FREEMEM(theStream, sizeof (Stream));
}

int32 ReadDiskStream(Stream *theStream, char *buffer, int32 nBytes)
{
  int32 takeNow, chew;
  int32 toFill, toEOF, toEOB, fillAt, seekDelta, seekPoint;
  uint32 err;
  IOInfo theInfo;
  char *origBuffer;
  origBuffer = buffer;

  if (theStream == NULL)
    return -1;

  switch (theStream->st_SeekOrigin) {
  case SEEK_NOT:
  case SEEK_CUR:
  case SEEK_END:
  case SEEK_SET:
    break;
  default:
    printf("Seek value corrupted: %x\n", theStream->st_SeekOrigin);
  }
/******* Why did I move this here??? It isn't right! 
  toEOF = theStream->st_FileLength - theStream->st_CursorPosition;
  if (nBytes > toEOF) {
    nBytes = toEOF;
  }
 *******/
/*
  If a readahead has been completed, handle it.  Force a wait-for-
  completion if we've been asked to seek to a different location in the
  file.
*/
 check_io_in_progress:
  if (theStream->st_IOInProgress) {
    if (theStream->st_SeekOrigin == SEEK_NOT) {
#ifdef DEBUG2
      printf("Stream I/O in progress; checking\n");
#endif
      if (CheckIO(theStream->st_IOReq->io.n_Item) == 0) goto check_finished;
    } else {
#ifdef DEBUG2
      printf("Stream I/O in progress, seeking %d; must sleep for it\n",
	      theStream->st_SeekOrigin);
#endif
      (void) WaitIO(theStream->st_IOReq->io.n_Item);
    }
  handle_endaction:
#ifdef DEBUG
    printf("Disk-stream endaction, error %d, actual %ld\n",
	    theStream->st_IOReq->io_Error,
	    theStream->st_IOReq->io_Actual);
#endif
#ifdef DEBUG1
    printf("@");
#endif
    theStream->st_BufBytesAvail += theStream->st_IOReq->io_Actual;
    theStream->st_FileOffset    += theStream->st_IOReq->io_Actual;
    theStream->st_IOInProgress   = FALSE;
    if (theStream->st_IOReq->io_Error != 0) {
      theStream->st_HadError = TRUE;
#ifdef DEBUG
      printf("Stream I/O error %x\n", theStream->st_IOReq->io_Error);
#endif
    }
  }
check_finished:
  if (theStream->st_SeekOrigin != SEEK_NOT) {
    DBUG2(("Seek required\n"));
    seekDelta = theStream->st_SeekTo - theStream->st_CursorPosition;
    if ((seekDelta >= 0 && seekDelta < theStream->st_BufBytesAvail) ||
	(seekDelta < 0 &&
	 (theStream->st_SeekTo / theStream->st_FileBlockSize ==
	  theStream->st_CursorPosition / theStream->st_FileBlockSize))) {
/*
  This seek falls within the bounds of the data currently known to be
  available in the buffer.  Adjust pointers, mark seek as completed,
  and continue onwards;
*/
      DBUG2(("Seek within buffer bounds\n"));
      theStream->st_BufBytesAvail  -= seekDelta;
      theStream->st_CursorPosition += seekDelta;
      theStream->st_NextBufByteOffset = (theStream->st_NextBufByteOffset +
					 seekDelta) % theStream->st_BufferLen;
      theStream->st_SeekOrigin = SEEK_NOT;
    } else {
/*
  We need to look elsewhere for the data we're seeking towards.  Find it
  by using an inelegant hack - reset the buffer pointers and cursor to
  the base of the page in which the desired data lives, mark the
  buffer as empty, and jump to the "followon" code at the end of this
  subroutine (the code which normally does a "hungry" buffer-filling
  readahead).  It will fire up the asynchronous read, notice that the
  SEEK mode is still set, and branch back to the beginning to wait
  for the I/O to complete.  We'll fall into the seek-is-in-progress
  code again, notice that the sought-after data is now in the buffer,
  and clean up as above.
*/
      DBUG2(("Seek outside buffer bounds... reset\n"));
      seekPoint = theStream->st_SeekTo - theStream->st_SeekTo %
	theStream->st_FileBlockSize;
      theStream->st_FileOffset = seekPoint;
      theStream->st_CursorPosition = seekPoint;
      theStream->st_BufBytesAvail = 0;
      theStream->st_NextBufByteOffset = 0;
      goto followon;
    }
  }
  toEOF = theStream->st_FileLength - theStream->st_CursorPosition;
  if (nBytes > toEOF) {
    nBytes = toEOF;
  }
  if (nBytes <= 0) {
    return 0;
  }
/*
  Take the entire remainder of the buffer if available and needed.
*/
  chew = (int32) (theStream->st_BufferLen - theStream->st_NextBufByteOffset);
  if (chew <= nBytes && chew <= theStream->st_BufBytesAvail) {
#ifdef DEBUG3
    printf("Transfer %d bytes to user buffer\n", chew);
#endif
    memcpy(buffer,
	   theStream->st_Buffer + theStream->st_NextBufByteOffset,
	   (int32) chew);
    theStream->st_NextBufByteOffset = 0;
    theStream->st_BufBytesAvail -= chew;
    theStream->st_CursorPosition += chew;
    nBytes -= chew;
    buffer += chew;
  }
  if (nBytes == 0) {
    goto followon;
  }
/*
  Take as much as we have in the buffer
*/
  takeNow = (int32) (nBytes <= theStream->st_BufBytesAvail ?
		   nBytes :  theStream->st_BufBytesAvail);
  if (takeNow > 0) {
#ifdef DEBUG3
    printf("Transfer %d bytes to user buffer\n", takeNow);
#endif
    memcpy(buffer,
	   theStream->st_Buffer + theStream->st_NextBufByteOffset,
	   (int32) takeNow);
    theStream->st_NextBufByteOffset += takeNow;
    theStream->st_BufBytesAvail -= takeNow;
    theStream->st_CursorPosition += takeNow;
    nBytes -= takeNow;
    buffer += takeNow;
    if (nBytes == 0) {
      goto followon;
    }
  }
/*
  The buffer is now entirely empty.  If we had an error on the last readahead,
  bail out.
*/
  if (theStream->st_HadError) {
    goto fini;
  }
/*
  If we had a readahead in progress, wait for it, then go back and work
  with whatever data it delivered to us.
*/
  if (theStream->st_IOInProgress) {
#ifdef DEBUG2
    printf("Need more data, awaiting I/O completion\n");
#endif
    (void) WaitIO(theStream->st_IOReq->io.n_Item);
    goto handle_endaction;
  }
/*
  "Please, sir, may I have more?"

  Transfer entire full blocks from the medium directly into the user's
  buffer, and wait for completion.
*/
  while (nBytes >= theStream->st_FileBlockSize) {
    chew = (int32) (nBytes - nBytes % theStream->st_FileBlockSize);
    memset(&theInfo, 0, sizeof theInfo);
    theInfo.ioi_Command = CMD_READ;
    theInfo.ioi_Offset = (int32) (theStream->st_FileOffset /
				theStream->st_FileBlockSize);
    theInfo.ioi_Recv.iob_Buffer = buffer;
    theInfo.ioi_Recv.iob_Len = (int32) chew;
#ifdef DEBUG2
    printf("Slurping %d bytes directly into user buffer\n", chew);
#endif
#ifdef DEBUG1
    printf("!");
#endif
    err = DoIO(theStream->st_IOReq->io.n_Item, &theInfo);
#ifdef DEBUG2
    printf("... actually got %d bytes\n", theStream->st_IOReq->io_Actual);
#endif
    takeNow = theStream->st_IOReq->io_Actual;
    nBytes -= takeNow;
    buffer += takeNow;
    theStream->st_FileOffset += takeNow;
    theStream->st_CursorPosition += takeNow;
    if (err != 0) {
      theStream->st_HadError = TRUE;
      goto fini;
    }
  }
/*
  Either we're done, or less than a block remains to fetch.  In either
  case, the circular buffer is empty.
*/
  theStream->st_NextBufByteOffset = 0;
  if (nBytes > 0) {
/*
  We need a fraction of a block.  Read just one block, into the beginning
  of the circular buffer.  We may end up getting less than a full block, if
  this is a remote-mounted file and we're trying to read beyond the logical
  end-of-file.
*/
#ifdef DEBUG
    printf("Buffer is now empty, need %d more, read & sleep\n", nBytes);
#endif
    memset(&theInfo, 0, sizeof theInfo);
    theInfo.ioi_Command = CMD_READ;
    theInfo.ioi_Offset = (int32) (theStream->st_FileOffset /
				theStream->st_FileBlockSize);
    theInfo.ioi_Recv.iob_Buffer = theStream->st_Buffer;
    theInfo.ioi_Recv.iob_Len = (int32) theStream->st_FileBlockSize;
#ifdef DEBUG1
    printf("!");
#endif
    err = DoIO(theStream->st_IOReq->io.n_Item, &theInfo);
    chew = theStream->st_IOReq->io_Actual;
    if (chew > nBytes) {
      chew = nBytes;
    }
#ifdef DEBUG2
    printf("Actually read %d, taking %d\n", theStream->st_IOReq->io_Actual,
	    chew);
#endif
    memcpy(buffer, theStream->st_Buffer, (int32) chew);
    theStream->st_FileOffset += theStream->st_IOReq->io_Actual;
    theStream->st_NextBufByteOffset = chew;
    theStream->st_BufBytesAvail = theStream->st_FileBlockSize - chew;
    theStream->st_CursorPosition += chew;
    buffer += chew;
    if (err != 0) {
      theStream->st_HadError = TRUE;
    }
  }
/*
  Once all requested data has been transferred, check to see if there's
  at least one block of free space available in the circular buffer.  If
  so, set up a readahead (unless there's already one in progress).  

  The readahead will consume the largest number of blocks which will
  fill to the end of the buffer or read up through the last block of the
  file, whichever comes first.
*/
 followon:
#ifdef DEBUG3
  printf("%d left in buffer, %d free\n", theStream->st_BufBytesAvail,
	  theStream->st_BufferLen - theStream->st_BufBytesAvail);
#endif
  if (!theStream->st_IOInProgress) {
    toFill = (int32) ((theStream->st_BufferLen - theStream->st_BufBytesAvail) /
		    theStream->st_FileBlockSize);
    if (toFill > 0) {
      toEOF = (int32) (theStream->st_FileBlockCount -
		     (theStream->st_FileOffset / theStream->st_FileBlockSize));
#ifdef DEBUG
      printf("%d blocks available in buffer, %d blocks to EOF\n",
	      toFill, toEOF);
#endif
      if (toEOF <= 0) {
	goto fini;
      }
      if (toEOF < toFill) {
	toFill = toEOF; 
      }
      fillAt = (int32) (theStream->st_NextBufByteOffset +
		      theStream->st_BufBytesAvail);
      if (fillAt < theStream->st_BufferLen) {
	toEOB = (int32) ((theStream->st_BufferLen - fillAt) /
		       theStream->st_FileBlockSize);
	if (toEOB < toFill) {
	  toFill = toEOB;
	}
      } else {
	fillAt -= (int32) theStream->st_BufferLen;
      }
#ifdef DEBUG
      printf("Next-buf-byte-offset %d, buf-bytes-avail %d\n",
	      theStream->st_NextBufByteOffset,
	      theStream->st_BufBytesAvail);
      printf("Fill point %d, to-EOB %d, to-fill %d\n",
	      fillAt, toEOB, toFill);
#endif
      memset(&theInfo, 0, sizeof theInfo);
      theInfo.ioi_Command = CMD_READ;
      theInfo.ioi_Offset = (int32) (theStream->st_FileOffset /
				  theStream->st_FileBlockSize);
      theInfo.ioi_Recv.iob_Buffer = theStream->st_Buffer + fillAt;
      theInfo.ioi_Recv.iob_Len = (int32) (toFill * theStream->st_FileBlockSize);
#ifdef DEBUG 
      printf("Starting readahead for %d bytes\n", theInfo.ioi_Recv.iob_Len);
#endif
#ifdef DEBUG1
    printf(">");
#endif
      if (0 != SendIO(theStream->st_IOReq->io.n_Item, &theInfo)) {
	return -1;
      }
      theStream->st_IOInProgress = TRUE;
    }
    if (theStream->st_SeekOrigin != SEEK_NOT && nBytes > 0) {
      goto check_io_in_progress; /* I really hate this */
    }
  }
 fini:
  return buffer - origBuffer;
}
  
int32 SeekDiskStream(Stream *theStream, int32 offset,
			enum SeekOrigin whence)
{

  if (theStream == NULL)
    return -1;

  switch (whence) {
  case SEEK_CUR:
    switch (theStream->st_SeekOrigin) {
    case SEEK_NOT:
      offset += theStream->st_CursorPosition;
      break;
    case SEEK_CUR:
    case SEEK_SET:
    case SEEK_END:
      offset += theStream->st_SeekTo;
      break;
    }
    break;
  case SEEK_END:
    offset = theStream->st_FileLength - offset;
    break;
  case SEEK_SET:
    break;
  default:
    return -1;
  }
  if (offset < 0 || offset > theStream->st_FileLength) {
    return -1;
  }
  theStream->st_SeekOrigin = whence;
  theStream->st_SeekTo = offset;
  return offset;
}
