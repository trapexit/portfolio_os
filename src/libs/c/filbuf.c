/* $Id: filbuf.c,v 1.4 1994/02/09 01:22:45 limes Exp $ */
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

int32 MacExpect(char *buff, int32 maxchars)
{
	IOInfo ioInfo;
	char *Prompt;
	if (InitMacIO() == 0)
	{
		DBUG1(("InitMacIO failed\n"));
		return 0;
	}
	
	Prompt = "";

	memset(&ioInfo,0,sizeof(ioInfo));
	ioInfo.ioi_Command = MACCMD_ASK;
	ioInfo.ioi_Send.iob_Buffer = Prompt;
	ioInfo.ioi_Recv.iob_Buffer = buff;
/*
	ioInfo.ioi_Offset = 0;
	ioInfo.ioi_Flags = 0;
	ioInfo.ioi_Unit = 0;
*/
	ioInfo.ioi_Recv.iob_Len = maxchars;
	ioInfo.ioi_Send.iob_Len = strlen(Prompt)+1;
 
	kprintf("\n");
	DoIO(_stdlib_MacIORItm,&ioInfo);
	kprintf("\n");
	if (_stdlib_MacIOReq->io_Error == 0)
	{
		/* make sure it is zero terminated */
		buff[_stdlib_MacIOReq->io_Actual] = 0;
	}
	else
	{
	    kprintf("Actual=%d\n",_stdlib_MacIOReq->io_Actual);
		kprintf("Error=%d ", _stdlib_MacIOReq->io_Error);
	}
	return (_stdlib_MacIOReq->io_Actual);
}

int
filbuf(FILE *stream)
{
	IOInfo ioInfo;
	long len;

	DBUG(("filbuf:"));
	len = stream->fcb_buffsize;
	if (stream == stdin)
	{
		len = (long) MacExpect( stream->fcb_buffer, (int) len);
		stream->fcb_buffer[len] = '\n';
		len++;
	}
	else
	{
	    if (len > stream->fcb_bytesleft) len = stream->fcb_bytesleft;
	    if (len == 0)	return EOF;

	    memset(&ioInfo,0,sizeof(ioInfo));
	    ioInfo.ioi_Command = CMD_READ;
	    ioInfo.ioi_Recv.iob_Buffer = stream->fcb_buffer;
	    ioInfo.ioi_Recv.iob_Len = len;
	    ioInfo.ioi_Send.iob_Buffer = stream->fcb_filename;
	    ioInfo.ioi_Send.iob_Len = strlen(stream->fcb_filename)+1L;
/*
	    ioInfo.ioi_Flags = 0;
	    ioInfo.ioi_Unit = 0;
*/
	    ioInfo.ioi_Offset = stream->fcb_currentpos;
	    DoIO(_stdlib_MacIORItm,&ioInfo);

	    if (_stdlib_MacIOReq->io_Error)
	    {
	        stream->fcb_numinbuf = 0;
	        kprintf("error during filbuf=%d of %s\n",
			_stdlib_MacIOReq->io_Error, stream->fcb_filename);
	        return EOF;
	    }
	}
	stream->fcb_numinbuf = len-1;
	stream->fcb_cp = stream->fcb_buffer+1;
	stream->fcb_bytesleft -= len;
	stream->fcb_currentpos += len;

	DBUG((" numinbuf=%d return:%lx\n",stream->fcb_numinbuf,*stream->fcb_buffer));
	return stream->fcb_buffer[0];
}
