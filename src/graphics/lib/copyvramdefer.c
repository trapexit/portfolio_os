/* $Id: copyvramdefer.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "mem.h"
#include "string.h"
#include "graphics.h"


/*****************************************************************************/


Err
CopyVRAMPagesDefer (Item ioreq, void *dest, void *src, ulong numpages, ulong mask)
{
  IOInfo ioi;
  uint32 pagesize;

  memset ((char *)&ioi, 0, sizeof(ioi));
  pagesize = (ulong)GetPageSize(MEMTYPE_VRAM);

  ioi.ioi_Command = SPORTCMD_COPY;
  ioi.ioi_Offset = (int)mask;
/*  ioi.ioi_Flags = 0; */
/*  ioi.ioi_Unit = 0; */
  ioi.ioi_Recv.iob_Buffer = (void *)((ulong)dest&(-pagesize));
  ioi.ioi_Recv.iob_Len = (int)(numpages*pagesize);
  ioi.ioi_Send.iob_Buffer = (void *)((ulong)src&(-pagesize));
  ioi.ioi_Send.iob_Len = (int)(numpages*pagesize);
  return SendIO(ioreq,&ioi);
}
