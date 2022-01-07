/* $Id: clonevram.c,v 1.2 1994/09/02 01:22:29 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "mem.h"
#include "graphics.h"


/*****************************************************************************/


Err
CloneVRAMPages(Item ioreq, void *dest, void *src, ulong numpages, ulong mask)
{
IOInfo ioInfo;
uint32 pagesize;

    pagesize = GetPageSize(MEMTYPE_VRAM);

    ioInfo.ioi_Command         = SPORTCMD_CLONE;
    ioInfo.ioi_Flags           = 0;
    ioInfo.ioi_Unit            = 0;
    ioInfo.ioi_Flags2          = 0;
    ioInfo.ioi_CmdOptions      = 0;
    ioInfo.ioi_User            = 0;
    ioInfo.ioi_Offset          = mask;
    ioInfo.ioi_Send.iob_Buffer = (void *)((ulong)src&(-pagesize));
    ioInfo.ioi_Send.iob_Len    = pagesize;
    ioInfo.ioi_Recv.iob_Buffer = (void *)((ulong)dest&(-pagesize));
    ioInfo.ioi_Recv.iob_Len    = numpages*pagesize;

    return DoIO(ioreq, &ioInfo);
}
