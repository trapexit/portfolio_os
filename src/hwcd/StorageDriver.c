/* $Id: StorageDriver.c,v 1.2 1994/09/17 02:29:59 stan Exp $ */
/*
    File:       StorageDriver.c

    Contains:   Driver implementation for the 3DO Card Reader with local RAM.
		Implicit assumptions for this driver:
		    - there will be local RAM on the card reader
		    - there will be a lot on the card reader
		If any of these assumptions are invalid, bad things will happen.

    Written by: Tim Nichols

    Copyright:  (c) 1994 by The 3DO Company, all rights reserved.

    Change History (most recent first):

	 <4>     9/15/94    stan    Cleaned up for RCS and nightly build
	<3+>      9/9/94    tcn     get ram size from rom.
	 <3>      9/9/94    tcn     get card size from depot config registers
	 <2>      9/7/94    tcn     LED support, ReadDepotRegister works correctly, added timeout
				    routines but haven't turned them on yet.
	 <1>     8/31/94    tcn     first checked in

    To Do:
	- read ROM to get RAM size
	- timeout support (?)
*/

#include "device.h"
#include "driver.h"
#include "io.h"
#include "mem.h"
#include "operror.h"
#include "debug.h"
#include "string.h"
#include "interrupts.h"
#include "kernel.h"
#include "super.h"
#include "stdio.h"
#include "semaphore.h"
#include "time.h"
#include "timer.h"

/* ////////////////////////////////////////////////////////////////////////// */
/* /////////		    defines			 ///////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#define HACK_SIZES

#ifdef DEBUG
    #define KDBUG(x)	    kprintf("Storage: "); kprintf x 
    #define SDBUG(x)	    Superkprintf("Storage: ");  Superkprintf x
#else
    #define KDBUG(x)
    #define SDBUG(x)
#endif

#define DAEMON_STACKSIZE	0x200
#define FAIL_IF_ERROR(x,str)    if((x)<0) { SDBUG(str); return(x); }

#define DI_STORAGE	      0x0A	/* FIXME this belongs in driver.h */
#define DS_DEVTYPE_STORAGE      0x00	/* FIXME this belongs in driver.h */

#define DEPOT_MANU_ID	   0xFFFF      /* depot id from ReadID cmd */
#define DEPOT_MANU_DEV_NUM      0x0002

#define CARD_DETECT_BIT	 (1)	 /* status byte bit 0 */
#define UNKNOWN_CMD_BIT	 (1<<1)      /* status byte bit 1 */
#define ERROR_BIT	       (1<<4)      /* status byte bit 2 */

#define DEPOT_ERROR	     -1234;      /* depot hardware error code */

#define TIME_OUT_SECS	   5	   /* time out in seconds */

#define NO_CALLBACK	     0	   /* flags for xbus read/write */
#define USE_CALLBACK	    1

#define CMD_BUSY		(1)	 /* internal state flags */
#define DATA_BUSY	       (1<<1)
#define TIMED_OUT	       (1<<2)

#define DEPOT_READ_XFER	 0x20	/* depot cmds */
#define DEPOT_WRITE_XFER	0x21
#define DEPOT_ABORT	     0x24

#define DEPOT_CARD_BASE	 0x04000000  /* depot base addresses */
#define DEPOT_REGISTER_BASE     0x0C000000
#define DEPOT_ROM_BASE	  0x10000000
#define DEPOT_RAM_BASE	  0x10800000

enum { 
	/* offsets from register base */
	DEPOT_GPIO_DIRECTION = 0,
	DEPOT_GPIO_VALUE,
	DEPOT_GPIO_INT_ENABLE,
	DEPOT_GPIO_INT_POLARITY,
	DEPOT_GPIO_CAPTURE_CNTL,
	DEPOT_GPIO_CAPTURE_CLR,
	DEPOT_TIMING_HOLD,
	DEPOT_CYCLE_TIME1,
	DEPOT_CYCLE_TIME2,
	DEPOT_SLOT_INPUTS,
	DEPOT_SLOT_INPUTS_INT,
	DEPOT_FUNC_CONTROL,
	DEPOT_READ_CHUNK_SIZE,
	DEPOT_WRITE_CHUNK_SIZE,
	DEPOT_ID,
	DEPOT_REVISION,
	DEPOT_MODE,
	DEPOT_VISA_CONFIG1,
	DEPOT_VISA_CONFIG2,
	DEPOT_VISA_CONFIG3,
	DEPOT_VISA_CONFIG4,
	DEPOT_NUM_REGISTERS
};


/* ////////////////////////////////////////////////////////////////////////// */
/* /////////		    prototypes		      ///////////// */
/* ////////////////////////////////////////////////////////////////////////// */

Item    CreateStorageDevice(void);
Item    DeviceInit(Device* dev);
Item    DeviceOpen(Device* dev);
Item    DeviceClose(Device* dev);
Err     CmdWrite(IOReq* theReq);
Err     CmdRead(IOReq* theReq);
Err     CmdStatus(IOReq* theReq);
Err     CmdLightOn(IOReq* theReq);
Err     CmdLightOff(IOReq* theReq);
void    MyAbortIO(IOReq *theReq);
void    StartDaemon(void);
Err     Daemon(void);
IOReq*  CallBack(IOReq* myior);
void    DriverInit(Item xItem);
Err     HandleReq(IOReq* theReq);
Err     XBusStatus(IOReq* xReq,void* xstat,uint8 unit);
Err     XBusCommand(uint32* cmdBuffer,uint32 cmdCount,uint32 statCount,uint8 useCallBack);
Err     XBusWrite(IOBuf* theBuf,uint8 useCallBack);
Err     XBusRead(IOBuf* theBuf,uint8 useCallBack);
Err     DepotInit(void);
Err     WriteDepotRegister(int32 whichRegister,uint8 value);
Err     ReadDepotRegister(int32 whichRegister,uint8* value);
Err     StartTimer(uint32 time);
Err     StopTimer(void);

/* ////////////////////////////////////////////////////////////////////////// */
/* /////////		    types			   ///////////// */
/* ////////////////////////////////////////////////////////////////////////// */

typedef struct DriverData {
    IOReq*  workingReq;     /* current client ior depot is processing */
    List    waitingReqs;    /* client reqs waiting for depot to free up */
    List    returnedReqs;   /* our reqs that have returned from xbus */
    uint32  cmdBuffer[2];   /* for commands to depot */
    uint8   statBuffer[12]; /* for status from commands to depot */
    uint32  statCount;      /* number of status byte returned */
    IOReq*  xbusCmdReq;     /* for sending commands to depot via xbus */
    IOReq*  xbusDataReq;    /* for sending/receiving data to/from depot via xbus */
    IOReq*  timerReq;       /* for timeouts */
    Task*   daemon;	 /* ptr to io daemon task */
    uint32  daemonSig;      /* for signaling the daemon */
    uint32  ramSize;	/* size of ram in bytes */
    uint32  cardSize;       /* size of card in bytes */
    uint32  byteCount;      /* size of client read/write xfer */
    uint32  state;	  /* state info for driver */
    uint32  error;	  /* error from xbus reqs */
} DriverData;

/* ////////////////////////////////////////////////////////////////////////// */
/* /////////		    globals			 ///////////// */
/* ////////////////////////////////////////////////////////////////////////// */

DriverData      gData;
uint8	   gDaemonStack[DAEMON_STACKSIZE];
struct timeval  gTimeVal;
uint8	   gTimerActive;

static Err (*gCmdTable[])() =
{
    CmdWrite,
    CmdRead,
    CmdStatus,
    CmdLightOn,
    CmdLightOff,
};

/*  tags for creation of driver */
static TagArg gDriverTags[] =   
{
    TAG_ITEM_NAME,	      "Storage",
    TAG_ITEM_PRI,	       (void *)1,      /* this is ignored */
    CREATEDRIVER_TAG_ABORTIO,   (void *)(long)MyAbortIO,
    CREATEDRIVER_TAG_MAXCMDS,   (void *)(sizeof(gCmdTable) / sizeof(Err (*)(IOReq*))),
    CREATEDRIVER_TAG_CMDTABLE,  (void *)gCmdTable,
    TAG_END,		    0,
};

/*  tags for creation of device */
static TagArg gDeviceTags[] =
{
    TAG_ITEM_NAME,	  "Storage",
    TAG_ITEM_PRI,	   (void *)1,	  /* this is ignored */
    CREATEDEVICE_TAG_DRVR,  (void *)1,	  /* this gets filled in later */
    CREATEDEVICE_TAG_INIT,  (void *)(long)DeviceInit,
    CREATEDEVICE_TAG_OPEN,  (void *)(long)DeviceOpen,
    CREATEDEVICE_TAG_CLOSE, (void *)(long)DeviceClose,
    TAG_END,		0,
};

/*  used for creation of daemon task */
static TagArg gDaemonTags[]=
{
    TAG_ITEM_NAME,		  "Storage Daemon",
    TAG_ITEM_PRI,		   (void*)220,
    CREATETASK_TAG_PC,	      (void*)((long)StartDaemon),
    CREATETASK_TAG_SP,	      (void*)(gDaemonStack + DAEMON_STACKSIZE),
    CREATETASK_TAG_STACKSIZE,       (void*)DAEMON_STACKSIZE,
    CREATETASK_TAG_SUPER,	   (void*)TRUE,
    TAG_END,			0
};

static uint32 gVisaPartSizeTable[16] = {  0x800,0x1000,0x2000,0x4000,
					 0x8000,0x10000,0x20000,0x40000,
					 0x80000,0x100000,0x200000,0x400000, 
					 0x80000,0x1000000,0x2000000,0x4000000};
					

/* ////////////////////////////////////////////////////////////////////////// */
/* /////////		    code			    ///////////// */
/* ////////////////////////////////////////////////////////////////////////// */

int main(int32 argc,char *argv[])
{
    Item server;
    
    server = CreateStorageDevice();
    if(server<0)
    {
	kprintf("Storage: CreateDevice failed - ");
	PrintfSysErr(server);
	return -1;
    }
    WaitSignal(0);
}

Item CreateStorageDevice(void)
{
    Item devItem = 0;
    Item drvrItem;

    drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),gDriverTags);
    KDBUG(("driver item = %d\n",drvrItem));
    if (drvrItem >= 0)
    {
	gDeviceTags[2].ta_Arg = (void *)drvrItem;
	devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),gDeviceTags);
	KDBUG(("device item = %d\n",devItem));
	if (devItem < 0)
	    DeleteItem(drvrItem);
    }
    return devItem;
}

Item DeviceInit(Device* dev)
{
    Item    daemonItem;
    Item    xbusItem;
    Item    xItem;
    Item    iorItem;
    IOReq*  xReq;
    Err     err;
    struct XBusDeviceStatus xstat;
    
    daemonItem = SuperCreateItem(MKNODEID(KERNELNODE, TASKNODE), gDaemonTags);
    FAIL_IF_ERROR(daemonItem, ("SuperCreateItem for server daemon failed\n"));
    
    xbusItem = SuperFindNamedItem(MKNODEID(KERNELNODE, DEVICENODE),"xbus");
    FAIL_IF_ERROR( xbusItem, ("Error finding xbus device!\n") );

    xItem = SuperOpenItem( xbusItem , 0 );
    FAIL_IF_ERROR( xItem, ("Error opening xbus device!\n") );

    iorItem = SuperCreateIOReq(0,128,xItem,0);
    FAIL_IF_ERROR( iorItem, ("Error creating xbus ior!\n") );

    xReq = (IOReq*)LookupItem(iorItem);
    err = XBusStatus(xReq,(void *)&xstat,1);
    if(err >= 0)
    {
	SuperWaitIO(iorItem);
	if(!xReq->io_Error)
	{
	    if(xstat.xbus_ManuIdNum == DEPOT_MANU_ID && xstat.xbus_ManuDevNum == DEPOT_MANU_DEV_NUM)
	    {
		DriverInit(xItem);
	    }
	    else
	    {
		SDBUG(("Depot not found at xbus unit 1\n"));
		SuperDeleteItem(iorItem);
		SuperCloseItem(xItem);
		return NOSUPPORT;
	    }
	}
    }
    SuperDeleteItem(iorItem); /* we're done with this item */
    dev->dev_MaxUnitNum = 1;
    return dev->dev.n_Item;
}

void DriverInit(Item xItem)
{
    Item iorItem;
    Item timerItem;
    Item tItem;
    
    memset(&gData,0,sizeof(gData)); /* clear everything first */
    
    InitList(&gData.waitingReqs,"Waiting Reqs");
    InitList(&gData.returnedReqs,"Returned Reqs");

    gData.workingReq = 0;
    
    /* create cmd req for xbus driver */
    iorItem = SuperCreateIOReq(0,128,xItem,0);
    if(iorItem < 0)
    {
	SDBUG(("Error creating xbus cmd ior!\n"));
	return;
    }
    gData.xbusCmdReq = (IOReq*)LookupItem(iorItem);
    
    /* create data req for xbus driver */
    iorItem = SuperCreateIOReq(0,128,xItem,0);
    if(iorItem < 0)
    {
	SDBUG(("Error creating xbus data ior!\n"));
	return;
    }
    gData.xbusDataReq = (IOReq*)LookupItem(iorItem);
    
    /* create req for timer */
    timerItem = SuperFindNamedItem(MKNODEID(KERNELNODE, DEVICENODE),"timer");
    if(timerItem < 0)
    {
	SDBUG(("Error finding timer!\n"));
	return;
    }
    tItem = SuperOpenItem( timerItem , 0 );
    if(tItem < 0)
    {
	SDBUG(("Error opening timer!\n"));
	return;
    }
    iorItem = SuperCreateIOReq(0,128,tItem,0);
    if(iorItem < 0)
    {
	SDBUG(("Error creating timer ior!\n"));
	return;
    }
    gData.timerReq = (IOReq*)LookupItem(iorItem);
    
    gTimerActive = 0;

    DepotInit();

#ifdef HACK_SIZES   
    gData.ramSize = 0x8000;
    gData.cardSize = 0x8000;
#endif
}

Item DeviceOpen(Device* dev)
{
    return dev->dev.n_Item;
}

Item DeviceClose(Device* dev)
{
    return dev->dev.n_Item;
}

Err CmdWrite(IOReq* theReq)
{
    Err     err;
    uint32  oldInts;
    IOReq*  ior;
    
    err = 0;
    if(gData.workingReq) {
	SDBUG(("CmdWrite - adding req to waiting list\n"));
	/*  the device is busy, queue this req for later */
	oldInts = Disable();
	AddTail(&gData.waitingReqs,(Node*)theReq);
	Enable(oldInts);
    } else {
	/*  the device is not busy, go ahead and do the i/o */
	SDBUG(("CmdWrite - processing req\n"));
	gData.workingReq = theReq;
	err = HandleReq(theReq);
	if(err != 0)
	{
	    SDBUG(("CmdWrite - error in HandleReq, calling SuperCompleteIO\n"));
	    ior = gData.workingReq;
	    ior->io_Error = err;
	    SuperCompleteIO(ior);
	    gData.workingReq = 0;
	}
    }
    theReq->io_Flags &= ~IO_QUICK;
    return err;
}

Err CmdRead(IOReq* theReq)
{
    Err     err;
    uint32  oldInts;
    IOReq*  ior;
    
    err = 0;
    if(gData.workingReq) {
	SDBUG(("CmdRead - adding req to waiting list\n"));
	/*  the device is busy, queue this req for later */
	oldInts = Disable();
	AddTail(&gData.waitingReqs,(Node*)theReq);
	Enable(oldInts);
    } else {
	/*  the device is not busy, go ahead and do the i/o */
	gData.workingReq = theReq;
	err = HandleReq(theReq);
	if(err != 0)
	{
	    SDBUG(("CmdRead - error in HandleReq, calling SuperCompleteIO\n"));
	    ior = gData.workingReq;
	    ior->io_Error = err;
	    SuperCompleteIO(ior);
	    gData.workingReq = 0;
	}
    }
    theReq->io_Flags &= ~IO_QUICK;
    return err;
}

Err CmdStatus(IOReq* theReq)
{
    DeviceStatus *dst;
    DeviceStatus stat;
    int32 len;

    
    dst = (DeviceStatus *)theReq->io_Info.ioi_Recv.iob_Buffer;
    len = theReq->io_Info.ioi_Recv.iob_Len;
    
    SDBUG(("CmdStatus dst=%lx len=%d unit=%d\n",dst,len,theReq->io_Info.ioi_Unit));

    if (len <= 0)
    {
	theReq->io_Error = BADIOARG;
	return 1;
    }

    memset(&stat,0,sizeof(DeviceStatus));

    stat.ds_DriverIdentity = DI_STORAGE;
    stat.ds_FamilyCode = DS_DEVTYPE_STORAGE;
    stat.ds_MaximumStatusSize = sizeof(stat);
    stat.ds_DeviceBlockSize = 1;
    switch(theReq->io_Info.ioi_Unit)
    {
	case 0:
	    stat.ds_DeviceBlockCount = gData.ramSize;
	    break;
	case 1:
	    stat.ds_DeviceBlockCount = gData.cardSize;
	    break;
	default:
	    SDBUG(("CmdStatus bogus unit %ld\n",theReq->io_Info.ioi_Unit));
	    break;
    }
    stat.ds_DeviceUsageFlags = DS_USAGE_FILESYSTEM;

    if (len > sizeof(DeviceStatus)) 
	len = sizeof(DeviceStatus);
	
    memcpy(dst,&stat,len);
    theReq->io_Actual = len;

    return 1;
}

Err CmdLightOn(IOReq* theReq)
{
    WriteDepotRegister(DEPOT_GPIO_VALUE,0x02);
    return 1;
}

Err CmdLightOff(IOReq* theReq)
{
    WriteDepotRegister(DEPOT_GPIO_VALUE,0x00);
    return 1;
}

/* this runs with interrupts turned off */
void MyAbortIO(IOReq *theReq)
{
    if(theReq == gData.workingReq)
    {
	/* the ior to be aborted is currently active. */
    }
    else
    {
	/* the ior to be aborted is in the waiting list so remove it, */
	/*  set the error code and complete the io */
	RemNode((Node*)theReq);
	theReq->io_Error = ABORTED;
	SuperCompleteIO(theReq);
    }
}

/* this runs with interrupts turned off */
IOReq* CallBack(IOReq* myior)
{
    uint8*  buf;
    uint32  len;

    if(myior == gData.xbusDataReq) {
	if(myior->io_Info.ioi_Command == CMD_WRITE)
	    len = myior->io_Info.ioi_Send.iob_Len;
	else
	    len = myior->io_Info.ioi_Recv.iob_Len;

	/* we send data in 16 byte chunks */
	gData.byteCount -= len;
	if(gData.byteCount > 0) {
	    if(myior->io_Info.ioi_Command == CMD_WRITE) {
		buf = (uint8*)myior->io_Info.ioi_Send.iob_Buffer;
		myior->io_Info.ioi_Send.iob_Buffer = buf + myior->io_Info.ioi_Send.iob_Len;
		myior->io_Info.ioi_Send.iob_Len = (gData.byteCount > 16 ? 16 : gData.byteCount);
	    } else {
		buf = (uint8*)myior->io_Info.ioi_Recv.iob_Buffer;
		myior->io_Info.ioi_Recv.iob_Buffer = buf + myior->io_Info.ioi_Recv.iob_Len;
		myior->io_Info.ioi_Recv.iob_Len = (gData.byteCount > 16 ? 16 : gData.byteCount);
	    }
	    return myior;
	}
    }
    AddTail(&gData.returnedReqs,(Node*)myior);
    SuperInternalSignal(gData.daemon, gData.daemonSig);
    return 0;
}

void StartDaemon(void)
{
    /* launch the daemon in supervisor mode, and don't come back */
    CallBackSuper(Daemon, 0, 0, 0);
    KDBUG(("PROBLEM! Daemon didn't start up!\n"));
}

Err Daemon(void)
{
    uint32 sig;
    uint32 oldInts;
    IOReq*  myReq;
    IOReq*  newReq;
    IOReq*  oldReq;
    
    gData.daemon = CURRENTTASK; /*  CallBack will signal us when io has finished */
    gData.daemonSig = SuperAllocSignal(0);

    while(1) {
	SDBUG(("Daemon waiting for signal...\n\n"));
	sig = SuperWaitSignal(gData.daemonSig);

	while(1) {
	    oldInts = Disable();
	    myReq = (IOReq*)RemHead(&gData.returnedReqs);
	    Enable(oldInts);
	    
	    if(myReq == 0)
		break;
	    
	    if(myReq == gData.xbusDataReq) 
	    {
		SDBUG(("Daemon got back xbusDataReq.\n"));
		gData.state &= ~DATA_BUSY;
		if(gData.xbusDataReq->io_Error != 0) {
		    SDBUG(("Daemon got a software error in xbusDataReq. err = %ld\n",gData.xbusDataReq->io_Error));
		    gData.error = gData.xbusDataReq->io_Error;
		}   
	    }  
	    else if(myReq == gData.xbusCmdReq)
	    {
		SDBUG(("Daemon got back xbusCmdReq.\n"));
		gData.state &= ~CMD_BUSY;
		if(gData.xbusCmdReq->io_Error != 0) {
		    SDBUG(("Daemon got a software error in xbusCmdReq. err = %ld\n",gData.xbusCmdReq->io_Error));
		    gData.error = gData.xbusCmdReq->io_Error;
		}   
		if(gData.statBuffer[gData.statCount-1] & ERROR_BIT) {
		    SDBUG(("Daemon got a hardware error in xbusCmdReq. sb = %lx\n",gData.statBuffer[gData.statCount-1]));
		    gData.error = DEPOT_ERROR;
		}
	    }
	    else
	    {
		SDBUG(("Daemon got back unknown req.\n"));
	    }
	    
	    if((gData.state & (CMD_BUSY|DATA_BUSY)) == 0)
	    {
		/* 
		    if there is a workingReq, complete the io
		    and start up another req if one is waiting.
		    if there are no working reqs, then the io
		    was initiated internally and we signal the
		    driver to tell it is done
		*/
		if(gData.workingReq) {
		    SDBUG(("Daemon completing workingReq.\n"));
		    oldReq = gData.workingReq;
		    gData.workingReq = 0;
		    oldReq->io_Error = gData.error;
		    SuperCompleteIO(oldReq);
		    oldInts = Disable();
		    newReq = (IOReq *)RemHead(&gData.waitingReqs);
		    Enable(oldInts);
		    if(newReq)
		    {
			SDBUG(("Daemon starting up another io\n"));
			(*gCmdTable[newReq->io_Info.ioi_Command])(newReq);
		    }
		}
		else {
		    SDBUG(("Daemon thinks io is done, but there is no working req!.\n"));
		}
	    }
	}
    }
    return 0;
}

Err DepotInit(void)
{
    Err     err;
    uint8   value;
    uint8   limit;
    uint32  pop;
    uint32  size;

    value = 0;
    err = WriteDepotRegister(DEPOT_FUNC_CONTROL,0x44);
    err = WriteDepotRegister(DEPOT_GPIO_DIRECTION,0x02);
    err = ReadDepotRegister(DEPOT_VISA_CONFIG1,&value);
    if(err == 0) {
	SDBUG(("value of DEPOT_VISA_CONFIG1 = %x\n",value));
	if(value == 0 || value == 0xFF) {
	    SDBUG(("no visa found\n"));
	    gData.cardSize = 0;
	}
	else {
	    limit = (value >> 4) & 0x0F;
	    SDBUG(("VISA limit = %x\n",limit));
	    err = ReadDepotRegister(DEPOT_VISA_CONFIG2,&value);
	    SDBUG(("value of DEPOT_VISA_CONFIG2 = %x\n",value));
	    size = (uint32)value & 0x0F;
	    pop = 1 + (((uint32)value >> 4) & 0x07);
	    SDBUG(("pop=%lx, size=%lx\n",pop,size));
	    gData.cardSize = pop*gVisaPartSizeTable[size];
	}
	SDBUG(("card holds %lx bytes\n",gData.cardSize));
    }
    SDBUG(("Error from DepotInit = %lx\n",err));
    return err;
}

/* write a 1 byte value to a depot register */
Err WriteDepotRegister(int32 whichRegister,uint8 value)
{
    Err     err;
    uint32  address;
    uint32  *buf;
    IOBuf   iob;
    uint8   registerBuffer;
    
    address = DEPOT_REGISTER_BASE + whichRegister;
    buf = gData.cmdBuffer;
    buf[0] = (DEPOT_WRITE_XFER << 24) | (address >> 8);
    buf[1] = (address << 24) | (1 << 8);
    registerBuffer = value;
    iob.iob_Buffer = &registerBuffer;
    iob.iob_Len = 1;
    err = XBusWrite(&iob,NO_CALLBACK);
    if(err == 0)
    {
	err = XBusCommand(buf,7,2,NO_CALLBACK); 
	if(err == 0) {
	    SuperWaitIO(gData.xbusDataReq->io.n_Item);
	    gData.state &= ~DATA_BUSY;
	    SuperWaitIO(gData.xbusCmdReq->io.n_Item);
	    gData.state &= ~CMD_BUSY;
	    if(gData.xbusCmdReq->io_Error || gData.xbusDataReq->io_Error) {
		SDBUG(("software error from register write\n"));
		err = gData.xbusCmdReq->io_Error | gData.xbusDataReq->io_Error;
	    }
	    if(gData.statBuffer[gData.statCount-1] & ERROR_BIT) {
		SDBUG(("hardware error from register write\n"));
		err = gData.statBuffer[gData.statCount-1];
	    }
	}
    }
    return err;
}

/* Read a 1 byte value from a depot register */
Err ReadDepotRegister(int32 whichRegister,uint8 *value)
{
    Err     err;
    uint32  address;
    uint32  *buf;
    IOBuf   iob;
    
    address = DEPOT_REGISTER_BASE + whichRegister;
    buf = gData.cmdBuffer;
    buf[0] = (DEPOT_READ_XFER << 24) | (address >> 8);
    buf[1] = (address << 24) | (1 << 8);
    iob.iob_Buffer = value;
    iob.iob_Len = 1;
    err = XBusRead(&iob,NO_CALLBACK);
    if(err == 0)
    {
	err = XBusCommand(buf,7,2,NO_CALLBACK); 
	if(err == 0) {
	    SuperWaitIO(gData.xbusCmdReq->io.n_Item);
	    gData.state &= ~CMD_BUSY;
	    SuperWaitIO(gData.xbusDataReq->io.n_Item);
	    gData.state &= ~DATA_BUSY;
	    if(gData.xbusCmdReq->io_Error || gData.xbusDataReq->io_Error) {
		SDBUG(("software error from register read\n"));
		err = gData.xbusCmdReq->io_Error | gData.xbusDataReq->io_Error;
	    }
	    if(gData.statBuffer[gData.statCount-1] & ERROR_BIT) {
		SDBUG(("hardware error from register read\n"));
		err = gData.statBuffer[gData.statCount-1];
	    }
	}
    }
    return err;
}

/* send client IOReq to depot via xbus driver */
Err HandleReq(IOReq* theReq)
{
    Err     err;
    uint32  address;
    uint32  len;
    uint32  *buf;
    uint32  depotOffset = DEPOT_RAM_BASE;
    uint32  cmd;
    
    err = 0;
    switch(theReq->io_Info.ioi_Unit)
    {
	case 0:
	    depotOffset = DEPOT_RAM_BASE;
	    break;
	case 1:
	    depotOffset = DEPOT_CARD_BASE;
	    break;
    }
    address = theReq->io_Info.ioi_Offset + depotOffset;
    if(theReq->io_Info.ioi_Command == CMD_WRITE) {
	len = theReq->io_Info.ioi_Send.iob_Len;
	cmd = DEPOT_WRITE_XFER;
     } else {
	len = theReq->io_Info.ioi_Recv.iob_Len;
	cmd = DEPOT_READ_XFER;
    }
    buf = gData.cmdBuffer;
    gData.byteCount = len;
    SDBUG(("HandleReq - address = %lx, len = %d\n",address,len));

    /*  stuff the command bytes */
    buf[0] = (cmd << 24) | (address >> 8);
    buf[1] = (address << 24) | ( ( len & 0xFFFF ) << 8);
    err = XBusCommand(buf,7,2,USE_CALLBACK);    
    if(err == 0)
    {
	if(cmd == DEPOT_WRITE_XFER)
	    err = XBusWrite(&theReq->io_Info.ioi_Send,USE_CALLBACK);
	else
	    err = XBusRead(&theReq->io_Info.ioi_Recv,USE_CALLBACK);
    }
    
    return err;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* /////////		    xbus routines		   ///////////// */
/* ////////////////////////////////////////////////////////////////////////// */

Err XBusStatus(IOReq* xReq,void* xstat,uint8 unit)
{
    memset(&xReq->io_Info,0,sizeof(IOInfo));
    xReq->io_Info.ioi_Command = CMD_STATUS;
    xReq->io_Info.ioi_Recv.iob_Buffer = xstat;
    xReq->io_Info.ioi_Recv.iob_Len = sizeof(struct XBusDeviceStatus);
    xReq->io_Info.ioi_Unit = unit;
    return SuperInternalSendIO(xReq);
}

Err XBusCommand(uint32* cmdBuffer,uint32 cmdCount,uint32 statCount,uint8 useCallBack)
{
    IOReq* xior;
    
    gData.state |= CMD_BUSY;
    xior = gData.xbusCmdReq;
    memset(&xior->io_Info,0,sizeof(IOInfo));
    if(useCallBack)
	xior->io_CallBack = CallBack;
    xior->io_Info.ioi_Command = XBUSCMD_Command;
    xior->io_Info.ioi_Unit = 1;
    xior->io_Info.ioi_Send.iob_Buffer = cmdBuffer;
    xior->io_Info.ioi_Send.iob_Len = cmdCount;
    xior->io_Info.ioi_Recv.iob_Buffer = gData.statBuffer;
    xior->io_Info.ioi_Recv.iob_Len = statCount;
    gData.statCount = statCount;
    return SuperInternalSendIO(xior);
}

Err XBusWrite(IOBuf* theBuf,uint8 useCallBack)
{
    IOReq* xior;
    uint32 len;
    
    gData.state |= DATA_BUSY;

    xior = gData.xbusDataReq;
    memset(&xior->io_Info,0,sizeof(IOInfo));
    if(useCallBack)
	xior->io_CallBack = CallBack;
    xior->io_Info.ioi_Command = CMD_WRITE;
    xior->io_Info.ioi_Unit = 1;
    xior->io_Info.ioi_Send.iob_Buffer = theBuf->iob_Buffer;
    len = (theBuf->iob_Len > 16 ? 16 : theBuf->iob_Len);
    xior->io_Info.ioi_Send.iob_Len = len;
    return SuperInternalSendIO(xior);
}

Err XBusRead(IOBuf* theBuf,uint8 useCallBack)
{
    IOReq* xior;
    int32 len;
    
    gData.state |= DATA_BUSY;

    xior = gData.xbusDataReq;
    memset(&xior->io_Info,0,sizeof(IOInfo));
    if(useCallBack)
	xior->io_CallBack = CallBack;
    xior->io_Info.ioi_Command = CMD_READ;
    xior->io_Info.ioi_Unit = 1;
    xior->io_Info.ioi_Recv.iob_Buffer = theBuf->iob_Buffer;
    len = (theBuf->iob_Len > 16 ? 16 : theBuf->iob_Len);
    xior->io_Info.ioi_Recv.iob_Len = len;
    return SuperInternalSendIO(xior);
}

/* ////////////////////////////////////////////////////////////////////////// */
/* /////////		    timer routines		  ///////////// */
/* ////////////////////////////////////////////////////////////////////////// */

Err StartTimer(uint32 time)
{
    IOReq* tior;
    
    SDBUG(("StartTimer delay=%ld secs.\n",time));

    gTimerActive = 1;
    
    gTimeVal.tv_sec = time;
    gTimeVal.tv_usec = 0;

    tior = gData.timerReq;
    memset(&tior->io_Info,0,sizeof(IOInfo));
    tior->io_Info.ioi_Command = TIMERCMD_DELAY;
    tior->io_Info.ioi_Unit = TIMER_UNIT_USEC;
    tior->io_Info.ioi_Send.iob_Buffer = &gTimeVal;
    tior->io_Info.ioi_Send.iob_Len = sizeof(gTimeVal);
    return SuperInternalSendIO(tior);
}

Err StopTimer(void)
{
    if(gTimerActive) {
	SuperInternalAbortIO(gData.timerReq);
	SuperWaitIO(gData.timerReq->io.n_Item);
	SDBUG(("StopTimer - timer stopped.\n"));
	gTimerActive = 0;
    }
    return 0;
}


