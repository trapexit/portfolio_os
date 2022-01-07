/* $Id: eeprom.c,v 1.5 1994/02/18 01:54:55 limes Exp $ */
/***************************************************************\
*								*
* Boot a new version of OS from file
*								*
* By:  Dale
*								*
\***************************************************************/



#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) /* { printf x ; } */

#include "types.h"

#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
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
      DBUG (("GETFILESIZE:  Unable to create IOReq node (%d)\n", IOReqItem));
      return -1;
    }

    ior = (IOReq *)LookupItem(IOReqItem);

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
    return NULL;
    CloseDiskStream (fstream);
  }

  CloseDiskStream (fstream);
  return buffer;
}

Item eeprom;
Item eepromiori;

#define ROM_SIZE	1024*1024
#define BUF_SIZE	128*1024

int
main (argc, argv)
int argc;
char **argv;
{
    uint32 *image;
    uint32 *p;
    uint32 *q;
    int i,j;
    IOInfo ioInfo;
    uint32 *ver;
    int32 ret;

    DBUG(("Entering eeprom utility main\n"));

    eeprom = OpenNamedDevice("eeprom",0);
    if (eeprom < 0)
    {
	printf("no eeprom device\n");
	exit(-1);
    }
    eepromiori = CreateIOReq(0,0,eeprom,0);
    if (eepromiori < 0)
    {
	printf("could not create ioreq for eeprom\n");
	exit(-1);
    }

    printf("loading file: %s\n",argv[1]);
    image = (uint32 *)loadfile (argv[1], 0, 0, MEMTYPE_STARTPAGE);
    if (!image)
    {
	kprintf("error loading boot image\n");
	exit(-1);
    }

    memset(&ioInfo,0,sizeof(ioInfo));

    ioInfo.ioi_Send.iob_Buffer = image;
    ioInfo.ioi_Send.iob_Len = ROM_SIZE;

    printf("burn the eeprom\n");
    ret = DoIO(eepromiori,&ioInfo);

    printf("Done programming eeprom\n");

#ifdef VERIFY
/*
 * WARNING WARNING WARNING WARNING
 *
 * As of 1/14/94, Dale Luck says this verify code was never tested
 * AND DOES NOT WORK!!!
 *
 * WARNING WARNING WARNING WARNING
 */
    printf("verifying--\n");

    DeleteItem(eepromiori);
    CloseItem(eeprom);

    eeprom = OpenNamedDevice("ram",0);
    if (eeprom < 0)
    {
	printf("No Rom device\n");
	exit (-1);
    }
    eepromiori = CreateIOReq(0,0,eeprom,0);
    if (eepromiori < 0)
    {
	printf("could not create ioreq for rom\n");
	exit(-1);
    }

    ver = (uint32 *)ALLOCMEM(BUF_SIZE,0);

    memset(&ioInfo,0,sizeof(ioInfo));
    p = image;
    ioInfo.ioi_Recv.iob_Buffer = ver;
    ioInfo.ioi_Recv.iob_Len = BUF_SIZE;
    ioInfo.ioi_Unit = 2;
    for (i = 0 ; i  < ROM_SIZE/BUF_SIZE ; i++)
    {
	ret = DoIO(eepromiori,&ioInfo);
	if (ret < 0)
	{
	    PrintError(0,"\\error from DoIO to","rom",ret);
	    exit(-1);
	}
	q = ver;
	for (j = 0; j < BUF_SIZE/sizeof(uint32) ; j++)
	{
	    if (*p++ != *q++)
	    {
		printf("Verify error: src:%lx [%x] != rom[%lx]\n",p-1,*(p-1),*(q-1));
		exit (-1);
	    }
	}
	ioInfo.ioi_Offset += BUF_SIZE;
    }
    printf("Verify OK\n");
/*
 * NOTE: See notice above. Verify DOES NOT WORK.
 */
#endif

    return 0;
}

