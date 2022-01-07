
/*****

$Id: EventBroker.c,v 1.41 1994/09/30 18:59:27 dplatt Exp $

$Log: EventBroker.c,v $
 * Revision 1.41  1994/09/30  18:59:27  dplatt
 * Rename EventTable to PodStateTable to avoid misrepresenting its
 * true meaning and intended use.
 *
 * Revision 1.40  1994/09/24  04:45:24  dplatt
 * Delete the Current Events Table semaphore when we destroy the table.
 *
 * Revision 1.39  1994/09/24  04:01:28  dplatt
 * Implement the in-memory Current Events table.
 *
 * Revision 1.38  1994/08/10  00:39:09  gregm
 * The same bug that Dave fixed for me about a month ago -- an off-by-one
 * problem with the manipulation of event numbers, which are numbered
 * starting at 1 rather than at 0.
 *
 * Revision 1.37  1994/07/26  23:59:38  limes
 * DiscOsVersion revisions: fix name conflicts. We now have:
 *  - SuperDiscOsVersion(), which does a folio call to it
 *  - swiDiscOsVersion(), which SWIs to it (bad name! what would be better?)
 *  - DiscOsVersion(), which choses Super or swi as appropriate
 *
 * Revision 1.36  1994/07/14  18:19:37  dplatt
 * Off-by-one error in the DispatchEvent() subroutine.  Tended to prevent
 * driver-initiated events from being dispatched to the correct
 * listeners.
 *
 * Revision 1.35  1994/07/09  06:46:53  limes
 * Corrections to the workaround code
 *
 * Revision 1.34  1994/07/09  03:18:00  limes
 * If we are running Rom-over-CD and the OS version on the CD is prior to
 * Portfolio 1.3, then force us into a compatibility mode: do not use
 * short control port DMA transfers. This is done to keep the task
 * switching patterns from changing to the point where certain carefully
 * balanced multithreaded titles stop working smoothly.
 *
 * Revision 1.33  1994/05/27  18:05:30  dplatt
 * Mask off the internal-use-only bits in the pod flags word when sending
 * back an DescribePods response.  SuperWingCommander crashes if a joystick
 * has any bits other than the generic-identity bits set.
 *
 * Revision 1.32  1994/04/28  03:53:41  dplatt
 * Clean up a bit more thoroughly after a listener dies, to ensure that
 * any event-message replies en route from this listener do not cause
 * us to stomp on already-freed-up memory.
 *
 * Revision 1.31  1994/04/07  02:59:27  dplatt
 * Disable initial "seeding" event.  Rethink this - maybe add a
 * "seed me" command-message which will trigger a seed.
 *
 * Revision 1.30  1994/03/24  01:53:11  dplatt
 * Add hooks for the filesystem automounter.  Fix bugs in high-level
 * event dispatch.
 *
 * Revision 1.29  1994/03/19  01:18:10  dplatt
 * Add management/support for short Control Port DMA transfers, which
 * have a substantially lower latency than full-length ones.
 *
 * Revision 1.28  1994/03/16  23:27:16  dplatt
 * Move glasses and mouse drivers out of the application and into
 * separate loadable driverlet files.
 *
 * The Control Port spec is changing from 1 block of 200 bytes, to
 * 50 blocks of 4 bytes.  This will permit short input frames to be
 * implemented.  Do buffer-size calculation properly!
 *
 * Revision 1.27  1994/03/04  01:40:05  dplatt
 * Add some memory-monitor debug code.  #define MEMWATCH to enable it.
 *
 * Revision 1.26  1994/03/01  00:53:54  dplatt
 * Make use of the new #defined symbols.  Also, remove the last (I think)
 * vestiges of support for the old 8-bit control pads.
 *
 * Revision 1.25  1994/02/18  01:54:55  limes
 * enhanced error reporting
 *
 * Revision 1.24  1994/02/02  01:18:35  limes
 * Add support for print_vinfo
 *
 * Revision 1.23  1994/01/04  00:46:29  dplatt
 * Remove remains of support code for RED chipset and old-style 8-bit
 * control pads.
 * Add sanity-check/defense code to detect pods who say that their
 * input width is less than the size of their header (this can happen
 * if a pod fries itself and simply sends in zeroes in all header fields).
 * Keep such pods from generating an infinitely-replicating chain of ghosts.
 *
 * Revision 1.22  1994/01/03  23:28:42  deborah
 * added comment about state of podDrivers list to main routine
 *
 * Revision 1.21  1993/12/23  22:32:06  deborah
 * Created #defines for different bogosity conditions in pod
 * login and initialization.
 * Also added printing of readersAllocated to DBUG message in AddPortReader.
 *
 * Revision 1.20  1993/12/20  22:41:10  deborah
 * Fixed so compiling with -DDEBUG -DDEBUG2 works again.
 *
 * Revision 1.19  1993/12/16  00:07:33  dplatt
 * Don't bog down quite so badly if a listener gets behind in processing
 * high-repetition-rate events (e.g. data-arrived, fire-tracking).
 *
 * Revision 1.18  1993/11/24  06:57:47  limes
 * Give revision number in greeting message
 *
 * Revision 1.17  1993/09/02  23:21:40  dplatt
 * Devices 0x58-0x7F do not work right, due to accessing the wrong table
 * and settinng the wrong hasROM bit.
 *
 * Revision 1.16  1993/08/30  18:56:04  dplatt
 * Need to re-calculate generic-device numbers if the default
 * driverlet informs us that it has handed over control of a pod to that
 * pod's downloadable/disk-loadable driverlet (which may have set
 * flags which were not set before).
 *
 * Revision 1.15  1993/08/04  00:00:42  dplatt
 * Remove Control Port driver setup (actually, make it conditional).
 *
 * Revision 1.14  1993/07/28  00:25:24  dplatt
 * Make sure that all IOInfo structures are zeroed out.
 *
 * Revision 1.13  1993/07/02  22:39:59  dplatt
 * Report "control port changed" event properly.  Reduce memory usage
 * by freeing up pods which were never really logged in.
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  EventBroker.c - the main top-level code for the Event Broker (a.k.a
  the Input Manager).

*/

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
#include "controlport.h"
#include "poddriver.h"

#ifdef ARMC
#include "stdio.h"
#else
#include <stdlib.h>
#endif

#include "strings.h"
#include "time.h"

#include "sherryvers.h"

#define MASKFLAGS 0xFFFFFF00

typedef struct Listener {
  Node                  li;
  MsgPort              *li_Port;
  Item                  li_PortItem;
  enum ListenerCategory li_Category;
  uint32                li_TriggerMask[8]; /* events to trigger on */
  uint32                li_CaptureMask[8]; /* events to capture */
  uint8                 li_QueueMax;
  uint8                 li_QueueOutstanding;
  uint8                 li_HasFocus;
  uint8                 li_ToldFocus;
  uint8                 li_LostEvents;
  uint8                 li_Tickle;
  uint8                 li_NewlyInitialized;
  uint8                 li_ReportPortChange;
  uint8                 li_SeedCurrentStatus;
  uint8                 li_UsingPodStateTable;
} Listener;

typedef struct EventMsg {
  Node                  em;
  Message              *em_Message;
  Listener             *em_Listener;
} EventMsg;

List listenerList;
List eventMsgsPending;
List eventMsgsAvail;
Item eventBrokerPortItem;
MsgPort *eventBrokerPort;
Listener *focusListener;


Item controlPortDeviceItem;
Item controlPortWriteItem;
Item joyDeviceItem;
Item joyRequestItem;

IOReq *joyRequest;
IOReq *controlPortWrite;

int32 units;
int32 debugFlag;
uint32 controlPortBufferSize;

uint32 currentBufferSize;
uint32 wantBufferSize;

uint32 vblTicks;
struct timeval vblStruct;

List gotPods;
List lostPods;
List podDrivers;
List sharedDrivers;
List readRequests;
Pod *currentSplitter;
ManagedBuffer *cpBuffer;
PodInterface *interfaceStruct;
uint8 nextPodNumber = 0;
uint32 latestVCNT;
int32 blipvert, flipvert;
int32 bufferHoldoff = 5;
int32 portChanged = FALSE;
int32 driverletInitiatedChange = FALSE;
uint32 timestamp;

List *portReaders;
int32 *readersCamped;
int32 *readersAllocated;

PodStateTable *podStateTable;
const uint32 podStateTableEntrySizes[16] = { PODSTATETABLEARRAYSIZES };

MakeTableResponse tableResponse;

#define READER_LIMIT 5

#define EVENT_PAYLOAD_LIMIT 1024

struct {
  EventBrokerHeader ev;
  char              ev_Frame[EVENT_PAYLOAD_LIMIT];
} ev;

/* #defines for bogosity bailout codes in ProcessControlPortInput */
#define	BOGUS_NOBOGUS		0x0		/* no bogosity */
#define	BOGUS_POD_MISMATCH	0x00000001	/* pod id mismatch */
#define	BOGUS_POD_LOGOUT	0x00000002	/* pod logging out */
#define	BOGUS_POD_LOGIN		0x00000004	/* pod logging in */
#define	BOGUS_DEVICE_CHANGE	0x00000008	/* online device change */
#define	BOGUS_POD_ADDED		0x00000010	/* new pod device added */

typedef struct {
  NamelessNode  reader;
  IOReq        *reader_IOReq;
  IOInfo        reader_IOInfo;
  uint32        reader_Busy;
  uint8         reader_Buffer[4];
} PortReader;

typedef struct {
  NamelessNode  request;
  Item          request_Item;
} DeferredMessage;

static Boolean DequeuePendingEvent(Item msgItem);
static void ProcessNewMessage(Item msgItem, int32 isDeferred);
static Listener *GetListener(Item replyPort, int32 canAllocate);
static Message *GetMessageFor(Listener *listener, int32 dataSize);

static void AddStaticDriver(int32 deviceType, Err (*entry)());
static Err CallPodDriver(Pod *thisPod, PodInterface *podInterface);
extern Err ControlPadDriver(PodInterface *pi);
extern Err SplitterDriver(PodInterface *pi);
extern Err EndOfSplitterDriver(PodInterface *pi);
extern Err DefaultDriver(PodInterface *pi);
static int32 GetNextPodNumber(void);
static Pod *LocatePod(int32 podNumber);
static EventFrame *AllocEventFrame(uint32 eventNum,
				   uint32 frameSize,
				   EventFrame **next,
				   void **end);
static Err PackControlBits(uint32 dataBits,
			   uint32 bitCount,
			   uint32 leftJustified,
			   ManagedBuffer *buf,
			   uint32 bufferSegment);
static void AddPortReader(int32 unit, int32 bufferSize);
static void StartPortRead(PortReader *reader);
static void ProcessControlPortInput(PortReader *reader);
static void ProcessHighLevelEvents(PortReader *reader);
static void ConstructControlPortOutput(void);
static int32 ProcessPortInputList(int32 listNumber,
				  void (*processor) (PortReader *));
static void DispatchEvent(EventFrame *frame, uint32 actualBytes,
			  Item originatorItem);
static void SeedListeners(void);
static void MakePodStateTable(void);
static void UpdatePodStateTable(int32 portChanged);
static void DestroyPodStateTable(void);
extern int SuperIsRAMAddr(void *p, int32 len);

#ifdef CREATECONTROLPORT
extern Item createControlPortDevice(void);
#endif


#ifdef	notdef	
extern int32 controlPortHits;
extern int32 controlPortLossage;
#endif	/* notdef */
extern int32 controlPortLine;

extern void print_vinfo(void);

/* #define DEBUG */

#ifdef DEBUG
# define DBUG(x)  printf x
#else
# define DBUG(x) /* x */
#endif

#ifdef DEBUG2
# define DBUG2(x)  printf x
#else
# define DBUG2(x) /* x */
#endif

#ifdef PRODUCTION
# define DBUG0(x) /* x */
# define qprintf(x) /* x */
#else
# define DBUG0(x) if (debugFlag) { printf x ; }
# define qprintf(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) printf x
#endif

int	compatNoShortTransfers;

const struct {
  uint8 in;
  uint8 out;
} BitTable[24] = {
  { 16, 16 },              /* RFU*/
  { 8, 4 },                /* Stereoscopic glasses, output only */  
  { 32, 8 },               /* Gun */  
  { 16, 8 },               /* RFU */  
  { 8, 8 },                /* Gamesaver, off FIXME */  
  { 16, 16 },              /* Gamesaver, read FIXME */  
  { 16, 16 },              /* Gamesaver, write FIXME */  
  { 16, 16 },              /* Gamesaver, reserved FIXME */  
  { 16, 8 },               /* RFU */  
  { 32, 2 },               /* Mouse */  
  { 40, 2 },               /* Air mouse */  
  { 24, 16 },              /* Keyboard */  
  { 32, 2 },               /* Analog joystick */  
  { 40, 8 },               /* Light pen */  
  { 48, 8 },               /* Graphics tablet */  
  { 40, 8 },               /* Steering wheel */  
  { 24, 16 },              /* Diagnostics controller */  
  { 16, 8 },               /* Barcode reader */  
  { 40, 32 },              /* Infra-red transceiver */  
  { 40, 32 },              /* Dual ported interconnection device */  
  { 40, 32 },              /* Inertial controller */  
  { 16, 8 },               /* RFU */
  { 32, 32 },              /* Control Port splitter */
  { 32, 32 },              /* reserved */
};

const struct {
  uint8 in;
  uint8 out;
} FunkyTable[8] = {
  { 0, 0 },                /* unused entry */
  { 0, 0 },                /* unused entry */
  { 0, 0 },                /* unused entry */
  { 16, 8 },               /* IDs 0x58 - 0x5F */
  { 24, 8 },               /* IDs 0x60 - 0x67 */
  { 24, 24 },              /* IDs 0x68 - 0x6F */
  { 32, 32 },              /* IDs 0x70 - 0x77 */
  { 40, 32 },              /* IDs 0x78 - 0x7F */
};

const PodInterface iTemplate = {
  1,
  PD_ParsePodInput,
  NULL,                     /* pod ptr */
  NULL,                     /* buffer management struct ptr */
  NULL,                     /* command-in ptr and len */
  0,
  NULL,                     /* command-out ptr and len */
  0,
  NULL,                     /* next-available-frame */
  NULL,                     /* end-of-frame-area */
  NULL,                     /* trigger event array ptr */
  NULL,                     /* capture event array ptr */
  0,                        /* Hardware VCNT at capture time */
  FALSE,                    /* recovering from lost event? */
  PackControlBits,
  malloc,
  free,
  AllocEventFrame,
};


#ifdef MEMWATCH
void
printmemusage(l)
List *l;
{
	MemList *m;
	for ( m=(MemList *)FIRSTNODE(l);ISNODE(l,m); m = (MemList *)NEXTNODE(m))
	{
	    ulong *p = m->meml_OwnBits;
	    int ones = 0;
	    int32 size;
	    int i =  m->meml_OwnBitsSize;
	    while (i--) ones += CountBits(*p++);
	    size = ones*m->meml_MemHdr->memh_PageSize;
	    kprintf("MemList :0x%lx ",(long)m);
	    kprintf("%s allocated:%d ($%lx) ",m->meml_n.n_Name,
			(int)size,(ulong)size);
	    {
		int32 largest = 0;
		List *ml = m->meml_l;	/* list of freenodes */
		Node *n;
		size = 0;
		for (n = FIRSTNODE(ml); ISNODE(ml,n); n = NEXTNODE(n) )
		{
		    if (largest < n->n_Size) largest = n->n_Size;
		    size += n->n_Size;
		}
		kprintf("unused: %d largest=%d\n",(int)size,(int)largest);
	    }
	}
}
#endif

int main(int argc, char **argv)
{
  Item msgItem;

  uint32 theSignal;

  int32 i;
  Err err;
  int32 argnum;
  int32 writeInProgress, needWrite;
  char *base;
  TagArg portTags[2];
  TagArg ioReqTags[3];

#ifdef DOTIMER
  Item timerDeviceItem;
  Item timerRequestItem;
  IOInfo timerIOInfo;
  IOReq *timerIOReq;
#endif

  IOInfo portIO;
  DeviceStatus status;

  print_vinfo();

  {
    uint32	dov;
    dov = DiscOsVersion(0);
    if (dov < DiscOs_1_3)
      compatNoShortTransfers = 1;
  }

  for (argnum = 1; argnum < argc; argnum++) {
    if (strcmp(argv[argnum], "-d") == 0) {
      debugFlag = 1;
      qprintf(("Debug output enabled\n"));
    } else {
      qprintf(("Unknown option %s\n",argv[argnum]));
    }
  }

#ifdef MEMWATCH
  qprintf(("After launch:\n"));
  printmemusage(CURRENTTASK->t_FreeMemoryLists);
#endif

  if (CURRENTTASK->t.n_Priority < 199) {
    SetItemPri(CURRENTTASK->t.n_Item, 199);
  }

  InitList(&eventMsgsPending, "events pending");
  InitList(&eventMsgsAvail, "events avail");
  InitList(&readRequests, "read requests");
  InitList(&sharedDrivers, "shared driverlets");
  InitList(&listenerList, "listeners");

  portTags[0].ta_Tag = TAG_ITEM_NAME;
  portTags[0].ta_Arg = (void *) EventPortName;
  portTags[1].ta_Tag = TAG_END;
  
  eventBrokerPortItem = CreateItem(MKNODEID(KERNELNODE,MSGPORTNODE), portTags);

  if (eventBrokerPortItem < 0) {
    PrintError(0,"create eventbroker port",0,eventBrokerPortItem);
    return 0;
  }

  eventBrokerPort = (MsgPort *) LookupItem(eventBrokerPortItem);

  DBUG(("Event broker port item 0x%x at 0x%x\n",eventBrokerPortItem,eventBrokerPort));

#ifdef DOTIMER
  timerDeviceItem = OpenNamedDevice("timer", (void *) NULL);

  if (timerDeviceItem < 0) {
    PrintError(0,"open timer",0,timerDeviceItem);
    return 0;
  }

  timerRequestItem = CreateIOReq(NULL, CURRENTTASK->t.n_Priority, timerDeviceItem, 0);

  if (timerRequestItem < 0) {
    PrintError(0,"create IOReq",0,timerRequestItem);
    return 0;
  }

  DBUG(("Timer IOReq item 0x%x\n", timerRequestItem));

  timerIOReq = (IOReq *) LookupItem(timerRequestItem);

#endif

  InitList(&gotPods, "pods seen");
  InitList(&lostPods, "pods lost");
  InitList(&podDrivers, "pod drivers");

  AddStaticDriver(-1, DefaultDriver);
  AddStaticDriver(0xFE, EndOfSplitterDriver);
  AddStaticDriver(0x56, SplitterDriver);
  AddStaticDriver(0x80, ControlPadDriver);
  AddStaticDriver(0xA0, ControlPadDriver);
  AddStaticDriver(0xC0, ControlPadDriver);

  /*
   * At this point, the podDrivers list contains a linked list
   * of podDriver structures. Each is empty except for the
   * pd_DriverEntry and pd_DeviceType fields.
   * The list looks like this:
   *
   * (1st list item:)	PodDriver->pd_DeviceType = 0xc0
   *			PodDriver->pd_DriverEntry = ControlPadDriver
   *			  (rest of elements of this PodDriver are empty)
   *
   * (2nd list item:)	PodDriver->pd_DeviceType = 0xa0
   *			PodDriver->pd_DriverEntry = ControlPadDriver
   *			  (rest of elements of this PodDriver are empty)
   *
   *			. . .
   *    
   * (last list item:)	PodDriver->pd_DeviceType = -1
   *			PodDriver->pd_DriverEntry = DefaultDriver
   *			  (rest of elements of this PodDriver are empty)
   *     
   */

#ifdef DOTIMER
  memset (&timerIOInfo, 0, sizeof timerIOInfo);
  timerIOInfo.ioi_Recv.iob_Buffer = &vblStruct;
  timerIOInfo.ioi_Recv.iob_Len = sizeof vblStruct;
  timerIOInfo.ioi_Command = CMD_READ;
#endif

#ifdef CREATECONTROLPORT
  controlPortDeviceItem = CreateControlPort();

  if (controlPortDeviceItem < 0) {
    PrintError(0,"create control port",0,controlPortDeviceItem);
    return -1;
  } else {
    DBUG(("Control Port device is item 0x%x\n", controlPortDeviceItem));
  }
#else
  controlPortDeviceItem = FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
					"controlport");

  if (controlPortDeviceItem < 0) {
    PrintError(0,"find control port",0,controlPortDeviceItem);
    return -1;
  } else {
    DBUG(("Control Port device is item 0x%x\n", controlPortDeviceItem));
  }
#endif  

  err = OpenItem(controlPortDeviceItem, 0);

  if (err < 0) {
    PrintError(0,"open control port",0,err);
    return -1;
  }

  units =
    (int32) ((Device *) LookupItem(controlPortDeviceItem))->dev_MaxUnitNum + 1;

  portReaders = (List *) AllocMem(units * sizeof (List), MEMTYPE_FILL);
  readersCamped = (int32 *) AllocMem(units * sizeof (int32), MEMTYPE_FILL);
  readersAllocated = (int32 *) AllocMem(units * sizeof (int32), MEMTYPE_FILL);

  for (i = 0; i < units; i++) {
    readersAllocated[i] = 0;
    readersCamped[i] = 0;
    InitList(&portReaders[i], NULL);
    DBUG(("Init reader list for unit %d\n", i));
  }

  ioReqTags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
  ioReqTags[0].ta_Arg = (void *) controlPortDeviceItem;
  ioReqTags[1].ta_Tag = TAG_END;
  
  controlPortWriteItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), ioReqTags);
  
  if (controlPortWriteItem < 0) {
    PrintError(0,"creating IOReq",0,controlPortWriteItem);
    return -1;
  }
  
  controlPortWrite = (IOReq *) LookupItem(controlPortWriteItem);
  
  DBUG(("ControlPort IOReq item 0x%x\n", controlPortWriteItem));

  memset(&portIO, 0, sizeof portIO);

  portIO.ioi_Recv.iob_Buffer = &status;
  portIO.ioi_Recv.iob_Len = sizeof status;
  portIO.ioi_Command = CMD_STATUS;

  err = DoIO(controlPortWriteItem, &portIO);

  if (err < 0 || (err = controlPortWrite->io_Error) < 0) {
    PrintError(0,"get control port status",0,err);
    return -1;
  }

  controlPortBufferSize = status.ds_DeviceBlockSize *
    status.ds_DeviceBlockCount;

  currentBufferSize = 0;
  wantBufferSize = controlPortBufferSize;

  cpBuffer = (ManagedBuffer *) AllocMem(sizeof (ManagedBuffer),
					MEMTYPE_FILL);
  base = (uchar *) AllocMem(2 * controlPortBufferSize, MEMTYPE_FILL);

  if (!cpBuffer) {
    qprintf(("Unable to allocate Control Port buffers:\n"));
    PrintfSysErr(MakeKErr(ER_SEVER,ER_C_STND,ER_NoMem));
    return -1;
  }

  cpBuffer->mb_BufferTotalSize = controlPortBufferSize * 2;
  cpBuffer->mb_BufferSegmentSize = controlPortBufferSize;
  cpBuffer->mb_NumSegments = 3;
  cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBase = base;
  cpBuffer->mb_Segment[MB_FLIPBITS_SEGMENT].bs_SegmentBase = base +
    controlPortBufferSize;

  portIO.ioi_Send.iob_Buffer =
    cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBase;
  portIO.ioi_Send.iob_Len = controlPortBufferSize;
  portIO.ioi_Recv.iob_Buffer = NULL;
  portIO.ioi_Recv.iob_Len = 0;
  portIO.ioi_Command = CMD_WRITE;

  writeInProgress = FALSE;
  
  AddPortReader(0, controlPortBufferSize);

  if (units >= 2) {
    AddPortReader(1, EVENT_PAYLOAD_LIMIT);
  }

  SeedListeners();

#ifdef MEMWATCH
  qprintf(("After full startup:\n"));
  printmemusage(CURRENTTASK->t_FreeMemoryLists);
#endif

  while (TRUE) {

    theSignal = WaitSignal(eventBrokerPort->mp_Signal | SIGF_IODONE);
    if (theSignal & SIGF_IODONE) {
#ifdef DOTIMER
      DBUG(("Reading timer\n"));
      err = DoIO(timerRequestItem, &timerIOInfo);
      DBUG(("Timer has been read.\n"));
      if (err < 0 || (err = timerIOReq->io_Error) < 0) {
	qprintf(("Error reading timer: "));
	PrintfSysErr(err);
	return (int) err;
      }
#endif

      blipvert = FALSE;

      ProcessPortInputList(0, ProcessControlPortInput);
      if (units >= 2) {
	ProcessPortInputList(1, ProcessHighLevelEvents);
      }

      if (writeInProgress) {
	err = CheckIO(controlPortWriteItem);
	if (err != 0) {
	  writeInProgress = FALSE;
	  if (err < 0 || (err = controlPortWrite->io_Error) < 0) {
	    qprintf(("Error writing Control Port: "));
	    PrintfSysErr(err);
	    return 0;
	  }
	}
      }
      if ((blipvert || currentBufferSize != wantBufferSize) &&
	  !writeInProgress) {
	flipvert = FALSE;
	currentBufferSize = wantBufferSize;
	ConstructControlPortOutput();
	/* flipvert was just set FALSE. Why is this next if here? -deborah */
        /** Because ConstructControlPortOutput can set it again!  dcp **/
	if (flipvert) {
	  portIO.ioi_Send.iob_Len = 2 * controlPortBufferSize;
	} else {
	  portIO.ioi_Send.iob_Len = controlPortBufferSize;
	}
	portIO.ioi_CmdOptions = (latestVCNT & 0x0000FFFF) |
	                        (wantBufferSize << 16);
	err = SendIO(controlPortWriteItem, &portIO);
	if (err < 0) {
	  qprintf(("Error starting Control Port write: "));
	  PrintfSysErr(err);
	  return 0;
	}
	writeInProgress = TRUE;
	needWrite = FALSE;
      }
    }
    if (theSignal & eventBrokerPort->mp_Signal) {
      while ((msgItem = GetMsg(eventBrokerPortItem)) > 0) {
	DBUG2(("Broker wakeup!\n"));
	if (!DequeuePendingEvent(msgItem)) {
	  DBUG(("Processing message\n"));
	  ProcessNewMessage(msgItem, FALSE);
	}
      }
      if (msgItem < 0) {
	DBUG(("Broker GetMsg error 0x%x: ", msgItem));
	PrintfSysErr(msgItem);
	return 0;
      }
    }
  }
}

static int32 ProcessPortInputList(int32 listNumber,
				 void (*processor) (PortReader *))
{
  int32 minCampers;
  PortReader *thisReader, *nextReader;
  Err err;
  int32 burstLimit;
  minCampers = readersCamped[listNumber];
  thisReader = (PortReader *) FIRSTNODE(&portReaders[listNumber]);
  burstLimit = debugFlag ? 2 : 10;
  while (IsNode(&portReaders[listNumber], thisReader) && --burstLimit >= 0) {
    nextReader = (PortReader *) NextNode(thisReader);
    if (thisReader->reader_Busy) {
      err = CheckIO(thisReader->reader_IOReq->io.n_Item);
      if (err < 0) {
	qprintf(("Error reading Control Port: "));
	PrintfSysErr(err);
	return err;
      }
      if (err == 0) {
	break;
      }
      minCampers --;
      readersCamped[listNumber] --;
      thisReader->reader_Busy = FALSE;
      RemNode((Node *) thisReader);
      AddTail(&portReaders[listNumber], (Node *) thisReader);
      (*processor) (thisReader);
      StartPortRead(thisReader);
    }
    thisReader = nextReader;
  }
  if (minCampers < 1 && readersAllocated[listNumber] < READER_LIMIT) {
    AddPortReader(listNumber, controlPortBufferSize);
  }
  return 0;
}

static Boolean DequeuePendingEvent(Item msgItem)
{
  EventMsg *pendingEvent;
  Listener *itsListener;
  pendingEvent = (EventMsg *) FirstNode(&eventMsgsPending);
  while (IsNode(&eventMsgsPending,pendingEvent)) {
    if (pendingEvent->em_Message->msg.n_Item == msgItem) {
      DBUG(("Event message 0x%x returned\n", msgItem));
      RemNode((Node *) pendingEvent);
      itsListener = pendingEvent->em_Listener;
      AddHead(&eventMsgsAvail, (Node *) pendingEvent);
      if (itsListener) {
	itsListener->li_QueueOutstanding --;
	DBUG(("Listener queue now %d\n", itsListener->li_QueueOutstanding));
	itsListener->li_Tickle = TRUE;
      } else {
	DBUG(("Orphaned event\n"));
      }
      return TRUE;
    }
    pendingEvent = (EventMsg *) NextNode(pendingEvent);
  }
  DBUG(("Message 0x%x no event-message-pending\n"));
  return FALSE;
}

static Pod *LocatePod(int32 podNumber)
{
  Pod *pod;
  pod = (Pod *) FirstNode(&gotPods);
  while (IsNode(&gotPods,pod)) {
    if (pod->pod_Number == podNumber) {
      return pod;
    }
    pod = (Pod *) NextNode(pod);
  }
  return (Pod *) NULL;
}

static void ProcessNewMessage(Item msgItem, int32 isDeferred)
{
  Message *incoming;
  EventBrokerHeader *header, response;
  ConfigurationRequest *configure;
  Listener *listener;
  int32 i;
  Err err, errorCode;
  void *sendResponseData;
  int32 sendResponseSize, deallocateResponseSize;
  SetFocus *setFocus;
  SendEvent *sendEvent;
  ListenerList *listeners;
  PodDescriptionList *podDescriptionList;
  Pod *pod;
  PodData *podData;
  DeferredMessage *defer;
#define PDR_Size 64
  struct {
    PodData      pr_Base;
    uint8        pd_Buf[PDR_Size];
  } podResponse;
  PodInterface podInterface;
  int32 responseSize, responseAvail;
  errorCode = sendResponseSize = deallocateResponseSize = 0;
  sendResponseData = NULL;
  response.ebh_Flavor = EB_NoOp;
  incoming = (Message *) CheckItem(msgItem, KERNELNODE, MESSAGENODE);
  if (!incoming) {
    return;
  }
  if (!incoming->msg_ReplyPort) {
    return;
  }
  if (!SuperIsRAMAddr(incoming->msg_DataPtr, incoming->msg_DataSize) ||
      (((uint32) incoming->msg_DataPtr) & 0x00000003) != 0 ||
      incoming->msg_DataSize < sizeof (EventBrokerHeader)) {
    ReplyMsg(msgItem, MAKEEB(ER_SEVERE,ER_C_STND,ER_BadPtr), NULL, 0);
    return;
  }
  header = (EventBrokerHeader *) incoming->msg_DataPtr;
  switch (header->ebh_Flavor) {
  case EB_NoOp:
    break;    
  case EB_Configure:
    DBUG0(("Got configure message from port 0x%x\n", incoming->msg_ReplyPort));
    sendResponseData = &response;
    sendResponseSize = sizeof response;
    response.ebh_Flavor = EB_ConfigureReply;
    listener = GetListener(incoming->msg_ReplyPort, TRUE);
    if (!listener) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_SoftErr);
      break;
    }
    configure = (ConfigurationRequest *) incoming->msg_DataPtr;
    listener->li_Category = configure->cr_Category;
/**
  Rewrite what follows.  If we're switching from a non-focus listener to a
  focus listener, seize the focus.  If we switch from focus to non-focus,
  we must relinquish focus.  dcp
**/
    memcpy(listener->li_TriggerMask, configure->cr_TriggerMask,
	   sizeof listener->li_TriggerMask);
    memcpy(listener->li_CaptureMask, configure->cr_CaptureMask,
	   sizeof listener->li_CaptureMask);
    if (configure->cr_QueueMax > 0 &&
	configure->cr_QueueMax <= EVENT_QUEUE_MAX_PERMITTED) {
      listener->li_QueueMax = (uint8) configure->cr_QueueMax;
    }
    if (listener->li_NewlyInitialized) {
      listener->li_NewlyInitialized = FALSE;
      if (listener->li_Category == LC_FocusListener ||
	  listener->li_Category == LC_FocusUI) {
	if (focusListener) {
	  DBUG(("Removing focus from port 0x%x\n",
		 focusListener->li_PortItem));
	  focusListener->li_HasFocus = FALSE;
	}
	RemNode((Node *) listener);
	AddHead(&listenerList, (Node *) listener);
	focusListener = listener;
	focusListener->li_HasFocus = TRUE;
	DBUG(("Newly-established listener on port 0x%x has the focus\n",
	       focusListener->li_PortItem));
      }
    }
    break;
  case EB_SetFocus:
    response.ebh_Flavor = EB_SetFocusReply;
    setFocus = (SetFocus *) header;
    listener = GetListener(setFocus->sf_DesiredFocusListener, FALSE);
    if (listener) {
      if (listener->li_Category == LC_FocusListener ||
	  listener->li_Category == LC_FocusUI) {
	if (focusListener) {
	  focusListener->li_HasFocus = FALSE;
	}
	focusListener = listener;
	focusListener->li_HasFocus = TRUE;
	errorCode = focusListener->li_PortItem;
      } else {
	errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_BadItem);
      }
    } else {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_BadItem);
    }
    break;
  case EB_GetFocus:
    if (focusListener) {
      errorCode = focusListener->li_PortItem;
    } else {
      errorCode = 0;
    }
#ifdef	notdef
    DBUG(("Hit count %d, lossage count %d\n", controlPortHits, controlPortLossage));
#endif /* notdef */
    break;
  case EB_GetListeners:
    responseSize = incoming->msg_DataPtrSize;
    if (responseSize < sizeof (ListenerList) ||
	(listeners = (ListenerList *) AllocMem(responseSize, MEMTYPE_FILL)) == NULL) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
      break;
    }
    listeners->ll_Header.ebh_Flavor = EB_GetListenersReply;
    listeners->ll_Count = 0;
    responseAvail = responseSize - sizeof (ListenerList);
    i = 0;
    listener = (Listener *) FirstNode(&listenerList);
    while (IsNode(&listenerList,listener)) {
      if (responseAvail <= 0) {
	errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
	break;
      }
      i = listeners->ll_Count ++;
      listeners->ll_Listener[i].li_PortItem = listener->li_PortItem;
      listeners->ll_Listener[i].li_Category = listener->li_Category;
      responseAvail -= sizeof (listeners->ll_Listener[0]);
      listener = (Listener *) NextNode(listener);
    }
    sendResponseData = listeners;
    sendResponseSize = deallocateResponseSize = responseSize;
    break;
  case EB_DescribePods:
    /*responseSize = incoming->msg.n_Size - sizeof (Message);*/
    responseSize = incoming->msg_DataPtrSize;
    if (responseSize < sizeof (PodDescriptionList) ||
	(podDescriptionList = (PodDescriptionList *) AllocMem(responseSize, MEMTYPE_FILL)) == NULL) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
      break;
    }
    podDescriptionList->pdl_Header.ebh_Flavor = EB_DescribePodsReply;
    podDescriptionList->pdl_PodCount = 0;
    responseAvail = responseSize - sizeof (PodDescriptionList);
    i = 0;
    pod = (Pod *) FirstNode(&gotPods);
    while (IsNode(&gotPods,pod)) {
      if (responseAvail <= 0) {
	errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
	break;
      }
      i = podDescriptionList->pdl_PodCount ++;
      podDescriptionList->pdl_Pod[i].pod_Number = pod->pod_Number;
      podDescriptionList->pdl_Pod[i].pod_Position = pod->pod_Position;
      podDescriptionList->pdl_Pod[i].pod_Type = pod->pod_Type;
      podDescriptionList->pdl_Pod[i].pod_BitsIn = pod->pod_BitsIn;
      podDescriptionList->pdl_Pod[i].pod_BitsOut = pod->pod_BitsOut;
#ifdef MASKFLAGS
      podDescriptionList->pdl_Pod[i].pod_Flags = pod->pod_Flags & MASKFLAGS;
#else
      podDescriptionList->pdl_Pod[i].pod_Flags = pod->pod_Flags;
#endif
      podDescriptionList->pdl_Pod[i].pod_LockHolder = pod->pod_LockHolder;
      memcpy(podDescriptionList->pdl_Pod[i].pod_GenericNumber,
	     pod->pod_GenericNumber,
	     sizeof pod->pod_GenericNumber);
      responseAvail -= sizeof (podDescriptionList->pdl_Pod[0]);
      pod = (Pod *) NextNode(pod);
    }
    sendResponseData = podDescriptionList;
    sendResponseSize = deallocateResponseSize = responseSize;
    break;
  case EB_ReadPodData:
    if (!isDeferred) {
      defer = (DeferredMessage *) AllocMem(sizeof (DeferredMessage), MEMTYPE_FILL);
      if (!defer) {
	errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NoMem);
	break;
      }
      defer->request_Item = msgItem;
      AddTail(&readRequests, (Node *) defer);
      return;
    }
  case EB_IssuePodCmd:
  case EB_WritePodData:
    /*responseSize = incoming->msg.n_Size - sizeof (Message);*/
    responseSize = incoming->msg_DataPtrSize;
    podData = (PodData *) header;
    pod = LocatePod(podData->pd_PodNumber);
    if (!pod || pod->pod_LoginLogoutPhase != POD_Online) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_BadUnit);
      break;
    }
    podInterface = iTemplate;
    podInterface.pi_Command = PD_ProcessCommand;
    podInterface.pi_Pod = pod;
    podInterface.pi_CommandIn = podData->pd_Data;
    podInterface.pi_CommandInLen = podData->pd_DataByteCount;
    podInterface.pi_CommandOut = podResponse.pr_Base.pd_Data;
    podInterface.pi_CommandOutLen = PDR_Size;
    memcpy(&podResponse.pr_Base, podData, sizeof (PodData));
    podResponse.pr_Base.pd_Header.ebh_Flavor =
      (enum EventBrokerFlavor) (header->ebh_Flavor + 1); /*ugh*/
    errorCode = CallPodDriver(pod, &podInterface);
    podResponse.pr_Base.pd_DataByteCount = podInterface.pi_CommandOutLen;
    if (podResponse.pr_Base.pd_DataByteCount > 0) {
      sendResponseSize = podResponse.pr_Base.pd_DataByteCount +
	sizeof (PodData);
      if (sendResponseSize <= responseSize) {
	sendResponseData = &podResponse;
      }
    }
    break;
  case EB_SendEvent:
    if (!incoming->msg_ReplyPort) {
      break;
    }
    sendEvent = (SendEvent *) header;
    DispatchEvent(&sendEvent->se_FirstFrame,
		  incoming->msg_DataPtrSize - sizeof (EventBrokerHeader),
		  incoming->msg_ReplyPort);
    response.ebh_Flavor = EB_SendEventReply;
    break;
  case EB_MakeTable:
    listener = GetListener(incoming->msg_ReplyPort, TRUE);
    if (!listener) {
      errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_SoftErr);
      break;
    }
    MakePodStateTable();
    tableResponse.mtr_Header.ebh_Flavor = EB_MakeTableReply;
    tableResponse.mtr_PodStateTable = podStateTable;
    sendResponseData = &tableResponse;
    sendResponseSize = sizeof tableResponse;
    listener->li_UsingPodStateTable = TRUE;
    break;
  case EB_RegisterEvent:
  case EB_SendEventReply:
  case EB_CommandReply:
  case EB_EventRecord:
  case EB_EventReply:
  case EB_ConfigureReply:
  case EB_MakeTableReply:
  default:
   DBUG(("Sending reject due to arrival of message %d\n", header->ebh_Flavor));
    errorCode = MAKEEB(ER_SEVERE,ER_C_STND,ER_NotSupported);
  }
  if (!sendResponseData ||
      0 > (err = ReplyMsg(msgItem, errorCode, sendResponseData, sendResponseSize))) {
    ReplyMsg(msgItem, errorCode, NULL, 0);
  }
  if (deallocateResponseSize) {
    FreeMem(sendResponseData, deallocateResponseSize);
  }
  return;
}
    
static Listener *GetListener(Item replyPort, int32 canAllocate)
{
  Listener *listener;
  listener = (Listener *) FirstNode(&listenerList);
  while (IsNode(&listenerList,listener)) {
    if (listener->li_PortItem == replyPort) {
      return listener;
    }
    listener = (Listener *) NextNode((Node *) listener);
  }
  if (!canAllocate) {
    return NULL;
  }
  listener = (Listener *) AllocMem(sizeof(Listener), MEMTYPE_FILL);
  if (!listener) {
    return listener;
  }
  listener->li_PortItem = replyPort;
  listener->li_QueueMax = EVENT_QUEUE_DEFAULT; /* useful default */
  listener->li_NewlyInitialized = TRUE;
#ifdef DOSEEDING
  listener->li_SeedCurrentStatus = TRUE;
#endif
  AddHead(&listenerList, (Node *) listener);
  DBUG(("Broker added event listener at port 0x%x\n", replyPort));
  return listener;
}

int SuperIsRAMAddr(void *p, int32 len)
{
  return 1;
}

static EventFrame *AllocEventFrame(uint32 eventNum,
				   uint32 frameSize,
				   EventFrame **next,
				   void **end)
{
  EventFrame *frame;
  uint32 totalSize;
  frame = *next;
  if (frameSize + 2 * sizeof (EventFrame) >
      ((char *) *end) - ((char *) frame)) {
    DBUG(("Reject building a %d-byte frame, %d left\n", frameSize, ((char *) *end) - ((char *) frame)));
    return NULL;
  }
  totalSize = sizeof (EventFrame) + frameSize - sizeof frame->ef_EventData;
  DBUG(("Build frame, total size %x\n", totalSize));
  memset(frame, 0x00, totalSize);
  frame->ef_ByteCount = totalSize;
  frame->ef_EventNumber = (uint8) eventNum;
  frame->ef_SystemTimeStamp = timestamp;
  *next = (EventFrame *) (totalSize + (char *) frame);
  return frame;
}

static Message *GetMessageFor(Listener *listener, int32 dataSize)
{
  int32 messageSize;
  EventMsg *eventMsg;
  Item msgItem;
  Message *message;
  TagArg msgTags[3];
  if (listener->li_QueueOutstanding >= listener->li_QueueMax) {
    return NULL;
  }
  messageSize = dataSize;
  messageSize = (messageSize + 15) & 0xFFFFFFF0;
  eventMsg = (EventMsg *) FirstNode(&eventMsgsAvail);
  message = NULL;
  while (IsNode(&eventMsgsAvail,eventMsg)) {
    message = eventMsg->em_Message;
    if (message->msg_DataPtrSize >= messageSize) {
      RemNode((Node *) eventMsg);
      break;
    }
    message = NULL;
    eventMsg = (EventMsg *) NextNode((Node *) eventMsg);
  }
  if (!message) {
    DBUG(("Building new message of %d bytes\n", messageSize));
    msgTags[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
    msgTags[0].ta_Arg = (void *) eventBrokerPortItem;
    msgTags[1].ta_Tag = CREATEMSG_TAG_DATA_SIZE;
    msgTags[1].ta_Arg = (void *) messageSize;
    msgTags[2].ta_Tag = TAG_END;
    msgItem = CreateItem(MKNODEID(KERNELNODE,MESSAGENODE), msgTags);
    if (msgItem < 0) {
/* #ifdef DEBUG */
      DBUG(("Can't create event message: "));
      PrintfSysErr(msgItem);
/* #endif */
      return NULL;
    }
    message = (Message *) LookupItem(msgItem);
    eventMsg = (EventMsg *) AllocMem(sizeof (EventMsg), MEMTYPE_FILL);
    if (!eventMsg) {
      DBUG(("EventBroker out of memory!\n"));
      DeleteItem(msgItem);
      return NULL;
    }
    eventMsg->em_Message = message;
    DBUG(("New message item 0x%x at 0x%x\n", message->msg.n_Item, message));
    DBUG(("Data area at 0x%x size %d, flags 0x%x\n", message->msg_DataPtr,
	   message->msg_DataPtrSize, message->msg.n_Flags));
  }
  AddTail(&eventMsgsPending, (Node *) eventMsg);
  eventMsg->em_Listener = listener;
  return message;
}

static void AddStaticDriver(int32 deviceType, Err (*entry)(void))
{
  PodDriver *pd;
  pd = (PodDriver *) AllocMem(sizeof (PodDriver), MEMTYPE_FILL);
  if (!pd) {
    return;
  }
  pd->pd_DriverEntry = entry;
  pd->pd_DeviceType = deviceType;
  AddHead(&podDrivers, (Node *) pd);
}

static Err PackControlBits(uint32 dataBits,
			   uint32 bitCount,
			   uint32 leftJustified,
			   ManagedBuffer *buf,
			   uint32 bufferSegment)
{
/*
  May wish to add a check to avoid walking off beginning of buffer if
  some device gets really piggish about output-buffer space
*/
  BufferSegment *seg;
  uint32 *bufferWord;
  uint32 wordIndex, bitsAvail, bitShift, bitsToChew;
  seg = &buf->mb_Segment[bufferSegment];
  DBUG(("Pack %d bits, %d filled now\n", bitCount, seg->bs_SegmentBitsFilled));
  wordIndex = (buf->mb_BufferSegmentSize / 4) - 1 -
    (seg->bs_SegmentBitsFilled / 32);
  bufferWord = ((uint32 *) seg->bs_SegmentBase) + wordIndex;
  if (leftJustified) {
    dataBits = dataBits >> (32 - bitCount);
  }
  while (bitCount > 0) {
    bitsAvail = 32 - (seg->bs_SegmentBitsFilled % 32);
    bitsToChew = (bitsAvail > bitCount) ? bitCount : bitsAvail;
    DBUG(("Pack %d bits into this word\n", bitsToChew));
    bitShift = 32 - bitsAvail;
    *bufferWord |= (dataBits & ~(((uint32) 0xFFFFFFFF) << bitsToChew)) << bitShift;
    dataBits = dataBits >> bitsToChew;
    seg->bs_SegmentBitsFilled += bitsToChew;
    bitCount -= bitsToChew;
    if (bitsAvail == bitsToChew) {
      bufferWord --;
    }
  }
  return 0;
}

static Err CallPodDriver(Pod *thisPod, PodInterface *podInterface)
{
  return (*thisPod->pod_Driver->pd_DriverEntry)(podInterface, KernelBase);
}

static void KillListener(Listener *listener)
{
  Listener *newListener;
  EventMsg *pendingEvent;
  RemNode((Node *) listener);
  FreeMem(listener, sizeof (Listener));
  pendingEvent = (EventMsg *) FirstNode(&eventMsgsPending);
  while (IsNode(&eventMsgsPending,pendingEvent)) {
    if (pendingEvent->em_Listener == listener) {
      DBUG(("Orphaning an event\n"));
      pendingEvent->em_Listener = NULL;
    }
    pendingEvent = (EventMsg *) NextNode(pendingEvent);
  }
  if (listener == focusListener) {
    DBUG(("Focus listener on port 0x%x has been killed\n",
	   listener->li_PortItem));
    focusListener = NULL;
    newListener = (Listener *) FirstNode(&listenerList);
    while (IsNode(&listenerList,newListener)) {
      if (newListener->li_Category == LC_FocusListener ||
	  newListener->li_Category == LC_FocusUI) {
	focusListener = newListener;
	focusListener->li_HasFocus = TRUE;
	DBUG(("Reestablished port 0x%x as focus listener\n",
	       focusListener->li_PortItem));
	break;
      }
      newListener = (Listener *) NextNode(newListener);
    }
  }
}

/*
  This code isn't working quite right in some transient cases... two pods
  end up logging in with the same number.  To reproduce: start up with
  a glasses controller alone on the bus.  Let it log in.  Disconnect,
  plug it into an old-style pad, connect pad to 3DO port... both
  devices log in as ID 2.
*/

static int32 GetNextPodNumber(void) {
  Pod *thisPod;
  int32 isOK, tries;
  tries = 256;
  do {
    nextPodNumber ++;
    isOK = TRUE;
    thisPod = (Pod *) FirstNode(&gotPods);
    while (isOK && IsNode(&gotPods, thisPod)) {
      if (thisPod->pod_Number == nextPodNumber) {
	isOK = FALSE;
      } else {
	thisPod = (Pod *) NextNode(thisPod);
      }
    }
    thisPod = (Pod *) FirstNode(&lostPods);
    while (isOK && IsNode(&lostPods, thisPod)) {
      if (thisPod->pod_Number == nextPodNumber) {
	isOK = FALSE;
      } else {
	thisPod = (Pod *) NextNode(thisPod);
      }
    }
  } while (!isOK && --tries != 0);
  if (!isOK) {
    if (IsEmptyList(&lostPods)) {
      return -1;
    }
    thisPod = (Pod *) LastNode(&lostPods);
    RemNode((Node *) thisPod);
    nextPodNumber = thisPod->pod_Number;
    FreeMem(thisPod, sizeof (Pod));
  }
  return (int32) nextPodNumber;
}

static Boolean TriggerListener(Listener *listener, uint32 *accumulatedEvents)
{
  if (!CheckItem(listener->li_PortItem, KERNELNODE, MSGPORTNODE)) {
    DBUG(("Listener died\n"));
    KillListener(listener);
    return FALSE;
  }
  if (listener->li_LostEvents &&
      listener->li_QueueOutstanding >= listener->li_QueueMax) {
    return FALSE;
  }
  if (listener->li_Category == LC_NoSeeUm) {
    return FALSE;
  }
  if (listener->li_HasFocus != listener->li_ToldFocus) {
    return TRUE;
  }
  if (listener->li_Category == LC_FocusListener &&
      listener != focusListener) {
    return FALSE;
  }
  if (listener->li_Category == LC_Observer ||
      listener == focusListener) {
    if (listener->li_HasFocus != listener->li_ToldFocus ||
	(listener->li_TriggerMask[0] & accumulatedEvents[0]) ||
	(listener->li_TriggerMask[1] & accumulatedEvents[1]) ||
	(listener->li_TriggerMask[4] & accumulatedEvents[4]) ||
	(listener->li_TriggerMask[5] & accumulatedEvents[5])) {
      return TRUE;
    }
  }
  if (listener->li_LostEvents ||
      (listener->li_ReportPortChange &&
       (listener->li_TriggerMask[2] & EVENTBIT2_ControlPortChange)) ||
      (listener->li_TriggerMask[2] & accumulatedEvents[2]) ||
      (listener->li_TriggerMask[3] & accumulatedEvents[3]) ||
      (listener->li_TriggerMask[6] & accumulatedEvents[6]) ||
      (listener->li_TriggerMask[7] & accumulatedEvents[7])) {
    return TRUE;
  }
  return FALSE;
}

static void DoBoilerplateEvents(Listener *listener, PodInterface *podInterface)
{
  if (listener->li_LostEvents) {
    (void) AllocEventFrame(EVENTNUM_EventQueueOverflow, 0,
			   &podInterface->pi_NextFrame,
			   &podInterface->pi_EndOfFrameArea);
  }
  if (listener->li_ReportPortChange &&
      (listener->li_TriggerMask[2] & EVENTBIT2_ControlPortChange)) {
    (void) AllocEventFrame(EVENTNUM_ControlPortChange, 0,
			   &podInterface->pi_NextFrame,
			   &podInterface->pi_EndOfFrameArea);
  }
  if (listener->li_HasFocus != listener->li_ToldFocus) {
    listener->li_ToldFocus = listener->li_HasFocus;
    if (listener->li_HasFocus) {
      if (listener->li_TriggerMask[0] & EVENTBIT0_GivingFocus) {
	(void) AllocEventFrame(EVENTNUM_GivingFocus, 0,
			       &podInterface->pi_NextFrame,
			       &podInterface->pi_EndOfFrameArea);
      }
    } else {
      if (listener->li_TriggerMask[0] & EVENTBIT0_LosingFocus) {
	(void) AllocEventFrame(EVENTNUM_LosingFocus, 0,
			       &podInterface->pi_NextFrame,
			       &podInterface->pi_EndOfFrameArea);
      }
    }
  }
}

static void SendEventToListener(Listener *listener, void *ev, uint32 eventSize)
{
  Err err;
  Message *message;
  if (listener->li_QueueOutstanding >= listener->li_QueueMax) {
    DBUG(("Listener queue limit, events lost\n"));
    listener->li_LostEvents = TRUE;
    return;
  }
  message = GetMessageFor(listener, eventSize);
  if (message) {
    err = SendMsg(listener->li_PortItem, message->msg.n_Item,
		  ev, eventSize);
    if (err < 0) {
      if (debugFlag) {
	qprintf(("Failed to send event 0x%x to 0x%x:\n", message->msg.n_Item, listener->li_PortItem));
	PrintfSysErr(err);
	qprintf(("Removing listener on port 0x%x\n", listener->li_PortItem));
      }
      KillListener(listener);
      (void) DequeuePendingEvent(message->msg.n_Item);
    } else {
      DBUG(("Sent 0x%x to 0x%x\n", message->msg.n_Item, listener->li_PortItem));
      listener->li_LostEvents = FALSE;
      listener->li_ReportPortChange = FALSE;
      listener->li_SeedCurrentStatus = FALSE;
      listener->li_QueueOutstanding ++;
    }
  } else {
    listener->li_LostEvents = TRUE;
    DBUG(("Listener 0x%x lost event\n", listener->li_PortItem));
  }
}

static void DispatchEvent(EventFrame *firstFrame, uint32 actualBytes,
			  Item originatorItem)
{
  uint32 accumulatedEvents[8];
  EventFrame *thisFrame, *targetFrame;
  Listener *listener, *nextListener;
  PodInterface podInterface;
  uint32 offset, frameSize, eventNumber, eventWord, eventBit;
  int32 eventSize;
/*
   Figure out what events arrived in this frameset of stuff
*/
  memset(accumulatedEvents, 0, sizeof accumulatedEvents);
  offset = 0;
  while (offset < actualBytes) {
    thisFrame = (EventFrame *) (offset + (char *) firstFrame);
    frameSize = thisFrame->ef_ByteCount;
    frameSize = (frameSize + 3) & ~ 3;
    if (frameSize + offset > actualBytes) {
      break;
    }
    eventNumber = thisFrame->ef_EventNumber;
    accumulatedEvents[(eventNumber-1) >> 5] |= 1L << (31 - ((eventNumber-1) & 0x1F));
    offset += frameSize;
  }
/*
  Go through the list of listeners, and send each one any events which
  are appropriate for it.
*/
  listener = (Listener *) FirstNode(&listenerList);
  while (IsNode(&listenerList,listener)) {
    DBUG(("Try listener 0x%x\n", listener));
    nextListener = (Listener *) NextNode((Node *) listener);
    if (!TriggerListener(listener, accumulatedEvents)) {
      listener = nextListener;
      continue;
    }
    ev.ev.ebh_Flavor = EB_EventRecord;
    podInterface.pi_NextFrame = (EventFrame *) &ev.ev_Frame;
    podInterface.pi_EndOfFrameArea = sizeof ev + (char *) &ev;
    DoBoilerplateEvents(listener, &podInterface);
    offset = 0;
    while (offset < actualBytes) {
      thisFrame = (EventFrame *) (offset + (char *) firstFrame);
      frameSize = thisFrame->ef_ByteCount;
      frameSize = (frameSize + 3) & ~ 3;
      if (frameSize + offset > actualBytes) {
	break;
      }
      eventNumber = thisFrame->ef_EventNumber;
      eventWord = (eventNumber - 1) >> 5;
      eventBit = 1L << (31 - ((eventNumber - 1) & 0x1F));
      if (eventBit & (listener->li_TriggerMask[eventWord] |
		      listener->li_CaptureMask[eventWord])) {
	targetFrame = podInterface.pi_NextFrame;
	if ((char *) podInterface.pi_EndOfFrameArea -
	    (char *) targetFrame >= frameSize + 2 * sizeof (EventFrame)) {
	  podInterface.pi_NextFrame =
	    (EventFrame *) (frameSize + (char *) targetFrame);
	  memcpy(targetFrame, thisFrame, frameSize);
	  if (originatorItem != 0) {
	    targetFrame->ef_Submitter = originatorItem;
	  }
	  targetFrame->ef_Trigger =
	    (eventBit & listener->li_TriggerMask[eventWord]) ? 1 : 0;
	  targetFrame->ef_ByteCount = frameSize;
	  if (targetFrame->ef_SystemTimeStamp == 0) {
	    targetFrame->ef_SystemTimeStamp = timestamp;
	  }
	} else {
	  listener->li_LostEvents = TRUE;
	}
      }
      offset += frameSize;
    }
    podInterface.pi_NextFrame->ef_ByteCount = 0;
    podInterface.pi_NextFrame = 
      (EventFrame *) (sizeof (int32) + (char *) podInterface.pi_NextFrame);
    eventSize = ((char *) podInterface.pi_NextFrame) - ((char *) &ev.ev_Frame);
    DBUG(("Event built, %d bytes\n", eventSize));
    if (eventSize > sizeof (int32)) {
      eventSize += sizeof (EventBrokerHeader);
      SendEventToListener(listener, &ev, eventSize);
    }
    listener = nextListener;
  }
  return;
}

static void ProcessHighLevelEvents(PortReader *reader)
{
  DBUG(("Processing high-level event of %d bytes\n", reader->reader_IOReq->io_Actual));
  timestamp = reader->reader_IOReq->io_Info.ioi_Offset;
  DispatchEvent((EventFrame *) reader->reader_IOInfo.ioi_Recv.iob_Buffer,
		reader->reader_IOReq->io_Actual, 0);
  return;
}

static void ProcessControlPortInput(PortReader *reader)
{
  uint32 actual;
  uint8 streamByte, *buffer, topThree, hasROM;
  int32 offset, devOffset, nextDevOffset, inputLength;
  int32 podNumber, podPosition, genericNumbers[16];
  int32 deviceCode;
  int32 inBits, outBits, totalOut, needBytes;
  int32 haltScan, busChanged, i;
  int32 eventSize;
  int32 bogus;
  uint32 found, flags;
  uint32 accumulatedEvents[8];
  uint8 jjj, kkkk;
  uint32 podStateTableInUse;
  Err err;
  Pod *thisPod, *lastPod, *newPod;
  PodDriver *thisDriver;
  PodInterface podInterface;
  Listener *listener, *nextListener;
  DeferredMessage *defer;
  uint32 mergedPodFlags;
  if (bufferHoldoff > 0) {
    -- bufferHoldoff;
    return;
  }
  bogus = BOGUS_NOBOGUS;
  currentSplitter = NULL;
  actual = reader->reader_IOReq->io_Actual;
  DBUG2(("Got %d bytes\n", actual));
  timestamp = reader->reader_IOReq->io_Info.ioi_Offset;
  buffer = cpBuffer->mb_Segment[MB_INPUT_SEGMENT].bs_SegmentBase =
    reader->reader_Buffer;
  podPosition = 0;
  offset = sizeof (uint32); /* skip first word which is garbage */
  if (actual < 6) {
    DBUG(("Empty frame\n"));
    actual = 5;
  }
#ifdef NOTDEF
  if (actual > 8) {
    DBUG(("actual = %d: ", actual));
    offset = 0;
    do {
      DBUG(("%x ", buffer[offset]));
      offset ++;
    } while (offset < actual);
    DBUG(("\n"));
  }
#endif
  thisPod = (Pod *) FirstNode(&gotPods);
  haltScan = FALSE;
  totalOut = 0;
  busChanged = driverletInitiatedChange;
  portChanged |= driverletInitiatedChange;
  driverletInitiatedChange = FALSE;
  offset = sizeof (uint32); /* skip first word which is garbage */
  while (offset <= actual && !haltScan) { /* assume we got at least one pullup byte */
    podPosition ++;
    devOffset = offset;
    streamByte = buffer[offset++];
    topThree = streamByte & POD_CONTROL_PAD_MASK;
    hasROM = FALSE;
    if (streamByte == PODID_SPLITTER_END_CHAIN_1 ||
	streamByte == PODID_END_CHAIN) {
      deviceCode = streamByte;
      inBits = 8;
      outBits = 2;
    } else if (topThree == PODID_3DO_BASIC_CONTROL_PAD) {
      deviceCode = topThree;
      inBits = 16;
      outBits = 2;
    } else if (topThree == PODID_3DO_EXTD_CONTROL_PAD) {
      deviceCode = topThree;
      inBits = 16;
      outBits = 4;
    } else if (topThree == PODID_3DO_SILLY_CONTROL_PAD) {
      deviceCode = topThree;
      inBits = 24;
      outBits = 8;
    } else if ((streamByte & 0xC0) == 0x40) {
      deviceCode = streamByte;
      if (deviceCode <= 0x57) {
	inBits = BitTable[streamByte & 0x3F].in;
	outBits = BitTable[streamByte & 0x3F].out;
      } else {
	hasROM = (uint8) deviceCode & 0x04;
	inBits = FunkyTable[(deviceCode >> 3) & 0x07].in;
	outBits = FunkyTable[(deviceCode >> 3) & 0x07].out;
      }
    } else {
      deviceCode = streamByte;
      streamByte = buffer[offset++];
      if (deviceCode == 0x00) {
	deviceCode = ((uint32) buffer[offset++]) << 8;
      }
      hasROM = streamByte & POD_ROMFUL_MASK_BIT;
      jjj = (streamByte >> 4) & 0x07;
      kkkk = streamByte & 0x0F;
      if (jjj == 0x7) {
	jjj = buffer[offset++];
      }
      inBits = ((int32) jjj + 1) * 8;
      if (kkkk == 0x0F) {
	outBits = ((int32) buffer[offset++] + 1) * 8;
      } else {
	outBits = ((int32) kkkk + 1) * 2;
      }
    }
    DBUG(("Device code 0x%x at offset %d\n", deviceCode, devOffset));
    DBUG(("Input %d bits, output %d bits\n", inBits, outBits));
    nextDevOffset = devOffset + (inBits >> 3);
    if (nextDevOffset < offset) {
/*
   Whoops!  We got a pod whose input length is smaller than the
   amount of data we've already chewed through.  This can occur if a
   pod becomes ill and sends in a string of zeros.  In this case, halt
   scanning 'cause we can't trust the data which follows.  Don't force
   bogosity - let this device log in if it can, so we can diagnose the
   problem.
*/
      haltScan = TRUE;
    } else {
      offset = nextDevOffset;
    }
    totalOut += outBits;
    if (IsNode(&gotPods,thisPod)) {
      if (thisPod->pod_Type != deviceCode) {
/*
  Pod mismatch - the data on this frame is inconsistent with the pod
  we thought was out there, or, we've hit the pullup code.  

  Start or progress with disconnection of
  this pod and everything downstream.  Terminate scan of the port data
  during this cycle.
*/
	bogus |= BOGUS_POD_MISMATCH;
	thisPod->pod_LoginLogoutPhase = POD_LoggingOut;
	DBUG(("Start logout of type 0x%x due to seeing 0x%x at offset %d\n",
	       thisPod->pod_Type, deviceCode, devOffset));
	if (++ thisPod->pod_LoginLogoutTimer > POD_LoginLogoutDelay) {
	  qprintf(("Time %d logged out pod number %d type 0x%x position %d offset %d\n",
		 vblTicks, thisPod->pod_Number, thisPod->pod_Type, podPosition,
		 devOffset));
	  busChanged = TRUE;
	  portChanged = TRUE;
	  if (currentSplitter) {
	    currentSplitter->pod_PrivateData[0] = 0; /** force reset **/
	    DBUG(("Device logged off arm 1, reconfigure splitter\n"));
	  }
	  while (TRUE) {
	    lastPod = (Pod *) LastNode(&gotPods);
	    RemNode((Node *) lastPod);
	    /** Send the pod driver a farewell kiss **/
	    if (lastPod->pod_Driver) {
	      podInterface = iTemplate;
	      podInterface.pi_Command = PD_TeardownPod;
	      podInterface.pi_Pod = lastPod;
	      (void) CallPodDriver(lastPod, &podInterface);
	      thisDriver = lastPod->pod_Driver;
	      if (thisDriver->pd_Flags & PD_LOADED_INTO_RAM) {
		thisDriver->pd_UseCount --;
		if (thisDriver->pd_UseCount == 0 &&
		    !(thisDriver->pd_Flags & PD_SHARED)) {
		  DBUG(("Release RAM-loaded driverlet\n"));
		  podInterface.pi_Command = PD_ShutdownDriver;
		  (void) CallPodDriver(lastPod, &podInterface);
		  free(thisDriver->pd_DriverArea);
		  free(thisDriver);
		}
	      }
	      lastPod->pod_Driver = (PodDriver *) NULL;
	    }
	    if (lastPod->pod_SuccessfulLogin) {
	      AddHead(&lostPods, (Node *) lastPod);
	    } else {
	      FreeMem(lastPod, sizeof (Pod));
	    }
	    if (lastPod == thisPod) {
	      break;
	    }
	  }
	}
	haltScan = TRUE; /* force exit */
      } else {
	/*
	  Same kind we saw last time.  If pod was logging in or out during the
	  last cycle, move it towards online status and (if it finishes logging
	  in) give it a wakeup kiss.  In either case stop scanning the port for
	  this cycle.
	  */
	switch (thisPod->pod_LoginLogoutPhase) {
	case POD_LoggingOut:
	  DBUG(("Pod at position %d logout continues\n", podPosition));
	  if (-- thisPod->pod_LoginLogoutTimer <= 0) {
	    DBUG(("Pod at position %d going back online\n", podPosition));
	    thisPod->pod_LoginLogoutPhase = POD_Online;
	  }
	  haltScan = TRUE; /* force exit */
	  bogus |= BOGUS_POD_LOGOUT;
	  break;
	case POD_LoggingIn:
	  DBUG(("Pod at position %d login continues\n", podPosition));
	  if (-- thisPod->pod_LoginLogoutTimer <= 0) {
	    thisPod->pod_LoginLogoutPhase = POD_LoadDriver;
	    DBUG(("Pod at position %d offset %d is now online\n",
	      podPosition, devOffset));
	  }
	  haltScan = TRUE; /* force exit */
	  bogus |= BOGUS_POD_LOGIN;
	  break;
	case POD_LoadDriver:
	  DBUG(("Here we load the driver for pod %d\n", podPosition));
	  thisPod->pod_LoginLogoutPhase = POD_Online;
	  haltScan = TRUE; /* force exit */
	  busChanged = TRUE;
	  portChanged = TRUE;
	  qprintf(("Time %d logged in pod number %d type 0x%x position %d offset %d\n",
		 vblTicks, thisPod->pod_Number, deviceCode, podPosition,
		 devOffset));
	  thisDriver = (PodDriver *) FirstNode(&podDrivers);
	  while (IsNode(&podDrivers,thisDriver)) {
	    if (thisDriver->pd_DeviceType == -1 ||
		thisDriver->pd_DeviceType == deviceCode) {
	      thisPod->pod_Driver = thisDriver;
	      break;
	    }
	    thisDriver = (PodDriver *) NextNode(thisDriver);
	  }
	  if (thisPod->pod_Driver) {
	    podInterface = iTemplate;
	    if (thisPod->pod_SuccessfulLogin) {
	      podInterface.pi_Command = PD_ReconnectPod;
	    } else {
	      podInterface.pi_Command = PD_InitPod;
	    }
	    podInterface.pi_Pod = thisPod;
	    if ((err = CallPodDriver(thisPod, &podInterface)) < 0) {
	      thisPod->pod_LoginLogoutPhase = POD_InitFailure;
	      DBUG(("Pod init failed: "));
	      PrintfSysErr(err);
	    } else {
	      thisPod->pod_SuccessfulLogin = TRUE;
	    }
	  } else {
	    DBUG(("No driver available for this pod\n"));
	  }
#ifdef MEMWATCH
	  qprintf(("After pod login:\n"));
	  printmemusage(CURRENTTASK->t_FreeMemoryLists);
#endif
	  break;
	case POD_Online:
	  /* 
	    Update insie, outsie, position values in case the device or somebody
	    upstream changed things
	    */
	  if (currentSplitter) {
	    if (thisPod->pod_BitsIn != inBits ||
		thisPod->pod_BitsOut != outBits ||
		thisPod->pod_InputByteOffset != devOffset) {
	      DBUG(("Device change, reset splitter\n"));
	      currentSplitter->pod_PrivateData[0] = 0; /** force reset **/
	      bogus |= BOGUS_DEVICE_CHANGE;
	      haltScan = TRUE;
	    }
	    if (deviceCode == 0xFE) {
	      currentSplitter = NULL; /* hit air, end of arm 1 */
	    } else if (deviceCode == PODID_3DO_SPLITTER ||
		       deviceCode == PODID_3DO_SPLITTER_2) {
	      currentSplitter->pod_PrivateData[0] = 0; /** force reset **/
	      DBUG(("Second splitter on arm 1, reconfigure\n"));
	      haltScan = TRUE; /* splitter chained on arm 1, illegal, stop */
	    }
	  } else {
	    if (deviceCode == PODID_3DO_SPLITTER ||
		deviceCode == PODID_3DO_SPLITTER_2) {
	      currentSplitter = thisPod;
	    }
	  }
	  thisPod->pod_BitsIn = inBits;
	  thisPod->pod_BitsOut = outBits;
	  thisPod->pod_InputByteOffset = devOffset;
	  break;
	case POD_InitFailure:
	  break;
	}
      }
      thisPod = (Pod *) NextNode(thisPod);
    } else {
      if (streamByte == PODID_END_CHAIN) {
	break; /*** end of input stream ***/
      }
/* 
  Something has been added to the daisychain.  Search the lost-pods list
  for a pod of this type and (if one is found) steal it - the new device
  acquires the identity of the old/lost one.  If no such pod can be found,
  create a new one.  In either case, terminate the scan through the port
  during this cycle.
*/
      if (currentSplitter && deviceCode != PODID_SPLITTER_END_CHAIN_1) {
	currentSplitter->pod_PrivateData[0] = 0; /** force reset **/
	DBUG(("New device added to arm 1, reconfigure splitter\n"));
      }
      found = FALSE;
      bogus |= BOGUS_POD_ADDED;
      newPod = (Pod *) FirstNode(&lostPods);
      while (IsNode(&lostPods,newPod)) {
	if (newPod->pod_Type == deviceCode) {
	  found = TRUE;
	  break;
	}
	newPod = (Pod *) NextNode(newPod);
      }
      if (found) {
	DBUG(("Recycling pod number %d\n", newPod->pod_Number));
	RemNode((Node *) newPod);
      } else {
	podNumber = GetNextPodNumber();
	if (podNumber < 0) {
	  DBUG(("Can't get new pod number\n"));
	  goto bailout;
	}
	DBUG(("Got new pod number %d\n", podNumber));
	newPod = (Pod *) AllocMem(sizeof (Pod), MEMTYPE_FILL);
	if (!newPod) {
	  DBUG(("Cannot allocate memory space for new pod\n"));
	  goto bailout;
	}
	newPod->pod_Type = deviceCode;
	newPod->pod_Number = (uint8) podNumber;
	DBUG(("New pod allocated\n"));
      }
      AddTail(&gotPods, (Node *) newPod);
      newPod->pod_LoginLogoutPhase = POD_LoggingIn;
      newPod->pod_LoginLogoutTimer = POD_LoginLogoutDelay;
      newPod->pod_BitsIn = inBits;
      newPod->pod_BitsOut = outBits;
      newPod->pod_InputByteOffset = devOffset;
      DBUG(("Pot at offset %d set for login\n", devOffset));
      haltScan = TRUE;
    }
  }
 bailout:
  DBUG(("Final offset was %d\n", offset));
  inputLength = offset;
  if (bogus) {
    DBUG(("Bogosity occurred, code 0x%x\n", bogus));
/*
   If there was anything even slightly amiss, force the Control Port to
   its maximum frame length.  This will help keep the input clean.
*/
    wantBufferSize = controlPortBufferSize;
    return;
  }
/*
  Tell all the pods to parse their input and record the identities
  of the events they wish to generate this time around.
*/
  podInterface.pi_Version = 1;
  podInterface.pi_Command = PD_ParsePodInput;
  latestVCNT = reader->reader_IOReq->io_Info.ioi_CmdOptions; /*ick*/
  podInterface.pi_VCNT = latestVCNT;
  haltScan = FALSE;
  if (busChanged) {
    memset(genericNumbers, 0, sizeof genericNumbers);
    podPosition = 0;
  }
  memset(accumulatedEvents, 0, sizeof accumulatedEvents);

  if (compatNoShortTransfers)
    mergedPodFlags = 0;
  else
    mergedPodFlags = POD_ShortFramesOK + POD_MultipleFramesOK + POD_FilteringOK;

  thisPod = (Pod *) FirstNode(&gotPods);
  while (IsNode(&gotPods,thisPod)) {
    flags = thisPod->pod_Flags;
    mergedPodFlags &= flags;
    if (busChanged) {
      thisPod->pod_Position = (uint8) ++ podPosition;
      i = 0;
      do {
	if (flags & 0x80000000) {
	  thisPod->pod_GenericNumber[i] = (uint8) ++ genericNumbers[i];
	  DBUG0(("Pod position %d is generic %d of class %d\n", podPosition, thisPod->pod_GenericNumber[i], i));
	} else {
	  thisPod->pod_GenericNumber[i] = 0;
	}
	flags = flags << 1;
      } while (++i < 16);
    }
    switch (thisPod->pod_LoginLogoutPhase) {
    case POD_InitFailure:
      break;
    case POD_Online:
      if (thisPod->pod_Driver) {
	podInterface = iTemplate;
	podInterface.pi_Command = PD_ParsePodInput;
	podInterface.pi_ControlPortBuffers = cpBuffer;
	podInterface.pi_Pod = thisPod;
	thisPod->pod_EventsReady[0] =
	  thisPod->pod_EventsReady[1] =
	    thisPod->pod_EventsReady[2] =
	      thisPod->pod_EventsReady[3] =
		thisPod->pod_EventsReady[4] =
		  thisPod->pod_EventsReady[5] =
		    thisPod->pod_EventsReady[6] =
		      thisPod->pod_EventsReady[7] = 0;
	if ((err = CallPodDriver(thisPod, &podInterface)) < 0) {
	  thisPod->pod_LoginLogoutPhase = POD_InitFailure;
	  DBUG(("Pod parse failed: "));
	  PrintfSysErr(err);
	}
	accumulatedEvents[0] |= thisPod->pod_EventsReady[0];
	accumulatedEvents[1] |= thisPod->pod_EventsReady[1];
	accumulatedEvents[2] |= thisPod->pod_EventsReady[2];
	accumulatedEvents[3] |= thisPod->pod_EventsReady[3];
	accumulatedEvents[4] |= thisPod->pod_EventsReady[4];
	accumulatedEvents[5] |= thisPod->pod_EventsReady[5];
	accumulatedEvents[6] |= thisPod->pod_EventsReady[6];
	accumulatedEvents[7] |= thisPod->pod_EventsReady[7];
      }
      break;
    default:
      haltScan = TRUE;
    }
    blipvert |= thisPod->pod_Blipvert;
    thisPod->pod_Blipvert = FALSE;
    thisPod = (Pod *) NextNode((Node *) thisPod);
  }
/*
  Go through the list of listeners, and send each one any events which
  are appropriate for it.  Keep track of whether anybody is still using
  the in-memory event table.
*/
  listener = (Listener *) FirstNode(&listenerList);
  podStateTableInUse = FALSE;
  while (IsNode(&listenerList,listener)) {
    DBUG(("Try listener 0x%x\n", listener));
    podStateTableInUse |= listener->li_UsingPodStateTable;
    nextListener = (Listener *) NextNode((Node *) listener);
    listener->li_ReportPortChange |= (uint8) portChanged;
    if (!TriggerListener(listener, accumulatedEvents)) {
      listener = nextListener;
      continue;
    }
    ev.ev.ebh_Flavor = EB_EventRecord;
    podInterface = iTemplate;
    podInterface.pi_Command = PD_AppendEventFrames;
    podInterface.pi_VCNT = reader->reader_IOReq->io_Flags;
    podInterface.pi_ControlPortBuffers = cpBuffer;
    podInterface.pi_NextFrame = (EventFrame *) &ev.ev_Frame;
    podInterface.pi_EndOfFrameArea = sizeof ev + (char *) &ev;
    podInterface.pi_TriggerMask = listener->li_TriggerMask;
    podInterface.pi_CaptureMask = listener->li_CaptureMask;
    podInterface.pi_RecoverFromLostEvents = listener->li_LostEvents |
      listener->li_SeedCurrentStatus;
    DoBoilerplateEvents(listener, &podInterface);
    thisPod = (Pod *) FirstNode(&gotPods);
    while (IsNode(&gotPods,thisPod)) {
      if (thisPod->pod_LoginLogoutPhase == POD_Online && thisPod->pod_Driver) {
	podInterface.pi_Pod = thisPod;
	if ((err = CallPodDriver(thisPod, &podInterface)) < 0) {
	  DBUG(("Pod event-build failed: "));
	  PrintfSysErr(err);
	}
      }
      thisPod = (Pod *) NextNode(thisPod);
    }
    podInterface.pi_NextFrame->ef_ByteCount = 0;
    podInterface.pi_NextFrame = 
      (EventFrame *) (sizeof (int32) + (char *) podInterface.pi_NextFrame);
    eventSize = ((char *) podInterface.pi_NextFrame) - ((char *) &ev.ev_Frame);
    DBUG(("Event built, %d bytes\n", eventSize));
    if (eventSize > sizeof (int32)) {
      eventSize += sizeof (EventBrokerHeader);
      SendEventToListener(listener, &ev, eventSize);
    }
    listener = nextListener;
  }
/*
   Update the in-memory event table if present and still needed.
*/
  if (podStateTable) {
    if (podStateTableInUse) {
      UpdatePodStateTable(portChanged);
    } else {
      DestroyPodStateTable();
    }
  }
/*
  Process any ReadPodData messages which had been queued up since
  the last time we came through here.  We have a stable buffer of
  data, newly-received;  the driverlets can use it.
*/
  while (!IsEmptyList(&readRequests)) {
    defer = (DeferredMessage *) RemHead(&readRequests);
    ProcessNewMessage(defer->request_Item, TRUE);
    FreeMem(defer, sizeof (DeferredMessage));
  }
  portChanged = FALSE;
/*
   Calculate the length of the next Control Port input frame.
   Be conservative (use maximal length) if there were any changes to
   the bus this time around, or if any pod on the bus vetos the use
   of short frames.

   If short frames are used:  take the longer of the input and output
   byte counts.  Add 4 bytes to account for the sludge word at the beginning
   of the input buffer.  Add 5 bytes to allow for the maximum-length header
   of a newly connected pod.  Round up to a longword.  Add an extra word
   just to be safe.
*/

  if (!busChanged && (mergedPodFlags & POD_ShortFramesOK) != 0) {
    totalOut = (totalOut + 7) >> 3;
    needBytes = (inputLength > totalOut) ? inputLength : totalOut;
    needBytes = ((needBytes + (4+5+3)) & ~ 3) + 4;
    if (needBytes > controlPortBufferSize) {
      needBytes = controlPortBufferSize;
    }
    wantBufferSize = needBytes;
  } else {
    wantBufferSize = controlPortBufferSize;
  }
  return;
}

static void ConstructControlPortOutput()
{
  uint32 consumed;
  Err err;
  Pod *thisPod;
  PodInterface podInterface;
  memset(cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBase, 0,
	 cpBuffer->mb_BufferSegmentSize);
  memset(cpBuffer->mb_Segment[MB_FLIPBITS_SEGMENT].bs_SegmentBase, 0,
	 cpBuffer->mb_BufferSegmentSize);
  cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBitsFilled =
    cpBuffer->mb_Segment[MB_FLIPBITS_SEGMENT].bs_SegmentBitsFilled =
      (controlPortBufferSize - currentBufferSize) * 8;
  podInterface = iTemplate;
  podInterface.pi_Command = PD_ConstructPodOutput;
  podInterface.pi_VCNT = latestVCNT;
  podInterface.pi_ControlPortBuffers = cpBuffer;
  thisPod = (Pod *) FirstNode(&gotPods);
  while (IsNode(&gotPods,thisPod)) {
    if (thisPod->pod_Driver) {
      podInterface.pi_Pod = thisPod;
      DBUG(("Build output for pod %d\n", thisPod->pod_Number));
      consumed = cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBitsFilled;
      if ((err = CallPodDriver(thisPod, &podInterface)) < 0) {
	DBUG(("Pod output-build failed: "));
	PrintfSysErr(err);
      }
      cpBuffer->mb_Segment[MB_OUTPUT_SEGMENT].bs_SegmentBitsFilled = 
	cpBuffer->mb_Segment[MB_FLIPBITS_SEGMENT].bs_SegmentBitsFilled =
	  consumed + thisPod->pod_BitsOut; /* Don't assume driverlets do it! */
    }
    flipvert |= thisPod->pod_Flipvert;
    thisPod->pod_Flipvert = FALSE;
    thisPod = (Pod *) NextNode(thisPod);
  }
  return;
}

static void AddPortReader(int32 unit, int32 bufferSize)
{
  PortReader *newReader;
  Item ioReqItem;
  newReader = (PortReader *) AllocMem(sizeof (PortReader) + bufferSize,
				      MEMTYPE_FILL);
  if (!newReader) {
    return;
  }
  ioReqItem = CreateIOReq(NULL, 200, controlPortDeviceItem, 0);
  if (ioReqItem < 0) {
    PrintfSysErr(ioReqItem);
    FreeMem(newReader, sizeof (PortReader) + bufferSize);
    return;
  }
  newReader->reader_IOReq = (IOReq *) LookupItem(ioReqItem);
  newReader->reader_IOInfo.ioi_Recv.iob_Buffer = newReader->reader_Buffer;
  newReader->reader_IOInfo.ioi_Recv.iob_Len = bufferSize;
  newReader->reader_IOInfo.ioi_Command = CMD_READ;
  newReader->reader_IOInfo.ioi_Unit = (uint8) unit;
  AddTail(&portReaders[unit], (Node *) newReader);
  readersAllocated[unit] ++;
  DBUG(("Added port reader %d for unit %d\n", readersAllocated[unit], unit));
  StartPortRead(newReader);
  return;
}

static void StartPortRead(PortReader *reader)
{
  Err err;
  DBUG2(("Issuing read, buffer 0x%x, %d bytes\n",
	 reader->reader_IOInfo.ioi_Recv.iob_Buffer,
	 reader->reader_IOInfo.ioi_Recv.iob_Len));
  err = SendIO(reader->reader_IOReq->io.n_Item,
	       &reader->reader_IOInfo);
  DBUG2(("Read initiated\n"));
  if (err < 0) {
    PrintfSysErr(err);
  } else {
    reader->reader_Busy = TRUE;
    readersCamped[reader->reader_IOInfo.ioi_Unit] ++;
  }
}

static void SeedListeners(void)
{
  Item automounterItem;
  Listener *automounter;

  automounterItem = FindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),
				  "Automount");

  if (automounterItem < 0) {
    return;
  }

  automounter = GetListener(automounterItem, TRUE);

  if (!automounter) {
    return;
  }

  automounter->li_Category = LC_Observer;
  automounter->li_TriggerMask[2] = EVENTBIT2_DeviceOnline |
                                   EVENTBIT2_DeviceOffline;
  automounter->li_QueueMax = EVENT_QUEUE_MAX_PERMITTED;
  automounter->li_NewlyInitialized = FALSE;

  DBUG(("Automounter link OK\n"));
}

uint32 CreateTableEntry(uint32 **stampPtr, void **structPtr, uint32 structSize,
			uint8 maxGeneric) {
  int32 stampSize, i;
  uint32 *stamp;
  void *structure;
  i = (uint32) maxGeneric + 8;
  stampSize = i * sizeof (uint32 *);
  structSize *= i;
  stamp = (uint32 *) AllocMem(stampSize, MEMTYPE_FILL);
  structure = (void *) AllocMem(structSize, MEMTYPE_FILL);
  if (stamp && structure) {
    *stampPtr = stamp;
    *structPtr = structure;
    return i;
  }
  if (stamp) {
    FreeMem((void *) stamp, stampSize);
  }
  if (structure) {
    FreeMem(structure, structSize);
  }
  return 0;
}
    
static void PrepPodStateTable(PodStateTable *pst)
{
  Pod *pod;
  PodStateTableOverlay *psto;
  uint8 maxGeneric;
  uint32 i;
  psto = (PodStateTableOverlay *) pst;
  for (i = 0; i < 16; i++) {
    if (podStateTableEntrySizes[i] > 0 &&
	psto->psto_Array[i].gt_HowMany == 0) {
      maxGeneric = 0;
      pod = (Pod *) FirstNode(&gotPods);
      while (IsNode(&gotPods,pod)) {
	if (pod->pod_GenericNumber[i] > maxGeneric) {
	  maxGeneric = pod->pod_GenericNumber[i];
	}
	pod = (Pod *) NextNode(pod);
      }
      if (maxGeneric > 0) {
	qprintf(("Prepping event table generic class %d\n", i));
	psto->psto_Array[i].gt_HowMany =
	  CreateTableEntry(&psto->psto_Array[i].gt_ValidityTimeStamps,
			   (void **) &psto->psto_Array[i].gt_EventSpecificData,
			   podStateTableEntrySizes[i], maxGeneric);
      }
    }
  }
}

static void MakePodStateTable(void)
{
  PodStateTable *pst;
  if (podStateTable) {
    return;
  }
  qprintf(("Initializing in-memory pod state table\n"));
  pst = (PodStateTable *) AllocMem(sizeof (PodStateTable), MEMTYPE_FILL);
  if (!pst) {
    return;
  }
  pst->pst_SemaphoreItem = CreateSemaphore("PodStateTable", 0);
  PrepPodStateTable(pst);
  podStateTable = pst;
}

static void DestroyPodStateTable(void)
{
  int i;
  PodStateTableOverlay *psto;
  if (podStateTable) {
    qprintf(("Destroying pod-state table\n"));
    psto = (PodStateTableOverlay *) podStateTable;
    for (i = 0 ; i < 16 ; i++ ) {
      if (psto->psto_Array[i].gt_HowMany > 0) {
	qprintf(("Destroying generic %d\n", i));
 	FreeMem(psto->psto_Array[i].gt_ValidityTimeStamps,
		psto->psto_Array[i].gt_HowMany * sizeof (uint32));
	FreeMem(psto->psto_Array[i].gt_EventSpecificData,
		psto->psto_Array[i].gt_HowMany * podStateTableEntrySizes[i]);
      }
    }
    if (podStateTable->pst_SemaphoreItem) {
      DeleteItem(podStateTable->pst_SemaphoreItem);
    }
    FreeMem(podStateTable, sizeof *podStateTable);
    podStateTable = NULL;
  }
}

static void UpdatePodStateTable(int32 portChanged)
{
  Pod *pod;
  PodInterface podInterface;
  Err err;
  if (podStateTable->pst_SemaphoreItem) {
    LockSemaphore(podStateTable->pst_SemaphoreItem, TRUE);
  }
  if (portChanged) {
    PrepPodStateTable(podStateTable);
  }
  podInterface = iTemplate;
  podInterface.pi_Command = PD_UpdatePodStateTable;
  podInterface.pi_VCNT = latestVCNT;
  podInterface.pi_PodStateTable = podStateTable;
  pod = (Pod *) FirstNode(&gotPods);
  while (IsNode(&gotPods,pod)) {
    podInterface.pi_Pod = pod;
    if ((err = CallPodDriver(pod, &podInterface)) < 0) {
      DBUG(("Pod table update failed: "));
      PrintfSysErr(err);
    }
    pod = (Pod *) NextNode(pod);
  }
  if (podStateTable->pst_SemaphoreItem) {
    UnlockSemaphore(podStateTable->pst_SemaphoreItem);
  }
}

