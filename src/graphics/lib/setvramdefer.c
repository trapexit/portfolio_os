/* $Id: setvramdefer.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "mem.h"
#include "string.h"
#include "graphics.h"


/*****************************************************************************/


/* We use the io device path for FW, since it is a (albeit circuitous) */
/* route to DoIO() in the kernal, which does ValidateMem() for us. */

Err
SetVRAMPagesDefer (Item ioreq, void *dest, int32 val, int32 numpages, int32 mask)
{
  IOInfo ioi;
  uint32 pagesize;

  pagesize = (ulong)GetPageSize(MEMTYPE_VRAM);
  memset ((char *)&ioi, 0, sizeof(ioi));

  ioi.ioi_Command = FLASHWRITE_CMD;
  ioi.ioi_Offset = val;
  ioi.ioi_CmdOptions = mask;
/*  ioi.ioi_Unit = 0; */
  ioi.ioi_Recv.iob_Buffer = (void *)((ulong)dest&(-pagesize));
  ioi.ioi_Recv.iob_Len = (int)(numpages*pagesize);
  /* Fake source */
  ioi.ioi_Send.iob_Buffer = ioi.ioi_Recv.iob_Buffer;
/*  ioi.ioi_Send.iob_Len = 0; */
  return SendIO(ioreq,&ioi);
}
