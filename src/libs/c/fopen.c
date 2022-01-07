/* $Id: fopen.c,v 1.4 1994/02/09 01:22:45 limes Exp $ */
/******************************************************************
** "Standard I/O routines for Opera.
** NOTE: These may not conform exactly to what you expect but
** are pretty close.  Here are some known differences:
**   1) fopen() only supports "w", "r", and "a".
**
** 00001 PLB 8/27/92 Added routines to further conformance with stdio
** 00002 PLB 8/28/92 Added fwrite and fputs
** 00003 PLB 9/3/92  Automatically initialize on every call.
** 00004 Dale 9/8/92 Changed buffers to use pointers, etc.
** 00005 PLB 9/9/92 Removed fflush() from internalfwrite, reset _cp in fflush
**                  Optional memory allocation test. TESTALLOCMEM
** 00006 Dale 11/30/92 fixed fflush, it was reseting wrong bufptr
******************************************************************/

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "io.h"
#include "debug.h"
#include "strings.h"
#include "mem.h"
#include "item.h"
#include "stdio.h"

#include "stdlibcom.h"

/* Get filesize and reset position. */
FILE *fopen(char *filename, char *type)
{
	int32 ret;
	IOInfo ioInfo;
	FILE *stream;
	
	stream = AllocFileBlock(); /* 00001 */
	if (stream == 0) return 0;

	memset(&ioInfo,0,sizeof(ioInfo));

	DBUG1(("fopen(%s,%s)\n",filename,type));
	strcpy(stream->fcb_filename,filename);
	
	if (InitMacIO() == 0)
	{
	        TESTFREEMEM(stream,sizeof(FILE));
		DBUG1(("InitMacIO failed\n"));
		return 0;
	}

/* Open in w, r or a mode. */
	switch(type[0])
	{
		case 'r':
			ioInfo.ioi_Command = MACCMD_FILELEN;
			ioInfo.ioi_Recv.iob_Buffer = &stream->fcb_filesize;
			ioInfo.ioi_Recv.iob_Len = 4;
			ioInfo.ioi_Send.iob_Buffer = stream->fcb_filename;
			ioInfo.ioi_Send.iob_Len = strlen(filename)+1L;
			stream->fcb_mode = FCB_READ_MODE;
			break;
			
		case 'w':
			ioInfo.ioi_Command = MACCMD_WRITECR;
			ioInfo.ioi_Recv.iob_Buffer = stream->fcb_filename;
			ioInfo.ioi_Recv.iob_Len =  strlen(filename)+1L;
			ioInfo.ioi_Send.iob_Buffer = stream->fcb_buffer;
			ioInfo.ioi_Send.iob_Len = 0;
			stream->fcb_filesize = 0;
			stream->fcb_mode = FCB_WRITE_MODE;
			stream->fcb_numinbuf = stream->fcb_buffsize;
			break;
			
		case 'a':
			ioInfo.ioi_Command = MACCMD_APPEND;
			ioInfo.ioi_Recv.iob_Buffer = stream->fcb_filename;
			ioInfo.ioi_Recv.iob_Len =  strlen(filename)+1L;
			ioInfo.ioi_Send.iob_Buffer = stream->fcb_buffer;
			ioInfo.ioi_Send.iob_Len = 0;
			stream->fcb_filesize = 0;
			stream->fcb_numinbuf = stream->fcb_buffsize;
			stream->fcb_mode = FCB_WRITE_MODE;
			break;
			
		default:
			TESTFREEMEM(stream,sizeof(FILE));
			return 0;
	}
	
	ioInfo.ioi_Offset = 0;
	ioInfo.ioi_Flags = 0;
	ioInfo.ioi_Unit = 0;
	ret = DoIO(_stdlib_MacIORItm,&ioInfo);
	DBUG1(("ret=%lx error=%d filesize=%lx\n",\
		(long)ret,_stdlib_MacIOReq->io_Error,stream->fcb_filesize));
	if (_stdlib_MacIOReq->io_Error)
	{
	        TESTFREEMEM(stream,sizeof(FILE));
		return 0;
	}
	
	stream->fcb_currentpos = 0;
	stream->fcb_bytesleft = stream->fcb_filesize;
	
	return stream;
}
