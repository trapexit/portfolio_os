/* $Id: fread.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */
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

int32 fread(void *ptr, int32 size, int32 nitems, FILE *stream)
{

	int32 len = size*nitems;
	char *lptr = (char *)ptr;
	int32 sent;
	uchar *savebuff;
	int32 savelen;
	
	CHECKSTREAM("fread",0);
	
	if (!(stream->fcb_mode & FCB_READ_MODE)) return (0);

	DBUG(("fread(%lx,%d)\n",ptr,len));
	DBUG(("bytesleft=%d len=%d\n",stream->fcb_bytesleft,len));
	DBUG(("filename=%s\n",stream->fcb_filename));
	if (len == 0)	return 0;

again:
	sent = (int) min(len,stream->fcb_numinbuf);
	if (sent)
	{
	    /* satisfy it out of bytes already read*/
	    DBUG(("QUICK READ numinbuf=%d\n",stream->fcb_numinbuf));
	    bcopy(stream->fcb_cp,lptr,(int)sent);
	    stream->fcb_numinbuf -= sent;
	    stream->fcb_cp += sent;
	    len -= sent;
	    if (len == 0) return nitems;
	    lptr += sent;
	}

	/* should we continue to use buffer? */
	if (len < stream->fcb_buffsize)
	{	/* yes, grab another full block */
	    int c;
	    DBUG(("yes, grab another full block\n"));
	    c = filbuf(stream);
	    if (c == EOF)	return  sent/size;
	    stream->fcb_numinbuf++;
	    stream->fcb_cp--;
	    goto again;
	}
	if (stream->fcb_bytesleft < len)
	{
		len = stream->fcb_bytesleft; /* don't read past end */
		nitems = len / size;
		len = nitems * size;
	}
	/* swap buff ptrs and use filbuf */
	savebuff = stream->fcb_buffer;
	savelen = stream->fcb_buffsize;
	stream->fcb_buffer = lptr;
	stream->fcb_buffsize = len;
	filbuf(stream);
	stream->fcb_buffer = savebuff;
	stream->fcb_buffsize = savelen;

/* clear character level buffer */
	stream->fcb_numinbuf = 0;
	
	DBUG(("fread returning:%d\n",nitems));
	return nitems;
}
