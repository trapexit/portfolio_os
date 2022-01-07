/* $Id: createtimerio.c,v 1.4 1994/09/21 20:05:02 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "device.h"
#include "time.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/timer/createtimerioreq
|||	CreateTimerIOReq - Create a timer device IO request
|||
|||	  Synopsis
|||
|||	    Item CreateTimerIOReq(void);
|||
|||	  Description
|||
|||	    Creates an IO request for communication with the timer device.
|||
|||	  Return Value
|||
|||	    Returns a timer IO request Item, or a negative error code
|||	    for failure.
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
|||	    DeleteTimerIOReq(), WaitTime(), WaitUntil()
|||
**/

Item CreateTimerIOReq(void)
{
Item device;
Item ioreq;

    device = OpenNamedDevice("timer",NULL);
    if (device < 0)
        return device;

    ioreq = CreateIOReq(NULL,0,device,0);
    if (ioreq < 0)
        CloseNamedDevice(device);

    return ioreq;
}
