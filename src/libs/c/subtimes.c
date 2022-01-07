/* $Id: subtimes.c,v 1.3 1994/09/10 02:52:22 vertex Exp $ */

#include "types.h"
#include "time.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/timer/subtimes
|||	SubTimes - Subtract one time value from another
|||
|||	  Synopsis
|||
|||	    void SubTimes(const TimeVal *tv1, const TimeVal *tv2,
|||	                  TimeVal *result);
|||
|||	  Description
|||
|||	    Subtracts two time values, yielding the difference in time
|||	    between the two.
|||
|||	  Arguments
|||
|||	    tv1                         The first time value.
|||
|||	    tv2                         The second time value.
|||
|||	    result                      A pointer to the location where
|||	                                the resulting time value will be
|||	                                stored. This pointer can match either
|||	                                of 'tv1' or 'tv2'. The value which
|||	                                is stored corresponds to (tv2 - tv1)
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
|||	    AddTimes(), CompareTimes()
|||
**/

void SubTimes(const TimeVal *tv1, const TimeVal *tv2, TimeVal *result)
{
    if (tv2->tv_Microseconds >= tv1->tv_Microseconds)
    {
        result->tv_Seconds      = tv2->tv_Seconds - tv1->tv_Seconds;
        result->tv_Microseconds = tv2->tv_Microseconds - tv1->tv_Microseconds;
    }
    else
    {
        result->tv_Seconds      = tv2->tv_Seconds - tv1->tv_Seconds - 1;
        result->tv_Microseconds = 1000000 - (tv1->tv_Microseconds - tv2->tv_Microseconds);
    }
}
