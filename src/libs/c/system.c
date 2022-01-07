/* $Id: system.c,v 1.1 1994/09/08 17:34:58 vertex Exp $ */

#include "types.h"
#include "string.h"
#include "task.h"
#include "filefunctions.h"
#include "stdio.h"
#include "stdlib.h"


/*****************************************************************************/


extern int32 errno;


int system(const char *cmdString)
{
Err err;

    err = LoadProgram((char *)cmdString);
    if (err >= 0)
    {
        while (LookupItem(err))
            WaitSignal(SIGF_DEADTASK);

	return 0;
    }

    errno = err;

    return -1;
}
