/* $Id: readdiskstream.c,v 1.1 1994/07/07 18:57:28 vertex Exp $ */

#include "filefolio_lib.h"


/*****************************************************************************/


FolioGlue(ReadDiskStream,
	  -2,
	  (Stream *theStream, char *buffer, int32 nBytes),
	  (theStream, buffer, nBytes),
	  int32)
