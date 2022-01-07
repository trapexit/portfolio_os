/*****

$Id: FileDaemon.c,v 1.26 1994/08/15 17:16:04 shawn Exp $

$Log: FileDaemon.c,v $
 * Revision 1.26  1994/08/15  17:16:04  shawn
 * Badly placed endif for DOCACHE.
 *
 * Revision 1.25  1994/07/26  21:04:34  dplatt
 * Modify cache flush logic to make sure that we don't release memory
 * out from under an in-progress direct-read-into-cache (as occurs
 * in the Catapult index reader).  If cache has busy I/O, delay the
 * flush until it winds down.
 *
 * Revision 1.24  1994/05/13  00:42:22  cramer
 * fix compilation warnings.
 *
 * Revision 1.23  1994/01/21  00:29:21  dplatt
 * Recover from RCS bobble
 *
 * Revision 1.23  1994/01/19  20:31:13  dplatt
 * Run scheduler even if requests-to-do and requests-pending lists are
 * empty.  Some filesystems need a schedule-pop to handle the endaction.
 *
 * Revision 1.22  1993/12/16  00:43:19  dplatt
 * Remove residue of CD-ROM daemon support, as it's handled in the operator
 * now.  Add support for filesystems which need a timeslice every few
 * VBLs to handle timeouts.
 *
 * Revision 1.21  1993/11/24  06:57:07  limes
 * Quiet down the debug messages.
 *
 * Revision 1.20  1993/08/09  22:09:24  dplatt
 * SuperWait -> SuperWaitSignal
 *
 * Revision 1.19  1993/07/20  07:04:19  dplatt
 * Directory cache
 *
 * Revision 1.18  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.18  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.17  1993/05/08  01:08:14  dplatt
 * Add flat-file-system/NVRAM support, and recover from RCS bobble
 *
 * Revision 1.16  1993/04/26  20:12:55  dplatt
 * Quiet coldstart;  CD-ROM retry limit increased to 10
 *
 * Revision 1.15  1993/04/22  21:03:15  dplatt
 * New features, timeout support, bug fixes
 *
 * Revision 1.14  1993/03/24  23:41:16  dplatt
 * New drive, timeouts, multiple drives, new xbus features, etc.
 *
 * Revision 1.13  1993/03/17  18:13:52  dplatt
 * Use new timer-device semantics
 *
 * Revision 1.12  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.11  1993/01/05  20:57:47  dplatt
 * CES changes redux
 *
 * Revision 1.10  1993/01/04  02:19:26  dplatt
 * CES Changes
 *
 * Revision 1.9  1992/12/22  10:29:16  dplatt
 * Fix timing problems when fs starts
 *
 * Revision 1.8  1992/12/22  09:38:50  dplatt
 * Fix CD-ROM aborts and timer trouble
 *
 * Revision 1.7  1992/12/22  07:58:10  dplatt
 * Magneto 3 changes and CD-ROM support
 *
 * Revision 1.6  1992/12/08  05:59:52  dplatt
 * Magenta changes
 *
 * Revision 1.5  1992/10/24  00:40:56  dplatt
 * Bluebird changes and bug fixes
 *
 * Revision 1.4  1992/10/16  01:22:24  dplatt
 * First cut at bluebird release changes
 *
 * Revision 1.3  1992/10/01  23:36:21  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.2  1992/09/11  22:37:38  dplatt
 * New compiler
 *
 * Revision 1.1  1992/09/11  00:42:25  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright New Technologies Group, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  FileDaemon.c - code to implement the task which runs as a daemon
  on behalf of the file folio.  This task takes care of actions such as
  I/O scheduling, which need to run within a task environment (i.e. not
  at the interrupt level).
*/

#define SUPER

#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "debug.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "folio.h"
#include "io.h"
#include "filesystem.h"
#include "filesystemdefs.h"
#include "discdata.h"
#include "super.h"
#include "time.h"

void FileDaemonStartup(void);
void FileDaemonInternals(void);

#define FILEDAEMON_STARTIO_LATENCY 1

/* #define DEBUG */

#define FASTSCHEDULE
#define VBLTIMER

#ifdef DEBUG
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifdef PRODUCTION
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
# define DBUG0(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
# define oqprintf(x) /* x */
# define DBUG0(x) /* if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x */
#endif

#ifdef DOCACHE

extern char *fsCacheBase;
extern int32 fsCacheSize;
extern int32 fsCacheBusy;
extern IoCacheEntry *fsCacheEntryArray;
extern int32 fsCacheEntrySize;
extern List fsCacheList;

#endif

extern struct KernelBase *KernelBase;

extern void ScheduleDiskIo(OptimizedDisk *theDevice);

extern void MySysErr(int32 err);

/*
  Here's where all the fun happens.  The daemon allocates a set of signals
  for the following purposes:

  "QueuedSignal" - sent by the file driver to tell the daemon that the
     driver has just queued up the "first" I/O for a relatively slow
     device such as a CD-ROM... "first" meaning "the queue was empty, and
     now it is not."  The daemon fires off a timer which will signal it
     after a latency period of a few milliseconds... the expiration of the
     timer will result in the scheduling and initiation of I/Os on all
     devices known to the daemon.

  "WaitingSignal" - sent by the file driver to tell the daemon that a
     filesystem user has just gone to sleep, awaiting an I/O completion.
     This signal will trigger the scheduling of I/Os on any idle devices.

  "RescheduleSignal" - sent by the file driver's I/O endaction handler to
     tell the daemon that it was unable to start another I/O... either
     because the to-do list has been emptied, or because a higher-
     priority I/O request has caused the device's BOINK bit to be set.
     The daemon will reschedule and initiate I/Os on all idle devices.
*/

void FileDaemonInternals(void)
{
#ifdef DODAEMON
  uint32 queuedSignal;
  uint32 waitingSignal;
  uint32 rescheduleSignal;
  uint32 sleepMask;
  Item timerDeviceItem, ioReqItem;
#ifdef VBLTIMER
  Item vblTimerItem;
  int32 vblRunning, vblExpired, needVbl;
  IOReq *vbl;
#endif
  FileSystem *fileSystem;
  IOReq *ior;
  HighLevelDisk *theDisk;
  int32 timerRunning, startTimer, scheduleNow;
  int32 startHeartbeat;
  uint32 awakenedMask;
  TagArg ioReqTags[2];
#ifdef DOCACHE
  int32 interrupts;
  int32 mustFlushCache;
#endif
  const struct timeval scheduleDelay = {0, FILEDAEMON_STARTIO_LATENCY * 1000};
/*
  Check to see if we're being called by the blessed filesystem daemon.
  If not, terminate the caller with extreme prejudice.
*/
  oqprintf(("File-daemon SWI!\n"));
  if (fileFolio == (FileFolio *) NULL
/***      || fileFolio->ff_Daemon.ffd_Task != CURRENTTASK ***/ ) {
    goto seppuku;
  }
  queuedSignal = SuperAllocSignal(0L);
  waitingSignal = SuperAllocSignal(0L);
  rescheduleSignal = SuperAllocSignal(0L);
/*
  Publish them
*/
  fileFolio->ff_Daemon.ffd_QueuedSignal = queuedSignal;
  fileFolio->ff_Daemon.ffd_WaitingSignal = waitingSignal;
  fileFolio->ff_Daemon.ffd_RescheduleSignal = rescheduleSignal;
  DBUG(("Daemon signals created\n"));
  DBUG(("Seeking timer device\n"));
  timerDeviceItem = SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
				       "timer");
  if (timerDeviceItem < 0) {
    qprintf(("Error finding timer device\n"));
    MySysErr(timerDeviceItem);
    goto shutdown;
  }
  DBUG(("Timer is item 0x%x\n", timerDeviceItem));
  timerDeviceItem = SuperOpenItem(timerDeviceItem, (void *) NULL);
  if (timerDeviceItem < 0) {
    qprintf(("Error opening timer device\n"));
    MySysErr(timerDeviceItem);
    goto shutdown;
  }
  DBUG(("Timer open\n"));
  fileFolio->ff_Daemon.ffd_TimerDevice = (TimerDevice *) LookupItem(timerDeviceItem);
  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) timerDeviceItem;
  ioReqTags[1].ta_Tag = TAG_END;
  DBUG(("Creating IOReq(s)s for timer\n"));
  ioReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    qprintf(("Error creating IOReq node\n"));
    goto shutdown;
  }
  DBUG(("Timer IOReq is item 0x%x ", ioReqItem));
  ior = (IOReq *) LookupItem(ioReqItem);
  DBUG(("at 0x%x\n", ior));
#ifdef VBLTIMER
  vblTimerItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (vblTimerItem < 0) {
    qprintf(("Error creating IOReq node\n"));
    goto shutdown;
  }
  DBUG(("VBL IOReq is item 0x%x ", vblTimerItem));
  vbl = (IOReq *) LookupItem(vblTimerItem);
  vblRunning = FALSE;
#endif
  sleepMask = queuedSignal | waitingSignal | rescheduleSignal |
    SIGF_IODONE | SIGF_MEMLOW | SIGF_MEMGONE;
  timerRunning = FALSE;
#ifdef DOCACHE
  mustFlushCache = FALSE;
#endif
  oqprintf(("File-daemon idle-loop starting\n"));
  fileFolio->ff_Daemon.ffd_Task = KernelBase->kb_CurrentTask;
  do {
    DBUG(("File daemon zzz...\n"));
    awakenedMask = SuperWaitSignal(sleepMask);
    DBUG(("File daemon awake\n"));
    scheduleNow = FALSE;
    startTimer = FALSE;
    startHeartbeat = FALSE;
    vblExpired = FALSE;
    if (awakenedMask & SIGF_ABORT) {
      DBUG0(("SIGF_ABORT!\n"));
      break;
    }
    if (awakenedMask & (SIGF_MEMLOW | SIGF_MEMGONE)) {
      DBUG(("Memory is low!\n"));
#ifdef DOCACHE
      if (fsCacheBase) {
	mustFlushCache = TRUE;
      }
#endif
    }
#ifdef DOCACHE
    if (mustFlushCache && fsCacheBusy == 0) {
      mustFlushCache = FALSE;
      if (fsCacheBase) {
	DBUG(("Purging filesystem cache\n"));
	interrupts = Disable();
	FreeMem(fsCacheBase, fsCacheSize);
	fsCacheBase = NULL;
	FreeMem(fsCacheEntryArray, fsCacheEntrySize);
	fsCacheEntryArray = NULL;
	InitList(&fsCacheList, "no cache");
	Enable(interrupts);
      }
    }
#endif
    if (awakenedMask & queuedSignal) {
#ifdef FASTSCHEDULE
      scheduleNow = TRUE;
#else
      startTimer = TRUE;
#endif
      DBUG(("Got an I/O-queued signal\n"));
    }
    if (awakenedMask & (waitingSignal | rescheduleSignal)) {
      DBUG(("Schedule now!\n"));
      scheduleNow = TRUE;
    }
    if (awakenedMask & SIGF_IODONE) {
      if (timerRunning && CheckIO(ioReqItem)) {
	DBUG(("Timer runout, schedule now!\n"));
	timerRunning = FALSE;
	scheduleNow = TRUE;
      }
#ifdef VBLTIMER
      if (vblRunning && CheckIO(vblTimerItem)) {
	DBUG(("VBL runout, delay %d, actual %d\n", vbl->io_Info.ioi_Offset,
	       vbl->io_Actual));
	if (vbl->io_Error < 0) {
	  MySysErr(vbl->io_Error);
	}
	vblRunning = FALSE;
	vblExpired = TRUE;
      }
#endif
    }
    if (startTimer && !timerRunning && !scheduleNow) {
      ior->io_Info.ioi_Unit = TIMER_UNIT_USEC;
      ior->io_Info.ioi_Command = TIMERCMD_DELAY;
      ior->io_Info.ioi_Send.iob_Buffer = (void *) &scheduleDelay;
      ior->io_Info.ioi_Send.iob_Len = sizeof scheduleDelay;
      DBUG(("Setting timer\n"));
      if (SuperinternalSendIO(ior) < 0) {
	qprintf(("Error posting timer delay request"));
	scheduleNow = TRUE;
      } else {
	timerRunning = TRUE;
      }
    }
#ifdef VBLTIMER
    needVbl = FALSE;
    if (scheduleNow || vblExpired) {
      DBUG(("Scheduling\n"));
      for (fileSystem = (FileSystem *) FIRSTNODE(&fileFolio->ff_Filesystems);
	   ISNODE(&fileFolio->ff_Filesystems, fileSystem);
	   fileSystem = (FileSystem *) NEXTNODE(fileSystem)) {
	theDisk = fileSystem->fs_Device;
	if (fileSystem->fs_Flags & FILESYSTEM_NEEDS_VBLS) {
	  needVbl = TRUE;
	  (*theDisk->hdi_ScheduleIO)(theDisk);
	  (*theDisk->hdi_StartIO)(theDisk);
	} else if (scheduleNow) {
	  (*theDisk->hdi_ScheduleIO)(theDisk);
	  (*theDisk->hdi_StartIO)(theDisk);
	}
      }
    }
    if (needVbl && !vblRunning) {
      vbl->io_Info.ioi_Unit = TIMER_UNIT_VBLANK;
      vbl->io_Info.ioi_Command = TIMERCMD_DELAY;
      vbl->io_Info.ioi_Offset = 5;
      if (SuperinternalSendIO(vbl) < 0) {
	qprintf(("Error posting vbltimer delay request"));
      } else {
	vblRunning = TRUE;
      }
    }
    DBUG(("Scheduling done\n"));
#else
    if (scheduleNow) {
      DBUG(("Scheduling\n"));
      for (fileSystem = (FileSystem *) FIRSTNODE(&fileFolio->ff_Filesystems);
	   ISNODE(&fileFolio->ff_Filesystems, fileSystem);
	   fileSystem = (FileSystem *) NEXTNODE(fileSystem)) {
	theDisk = fileSystem->fs_Device;
	if (!theDisk->hdi_DeviceBusy && 
	    (ISEMPTYLIST(&theDisk->hdi_RequestsToDo) ||
	     ISEMPTYLIST(&theDisk->hdi_RequestsDeferred))) {
	  (*theDisk->hdi_ScheduleIO)(theDisk);
	  (*theDisk->hdi_StartIO)(theDisk);
	}
      }
    }
#endif
  } while (TRUE);
 shutdown:
  DBUG0(("File-daemon shutdown!\n"));
  fileFolio->ff_Daemon.ffd_Task = (Task *) NULL;
 seppuku:
  SuperDeleteItem(CURRENTTASK->t.n_Item); /* Eat frozen death, alien slime! */
#endif
}
