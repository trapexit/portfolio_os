/* $Id: waituntil.c,v 1.4 1994/09/21 20:05:02 vertex Exp $ */

#include "types.h"
#include "string.h"
#include "io.h"
#include "time.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/timer/waituntil
|||	WaitUntil - Wait for a given amount of time to pass
|||
|||	  Synopsis
|||
|||	    Err WaitTime(Item ioreq, uint32 seconds, uint32 micros);
|||
|||	  Description
|||
|||	    Puts the current context to sleep until the system clock reaches
|||	    a given time.
|||
|||	  Arguments
|||
|||	    ioreq                       An active timer device IO request, as
|||	                                obtained from CreateTimerIOReq().
|||
|||	    seconds                     The seconds value that the timer
|||	                                must reach.
|||
|||	    micros                      The microseconds value that the timer
|||	                                must reach.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V24.
|||
|||	  Associated Files
|||
|||	    time.h, clib.lib
|||
|||	  See Also
|||
|||	    CreateTimerIOReq(), DeleteTimerIOReq(), WaitTime()
|||
**/

Err WaitUntil(Item ioreq, uint32 seconds, uint32 micros)
{
IOInfo  ioInfo;
TimeVal tv;

    memset(&ioInfo,0,sizeof(IOInfo));

    tv.tv_Seconds              = seconds;
    tv.tv_Microseconds         = micros;
    ioInfo.ioi_Command         = TIMERCMD_DELAYUNTIL;
    ioInfo.ioi_Unit            = TIMER_UNIT_USEC;
    ioInfo.ioi_Send.iob_Buffer = &tv;
    ioInfo.ioi_Send.iob_Len    = sizeof(TimeVal);

    return DoIO(ioreq, &ioInfo);
}
