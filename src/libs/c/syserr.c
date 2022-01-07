/* $Id: syserr.c,v 1.21 1994/09/10 02:52:22 vertex Exp $ */

extern void printf(const char *fmt, ...);

#include "types.h"
#include "operror.h"

/**
|||	AUTODOC PUBLIC spg/kernel/printfsyserr
|||	PrintfSysErr - Print the error string for an error.
|||
|||	  Synopsis
|||
|||	    void PrintfSysErr( Err err )
|||
|||	  Description
|||
|||	    This procedure prints the error string for an error to the
|||	    Macintosh console for a 3DO development system.  This has the same
|||	    effect as using Printf() to print the string constructed by
|||	    GetSysErr().  The string is not displayed on production machines.
|||
|||	    To copy an error string into a buffer instead of sending it the
|||	    console, use GetSysErr().
|||
|||	  Arguments
|||
|||	    err                         An error code.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    operror.h                   ANSI C Macro definition
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  See Also
|||
|||	    GetSysErr()
|||
**/

void
PrintfSysErr(i)
Item i;
{
	char errtext[128];
	GetSysErr(errtext,sizeof(errtext),i);
	printf("%s\n",errtext);
}
