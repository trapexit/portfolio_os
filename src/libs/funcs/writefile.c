/***************************************************************\
*								*
* General routine to write a file to the mac                    *
*								*
* By:  Stephen H. Landrum					*
* Mostly stolen from RJ's code                                  *
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
#include "strings.h"
#include "stdlib.h"
#include "stdio.h"
#include "operror.h"


static Item MacIO, MacDev, ReplyPort;
static IOInfo FileInfo;
IOReq *IOReqPtr;


int32
OpenMacLink( void )
/* Open the communications channel between the Opera and the Macintosh */
{
  TagArg targs[3];
  
  if ( (MacDev = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),"mac"),0) ) < 0 ) {
    printf( "Can't open mac device\n" );
    return -1;
  }
  
  targs[0].ta_Tag = TAG_ITEM_NAME;
  targs[0].ta_Arg = (void *)"reply port";
  targs[1].ta_Tag = TAG_END;
  
  if ( (ReplyPort = CreateItem(MKNODEID(KERNELNODE,MSGPORTNODE),targs)) < 0 ) {
    printf( "Can't create reply port.\n" );
    return -1;
  }
  
  targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  targs[0].ta_Arg = (void *)MacDev;
  targs[1].ta_Tag = CREATEIOREQ_TAG_REPLYPORT;
  targs[1].ta_Arg = (void *)ReplyPort;
  targs[2].ta_Tag = TAG_END;
  
  if ( (MacIO = CreateItem( MKNODEID (KERNELNODE, IOREQNODE), targs )) < 0 ) {
    printf( "Can't create IOReqPtr node.\n" );
    return -1;
  }
  IOReqPtr = (IOReq *)LookupItem( MacIO );

  memset (&FileInfo, 0, sizeof(FileInfo));
  
  return 0;
}





int32
WriteFile( char *filename, char *buf, uint32 count )
/* Reads count bytes from the filename file into 
 * the specified buffer.  Returns the actual length of 
 * the read, 
 */
{
  int32 j;
  int32 retvalue;
  
  retvalue = -1;
  FileInfo.ioi_Command = CMD_WRITE;
  FileInfo.ioi_Unit = 0;
  
  FileInfo.ioi_Send.iob_Buffer = buf;
  FileInfo.ioi_Send.iob_Len = (int)( count & ~3 );
  FileInfo.ioi_Recv.iob_Buffer = filename;
  FileInfo.ioi_Recv.iob_Len = strlen( filename ) + 1;

  FileInfo.ioi_Offset = 0;
  if ( (j = DoIO( MacIO, &FileInfo )) < 0 )
    {
      printf ("ERROR:  couldn't write Mac file %s : %ld ",
	       filename, j );
      printf ("($%lx), %ld ($%lx)\n",
	       j, IOReqPtr->io_Error, IOReqPtr->io_Error);
      printf( "SysErr() = " );
      PrintfSysErr( j );
    }
  else retvalue = IOReqPtr->io_Actual;
  
  return ( retvalue );
}




