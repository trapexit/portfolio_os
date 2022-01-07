/* $Id: nsio.c,v 1.2 1994/11/10 01:23:03 limes Exp $ */

/*
 * NS16550 Serial Device Driver
 *
 * This driver implements only enough of a driver to service
 * the requirements for the Toshiba Car Navigation system.
 */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "interrupts.h"
#include "operror.h"
#include "mem.h"
#include "super.h"

#include "interrupts.h"

#include "nsio.h"

/*****************************************************************************/


#define DRIVER_NAME  "ns driver"
#define DEVICE_NAME  "ns"
#define DEVICE_UNITS 1

/*****************************************************************************/

static List             nsListRX = INITLIST (nsListRX, "ns Readers");

static int              nsReady = 0;

static char             nsFirqName[] = "ns rx firq";
static Item             nsFirqItem;

/*****************************************************************************/

void
nsPollRX (void)
{
    ns16550                *nsp = (ns16550 *) NS_BASE;
    IOReq                  *ior;
    uint8                  *p;
    uint32                  lsr;
    int32                   s;
    int32                   a;
    int32                   ch;

    if (!nsReady) {
	nsp->ns_ier &= ~NS_IER_ERBFI;
	return;
    }
/*
 * XXX- This serial polling loop only functions for input, and will
 * need major rework when it has to handle output. Plus, what about
 * when we want to start watching the incoming modem control signals?
 *
 * XXX- We have *NO* way of reporting incoming break signals or data
 * errors to the application. Do we want to set up a way? In
 * particular, we should watch out for NS_LSR_OE as it means that we
 * have lost incoming data.
 *
 * XXX- just in general, I apologise for the fact that this loop may
 * be considered to be "inside out" and that it has such a strange
 * control structure (returns from strange places). However, it does
 * seem to be the most optimal organization that I could find that did
 * not also make (excessive) use of GOTO.
 */

    while (1) {				/* loop once per request */
/*
 * FASTPATH: If nsListRX queue is empty, disable RxFIFO ints and
 * return. This gives us up to ~16 character times to get another
 * IOReq set up before we start losing data.
 */
	ior = (IOReq *) FIRSTNODE (&nsListRX);
	if (!ISNODE (&nsListRX, ior)) {
	    nsp->ns_ier &= ~NS_IER_ERBFI;
	    return;
	}
/*
 * FASTPATH: If no data available, terminate processing, but leave interrupts
 * enabled so we can start filling our data buffer when data arrives.
 */
	lsr = nsp->ns_lsr;
	if (!(lsr & NS_LSR_DR)) {
	    nsp->ns_ier |= NS_IER_ERBFI;
	    return;
	}
/*
 * OPTIMIZATION: cache destination ptr, length, and actual completed
 * so far in local registers.
 */
	p = (uint8 *) ior->io_Info.ioi_Recv.iob_Buffer;
	s = ior->io_Info.ioi_Recv.iob_Len;
	a = ior->io_Actual;
/*
 * PARANOIA: If the pointer is null, or actual is not smaller than the
 * receive buffer size, drop on through to the "complete" clause.
 */
	if ((p != (uint8 *) 0) && (a < s)) {
	    while (1) {
		ch = nsp->ns_rbr & 0x7F;
/*
 * Transfer a byte. If this is the last in the request, send it back
 * to the user, and loop back to get another request.
 */
		p[a] = (unsigned char) ch;
		a++;
		if (a == s)
		    break;

/*
 * Check for data available. If not, update io_Actual and
 * terminate processing.
 */
		lsr = nsp->ns_lsr;
		if (!(lsr & NS_LSR_DR)) {
		    ior->io_Actual = a;
		    nsp->ns_ier |= NS_IER_ERBFI;
		    return;
		}
	    }
	}
/*
 * If we got here, we completed a request. Remove it from the pending
 * request queue, and complete it back to the sender.
 */
	ior->io_Actual = a;
	ior->io_Flags |= SIOFLAGS_COMPLETE;
	RemNode ((Node *) ior);
	SuperCompleteIO (ior);
    }
}

/*****************************************************************************/

int32
nsFirq (void)
{
    nsPollRX();
    return 0;
}

/*****************************************************************************/

static                  Err
nsDevDelete (Device * dev)
{
    DeleteItem (dev->dev_Driver->drv.n_Item);
    return 0;
}


/*****************************************************************************/


static Item
nsDevInit (Device * dev)
{
    dev->dev_MaxUnitNum = DEVICE_UNITS - 1;

    /*
     * this should be installed with a tag, but there is none for this
     * purpose yet, so we poke it by hand
     */
    dev->dev_DeleteDev = nsDevDelete;
    return dev->dev.n_Item;
}

/*****************************************************************************/

static Item
nsDrvInit (Driver * drv)
{

    /*
     * This should create whatever FIRQ/HLINT items needed to do its
     * work
     */

    ns16550                *nsp = (ns16550 *) NS_BASE;

    nsReady = 0;

    nsp->ns_scr = 0x00;
    if ((nsp->ns_scr & 0xFF) != 0x00)
	return BADUNIT;

    nsp->ns_scr = 0xFF;
    if ((nsp->ns_scr & 0xFF) != 0xFF)
	return BADUNIT;

    nsp->ns_lcr = 0;
    nsp->ns_ier = 0;

    nsp->ns_lcr = NS_LCR_WLS_7 | NS_LCR_STB_1 | NS_LCR_PEN | NS_LCR_EPS | NS_LCR_DLAB;
    nsp->ns_ms = NS_BAUD_DIV_HI (NS_XTAL, NS_BAUD);
    nsp->ns_ls = NS_BAUD_DIV_LO (NS_XTAL, NS_BAUD);
    nsp->ns_lcr = NS_LCR_WLS_7 | NS_LCR_STB_1 | NS_LCR_PEN | NS_LCR_EPS;

    nsp->ns_fcr = NS_FCR_FIFOEN | NS_FCR_RXT_8;
    nsp->ns_mcr = NS_MCR_DTR | NS_MCR_RTS;

    nsFirqItem = SuperCreateFIRQ (nsFirqName, 200, nsFirq, INT_PD);
    if (nsFirqItem < 0) {
	PrintError ("ns", "create FIRQ",
		    nsFirqName, nsFirqItem);
	return nsFirqItem;
    }
    nsReady = 1;

    return drv->drv.n_Item;
}


/*****************************************************************************/


/* Abort an IO. When this is called, we know that interrupts are
 * disabled, and that the IO is indeed currently in progress.
 */
static void
nsDrvAbortIO (IOReq *ior)
{
    /* remove the ior from the reader queue */
    RemNode ((Node *) ior);

    /* mark the ior as aborted */
    ior->io_Error = ABORTED;

    /* return the ior to the client */
    SuperCompleteIO (ior);
}


/*****************************************************************************/


static int32
nsRead (IOReq *ior)
{
    int32                   oldInts;
    int32                   wasempty;
    int32                   rv = 0;

    oldInts = Disable ();
    ior->io_Flags &= ~SIOFLAGS_COMPLETE;
    wasempty = ISLISTEMPTY (&nsListRX);
    AddTail (&nsListRX, (Node *) ior);

/*
 * If the nsListRX list was empty,
 * kickstart the receive interrupt system.
 */
    if (wasempty)
	nsPollRX ();

/*
 * It is possible that the ior just got completed.
 */
    if (ior->io_Flags & SIOFLAGS_COMPLETE) {
	rv = 1;
    } else {
	ior->io_Flags &= ~IO_QUICK;
    }
    Enable (oldInts);
    return rv;
}

/*****************************************************************************/


static int32
nsWrite (IOReq *ior)
{
    /* transmit data out serial port */

    ior->io_Error = BADCOMMAND;
    return 1;
}


/*****************************************************************************/


static int32
nsStatus (IOReq *ior)
{
    /* fill in a DeviceStatus structure */

    ior->io_Error = BADCOMMAND;
    return 1;
}


/*****************************************************************************/


static int32            (*nsDrvCommands[]) () =
{
    nsWrite,
    nsRead,
    nsStatus
};

static TagArg           nsDrvTags[] =
{
    TAG_ITEM_NAME,			(void *) DRIVER_NAME,
    CREATEDRIVER_TAG_ABORTIO,		(void *) nsDrvAbortIO,
    CREATEDRIVER_TAG_MAXCMDS,		(void *) (sizeof (nsDrvCommands) / sizeof (void *)),
    CREATEDRIVER_TAG_CMDTABLE,		(void *) nsDrvCommands,
    CREATEDRIVER_TAG_INIT,		(void *) nsDrvInit,
    TAG_END,				0
};

static TagArg           nsDevTags[] =
{
    TAG_ITEM_NAME,			(void *) DEVICE_NAME,
    CREATEDEVICE_TAG_DRVR,		(void *) 0,
    CREATEDEVICE_TAG_INIT,		(void *) nsDevInit,
    TAG_END,				0
};

int
main (int32 argc, char **argv)
{
    Item                    deviceItem;
    Item                    driverItem;

    print_vinfo ();

    if (argc != DEMANDLOAD_MAIN_CREATE) {
        return 0;
    }
    driverItem = CreateItem (MKNODEID (KERNELNODE, DRIVERNODE), nsDrvTags);
    if (driverItem < 0)
	return (int) driverItem;
    nsDevTags[1].ta_Arg = (void *) driverItem;
    deviceItem = CreateItem (MKNODEID (KERNELNODE, DEVICENODE), nsDevTags);
    if (deviceItem < 0) {
	DeleteItem (driverItem);
	return (int) deviceItem;
    }
    return (int) deviceItem;
}
