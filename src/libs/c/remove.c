/* $Id: remove.c,v 1.2 1994/09/12 20:18:24 vertex Exp $ */

#include "types.h"
#include "string.h"
#include "filefunctions.h"


/*****************************************************************************/


extern int32 errno;


int32 remove(const char *name)
{
Err err;

    err = DeleteFile((char *)name);
    if (err >= 0)
	return 0;

    errno = err;

    return err;
}
