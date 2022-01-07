/* $Id: clonevramdefer.c,v 1.2 1994/08/25 23:00:45 ewhac Exp $ */

#include "types.h"
#include "io.h"
#include "mem.h"
#include "string.h"
#include "graphics.h"


/*****************************************************************************/


Err
CloneVRAMPagesDefer (Item ioreq, void *dest, void *src, ulong numpages, ulong mask)
{
  IOInfo ioi;
  uint32 pagesize;

  memset ((char *)&ioi, 0, sizeof(ioi));
  pagesize = (ulong)GetPageSize(MEMTYPE_VRAM);

  ioi.ioi_Command = SPORTCMD_CLONE;
  ioi.ioi_Offset = (int)mask;
/*  ioi.ioi_Flags = 0; */
/*  ioi.ioi_Unit = 0; */
  ioi.ioi_Recv.iob_Buffer = (void *)((ulong)dest&(-pagesize));
  ioi.ioi_Recv.iob_Len = (int)(numpages*pagesize);
  ioi.ioi_Send.iob_Buffer = (void *)((ulong)src&(-pagesize));
  ioi.ioi_Send.iob_Len = (int)(pagesize);
  return (SendIO (ioreq, &ioi));
}
