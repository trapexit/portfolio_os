/* $Id: zsio.c,v 1.5 1994/06/22 22:09:02 limes Exp $ */

#include <types.h>
#include <internalf.h>
#include <interrupts.h>
#include <operror.h>

#include "conio.h"
#include "scc8530.h"

/*
 * ZSIO: ZS85C30 driver
 * Compatible with GREEN silicon
 */

#ifdef	CIO_ZSIO

extern Item		AllocFirq (char *name, int pri, int num, void (*code) ());

static void             zsioInitSCC (void);
static void             zsioFirqHandler (void);
static void             zsioWriteReg (uint32 reg, uint32 data);
static uint32           zsioReadReg (uint32 reg);

extern int              ignoreabort;
extern int              abortignored;

#define	ZSIO_SCC_BASE	0x03180000

#define	ZSIO_PORT	((scc8530 *)ZSIO_SCC_BASE)->ctlA

int                     zsioMaxUnready = 10000;

#define	CTRL(ch)	(0x1F & (ch))
int                     zsioFlushOutChar = CTRL ('O');
int                     zsioHoldOutChar = CTRL ('S');
int                     zsioResumeOutChar = CTRL ('Q');
int                     zsioQuoteInChar = CTRL ('V');

int                     zsioOutputFlushed = 0;
int                     zsioOutputHolding = 0;
int                     zsioInputQuoted = 0;

Item			zsioFirqItem = -1;

#define	ZSIO_INPUTRINGSIZE	256

char                    zsioInputRing[ZSIO_INPUTRINGSIZE];
char                   *zsioIRput;
char                   *zsioIRget;

int
zsioInit (void)
{
    uint32                  csr;

    zsioIRput = zsioInputRing;
    zsioIRget = zsioInputRing;

    zsioWriteReg (SCC_MICR, SCC_MICR_RESET);
    csr = zsioReadReg (SCC_CSR);
    if ((csr & SCC_CSR_ATRESET_MASK) != SCC_CSR_ATRESET_VAL)
	return 0;
    zsioInitSCC ();
    csr = zsioReadReg (SCC_CSR);
    if ((csr & SCC_CSR_ATRESET_MASK) != SCC_CSR_ATRESET_VAL)
	return 0;
    serial_ok = 1;
    return 1;
}

void
zsioFirqInit(void)
{
    /* XXX- would rather use INT_PD but having trouble with it. */
    zsioFirqItem = AllocFirq("zsio", 100, INT_V1, zsioFirqHandler);
    if (zsioFirqItem < 0)
    {
	PrintError("zsioFirqInit", "AllocFirq", "zsioFirqHandler", zsioFirqItem);
    }
    else
    {
	zsioInitSCC();
    }
}

int                     zsioLastChar = 0;

int
zsioPutChar (int c)
{
    int                     watchdog;

    watchdog = zsioMaxUnready;

    while (1)
    {
	if (Disabled())
	    zsioFirqHandler();
	if (zsioOutputFlushed)
	    return c;
	if (zsioOutputHolding)
	    continue;
	if (zsioReadReg (SCC_CSR) & SCC_CSR_TXRDY)
	    break;
	if (!--watchdog)
	{
	    zsioInitSCC ();
	    zsioPutChar (zsioLastChar);
	    watchdog = zsioMaxUnready;
	}
    }
    zsioWriteReg (SCC_TXDATA, c);
    zsioLastChar = c;
    return c;
}

int
zsioGetChar (void)
{
    int                     rv = -1;
    char                   *gp;
    char                   *pp;
    uint32                  oldints;

    oldints = Disable ();
    gp = zsioIRget;
    pp = zsioIRput;
    if (gp != pp)
    {
	rv = *gp++;
	if (gp == (zsioInputRing + ZSIO_INPUTRINGSIZE))
	    gp = zsioInputRing;
	zsioIRget = gp;
    }
    Enable (oldints);
    return rv;
}

static void
zsioFirqHandler (void)
{
    int                     ch = -1;
    char                   *gp, *pp;

    zsioWriteReg(SCC_MODE, SCC_MODE_EXTRES);
    if (zsioReadReg (SCC_CSR) & SCC_CSR_RXRDY)
    {
	ch = (int) zsioReadReg (SCC_RXDATA);
	if (zsioInputQuoted)
	{
	    zsioInputQuoted = 0;
	}
	else
	{
	    if (ch == zsioQuoteInChar)
	    {
		zsioInputQuoted = 1;
		ch = -1;
	    }
	    else if (ch == zsioFlushOutChar)
	    {
		zsioOutputFlushed = 1;
		ch = -1;
	    }
	    else if (ch == zsioHoldOutChar)
	    {
		zsioOutputHolding = 1;
		ch = -1;
	    }
	    else if (ch == zsioResumeOutChar)
	    {
		zsioOutputFlushed = 0;
		zsioOutputHolding = 0;
		ch = -1;
	    }
	}
	if (ch != -1)
	{
	    gp = zsioIRget;
	    pp = zsioIRput;
	    *pp++ = ch;
	    if (pp == (zsioInputRing + ZSIO_INPUTRINGSIZE))
		pp = zsioInputRing;
	    if (pp != gp)
		zsioIRput = pp;
	    /* XXX- else overflow; ring bell? */
	}
    }
    zsioWriteReg (SCC_MODE, SCC_MODE_RXINTEN);
}

static void
zsioSetBaud (uint32 br)
{
    zsioWriteReg (SCC_BAUDLO, SCC_BAUD_CONST_LO (br));
    zsioWriteReg (SCC_BAUDHI, SCC_BAUD_CONST_HI (br));
}

static void
zsioInitSCC (void)
{
    zsioWriteReg (SCC_MICR, SCC_MICR_RESET);
    zsioWriteReg (SCC_MISCPARM, SCC_MISCPARM_CLK1 | SCC_MISCPARM_1STOP);
    zsioWriteReg (SCC_RXPARM, SCC_RXPARM_8BIT);
    zsioWriteReg (SCC_TXPARM, SCC_TXPARM_8BIT);
    zsioWriteReg (SCC_MICR, 0);
    zsioWriteReg (SCC_MISC, 0);
    zsioWriteReg (SCC_CLOCK, SCC_CLOCK_TRXCO_XTAL | SCC_CLOCK_TXCI_BRGEN | SCC_CLOCK_RXCI_BRGEN);
    zsioSetBaud (9600);
    zsioWriteReg (SCC_MISCCTL, 0);
    zsioWriteReg (SCC_MISCCTL, SCC_MISCCTL_BRGEN);
    zsioWriteReg (SCC_RXPARM, SCC_RXPARM_RXEN | SCC_RXPARM_8BIT);
    zsioWriteReg (SCC_TXPARM, SCC_TXPARM_RTS | SCC_TXPARM_TXEN | SCC_TXPARM_8BIT | SCC_TXPARM_DTR);
    if (zsioFirqItem < 0)
	return;
    zsioWriteReg (SCC_IVEC, 0);
    zsioWriteReg (SCC_MICR, SCC_MICR_MIE);
    zsioWriteReg (SCC_INTSETUP, SCC_INTSETUP_RXINTALL);
    zsioWriteReg (SCC_MODE, SCC_MODE_RXINTEN);
}

static void
zsioWriteReg (uint32 reg, uint32 data)
{
    uint32                  oldints;
    uint32                  junk;

    oldints = Disable ();

    ignoreabort = 1;
    while (1)
    {
	abortignored = 0;
	SCC_HOLDOFF ();
	junk = ZSIO_PORT;
	if (abortignored)
	    continue;
	SCC_HOLDOFF ();
	ZSIO_PORT = reg;
	if (abortignored)
	    continue;
	SCC_HOLDOFF ();
	ZSIO_PORT = data;
	if (abortignored)
	    continue;
	break;
    }
    ignoreabort = 0;

    Enable (oldints);
    return;
}

static uint32
zsioReadReg (uint32 reg)
{
    uint32                  oldints;
    uint32                  data;
    uint32                  junk;

    oldints = Disable ();

    ignoreabort = 1;
    while (1)
    {
	abortignored = 0;
	SCC_HOLDOFF ();
	junk = ZSIO_PORT;
	if (abortignored)
	    continue;
	SCC_HOLDOFF ();
	ZSIO_PORT = reg;
	if (abortignored)
	    continue;
	SCC_HOLDOFF ();
	data = ZSIO_PORT;
	if (abortignored)
	    continue;
	break;
    }
    ignoreabort = 0;

    Enable (oldints);
    return data;
}

#endif
