/* $Id: deletetimerio.c,v 1.4 1994/09/21 20:05:02 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "device.h"
#include "operror.h"
#include "time.h"


/*****************************************************************************/


/**
|||	AUTODOC PUBLIC spg/timer/deletetimerioreq
|||	DeleteTimerIOReq - Delete a timer device IO request
|||
|||	  Synopsis
|||
|||	    Err DeleteTimerIOReq(Item ioreq);
|||
|||	  Description
|||
|||	    Frees any resources used by a previous call to CreateTimerIOReq().
|||
|||	  Arguments
|||
|||	    ioreq                       The IO request Item, as returned by
|||	                                a previous call to CreateTimerIOReq().
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
|||	    CreateTimerIOReq(), WaitTime(), WaitUntil()
|||
**/

Err DeleteTimerIOReq(Item ioreq)
{
IOReq  *io;
Device *dev;
Err     result;

    io = (IOReq *)CheckItem(ioreq,KERNELNODE,IOREQNODE);
    if (!io)
        return BADITEM;

    dev = io->io_Dev;
    result = DeleteIOReq(ioreq);
    CloseNamedDevice(dev->dev.n_Item);

    return result;
}
