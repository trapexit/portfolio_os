/***************************************************************\
*								*
* General routine to read the timer                             *
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  27-Jul-93					*
*								*
* Copyright (c) 1993, The 3DO Company                           *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/



#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) { printf x ; }

#include "types.h"

#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "operror.h"
#include "time.h"
#include "strings.h"
#include "stdlib.h"
#include "stdio.h"

#include "gettime.h"


Item timerior;
IOReq *timerioreq;
IOInfo __ioi;


extern void terminate (int32);


void
opentimer (void)
{
  TagArg targs[2];
  Item timerDevice;

  FULLDBUG (("Opening Timer device\n"));
  if ((timerDevice = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
					    "timer"), 0)) < 0) {
    DBUG (("Error opening timer device (%ld)\n", timerDevice));
    terminate (1);
  }

  targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  targs[0].ta_Arg = (void *)timerDevice;
  targs[1].ta_Tag = TAG_END;

  FULLDBUG (("Creating timer IO request\n"));
  timerior = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), targs);
  FULLDBUG (("timerior = %lx\n", timerior));

  memset (&__ioi, 0, sizeof(__ioi));

  __ioi.ioi_Flags = 0;
  __ioi.ioi_Unit = 1;
  __ioi.ioi_Command = CMD_READ;
  __ioi.ioi_Offset = 0;
  __ioi.ioi_Recv.iob_Len = 8;
  __ioi.ioi_Send.iob_Buffer = NULL;
  __ioi.ioi_Send.iob_Len = 0;
}


void
gettime (uint32 timebuffer[2])
{
  int32 error;

  __ioi.ioi_Recv.iob_Buffer = (void*)timebuffer;

  error = DoIO (timerior, &__ioi);
  if (error<0) {
    DBUG (("Error getting timer status (%lx)\n", error));
    PrintfSysErr (error);
    FULLDBUG (("timerior = %lx\n", timerior));
  }
}


