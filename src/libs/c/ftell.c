/* $Id: ftell.c,v 1.5 1994/02/09 01:22:45 limes Exp $ */
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

int32 ftell(FILE *stream)
{
    int32 buffcnt = (int32)(stream->fcb_cp - stream->fcb_buffer);
    CHECKSTREAM("ftell", -1);
    /*return (stream->fcb_currentpos - stream->fcb_numinbuf);*/
    if (stream->fcb_mode & FCB_WRITE_MODE)
	 return stream->fcb_currentpos + buffcnt;
    else return stream->fcb_currentpos - (int32)stream->fcb_numinbuf;
}

