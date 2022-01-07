/*****

$Id: CDROMDriver.c,v 1.44 1994/12/08 18:17:15 dplatt Exp $

$Log: CDROMDriver.c,v $
 * Revision 1.44  1994/12/08  18:17:15  dplatt
 * Fix CR 3858.  There is a reentrancy problem in the driver - if the
 * level 2 "FinishRequest" code calls SuperCompleteIO, another IOReq
 * can be submitted to the I/O dispatch entry point, and the "turbo"
 * logic can try to give the driver a timeslice and enter the level 2
 * code again.  This leads to all sorts of problems (I'm amazed it
 * never showed up before).  Fix:  check IO_INTERNAL on the IOReq
 * handed to the dispatcher, and don't call the FSM timeslice routine
 * if it's set (since we're already in that code!)
 *
 * Revision 1.43  1994/11/17  07:17:22  bungee
 * If no MKE devices are present, we now return from the driver
 * initialization routine with the newly implemented error,
 * ER_NoHardware.
 *
 * Revision 1.42  1994/11/09  21:12:56  bungee
 * Removed bogus DBUG statements (and useless conditional) related to detection
 * of LCCD.
 *
 * Revision 1.41  1994/10/04  18:51:10  dplatt
 * Refresh the timeout clock after receiving each data block, so that the
 * drive isn't timed out and aborted inappropriately during long reads.
 *
 * Revision 1.40  1994/09/22  00:21:16  bungee
 * Removed the printing code in ScreamAndLeap(); because it caused a stack
 * overflow if you attempted to intentionally kill the daemon.  Also changed
 * the naming convention for multiple drives from "CD-ROM x" to "CD-ROMx".
 *
 * Revision 1.39  1994/09/01  17:18:11  dplatt
 * Remove the code which requires the CD-ROM device to be the first
 * unit on the xbus.  This also enables support for multiple MKE CD-ROM
 * drives.
 *
 * Revision 1.38  1994/08/29  18:22:53  sdas
 * CallBackSuper() now takes three arguments. Also, some comment changes.
 *
 * Revision 1.37  1994/08/02  22:28:50  bungee
 * Scratch that.  The previous fix in 1.36 didn't achieve the desired results;
 * so we're removing the extended timeout changes all together.
 *
 * Revision 1.36  1994/08/02  00:47:28  bungee
 * Corrected a new bug (introduced in rev 1.35) which would intermittently
 * cause the driver to take 30 seconds to timeout after the drawer had been
 * opened...causing it to take 30 sec before the "NoCD" app would take over.
 *
 * Revision 1.35  1994/07/26  01:44:33  dplatt
 * Provide a longer timeout during "spin up" command, to allow multisession
 * CD-ROMs to come on-line properly.  Make sure not to do an xbus poll-
 * reset at inappropriate times (e.g. during timeout recovery).
 *
 * Revision 1.34  1994/07/05  21:30:19  dplatt
 * Improve error recovery after device times out.  Kill pending I/Os if
 * issued when the drawer is open (CR-563 fix).
 *
 * Revision 1.33  1994/06/29  23:19:06  dplatt
 * MKE drive returns "command error" rather than "disc removed while
 * command was in progress" if you hit the eject button during a read.
 * Return a device-offline error rather than a parameter error.
 *
 * Revision 1.32  1994/06/03  00:09:18  dplatt
 * Update error message.
 *
 * Revision 1.31  1994/06/02  23:59:08  dplatt
 * Pick up proper status byte after a READ ERROR (was looking in wrong
 * buffer).  This fixes a problem whereby an I/O error on a CD-ROM
 * would cause a SoundBlaster drive to be soft-reset inappropriately.
 *
 * Revision 1.30  1994/06/01  18:18:15  dplatt
 * Change the soft-reset mechanism around to use the new xbusdevice
 * "reset unit" feature, which forces DIPIR.
 *
 * Revision 1.29  1994/05/17  20:17:47  dplatt
 * Don't print "Drive has not spun up" message at boot time... current
 * development kernel makes this happen a lot.
 *
 * Revision 1.28  1994/05/13  23:36:40  dplatt
 * Add soft-reset code for SoundBlaster.
 *
 * Revision 1.27  1994/05/13  19:55:07  dplatt
 * More SoundBlaster changes
 *
 * Revision 1.26  1994/05/02  21:24:19  dplatt
 * Don't start tickling until a couple of seconds of idle time have gone
 * by.  First cut of SoundBlaster CD-ROM driver changes.
 *
 * Revision 1.25  1994/04/25  18:03:03  bungee
 * Changes required to support a Low Cost CD device being instantiated with
 * device name 'CD-ROM'.  If a LCCD device exists, naming of MKE devices starts
 * at 'CD-ROM 2'; otherwise, MKE devices start at 'CD-ROM', 'CD-ROM 2', etc.
 *
 * Revision 1.24  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.23  1994/02/04  23:28:04  dplatt
 * Switch over to standard device error codes.
 *
 * Revision 1.22  1993/08/05  02:28:25  dale
 * SuperWait -> SuperWaitSignal
 *
 * Revision 1.21  1993/07/26  21:00:35  andy
 * removed obsolete comment
 *
 * Revision 1.20  1993/07/25  05:55:16  andy
 * *** empty log message ***
 *
 * Revision 1.19  1993/07/24  06:57:01  andy
 * multiple drives don't work;  so I disabled them for developer release as
 * well as rom, until Dave gets a chance to investigate
 *
 * Revision 1.18  1993/07/23  01:29:40  dplatt
 * Reworked the multiple-CD-drive fix a bit.  If ROMBUILD is defined at
 * compile time, only the internal drive (xbus unit 0) will be probed to
 * see if it's an MEI CD-ROM drive.  If ROMBUILD is not defined, all
 * xbus units are probed, from unit 0 up to maxunix.  Unit 0, if an MEI
 * drive, will always end up with the Device name of "CD-ROM" thus
 * removing the no-fixed-name syndrome.
 *
 * Revision 1.17  1993/07/19  17:57:59  andy
 * ready for the real fix to go in...
 *
 * Revision 1.16  1993/07/17  03:03:40  andy
 * put endif in proper place
 *
 * Revision 1.15  1993/07/17  00:02:45  andy
 * missing endif added
 *
 * Revision 1.14  1993/07/16  23:57:04  andy
 * Found bug in CDROMDriver.c with multiple units attached.  The wrong device pointer gets
 * stuffed into IOR.  A quick hack to install the correct pointer during IO creation was
 * only partially successful.  Since we want units other than 0 disabled for the ROM anyway,
 * CDROMdriver.c for rom now only looks at unit 0.  No problem.
 *
 * Revision 1.13  1993/07/15  04:47:02  dplatt
 * The CDROM_PASSTHROUGH command is hereby made off-limits to non-
 * privileged programs.  Possible security risks... and it's a bad idea
 * to encourage apps to do device-specific things behind the driver's
 * back.
 *
 * Revision 1.12  1993/07/15  04:35:13  dplatt
 * Volume-offline status might not always be returned to caller.
 *
 * Revision 1.11  1993/07/14  01:45:08  dplatt
 * Also flush the remembered MODE SET data in a few other places where
 * we can confirm that the drive has gone off-line or the drawer is
 * open.
 *
 * Revision 1.10  1993/07/13  23:45:22  dplatt
 * Bug 1: open-drawer and close-drawer commands should be assumed to reset
 *        drive state;  erase remembered-MODE-SET data.
 * Bug 2: don't retry CLOSE DRAWER command if the first one times out after
 *        5 seconds without returning status... the DIPIR process and the
 *        drive conspire to prevent such status info from ever getting back
 *        to the driver.  Assume timeout in a CLOSE DRAWER command means
 *        that it worked.
 *
 * Revision 1.9  1993/07/13  17:54:02  dplatt
 * Move pitch specifiers to CDROMDriver.c, and set the fast and slow
 * pitch values to 1% rather than 10%.
 *
 * Revision 1.8  1993/07/07  22:40:22  dplatt
 * TOC entry for track 99 not being stored properly.
 *
 * Revision 1.7  1993/06/24  01:26:22  andy
 * new stack min is 256 bytes (allows room for stack recovery)
 *
 * Revision 1.6  1993/06/24  00:51:12  dplatt
 * If readahead is active, must abort it before trying to issue a
 * drawer open, drawer close, or read subcode or disc code command.
 *
 * Revision 1.5  1993/06/20  04:03:59  dale
 * fixed CmdStatus, if you passed in a buffer to small it overran it.
 *
 * Revision 1.4  1993/06/18  18:00:30  andy
 * only clears driver specific bits
 *
 * Revision 1.3  1993/06/16  06:22:11  dale
 * thread mke daemon
 *
 * Revision 1.28  1993/06/16  00:48:23  dplatt
 * Turn the MKE Daemon into a thread.
 *
 * Revision 1.27  1993/06/15  03:30:32  dplatt
 * Use a dummy endaction on xbus status request, to get past the
 * privileged-I/O check.
 *
 * Revision 1.26  1993/06/15  00:55:14  dplatt
 * Ensure that OpenItem calls are in place
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  CDROMDriver.c - implements the driver which handles CD ROM drives
*/

#define SUPER

#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "msgport.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "super.h"
#include "cdrom.h"
#include "operror.h"
#include "filesystem.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"

/* #define DEBUG */
/* #define TRACE */
/* #define DEBUG2 */
/* #define DEBUG3 */

/* #define UNGODLYSLEAZE */

#ifdef PRODUCTION
# define qprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif


#ifdef DEBUG
#define SHOWERROR
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifdef DEBUG2
#define DBUG2(x) Superkprintf x
#else
#define DBUG2(x) /* x */
#endif

#ifdef DEBUG3
#define DBUG3(x) Superkprintf x
#else
#define DBUG3(x) /* x */
#endif

#define ZEROABORT
#define SLEEPDRIVE
#define DOUBLESPEED
#define READAHEAD
#define TURBO

#define CDROM_DEFAULT_TIMEOUT 5
#define CDROM_LONG_TIMEOUT    30
#define CDROM_COOLDOWN_MS 0

/* #define NORECOVERY */

#define DAEMONUSERSTACK 256

static char daemonStack[DAEMONUSERSTACK];

#define MakeCDErr(svr,class,err) MakeErr(ER_DEVC,((Make6Bit('C')<<6)|Make6Bit('D')),svr,ER_E_SSTM,class,err)

Driver *CDROMDriver;
Task *daemon;
uint32 daemonSignal;

static int32 wrongSize, badSize, caughtError, eaCount = 0;

static int32 doReadahead = 0;

int32 doDoublespeedByDefault = 0;

int32 CDROMDriverInit(struct Driver *me);
int32 CDROMDriverDeviceInit(CDROM *theDevice);
int32 CDROMDriverDispatch(IOReq *theRequest);

void CDROMDriverAbortIo(IOReq *theRequest);
int32  CDROMDriverDoit(IOReq *theRequest);
void  CDROMDriverSetStatus(IOReq *theRequest);
void  CDROMDriverSetDiscData(IOReq *theRequest);

void ScheduleCDROMIo(CDROM *theDevice);
void StartCDROMIo (CDROM *theDevice);
IOReq *CDROMDriverEndAction(IOReq *theRequest);
IOReq *CDROMDriverDummyEndAction(IOReq *theRequest);
void AbortCDROMIo(IOReq *userRequest);

uint32 CDROMDriverHeartbeat(CDROM *theDevice);
uint32 CDROMDriverTimeSlice(CDROM *theDevice);

static Item expansionItem;

List cdRomDeviceList;

enum DoorOpenState {notOpened = 0,
		    openedMustReset,
                    openedWasReset};

#define MySysErr(x)

static TagArg cdRomDriverTags[] = {
  { TAG_ITEM_NAME,              (void *) "MKE CD-ROM" },
  { CREATEDRIVER_TAG_ABORTIO,   (void *) (int32) CDROMDriverAbortIo },
  { CREATEDRIVER_TAG_MAXCMDS,   (void *) 9 },
  { CREATEDRIVER_TAG_DISPATCH,  (void *) (int32) CDROMDriverDispatch },
  { CREATEDRIVER_TAG_INIT,      (void *) (int32) CDROMDriverInit },
  { TAG_END,                    NULL }
};

static TagArg cdRomDeviceTags[] = {
  { TAG_ITEM_NAME,           (void *) "CD-ROM" },
  { TAG_ITEM_PRI,            (void *) 50, },
  { CREATEDEVICE_TAG_DRVR,    NULL },
  { TAG_END,                 0, },
};

static void ScreamAndLeap(void);

static TagArg cdRomDaemonTags[] =
{
  TAG_ITEM_PRI,	        	(void *) 220,
  CREATETASK_TAG_PC,		(void *) ((long)ScreamAndLeap),
  TAG_ITEM_NAME,		"MKE daemon",
  CREATETASK_TAG_SP,            (void *) (daemonStack+DAEMONUSERSTACK),
  CREATETASK_TAG_STACKSIZE,	(void *) DAEMONUSERSTACK,
  CREATETASK_TAG_SUPER,         (void *) TRUE,
  TAG_END,			0
};

/* Normal pitch - VPON bit (0x04 in MSB) off, zero pitch adjust */

#define MEI_VPITCH_NORMAL_MSB 0x00
#define MEI_VPITCH_NORMAL_LSB 0x00

/* Fast pitch - VPON set, msb/lsb set to 10 for 1% pitch increase */

#define MEI_VPITCH_FAST_MSB   0x04
#define MEI_VPITCH_FAST_LSB   0x0A

/* Slow pitch - VPON set, msb/lsb set to -10 for 1% pitch decrease */

#define MEI_VPITCH_SLOW_MSB   0x07
#define MEI_VPITCH_SLOW_LSB   0xF6

static CDROM_Parameters cdRomDefaultParameters =
{
  /* block: default density, 2048 bytes/block, no flags */
  {
    MEI_CDROM_DEFAULT_DENSITY,
    2048 >> 8,
    2048 & 0xFF,
    0,
    2048
  },
  /* error recovery: defaults */
  {
#ifdef NORECOVERY
    MEI_CDROM_CIRC_RETRIES_ONLY,
    0
#else
    MEI_CDROM_DEFAULT_RECOVERY,
    8
#endif
  },
  /* stop time: default */
  {     
    0
  },
  /* drive speed: default */
  {
#ifdef DOUBLESPEED
    0x80,
#else
    0,
#endif
    MEI_VPITCH_NORMAL_MSB,
    MEI_VPITCH_NORMAL_LSB
  },
  /* chunk size: 1 */
  {
    1
  }
};

#ifdef CDLOG
static void CDROMLog(CDROM *theDevice, uint8 who)
{
  uint32 index;
/*  if (theDevice->cdrom_LogIndex >= CDLOG) return; */
  index = theDevice->cdrom_LogIndex % CDLOG;
  theDevice->cdrom_Log[index].index = theDevice->cdrom_LogIndex ++;
  theDevice->cdrom_Log[index].actor = who;
  theDevice->cdrom_Log[index].level0 = theDevice->cdrom_Level0;
  theDevice->cdrom_Log[index].level1 = theDevice->cdrom_Level1;
  theDevice->cdrom_Log[index].level2 = theDevice->cdrom_Level2;
  theDevice->cdrom_Log[index].doRA = theDevice->cdrom_DoReadahead;
  theDevice->cdrom_Log[index].raActive = theDevice->cdrom_ReadaheadActive;
  
}
# define LOG CDROMLog
# define DISPATCHER 0
# define TIMESLICE 1
# define ABORTIO 2
# define ENDACTION 3
# define ENDOFENDACTION 4
#else
# define LOG(foo,bar) /* foo bar */
#endif

Err CDRomFSM()
{
#define CDROM_HOLDOFF 1
  Item timerDeviceItem, cdRomTimerItem, heartbeatItem;
  CDROM *cdRom;
  CDROMDriveQueueEntry *queueEntry;
  IOReq *cdTimerIOR;
  IOReq *heartbeatIOR;
  int32 cdRomPeriodic, cdRomSlice;
  int32 cdRomTimerRunning;
  int32 heartbeatRunning, startHeartbeat;
  uint32 awakenedMask, sleepMask;
  Err err;
  TagArg ioReqTags[2];
  const struct timeval cdromDelay = {0, CDROM_HOLDOFF * 1000};
  DBUG(("CD-ROM daemon is now in supervisor mode\n"));
  err = 0;
  queueEntry = (CDROMDriveQueueEntry *) FirstNode(&cdRomDeviceList);
  daemonSignal = SuperAllocSignal(0L);
  DBUG(("Seeking timer device\n"));
  timerDeviceItem = SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
				       "timer");
  if (timerDeviceItem < 0) {
    qprintf(("Error finding timer device\n"));
    err = timerDeviceItem;
    goto shutdown;
  }
  DBUG(("Timer is item 0x%x\n", timerDeviceItem));
  timerDeviceItem = SuperOpenItem(timerDeviceItem, (void *) NULL);
  if (timerDeviceItem < 0) {
    qprintf(("Error opening timer device\n"));
    err = timerDeviceItem;
    goto shutdown;
  }
  DBUG(("Timer open\n"));
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) timerDeviceItem;
  ioReqTags[1].ta_Tag = TAG_END;
  cdRomTimerItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (cdRomTimerItem < 0) {
    qprintf(("Error creating IOReq node\n"));
    err = cdRomTimerItem;
    goto shutdown;
  }
  DBUG(("CD-ROM timer IOReq is item 0x%x ", cdRomTimerItem));
  cdTimerIOR = (IOReq *) LookupItem(cdRomTimerItem);
  DBUG(("at 0x%x\n", cdTimerIOR));
  heartbeatItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (heartbeatItem < 0) {
    qprintf(("Error creating IOReq node\n"));
    err = heartbeatItem;
    goto shutdown;
  }
  DBUG(("Heartbeat timer IOReq is item 0x%x ", cdRomTimerItem));
  heartbeatIOR = (IOReq *) LookupItem(heartbeatItem);
  DBUG(("at 0x%x\n", heartbeatIOR));
  DBUG(("Setting up heartbeat IOReq\n"));
  heartbeatIOR->io_Info.ioi_Unit = 0;
  heartbeatIOR->io_Info.ioi_Command = TIMERCMD_DELAY;
  heartbeatIOR->io_Info.ioi_Send.iob_Buffer = NULL;
  heartbeatIOR->io_Info.ioi_Send.iob_Len = 0;
  heartbeatIOR->io_Info.ioi_Recv.iob_Buffer = NULL;
  heartbeatIOR->io_Info.ioi_Recv.iob_Len = 0;
  heartbeatIOR->io_Info.ioi_Offset = 60;
  DBUG(("Setting heartbeattimer\n"));
  if ((err = SuperinternalSendIO(heartbeatIOR)) < 0) {
    qprintf(("Error posting timer delay request"));
    goto shutdown;
  } else {
    heartbeatRunning = TRUE;
  }
  sleepMask = daemonSignal | SIGF_IODONE;
  cdRomTimerRunning = FALSE;
  daemon = CURRENTTASK;
  do {
    DBUG(("CD daemon zzz...\n"));
    awakenedMask = SuperWaitSignal(sleepMask);
    DBUG(("CD daemon awake\n"));
    startHeartbeat = FALSE;
    cdRomPeriodic = FALSE;
    cdRomSlice = FALSE;
    if (awakenedMask & SIGF_ABORT) {
      break;
    }
    if (awakenedMask & daemonSignal) {
      DBUG(("CD-ROM signal\n"));
      cdRomSlice = TRUE;
    }
    if (awakenedMask & SIGF_IODONE) {
      if (cdRomTimerRunning && CheckIO(cdRomTimerItem)) {
	DBUG(("CD-ROM timer runout, do next I/O\n"));
	cdRomSlice = TRUE;
	cdRomTimerRunning = FALSE;
      }
      if (heartbeatRunning && CheckIO(heartbeatItem)) {
	DBUG(("Heartbeat timer runout, do next I/O\n"));
	heartbeatRunning = FALSE;
	queueEntry = (CDROMDriveQueueEntry *) FirstNode(&cdRomDeviceList);
	while (IsNode(&cdRomDeviceList,queueEntry)) {
	  cdRom = queueEntry->cdrdqe_Device;
	  cdRomSlice |= CDROMDriverHeartbeat(cdRom);
	  queueEntry = (CDROMDriveQueueEntry *) NextNode(queueEntry);
	}
	DBUG(("Heartbeats done\n"));
	if (SuperinternalSendIO(heartbeatIOR) < 0) {
	  heartbeatRunning = FALSE;
	  qprintf(("Error posting timer delay request"));
	} else {
	  heartbeatRunning = TRUE;
	}
      }
    }
    if (cdRom && cdRomSlice) {
	queueEntry = (CDROMDriveQueueEntry *) FirstNode(&cdRomDeviceList);
	while (IsNode(&cdRomDeviceList,queueEntry)) {
	  cdRom = queueEntry->cdrdqe_Device;
	  cdRomPeriodic |= CDROMDriverTimeSlice(cdRom);
	  queueEntry = (CDROMDriveQueueEntry *) NextNode(queueEntry);
	}
    }
    if (cdRomPeriodic && !cdRomTimerRunning) {
      cdTimerIOR->io_Info.ioi_Unit = 1;
      cdTimerIOR->io_Info.ioi_Command = TIMERCMD_DELAY;
      cdTimerIOR->io_Info.ioi_Send.iob_Buffer = (void *) &cdromDelay;
      cdTimerIOR->io_Info.ioi_Send.iob_Len = sizeof cdromDelay;
     DBUG(("Setting timer\n"));
      if (SuperinternalSendIO(cdTimerIOR) < 0) {
	qprintf(("Error posting timer delay request"));
      } else {
	cdRomTimerRunning = TRUE;
      }
    }
  } while (TRUE);
 shutdown:
  return err;
}

void ScreamAndLeap()
{
  Err err;
#ifdef DEBUG
  kprintf("CD-ROM daemon task created, hyperjumping!\n");
#endif
  err =  CallBackSuper(CDRomFSM, 0, 0, 0);
#if 0
/* This code has been ifdef'd out to allow the daemon to be killed.  */
/* With the advent of stack space reduction this printing causes the */
/* system to crash.                                                  */
#ifndef	PrintError
  kprintf("CD-ROM daemon callback exit, code 0x%x!!! AAGH!\n", err);
  PrintfSysErr(err);
#else
  PrintError(0,"\\CD-ROM callback failed",0,err);
#endif
#endif
  DeleteItem(CURRENTTASK->t.n_Item);
}

Item createCDRomDriver()
{
  Item i;
  i = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE), cdRomDriverTags);
  return i;
}

uint32 beq(void *s, void *d, int32 n)
{
  char *s1 = (char *) s;
  char *d1 = (char *) d;
  while (--n >= 0) {
    if (*s1++ != *d1++) return 0;
  }
  return 1;
}

int32 CDROMDriverInit(struct Driver *me)
{
  Item i;
  TagArg tags[2];
  Item ioReqItem, lccdItem, cdRomItem, daemonItem;
  Err err;
  IOReq *ioReq;
  int unit, maxUnit;
  int devNum;
  int devNameBaseChar;
  int32 foundIt, tweaks;
  static char devName[] = "CD-ROMx";
  struct XBusDeviceStatus xbds;
  CDROM *theDevice;
  CDROMDriveQueueEntry *queueEntry;
  DBUG(("CD-ROM driver init, driver at %x!\n", me));
  devNum = 0;
  expansionItem = -1;
  InitList(&cdRomDeviceList, "cdroms");
  i = SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
			 "xbus");
  if (i < 0) {
    DBUG(("Could not find expansion-bus device, 0x%x\n", i));
    return i;
  }
  expansionItem = SuperOpenItem(i, (void *) NULL);
  if (expansionItem < 0) {
    DBUG(("Could not open expansion-bus device, 0x%x\n", expansionItem));
    return expansionItem;
  } else {
    DBUG(("Expansion-bus device is item 0x%x\n", expansionItem));
  }
  /* Look for LowCostCD device first */
  lccdItem = SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE), "CD-ROM");
  devNameBaseChar = (lccdItem < 0) ? '0' : '1';

#ifdef DEBUG
  DBUG(("Creating IOReq\n"));
#endif
  tags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  tags[0].ta_Arg = (void *) expansionItem;
  tags[1].ta_Tag = TAG_END;
  ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), tags);
  if (ioReqItem < 0) {
    DBUG(("Could not create expansion-bus IOReq\n"));
    return ioReqItem;
  }
  ioReq = (IOReq *) LookupItem(ioReqItem);
  cdRomDeviceTags[2].ta_Arg = (void *) me->drv.n_Item;
  maxUnit = ((Device *)LookupItem(expansionItem))->dev_MaxUnitNum;
  for (unit = 0; unit <= maxUnit; unit++) {
    ioReq->io_Info.ioi_Command = CMD_STATUS;
    ioReq->io_Info.ioi_Unit = unit;
    ioReq->io_Info.ioi_Send.iob_Buffer = NULL;
    ioReq->io_Info.ioi_Send.iob_Len = 0;
    ioReq->io_Info.ioi_Recv.iob_Buffer = &xbds;
    ioReq->io_Info.ioi_Recv.iob_Len = sizeof xbds;
    ioReq->io_Info.ioi_Offset = 0;
    DBUG(("SendIO of expansion bus status request for unit %d\n", unit));
    err = SuperinternalSendIO(ioReq);
    if (err >= 0) {
      DBUG(("WaitIO of expansion bus status request for unit %d\n", unit));
      err = SuperWaitIO(ioReqItem);
      DBUG(("Got it\n"));
      if (err == 0 && ioReq->io_Error == 0) {
	DBUG(("Found something at expansion-bus unit %d\n", unit));
	DBUG(("io_Actual = %d, asked for %d\n", ioReq->io_Actual, sizeof xbds));
	DBUG(("mfg %d dev %d rev %d\n", xbds.xbus_ManuIdNum, xbds.xbus_ManuDevNum, xbds.xbus_ManuRevNum));
	foundIt = FALSE;
	tweaks = 0x00;
	if (xbds.xbus_ManuIdNum == XBUS_MFG_MEI &&
	    xbds.xbus_ManuDevNum == XBUS_DEVICE_CDROM) {
	  foundIt = TRUE;
	  tweaks = CDROM_TWEAK_SUPPORTS_CHUNKS | CDROM_TWEAK_SENDS_TAG |
	           CDROM_TWEAK_SUPPORTS_SUBCODE;
	} else if (xbds.xbus_ManuIdNum == 0x4352 /* 'CR' */ &&
		   xbds.xbus_ManuDevNum == 0x2D35 /* '-5' */ &&
		   xbds.xbus_ManuRevNum == 0x3633 /* '63' */) {
/*
   TBD - add a clause to SuperQuerySysInfo and check to see whether this
   system is allowed to support the SoundBlaster drive, which does not do
   Media Access.
*/
	  foundIt = TRUE;
	  tweaks = CDROM_TWEAK_SLOW_DMA | CDROM_TWEAK_EMULATE_MEDIA_ACCESS;
	}
	if (foundIt) {
	  if (!(xbds.xbus_Flags & XBUS_OLDSTAT)) {
	    tweaks |= CDROM_TWEAK_SENDS_TAG;
	  }
	  DBUG(("Creating MEI CD-ROM device for xbus unit %d\n", unit));
	  devNum++;
	  devName[6] = devNameBaseChar + devNum;
	  if (devName[6] >= '2')
	    cdRomDeviceTags[0].ta_Arg = devName;
	  cdRomItem = SuperCreateSizedItem(MKNODEID(KERNELNODE,DEVICENODE),
			      cdRomDeviceTags,
			      sizeof (CDROM));
	  if (cdRomItem < 0) {
	    DBUG(("Couldn't create CD-ROM device (0x%x)\n", cdRomItem));
	    MySysErr((uint32) cdRomItem);
	    goto noRoms;
	  }
	  theDevice = (CDROM *) LookupItem(cdRomItem);
	  theDevice->cdrom_XBusUnit = unit;
	  theDevice->cdrom_TweakFlags = (uint8) tweaks;
	  queueEntry = (CDROMDriveQueueEntry *) AllocMem (sizeof (CDROMDriveQueueEntry), MEMTYPE_FILL);
	  queueEntry->cdrdqe_Device = theDevice;
	  (void) CDROMDriverDeviceInit(theDevice);
	  if (tweaks & CDROM_TWEAK_SLOW_DMA) {
	    ioReq->io_Info.ioi_Command = XBUSCMD_SetXferSpeed;
	    ioReq->io_Info.ioi_Unit = unit;
	    ioReq->io_Info.ioi_Recv.iob_Buffer = NULL;
	    ioReq->io_Info.ioi_Recv.iob_Len = 0;
	    ioReq->io_Info.ioi_Offset = 640; /* 640 seems to work reliably */
	    DBUG(("CD-ROM: setting expansion bus status xfer speed for unit %d\n", unit));
	    err = SuperinternalSendIO(ioReq);
	    if (err >= 0) {
	      err = SuperWaitIO(ioReqItem);
	    }
	    if (err == 0 && (err = ioReq->io_Error) == 0) {
	      DBUG(("OK\n"));
	    } else {
	      DBUG(("Speed-set rejected\n"));
	      MySysErr(err);
	    }
	  }
 	  AddHead(&cdRomDeviceList, (Node *) queueEntry);
	  DBUG(("Added new CD-ROM device 0x%x at 0x%x\n", cdRomItem,
		theDevice));
	}
      }
    }
  }
 noRoms: ;
  SuperDeleteItem(ioReqItem);
  if (doDoublespeedByDefault) {
    cdRomDefaultParameters.driveSpeed.speed = MEI_CDROM_DOUBLE_SPEED;
    DBUG(("Doublespeed enabled\n"));
  }
  if (!devNum)		/* if no MKE devices present */
    return MakeCDErr(ER_SEVER, ER_C_STND, ER_NoHardware);
#ifdef TURBO
  DBUG(("Turbo enabled\n"));
#endif
  DBUG(("Creating CD-ROM resident task\n"));
  daemonItem = SuperCreateItem(MKNODEID(KERNELNODE,TASKNODE),
			       cdRomDaemonTags);
  if (daemonItem < 0) {
    qprintf(("Could not create CD-ROM daemon task\n"));
    MySysErr(daemonItem);
  }
  return me->drv.n_Item;
}

void CDROMDriverAbortIo(IOReq *theRequest)
{
  CDROM *theDevice;
  theDevice = (CDROM *) theRequest->io_Dev;
  theRequest->io_Error = ABORTED;
  if (theRequest == theDevice->cdrom_RequestRunning) {
/* 
   The following is not quite right, because the IO system assumes that
   all requests can be aborted synchronously.  The CD-ROM driver
   requires a chance to clean up the request before permitting the
   abort to complete.  Dale and I need to do some work on this...
*/
#ifdef NUKEIT
    switch (theDevice->cdrom_Level0) {
    case CDROM_Level0_Command:
      SuperinternalAbortIO(theDevice->cdrom_CommandStatusReq);
      break;
    case CDROM_Level0_Data:
      SuperinternalAbortIO(theDevice->cdrom_DataReq);
      break;
    case CDROM_Level0_CommandData:
      SuperinternalAbortIO(theDevice->cdrom_DataReq);
      SuperinternalAbortIO(theDevice->cdrom_CommandStatusReq);
      break;
    }
#endif
  } else {
    RemNode((Node *) theRequest);
    SuperCompleteIO(theRequest);
  }
  return;
}

uint32 CDROMDriverHeartbeat(CDROM *theDevice)
{
  int32 interrupts;
  DBUG2(("Thump %d %d %d...\n", theDevice->cdrom_Level0, theDevice->cdrom_Level1, theDevice->cdrom_Level2));
  if (theDevice->cdrom_Level0 == CDROM_Level0_Idle &&
      theDevice->cdrom_Level1 == CDROM_Level1_Idle &&
      (theDevice->cdrom_Level2 == CDROM_Level2_Await_Request ||
       theDevice->cdrom_Level2 == CDROM_Level2_Idle)) {
    if (theDevice->cdrom_TickleDelay > 0) {
      theDevice->cdrom_TickleDelay --;
      return FALSE;
    }
    theDevice->cdrom_Level2 = CDROM_Level2_Poke_Drive;
    return TRUE;
  }
  if (theDevice->cdrom_TimeoutClock == 0 ||
      --theDevice->cdrom_TimeoutClock > 0) {
    return FALSE;
  }
  DBUG0(("CD-ROM timeout on command 0x%X!\n",
	 theDevice->cdrom_CommandBuffer[0]));
  DBUG(("  Level 0/1/2 FSM states are %d, %d, %d\n",
	 theDevice->cdrom_Level0,
	 theDevice->cdrom_Level1,
	 theDevice->cdrom_Level2));
  if (theDevice->cdrom_RequestRunning) {
    DBUG(("  io_actual for request is %d\n",
	   theDevice->cdrom_RequestRunning->io_Actual));
  }
  interrupts = Disable();
  theDevice->cdrom_TimedOut = TRUE;
  switch (theDevice->cdrom_Level0) {
  case CDROM_Level0_Command:
    SuperinternalAbortIO(theDevice->cdrom_CommandStatusReq);
    break;
  case CDROM_Level0_Data:
    SuperinternalAbortIO(theDevice->cdrom_DataReq);
    break;
  case CDROM_Level0_CommandData:
    SuperinternalAbortIO(theDevice->cdrom_DataReq);
    SuperinternalAbortIO(theDevice->cdrom_CommandStatusReq);
    break;
  }
  Enable(interrupts);
  return FALSE;
}

int32 CDROMDriverDispatch (IOReq *theRequest)
{
  CDROM *theDevice;
  switch (theRequest->io_Info.ioi_Command) {
  case CMD_READ:
  case CDROMCMD_SETDEFAULTS:
  case CDROMCMD_READ_SUBQ:
  case CDROMCMD_READ_DISC_CODE:
  case CDROMCMD_OPEN_DRAWER:
  case CDROMCMD_CLOSE_DRAWER:
    return CDROMDriverDoit(theRequest);
  case CDROMCMD_PASSTHROUGH:
    if (((CURRENTTASK->t.n_Flags & TASK_SUPER) == 0) &&
	(theRequest->io_CallBack == NULL)) {
      theRequest->io_Error =  BADPRIV; 
      return 1;
    }
    return CDROMDriverDoit(theRequest);
  case CMD_STATUS:
    theDevice = (CDROM *) theRequest->io_Dev;
    if (! theDevice->cdrom_DiscInfoAvailable) {
      return CDROMDriverDoit(theRequest);
    }
    CDROMDriverSetStatus(theRequest);
    if (theRequest->io_Flags & IO_INTERNAL) {
      return 1;
    }
    SuperCompleteIO(theRequest);
    break;
  case CDROMCMD_DISCDATA:
    theDevice = (CDROM *) theRequest->io_Dev;
    if (! theDevice->cdrom_DiscInfoAvailable) {
      return CDROMDriverDoit(theRequest);
    }
    CDROMDriverSetDiscData(theRequest);
    if (theRequest->io_Flags & IO_INTERNAL) {
      return 1;
    }
    SuperCompleteIO(theRequest);
    break;
  default:
    return MakeCDErr(ER_SEVER,ER_C_STND,ER_BadCommand);
  }
  return 0;
}

int32 CDROMDriverSendCommand(CDROM *theDevice,
			     uint32 commandCode,
			     uint32 statusExpected,
			     uint32 useErrorBuffer)
{
  IOReq *ior;
  int32 err;
  uint32 interrupts;
#ifdef DEBUG
  int32 i;
#endif
#ifdef CLEARSTATUS
  if (commandCode != CDROM_READ_ERROR) {
    theDevice->cdrom_StatusByte = 0x00;
  }
#endif
  theDevice->cdrom_CommandBuffer[0] = (uint8) commandCode;
  ior = theDevice->cdrom_CommandStatusReq;
  ior->io_Info.ioi_Command = CMD_STATUS;
  ior->io_Info.ioi_Send.iob_Buffer = NULL;
  ior->io_Info.ioi_Send.iob_Len = 0;
  ior->io_Info.ioi_Recv.iob_Buffer = &theDevice->cdrom_StatusBuffer;
  ior->io_Info.ioi_Recv.iob_Len = 1;
  ior->io_CallBack = CDROMDriverDummyEndAction;
  ior->io_Flags &= 0x0000ffff;	/* clear the upper bits only */

  ior->io_Info.ioi_Flags = IO_QUICK;
  caughtError = FALSE;
  DBUG3(("Issuing CMD_STATUS to poll CD-ROM\n"));
  err = SuperinternalSendIO(ior);
  if (err < 0) {
    DBUG(("CD-ROM error 0x%x polling drive\n", err));
    return MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
  }
  DBUG3(("Command accepted.\n"));
  err = SuperWaitIO(ior->io.n_Item);
  if (err < 0 || (err = ior->io_Error) < 0) {
    DBUG(("CD-ROM error 0x%x polling drive\n", err));
    return MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
  }
  DBUG3(("CD-ROM status byte is 0x%x\n", theDevice->cdrom_StatusBuffer[0]));
  if (theDevice->cdrom_StatusBuffer[0] == 0xFF) {
    DBUG(("CD-ROM drive appears not to be present\n"));
    return MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
  }
#ifdef SYNCREQUIRED
  switch (commandCode) {
  case CDROM_ABORT:
  case CDROM_READ_ERROR:
  case CDROM_FLUSH:
    ior->io_Info.ioi_Command = XBUSCMD_CommandSyncStat;
    break;
  default:
    ior->io_Info.ioi_Command = XBUSCMD_Command;
    break;
  }
#else
  ior->io_Info.ioi_Command = XBUSCMD_Command;
#endif
  ior->io_Info.ioi_Send.iob_Buffer = &theDevice->cdrom_CommandBuffer;
  if (commandCode == CDROM_ABORT) {
    ior->io_Info.ioi_Send.iob_Len = 1;
  } else {
    ior->io_Info.ioi_Send.iob_Len = 7;
  }
  if (statusExpected == -1) {
    ior->io_Info.ioi_Recv.iob_Buffer = (void *) NULL;
  } else if (useErrorBuffer) {
    ior->io_Info.ioi_Recv.iob_Buffer = theDevice->cdrom_ErrorBuffer;
    theDevice->cdrom_ErrorBuffer[statusExpected] = CDROM_STATUS_ERROR;
  } else {
    ior->io_Info.ioi_Recv.iob_Buffer = theDevice->cdrom_StatusBuffer;
    theDevice->cdrom_StatusBuffer[statusExpected] = CDROM_STATUS_ERROR;
  }
  if (statusExpected == -1) {
    ior->io_Info.ioi_Recv.iob_Len = 0;
  } else {
    ior->io_Info.ioi_Recv.iob_Len = sizeof theDevice->cdrom_StatusBuffer;
  }
  ior->io_CallBack = CDROMDriverEndAction;
#ifdef DEBUG
  {
    int i;
    DBUG0(("Issue command for xunit %ld, unit %ld:",theDevice->cdrom_XBusUnit,ior->io_Info.ioi_Unit));
    for (i = 0; i < 7; i++) {
      DBUG0((" %02x", theDevice->cdrom_CommandBuffer[i]));
    }
    DBUG0((" length %d expect %d\n", ior->io_Info.ioi_Send.iob_Len, ior->io_Info.ioi_Recv.iob_Len));
  }
#endif
/*
   For SoundBlaster CD-ROM drive compatibility (MKE 563) pretend that the
   drive actually returned a tag byte.
*/
  if (!(theDevice->cdrom_TweakFlags & CDROM_TWEAK_SENDS_TAG) &&
      ior->io_Info.ioi_Recv.iob_Buffer != NULL) {
    ((char *)ior->io_Info.ioi_Recv.iob_Buffer)[0] =
      theDevice->cdrom_CommandBuffer[0];
    ior->io_Info.ioi_Recv.iob_Buffer = 1 + (char *) ior->io_Info.ioi_Recv.iob_Buffer;
    ior->io_Info.ioi_Recv.iob_Len --;
  }
  interrupts = Disable();
  switch (theDevice->cdrom_Level0) {
  case CDROM_Level0_Idle:
    theDevice->cdrom_Level0 = CDROM_Level0_Command;
    break;
  case CDROM_Level0_Data:
    theDevice->cdrom_Level0 = CDROM_Level0_CommandData;
    break;
  default:
    Enable(interrupts);
    DBUG(("AAGH! Sent CD-ROM while in wrong mode %d!\n", theDevice->cdrom_Level0));
    return -1;
  }
  if (theDevice->cdrom_Level1 == CDROM_Level1_Idle) {
    theDevice->cdrom_Level1 = CDROM_Level1_Running;
  }
  theDevice->cdrom_TimeoutClock = CDROM_DEFAULT_TIMEOUT;
  if (commandCode != CDROM_READ_ERROR) {
    theDevice->cdrom_TimedOut = FALSE;
  }
  err = SuperinternalSendIO(ior);
  if (err == 0) {
    err = ior->io_Error;
  }
  Enable(interrupts);
  if (err >= 0) {
    DBUG2(("Command accepted.\n"));
  } else {
    DBUG(("CD-ROM error 0x%x sending command\n", err));
    theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
    theDevice->cdrom_Level1 = CDROM_Level1_Idle;
  }
  return err;
}

uint32 OffsetToMSF(int32 offset)
{
  uint32 minutes, seconds, fields;
  minutes = offset / (75L * 60L);
  offset -= minutes * (75L * 60L);
  seconds = offset / 75L;
  fields = offset - seconds * 75L;
  return (minutes << 16) + (seconds << 8) + fields;
}

uint32 MSFToOffset(int32 msf)
{
  uint32 minutes, seconds, fields;
  minutes = (msf >> 16) & 0xFF;
  seconds = (msf >> 8) & 0xFF;
  fields = msf & 0xFF;
  return ((minutes * 60) + seconds) * 75 + fields;
}

void PollReset(CDROM *theDevice)
{
  IOReq *ior;
  Err err;
  ior = theDevice->cdrom_CommandStatusReq;
  ior->io_Info.ioi_Command = XBUSCMD_ResetUnit;
  ior->io_Info.ioi_Send.iob_Buffer = NULL;
  ior->io_Info.ioi_Send.iob_Len = 0;
  ior->io_Info.ioi_Recv.iob_Buffer = NULL;
  ior->io_Info.ioi_Recv.iob_Len = 0;
  ior->io_CallBack = CDROMDriverDummyEndAction;
  ior->io_Flags &= 0x0000ffff;	/* clear the upper bits only */
  ior->io_Info.ioi_Flags = IO_QUICK;
  DBUG0(("Issuing XBUSCMD_ResetUnit to reset CD-ROM\n"));
  err = SuperinternalSendIO(ior);
  if (err < 0) {
    DBUG(("CD-ROM error 0x%x resetting drive\n", err));
    return;
  }
  DBUG(("Command sent\n"));
  err = SuperWaitIO(ior->io.n_Item);
  if (err != 0 || 0 != (err = ior->io_Error)) {
    DBUG(("CD-ROM error 0x%x resetting drive\n", err));
  }
  DBUG(("Command complete\n"));
}

int32 CDROMDriverLevel2Idle(CDROM *theDevice)
{
  int32 err;
  IOReq *ior, *dataReq;
  int32 driveBlockSize;
  int32 blockCount;
  uint32 blockOffset;
  uint32 userOffset;
  uint32 croak;
  uint32 interrupts;
  uint32 readingAhead;
  uint32 abortReadahead;
  uint32 xfer;
  uint32 requiresSubcode;
  union CDROMCommandOptions options;
  DBUG2(("Level 2 idle, state %d\n", theDevice->cdrom_Level2));
  /* UGLY BUT FAST */
  *     (uint32 *) theDevice->cdrom_CommandBuffer  = 0;
  * (1+ (uint32 *) theDevice->cdrom_CommandBuffer) = 0;
  /* UGLY BUT FAST */
  err = TRUE;
  switch (theDevice->cdrom_Level2) {
  case CDROM_Level2_Idle:
#ifdef SLEEPDRIVE
    if (theDevice->cdrom_RequestRunning == NULL &&
	ISEMPTYLIST(&theDevice->cdrom_RequestsToDo) &&
	theDevice->cdrom_DoorOpened == notOpened) {
      return FALSE;
    } else {
      theDevice->cdrom_Level2 = CDROM_Level2_Wake_Up;
      DBUG(("Initiating wakeup from level-2 idle\n"));
      return TRUE;
    }
#else
    theDevice->cdrom_Level2 = CDROM_Level2_Wake_Up;
    DBUG(("Initiating wakeup from level-2 idle\n"));
#endif
    break;
  case CDROM_Level2_Wake_Up:
    DBUG(("Level 2 awake, initiate data path check\n"));
    theDevice->cdrom_Level2 = CDROM_Level2_Check_Data_Path;
    break;
  case CDROM_Level2_Check_Data_Path:
#ifdef DEBUG
    memset(theDevice->cdrom_StatusBuffer, '?',
	   sizeof theDevice->cdrom_StatusBuffer);
#endif
    DBUG(("Checking CD-ROM drive\n"));
    if (CDROMDriverSendCommand(theDevice,
			       CDROM_DATA_PATH_CHECK, 2, FALSE) < 0) {
      theDevice->cdrom_Level2 = CDROM_Level2_Kill_Pending_IOs;
    }
    break;
  case CDROM_Level2_Spin_Up:
    switch (theDevice->cdrom_DoorOpened) {
    case openedMustReset:
      theDevice->cdrom_Level2 = CDROM_Level2_Idle;
      if (theDevice->cdrom_StatusByte & CDROM_STATUS_DOOR) {
	DBUG(("Door has been closed, resetting drive\n"));
	theDevice->cdrom_DoorOpened = notOpened;
	theDevice->cdrom_Holdoff_MS = 5000;
	theDevice->cdrom_Level2 = CDROM_Level2_Idle;
	theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
	PollReset(theDevice);
	DBUG(("Reset complete, begin 5-second idle\n"));
	return TRUE;
      }
      DBUG(("Drawer not closed yet, kill pending I/Os\n"));
      theDevice->cdrom_Level2 = CDROM_Level2_Kill_Pending_IOs;
      return TRUE;
    case openedWasReset:
      theDevice->cdrom_DoorOpened = notOpened;
      DBUG(("Post-reset timeout done, try first spin-up\n"));
    case notOpened:
      CDROMDriverSendCommand(theDevice, CDROM_SPIN_UP, 0, FALSE); 
      break;
    }
    break;
  case CDROM_Level2_Read_Disc_ID:
    theDevice->cdrom_NextBlockOffset = -1;
    memset(theDevice->cdrom_TOC_Entry, 0,
	   (size_t) sizeof theDevice->cdrom_TOC_Entry);
    CDROMDriverSendCommand(theDevice, CDROM_READ_DISC_INFORMATION, 6, FALSE); 
    break;
  case CDROM_Level2_Read_TOC_Entry:
    DBUG(("Next track to probe is %d\n", theDevice->cdrom_TrackToProbe));
    if (theDevice->cdrom_TrackToProbe >
	theDevice->cdrom_DiscInformation.lastTrackNumber) {
      theDevice->cdrom_Level2 = CDROM_Level2_Read_Session;
    } else {
      theDevice->cdrom_CommandBuffer[2] =
	(uint8) theDevice->cdrom_TrackToProbe;
      CDROMDriverSendCommand(theDevice, CDROM_READ_TOC, 8, FALSE);
    }
    break;
  case CDROM_Level2_Read_Session:
    CDROMDriverSendCommand(theDevice, CDROM_READ_SESSION_INFORMATION, 6, FALSE);
    break;
  case CDROM_Level2_Read_Capacity:
    CDROMDriverSendCommand(theDevice, CDROM_READ_CAPACITY, 6, FALSE);
    break;
  case CDROM_Level2_Await_Request:
    if ((ior = theDevice->cdrom_RequestRunning) == (IOReq *) NULL) {
      if (ISEMPTYLIST(&theDevice->cdrom_RequestsToDo)) {
	return FALSE;
      }
      DBUG(("Got a CD-ROM IOReq to parse\n"));
      ior = (IOReq *) FIRSTNODE(&theDevice->cdrom_RequestsToDo);
      RemNode((Node *) ior);
      theDevice->cdrom_RequestRunning = ior;
    }
    options.asLongword = ior->io_Info.ioi_CmdOptions;
    theDevice->cdrom_DesiredParameters = cdRomDefaultParameters;
    croak = FALSE;
    if (options.asLongword != 0) {
      switch (options.asFields.densityCode) {
      case CDROM_Option_Unspecified:
	break;
      case CDROM_DEFAULT_DENSITY:
	theDevice->cdrom_DesiredParameters.block.densityCode =
	  MEI_CDROM_DEFAULT_DENSITY;
	break;
      case CDROM_DATA: 
	theDevice->cdrom_DesiredParameters.block.densityCode =
	  MEI_CDROM_DATA;
	break;
      case CDROM_MODE2_XA:
	theDevice->cdrom_DesiredParameters.block.densityCode =
	  MEI_CDROM_MODE2_XA;
	break;
      case CDROM_DIGITAL_AUDIO:
	theDevice->cdrom_DesiredParameters.block.densityCode =
	  MEI_CDROM_DIGITAL_AUDIO;
	break;
      default:
	croak = TRUE;
      }
      switch (options.asFields.errorRecovery) {
      case CDROM_Option_Unspecified:
	break;
      case CDROM_DEFAULT_RECOVERY:
	theDevice->cdrom_DesiredParameters.errorRecovery.type =
	  MEI_CDROM_DEFAULT_RECOVERY;
	break;
      case CDROM_CIRC_RETRIES_ONLY:
	theDevice->cdrom_DesiredParameters.errorRecovery.type =
	  MEI_CDROM_CIRC_RETRIES_ONLY;
	break;
      case CDROM_BEST_ATTEMPT_RECOVERY:
	theDevice->cdrom_DesiredParameters.errorRecovery.type =
	  MEI_CDROM_BEST_ATTEMPT_RECOVERY;
	break;
      default:
	croak = TRUE;
      }
      if (options.asFields.errorRecovery != CDROM_Option_Unspecified) {
	theDevice->cdrom_DesiredParameters.errorRecovery.retryCount =
	  (1 << options.asFields.retryShift) - 1;
      }
      switch (options.asFields.speed) {
      case CDROM_Option_Unspecified:
	break;
      case CDROM_SINGLE_SPEED:
	theDevice->cdrom_DesiredParameters.driveSpeed.speed =
	  MEI_CDROM_SINGLE_SPEED;
	break;
      case CDROM_DOUBLE_SPEED:
	theDevice->cdrom_DesiredParameters.driveSpeed.speed =
	  MEI_CDROM_DOUBLE_SPEED;
	break;
      default:
	croak = TRUE;
      }
      switch (options.asFields.pitch) {
      case CDROM_Option_Unspecified:
	break;
      case CDROM_PITCH_SLOW:
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchMSB =
	  MEI_VPITCH_SLOW_MSB;
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchLSB =
	  MEI_VPITCH_SLOW_LSB;
	break;
      case CDROM_PITCH_NORMAL:
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchMSB =
	  MEI_VPITCH_NORMAL_MSB;
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchLSB =
	  MEI_VPITCH_NORMAL_LSB;
	break;
      case CDROM_PITCH_FAST:
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchMSB =
	  MEI_VPITCH_FAST_MSB;
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchLSB =
	  MEI_VPITCH_FAST_LSB;
	break;
      default:
	croak = TRUE;
      }
      if (options.asFields.blockLength != CDROM_Option_Unspecified) {
	driveBlockSize = options.asFields.blockLength;
	theDevice->cdrom_DesiredParameters.block.userBlockSize =
	  driveBlockSize;
	requiresSubcode = FALSE;
	switch (driveBlockSize) {
	case CDROM_DA_PLUS_ERR:
	  driveBlockSize = CDROM_DA;
	  theDevice->cdrom_DesiredParameters.block.flags = 0x80;
	  break;
	case CDROM_DA_PLUS_SUBCODE:
	  driveBlockSize = CDROM_DA;
	  theDevice->cdrom_DesiredParameters.block.flags = 0x40;
	  requiresSubcode = TRUE;
	  break;
	case CDROM_DA_PLUS_BOTH:
	  driveBlockSize = CDROM_DA;
	  theDevice->cdrom_DesiredParameters.block.flags = 0xC0;
	  requiresSubcode = TRUE;
	  break;
	default:
	  theDevice->cdrom_DesiredParameters.block.flags = 0x00;
	  break;
	}
	if (requiresSubcode &&
	    !(theDevice->cdrom_TweakFlags & CDROM_TWEAK_SUPPORTS_SUBCODE)) {
	  croak = TRUE;
	}
	theDevice->cdrom_DesiredParameters.block.lengthMSB = 
	  (uint8) (driveBlockSize >> 8) & 0x0F;
	theDevice->cdrom_DesiredParameters.block.lengthLSB = 
	  (uint8) (driveBlockSize & 0xFF);
      }
      if (options.asFields.addressFormat == CDROM_Address_Abs_MSF) {
	ior->io_Info.ioi_Offset= MSFToOffset(ior->io_Info.ioi_Offset);
	options.asFields.addressFormat = CDROM_Address_Blocks;
	ior->io_Info.ioi_CmdOptions = options.asLongword; /** UGH! **/
      }
    }
    if (croak) {
      DBUG(("Bad CD-ROM-specific I/O parameters\n"));
      ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_BadIOArg); /* kluge */
      theDevice->cdrom_RequestRunning = (IOReq *) NULL;
      SuperCompleteIO(ior);
      return FALSE;
    }
    theDevice->cdrom_Level2 = CDROM_Level2_Prepare_Request;
    break;
  case CDROM_Level2_Prepare_Request:
    ior = theDevice->cdrom_RequestRunning;
    interrupts = Disable();
    switch (ior->io_Info.ioi_Command) {
    case CMD_STATUS:
    case CDROMCMD_DISCDATA:
    case CDROMCMD_SETDEFAULTS:
      theDevice->cdrom_Level2 = CDROM_Level2_Run_Request;
      Enable(interrupts);
      return TRUE;
    case CDROMCMD_PASSTHROUGH:
    case CDROMCMD_READ_SUBQ:
    case CDROMCMD_READ_DISC_CODE:
    case CDROMCMD_OPEN_DRAWER:
    case CDROMCMD_CLOSE_DRAWER:
      if (theDevice->cdrom_ReadaheadActive) {
	abortReadahead = TRUE;
	break;
      } else {
	theDevice->cdrom_Level2 = CDROM_Level2_Run_Request;
	Enable(interrupts);
	return TRUE;
      }
    case CMD_READ:
      userOffset = ior->io_Info.ioi_Offset;
      if (theDevice->cdrom_ReadaheadActive &&
	(userOffset != theDevice->cdrom_NextBlockOffset ||
	 ! beq(&theDevice->cdrom_CurrentParameters,
	       &theDevice->cdrom_DesiredParameters,
	       sizeof theDevice->cdrom_DesiredParameters))) {
	abortReadahead = TRUE;
      } else {
	abortReadahead = FALSE;
      }
      break;
    }
    if (abortReadahead) {
      SuperinternalAbortIO(theDevice->cdrom_CommandStatusReq);
      Enable(interrupts);
      if (theDevice->cdrom_ReadaheadActive) {
	DBUG(("The readahead-active bit didn't clear!\n"));
	theDevice->cdrom_ReadaheadActive = FALSE;
      }
      theDevice->cdrom_Level2 = CDROM_Level2_Abort_Readahead;
      return TRUE;
    }
    Enable(interrupts);
    if (beq(&theDevice->cdrom_DesiredParameters,
	    &theDevice->cdrom_CurrentParameters,
	    sizeof theDevice->cdrom_DesiredParameters)) {
      theDevice->cdrom_Level2 = CDROM_Level2_Run_Request;
      return TRUE;
    }
    if (theDevice->cdrom_Cooldown_MS != 0) {
      theDevice->cdrom_Holdoff_MS = theDevice->cdrom_Cooldown_MS;
      theDevice->cdrom_Cooldown_MS = 0;
      theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
      return TRUE;
    }
    if (theDevice->cdrom_DesiredParameters.block.densityCode !=
	theDevice->cdrom_CurrentParameters.block.densityCode ||
	theDevice->cdrom_DesiredParameters.block.lengthMSB !=
	theDevice->cdrom_CurrentParameters.block.lengthMSB ||
	theDevice->cdrom_DesiredParameters.block.lengthLSB !=
	theDevice->cdrom_CurrentParameters.block.lengthLSB ||
	theDevice->cdrom_DesiredParameters.block.flags !=
	theDevice->cdrom_CurrentParameters.block.flags) {
      theDevice->cdrom_CommandBuffer[1] = 0;
      theDevice->cdrom_CommandBuffer[2] =
	theDevice->cdrom_DesiredParameters.block.densityCode;
      theDevice->cdrom_CommandBuffer[3] =
	theDevice->cdrom_DesiredParameters.block.lengthMSB;
      theDevice->cdrom_CommandBuffer[4] =
	theDevice->cdrom_DesiredParameters.block.lengthLSB;
      theDevice->cdrom_CommandBuffer[5] =
	theDevice->cdrom_DesiredParameters.block.flags;
      DBUG(("Setting drive block parameters\n"));
      CDROMDriverSendCommand(theDevice, CDROM_MODE_SET, 0, FALSE);
      return FALSE;
    }
    if (theDevice->cdrom_DesiredParameters.errorRecovery.type !=
	theDevice->cdrom_CurrentParameters.errorRecovery.type ||
	theDevice->cdrom_DesiredParameters.errorRecovery.retryCount !=
	theDevice->cdrom_CurrentParameters.errorRecovery.retryCount) {
      theDevice->cdrom_CommandBuffer[1] = 1;
      theDevice->cdrom_CommandBuffer[2] =
	theDevice->cdrom_DesiredParameters.errorRecovery.type;
      theDevice->cdrom_CommandBuffer[3] =
	theDevice->cdrom_DesiredParameters.errorRecovery.retryCount;
      DBUG(("Setting drive error-recovery parameters\n"));
      CDROMDriverSendCommand(theDevice, CDROM_MODE_SET, 0, FALSE);
      return FALSE;
    }	
    if (theDevice->cdrom_DesiredParameters.stopTime.time !=
	theDevice->cdrom_CurrentParameters.stopTime.time) {
      theDevice->cdrom_CommandBuffer[1] = 2;
      theDevice->cdrom_CommandBuffer[2] =
	theDevice->cdrom_DesiredParameters.stopTime.time;
      DBUG(("Setting drive stop-time parameters\n"));
      CDROMDriverSendCommand(theDevice, CDROM_MODE_SET, 0, FALSE);
      return FALSE;
    }	
    if (theDevice->cdrom_DesiredParameters.driveSpeed.speed !=
	theDevice->cdrom_CurrentParameters.driveSpeed.speed ||
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchMSB !=
	theDevice->cdrom_CurrentParameters.driveSpeed.vpitchMSB ||
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchLSB !=
	theDevice->cdrom_CurrentParameters.driveSpeed.vpitchLSB) {
      theDevice->cdrom_CommandBuffer[1] = 3;
      theDevice->cdrom_CommandBuffer[2] =
	theDevice->cdrom_DesiredParameters.driveSpeed.speed;
      theDevice->cdrom_CommandBuffer[3] =
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchMSB;
      theDevice->cdrom_CommandBuffer[4] =
	theDevice->cdrom_DesiredParameters.driveSpeed.vpitchLSB;
      DBUG(("Setting drive speed: %x %x %x",
	     theDevice->cdrom_CommandBuffer[0],
	     theDevice->cdrom_CommandBuffer[1],
	     theDevice->cdrom_CommandBuffer[2]));
      DBUG((" %x %x\n",
	     theDevice->cdrom_CommandBuffer[3],
	     theDevice->cdrom_CommandBuffer[4]));
      CDROMDriverSendCommand(theDevice, CDROM_MODE_SET, 0, FALSE);
      return FALSE;
    }	
    if (theDevice->cdrom_DesiredParameters.chunkSize.size !=
	theDevice->cdrom_CurrentParameters.chunkSize.size) {
      if ((theDevice->cdrom_TweakFlags & CDROM_TWEAK_SUPPORTS_CHUNKS)) {
	theDevice->cdrom_CommandBuffer[1] = 4;
	theDevice->cdrom_CommandBuffer[2] =
	  theDevice->cdrom_DesiredParameters.chunkSize.size;
	DBUG(("Setting drive chunk-size parameters\n"));
	CDROMDriverSendCommand(theDevice, CDROM_MODE_SET, 0, FALSE);
	return FALSE;
      } else {
	theDevice->cdrom_DesiredParameters.chunkSize =
	  theDevice->cdrom_CurrentParameters.chunkSize;
      }
    }	
    theDevice->cdrom_Level2 = CDROM_Level2_Run_Request;
    break;
  case CDROM_Level2_Run_Request:
    ior = theDevice->cdrom_RequestRunning;
    switch (ior->io_Info.ioi_Command) {
    case CMD_STATUS:
      CDROMDriverSetStatus(ior);
      theDevice->cdrom_Level2 = CDROM_Level2_Finish_Request;
      return TRUE;
    case CDROMCMD_DISCDATA:
      CDROMDriverSetDiscData(ior);
      theDevice->cdrom_Level2 = CDROM_Level2_Finish_Request;
      return TRUE;
    case CDROMCMD_SETDEFAULTS:
      cdRomDefaultParameters = theDevice->cdrom_DesiredParameters;
      theDevice->cdrom_Level2 = CDROM_Level2_Finish_Request;
      return TRUE;
    case CMD_READ:
      theDevice->cdrom_Cooldown_MS = CDROM_COOLDOWN_MS;
      userOffset = ior->io_Info.ioi_Offset;
      driveBlockSize =
	(int32) theDevice->cdrom_CurrentParameters.block.userBlockSize;
      DBUG(("Read %d bytes from a %d-byte-block device\n", ior->io_Info.ioi_Recv.iob_Len, driveBlockSize));
      blockCount = ior->io_Info.ioi_Recv.iob_Len / driveBlockSize;
      ior->io_Actual = 0;
      wrongSize = FALSE;
      if (ior->io_Info.ioi_Recv.iob_Len == 0 ||
	  ior->io_Info.ioi_Recv.iob_Len != blockCount * driveBlockSize) {
	DBUG(("Bad request size, killing request!\n"));
	theDevice->cdrom_Level2 = CDROM_Level2_Finish_Request;
	ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
	return TRUE;
      }
      DBUG(("Read offset %d\n", userOffset));
      if (userOffset + blockCount >
	  theDevice->cdrom_MediumBlockCount) {
	DBUG(("Request past end-of-medium (%d > %d)\n", userOffset + blockCount, theDevice->cdrom_MediumBlockCount));
	theDevice->cdrom_Level2 = CDROM_Level2_Finish_Request;
	ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_BadIOArg);
	return TRUE;
      }
#ifdef DEBUG
      memset((char *) ior->io_Info.ioi_Recv.iob_Buffer, '?',
	     ior->io_Info.ioi_Recv.iob_Len);
#endif
      options.asLongword = ior->io_Info.ioi_CmdOptions;
      dataReq = theDevice->cdrom_DataReq;
      dataReq->io_Info.ioi_Command = CMD_READ;
      dataReq->io_Info.ioi_Send.iob_Buffer = NULL;
      dataReq->io_Info.ioi_Send.iob_Len = 0;
      dataReq->io_Info.ioi_Recv.iob_Buffer = ior->io_Info.ioi_Recv.iob_Buffer;
      dataReq->io_Info.ioi_Recv.iob_Len = driveBlockSize;
      dataReq->io_CallBack = CDROMDriverEndAction;
      theDevice->cdrom_ReadRemaining = ior->io_Info.ioi_Recv.iob_Len -
	driveBlockSize;
      theDevice->cdrom_NextBlockOffset = userOffset;
      theDevice->cdrom_BlocksPerDataXfer = 1; /******* fix this *******/
#ifdef READAHEAD
      switch (options.asFields.readAhead) {
      case CDROM_READAHEAD_ENABLED:
	theDevice->cdrom_DoReadahead = TRUE;
	break;
      case CDROM_READAHEAD_DISABLED:
	theDevice->cdrom_DoReadahead = FALSE;
	break;
      case CDROM_Option_Unspecified:
	theDevice->cdrom_DoReadahead = (uint8) doReadahead;
	break;
      }
      if (theDevice->cdrom_DoReadahead) {
	blockCount = theDevice->cdrom_MediumBlockCount - userOffset;
	DBUG(("Readahead enabled, block count set to %d\n", blockCount));
      }
#endif
      blockOffset = OffsetToMSF(userOffset);
      theDevice->cdrom_CommandBuffer[1] = (uint8) (blockOffset >> 16);
      theDevice->cdrom_CommandBuffer[2] = (uint8) (blockOffset >>  8);
      theDevice->cdrom_CommandBuffer[3] = (uint8) (blockOffset      );
      theDevice->cdrom_CommandBuffer[4] = (uint8) (blockCount  >> 16);
      theDevice->cdrom_CommandBuffer[5] = (uint8) (blockCount  >>  8);
      theDevice->cdrom_CommandBuffer[6] = (uint8) (blockCount       );
    reissue:
      DBUG(("Sending CMD_READ to expansion bus, followon %d\n", theDevice->cdrom_ReadRemaining));
      interrupts = Disable();
      readingAhead = theDevice->cdrom_ReadaheadActive;
      switch (theDevice->cdrom_Level0) {
      case CDROM_Level0_Idle:
	theDevice->cdrom_Level0 = CDROM_Level0_Data;
	break;
      case CDROM_Level0_Readahead:
	theDevice->cdrom_Level0 = CDROM_Level0_CommandData;
	break;
      }	

      err = SuperinternalSendIO(dataReq);
      Enable(interrupts);
      if (err || (err = dataReq->io_Error) != 0) {
	theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
	ior->io_Error = err;
	theDevice->cdrom_Level2 = CDROM_Level2_Finish_Request;
	DBUG(("CMD_READ failed, error 0x%x\n", err));
	return TRUE;
      }
      if (theDevice->cdrom_Level0 == CDROM_Level0_EndAction && !readingAhead) {
	DBUG(("CMD_DATA completed immediately, reissuing it!\n"));
/*
  Undo the changes made during data endaction.
  We really need a better way of handling the drive's nasty habit of
  leaving the data-ready signal high after a FLUSH.
*/
	ior->io_Actual = 0;
	dataReq->io_Info.ioi_Recv.iob_Buffer = ior->io_Info.ioi_Recv.iob_Buffer;
	theDevice->cdrom_NextBlockOffset -= theDevice->cdrom_BlocksPerDataXfer;
	goto reissue;
      }
      if (theDevice->cdrom_ReadaheadActive) {
	DBUG(("Readahead active and suitable, no new command\n"));
      } else {
	CDROMDriverSendCommand(theDevice, CDROM_READ_DATA, 0, FALSE);
      }
      break;
    case CDROMCMD_READ_SUBQ:
      CDROMDriverSendCommand(theDevice, CDROM_READ_SUBQ, 10, FALSE);
      break;
    case CDROMCMD_READ_DISC_CODE:
      CDROMDriverSendCommand(theDevice, CDROM_READ_DISC_CODE, 10, FALSE);
      break;
    case CDROMCMD_OPEN_DRAWER:
      CDROMDriverSendCommand(theDevice, CDROM_DRAWER_OPEN, 0, FALSE);
      break;
    case CDROMCMD_CLOSE_DRAWER:
      CDROMDriverSendCommand(theDevice, CDROM_DRAWER_CLOSE, 0, FALSE);
      break;
    }
    return FALSE;
  case CDROM_Level2_Finish_Request:
    ior = theDevice->cdrom_RequestRunning;
    if (theDevice->cdrom_TimedOut &&
	ior->io_Info.ioi_Command != CDROMCMD_CLOSE_DRAWER) {
      DBUG(("Timeout recovery\n"));
      memset(&theDevice->cdrom_CurrentParameters, 0xFF,
	     sizeof (CDROM_Parameters));
      if (ior->io_Flags & CDROM_TIMED_OUT_ONCE) {
 #ifdef TRYRESET
 	if (!(ior->io_Flags & CDROM_TIMED_OUT_TWICE)) {
 	  ior->io_Flags |= CDROM_TIMED_OUT_TWICE;
	  DBUG0(("Try resetting the drive\n"));
	  theDevice->cdrom_Level2 = CDROM_Level2_Wake_Up;
	  theDevice->cdrom_Holdoff_MS = 1500;
	  theDevice->cdrom_MustReadError = TRUE;
	  CDROMDriverSendCommand(theDevice, CDROM_RESET, 0, FALSE);
 	  return TRUE;
 	}
 #endif
	DBUG(("Timeout recovery failed\n"));
 	ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceError);
      } else {
	DBUG(("Try abort, flush, reset parameters\n"));
	ior->io_Flags |= CDROM_TIMED_OUT_ONCE;
	theDevice->cdrom_Level2 = CDROM_Level2_Abort_Readahead;
	return TRUE;
      }
    }
    switch (ior->io_Info.ioi_Command) {
    case CDROMCMD_READ_SUBQ:
    case CDROMCMD_READ_DISC_CODE:
      xfer = (uint32) theDevice->cdrom_StatusBytesRead;
      if (ior->io_Info.ioi_Recv.iob_Len > xfer) {
	xfer = ior->io_Info.ioi_Recv.iob_Len;
      }
      if (xfer > 0) {
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer,
	       theDevice->cdrom_StatusBuffer,
	       xfer);
	ior->io_Actual = xfer;
      }
      break;
    case CDROMCMD_OPEN_DRAWER:
    case CDROMCMD_CLOSE_DRAWER:
      theDevice->cdrom_DiscInfoAvailable = FALSE;
      memset(&theDevice->cdrom_CurrentParameters, 0xFF,
	     sizeof (CDROM_Parameters));
      break;
    default:
      break;
    }
    if (theDevice->cdrom_DoorOpened != notOpened) {
      theDevice->cdrom_DiscInfoAvailable = FALSE;
    }
    if (wrongSize) {
      DBUG(("CD-ROM endaction routine got wrong data-transfer size %d!\n", badSize));
    }
    DBUG(("Clearing down the I/O request, status 0x%x\n", theDevice->cdrom_StatusByte));
    if (ior->io_Error != 0) {
#ifdef DEBUG
      qprintf(("CD-ROM error 0x%x finishing request!\n", ior->io_Error));
      MySysErr(ior->io_Error);
#endif
    }
    theDevice->cdrom_RequestRunning = (IOReq *) NULL;
    SuperCompleteIO(ior);
    if (theDevice->cdrom_DiscInfoAvailable) {
      theDevice->cdrom_Level2 = CDROM_Level2_Await_Request;
      return TRUE;
    } else {
      theDevice->cdrom_Level2 = CDROM_Level2_Idle;
      return FALSE;
    }
    break;
  case CDROM_Level2_Abort_Readahead:
    CDROMDriverSendCommand(theDevice, CDROM_ABORT, 31, FALSE);
    return FALSE;
  case CDROM_Level2_Flush_FIFO:
    CDROMDriverSendCommand(theDevice, CDROM_FLUSH, 31, FALSE);
  return FALSE;
  case CDROM_Level2_Spin_Down:
    break;
  case CDROM_Level2_Open_Drawer:
    break;
  case CDROM_Level2_Kill_Pending_IOs:
    DBUG(("CD-ROM drive not on-line or not functioning, killing I/Os\n"));
    ior = theDevice->cdrom_RequestRunning;
    if (ior) {
      if (ior->io_Error == 0) {
	ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
      }
      SuperCompleteIO(ior);
    }
    while (!ISEMPTYLIST(&theDevice->cdrom_RequestsToDo)) {
      ior = (IOReq *) FIRSTNODE(&theDevice->cdrom_RequestsToDo);
      RemNode((Node *) ior);
      DBUG(("Killing queued I/O request at 0x%x\n", ior));
      ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
      SuperCompleteIO(ior);
    }
    memset(&theDevice->cdrom_CurrentParameters, 0xFF,
	   sizeof (CDROM_Parameters));
    theDevice->cdrom_Level2 = CDROM_Level2_Idle;
    theDevice->cdrom_RetrysPermitted = 5;
    return TRUE;
  case CDROM_Level2_Sluggish:
    theDevice->cdrom_TimedOut = FALSE;
    if (theDevice->cdrom_RetrysPermitted == 0) {
      qprintf(("I'll never get out of here.  I'll die in Casablanca.\n"));
      theDevice->cdrom_Level2 = CDROM_Level2_Kill_Pending_IOs;
    } else {
      DBUG(("Waiting for drive to come alive\n"));
      theDevice->cdrom_RetrysPermitted --;
      theDevice->cdrom_Level2 = CDROM_Level2_Idle;
      theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
      theDevice->cdrom_Holdoff_MS = 1000;
    }
    return TRUE;
  case CDROM_Level2_Poke_Drive:
    DBUG(("Probe\n"));
    CDROMDriverSendCommand(theDevice, CDROM_READ_IDENTIFICATION, 10, FALSE);
    break;
  }
  return err;
}

static void CDROMErrorMapper(CDROM *theDevice, IOReq *ior)
{
  switch (theDevice->cdrom_ErrorCodeRead) {
  case MEI_CDROM_toc_error:
  case MEI_CDROM_unrecv_error:
  case MEI_CDROM_seek_error:
  case MEI_CDROM_track_error:
  case MEI_CDROM_data_error:
    ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_MediaError);
    DBUG0(("I will not buy this phonograph record, it is scratched.  Code 0x%X\n", theDevice->cdrom_ErrorCodeRead));
    break;
  case MEI_CDROM_not_ready:
  case MEI_CDROM_media_changed:
  case MEI_CDROM_hard_reset:
  case MEI_CDROM_disc_out:
  case MEI_CDROM_cmd_error:  /* oddly enough, this is what happens! */
    ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
    theDevice->cdrom_DiscInfoAvailable = FALSE;
    if (theDevice->cdrom_TweakFlags & CDROM_TWEAK_EMULATE_MEDIA_ACCESS) {
      theDevice->cdrom_DoorOpened = openedMustReset;
    }
    DBUG0(("Code 0x%x, reporting device-offline error\n", theDevice->cdrom_ErrorCodeRead));
    break;
  case MEI_CDROM_ram_error:
  case MEI_CDROM_diag_error:
  case MEI_CDROM_focus_error:
  case MEI_CDROM_clv_error:
  case MEI_CDROM_rom_error:
  case MEI_CDROM_hardware_error:
    ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceError);
    DBUG0(("Code 0x%x, reporting device hardware error\n", theDevice->cdrom_ErrorCodeRead));
    break;
  case MEI_CDROM_address_error:
  case MEI_CDROM_cdb_error:
  case MEI_CDROM_end_address:
  case MEI_CDROM_mode_error:
  case MEI_CDROM_illegal_request:
    ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_ParamError);
    DBUG0(("Code 0x%x, reporting illegal-parameter error\n", theDevice->cdrom_ErrorCodeRead));
    break;
  case MEI_CDROM_recv_retry:
    DBUG(("Retries required\n"));
    break;
  case MEI_CDROM_recv_ecc:
    DBUG(("ECC required\n"));
    break;
  default:
    ior->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_DeviceError);
    DBUG0(("Unknown error code 0x%x\n", theDevice->cdrom_ErrorCodeRead));
    break;
  }
}

int32 CDROMDriverLevel2EndAction(CDROM *theDevice)
{
  int32 i;
  int32 actual;
  IOReq *ior;
  theDevice->cdrom_TickleDelay = CDROM_TICKLE_DELAY;
  ior = theDevice->cdrom_RequestRunning;
  DBUG2(("Level 2 endaction, state %d\n", theDevice->cdrom_Level2));
  actual = theDevice->cdrom_CommandStatusReq->io_Actual;
  DBUG2(("Status FIFO actually delivered %d bytes\n", actual));
  switch (theDevice->cdrom_Level2) {
  case CDROM_Level2_Idle:
    break; /* shouldn't get here, really */
  case CDROM_Level2_Wake_Up:
    break;
  case CDROM_Level2_Check_Data_Path:
    DBUG(("Data path check got %d bytes\n", actual));
    if (theDevice->cdrom_TimedOut) {
      DBUG(("Data path check timed out\n"));
      theDevice->cdrom_Level2 = CDROM_Level2_Sluggish;
    } 
#ifdef STATUSONDPC
    else if (!(theDevice->cdrom_StatusByte & CDROM_STATUS_DISC_IN)) {
      DBUG0(("No disc, status byte 0x%x\n", theDevice->cdrom_StatusByte));
      theDevice->cdrom_Level2 = CDROM_Level2_Kill_Pending_IOs;
    } 
#endif
    else if (actual == 4 &&
	       theDevice->cdrom_StatusBuffer[1] == 0xAA &&
	       theDevice->cdrom_StatusBuffer[2] == 0x55 &&
	       (theDevice->cdrom_StatusByte & CDROM_STATUS_ERROR) == 0 &&
	       !theDevice->cdrom_TimedOut) {
      DBUG(("Data path checks out OK, status byte 0x%x\n", theDevice->cdrom_StatusByte));
      theDevice->cdrom_Level2 = CDROM_Level2_Spin_Up;
    } else {
      /* hardware error, go idle?? or mark device bad?? */
      qprintf(("Data path check failed:"));
      for (i = 0; i < actual; i++) {
	qprintf((" %02x", theDevice->cdrom_StatusBuffer[i]));
      }
      qprintf(("\n"));
      theDevice->cdrom_Level2 = CDROM_Level2_Sluggish;
    }
    break;
  case CDROM_Level2_Spin_Up:
    if (theDevice->cdrom_TimedOut ||
	(theDevice->cdrom_StatusByte & CDROM_STATUS_ERROR)) {
      DBUG(("Drive has not spun up\n"));
      DBUG(("Status byte contains 0x%x\n", theDevice->cdrom_StatusByte));
      if (theDevice->cdrom_TimedOut ||
	  (theDevice->cdrom_StatusByte & CDROM_STATUS_DISC_IN)) {
	theDevice->cdrom_Level2 = CDROM_Level2_Sluggish;
      } else {
	DBUG(("Marking drive off-line due to spin-up no-disc\n"));
	theDevice->cdrom_Level2 = CDROM_Level2_Kill_Pending_IOs;
      }
    } else {
      theDevice->cdrom_Level2 = CDROM_Level2_Read_Disc_ID;
    }
    break;
  case CDROM_Level2_Read_Disc_ID:
    DBUG(("Read disc info command got %d bytes\n", actual));
    if (actual != 8 || theDevice->cdrom_StatusByte & CDROM_STATUS_ERROR) {
      qprintf(("Could not read disc information, status byte contains 0x%x\n", theDevice->cdrom_StatusByte));
      if (theDevice->cdrom_StatusByte & CDROM_STATUS_DISC_IN) {
	theDevice->cdrom_Level2 = CDROM_Level2_Sluggish;
      } else {
	DBUG(("Marking drive off-line due to read-ID no-disc\n"));
	theDevice->cdrom_Level2 = CDROM_Level2_Kill_Pending_IOs;
      }
#ifdef TRACE
      qprintf(("Disc info: "));
      for (i = 0; i < actual; i++) {
	qprintf((" %02x", theDevice->cdrom_StatusBuffer[i]));
      }
      qprintf(("\n"));
#endif
      break;
    }
    theDevice->cdrom_DiscInformation.discID =
      theDevice->cdrom_StatusBuffer[actual-7];
    theDevice->cdrom_DiscInformation.firstTrackNumber =
      theDevice->cdrom_StatusBuffer[actual-6];
    theDevice->cdrom_DiscInformation.lastTrackNumber =
      theDevice->cdrom_StatusBuffer[actual-5];
/*** the following can probably be deleted once we're sure that
     ReadCapacity works ***/
    theDevice->cdrom_DiscInformation.minutes =
      theDevice->cdrom_StatusBuffer[actual-4];
    theDevice->cdrom_DiscInformation.seconds =
      theDevice->cdrom_StatusBuffer[actual-3];
    theDevice->cdrom_DiscInformation.frames =
      theDevice->cdrom_StatusBuffer[actual-2];
    theDevice->cdrom_TrackToProbe =
      theDevice->cdrom_DiscInformation.firstTrackNumber;
    theDevice->cdrom_MediumBlockCount =
      ((theDevice->cdrom_DiscInformation.minutes * 60L) +
       theDevice->cdrom_DiscInformation.seconds) * 75L +
	 theDevice->cdrom_DiscInformation.frames;
/*** endof ***/
    theDevice->cdrom_Level2 = CDROM_Level2_Read_TOC_Entry;
#ifdef DEBUG
    DBUG(("Got disc information:"));
    for (i = 0; i < 8; i++) {
      DBUG((" %02x", theDevice->cdrom_StatusBuffer[i]));
    }
    DBUG(("\n"));
    DBUG(("First track %d, last track %d\n", theDevice->cdrom_DiscInformation.firstTrackNumber, theDevice->cdrom_DiscInformation.lastTrackNumber));
#endif
    break;
  case CDROM_Level2_Read_TOC_Entry:
    if (theDevice->cdrom_StatusByte & CDROM_STATUS_ERROR) {
      qprintf(("Could not read table-of-contents information\n"));
      qprintf(("Status byte contains 0x%x\n", theDevice->cdrom_StatusByte));
      if (theDevice->cdrom_StatusByte & CDROM_STATUS_DISC_IN) {
	theDevice->cdrom_Level2 = CDROM_Level2_Sluggish;
      } else {
	theDevice->cdrom_Level2 = CDROM_Level2_Kill_Pending_IOs;
      }
      break;
    }
    i = theDevice->cdrom_StatusBuffer[actual-7];
    if (i > 0 && i <= 99) {
      theDevice->cdrom_TOC_Entry[i].reserved0 =
	theDevice->cdrom_StatusBuffer[actual-9];
      theDevice->cdrom_TOC_Entry[i].addressAndControl =
	theDevice->cdrom_StatusBuffer[actual-8];
      theDevice->cdrom_TOC_Entry[i].trackNumber =
	theDevice->cdrom_StatusBuffer[actual-7];
      theDevice->cdrom_TOC_Entry[i].reserved3 =
	theDevice->cdrom_StatusBuffer[actual-6];
      theDevice->cdrom_TOC_Entry[i].minutes =
	theDevice->cdrom_StatusBuffer[actual-5];
      theDevice->cdrom_TOC_Entry[i].seconds =
	theDevice->cdrom_StatusBuffer[actual-4];
      theDevice->cdrom_TOC_Entry[i].frames =
	theDevice->cdrom_StatusBuffer[actual-3];
      theDevice->cdrom_TOC_Entry[i].reserved7 =
	theDevice->cdrom_StatusBuffer[actual-2];
#ifdef DEBUG
      DBUG(("Got TOC entry:"));
      for (i = 0; i < 9; i++) {
	DBUG((" %02x", theDevice->cdrom_StatusBuffer[i]));
      }
      DBUG(("\n"));
#endif
    }
    theDevice->cdrom_TrackToProbe ++;
    break;
  case CDROM_Level2_Read_Session:
    if (theDevice->cdrom_StatusByte & CDROM_STATUS_ERROR) {
      DBUG(("No session information on disc\n"));
      theDevice->cdrom_SessionInformation.valid = 0;
    } else {
      DBUG(("Got session information\n"));
      theDevice->cdrom_SessionInformation.valid =
	theDevice->cdrom_StatusBuffer[actual-7];
      theDevice->cdrom_SessionInformation.minutes =
	theDevice->cdrom_StatusBuffer[actual-6];
      theDevice->cdrom_SessionInformation.seconds =
	theDevice->cdrom_StatusBuffer[actual-5];
      theDevice->cdrom_SessionInformation.frames =
	theDevice->cdrom_StatusBuffer[actual-4];
    }
    theDevice->cdrom_Level2 = CDROM_Level2_Read_Capacity;
    break;
  case CDROM_Level2_Read_Capacity:
    i = ((theDevice->cdrom_StatusBuffer[actual-6] * 60L) +
	 theDevice->cdrom_StatusBuffer[actual-5]) * 75L +
	   theDevice->cdrom_StatusBuffer[actual-4];
    if (i < theDevice->cdrom_MediumBlockCount) {
      theDevice->cdrom_MediumBlockCount = i;
      theDevice->cdrom_DiscInformation.minutes =
	theDevice->cdrom_StatusBuffer[actual-6];
      theDevice->cdrom_DiscInformation.seconds =
	theDevice->cdrom_StatusBuffer[actual-5];
      theDevice->cdrom_DiscInformation.frames =
	theDevice->cdrom_StatusBuffer[actual-4];
      DBUG(("MSF is %d/%d/%d\n", theDevice->cdrom_DiscInformation.minutes,theDevice->cdrom_DiscInformation.seconds,theDevice->cdrom_DiscInformation.frames));
    }
    theDevice->cdrom_Level2 = CDROM_Level2_Await_Request;
    theDevice->cdrom_DiscInfoAvailable = TRUE; /* OK, we're alive now */
  case CDROM_Level2_Await_Request:
    break;
  case CDROM_Level2_Prepare_Request:
    if (theDevice->cdrom_StatusByte & CDROM_STATUS_ERROR) {
      theDevice->cdrom_Level2 = CDROM_Level2_Finish_Request;
      DBUG0(("MODE SET failure!\n"));
      CDROMErrorMapper(theDevice, ior);
    } else {
      switch (theDevice->cdrom_CommandBuffer[1]) {
      case 0:
	DBUG(("Block parameters have been set\n"));
	theDevice->cdrom_CurrentParameters.block =
	  theDevice->cdrom_DesiredParameters.block;
	break;
      case 1:
	DBUG(("Error-recovery parameters have been set\n"));
	theDevice->cdrom_CurrentParameters.errorRecovery =
	  theDevice->cdrom_DesiredParameters.errorRecovery;
	break;
      case 2:
	DBUG(("Stop-time parameters have been set\n"));
	theDevice->cdrom_CurrentParameters.stopTime =
	  theDevice->cdrom_DesiredParameters.stopTime;
	break;
      case 3:
	DBUG(("Speed parameters have been set\n"));
	theDevice->cdrom_CurrentParameters.driveSpeed =
	  theDevice->cdrom_DesiredParameters.driveSpeed;
	break;
      case 4:
	DBUG(("Chunk parameters have been set\n"));
	theDevice->cdrom_CurrentParameters.chunkSize =
	  theDevice->cdrom_DesiredParameters.chunkSize;
	break;
      }
#ifdef POSTCOOLDOWN
      theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
      theDevice->cdrom_Holdoff_MS = CDROM_COOLDOWN_MS;
#endif
    }
    break;
  case CDROM_Level2_Run_Request:
    DBUG(("Endaction after real disc I/O!\n"));
    if (theDevice->cdrom_StatusByte & CDROM_STATUS_ERROR) {
      DBUG(("CD-ROM error\n"));
      CDROMErrorMapper(theDevice, ior);
    }
    theDevice->cdrom_Level2 = CDROM_Level2_Finish_Request;
    break;
  case CDROM_Level2_Abort_Readahead:
    theDevice->cdrom_Level2 = CDROM_Level2_Flush_FIFO;
    break;
  case CDROM_Level2_Flush_FIFO:
    theDevice->cdrom_Level2 = CDROM_Level2_Prepare_Request;
    break;
  case CDROM_Level2_Spin_Down:
    break;
  case CDROM_Level2_Open_Drawer:
    break;
  case CDROM_Level2_Kill_Pending_IOs:
    break;
  case CDROM_Level2_Poke_Drive:
    if (theDevice->cdrom_DiscInfoAvailable) {
      if (!(theDevice->cdrom_StatusByte & CDROM_STATUS_DOOR)) {
	theDevice->cdrom_DiscInfoAvailable = FALSE;
	memset(&theDevice->cdrom_CurrentParameters, 0xFF,
	       sizeof (CDROM_Parameters));
	DBUG0(("Drawer has been opened!\n"));
	theDevice->cdrom_Level2 = CDROM_Level2_Idle;
      } else {
	theDevice->cdrom_Level2 = CDROM_Level2_Await_Request;
      }
    } else {
      theDevice->cdrom_Level2 = CDROM_Level2_Idle;
    }
    break;
  }
  return TRUE;
}

int32 CDROMDriverLevel1Idle(CDROM *theDevice)
{
  int32 err;
  err = 0;
  DBUG2(("Level 1 idle, state %d\n", theDevice->cdrom_Level1));
  switch (theDevice->cdrom_Level1) {
  case CDROM_Level1_Initialize:
    theDevice->cdrom_Level1 = CDROM_Level1_Idle;
    return TRUE;
    break;
  case CDROM_Level1_Idle:
    if (theDevice->cdrom_MustReadError) {
      theDevice->cdrom_MustReadError = FALSE;
      theDevice->cdrom_DoingReadError = TRUE;
      /* UGLY BUT FAST */
      *     (uint32 *) theDevice->cdrom_CommandBuffer  = 0;
      * (1+ (uint32 *) theDevice->cdrom_CommandBuffer) = 0;
      /* UGLY BUT FAST */
      DBUG(("Issuing a READ ERROR\n"));
      err = CDROMDriverSendCommand(theDevice, CDROM_READ_ERROR, 8, TRUE);
    } else {
      err = CDROMDriverLevel2Idle(theDevice);
    }
    break;
  default:
    DBUG(("AAUGH!  Implausible state %d at level 1!\n", theDevice->cdrom_Level1));
    }
  return err;
}

int32 CDROMDriverLevel1EndAction(CDROM *theDevice) 
{
  uint8 statusByte, tag;
  IOReq *ior;
  int32 actual;
#ifdef TRACE
  int32 i;
#endif
  ior = theDevice->cdrom_CommandStatusReq;
  actual = ior->io_Actual;
  if ((actual == 0 && ior->io_Info.ioi_Recv.iob_Len > 0) ||
      actual >= sizeof theDevice->cdrom_StatusBuffer) {
    DBUG(("Strange value in CD-ROM command io_actual: %d\n", actual));
    statusByte = CDROM_STATUS_ERROR; /* Ad hoc klugery, clean it up! */
  } else if (theDevice->cdrom_DoingReadError) {
    statusByte = theDevice->cdrom_ErrorBuffer[actual-1];
  } else {
    statusByte = theDevice->cdrom_StatusBuffer[actual-1];
  }
  tag = theDevice->cdrom_StatusBuffer[0];
  DBUG2(("Level 1 endaction, state %d, status 0x%x\n", theDevice->cdrom_Level1, statusByte));
  if ((theDevice->cdrom_TweakFlags & CDROM_TWEAK_EMULATE_MEDIA_ACCESS) &&
      tag != 0 && !theDevice->cdrom_TimedOut &&
      !(statusByte & CDROM_STATUS_DOOR)) {
    if (theDevice->cdrom_DoorOpened == notOpened) {
      theDevice->cdrom_DoorOpened = openedMustReset;
      DBUG(("Status 0x%X, actual %d, tag 0x%X\n  Door opened, remember to do a soft reset later\n", statusByte, actual, tag));
    }
  }
  switch (theDevice->cdrom_Level1) {
  case CDROM_Level1_Initialize:
    theDevice->cdrom_StatusByte = statusByte;
  case CDROM_Level1_Idle:
    theDevice->cdrom_Level1 = CDROM_Level1_Idle;
    return CDROMDriverLevel2EndAction(theDevice);
  case CDROM_Level1_Running:
    theDevice->cdrom_Level1 = CDROM_Level1_Idle;
    if (theDevice->cdrom_DoingReadError) {
      theDevice->cdrom_DoingReadError = FALSE;
      theDevice->cdrom_ErrorCodeRead = theDevice->cdrom_ErrorBuffer[actual-7];
#ifdef TRACE
      DBUG0(("Error status:"));
      for (i = 0; i < actual; i++) {
	DBUG0((" %02x", theDevice->cdrom_ErrorBuffer[i]));
      }
      DBUG0(("\n"));
#endif
      return CDROMDriverLevel2EndAction(theDevice);
    } else {
      DBUG2(("Level-1 endaction status byte 0x%x\n", statusByte));
      theDevice->cdrom_StatusByte = statusByte;
      theDevice->cdrom_StatusBytesRead = (uint8) actual;
      if ((statusByte & CDROM_STATUS_ERROR) && !theDevice->cdrom_TimedOut) {
	theDevice->cdrom_MustReadError = TRUE;
	DBUG(("Status byte 0x%X, READ ERROR is required\n", statusByte));
	return TRUE;
      }
      return CDROMDriverLevel2EndAction(theDevice);
    }
  default:
    DBUG(("AAUGH!  Implausible state %d at level 1 endaction!\n", theDevice->cdrom_Level1));
  }
  return FALSE;
}


uint32 CDROMDriverTimeSlice(CDROM *theDevice)
{
  int32 interrupts;
  DBUG3(("Level 0 timeslice, state %d\n", theDevice->cdrom_Level0));
#ifdef TURBO
 turboRepeat: ;
#endif
  interrupts = Disable();
  LOG(theDevice,TIMESLICE);
  switch (theDevice->cdrom_Level0) {
  case CDROM_Level0_Holdoff:
    Enable(interrupts);
    if (theDevice->cdrom_Holdoff_MS-- <= 0) {
      theDevice->cdrom_Level0 = CDROM_Level0_Idle;
      DBUG(("Holdoff interval finished\n"));
    }
    return TRUE;
  case CDROM_Level0_Idle:
  case CDROM_Level0_Readahead:
    Enable(interrupts);
#ifdef TURBO
    if (CDROMDriverLevel1Idle(theDevice)) {
      goto turboRepeat;
    }
    break;
#else
    return CDROMDriverLevel1Idle(theDevice);
#endif
  case CDROM_Level0_EndAction:
#ifdef TURBO
    theDevice->cdrom_Level0 = CDROM_Level0_Idle;
#else
    theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
    theDevice->cdrom_Holdoff_MS = 1;
#endif
    Enable(interrupts);
    DBUG(("Data ea count = %d\n", eaCount));
    if (caughtError) {
      DBUG(("Driver endaction routine detected an error in status byte\n"));
      caughtError = FALSE;
    }
    if (theDevice->cdrom_MustReadError) {
      DBUG(("Timeslice;  must-read-error is set\n"));
    }
    (void) CDROMDriverLevel1EndAction(theDevice);
#ifdef TUBO
    goto turboRepeat;
#else
    return TRUE;
#endif
  case CDROM_Level0_EndActionReadahead:
    theDevice->cdrom_Level0 = CDROM_Level0_Readahead;
/* 
   Fake up status since it isn't really there.  This isn't a good way
   to do it, as the command might actually terminate part way through
   level-1 endaction and graunch this byte... better to smarten up the
   level-1 code to know that the status byte isn't valid during a
   readahead.
*/
    theDevice->cdrom_CommandStatusReq->io_Actual = 1;
    theDevice->cdrom_StatusBuffer[0] = CDROM_STATUS_DOOR |
      CDROM_STATUS_DISC_IN | CDROM_STATUS_SPIN_UP | CDROM_STATUS_READY;
    Enable(interrupts);
    (void) CDROMDriverLevel1EndAction(theDevice);
#ifdef TUBO
    goto turboRepeat;
#else
    return TRUE;
#endif
  case CDROM_Level0_Command:
  case CDROM_Level0_Data:
  case CDROM_Level0_CommandData:
    Enable(interrupts);
    DBUG2(("I/O still in progress, nothing to do here\n"));
    break;
  case CDROM_Level0_KillData:
    Enable(interrupts);
    DBUG(("Status byte from command is 0x%x\n", theDevice->cdrom_StatusByte));
    DBUG(("CommandStatus ioActual = %d\n", theDevice->cdrom_CommandStatusReq->io_Actual));
    DBUG(("Killing data request due to error status\n"));
    theDevice->cdrom_DataReq->io_Error =
      MakeCDErr(ER_SEVER,ER_C_STND,ER_Aborted);
    theDevice->cdrom_ReadRemaining = 0;
    SuperinternalAbortIO(theDevice->cdrom_DataReq);
    DBUG(("Data request has been killed due to error status\n"));
    DBUG(("Callback status byte was 0x%x\n", theDevice->cdrom_StatusByte));
  default:
    Enable(interrupts);
    DBUG(("AAUGH!  Implausible state %d at level 0!\n", theDevice->cdrom_Level0));
    return FALSE;
    break;
  }
  return FALSE;
}

int32 CDROMDriverDeviceInit(CDROM *theDevice)
{
  TagArg ioReqTags[2];
  int32 i;
  Item ioReqItem;
  IOReq *rawIOReq;
  theDevice->cdrom_Holdoff_MS = 60;
  theDevice->cdrom_Cooldown_MS = 0;
  theDevice->cdrom_RetrysPermitted = 10; /* Man, it's slow off the mark! */
  theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
  theDevice->cdrom_Level1 = CDROM_Level1_Initialize;
  theDevice->cdrom_Level2 = CDROM_Level2_Idle;
  theDevice->cdrom_MustReadError = TRUE;
  theDevice->cdrom_DoingReadError = TRUE;
  theDevice->cdrom_DiscAvailable = CDROM_Disc_Availability_Unknown;
  theDevice->cdrom_DoorOpened = notOpened;
  memset(&theDevice->cdrom_CurrentParameters, 0xFF, sizeof (CDROM_Parameters));
  InitList(&theDevice->cdrom_RequestsToDo, "Requests");
#ifdef DEBUG
  DBUG(("Creating IOReqs\n"));
#endif
  i = 2;
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) expansionItem;
  ioReqTags[1].ta_Tag = TAG_END;
  do {
    ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
    if (ioReqItem < 0) {
      qprintf(("Can't allocate an expansion-bus IOReq for CD-ROM device\n"));
      return ioReqItem;
    }
    rawIOReq = (IOReq *) LookupItem(ioReqItem);
    rawIOReq->io_Info.ioi_User = (uint32) theDevice;
    rawIOReq->io_Info.ioi_Unit = theDevice->cdrom_XBusUnit;
    switch (i) {
    case 2:
      theDevice->cdrom_CommandStatusReq = rawIOReq;
      rawIOReq->io_Info.ioi_Command = XBUSCMD_Command;
      break;
    case 1:
      theDevice->cdrom_DataReq = rawIOReq;
      rawIOReq->io_Info.ioi_Command = CMD_READ;
      break;
    }
  } while (--i != 0);
  theDevice->cdrom_Initialized = TRUE;
  theDevice->cdrom_DiscInfoAvailable = FALSE;
  return 0;
}

int32 CDROMDriverDoit(IOReq *theRequest)
{
  CDROM *theDevice;
  int32 i;
  uint32 interrupts;
  int32 kickit;
  theRequest->io_Flags &=
    ~(IO_QUICK | CDROM_TIMED_OUT_ONCE | CDROM_TIMED_OUT_TWICE);
  theDevice = (CDROM *) theRequest->io_Dev;
  LOG(theDevice,DISPATCHER);
  if (!theDevice->cdrom_Initialized) {
    if ((i = CDROMDriverDeviceInit(theDevice)) < 0) {
      theRequest->io_Error = i;
      SuperCompleteIO(theRequest);
      return 0;
    }
  }
  DBUG(("CD-ROM command dispatch\n"));
  interrupts = Disable();
  InsertNodeFromTail(&theDevice->cdrom_RequestsToDo, (Node *) theRequest);
#ifdef TURBO
  kickit = TRUE;
/*
  Call the timeslicer directly if there's no I/O actually in progress
  and if we are not in the middle of a callback.
*/
  if ((theRequest->io_Flags & IO_INTERNAL) == 0 &&
      theDevice->cdrom_Level0 == CDROM_Level0_Idle) {
    Enable(interrupts);
    DBUG2(("Fast timeslice\n"));
    kickit = CDROMDriverTimeSlice(theDevice);
    DBUG2(("Fast timeslice kick is %d\n", kickit));
  } else {
    Enable(interrupts);
  }
  if (kickit && daemon != (Task *) NULL) {
    DBUG2(("Signal daemon\n"));
    SuperinternalSignal(daemon, daemonSignal);
  }
#else
  Enable(interrupts);
  if (daemon != (Task *) NULL) {
    DBUG2(("Signal daemon\n"));
    SuperinternalSignal(daemon, daemonSignal);
  }
#endif
  return 0;
}

void CDROMDriverSetStatus(IOReq *ior)
{
  DeviceStatus status;
  CDROM *theDevice;
  int32 actual;
#ifdef DEBUG
  qprintf(("CD-ROM status call via request at %d\n", ior));
#endif
  theDevice = (CDROM *) ior->io_Dev;
  status.ds_DriverIdentity = DI_MEI_CDROM;
  status.ds_DriverStatusVersion = 0;
  status.ds_MaximumStatusSize = sizeof status;
  status.ds_DeviceFlagWord = (uint32) theDevice->cdrom_StatusByte;
  status.ds_FamilyCode = DS_DEVTYPE_CDROM;
  if (theDevice->cdrom_TOC_Entry[1].addressAndControl &
      CD_CTL_DATA_TRACK) {
    DBUG(("Track 1 is data, assuming blocksize is 2048\n"));
    status.ds_DeviceBlockSize = CDROM_M1_D;
    status.ds_DeviceUsageFlags = DS_USAGE_FILESYSTEM | DS_USAGE_READONLY;
  } else {
    DBUG(("Track 1 is data, assuming blocksize is 2352\n"));
    status.ds_DeviceBlockSize = CDROM_DA;
    status.ds_DeviceUsageFlags = DS_USAGE_READONLY;
  }
  status.ds_DeviceBlockCount = theDevice->cdrom_MediumBlockCount;
  status.ds_DeviceLastErrorCode = theDevice->cdrom_LastErrorCode;
  status.ds_DeviceMediaChangeCntr = theDevice->cdrom_DeviceMediaChangeCntr;
  DBUG(("CDROMDriverSetStatus: dev status byte 0x%x\n", theDevice->cdrom_StatusByte));
  actual = (int32) ((ior->io_Info.ioi_Recv.iob_Len < sizeof status) ?
		    ior->io_Info.ioi_Recv.iob_Len :  sizeof status) ;
  memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &status, actual);
  ior->io_Actual = actual;
}

void CDROMDriverSetDiscData(IOReq *ior)
{
  struct CDROM_Disc_Data data;
  CDROM *theDevice;
  int32 actual;
#ifdef DEBUG
  qprintf(("CD-ROM disc-data call via request at %d\n", ior));
#endif
  theDevice = (CDROM *) ior->io_Dev;
  memcpy(&data.info, &theDevice->cdrom_DiscInformation, sizeof data.info);
  memcpy(data.TOC, theDevice->cdrom_TOC_Entry, sizeof data.TOC);
  memcpy(&data.session, &theDevice->cdrom_SessionInformation, sizeof data.session);
  actual = (int32) ((ior->io_Info.ioi_Recv.iob_Len >= sizeof data) ?
		    ior->io_Info.ioi_Recv.iob_Len :  sizeof data) ;
  memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &data, actual);
  ior->io_Actual = actual;
}

void AbortCDROMIo(IOReq *theRequest)
{
  CDROM *theDevice;
  uint32 interrupts;
  theDevice = (CDROM *) theRequest->io_Dev;
  interrupts = Disable(); /*** DISABLE ***/
  LOG(theDevice,ABORTIO);
/*
  If this is the request that the device is currently servicing, then
  simply abort the lower-level I/O requests - the endaction code will
  perform the cleanup for both levels.  If this request is not yet being
  serviced, dequeue and kill it immediately.
*/
  if (theDevice->cdrom_RequestRunning == theRequest) {
    switch (theDevice->cdrom_Level0) {
    case CDROM_Level0_Data:
      SuperinternalAbortIO(theDevice->cdrom_DataReq);
      break;
    case CDROM_Level0_Command:
      SuperinternalAbortIO(theDevice->cdrom_CommandStatusReq);
      break;
    case CDROM_Level0_CommandData:
      SuperinternalAbortIO(theDevice->cdrom_DataReq);
      SuperinternalAbortIO(theDevice->cdrom_CommandStatusReq);
      break;
    default:
      break;
    }
  } else {
    theRequest->io_Error = MakeCDErr(ER_SEVER,ER_C_STND,ER_Aborted);
    RemNode((Node *) theRequest);
    SuperCompleteIO(theRequest);
  }
  Enable(interrupts);
  return;
}

IOReq *CDROMDriverDummyEndAction(IOReq *ior)
{
  return NULL;
}

IOReq *CDROMDriverEndAction(IOReq *ior)
{
  CDROM *theDevice;
  uint32 signalDaemon;
  uint32 actual;
  uint8 statusByte;
  theDevice = (CDROM *) ior->io_Info.ioi_User;
  LOG(theDevice,ENDACTION);
  signalDaemon = FALSE;
  actual = ior->io_Actual;
  if (ior == theDevice->cdrom_DataReq) {
    eaCount ++;
    if (actual != 2048) {
      wrongSize = TRUE;
      badSize = actual;
    }
    theDevice->cdrom_RequestRunning->io_Actual += actual;
    theDevice->cdrom_NextBlockOffset += theDevice->cdrom_BlocksPerDataXfer;
    if (ior->io_Error == 0 && theDevice->cdrom_ReadRemaining > 0) {
      ior->io_Info.ioi_Recv.iob_Buffer =
	(char *) ior->io_Info.ioi_Recv.iob_Buffer + actual;
      if (ior->io_Info.ioi_Recv.iob_Len > theDevice->cdrom_ReadRemaining) {
	ior->io_Info.ioi_Recv.iob_Len = theDevice->cdrom_ReadRemaining;
      }
      theDevice->cdrom_ReadRemaining -= ior->io_Info.ioi_Recv.iob_Len;
      if (theDevice->cdrom_TimeoutClock < CDROM_DEFAULT_TIMEOUT) {
	theDevice->cdrom_TimeoutClock = CDROM_DEFAULT_TIMEOUT;
      }
      ior->io_Actual = 0;
      ior->io_Info.ioi_Flags &= ~IO_DONE; /* clean this up */
#ifdef DEBUG
      memset((char *) ior->io_Info.ioi_Recv.iob_Buffer, '@',
	     ior->io_Info.ioi_Recv.iob_Len);
#endif      
      return ior;
    }
    switch (theDevice->cdrom_Level0) {
    case CDROM_Level0_Data:
    case CDROM_Level0_KillData:
      theDevice->cdrom_Level0 = CDROM_Level0_EndAction;
      theDevice->cdrom_TimeoutClock = 0;
      signalDaemon = TRUE;
      break;
    case CDROM_Level0_CommandData:
      if (theDevice->cdrom_DoReadahead) {
	theDevice->cdrom_DoReadahead = FALSE;
	theDevice->cdrom_ReadaheadActive = TRUE;
	theDevice->cdrom_Level0 = CDROM_Level0_EndActionReadahead;
	theDevice->cdrom_TimeoutClock = 0;
	signalDaemon = TRUE;
      } else {
	theDevice->cdrom_ReadaheadActive = FALSE;
	theDevice->cdrom_Level0 = CDROM_Level0_Command;
      }
      break;
    default:
      break; /* shouldn't happen */
    }
  } else {
    if (!(theDevice->cdrom_TweakFlags & CDROM_TWEAK_SENDS_TAG) && actual > 0) {
      actual++;
      ior->io_Actual = actual;
    }
    theDevice->cdrom_ReadaheadActive = FALSE;
    switch (theDevice->cdrom_Level0) {
    case CDROM_Level0_Readahead:  /* usually ABORT but not at end-of-disc */
      if ((ior->io_Error & 0x0000001F) == ((ER_C_STND << ERR_CLASHIFT) +
					   (ER_Aborted << ERR_ERRSHIFT))) {
	theDevice->cdrom_Level0 = CDROM_Level0_Holdoff;
	theDevice->cdrom_Holdoff_MS = 1;
      } else {
	theDevice->cdrom_Level0 = CDROM_Level0_EndAction;
	theDevice->cdrom_MustReadError = TRUE;
	theDevice->cdrom_MustFlush = TRUE;
      }
      theDevice->cdrom_TimeoutClock = 0;
      signalDaemon = TRUE;
      break;
    case CDROM_Level0_EndActionReadahead:
      theDevice->cdrom_Level0 = CDROM_Level0_EndAction;
      theDevice->cdrom_TimeoutClock = 0;
      signalDaemon = TRUE;
      break;
    case CDROM_Level0_Command:    /* end of current command */
      theDevice->cdrom_Level0 = CDROM_Level0_EndAction;
      theDevice->cdrom_TimeoutClock = 0;
      if (actual > 0) {
	statusByte = theDevice->cdrom_StatusBuffer[actual-1];
      } else {
	statusByte = CDROM_STATUS_ERROR;
      }
      if (statusByte & CDROM_STATUS_ERROR) {
	theDevice->cdrom_StatusByte = statusByte;
	caughtError = TRUE;
      }
      signalDaemon = TRUE;
      break;
    case CDROM_Level0_CommandData:
      if (actual > 0) {
	statusByte = theDevice->cdrom_StatusBuffer[actual-1];
      } else {
	statusByte = CDROM_STATUS_ERROR;
      }
      if (statusByte & CDROM_STATUS_ERROR) {
	theDevice->cdrom_StatusByte = statusByte;
	theDevice->cdrom_Level0 = CDROM_Level0_KillData;
	caughtError = TRUE;
	signalDaemon = TRUE;
      } else {
	theDevice->cdrom_Level0 = CDROM_Level0_Data;
      }
      break;
    }
  }
  if (signalDaemon) {
    if (daemon != (Task *) NULL) {
      SuperinternalSignal(daemon, daemonSignal);
    }
  }
  LOG(theDevice,ENDOFENDACTION);
  return NULL;
}

