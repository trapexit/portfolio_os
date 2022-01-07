/*
 * file: InstallHWControlDriver.c
 */
/*
 * install HWControl driver
 * 1/12/94 George Mitsuoka
 * The 3DO Company Copyright © 1994
 */

#include "types.h"
#include "debug.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "graphics.h"
#include "kernel.h"
#include "hwcontroldriver.h"

#define	DBUG(x)		/* kprintf x */
#define	INFO(x)		kprintf x

int32                   debugFlag = 0L;
int32                   gSignalWaitVBL, gSignalWaitDone;
struct Task            *gMainTask;
Item                    gClientTaskItem;
Item                    gHWControlDeviceItem;

extern void             CreateHWControlDriver (void);

int
main (int32 argc, char *argv[])
{
    int32                   waitResult, arg, signalMask;
    Item                    mainTaskItem, VBLReq;

    /* process command line arguments */
    for (arg = 0; arg < argc; arg++)
    {
	if (strcmp (argv[arg], "-debug") == 0)
	{
	    debugFlag = 1L;
	    Debug ();
	}
    }
    INFO(("creating driver\n"));

    /* create the driver */
    CreateHWControlDriver ();

    INFO(("createdDriver\n"));

    /* this task occaisionally does a WaitVBL on behalf of the driver */
    /* set up signalling mechanism */
    mainTaskItem = CURRENTTASK->t.n_Item;
    gMainTask = (struct Task *) LookupItem (mainTaskItem);
    if (gMainTask == (struct Task *)0)
    {
	INFO(("hwControlDriver: couldn't find main task\n"));
	goto abort;
    }
    gSignalWaitVBL = AllocSignal (0);
    if (gSignalWaitVBL == 0)
    {
	INFO(("hwControlDriver: couldn't allocate signal\n"));
	goto abort;
    }
    signalMask = gSignalWaitVBL;
    VBLReq = GetVBLIOReq ();

    /* enter wait loop */
    while (1)
    {
	DBUG (("hwControlDriver: Waiting for waitVBL signal\n"));
	waitResult = WaitSignal (signalMask);
	DBUG (("hwControlDriver: Got waitVBL signal 0x%08lx\n", waitResult));
	if (waitResult & SIGF_ABORT)
	{
	    INFO(("hwControlDriver: received SIGF_ABORT\n"));
	    goto abort;
	}
	else if (waitResult & signalMask)
	{
	    DBUG (("hwControlDriver: received wait vbl signal\n"));
	    WaitVBL (VBLReq, 1);
	    SendSignal (gClientTaskItem, gSignalWaitDone);
	}
	else
	    INFO(("hwControlDriver: received spurious signal\n"));
    }
abort:
    INFO(("HWControlDriver exiting\n"));
}
