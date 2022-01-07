/* $Id: cdrom.c,v 1.28 1994/04/15 21:02:04 shawn Exp $ */
/* file: cdrom.c */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "strings.h"
#include "operror.h"
#include "super.h"

#include "debug.h"

extern Superkprintf(const char *fmt, ... );
extern SuperDebugTrigger(void);
extern SuperPause(void);

static void SendPacket(IOReq *, uint32 flags);

#define DBUG(x)	/* Superkprintf x */

/* Where we construct the packets to send to the mac */
static Item IOReqItem;
static Item DeviceItem;	/* for lomac device */

static IOReq *macior;

static List iorqs;

#define	BLOCKSIZE	2048
#define	MAXUNITS	1

static char macfnames[MAXUNITS][256];
static int32 macfsizes[MAXUNITS];

static void
myAbortIO(ior)
IOReq *ior;
{
}

char macname[]="mac";

#ifdef undef
static TagArg IOTags[] =
{
	CREATEIOREQ_TAG_DEVICE, 0,
	TAG_END, 0,
};
#endif

static Item
cdromInit(dev)
Driver *dev;
{
	DeviceItem = SuperOpenItem(SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),macname),0);
        if (DeviceItem < 0)
	{
#ifdef DEBUG
		Superkprintf("Error openning:%s\n",macname);
#endif
		return DeviceItem;
	}
#ifdef undef
	IOTags[0].ta_Arg = (void *)DeviceItem;
	IOReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE),
					(void *)IOTags);
#endif
	IOReqItem = SuperCreateIOReq(0,0,DeviceItem,0);
	if (IOReqItem < 0)
	{
		Superkprintf("Error creating ioreq\n");
		SuperCloseItem(DeviceItem);
		return IOReqItem;
	}
	macior = (struct IOReq *)LookupItem(IOReqItem);
	bzero(macfnames,sizeof(macfnames));
	InitList(&iorqs,"maccdrom.iorqs");
	return dev->drv.n_Item;
}

static IOReq *iorWorking;

static int
CmdStatus(ior)
IOReq *ior;
{
	ulong  unit = ior->io_Info.ioi_Unit;
	DeviceStatus *dst = (DeviceStatus *)ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	int bs;

	DBUG(("CmdStatus unit=%d dst=%lx len=%d\n",unit,dst,len));
	if (unit >= MAXUNITS) goto abort;

	if (len < 8)	goto abort;
	if ( (int)dst & 0x3)	goto abort;

	bs = (int)(macfsizes[unit]+BLOCKSIZE-1)/BLOCKSIZE;

	dst->ds_DriverIdentity = 0;
	dst->ds_DriverStatusVersion = 0;
	dst->ds_headerPad = 0;
	dst->ds_MaximumStatusSize = sizeof(DeviceStatus);
	if (len < sizeof(DeviceStatus) )
	{
		ior->io_Actual = 8;
		return 1;
	}
	dst->ds_DeviceBlockSize = BLOCKSIZE;
	dst->ds_DeviceBlockCount = bs;
	dst->ds_DeviceFlagWord = DS_USAGE_READONLY;
	dst->ds_DeviceUsageFlags = DS_USAGE_READONLY | DS_USAGE_FILESYSTEM;
	dst->ds_DeviceLastErrorCode = 0;
	ior->io_Actual = sizeof(DeviceStatus);

	return 1;
abort:
	ior->io_Error = BADIOARG;
	return 1;
}

IOReq *
cdromCallBack(ior)
IOReq *ior;
{
	IOReq *myio;
	myio = iorWorking;
	if (myio == 0)
	{
		while (1);
	}
	iorWorking = 0;
	myio->io_Error = ior->io_Error;
	if (myio->io_Info.ioi_Command == CMD_READ)
	    myio->io_Actual = ior->io_Actual;
	SuperCompleteIO(myio);
	if (iorWorking == 0)
	{
		myio = (IOReq *)RemHead(&iorqs);
		if (myio)
			SendPacket(myio,0);
	}
	if (iorWorking)	return  macior;
	return 0;
}

static void
SendPacket(IOReq *ior, uint32 flag)
{
	void *dst = ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	char *fname;
	int32  offset = ior->io_Info.ioi_Offset;
	ulong  unit = ior->io_Info.ioi_Unit;

	fname = macfnames[unit];

	macior->io_Info.ioi_Unit = 0;
	if (ior->io_Info.ioi_Command == CMD_READ)
	{
		macior->io_Info.ioi_Command = MACCMD_READCDDELAY;
		macior->io_Info.ioi_Recv.iob_Buffer = dst;
		macior->io_Info.ioi_Recv.iob_Len = len;
		macior->io_Info.ioi_Offset = BLOCKSIZE*offset;	/* 2k / block */
	}
	else 
	{
		macior->io_Info.ioi_Command = MACCMD_FILELEN;
		macior->io_Info.ioi_Recv.iob_Buffer = &macfsizes[unit];
		macior->io_Info.ioi_Recv.iob_Len = 4;
		macior->io_Info.ioi_Offset = 0;
	}
	macior->io_Info.ioi_Send.iob_Buffer = fname;
	macior->io_Info.ioi_Send.iob_Len = strlen(fname)+1;
	macior->io_Info.ioi_Offset = BLOCKSIZE*offset;	/* 2k / block */
	macior->io_Info.ioi_Flags = 0;
	macior->io_CallBack = cdromCallBack;


	SuperinternalSendIO(macior);
	if(flag)ior->io_Flags &= ~IO_QUICK;
	iorWorking = ior;
}

static int
CmdRead(ior)
IOReq *ior;
{
	/* Command Read */
	ulong  unit = ior->io_Info.ioi_Unit;
	int32  offset = ior->io_Info.ioi_Offset;
	char *fname;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	uint32 oldints;

	DBUG(("CmdREAD len=%d Unit=%d\n",len,unit));
	DBUG(("buffer=%lx\n",(long)(ior->io_Info.ioi_Recv.iob_Buffer)));
	DBUG(("offset=%lx\n",offset));

	if (unit >= MAXUNITS) goto abort;
	fname = macfnames[unit];
	if (fname[0] == 0)	goto abort;

	if (offset < 0) goto abort;
	if (len < 0) goto abort;
	if (len == 0) return 1;

	oldints = Disable();
	if (iorWorking)
	{

		AddTail(&iorqs,(Node *)ior);
		ior->io_Flags &= ~IO_QUICK;
		Enable(oldints);		

	}
	else
	{

		ior->io_Flags &= ~IO_QUICK;
		SendPacket(ior,1);
		Enable(oldints);
	}

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdSetUnitName(ior)
IOReq *ior;
{
	ulong  unit = ior->io_Info.ioi_Unit;
	char *src = (char *)ior->io_Info.ioi_Send.iob_Buffer;
	uint32 oldints;

	DBUG(("CmdSetUnitName unit=%d :%s\n",unit,src));

	if (unit >= MAXUNITS)	goto abort;

	strcpy(macfnames[unit],src);
	ior->io_Actual = strlen(src)+1;

	oldints = Disable();
	if (iorWorking)
	{


		AddTail(&iorqs,(Node *)ior);
		ior->io_Flags &= ~IO_QUICK;
		Enable(oldints);

	}
	else
	{
		SendPacket(ior,1);
		ior->io_Flags &= ~IO_QUICK;
		Enable(oldints);


	}

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdWrite(ior)
IOReq *ior;
{
	DBUG(("maccdroms do not write!\n"));
	ior->io_Error = BADCOMMAND;
	return 1;
}

static int (*CmdTable[])() =
{
	CmdWrite,
	CmdRead,
	CmdStatus,
	CmdSetUnitName,
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	TAG_ITEM_NAME,	"maccdrom",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)myAbortIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)4,
	CREATEDRIVER_TAG_CMDTABLE,	(void *)CmdTable,
	CREATEDRIVER_TAG_INIT,		(void *)((long)cdromInit),
	TAG_END,		0,
};

#ifdef undef
static TagArg devArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	CREATEDEVICE_TAG_DRVR,	0,
	TAG_ITEM_NAME,	"maccdrom",
	TAG_END,		0,
};
#endif

Item
createMacCDRomDriver(void)
{
	Item drvrItem;
	Item devItem;

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
	/*kprintf("Creating CDRom driver returns devItem=%d\n",drvrItem);*/
	if (drvrItem>=0)
	{
#ifdef undef
		devArgs[1].ta_Arg = (void *)drvrItem;
		devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),devArgs);
#endif
		devItem = CreateDevice("maccdrom",1,drvrItem);
		/*kprintf("Creating cdrom device returns devItem=%d\n",devItem);*/
	}
	return devItem;
}

