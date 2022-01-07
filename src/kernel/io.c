/* $Id: io.c,v 1.72 1994/09/19 17:54:19 vertex Exp $ */
/* file io.h */

#include "types.h"
#include "inthard.h"
#include "nodes.h"
#include "list.h"
#include "task.h"
#include "mem.h"
#include "string.h"
#include "stdio.h"
#include "gfx.h"

/* #define SILENTSHERRY */

#include "internalf.h"

#include "conio.h"

char                    serial_ok = 0;	/* set if any serial ports are available */
uint32                  cio_mask = 0;	/* which console driver(s) to use */

/* stuff to keep stdlib.o from being linked in */

extern void             Do_FIRQ (void);
extern bool             irqDisabled (void);
extern void             enable_firq_interrupt (void);

extern uint32           CPUFlags;

char                    okguru = 1;	/* remnant of GURU stuff in vdl.c */

#ifdef SILENTSHERRY
char                    kill_kprintf = 1;
#else
char                    kill_kprintf = 0;
#endif

char                    really_kill_kprintf;

/* Kernel raw serial I/O */

#define ABORT_MODE	0x17

int
MayGetChar ()
{
#ifdef	CIO_SSIO
    if (cio_mask & CIO_SSIO)
	return ssioGetChar ();
#endif
#ifdef	CIO_ZSIO
    if (cio_mask & CIO_ZSIO)
	return zsioGetChar ();
#endif
#ifdef	CIO_ZPIO
    if (cio_mask & CIO_ZPIO)
	return zpioGetChar ();
#endif
#if 0
/* there is no input device corresponding to GrafCon output */
#ifdef	CIO_GCIO
    if (cio_mask & CIO_GCIO)
	return gcioGetChar ();
#endif
#endif

#if 0
/* optical link "getchar" seems to be handled elsewhere. */
#ifdef	CIO_OLIO
    if (cio_mask & CIO_OLIO)
	return olioGetChar ();
#endif
#endif
    return -1;
}

int32
putc (char c, FILE * stream)
{
    *((int32 *) (MADAM)) = c;		/* store so a debugger can
					 * trigger on it */

#ifdef DEVELOPMENT
    if (c == 1)
    {
	really_kill_kprintf = 1;
	return c;
    }
    if (c == 2)
    {
	really_kill_kprintf = 0;
	return c;
    }

    if (really_kill_kprintf)
	return c;
#endif

    if (c == 4)
    {
	kill_kprintf = 1;
	okguru = 0;
	return c;
    }

    if (c == '\n')		/* mac hates newlines */
	c = '\r';		/* send return instead */

    if ((c > 126) || ((c < 32) && (c != '\r') && (c != '\t')))
	return c;		/* unprintable */

#ifdef	CIO_SSIO
    if (cio_mask & CIO_SSIO)
	return ssioPutChar (c);
#endif
#ifdef	CIO_ZSIO
    if (cio_mask & CIO_ZSIO)
	return zsioPutChar (c);
#endif
#ifdef	CIO_ZPIO
    if (cio_mask & CIO_ZPIO)
	return zpioPutChar (c);
#endif
#ifdef	CIO_OLIO
    if (cio_mask & CIO_OLIO)
	return olioPutChar (c);
#endif
#ifdef	CIO_GCIO
    if ((cio_mask & CIO_GCIO) && (kill_kprintf == 0))
	return gcioPutChar (c);
#endif
    return c;
}

extern void             FindRamSize (void);

/* Initialize display routines */
void
InitIO ()
{
    InstallArmVector (0x1c, Do_FIRQ);
    FindRamSize ();
    enable_firq_interrupt ();
    MakeKernelDisplay (kill_kprintf);

#ifdef	CIO_GCIO
    if (gcioInit (320, 240))
	cio_mask |= CIO_GCIO;
#endif
#ifdef	CIO_OLIO
    if (olioInit ())
	cio_mask |= CIO_OLIO;
#endif
#ifdef	CIO_SSIO
    if (ssioInit ())
	cio_mask |= CIO_SSIO;
#endif
#ifdef	CIO_ZPIO
    if (zpioInit ())
	cio_mask |= CIO_ZPIO;
#endif
#ifdef	CIO_ZSIO
    if (zsioInit ())
	cio_mask |= CIO_ZSIO;
#endif
}

void
InitConioFirqs()
{
#ifdef	CIO_ZSIO
    if (cio_mask & CIO_ZSIO)
	zsioFirqInit ();
#endif
}
