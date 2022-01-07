
/******************************************************************************
**
**  $Id: FreeBuffer.c,v 1.5 1994/10/05 18:21:06 vertex Exp $
**
**  Lib3DO routine to free a file buffer.
**
**  There is probably no need for this function; all non-obsolete functions
**  which load files have corresponding Unload functions of their own.
**
******************************************************************************/


#include "mem.h"
#include "utils3do.h"

void FreeBuffer (char *filename, int32 *fileBuffer)
{
	int32	buffSize;

	if ((buffSize = GetFileSize(filename)) <= 0)
	  return;

	FREEMEM (fileBuffer, buffSize);
}
