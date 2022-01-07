/* $Id: olio.c,v 1.2 1994/06/01 20:35:38 limes Exp $ */

#include <types.h>
#include <kernel.h>
#include <string.h>

#include "conio.h"

#ifdef	CIO_OLIO

/* #define	OLIO_DEBUG */

extern bool		irqDisabled(void);
extern void		DebugTrigger(void);

extern int32	       *PtrMacPkt;
extern uint32		CPUFlags;

int
olioInit (void)
{
    if (CPUFlags & KB_NODBGR) {
#ifdef	OLIO_DEBUG
	printf("olio_init: returning false\n");
#endif
	return 0;
    }

    memset (PtrMacPkt, 0, 64);

#ifdef	OLIO_DEBUG
    printf("olio_init: returning true\n");
#endif
    return 1;
}

int
olioPutChar (int c)
{
    if (!irqDisabled ())
    {
	volatile int32         *foo = PtrMacPkt;

	while (*foo)
	    ;
	*foo = (int32) c | ((int32) c << 24);
	DebugTrigger ();
    }
    return c;
}

#endif
