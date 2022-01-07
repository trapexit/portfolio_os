/* $Id: opendiskstream.c,v 1.1 1994/07/07 18:57:28 vertex Exp $ */

#include "filefolio_lib.h"


/*****************************************************************************/


FolioGlue(OpenDiskStream,
          -1,
          (char *theName, int32 bSize),
          (theName, bSize),
          Stream*)
