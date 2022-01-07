/* $Id: strerror.c,v 1.2 1994/09/07 16:41:20 vertex Exp $ */

#include "types.h"
#include "string.h"
#include "operror.h"


/*****************************************************************************/


static char errBuf[128];


char *strerror(int errorCode)
{
    if (errorCode >= 0)
        return NULL;

    GetSysErr(errBuf, sizeof(errBuf), errorCode);
    return errBuf;
}
