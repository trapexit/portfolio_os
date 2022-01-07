/* $Id: fgetc.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */
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

/* get a character from the console or a file, BUFFERED */
int32 getc(FILE *stream)
{
	int32 c;
	
	CHECKSTREAM("getc", -1);

/*	DBUG(("getc: numinbuf=%d\n",stream->fcb_numinbuf)); */

	if (--stream->fcb_numinbuf < 0) c = filbuf(stream);
	else
	{
		c = *stream->fcb_cp;
		stream->fcb_cp++;
	}
/*	DBUG(("getc returns:%lx\n",c)); */
/*	DBUG(("%c,",c)); */
	return c;
}

int32 fgetc(FILE *stream)
{
	return getc(stream);
}

