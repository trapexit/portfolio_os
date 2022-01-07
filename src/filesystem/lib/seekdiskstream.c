/* $Id: seekdiskstream.c,v 1.1 1994/07/07 18:57:28 vertex Exp $ */

#include "filefolio_lib.h"


/*****************************************************************************/


FolioGlue(SeekDiskStream,
	  -3,
	  (Stream *theStream, int32 offset, enum SeekOrigin whence),
	  (theStream, offset, whence),
	  int32)
