/*****

$Id: FileFolioMain.c,v 1.40 1994/09/14 00:32:59 dplatt Exp $

$Log: FileFolioMain.c,v $
 * Revision 1.40  1994/09/14  00:32:59  dplatt
 * Don't bother to report cannot-set-filename errors for the emulat4ed
 * Mac filesystem.  If the cdrom.image file isn't there, the mount
 * attempt will fail anyhow...
 *
 * Revision 1.39  1994/07/26  21:07:20  dplatt
 * & etc.
 *
 * Revision 1.38  1994/07/14  01:23:54  limes
 * Now that banners don't print on the screen,
 * we can print banners in non-DEV builds.
 *
 * Revision 1.37  1994/06/16  21:10:59  dplatt
 * Implement a "Filesystem is really, truly up and ready to go" flag.
 * This is FALSE during startup.  It is changed to TRUE when the main code
 * finishes chewing through all devices and attempting a mount on each
 * of them.
 *
 * Revision 1.36  1994/05/13  00:42:22  cramer
 * fix compilation warnings.
 *
 * Revision 1.35  1994/04/22  19:43:51  dplatt
 * Put DBUG0 back the way it belongs.
 *
 * Revision 1.34  1994/03/24  01:48:43  dplatt
 * Add support for kernel events, hot-mount/hot-dismount.  Minor
 * security enhancements and rearrangements of various things.
 *
 * Revision 1.33  1994/02/18  23:02:21  limes
 * Do not complain about unmountable devices.
 *
 * Revision 1.32  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.31  1994/02/04  05:32:38  limes
 * really call print_vinfo, do not just pretend.
 *
 * Revision 1.30  1994/02/02  02:03:27  limes
 * switch banner to "print_vinfo" version.
 *
 * Revision 1.29  1993/12/01  00:04:25  limes
 * Fix some compile warnings.
 *
 * Revision 1.28  1993/11/24  06:57:18  limes
 * Quiet down the debug messages.
 *
 * Revision 1.27  1993/10/28  22:56:46  limes
 * Oops ... we need fsItem more often than I thought. Better a warning
 * during some compiles, than an error during others (at least
 * until we figure out just when fsItem is really needed).
 *
 * Revision 1.26  1993/10/25  23:18:39  limes
 * Fix a compile warning.
 *
 * Revision 1.25  1993/06/24  03:02:06  dplatt
 * Fix nvram problems
 *
 * Revision 1.24  1993/06/16  01:09:39  dplatt
 * Use AllocMem rather than malloc
 *
 * Revision 1.23  1993/06/15  20:15:34  dplatt
 * Don't cache file info for Macintosh /remote files
 *
 * Revision 1.22  1993/06/15  00:55:14  dplatt
 * Ensure that OpenItem calls are in place
 *
 * Revision 1.21  1993/06/14  01:29:20  dplatt
 * Add startup message with built date
 *
 * Revision 1.20  1993/06/14  01:00:23  dplatt
 * Dragon beta release
 *
 * Revision 1.19  1993/05/28  21:43:11  dplatt
 * Cardinal3 changes, get ready for Dragon
 *
 * Revision 1.18  1993/05/08  01:08:14  dplatt
 * Add flat-file-system/NVRAM support, and recover from RCS bobble
 *
 * Revision 1.17  1993/04/26  20:12:55  dplatt
 * Quiet coldstart;  CD-ROM retry limit increased to 10
 *
 * Revision 1.16  1993/04/23  21:49:56  dplatt
 * CD-rom enhancements, cleanup
 *
 * Revision 1.15  1993/04/22  21:03:15  dplatt
 * New features, timeout support, bug fixes
 *
 * Revision 1.14  1993/04/07  16:58:19  dplatt
 * Don't set File Daemon quantum;  let Dale set default.
 *
 * Revision 1.13  1993/03/24  23:41:16  dplatt
 * New drive, timeouts, multiple drives, new xbus features, etc.
 *
 * Revision 1.12  1993/03/16  06:36:37  dplatt
 * Functional Freeze release
 *
 * Revision 1.11  1993/02/21  04:02:11  dplatt
 * Get ready to mount multiple CD-ROMs (not really there yet...)
 *
 * Revision 1.10  1993/02/11  19:39:37  dplatt
 * Developer-release and new-kernel changes
 *
 * Revision 1.9  1993/02/09  01:47:20  dplatt
 * Reorganize and update for developer release
 *
 * Revision 1.8  1993/01/04  02:19:26  dplatt
 * CES Changes
 *
 * Revision 1.7  1992/12/22  07:58:10  dplatt
 * Magneto 3 changes and CD-ROM support
 *
 * Revision 1.6  1992/12/08  05:59:52  dplatt
 * Magenta changes
 *
 * Revision 1.5  1992/10/27  01:35:41  dplatt
 * Oops, tested on UGO not on Blue
 *
 * Revision 1.4  1992/10/24  00:40:56  dplatt
 * Bluebird changes and bug fixes
 *
 * Revision 1.3  1992/10/16  01:22:24  dplatt
 * First cut at bluebird release changes
 *
 * Revision 1.2  1992/10/01  23:36:21  dplatt
 * Switch to int32/uint32
 *
 * Revision 1.1  1992/09/11  00:42:27  dplatt
 * Initial revision
 *

 *****/

/*
  Copyright The 3DO Company Inc., 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  FileFolioMain.c - coldstart code for the filesystem.  Creates the
  file folio and the file driver, and mounts filesystems.
*/

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
#include "filesystem.h"
#include "filesystemdefs.h"
#include "discdata.h"
#include "cdrom.h"
#include "operror.h"
#include "semaphore.h"

#include "filefunctions.h"
#include "event.h"

#ifndef ARMC
#include <stdlib.h>
#endif

#include "strings.h"
#include "stdio.h"

extern void FileDaemonStartup(void);

extern TagArg fileFolioTags[];
extern TagArg fileDriverTags[];

extern Item SuperCreateSizedItem(int32 itemType, void *whatever, int32 size);

#ifdef SCANMOUNT
static void ScanMount(void);
List cdRomDevList;
#endif

#ifdef AUTOMOUNT
static void Automount(void);
Item msgPortItem;
#endif

#define DAEMONUSERSTACK 128

static char daemonStack[DAEMONUSERSTACK];

int32 fsOpenForBusiness = FALSE;

#ifdef DODAEMON
static TagArg fileDaemonTags[] =
{
  TAG_ITEM_PRI,	        	(void *) 210,
  CREATETASK_TAG_PC,		(void *) ((long)FileDaemonStartup),
  TAG_ITEM_NAME,		"Aahz",
  CREATETASK_TAG_SP,            (void *) (daemonStack+DAEMONUSERSTACK),
  CREATETASK_TAG_STACKSIZE,	(void *) DAEMONUSERSTACK,
  TAG_END,			0
};
#endif

#ifdef PRODUCTION
# define qprintf(x) /* x */
# define oqprintf(x) /* x */
#else
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) printf x
# define oqprintf(x) /* x */
#endif

#ifdef DOCDROM

extern TagArg cdRomDriverTags[];

extern int32 doDoublespeedByDefault;

#endif

extern void MySysErr(int32 err);
extern Item createCDRomDriver(void);

extern const char whatstring[];

FileSystem * InitFileSystem(Device *theDevice);

#undef  DEBUG

#ifdef DEBUG
#define DBUG(x) qprintf (x)
#else
#define DBUG(x) /* x */
#endif

#define DBUG0(x) printf x

void pause() {
  static int i = 0;
  i++;
}

int main(int32 argc, char **argv)
{
  int32 i;
#ifdef MACFS
  Item pseudoRomItem, ioReqItem;
  IOInfo ioInfo;
  static TagArg ioReqTags[2] =
    {
      CREATEIOREQ_TAG_DEVICE,       0,
      TAG_END,			0
    };
  static const char unitName[] = "cdrom.image";
  static const char macPath[] = "remote";
#endif
#ifdef DOCDROM
  Item cdRomItem = 0;
  int8 unit;
  CDROM *cdRomDevice;
  int32 mountCD = TRUE;
#endif
#ifdef DODAEMON
  Item fileDaemonItem;
#endif
#ifndef SCANMOUNT
  Item ramItem;
#endif
#if defined(MACFS) || !defined(SCANMOUNT)
  Item fsItem;
#endif
  int32 argnum;

  print_vinfo();

  InitList(&cdRomDevList, NULL);
  for (argnum = 1; argnum < argc; argnum++) {
#ifdef DOCDROM
    if (strcmp(argv[argnum], "-ds") == 0) {
      doDoublespeedByDefault = TRUE;
    } else if (strcmp(argv[argnum], "-nocd") == 0) {
      mountCD = FALSE;
    } else
#endif
    {
      qprintf(("Unrecognized option %s\n", argv[argnum]));
    }
  }
  DBUG(("Creating folio\n"));
  i = CreateItem(MKNODEID(KERNELNODE,FOLIONODE), fileFolioTags);
  if (i != FILEFOLIO) {
    PrintError(0,"create file folio",0,(uint32) i);
    if (i >= 0) {
      qprintf(("expected item #%d, got item #%d\n", FILEFOLIO, i));
    }
    goto bailout;
  }
  DBUG(("File folio is item 0x%x ", i));
  fileFolio = (FileFolio *) LookupItem(i);
  DBUG(("at %x task data index %d\n", fileFolio, fileFolio->ff.f_TaskDataIndex));
  DBUG(("Creating file driver\n"));
  i = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE), fileDriverTags);
  if (i < 0) {
    qprintf(("Couldn't create file driver (0x%x)\n", i));
    PrintError(0,"create file driver",0,(uint32) i);
    goto bailout;
  }
  fileDriver = (Driver *) LookupItem(i);
  DBUG(("File driver is item 0x%x at 0x%x\n", i, fileDriver));
  /*
    At this point we should create a daemon task and link it up
    */
#ifdef DOCDROM
  DBUG(("Creating CD-ROM driver\n"));
  cdRomItem = createCDRomDriver();
  if (cdRomItem < 0) {
    PrintError(0,"install CD-ROM driver",0,cdRomItem);
  }
#endif
#ifdef DODAEMON
  DBUG(("Creating file-daemon task\n"));
  fileDaemonItem = CreateItem(MKNODEID(KERNELNODE,TASKNODE),
			      fileDaemonTags);
  if (fileDaemonItem<0) {
    PrintError(0,"create file daemon task",0,(uint32) fileDaemonItem);
  }
  while (!fileFolio->ff_Daemon.ffd_Task) {
    pause();
  }
#endif
#ifdef AUTOMOUNT
  msgPortItem = CreateMsgPort("Automount", 0, 0);
#endif
  oqprintf(("Mounting file systems\n"));
#ifdef SCANMOUNT
  ScanMount();
#else
  DBUG(("Opening RAM device\n"));
  ramItem = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),"ram"),
		     (void *) NULL);
  if (ramItem < 0) {
    DBUG(("Couldn't access RAM device (0x%x)\n", ramItem));
    goto openMassStorage;
  }
  for (unit = ((Device *)LookupItem(ramItem))->dev_MaxUnitNum;
       unit >= 0; unit--) {
    DBUG(("Initializing file system on RAM device %d unit %d\n", ramItem, unit));
    fsItem = MountFileSystem(ramItem, unit, 0);
    if (fsItem < 0) {
      DBUG(("RAM file system did not open (%x)\n", fsItem));
    } else {
      DBUG(("File system is open as 0x%x\n", fsItem));
    }
  }
 openMassStorage:
#ifdef DOCDROM
  if (mountCD) {
    queueEntry = (CDROMDriveQueueEntry *) FirstNode(&cdRomDeviceList);
    while (IsNode(&cdRomDeviceList,queueEntry)) {
      cdRomDevice = queueEntry->cdrdqe_Device;
      cdRomItem = cdRomDevice->cdrom.dev.n_Item;
      DBUG(("Initializing file system on device %d\n", cdRomItem ));
      fsItem = MountFileSystem(cdRomItem, 0, 75*2);
      if (fsItem < 0) {
	DBUG(("CD-ROM file system did not open (%x)\n", fsItem));
      } else {
	DBUG(("File system is open as 0x%x\n", fsItem));
      }
      queueEntry = (CDROMDriveQueueEntry *) NextNode(queueEntry);
    }
  }
#endif
#endif
#ifdef MACFS
  oqprintf(("Doing Mac mounts\n"));
  DBUG(("Opening emulated CD-ROM device\n"));
  pseudoRomItem = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
					 "maccdrom"), (void *) NULL);
  if (pseudoRomItem < 0) {
    qprintf(("Couldn't access emulated CD-ROM device (0x%x)\n", pseudoRomItem));
    goto openMac;
  }
  DBUG(("Creating IOReq\n"));
  ioReqTags[0].ta_Arg = (void *) pseudoRomItem;
  ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  if (ioReqItem < 0) {
    qprintf(("Couldn't create I/O request (0x%x)\n", pseudoRomItem));
    goto openMac;
  }
  DBUG(("Setting unit name\n"));
  memset(&ioInfo, 0, sizeof ioInfo);
  ioInfo.ioi_Command = CDROMCMD_SETUNITNAME;
  ioInfo.ioi_Send.iob_Buffer = (void *) unitName;
  ioInfo.ioi_Send.iob_Len = strlen(unitName);
  i = DoIO(ioReqItem, &ioInfo);
  DeleteItem(ioReqItem);
#ifdef REPORTNAMESETERROR
  if (i < 0) {
    qprintf(("Couldn't set emulated CD-ROM unit name (0x%x)\n", i));
    goto openMac;
  }
#endif
  DBUG(("Initializing file system on device %d\n", pseudoRomItem));
  fsItem = MountFileSystem(pseudoRomItem, 0, 0);
  if (fsItem < 0) {
    DBUG(("Emulated CD-ROM file system did not open (%x)\n", fsItem));
  } else {
    DBUG(("File system is open as 0x%x\n", fsItem));
  }
 openMac:
  DBUG(("Initializing Mac system on '%s'\n", macPath));
  fsItem = MountMacFileSystem((char *) macPath);
  if (fsItem < 0) {
    DBUG(("File system did not open (%x)\n", fsItem));
  } else {
    DBUG(("File system is open as 0x%x!\n", fsItem));
  }
#endif
  fsOpenForBusiness = TRUE;
  printf(("Filesystem startup completed.\n"));
#ifdef AUTOMOUNT
  Automount();
#endif
  WaitSignal(0); /* guaranteed never to return! */
  return 0;
 bailout:
  return (int) i;
}

#ifdef SCANMOUNT
static void ScanMount()
{
  Device *thisDevice, **devTable;
  Item ioReqItem;
#ifdef OLDSTYLE
  IOReq *ioReq;
  IOInfo query;
  DeviceStatus status;
#endif
  int32 i, j, unit, mounted;
  Err err;
  err = LockSemaphore(KernelBase->kb_DevSemaphore, TRUE);
  if (err < 0) {
    PrintError(0,"lock device semaphore in","ScanMount",err);
    return;
  }
  i = 0;
  thisDevice = (Device *) FirstNode(KernelBase->kb_Devices);
  while (IsNode(KernelBase->kb_Devices, thisDevice)) {
    i++;
    thisDevice = (Device *) NextNode(thisDevice);
  }
  devTable = (Device **) AllocMem(i * sizeof (Device *), MEMTYPE_ANY);
  if (!devTable) {
    UnlockSemaphore(KernelBase->kb_DevSemaphore);
    DBUG(("Can't get device table\n"));
    return;
  }
  DBUG(("%d devices in list\n", i));
  j = 0;
  ioReqItem = 0;
  thisDevice = (Device *) FirstNode(KernelBase->kb_Devices);
  while (IsNode(KernelBase->kb_Devices, thisDevice)) {
    devTable[j] = thisDevice;
    j++;
    thisDevice = (Device *) NextNode(thisDevice);
  }
  UnlockSemaphore(KernelBase->kb_DevSemaphore);
  for (j = 0; j < i; j++) {
    thisDevice = devTable[j];
    mounted = FALSE;
    err = OpenItem(thisDevice->dev.n_Item, 0);
    if (err < 0) {
      PrintError(0,"open device",thisDevice->dev.n_Name,err);
      continue;
    }
#ifdef OLDSTYLE
    ioReqItem = CreateIOReq(NULL, 0, thisDevice->dev.n_Item, 0);
    if (ioReqItem < 0) {
      PrintError(0,"create IOReq for device",thisDevice->dev.n_Name,ioReqItem);
      continue;
    }
    ioReq = (IOReq *) LookupItem(ioReqItem);
#endif
    for (unit = 0 ; unit <= thisDevice->dev_MaxUnitNum; unit++) {
#ifdef OLDSTYLE
      memset(&query, 0, sizeof query);
      memset(&status, 0, sizeof status);
      query.ioi_Command = CMD_STATUS;
      query.ioi_Unit = (uint8) unit;
      query.ioi_Recv.iob_Buffer = &status;
      query.ioi_Recv.iob_Len = sizeof status;
      err = DoIO(ioReqItem, &query);
      if (err < 0 || (err = ioReq->io_Error) < 0) {
	DBUG(("Error getting status of %s unit %d\n", thisDevice->dev.n_Name,
	       unit));
#if 0
	PrintError(0,"get status of",thisDevice->dev.n_Name,err);
#endif
	continue;
      }
      if (!(status.ds_DeviceUsageFlags & DS_USAGE_FILESYSTEM)) {
	DBUG(("Device %s unit %d doesn't support filesystems\n",
	       thisDevice->dev.n_Name, unit));
	continue;
      }
      DBUG(("Trying mount on %s unit %d\n", thisDevice->dev.n_Name,
	     unit));
      DBUG(("CMD_Status ioActual = %d\n", ioReq->io_Actual));
      DBUG(("Blocks %d, family code %d, usage flags 0x%x\n",
	     status.ds_DeviceBlockCount, status.ds_FamilyCode,
	     status.ds_DeviceUsageFlags));
      if (status.ds_FamilyCode == DS_DEVTYPE_CDROM) {
	err = MountFileSystem(thisDevice->dev.n_Item, unit, 150);
      } else {
	err = MountFileSystem(thisDevice->dev.n_Item, unit, 0);
      }
#else
      err = MountFileSystem(thisDevice->dev.n_Item, unit, 0);
#endif
      if (err < 0) {
	DBUG(("No file system on %s unit %d\n", thisDevice->dev.n_Name,
	       unit));
	
      } else {
	DBUG(("File system mounted on %s unit %d\n", thisDevice->dev.n_Name,
	       unit));
	mounted = TRUE;
      }
    }
    if (ioReqItem) {
      DeleteItem(ioReqItem);
      ioReqItem = 0;
    }
    if (!mounted) {
      CloseItem(thisDevice->dev.n_Item);
    }
  }
  FreeMem(devTable, i * sizeof (Device *));
}

#endif

#ifdef AUTOMOUNT

static void ProcessEvent(EventBrokerHeader *hdr)
{
  EventFrame *frame;
  DeviceStateEventData *ds;

  if (hdr->ebh_Flavor != EB_EventRecord) {
    return;
  }
  frame = (EventFrame *) (hdr + 1); /* Gawd that's ugly! */
  while (frame->ef_ByteCount > 0) {
    ds = (DeviceStateEventData *) frame->ef_EventData;
    switch (frame->ef_EventNumber) {
    case EVENTNUM_DeviceOnline:
      MountFileSystem(ds->dsed_DeviceItem, ds->dsed_DeviceUnit, 0);
      break;
    case EVENTNUM_DeviceOffline:
      qprintf(("Here the daemon should dismount something!\n"));
      break;
    }
    frame = (EventFrame *) (frame->ef_ByteCount + (char *) frame);
  }
}

static void Automount(void)
{
  Item eventItem;
  uint32 theSignal;
  Message *event;
  MsgPort *msgPort;
  EventBrokerHeader *msgHeader;

  if (msgPortItem < 0) {
    qprintf(("Cannot create event-listener port: "));
    PrintfSysErr(msgPortItem);
    return;
  }

  msgPort = (MsgPort *) LookupItem(msgPortItem);

  DBUG(("Automount listener starting\n"));

  while (TRUE) {
    theSignal = WaitSignal(msgPort->mp_Signal);
    if (theSignal & msgPort->mp_Signal) {
      DBUG(("Automounter wakeup!\n"));
      while (TRUE) {
	eventItem = GetMsg(msgPortItem);
	if (eventItem < 0) {
	  qprintf(("Error 0x%x getting message: ", eventItem));
	  PrintfSysErr(eventItem);
	  return;
	} else if (eventItem == 0) {
	  break;
	}
	event = (Message *) LookupItem(eventItem);
	msgHeader = (EventBrokerHeader *) event->msg_DataPtr;
	ProcessEvent(msgHeader);
	ReplyMsg(eventItem, 0, NULL, 0);
      }
    }
  }
}

#endif

