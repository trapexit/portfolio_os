/***************************************************************\
*								*
* General routine to load a file        			*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  27-Jul-93					*
*								*
* Copyright (c) 1993, The 3DO Company, Inc.                     *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/



#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) /* { printf x ; } */
#define FSDBUG(x) /* printf x */

#include "types.h"

#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"

#include "driver.h"
#include "device.h"

#include "filesystem.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "directory.h"
#include "directoryfunctions.h"

#include "strings.h"
#include "stdlib.h"
#include "stdio.h"


static IOInfo ioInfo;
static Item IOReqItem;
static IOReq *ior;


int32
openmacdevice (void)
{
  return 0;
}


int32
getfilesize (char *name)
{
  Item fitem;
  OpenFile *fptr;
  int32 filesize;
  TagArg targs[3];
  int32 j;
  FileStatus statbuffer;

  fitem = OpenDiskFile (name);
  fptr = (OpenFile *)LookupItem (fitem);
  if (fptr) {
    targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
    targs[0].ta_Arg = (void *)(fptr->ofi.dev.n_Item);
    targs[1].ta_Tag = TAG_END;
    if ((IOReqItem=CreateItem(MKNODEID(KERNELNODE,IOREQNODE),targs)) < 0) {
      DBUG (("GETFILESIZE:  Unable to create IOReq node (%ld)\n", IOReqItem));
      return -1;
    }

    ior = (IOReq *)LookupItem(IOReqItem);

    memset (&ioInfo, 0, sizeof(ioInfo));

    ioInfo.ioi_Command = CMD_STATUS;
    ioInfo.ioi_Recv.iob_Buffer = &statbuffer;
    ioInfo.ioi_Recv.iob_Len = sizeof(statbuffer);
    ioInfo.ioi_Send.iob_Buffer = NULL;
    ioInfo.ioi_Send.iob_Len = 0;
    ioInfo.ioi_Offset = 0;

    if (((j=DoIO(IOReqItem,&ioInfo)) < 0) || (ior->io_Error!=0)) {
      DeleteItem (IOReqItem);
      DBUG (("GETFILESIZE:  Error in attempt to get file status\n"));
      return -1;
    }

    filesize = statbuffer.fs_ByteCount;
  } else {
    filesize = -1;
  }
  DeleteItem (IOReqItem);
  FSDBUG (("fitem = %lx, fptr->ofi.dev.n_Item = %lx\n", fitem, fptr->ofi.dev.n_Item));
  CloseDiskFile (fitem);

  return filesize;
}



void *
loadfile (char *name, void *buffer, uint32 buffersize, uint32 memtype)
{
  int32 filesize;
  Stream *fstream;

  filesize = getfilesize (name);

  if (filesize<0) {
    return NULL;
  }

  if (filesize==0) {
    DBUG (("Empty file: %s\n", name));
    return NULL;
  }

  if (buffer && filesize>buffersize) {
    DBUG (("File size exceeds buffer size (%ld > %ld): %s\n", filesize, buffersize, name));
    return NULL;
  }
  if (!buffer) {
    if (memtype) {
      buffer = ALLOCMEM ((int)(filesize), memtype);
    } else {
      buffer = malloc((int)(filesize));
    }
    if (!buffer) {
      DBUG (("Unable to allocate memory for file (%ld): %s\n", filesize, name));
      return NULL;
    }
  }

  fstream = OpenDiskStream (name, 0);
  if (ReadDiskStream(fstream,(char *)buffer,filesize) != filesize) {
    DBUG (("Error reading file %s\n", name));
    CloseDiskStream (fstream);
    return NULL;
  }

  CloseDiskStream (fstream);
  return buffer;
}


