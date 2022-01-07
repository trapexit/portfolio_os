/* $Id: setvram.c,v 1.2 1994/09/02 16:36:04 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "mem.h"
#include "graphics.h"


/*****************************************************************************/


Err
SetVRAMPages(Item ioreq, void *dest, int32 val, int32 numpages, int32 mask)
{
IOInfo ioInfo;
uint32 pagesize;

    pagesize = GetPageSize(MEMTYPE_VRAM);

    ioInfo.ioi_Command         = FLASHWRITE_CMD;
    ioInfo.ioi_Flags           = 0;
    ioInfo.ioi_Unit            = 0;
    ioInfo.ioi_Flags2          = 0;
    ioInfo.ioi_CmdOptions      = mask;
    ioInfo.ioi_User            = 0;
    ioInfo.ioi_Offset          = val;
    ioInfo.ioi_Recv.iob_Buffer = (void *)((ulong)dest&(-pagesize));
    ioInfo.ioi_Recv.iob_Len    = numpages*pagesize;

    /* Fake source to keep the sport device happy */
    ioInfo.ioi_Send.iob_Buffer = ioInfo.ioi_Recv.iob_Buffer;
    ioInfo.ioi_Send.iob_Len    = 0;

    return DoIO(ioreq,&ioInfo);
}
