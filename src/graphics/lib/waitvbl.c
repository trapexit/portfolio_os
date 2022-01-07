/* $Id: waitvbl.c,v 1.3 1994/10/07 22:47:03 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "time.h"
#include "string.h"


/*****************************************************************************/


Err
WaitVBL (Item ioreq, uint32 numfields)
{
IOInfo ioInfo;

    memset(&ioInfo, 0, sizeof(IOInfo));
    ioInfo.ioi_Command = TIMERCMD_DELAY;
    ioInfo.ioi_Offset  = numfields;

    return DoIO (ioreq, &ioInfo);
}
