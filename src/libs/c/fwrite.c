/* $Id: fwrite.c,v 1.4 1994/12/21 21:29:04 peabody Exp $ */
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

/* Routines added by PLB, modified by Dale */
int32 fwrite(const void *ptr, int32 size, int32 nitems, FILE *stream)
{

	int32 len = size*nitems;
	
	CHECKSTREAM("fwrite",0);
	
	if (!(stream->fcb_mode & FCB_WRITE_MODE)) return (0);
	
	if (stream == stdout)
	{
		return(0);
	}
	if (len == 0)	return 0;

/* Is there room in buffer? */
	if (stream->fcb_numinbuf >= len)
	{
	    /* just cache it */
	    bcopy(ptr,stream->fcb_cp,len);  /* @@@ gets a warning on ARM because bcopy() says it takes an int instead of an int32 */
	    stream->fcb_cp += len;
	    stream->fcb_numinbuf -= len;
	    return nitems; /* 00005 nitems, not len */
	}
	else
	{
		fflush(stream);
		return(internalfwrite(ptr, size, nitems, stream));
	}
}
