/* $Id: putc.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */
#include "types.h"
#include "stdio.h"
#include "debug.h"

/* putc for simple char i/o */
/* and let sprintf work */

FILE *stdout = 0;

int32 putc(char c, FILE *stream)
{
    if (stream == stdout) kprintf("%c",c);
    else
    {
	if (--stream->fcb_numinbuf < 0)
	{
		/* Error! we should never get here */
		/* there is no stream system set up yet */
		return 0;
	}
	*stream->fcb_cp++ = c;
    }
    return c;
}
