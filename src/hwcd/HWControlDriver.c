/* $Id: HWControlDriver.c,v 1.8 1994/09/02 21:09:16 stan Exp $ */
/*
 * file: HWControlDriver.c
 * Opera IO driver for misc. hw control
 * 1/12/94 George Mitsuoka
 * The 3DO Company Copyright © 1994
 */

/*
 * 3/24/94  Added checking of current mode to determine bit sense
 */

#include "types.h"
#include "item.h"
#include "device.h"
#include "debug.h"
#include "driver.h"
#include "io.h"
#include "interrupts.h"
#include "inthard.h"
#include "kernel.h"
#include "operror.h"
#include "super.h"
#include "string.h"
#include "task.h"

#include "sysinfo.h"
#include "hwcontroldriver.h"

#define KDBUG(x)	/* kprintf x */
#define DBUG(x)		/* Superkprintf x */

/*
 * command routine definitions
 */
static void             HWControlAbortIO (IOReq * ior);
static Item             HWControlDriverInit (Driver * drv);
static Item             HWControlDevInit (Device * dev);

static Item             HWControlDevOpen (Device * dev);
static void             HWControlDevClose (Device * dev);

static int32            HWControlCmdWrite (IOReq * ior);
static int32            HWControlCmdRead (IOReq * ior);
static int32            HWControlCmdStatus (IOReq * ior);

static Item             HWControlDeviceItem = 0;

/*
 * driver command entry points
 *
 * AbortIO aborts the specified ior
 */
static void
HWControlAbortIO (IOReq * ior)
{
    DBUG (("HWControlDevice: HWControlAbortIO\n"));
}

/*
 * DriverInit gets called once when the driver is created
 * it returns the driver item or a negative value on error
 */
static Item
HWControlDriverInit (Driver * drv)
{
    DBUG (("HWControlDevice: HWControlDriverInit\n"));

    return drv->drv.n_Item;
}

/*
 * Each DevInit gets called once when the device is created
 * and returns the device item or a negative value on error
 */
static Item
HWControlDevInit (Device * dev)
{
    DBUG (("HWControlDevice: HWControlDevInit\n"));

    dev->dev_MaxUnitNum = 0;		/* max number of units - 1 */

    return dev->dev.n_Item;
}

/*
 * each DevOpen gets called once each time the device is opened
 * and returns the device item or a negative value on error
 */
static Item
HWControlDevOpen (Device * dev)
{
    DBUG (("HWControlDriver: HWControlDevOpen %ld\n", dev->dev_OpenCnt));

    return dev->dev.n_Item;
}

static void
HWControlDevClose (Device * dev)
{
    DBUG (("HWControlDriver: HWControlDevClose %ld\n", dev->dev_OpenCnt));
}

/*
 * Driver Commands
 * there is only one entry point for each command per driver
 * the driver dispatches to the appropriate device
 *
 * each command returns 0 if the request has been queued and will be completed later
 *                      1 if the request has been completed immediately
 *                      negative on error
 *
 * the OS performs some verification of the buffer address and length on read, write,
 * and status requests prior to calling the driver code
 */

static int32
HWControlCmdWrite (IOReq * ior)
{
    DBUG (("HWControlDriver: HWControlCmdWrite src = %08lx, len = %ld\n",
	   ior->io_Info.ioi_Send.iob_Buffer,
	   ior->io_Info.ioi_Send.iob_Len));

    ior->io_Actual = 0L;

    return (1);				/* return 0 if asynchronous */
}

static int32
HWControlCmdRead (IOReq * ior)
{
    DBUG (("HWControlDriver: HWControlCmdRead dst = %08lx, len = %ld\n",
	   ior->io_Info.ioi_Recv.iob_Buffer,
	   ior->io_Info.ioi_Recv.iob_Len));

    ior->io_Actual = 0L;

    return (1);				/* return 0 if asynchronous */
}

/*
 * CmdStatus is boilerplate
 */

static int32
HWControlCmdStatus (IOReq * ior)
{
    DeviceStatus           *dst = (DeviceStatus *) ior->io_Info.ioi_Recv.iob_Buffer;
    DeviceStatus            mystat;
    int32                   len = ior->io_Info.ioi_Recv.iob_Len;

    DBUG (("HWControlDriver: HWControlCmdStatus dst=%08lx len=%d\n", dst, len));
    if (len < 8)
	goto abort;

    memset (&mystat, 0, sizeof (DeviceStatus));

    /* check driver.h to see if there are more appropriate values for your driver */
    mystat.ds_DriverIdentity = DI_OTHER;
    mystat.ds_MaximumStatusSize = sizeof (DeviceStatus);
    mystat.ds_DeviceFlagWord = DS_DEVTYPE_OTHER;

    if (len > sizeof (DeviceStatus))
	len = sizeof (DeviceStatus);
    memcpy (dst, &mystat, len);
    ior->io_Actual = len;

    return (1);

abort:
    ior->io_Error = BADIOARG;
    return (-1);
}

/*
 * driver specific commands begin here
 *
 * the installer task performs services on behalf of the driver
 * these globals are for the signalling mechanism
 */
extern int32            gSignalWaitVBL, gSignalWaitDone;
extern struct Task     *gMainTask;
extern Item             gClientTaskItem;

static int32
HWControlWaitVSync ()
{
    /* signal the installer task to do the wait for us */
    /* set up signalling mechanism */
    gClientTaskItem = CURRENTTASK->t.n_Item;
    gSignalWaitDone = SuperAllocSignal (0L);
    if (gSignalWaitDone == 0)
    {
	DBUG ((" couldn't allocate gSignalWaitDone\n"));
	return (-1L);
    }
    /* signal the installer task to do a waitVBL */
    SuperInternalSignal (gMainTask, gSignalWaitVBL);

    /* wait for the installer task to complete the wait */
    DBUG (("HWControlDriver: waiting for gSignalWaitDone\n"));
    SuperWaitSignal (gSignalWaitDone);
    DBUG (("HWControlDriver: got gSignalWaitDone\n"));

    /* free the signal */
    SuperFreeSignal (gSignalWaitDone);
    return 0;
}

/*
#define DRIVE_INTERLACE_BIT     0x10L
#define INTERLACE_BIT           0x01L
*/
#define ODD_FIELD           VCNT_FIELD

static int32
HWControlCmdInterlace (IOReq * ior)
{
    int32                   lastField, thisField;

    DBUG (("HWControlDriver: HWControlCmdInterlace\n"));

    /* wait a couple fields to make sure any previous mode */
    /* change has had time to take effect */
    HWControlWaitVSync ();
    HWControlWaitVSync ();

    /* check if we're in interlace mode */
    /* wait for vsync */
    HWControlWaitVSync ();
    lastField = *VCNT & ODD_FIELD;

    /* wait for next vsync */
    HWControlWaitVSync ();
    thisField = *VCNT & ODD_FIELD;

    /* if we're in non-interlace mode, switch to interlace */
    if (lastField == thisField)
	SuperSetSysInfo(SYSINFO_TAG_SETINTERLACE, (void *)0, 0);

    return (1);				/* return 0 if asynchronous */
}

static int32
HWControlCmdNonInterlace (IOReq * ior)
{
    int32                   lastField, thisField;

    DBUG (("HWControlDriver: HWControlCmdNonInterlace\n"));

    /* wait a couple fields to make sure any previous mode */
    /* change has had time to take effect */
    HWControlWaitVSync ();
    HWControlWaitVSync ();

    /* check if we're in interlace mode */
    /* wait for vsync */
    HWControlWaitVSync ();
    lastField = *VCNT & ODD_FIELD;

    /* wait for next vsync */
    HWControlWaitVSync ();
    thisField = *VCNT & ODD_FIELD;

    /* if we're in interlace mode, switch to non-interlace */
    if (lastField != thisField)
    {
	/* only switch on an odd field */
	if (thisField != ODD_FIELD)
	    HWControlWaitVSync ();
	SuperSetSysInfo(SYSINFO_TAG_SETNONINTERLACE, (void *)0, 0);
    }
    return (1);				/* return 0 if asynchronous */
}

/*
 * build the system data structures necessary to install the driver code
 *
 * the routines in the command table need to be listed in order corresponding
 * to the indices specified in io.h and the interface header file
 */

static int32            (*HWControlCmdTable[]) () =
{
    HWControlCmdWrite,
    HWControlCmdRead,
    HWControlCmdStatus,
    HWControlCmdInterlace,
    HWControlCmdNonInterlace,
};

/*
 * there is one driver arguments table per driver
 */

static TagArg           drvrArgs[] =
{
    TAG_ITEM_PRI, (void *) 1,
    TAG_ITEM_NAME, HWCONTROL_DRIVER_NAME,
    CREATEDRIVER_TAG_ABORTIO, (void *) ((long)HWControlAbortIO),
    CREATEDRIVER_TAG_MAXCMDS, (void *) (sizeof (HWControlCmdTable) / sizeof (int (*) ())),
    CREATEDRIVER_TAG_CMDTABLE, (void *) HWControlCmdTable,
    CREATEDRIVER_TAG_INIT, (void *) ((long)HWControlDriverInit),
    TAG_END, 0,
};

/*
 * there is one device arguments table per device
 */

static TagArg           DevArgs[] =
{
    TAG_ITEM_PRI, (void *) 150,
    CREATEDEVICE_TAG_DRVR, (void *) 1,
    TAG_ITEM_NAME, HWCONTROL_DEVICE_NAME,
    CREATEDEVICE_TAG_INIT, (void *) ((long)HWControlDevInit),
    CREATEDEVICE_TAG_OPEN, (void *) ((long)HWControlDevOpen),
    CREATEDEVICE_TAG_CLOSE, (void *) ((long)HWControlDevClose),
    TAG_END, 0,
};

void
CreateHWControlDriver (void)
{
    Item                    drvrItem;

    /* create the driver */
    drvrItem = CreateItem (MKNODEID (KERNELNODE, DRIVERNODE), drvrArgs);
    KDBUG (("HWControlDriver: Creating driver returns drvrItem=%d\n", drvrItem));

    /* create each device */
    if (drvrItem >= 0)
    {
	DevArgs[1].ta_Arg = (void *) drvrItem;
	HWControlDeviceItem = CreateItem (MKNODEID (KERNELNODE, DEVICENODE), DevArgs);
	KDBUG (("HWControlDriver: Creating HWControl device returns %d\n", HWControlDeviceItem));
    }
}
