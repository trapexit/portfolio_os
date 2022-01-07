/* $Id: executeasthread.c,v 1.1 1994/07/07 18:57:28 vertex Exp $ */

#include "filefolio_lib.h"


/*****************************************************************************/


FolioGlue(ExecuteAsThread,
	  -14,
	  (CodeHandle code, uint32 argc, char **argv, char *threadName, int32 priority),
	  (code, argc, argv, threadName, priority),
	  Item)
