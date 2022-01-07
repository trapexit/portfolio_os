/* $Id: lomac.c,v 1.27 1994/02/09 01:22:45 limes Exp $ */
/* file: lomacdriver.c */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "strings.h"
#include "interrupts.h"
#include "kernelnodes.h"
#include "kernel.h"
#include "operror.h"
#include "super.h"

#include "debug.h"

extern Superkprintf(const char *fmt, ... );
extern SuperDebugTrigger(void);
extern SuperPause(void);

#define DBUG(x)	/* Superkprintf x */

List MacPckts;
IOReq *iorWorking;
extern struct KernelBase *KernelBase;

static void
myAbortIO(ior)
IOReq *ior;
{
}

int MacBusy()
{
	volatile int *p = (int *)KernelBase->kb_MacPkt;
	return p[0] | p[1];	/* use LOCK | READY */
}

void
WaitOwn(void)
{
	volatile long *p = (long *)KernelBase->kb_MacPkt;
	while (MacBusy());
	*p = -1;	/* should be a swap */
}

void
WaitClear(void)
{
	while (MacBusy());
}

int icnt;

void
DoPckt(ior)
IOReq *ior;
{
	void *src = ior->io_Info.ioi_Send.iob_Buffer;
	int32 len = ior->io_Info.ioi_Send.iob_Len;
	dbghdr *dbg = (dbghdr *)KernelBase->kb_MacPkt;

	WaitOwn();	/* Wait for Mac to be ready */
	/* buffer is locked */
	iorWorking = ior;
	memcpy(&dbg[1],src,len);	/* Copy packet to mac buffer verbatum */
	icnt = 0;
	dbg->dbgReady = -1;	/* packet ready to send */
	SuperDebugTrigger();	/* trigger mac to read it */
}

int32
clomacInt(void)
	/* called by interrupt routine */
{
	IOReq *oldior = iorWorking;
	IOReq *ior = 0;
	if (MacBusy())
	{
		if (oldior)	icnt++;
		return 0;
	}
	iorWorking = 0;
	if (oldior) {
		SuperCompleteIO(oldior);
	}
	/* Complete IO may have started another MacIO operation */
	if (iorWorking == 0)	/* Still not busy? */
	{	/* get next one off q */
		ior = (IOReq *)RemHead(&MacPckts);
		if (ior)
			DoPckt(ior);	/* have new one todo */
	}
	return 0;
}

static Item firqItem;
extern void lomacInt(void);

#ifdef undef
static
TagArg FirqTags[] =
{
	TAG_ITEM_PRI,	(void *)5,
	TAG_ITEM_NAME,	"mac",
	CREATEFIRQ_TAG_CODE, (void *)(int)clomacInt,
	CREATEFIRQ_TAG_NUM, (void *)(int)INT_V1,
	TAG_END,	(void *)0,
};
#endif

/*#define NOMAC*/

static Item
macInit(dev)
Driver *dev;
{
#ifdef NOMAC
	if ( KernelBase->kb_CPUFlags & KB_NODBGR) return -1;
#else
	if ( (KernelBase->kb_CPUFlags & (KB_NODBGR|KB_NODBGRPOOF))
	     == KB_NODBGR)	return -1;
#endif
	InitList(&MacPckts,"lomacpcktlist");
	/*firqItem = SuperCreateItem(MKNODEID(KERNELNODE,FIRQNODE),FirqTags);*/
	firqItem = SuperCreateFIRQ("mac",5,clomacInt,INT_V1);
	if (firqItem < 0)	return firqItem;
	return dev->drv.n_Item;
}

static int
CmdTrigger(ior)
IOReq *ior;
{
	SuperDebugTrigger();
	return 1;
}

static int
CmdWrite(ior)
IOReq *ior;
{
	/* Command Write */
	int32 len = ior->io_Info.ioi_Send.iob_Len;
	uint32 oldints;

	DBUG(("CmdWrite len=%d \n",len));
	DBUG(("iorWorking = %lx\n",iorWorking));
	DBUG(("MacBusy=%lx\n",(ulong)MacBusy()));

	if (len == 0) return 1;
	if (len < 0) goto abort;

	if (len > 128-sizeof(dbghdr)) goto abort;

	oldints = Disable();
	if (MacBusy() || iorWorking)
	{
		AddTail(&MacPckts,(Node *)ior);
		ior->io_Flags &= ~IO_QUICK;
		Enable(oldints);
		return 0;
	}

	ior->io_Flags &= ~IO_QUICK;
	DoPckt(ior);
	Enable(oldints);
	return 0;
abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdBad(ior)
IOReq *ior;
{
    ior->io_Error = BADCOMMAND;
    return 1;
}

static int (*CmdTable[])() =
{
	CmdWrite,
	CmdBad,	/* reserved for READ */
	CmdBad,	/* reserved for STATUS */
	CmdTrigger,
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	TAG_ITEM_NAME,	"lomac",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)myAbortIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)4,
	CREATEDRIVER_TAG_CMDTABLE,	(void *)CmdTable,
	CREATEDRIVER_TAG_INIT,		(void *)((long)macInit),
	TAG_END,		0,
};

#ifdef undef
static TagArg devArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	CREATEDEVICE_TAG_DRVR,	0,
	TAG_ITEM_NAME,	"lomac",
	TAG_END,	0,
};
#endif

Item
createLoMacDriver(void)
{
	Item drvrItem;
	Item devItem;

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
#ifdef undef
	DBUG(("Creating lomac driver returns drvrItem=%d\n",drvrItem));
	DBUG(("pcktaddr=%lx\n",KernelBase->kb_MacPkt));
#endif
#ifdef undef
	kprintf("address of lomacWrite = %lx\n",(ulong)CmdWrite);
#endif
	if (drvrItem>0)
	{
#ifdef undef
		devArgs[1].ta_Arg = (void *)drvrItem;
		devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),devArgs);
#endif
		devItem = CreateDevice("lomac",1,drvrItem);
		/*kprintf("Creating lomac device returns devItem=%d\n",devItem);*/
	}
	return devItem;
}

