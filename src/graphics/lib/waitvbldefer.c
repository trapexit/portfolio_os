/* $Id: waitvbldefer.c,v 1.2 1994/10/07 22:47:03 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "time.h"
#include "string.h"
#include "graphics.h"


/*****************************************************************************/


Err
WaitVBLDefer (Item ioreq, uint32 numfields)
{
  IOInfo ioi;

  memset ((char *)&ioi, 0, sizeof(ioi));

/*  ioi.ioi_Flags = 0; */
/*  ioi.ioi_Unit = 0; */
  ioi.ioi_Command = TIMERCMD_DELAY;
  ioi.ioi_Offset = numfields;
/*  ioi.ioi_Recv.iob_Buffer = NULL; */
/*  ioi.ioi_Recv.iob_Len = 0; */
/*  ioi.ioi_Send.iob_Buffer = NULL; */
/*  ioi.ioi_Send.iob_Len = 0; */

  return SendIO (ioreq, &ioi);
}
