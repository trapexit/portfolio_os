/*****

$Id: ControlPort.c,v 1.13 1994/07/28 18:44:56 limes Exp $

$Log: ControlPort.c,v $
 * Revision 1.13  1994/07/28  18:44:56  limes
 * (1) DiscOsVersion and friends are declared completely in sherryvers.h,
 *     no need to have an "extern" decl for it here.
 * (2) Use DiscOsVersion, not SuperDiscOsVersion; DiscOsVersion now does
 *     the right thing, super or not.
 *
 * Revision 1.12  1994/07/27  18:49:24  shawn
 * Fix specifically put in for "who shot Johnny Rock"
 * but in general, all titles that are encrypted with OS
 * release less that 1.3, are run with the old behavior of
 * the control port driver. That is all the changes made under
 * rev 1.6 are rolled back (see comments for rev 1.6).
 *
 * Revision 1.11  1994/03/25  01:30:31  dplatt
 * Switch to new method of hooking into ReportEvent().
 *
 * Revision 1.10  1994/03/24  01:54:18  dplatt
 * Debug some problems with the high-level-event unit.
 *
 * Revision 1.9  1994/03/19  01:18:56  dplatt
 * Add management/support for short Control Port DMA transfers, which
 * have a substantially lower latency than full-length ones.
 *
 * Revision 1.8  1994/03/16  23:44:57  dplatt
 * Remainder of Unit 1 implementation.  Adds a callable subroutine,
 * addressible via KernelBase, which takes an event structure and
 * passes it back to the first CMD_READ pending on unit 1 of the Control
 * Port.
 *
 * Revision 1.7  1994/03/16  23:31:30  dplatt
 * First level of real implementation for Unit 1.  Reads to Unit 1 are
 * simply queued.  Writes look for a reader (and get an "end of medium"
 * if none is available... maybe "device offline" would be better?) and
 * copy the data to the reader's buffer (both get "I/O incomplete") if
 * the reader's buffer is smaller that the writer's).
 *
 * Next level of change for shorter Control Port data transfers.  Device
 * now reports its block size as 4 bytes (minimum DMA granularity) with
 * a max of 50 such blocks (== the same 200-byte maximum transfer we
 * used to have).  The Event Broker has been rev'ed to understand this
 * new blocksize*blockcount rule.
 *
 * Revision 1.6  1994/03/16  22:53:37  dplatt
 * First round of changes for the Control Port / Event Broker speedups,
 * latency reductions, and general Good Stuff.
 *
 * The Control Port driver's operation is now split into two separate
 * FIRQ routines.  The VBL FIRQ processes new CMD_WRITE commands to the
 * output buffer (if any have arrived), does bitflipping, and enables
 * the Control Port DMA.  Another FIRQ has been created and is hung
 * off of the "end of Control Port DMA" interrupt... this routine
 * transfers the data up to the first CMD_READ buffer on the chain.
 *
 * The flow of control, and the location of the buffer size and other
 * error-checking code has been reorganized to reduce overhead and
 * FIRQ-time latency.
 *
 * Also, a second unit has been added to the Control Port device... this
 * will be the high-level-event unit.  Not really implemented yet.
 *
 * Revision 1.5  1994/02/21  19:33:35  limes
 * fix RCS ID information
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  ControlPort.c - the driver for the low-level DMA-driven Control Port
  device.

*/

#define SUPER

#include "types.h"
#include "item.h"
#include "kernel.h"
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
#include "operror.h"
#include "event.h"

#include "hardware.h" 
#include "inthard.h"
#include "interrupts.h"
#include "super.h"
#include "sherryvers.h"

#include "controlport.h"
#include "event.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

#include "strings.h"

/* #define DEBUG */

#ifdef DEBUG
#define SHOWERROR
#define DBUG(x) Superkprintf x
#else
#define DBUG(x) /* x */
#endif

#ifdef PRODUCTION
# define DBUG0(x) /* x */
# define qprintf(x) /* x */
#else
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR))  Superkprintf x
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

extern int debugFlag;

static List Unit0List;
static List Unit1List;

static TimerDevice *timer;

static uint32 *controlPortOut;
static uint32 *controlPortIn;
static uint32 *controlPortFlip;

static int32 doFlipping = FALSE;
static int32 missedFlip = FALSE;
static int32 haveWrite  = FALSE;
static int32 processAtDMADone = FALSE;

static int32 ControlPortSetups = 0;
static int32 ControlPortDones = 0;
static int32 controlPortLine = 0;
static int32 controlPortLossage = 0;

static int32 cpBufSize;
static int32 cpDMASize;

uint32 (*getControlPort)(int interrupt);
int32 ControlPortDone(void);

static void
ControlPortAbortIO(ior)
IOReq *ior;
{
	/* If we get this far then the IOReq must be on the list */
	/* We are called inside a Disable */
	RemNode((Node *)ior);
	ior->io_Error = ABORTED;
	SuperCompleteIO(ior);
}

static Item vblFirqItem, dmaFirqItem;

void EnableControlPort(void)
{
    static vuint32 *p = RAMtofrPLAYER;

	/* trigger playerbus dma for next transfer */

    p[0] = (vuint32) controlPortIn;
    p[2] = (vuint32) controlPortOut;
    p[1] = cpDMASize - 4;
    *MCTL |= PLAYXEN;
}

/*
   Control Port setup routine, called by VBL interrupt.  Process any
   CMD_WRITEs to push new data out to the bus, and/or any bitflipping
   which may need to occur.  Then, set up and enable the DMA.
*/

int32
ControlPortSetup()
{
    IOReq *ior, *next;
    uint32 writerVCNT;
    int32 complete;
    int32 i;
    uint32 *out, *flip;
    int32 didWrite;

    writerVCNT = 0;
    didWrite = FALSE;

    ControlPortSetups ++;
    controlPortLine = (int) *VCNT;

    if ((controlPortLine & 0xFF) > 20) {
      if (doFlipping) {
	missedFlip = ! missedFlip;
      }
      return 0;
    }
    
    if (!processAtDMADone) {
      ControlPortDone();         /* do at VBL time for older CD OS versions */
    }

    if (!haveWrite && !doFlipping) {  /** Fast path **/
      EnableControlPort();
      return 0;
    }

    ior = (IOReq *) FirstNode(&Unit0List);

    while (IsNode(&Unit0List, ior)) {
      uint32 len, newDMASize;
      char *buf;
      complete = FALSE;
      next = (IOReq *) NextNode(ior);
      if (ior->io_Info.ioi_Command == CMD_WRITE) {
	buf = (char *) ior->io_Info.ioi_Send.iob_Buffer;
	len = ior->io_Info.ioi_Send.iob_Len;
	writerVCNT = ior->io_Info.ioi_CmdOptions;
	newDMASize = writerVCNT >> 16;
	if (newDMASize > 8 && newDMASize <= cpBufSize &&
	    (newDMASize & 0x3) == 0) {
	  cpDMASize = newDMASize;
	}
	didWrite = TRUE;
	missedFlip = FALSE;
	if (len == cpBufSize) {
	  memcpy(controlPortOut, buf, len);
	  memset(controlPortFlip, 0, len);
	  doFlipping = FALSE;
	  ior->io_Actual = len;
	} else {
	  memcpy(controlPortOut, buf, cpBufSize);
	  memcpy(controlPortFlip, cpBufSize + (char *) buf,
		 cpBufSize);
	  doFlipping = TRUE;
	  ior->io_Actual = len;
	}
	RemNode((Node *) ior);
	SuperCompleteIO(ior);
      }
      ior = next;
    }

    haveWrite = FALSE;

/*
  Flip-bits processing is done if either of the following is true:

  [1] If there was no new data written down by the Event Broker.

  [2] If the VCNT field indicates that the Event Broker is running an odd
      number of fields behind.  Specifically:  the CP driver stores the
      current VCNT into the ioi_CmdOptions of each Read request when the
      read is processed ("This data was read at the beginning of the
      even field").  The Event Broker echoes this count into the
      ioi_CmdOptions of the Write request that it generates as a result
      of the read completion ("This output data is being generated during
      an even field and is intended to be written out to the port during
      the subsequent odd field").  

      If, when the write is actually processed, the field bit in the
      write ioi_Cmdoptions _matches_ the field bit in the current VCNT,
      then it indicates that the data is being written one field _later_
      than it should have been, and the bit-flipping must be performed.

  We apply a slight inhibition to this, though, because we sometimes
  get into vblank so late that we can not send a new frame out.  We keep
  track of the number of times that this has happened, and skip the
  bitflip if we're an odd number of frames behind.

  It'd probably be better to keep _explicit_ track of which field the
  output data is configured for, and flip this state (and the bits) when
  we need to.
*/

    if (doFlipping && !missedFlip) {
      if (!didWrite || !((writerVCNT ^ *VCNT) & 0x00000800)) {
	i = cpBufSize / sizeof (uint32);
	out = (uint32 *) controlPortOut;
	flip = (uint32 *) controlPortFlip;
	do {
	  *out = *out ^ *flip;
	  out++;
	  flip++;
	} while (--i > 0);
      }
    }

    missedFlip = FALSE;

#ifdef SCRUBBUFFER
    memset(controlPortIn, 0xFF, cpBufSize); /* i hate to do this */
#endif

    EnableControlPort();
    return 0;
}

int32
ControlPortDone()
	/* called by interrupt routine at end of DMA, or by VBL server */
{
  IOReq *ior, *next;
  ControlPortDones ++;
  ior = (IOReq *) FirstNode(&Unit0List);
  while (IsNode(&Unit0List, ior)) {
    uint32 len;
    char *buf;
    next = (IOReq *) NextNode(ior);
    if (ior->io_Info.ioi_Command == CMD_READ) {
      buf = (char *) ior->io_Info.ioi_Recv.iob_Buffer;
      len = ior->io_Info.ioi_Recv.iob_Len;
      if (len > cpDMASize) {
	len = cpDMASize;
      }
      memcpy(buf, controlPortIn, len);
      ior->io_Actual = len;
      ior->io_Info.ioi_CmdOptions = *VCNT;
      if (timer) {
	ior->io_Info.ioi_Offset = timer->timerdev_VBlankCount;
      } else {
	ior->io_Info.ioi_Offset = 0;
      }
      RemNode((Node *) ior);
      SuperCompleteIO(ior);
      return 0;
    }
    ior = next;
  }
  controlPortLossage ++;
  return 0;
}

int32 ControlPortEventForwarder(void *foo)
{
  int32 len, interrupts;
  IOReq *reader;
  interrupts = Disable();
  if (IsEmptyList(&Unit1List)) {
    Enable(interrupts);
    return MAKEERR(ER_DEVC,ER_CPORT,ER_SEVER,ER_E_SSTM,ER_C_STND,ER_EndOfMedium);
  }
  reader = (IOReq *) FirstNode(&Unit1List);
  RemNode((Node *) reader);
  Enable(interrupts);
  len = ((EventFrame *) foo)->ef_ByteCount;
  len = (len + 3) & ~ 3;
  if (len > reader->io_Info.ioi_Recv.iob_Len) {
    len = reader->io_Info.ioi_Recv.iob_Len;
    reader->io_Error = 
      MAKEERR(ER_DEVC,ER_CPORT,ER_SEVER,ER_E_SSTM,ER_C_STND,ER_IOIncomplete);
  }
  memcpy(reader->io_Info.ioi_Recv.iob_Buffer, foo, len);
  reader->io_Actual = len;
  if (timer) {
    reader->io_Info.ioi_Offset = timer->timerdev_VBlankCount;
  } else {
    reader->io_Info.ioi_Offset = 0;
  }
  DBUG(("Sending event\n"));
  SuperCompleteIO(reader);
  return len;
}

static Item
ControlPortInit(Driver *drv)
{
        void *err;
	uint32	ret;
	DBUG(("ControlPortInit\n"));

	InitList(&Unit0List,"ControlPort");
	InitList(&Unit1List,"Events");

	cpBufSize = 200;
	cpDMASize = cpBufSize;

	if ((ret = DiscOsVersion(0)) >= DiscOs_1_3) {
	  DBUG(("ControlPortInit: DiscOS >= release 1.3 (0x%x, 0x%x)\n", ret, DiscOs_1_3));
	  processAtDMADone = TRUE;
	} else {
	  DBUG(("ControlPortInit: DiscOS < release 1.3 (0x%x, 0x%x)\n", ret, DiscOs_1_3));
	}

	controlPortIn = (uint32 *) AllocMem(cpBufSize,
					    MEMTYPE_FILL | MEMTYPE_DMA);
	controlPortOut = (uint32 *) AllocMem(cpBufSize,
					     MEMTYPE_FILL | MEMTYPE_DMA);
	controlPortFlip = (uint32 *) AllocMem(cpBufSize,
					      MEMTYPE_FILL | MEMTYPE_DMA);

	DBUG(("CP out buf at 0x%x, flip buf at 0x%x\n",
		     controlPortOut, controlPortFlip));

	if (!controlPortIn || !controlPortOut || !controlPortFlip) {
	  return MakeKErr(ER_SEVER,ER_C_STND,ER_NoMem);
	}

	DBUG(("Creating FIRQs\n"));

	vblFirqItem = SuperCreateFIRQ("ControlPort setup", 200,
				      ControlPortSetup, INT_V1);

	if (vblFirqItem < 0) {
	  return vblFirqItem;
	}

	if (processAtDMADone) {
	  dmaFirqItem = SuperCreateFIRQ("ControlPort done", 200,
					ControlPortDone, INT_DPLY);
	  if (dmaFirqItem < 0) {
	    return dmaFirqItem;
	  }
	} else {
	  dmaFirqItem = -1;
	}

	DBUG(("V0 FIRQ is at 0x%X, DMA FIRQ is at 0x%X\n",
	      LookupItem(vblFirqItem), LookupItem(dmaFirqItem)));

	timer = (TimerDevice *) LookupItem(SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
							      "timer"));
	DBUG(("Input buffer at 0x%x\n", controlPortIn));
	DBUG(("Output buffer at 0x%x\n", controlPortOut));

	DBUG(("Enabling Control Port DMA\n"));

	EnableControlPort();

	if (processAtDMADone) {
	  DBUG(("Enabling Control Port interrupt\n"));
	  EnableInterrupt(INT_DPLY);
	}

	KernelBase->kb_CPUFlags |= KB_CONTROLPORT;

	err = SuperSetFunction(KERNELNODE, -25, VTYPE_SUPER,
			       (void *) ControlPortEventForwarder);
	
	if (0 > (int32) err) {
	  DBUG0(("Could not install ReportEvent handler!\n"));
	}

	DBUG(("Control Port initialization complete\n"));

	DBUG(("Unit 0 list at 0x%X, Unit 1 list at 0x%X\n",
	      &Unit0List, &Unit1List));

	DBUG(("Setup count at 0x%X, Done count at 0x%X\n",
	      &ControlPortSetups, &ControlPortDones));

	return drv->drv.n_Item;
}

static Item
ControlPortDevInit(Device *dev)
{
  dev->dev_MaxUnitNum = 1;
  return dev->dev.n_Item;
}

static int
CmdQueueIt(IOReq *ior)
{
  int32 interrupts;
  char *buf;
  uint32 len;
  int32 command;
  IOReq *reader;
  command = ior->io_Info.ioi_Command;
  switch (ior->io_Info.ioi_Unit) {
  case 0: /* the real Control Port */
    switch (command) {
    case CMD_READ:
      buf = (char *) ior->io_Info.ioi_Recv.iob_Buffer;
      len = ior->io_Info.ioi_Recv.iob_Len;
      if (buf == NULL || len > cpBufSize || (len & 0x3) != 0) {
	ior->io_Error = MakeKErr(ER_SEVERE,ER_C_STND,ER_BadIOArg);
	return 1;
      }
      break;
    case CMD_WRITE:
      buf = (char *) ior->io_Info.ioi_Send.iob_Buffer;
      len = ior->io_Info.ioi_Send.iob_Len;
      if (buf == NULL || (len != cpBufSize && len != cpBufSize * 2)) {
	ior->io_Error = MakeKErr(ER_SEVERE,ER_C_STND,ER_BadIOArg);
	return 1;
      }
      break;
    }
    interrupts = Disable();
    AddTail(&Unit0List, (Node *) ior);
    if (command == CMD_WRITE) {
      haveWrite = TRUE;
    }
    Enable(interrupts);
    break;
  case 1: /* the high-level-event shmoo */
    switch (command) {
    case CMD_READ:
      interrupts = Disable();
      AddTail(&Unit1List, (Node *) ior);
      Enable(interrupts);
      DBUG(("Event reader queued\n"));
      break;
    case CMD_WRITE:
      interrupts = Disable();
      if (IsEmptyList(&Unit1List)) {
	Enable(interrupts);
	ior->io_Error =
	  MAKEERR(ER_DEVC,ER_CPORT,ER_SEVER,ER_E_SSTM,ER_C_STND,ER_EndOfMedium);
	return 1;
      }
      reader = (IOReq *) FirstNode(&Unit1List);
      RemNode((Node *) reader);
      Enable(interrupts);
      len = ior->io_Info.ioi_Send.iob_Len;
      if (len > reader->io_Info.ioi_Recv.iob_Len) {
	len = reader->io_Info.ioi_Recv.iob_Len;
	ior->io_Error = reader->io_Error = 
	  MAKEERR(ER_DEVC,ER_CPORT,ER_SEVER,ER_E_SSTM,ER_C_STND,ER_IOIncomplete);
      }
      memcpy(reader->io_Info.ioi_Recv.iob_Buffer,
	     ior->io_Info.ioi_Recv.iob_Buffer, len);
      ior->io_Actual = reader->io_Actual = len;
      SuperCompleteIO(reader);
      return 1;
    }
    break;
  }
  return 0;
}

static int
CmdStatus(IOReq *ior)
{
  DeviceStatus status;
  int32 len;
  memset(&status, 0, sizeof status);
  status.ds_MaximumStatusSize = sizeof status;
  switch (ior->io_Info.ioi_Unit) {
  case 0:
    status.ds_DeviceBlockSize = sizeof (uint32);
    status.ds_DeviceBlockCount = cpBufSize / sizeof (uint32);
    break;
  case 1:
    status.ds_DeviceBlockSize = sizeof (uint32);
    status.ds_DeviceBlockCount = 1024 / sizeof (uint32);
    break;
  }
  len = ior->io_Info.ioi_Recv.iob_Len;
  if (len > sizeof status) {
    len = sizeof status;
  }
  if (len > 0) {
    memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &status, len);
  }
  ior->io_Actual = len;
  return 1;
}

static int (*CmdTable[])() =
{
	CmdQueueIt,   /* write */
	CmdQueueIt,   /* read */
	CmdStatus,    /* status */
	CmdQueueIt,   /* wait */
	CmdQueueIt,   /* readwrite */
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	TAG_ITEM_NAME,	"controlport",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)ControlPortAbortIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)5,
	CREATEDRIVER_TAG_CMDTABLE,	(void *)CmdTable,
	CREATEDRIVER_TAG_INIT,		(void *)((long)ControlPortInit),
	TAG_END,		0,
};

static TagArg devArgs[] =
{
	TAG_ITEM_NAME,	"ControlPort",
	CREATEDEVICE_TAG_DRVR,  0,
	CREATEDEVICE_TAG_INIT,	(void *)((long)ControlPortDevInit),
	TAG_END,		0,
};

Item
createControlPortDriver(void)
{
	Item devItem;
	Item drvrItem;

#ifdef DEBUG
	kprintf("Creating ControlPort driver\n");
#endif

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);

	if (drvrItem < 0) {
	  return drvrItem;
	}

#ifdef DEBUG
	kprintf("Creating ControlPort device\n");
#endif

	devArgs[1].ta_Arg = (void *) drvrItem;

	devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE), devArgs);

	if (devItem < 0) {
	  return devItem;
	}

#ifdef DEBUG
	kprintf("Control Port setup completed\n");
#endif

	return devItem;
}
