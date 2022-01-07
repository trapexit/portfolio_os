/* $Id: addtimes.c,v 1.3 1994/09/10 02:52:22 vertex Exp $ */

#include "types.h"
#include "time.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/timer/addtimes
|||	AddTimes - Adds two time values together
|||
|||	  Synopsis
|||
|||	    void AddTimes(const TimeVal *tv1, const TimeVal *tv2,
|||	                  TimeVal *result);
|||
|||	  Description
|||
|||	    Adds two time values together, yielding the total time for both.
|||
|||	  Arguments
|||
|||	    tv1                         The first time value to add.
|||
|||	    tv2                         The second time value to add.
|||
|||	    result                      A pointer to the location where
|||	                                the resulting time value will be
|||	                                stored. This pointer can match either
|||	                                of 'tv1' or 'tv2'.
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
|||	    SubTimes(), CompareTimes()
|||
**/

void AddTimes(const TimeVal *tv1, const TimeVal *tv2, TimeVal *result)
{
uint32 secs;
uint32 micros;

    secs   = tv1->tv_Seconds + tv2->tv_Seconds;
    micros = tv1->tv_Microseconds + tv2->tv_Microseconds;

    if (micros >= 1000000)
    {
	secs++;
	micros -= 1000000;
    }

    result->tv_Seconds      = secs;
    result->tv_Microseconds = micros;
}
