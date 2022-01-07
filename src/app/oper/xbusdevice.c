/* $id: xbusdevice.c,v 1.117.1.2 1994/11/21 19:06:41 stan Exp jhw $ */

/* file: xbusdevice.c */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "strings.h"
#include "interrupts.h"
#include "kernel.h"
#include "operror.h"
#include "setjmp.h"
#include "kernel.h"
#include "inthard.h"
#include "time.h"
#include "aif.h"
#include "discdata.h"
#include "rom.h"

#define ASYNC_DMA

#include "clio.h"

/*#define KERNEL*/
/*#define XCREATEIO*/

#include "mem.h"

#include "super.h"
#include "sysinfo.h"

#define KDBUG(x) /* kprintf x */
#define DBUG(x)	 /* Superkprintf x */
#define VDBUG(x) /* Superkprintf x */
#define INFO(x) /* Superkprintf x*/

#ifdef PRODUCTION
# define DBUG0(x) /* x */
#else
# define DBUG0(x) if (!(KernelBase->kb_CPUFlags & KB_NODBGR)) Superkprintf x
#endif

#define TESTREAD(s)	(s & XBUS_POLL_READ_VALID)
#define TESTWRITE(s)	(s & XBUS_POLL_WRIT_VALID)
#define TESTSTAT(s)	(s & XBUS_POLL_STAT_VALID)

#define POLL			xb->xb_Poll[0]
#define READPOLL(poll,unit)	xb->xb_Sel[0] = unit; \
				poll = (uint8)POLL
#define WRITEPOLL(val)		POLL = (vuint32)(val)

#define XBUS_IDSIZE	10

#define DIPIR_DRIVER 	0x01
#define KERNEL_DRIVER 	0x02
#define DRIVER_FILESYS 	0x03


typedef struct DriverTableElement
	{
	uint32	DriverTag;
	uint32	SizeOfDriverImage;
	} DriverTableElement;

typedef struct ExpDev
{
	Node	ed_n;
	uint8	ed_Unit;	/* device id */
	uint8	ed_Flags;	/* misc flags */
				/* result from Read Identification */
	uint32	ed_ManuIdNum;	/* Manufacturer's Identification */
	uint32	ed_ManuDevNum;	/* Manufacturer Device number */
	uint32	ed_ManuRevNum;	/* Revision number */
	uint32	ed_ManuFlags;	/* Manufacturer flags */
				/* set in ReadID */
	int32	ed_ManuDrvrTagTableSize;

	IOReq	*ed_iorWorking;		/* current ioreq in progress */
	IOReq	*ed_DataiorWorking;
	IOReq	*ed_WRDataiorWorking;
	IOReq	*ed_StatusWorking;	/* for draining the status fifo */
	List	ed_Commands;		/* list of waiting commands */
	List	ed_DataPackets;		/* list of waiting data xfer cmds */
	List	ed_WRDataPackets;	/* list of waiting data xfer cmds */
	List	ed_StatusPackets;	/* list of waiting status draining cmds */
	uint32	ed_DriverTag;		/* driver tag wanted and got 	  */
	DriverTableElement *ed_DriverTagTable;	/* pointer to driver tags */
	uint32	ed_LogicalSizeOfDriver;	/* allocated size of driver image */
	uint32	ed_PhysicalSizeOfDriver;/* physical size of driver image  */
	void	*ed_Driver;		/* ptr to start of driver */
        uint32  ed_XferSpeed;           /* DMA xfer speed, *100 kb/sec */
        uint32  ed_XferRegister;        /* ExpType register codes for above */
	uint32  ed_BurstSize;           /* Max DMA transfer size in bytes */
        uint32  ed_HogTime;             /* Max DMA bus-on time in usec */
} ExpDev;

#define MAX_EXP_UNITS	15

/* table of ptrs to exp-structs on the expansion bus */
ExpDev *Dev_table[MAX_EXP_UNITS];

/* DMA Q */
IOReq *DmaWorking;
List DMAReadyQ;		/* list of iors for dma */

#define DMA

typedef struct dmastr
{
	vint32 Address;
	vint32 Length;
	vint32 NextAddress;
	vint32 NextLength;
} dmastr;

/*
   XBUS DMA-handshaking speed control values.
   Fastest transfer is (0,0,0), 3 ticks @ 40 ns, 8.33 MHz.
   Slowest transfer is (7,7,3), 20 ticks @ 40 ns, 1.25 MHz.
*/

#define XBUS_MIN_PERIOD  120
#define XBUS_MAX_PERIOD  800

const uint8 xbus_speed_codes[] = {
   XBUSTYPE(0,0,0)  /* 0 */,
   XBUSTYPE(0,0,0)  /* 1 */,
   XBUSTYPE(0,0,0)  /* 2 */,
   XBUSTYPE(0,0,0)  /* 3 */,
   XBUSTYPE(0,1,0)  /* 4 */,
   XBUSTYPE(1,1,0)  /* 5 */,
   XBUSTYPE(1,1,1)  /* 6 */,
   XBUSTYPE(1,2,1)  /* 7 */,
   XBUSTYPE(2,2,1)  /* 8 */,
   XBUSTYPE(2,2,2)  /* 9 */,
   XBUSTYPE(2,3,2)  /* 10 */,
   XBUSTYPE(3,3,2)  /* 11 */,
   XBUSTYPE(3,3,3)  /* 12 */,
   XBUSTYPE(3,4,3)  /* 13 */,
   XBUSTYPE(4,4,3)  /* 14 */,
   XBUSTYPE(4,5,3)  /* 15 */,
   XBUSTYPE(5,5,3)  /* 16 */,
   XBUSTYPE(5,6,3)  /* 17 */,
   XBUSTYPE(6,6,3)  /* 18 */,
   XBUSTYPE(6,7,3)  /* 19 */,
   XBUSTYPE(7,7,3)  /* 20 */
 };



int dma_cnt = 0;
uint8 *dma_dst;
volatile uint32 *dma_src;

int wdma_cnt = 0;
volatile uint32 *wdma_dst;
uint8 *wdma_src;

List WaitDipirStartList;	/* List of ior's waiting for dipir start */
List WaitDipirEndList;		/* List of ior's waiting for dipir end */
bool DipirWaitInitDone = FALSE;

static int MaxDevice = -1;

Item xbusintItem;	/* interrupt for xbus */
Item xdmaintItem;	/* interrupt for dma xfer on xbus */
List ExpDevList;	/* List of Expansion Devices */

void DoXBusCommand(IOReq *,ExpDev *);
void BeginXferDataIn(IOReq *);
void BeginXferDataOut(IOReq *);
void PrimeStatus(IOReq *);

int32 cExpInt(void);
int32 cExpDmaInt(void);

/* If an I/O is aborted, it is up to the client to issue */
/* the right commands to do the proper cleanup, including */
/* draining the proper fifos */

uint8 XBusOwner;	/* current owner of the bus */

#define SELECTXBUS(u)	xb->xb_Sel[0] = u;

/* A prototype or two */

extern void XferDataFifo(IOReq *ior);
extern int SoftReset(void);
extern uint32 Read4Bytes(volatile uint32 *src);
extern void Write4Bytes(uint32 src,volatile uint32 *dst);

extern void	Duck(void);	/* from duckandcover */
extern void	MissedMe(void);	/* from duckandcover */

extern AIFHeader	*SherryHeader;

void
SelectXBus(int unit)
{
	ExpansionBus *xb = XBUS_ADDR;
	xb->xb_Sel[0] = unit;
}

void
WaitXBus(void)
{
	ExpansionBus *xb = XBUS_ADDR;
	while ( (xb->xb_SetExpCtl & XB_CPUHASXBUS) == 0) 
		; /* do nothing */
}

static void
DipirWaitInit(void)
{
	uint32 oldints;

	/*
	 * Initialize the lists if they're not initialized yet.
	 * We can't do this in expansionInit because dipir events
	 * can happen before the xbus device is created.
	 * (E.g. FMV dipir happens in ramInit, and createRamDevice
	 * happens before createExpansionDevice.)
	 */
	oldints = Disable();
	InitList(&WaitDipirStartList, "Dipir wait-start Queue");
	InitList(&WaitDipirEndList, "Dipir wait-end Queue");
	DipirWaitInitDone = TRUE;
	Enable(oldints);
}

static int32
NotifyDipirWait(List *WaitList)
{
	IOReq *ior;
	List TempList;
	uint32 oldints;

	if (!DipirWaitInitDone) 
		DipirWaitInit();

	/* Take all IO requests off the WaitList and put them on TempList.
	 * When we call CompleteIO, there may be a callback which will
	 * add another request to the WaitList.  We don't want these new
	 * ones to complete immediately! 
	 */
	InitList(&TempList, "");
	oldints = Disable();
	while ((ior = (IOReq *)RemHead(WaitList)) != 0) {
		AddTail(&TempList, (Node *)ior);
	}
	Enable(oldints);
	while ((ior = (IOReq *)RemHead(&TempList)) != 0) {
		SuperCompleteIO(ior);
	}
	return 0;
}

int32
NotifyXBusDipirStart(void)
{
	return NotifyDipirWait(&WaitDipirStartList);
}

int32
NotifyXBusDipirEnd(void)
{
	return NotifyDipirWait(&WaitDipirEndList);
}

uint8
GrabXBus(int unit)
{
	uint8 oldowner = XBusOwner;
	uint8 poll;
	ExpansionBus *xb = XBUS_ADDR;

	SELECTXBUS(unit);
	/* Now wait till we get it */
	WaitXBus();
	/* Now check for DIPIR */
	poll = (uint8)POLL;

	if (poll & XBUS_POLL_MEDIA_ACCESS) {
		int xbus_sr;
		uint32 cstatbits = *CSTATBits;
		Duck();
		cstatbits |= CLIO_SoftReset|CLIO_ClearDIPIR;
		/* Enable this SoftReset for New roms */
		do
		{
#if 0
			Enable(0x40);
			DBUG(("DIPIR EVENT!\n"));
#endif /* 0 */
			/*
			 * clear the media access bit
			 * this is needed to get the dipir to be noticed by clio
			 * for some reason?
			 * the following should be done by the dipir !!!
			 */
			WRITEPOLL((uint32)poll|XBUS_POLL_MEDIA_ACCESS);
			/*
	    		 * See dipir.h in dipir for return codes 
			 * (move somewere better)
			 */
			xbus_sr = SoftReset();
		} while (!xbus_sr);	/* Zero is failure, else return code */
		INFO(("xbus_sr= %ld\n", xbus_sr));

		MissedMe();
	}
	XBusOwner = unit;
	return oldowner;
}

uint8
ReadPoll(uint8 unit)
{
	uint8 val;
	ExpansionBus *xb = XBUS_ADDR;
	ExpDev *ed = Dev_table[unit&0xf];
	READPOLL(val,ed->ed_Unit);
	return val;
}

void
ReleaseXBus(int unit)
{
	/* We know we have the bus already */
	ExpansionBus *xb = XBUS_ADDR;
	ExpDev *ed = Dev_table[unit&0xf];

	XBusOwner = unit;
	xb->xb_Sel[0] = ed->ed_Unit;
	xb->xb_ClrExpCtl = XB_CPUHASXBUS;	/* give back to DMA */
}

void
StartDma(int unit)
{
	ExpansionBus *xb = XBUS_ADDR;
	ExpDev *ed = Dev_table[unit&0xf];

	xb->xb_Sel[0] = ed->ed_Unit;
	xb->xb_ClrExpCtl = XB_CPUHASXBUS;	/* give back to DMA */
}

void dotrigger(void);

void
DoNextDMA(void)
{
	if(DmaWorking == 0) {
		IOReq *ior;
		ior = (IOReq *)RemHead(&DMAReadyQ);
		if(ior) {
			int unit = ior->io_Info.ioi_Unit;
			ExpDev *ed = Dev_table[unit];
			/*dotrigger();*/
			GrabXBus(ed->ed_Unit);
			XferDataFifo(ior);
		}
	}
}

void
DoNext(ed)
ExpDev *ed;
{
	if (ed->ed_iorWorking == 0) { /* still not busy? */
		IOReq *ior;
		ior = (IOReq *)RemHead(&ed->ed_Commands);
		if (ior)
			DoXBusCommand(ior,ed);
	}
	if (ed->ed_DataiorWorking == 0) {
		IOReq *ior;
		ior = (IOReq *)RemHead(&ed->ed_DataPackets);
		if (ior)
			BeginXferDataIn(ior);
	}
	if (ed->ed_WRDataiorWorking == 0) {
		IOReq *ior;
		ior = (IOReq *)RemHead(&ed->ed_WRDataPackets);
		if (ior)
	BeginXferDataOut(ior);
	}
	if (ed->ed_StatusWorking == 0) {
		IOReq *ior;
		ior = (IOReq *)RemHead(&ed->ed_StatusPackets);
		if (ior)
			PrimeStatus(ior);
	}
}

void
myAbortIO(ior)
IOReq *ior;
{
	/* on entry, interrupts are disabled, the ior is busy */
	int unit;
	ExpDev *ed;
	ExpansionBus *xb = XBUS_ADDR;

	unit = ior->io_Info.ioi_Unit;
	ed = Dev_table[unit];

	if (ior == ed->ed_iorWorking)
		ed->ed_iorWorking = 0;
	else if (ior == ed->ed_DataiorWorking)
		ed->ed_DataiorWorking = 0;
	else if (ior == ed->ed_WRDataiorWorking)
		ed->ed_WRDataiorWorking = 0;
	else if (ior == ed->ed_StatusWorking)
		ed->ed_StatusWorking = 0;
	else if (ior == DmaWorking)
		DmaWorking = 0;
	else {
		/* Remove it from whatever list it is on */
		RemNode((Node *)ior);
	}


	/* What do we do about the io that was in progress? */
	/* This is really pretty ugly */
	ior->io_Error = ABORTED;
	SuperCompleteIO(ior);

	/* maybe we want to give the xbus back to the CPU ? */
	xb->xb_SetExpCtl = XB_DMARESET|XB_CPUHASXBUS;
	xb->xb_ClrExpCtl = XB_DMAON;

	DoNext(ed);
}

TagArg DmaTags[] =
{
	TAG_ITEM_PRI,	(void *)200,
	TAG_ITEM_NAME,	"ExpansionDma",
	CREATEFIRQ_TAG_CODE, (void *)((int )cExpDmaInt),
	CREATEFIRQ_TAG_NUM,	(void *)INT_DMA_EXP,
	TAG_END,	(void *)0,
};

/*#define SLOWREAD*/
#define REALFAST

void
CpuXferInAndChuck(src,cnt)
volatile uint32 *src;
int cnt;
{
	int wcnt = cnt & (~7);
	uint32 a;
	cnt -= wcnt;
	while (wcnt) {
		a = *src; a = *src; a = *src; a = *src;
		a = *src; a = *src; a = *src; a = *src;
		wcnt -= 8;
	}
	while (cnt--)
		a = (uint8)*src;
}

void
CpuXferIn(src,dst,cnt)
volatile uint32 *src;
uint8 *dst;
int cnt;
{
	uint32 *fdst;
	int wcnt = ((int)dst) & 3;
	if (cnt < 4)
		goto slow;
	if (wcnt) { /* dst not aligned, line it up */
		wcnt = 4 - wcnt;
		cnt -= wcnt;
		while (wcnt--)
			*dst++ = (uint8)*src;
	}
	fdst = (uint32 *)dst;
	wcnt = cnt & (~3);	/* how many sets of 4 can we transfer? */
	cnt -= wcnt;
	while (wcnt) {
#ifdef undef
		uint32 a,b,c,d;
		a = *src; b = *src; c = *src; d = *src;
		*fdst++ = (a<<24)|(d&0xff)|((c&0xff)<<8)|((b&0xff)<<16);
#else
		*fdst++ = Read4Bytes(src);
#endif
		wcnt -= 4;
	}
	dst = (uint8 *)fdst;
slow:
	while (cnt--)
		*dst++ = (uint8)*src;
}


void
CleanupDMA(IOReq *ior, int cnt, volatile uint32 *src, uint8 *dst) 
{
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];

	while (cnt--)
		*dst++ = (uint8)*src;

	ior->io_Actual = ior->io_Info.ioi_Recv.iob_Len;
	ed->ed_DataiorWorking = 0;
	SuperCompleteIO(ior);
	DoNext(ed);
}

void
CleanupWriteDMA(IOReq *ior, int cnt, uint8 *src,volatile uint32 *dst) 
{
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];

	while (cnt--)
		*dst = *src++;

	ior->io_Actual = ior->io_Info.ioi_Send.iob_Len;
	ed->ed_WRDataiorWorking = 0;
	SuperCompleteIO(ior);
	DoNext(ed);
}


void
DMAXferIn(src,dst,cnt)
volatile uint32 *src;
uint8 *dst;
int cnt;
{
	int wcnt = ((int)dst) & 3;

	if (cnt < 8)
		goto slow;

	if (wcnt) { /* dst not aligned, line it up */
		wcnt = 4 - wcnt;
		cnt -= wcnt;
		while (wcnt--) *dst++ = (uint8)*src;
	}
	wcnt = cnt & (~3);	/* how many sets of 4 can we transfer? */
	if (wcnt > 10*4) {
		Clio *clio = (Clio *)CLIO;
		dmastr *p = (dmastr *)DMAtofrEXP0;
		ExpansionBus *xb = XBUS_ADDR;
		/* Try the DMA */
	
		/* set the dma ram ptr */
		p->Address = (vint32)dst;
		p->Length = (vint32)wcnt-4;
	
	
		xb->xb_SetExpCtl = XB_DMARESET;
		xb->xb_XferCnt = wcnt;
		xb->xb_ClrExpCtl = XB_CPUHASXBUS|XB_DMAISOUT;
		xb->xb_SetExpCtl = XB_DMAON;
		dma_src = src;	
		dma_dst = dst + wcnt;
		dma_cnt = cnt - wcnt;
	
		clio->SetDMAEnable = EN_DMAtofrEXP;
		return;
	}
slow:  
	{
		IOReq *ior = DmaWorking;
		DmaWorking = NULL;	
		CleanupDMA(ior,cnt,src,dst);
		DoNextDMA();
	}
}


void
DMAXferOut(dst,src,cnt)
volatile uint32 *dst;	/* ptr to dma address */
uint8 *src;	/* buffer to get data from */
int cnt;
{
	int wcnt = ((int)src) & 3;

	if (cnt < 8)
		goto slow;

	if (wcnt) { /* src not aligned, line it up */
		wcnt = 4 - wcnt;
		cnt -= wcnt;
		while (wcnt--)
			*dst = *src++;
	}
	wcnt = cnt & (~3);	/* how many sets of 4 can we transfer? */

	if (wcnt > 10*4) {
		Clio *clio = (Clio *)CLIO;
		dmastr *p = (dmastr *)DMAtofrEXP0;
	
		ExpansionBus *xb = XBUS_ADDR;
		/* Try the DMA */
		/* set the dma ram ptr */
		p->Address = (vint32)src;
		p->Length = (vint32)wcnt-4;
	
		xb->xb_SetExpCtl = XB_DMARESET|XB_DMAISOUT;
		xb->xb_XferCnt = wcnt;
		xb->xb_ClrExpCtl = XB_CPUHASXBUS;
		xb->xb_SetExpCtl = XB_DMAON;
		wdma_src = src + wcnt;
		wdma_dst = dst;
		wdma_cnt = cnt - wcnt;

		clio->SetDMAEnable = EN_DMAtofrEXP;
		return;
	}
slow:
	{
		IOReq *ior = DmaWorking;
		DmaWorking = NULL;
		CleanupWriteDMA(ior,cnt,src,dst);
		DoNextDMA();
	}
}

#define FASTERWRITE
#define WRITE4

void
XferOut(dst,src,cnt)
volatile uint32 *dst;
uint8 *src;
int cnt;
{
#ifdef FASTERWRITE
	int *fsrc;
#endif
	int wcnt = (int)src & 3;
	if (wcnt) {	/* src not aligned */
		wcnt = 4 - wcnt;
		cnt -= wcnt;
		while (wcnt--)
			*dst = (int)*src++;
	}
	wcnt = cnt & (~3);
	cnt -= wcnt;
	fsrc = (int *)src;
	while (wcnt) {
	#ifdef FASTERWRITE
		Write4Bytes(*fsrc++,dst);
	#else
		*dst = (int)*src++;
		*dst = (int)*src++;
		*dst = (int)*src++;
		*dst = (int)*src++;
	#endif
		wcnt -= 4;
	}
#ifdef FASTERWRITE
	src = (uint8 *)fsrc;
#endif
	while (cnt--)
		*dst = (int)*src++;
}

void XferDataFifo(IOReq *ior)
{
	int len;

	if (ior->io_Info.ioi_Command == CMD_READ) {
		uint8 *dst;
		volatile uint32 *src;
		{
			ExpansionBus *xb = XBUS_ADDR;
			src = &xb->xb_Data[0];
	
			DmaWorking= ior;
	
			dst = (uint8 *)ior->io_Info.ioi_Recv.iob_Buffer;
			len = (int)ior->io_Info.ioi_Recv.iob_Len;
			DMAXferIn(src, dst,len);
		}
	} else {
		uint8 *src;
		volatile uint32 *dst;
		{
			ExpansionBus *xb = XBUS_ADDR;
			dst = &xb->xb_Data[0];
	
				DmaWorking= ior;
	
				src = (uint8 *)ior->io_Info.ioi_Send.iob_Buffer;
				len = (int)ior->io_Info.ioi_Send.iob_Len;
				DMAXferOut(dst,src,len);
		}
		/* must be done after xfer , really? */
		SelectXBus(Dev_table[ior->io_Info.ioi_Unit]->ed_Unit);
		ior->io_Actual = len;
	}
}

uint8
ReadStatusByte()
{
	volatile uint32 *src;
	ExpansionBus *xb = XBUS_ADDR;
	src = &xb->xb_CmdStat[0];
	return (uint8)*src;
}

uint32
ReadStatWord()
{
	uint32 a,b;
	a = ReadStatusByte();
	b = ReadStatusByte();
	return (a<<8)|b;
}

void
ReadStatusFifo(ior)
IOReq *ior;
{
	uint8 *dst = (uint8 *)ior->io_Info.ioi_Recv.iob_Buffer;
	int len = (int)ior->io_Info.ioi_Recv.iob_Len;
	int unit = ior->io_Info.ioi_Unit;
	int i = 0;

	/* bus already selected and owned by cpu */
	while (len--) {
		if (!TESTSTAT(ReadPoll(unit))) break;
		*dst++ = ReadStatusByte();
		i++;
	}
	if (TESTSTAT(ReadPoll(unit))) {
		ior->io_Error = IOINCOMPLETE;
	}
	ior->io_Actual = i;
}

void
SquirtOutCmd(src,len)
uint8 *src;
int len;
{
	ExpansionBus *xb = XBUS_ADDR;
	if (len == 7)
	{	/* most common case */
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src++;
		xb->xb_CmdStat[0] = *src;
	}
	else
		while (len--)	xb->xb_CmdStat[0] = *src++;
}

void
DoXBusCommand(ior,ed)
ExpDev *ed;
IOReq *ior;
{
	uint8 *src = (uint8 *)ior->io_Info.ioi_Send.iob_Buffer;
	ExpansionBus *xb = XBUS_ADDR;
	int len = (int)ior->io_Info.ioi_Send.iob_Len;
	uint8 oldunit;
	uint8 oldpoll;

	ed->ed_iorWorking = ior;

	oldunit = GrabXBus(ed->ed_Unit);
	SELECTXBUS(ed->ed_Unit);	/* transfer latest poll value */
	if (TESTSTAT(POLL)) {
		VDBUG(("sending new cmd but status fifo not empty: draining\n"));
		while (TESTSTAT(ReadPoll(ed->ed_Unit))) {
			int a;
			a = ReadStatusByte();
			VDBUG(("%lx ",a));
		}
		VDBUG(("\n"));
	}
	SquirtOutCmd(src,len);
	/* enable status fifo interrupts for this expansion unit */
	READPOLL(oldpoll,ed->ed_Unit);
	oldpoll &=  (XBUS_INTEN_BITS|XBUS_POLL_RESET);
	WRITEPOLL((uint32)oldpoll | XBUS_POLL_STAT_INT_EN);
	ReleaseXBus(oldunit);
	
	/*
	 * this is kind of goofy. if the recv count is 0, then
	 * this req isn't waiting for status bytes, so complete
	 * it right away.  there is a seperate command to drain
	 * the status fifo
	 */
	if(ior->io_Info.ioi_Recv.iob_Len == 0) {
		IOReq *oldior;
		
		oldior = ed->ed_iorWorking;
		ed->ed_iorWorking = 0;
		SuperCompleteIO(oldior);
		DoNext(ed);
	}
}

void
BeginXferDataIn(ior)
IOReq *ior;
{
	int unit = ior->io_Info.ioi_Unit;
	ExpansionBus *xb = XBUS_ADDR;
	ExpDev *ed = Dev_table[unit];
	uint8 oldunit;
	uint8 pollval;

	ed->ed_DataiorWorking = ior;

	/* enable interrupts for this expansion unit */
	oldunit = GrabXBus(ed->ed_Unit);
	READPOLL(pollval,ed->ed_Unit);
	pollval &= (XBUS_INTEN_BITS|XBUS_POLL_RESET);
	WRITEPOLL((uint32)pollval | XBUS_POLL_READ_INT_EN);
	ReleaseXBus(oldunit);
}

void
BeginXferDataOut(ior)
IOReq *ior;
{
	int unit = ior->io_Info.ioi_Unit;
	ExpansionBus *xb = XBUS_ADDR;
	ExpDev *ed = Dev_table[unit];
	uint8 oldunit;
	uint8 pollval;

	ed->ed_WRDataiorWorking = ior;
	/* enable interrupts for this expansion unit */
	oldunit = GrabXBus(ed->ed_Unit);
	READPOLL(pollval,ed->ed_Unit);
	pollval &= (XBUS_INTEN_BITS|XBUS_POLL_RESET);
	WRITEPOLL((uint32)pollval | XBUS_POLL_WRIT_INT_EN);
	ReleaseXBus(oldunit);
}

void
PrimeStatus(ior)
IOReq *ior;
{	
	int unit = ior->io_Info.ioi_Unit;
	ExpansionBus *xb = XBUS_ADDR;
	ExpDev *ed = Dev_table[unit];
	uint8 oldunit;
	uint8 pollval;

	ed->ed_StatusWorking = ior;
	/* enable interrupts for this expansion unit */
	oldunit = GrabXBus(ed->ed_Unit);
	READPOLL(pollval,ed->ed_Unit);
	pollval &=  (XBUS_INTEN_BITS|XBUS_POLL_RESET);
	WRITEPOLL((uint32)pollval | XBUS_POLL_STAT_INT_EN);
	ReleaseXBus(oldunit);
}

void
dotrigger(void)
{
}

int32
cExpInt()
	/* called by interrupt routine */
{
	int i;
	uint8 pollval;
	uint8 oldunit;
	ExpansionBus *xb = XBUS_ADDR;

	/* just grab it away, but remember the dma owner */
	oldunit = XBusOwner;
#ifdef undef
	{
		SelectXBus(0);
		WaitXBus();
	}
#endif
	/* first check to see if we have a new device plugged in */
	{
		/* device 15 is now always external */
		SelectXBus(XBUS_EXTERNAL|15);
		WaitXBus();
	
		/*READPOLL(pollval, 15);*/
		pollval = (int8)(POLL);
		if (pollval & XBUS_POLL_NEWDEV) {
			/*
			 * A new device was plugged into xbus
			 * need to alert system
			 * shut down xbus interrupts
			 */
			DisableInterrupt(INT_EXPANSION);
			Enable(0x40);
			DBUG(("Shutting down xbus, new dev plugged in\n"));
		}
	}

	while (1) {
		ExpDev *ed = 0;
		/* poll devices */
		for (i = 0; i <= MaxDevice; i++) {
			uint8 tpoll;
			ed = Dev_table[i];
			READPOLL(pollval,ed->ed_Unit);
			/* positive logic */
	
			tpoll = pollval & (pollval<<4);
			if (tpoll & XBUS_INTRQ_BITS) {
				pollval &= 0xf;	/* preserve int enable bits */
				pollval |= tpoll;
				break;
			}
		}

		if (i <= MaxDevice) {
		/* This unit is interrupting */
			if ( TESTREAD(pollval) ) {
				/*
				 * Data is in the DataFifo
				 * disable data read interrupts
				 * for this expansion unit
				 */
				pollval &= ~XBUS_POLL_READ_INT_EN;
				WRITEPOLL(pollval);
				if (ed->ed_DataiorWorking) {
					IOReq *dataoldior = 
						ed->ed_DataiorWorking;
		
					if(DmaWorking) {
						AddTail(&DMAReadyQ,
							(Node *)dataoldior);
					} else {
   					        xb->xb_ExpType = ed->ed_XferRegister; /* set speed for this device */
						XferDataFifo(dataoldior);
		
						oldunit = ed->ed_Unit;
						/* get it back */
						SelectXBus(oldunit);
						WaitXBus();
					}
				}
			} else if ( TESTWRITE(pollval) ) {
				/*
				 * room for more data in the write fifo
				 * disable write interrupts for 
				 * this expansion unit
				 */
				pollval &= ~XBUS_POLL_WRIT_INT_EN;
				WRITEPOLL(pollval);
				if (ed->ed_WRDataiorWorking) {
					IOReq *dataoldior =
						ed->ed_WRDataiorWorking;
		
					if(DmaWorking) {
						AddTail(&DMAReadyQ,
							(Node *)dataoldior);
					} else {
  					        xb->xb_ExpType = ed->ed_XferRegister; /* set speed for this device */

						XferDataFifo(dataoldior);
		
						oldunit = ed->ed_Unit;
						/* get it back */
						SelectXBus(oldunit);
						WaitXBus();
					}
				}
			} else if ( TESTSTAT(pollval) ) {
				/* give status drainers first shot */
				pollval &= ~XBUS_POLL_STAT_INT_EN;
				WRITEPOLL(pollval);
				if(ed->ed_StatusWorking) {
					IOReq *oldior = ed->ed_StatusWorking;
					ed->ed_StatusWorking = 0;
					ReadStatusFifo(oldior);
					SuperCompleteIO(oldior);
					DoNext(ed);  
				} else if (ed->ed_iorWorking) {
					IOReq *oldior = ed->ed_iorWorking;
				
					/*
					 * if the recv buffer has 0 length,
					 * then we can't use this ior
					 * to recover status bytes.
					 * need to wait for a status snarfer
					 */
					if(oldior->io_Info.ioi_Recv.iob_Len != 0) {
						ed->ed_iorWorking = 0;
						ReadStatusFifo(oldior);
						SuperCompleteIO(oldior);
						DoNext(ed);  
					}
				}
			}
		} else {
		/* nothing is interrupting now! return */
		/* do this now, it is supposed to be level sensitive */
		ReleaseXBus(oldunit);
		return 0;
    		}
	}
}

int32
cExpDmaInt()
	/* called by interrupt routine when data fifo xfer complete */
{
	/* Make sure we didn't get interrupt but was not doing anything? */
	if(DmaWorking) {
		ExpansionBus *xb = XBUS_ADDR;
		Clio *clio = (Clio *)CLIO;
		IOReq *ior;

		ior = DmaWorking;
		clio->ClrDMAEnable = EN_DMAtofrEXP;	/* turn dma off */
		xb->xb_ClrExpCtl = XB_DMAON;

		SELECTXBUS(XBusOwner);	/* get xbus back to cpu */
		WaitXBus();


		DmaWorking = NULL;
		if(ior->io_Info.ioi_Command == CMD_READ)
			CleanupDMA(ior,dma_cnt,dma_src,dma_dst);
		else
			CleanupWriteDMA(ior,wdma_cnt,wdma_src,wdma_dst);
		DoNextDMA();

	}
	return 0;
}




#define XBUS_READIDCMD 		0x83
#define XBUS_READDRIVERCMD 	0x8E
#define XBUS_STATUSERR 		0x08
#define CHUNKIT( a ) (a/sizeof(long) + ((a%sizeof(long)) ? 1 : 0))


/*
 * Check the status packet from a command.
 */
static bool 
CheckStatus(ExpDev *ed, uint8 tagByte)
{
	uint8 stat;

	/* Wait for status */
	while (TESTSTAT(ReadPoll(ed->ed_Unit)) == 0)
		continue;
	
	/* Read the status byte.  If we get the tagByte, read another one. */
	stat = ReadStatusByte();
	if (stat == tagByte)
	{
		stat = ReadStatusByte();	
	}
	return (stat & XBUS_STATUSERR) ? true : false;
}

/*
 * Download some bytes from the device ROM.
 */
static int 
DownloadDevice(ExpDev *ed, uint32 offset, uint8 *dest, uint32 size)
{
	volatile uint32 *src;
	int i;
	uint8 cmd[7];
	ExpansionBus *xb;

	xb = XBUS_ADDR;
	cmd[0] = XBUS_READDRIVERCMD;
	cmd[1] = (uint8)(offset >> 24);
	cmd[2] = (uint8)(offset >> 16);
	cmd[3] = (uint8)(offset >> 8);
	cmd[4] = (uint8)(offset);
	cmd[5] = (uint8)(size >> 8);
	cmd[6] = (uint8)(size);
	SquirtOutCmd(cmd, (int)sizeof(cmd));

	size = CHUNKIT(size);	/* number of longwords */
	src = &xb->xb_Data[0];
	for (i = 0; i < size; i++) 
	{
		/* Wait for data; read it in */
		while (TESTREAD(ReadPoll(ed->ed_Unit)) == 0)
			continue;
		*dest++ = (uint8)*src;
		*dest++ = (uint8)*src;
		*dest++ = (uint8)*src;
		*dest++ = (uint8)*src;
	}
	if (CheckStatus(ed, XBUS_READDRIVERCMD))
		return -1;
	return 0;
}

/*
 * Get all relevant info from the device ROM.
 * Currently all we look for is the DEVICE_INFO RomTag.
 * This gives us a new manufacturer ID and device ID for the device.
 */
int
GetDeviceRomInfo(ExpDev *ed)
{
	uint32 offset;
	RomTag rt;

	DBUG(("dev %x: GetDeviceRomInfo\n", ed->ed_Unit));
	/* Read rom tag table, one entry at a time. */
	for (offset = sizeof(DiscLabel);
	     ;
	     offset += sizeof(RomTag))
	{
		if (DownloadDevice(ed, offset, (uint8*)&rt, sizeof(rt)) < 0)
			return -1;
		if (rt.rt_SubSysType == 0)
			break;
		if (rt.rt_SubSysType == RSANODE && 
		    rt.rt_Type == RSA_DEVICE_INFO)
		{
			DBUG(("dev %x: override id ", ed->ed_Unit));
			DBUG(("%x,%x,%x with ",
				ed->ed_ManuIdNum, ed->ed_ManuDevNum, 
				ed->ed_ManuRevNum));
			DBUG(("%x,%x,%x\n",
				rt.rt_Reserved3[0], rt.rt_Reserved3[1], 
				rt.rt_Reserved3[2]));

			ed->ed_ManuIdNum = rt.rt_Reserved3[0];
			ed->ed_ManuDevNum = rt.rt_Reserved3[1];
			ed->ed_ManuRevNum = rt.rt_Reserved3[2];
			return 0;
		}
	}
	return 0;
}

int
ReadID( ExpDev *ed )
{
	uint8 a;

	a = ReadStatusByte();	
	
	if (a == XBUS_READIDCMD)		/* is new format?         */
		a = ReadStatusByte();		/* yes, get the next byte */
	else
		ed->ed_Flags |= XBUS_OLDSTAT;	/* no, remember it	  */

	ed->ed_ManuIdNum = ((uint32)a<<8) | ReadStatusByte();
	ed->ed_ManuDevNum = ReadStatWord();
	ed->ed_ManuRevNum = ReadStatWord();
	ed->ed_ManuFlags = ReadStatWord();
	ed->ed_ManuDrvrTagTableSize = ReadStatWord()*sizeof(long);
	
	/* Now get the status byte which may have error bit set */

	while (TESTSTAT(ReadPoll(ed->ed_Unit)) == 0);	/* spin */
	a = ReadStatusByte();
	DBUG(("a=%lx\n",a));
	
	/* Mark the expansion bus driver stillborn if error bit set */
	
	if ( a & XBUS_STATUSERR ) {
		DBUG(("dev %x: error in ReadId status\n", ed->ed_Unit));
		ed->ed_Flags |= XBUS_DEVDEAD;
		return -1;
	}
	if (ed->ed_ManuFlags & DEVHAS_DRIVER) {
		if (GetDeviceRomInfo(ed) < 0) {
			DBUG(("dev %x: error reading ROM info\n", ed->ed_Unit));
			ed->ed_Flags |= XBUS_DEVDEAD;
		}
	}
	if (ed->ed_ManuIdNum == 0xFFFF) {
		DBUG(("dev %x: manu id = FFFF\n", ed->ed_Unit));
		ed->ed_Flags |= XBUS_DEVDEAD;
	}
	return 0;
}

bool
AskForID( ExpDev *ed )
{
	uint32	cmd[2];
 	int32 	timeout;
	static int maintimeout = 100;
    
	cmd[0] = XBUS_READIDCMD;	/* Read Identification */
	cmd[0] <<= 24;
	cmd[1] = 0;
	
	SquirtOutCmd((uint8 *)cmd,7);
	DBUG(("SquirtOutCmd returned\n"));

	/* Now Wait for the data to come back */

	DBUG(("Waiting for Status FIFO to FILL\n"));

	timeout = 0;
	while ( TESTSTAT(ReadPoll(ed->ed_Unit)) == 0 ) {
		if (timeout++ > 100000) {
			VDBUG(("timeout waiting for ID response, trying again\n"));
			if (maintimeout--) {
				VDBUG(("Giving up\n"));
				ed->ed_Flags |= XBUS_DEVDEAD;
				maintimeout = 100;
				return false;
			}
			return true;
		}
	}
	DBUG(("Got Status FIFO ready\n"));
	return false;
}

/*
 * Initialize a new expansion device unit.
 * Allocate its ExpDev structure, initialize its list pointers and
 * add it to the ExpDevList.
 * Call ReadID to initialize the ExpDev struct.
 */
int
NewExpDev(int unit)
{
	ExpDev *ed;
	ExpansionBus *xb = XBUS_ADDR;
			
	ed = (ExpDev *)AllocMemFromMemLists(KernelBase->kb_MemFreeLists,
			sizeof(*ed),MEMTYPE_FILL);
			
	DBUG(("NewExpDev(%lx)\n",unit));
	if ( ed == NULL )
		return NOMEM;
		
	ed->ed_Unit = unit;
	MaxDevice = unit & ~XBUS_EXTERNAL;	/* set high water mark */
	
	/* Initialize Expansion Device structures */

	ed->ed_XferSpeed = 0;         /* default speed */
	ed->ed_XferRegister = XTYPE1(0,1,0)|XTYPE2(0,1,0)|XTYPE3(1,2,1)|XTYPE4(1,2,1);
	ed->ed_BurstSize = 0;         /* no burst chunking */
        ed->ed_HogTime = 0;           /* no DMA time limit */
	
	InitList(&ed->ed_Commands,"xbus cmd list");
	InitList(&ed->ed_DataPackets,"xbus data list");
	InitList(&ed->ed_WRDataPackets,"xbus wrdata list");
	InitList(&ed->ed_StatusPackets,"xbus status list");
	AddTail(&ExpDevList,(Node *)ed);
	Dev_table[unit&0xf] = ed;
	
	DBUG(("Issuing Identify command to %lx\n",unit));
		
	do {
		GrabXBus(unit);
	} while( AskForID( ed ) );

	ReadID( ed );

	SELECTXBUS(unit);	/* reselect */

	return 0;
}

static Item
expansionInit(dev)
Device *dev;
{
	int i;
	uint32 foo;
	ExpansionBus *xb = XBUS_ADDR;
	jmp_buf jb,*oldjmpbuf;
	uint32 oldquiet;
	static PlatformID platform;

	DBUG(("expansionInit\n"));
	/* There is no expansion DMA going on */

	InitList(&DMAReadyQ,"DMA Ready Queue");

	InitList(&ExpDevList,"Expansion Bus Device List");


	/* strobe the expansion bus to get the devices to assign */
	/* themselves addresses */

	oldjmpbuf = KernelBase->kb_CatchDataAborts;
	oldquiet  = KernelBase->kb_QuietAborts;

	/* get the platform information */
	memset(&platform, 0, sizeof platform);
	if (SuperQuerySysInfo(SYSINFO_TAG_PLATFORMID, &platform, sizeof platform)
	    != SYSINFO_SUCCESS)
	{
	    DBUG(("Couldn't locate platform ID from SysInfo\n"));
	    return MAKEKERR(ER_SEVERE, ER_C_NSTND, ER_Kr_CantOpen);
	}   

/* set the wave forms */
	foo = XTYPE1(0,1,0)|XTYPE2(0,1,0)|XTYPE3(1,2,1)|XTYPE4(1,2,1);
#ifdef notdef
	foo = XTYPE1(1,1,1)|XTYPE2(1,1,1)|XTYPE3(3,3,2)|XTYPE4(3,3,2);
#endif /* notdef */

	xb->xb_ExpType = foo;
	DBUG(("ExpType=%lx foo=%lx\n",xb->xb_ExpType,foo));

	/* strobe select 17 times to configure devices */
	DBUG(("Looking for Expansion Device\n"));

	for (i = 0; i < 17; i++)
		xb->xb_Sel[0] = 0;

	/* search for special device 15 */
	KernelBase->kb_CatchDataAborts = &jb;
	KernelBase->kb_QuietAborts = ABT_CLIOT;
	if (setjmp(jb) == 0) {
		uint8 poll;

		/*
		 * HW guys say its always going to
		 * be wired as external from now on
		 */
		xb->xb_Sel[0] = 15|XBUS_EXTERNAL;

		poll = (uint8)xb->xb_Poll[0];

		DBUG(("Poll15 = %lx\n",poll));
		if (poll & XBUS_POLL_DEV_OVERFLOW) {
			DBUG(("too many devices on Expansion Bus\n"));
			return MAKEKERR(ER_SEVERE,ER_C_NSTND,
				ER_Kr_XBUSOverFlow);
		}
		if (poll & XBUS_POLL_NEWDEV) {
			DBUG(("Unexpected newdev bit on\n"));
			return MAKEKERR(ER_SEVERE,ER_C_NSTND,
				ER_Kr_XBUSOverFlow);
		}
		/* enable interrupts for special device 15 */
		WRITEPOLL(XBUS_POLL_NEWDEV_INT_EN);
	}

	KernelBase->kb_CatchDataAborts = oldjmpbuf;
	KernelBase->kb_QuietAborts = oldquiet;
	/* Now look for 15 devices */
	DBUG(("Now look for 15 devices\n"));

	/*
	 * look for internal devices, but only on platforms other than the
	 * Scientific-Atlanta STT, which has a hardware bug that prevents it
	 * from using the Nicky device *unless* we pretend it's really
	 * external when it isn't.
	 */
	if (platform.mfgr != SYSINFO_MFGR_SA)
	{
	    KernelBase->kb_CatchDataAborts = &jb;
	    KernelBase->kb_QuietAborts = ABT_CLIOT;
	    if (setjmp(jb) == 0) {
		/* Maximum of MAX_EXP_UNITS devices */
		for (i = 0; i < MAX_EXP_UNITS; i++) {
		    uint32 poll;
		    ExpDev *ed;

		    xb->xb_Sel[0] = i;
		    poll = (uint8)xb->xb_Poll[0];

		    DBUG(("XBUS, found device i=%lx poll=%lx\n",i,poll));
#ifdef notdef
		    WRITEPOLL(0); 	/* pro xbus ram card had enable bits set */
#endif /* notdef */

		    NewExpDev(i);
		    ed = Dev_table[i];
		    VDBUG(("Device:%d flgs=%lx manu=%lx",
			   i,ed->ed_Flags,ed->ed_ManuIdNum));
		    VDBUG((" dev=%lx rev=%lx",ed->ed_ManuDevNum,ed->ed_ManuRevNum));
		    VDBUG((" manuflgs=%lx DrvrSize=%lx\n",
			   ed->ed_ManuFlags,ed->ed_LogicalSizeOfDriver));
		}
	    }
	    KernelBase->kb_CatchDataAborts = oldjmpbuf;
	    KernelBase->kb_QuietAborts = oldquiet;
	}

	i = MaxDevice+1;
	/* now look for external devices */
	DBUG(("Now look for external devices starting at:%d\n",i));

	KernelBase->kb_CatchDataAborts = &jb;
	KernelBase->kb_QuietAborts = ABT_CLIOT;
	if (setjmp(jb) == 0) {
		/* Maximum of MAX_EXP_UNITS devices */
		for (; i < MAX_EXP_UNITS; i++) {
		uint32 poll;
		ExpDev *ed;

		xb->xb_Sel[0] = (uint32)i|XBUS_EXTERNAL;
		poll = (uint8)xb->xb_Poll[0];

		DBUG(("XBUS, found device i=%d poll=%lx\n",i,poll));
#ifdef notdef
		WRITEPOLL(0); 	/* pro xbus ram card had enable bits set */
#endif /* notdef */

		DBUG(("new poll=%lx\n",poll));
		NewExpDev(i|XBUS_EXTERNAL);
		ed = Dev_table[i];
		VDBUG(("Device:%lx flgs=%lx manu=%lx",
			ed->ed_Unit,ed->ed_Flags,ed->ed_ManuIdNum));
		VDBUG((" dev=%lx rev=%lx",ed->ed_ManuDevNum,ed->ed_ManuRevNum));
		VDBUG((" manuflgs=%lx DrvrSize=%ld\n",
			ed->ed_ManuFlags,ed->ed_LogicalSizeOfDriver));
		}
	}
	KernelBase->kb_CatchDataAborts = oldjmpbuf;
	KernelBase->kb_QuietAborts = oldquiet;

	if (MaxDevice < 0) {
		/* No devices on expansion bus, punt */
		VDBUG(("no devices on Expansion Bus\n"));
		return MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_Kr_NoXBusHardware);
	}

	/* all devices that should have dipired have now dipired */
	/* We are now passed the point of no return */
	/* Set the lunch/launch bit in the kernel */
	{
		uint32 oldints;
		_3DOBinHeader *thdo = (_3DOBinHeader *)(SherryHeader + 1);
		oldints = Disable();
		thdo->_3DO_Flags |= _3DO_LUNCH;
		Enable(oldints);
	}

	StartDma(0);	/* Set Default to DMA */
	DBUG(("xbus MaxDevice=%d\n",MaxDevice));
	dev->dev_MaxUnitNum = MaxDevice;

	xbusintItem = SuperCreateFIRQ("xbus",200,cExpInt,INT_EXPANSION);
	if (xbusintItem < 0)
		return xbusintItem;
	EnableInterrupt(INT_EXPANSION);
	DBUG(("address of XbusInt handler=%lx\n",(long)cExpInt));

	xdmaintItem = SuperCreateFIRQ("xbusdma",200,cExpDmaInt,INT_DMA_EXP);
	if (xdmaintItem < 0)
		return xbusintItem;
	EnableInterrupt(INT_DMA_EXP);
	DBUG(("address of XDmaInt handler=%lx\n",(long)cExpDmaInt));

	return dev->dev.n_Item;
}

static int
CmdDrainDataFifo(ior)
IOReq *ior;
{
	int unit = ior->io_Info.ioi_Unit;
	int len = (int)ior->io_Info.ioi_Recv.iob_Len;
	ExpansionBus *xb = XBUS_ADDR;
	ExpDev *ed = Dev_table[unit];
	uint8 oldunit;

	/*
	 * immediate only command
	 * reads the recv # of bytes from the data fifo and
	 * chucks them
	 */
	DBUG(("CmdDrainDataFifo\n"));
	oldunit = GrabXBus(ed->ed_Unit);
	CpuXferInAndChuck(&xb->xb_Data[0],len);
	ReleaseXBus(oldunit);

	return 1;
}

static int
CmdRead(ior)
IOReq *ior;
{
	/* Command Read */
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];
	uint32 oldints;

	DBUG(("xbus:CmdRead\n"));

	ior->io_Flags &= ~IO_QUICK;	/* clear the quick bit */
	oldints = Disable();
	if (ed->ed_DataiorWorking) {
		AddTail(&ed->ed_DataPackets,(Node *)ior);
	} else {
		BeginXferDataIn(ior);
	}
	Enable(oldints);
	return 0;
}

static int
CmdCommand(ior)
IOReq *ior;
{
	/* Send a stream of command bytes to the command fifo */
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];
	uint32 oldints;

	DBUG(("xbus:CmdCommand(%lx) ed=%lx ",(long)ior,ed));
	DBUG(("byte0=%lx\n",*(uint8 *)ior->io_Info.ioi_Send.iob_Buffer));

#ifdef undef
	ior->io_Flags &= ~XBUS_IO_SyncStatus;
#endif
	ior->io_Flags &= ~IO_QUICK;	/* clear the quick bit */
	oldints = Disable();
	if (ed->ed_iorWorking) {
		InsertNodeFromTail(&ed->ed_Commands,(Node *)ior);
	}
	else DoXBusCommand(ior,ed);
	Enable(oldints);
	return 0;
}


static int
CmdStatus(ior)
IOReq *ior;
{
	/* return the Poll byte */
	uint8 *dst = (uint8 *)ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	int unit = ior->io_Info.ioi_Unit;
	struct XBusDeviceStatus xbs;
	struct ExpDev *ed = Dev_table[unit];

	DBUG(("CmdStatus unit=%ld, len=%ld\n",unit,len));

	if (len == 0)
		return 1;
	if (len < 0)
		goto abort;

	if (len == 1) {
		uint8 oldunit;
		uint32 oldints;
		oldints = Disable();
		oldunit = GrabXBus(ed->ed_Unit);
		*dst = ReadPoll(ed->ed_Unit);
		ReleaseXBus(oldunit);
		Enable(oldints);
		ior->io_Actual = 1;
		return 1;
	}
	/* expanded status request */
	memset(&xbs,0,sizeof(xbs));
	xbs.xbus_ds.ds_MaximumStatusSize = sizeof(xbs);
	xbs.xbus_ManuIdNum = ed->ed_ManuIdNum;
	xbs.xbus_ManuDevNum = ed->ed_ManuDevNum;
	xbs.xbus_ManuRevNum = ed->ed_ManuRevNum;
	xbs.xbus_ManuFlags = ed->ed_ManuFlags;
	xbs.xbus_Flags = ed->ed_Flags;
	xbs.xbus_XferSpeed = ed->ed_XferSpeed;
	xbs.xbus_BurstSize = ed->ed_BurstSize;
	xbs.xbus_HogTime = ed->ed_HogTime;

	if (len > sizeof(xbs))len = sizeof(xbs)
		;
	memcpy(dst,&xbs,len);
	ior->io_Actual = len;
	return 1;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdWrite(ior)
IOReq *ior;
{
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];
	uint32 oldints;

	DBUG(("xbus:CmdWrite\n"));

	ior->io_Flags &= ~IO_QUICK;	/* clear the quick bit */
	oldints = Disable();
	if (ed->ed_WRDataiorWorking) {
		AddTail(&ed->ed_WRDataPackets,(Node *)ior);
	} else {
		BeginXferDataOut(ior);
	}
	Enable(oldints);
	return 0;
}

static int
CmdSnarfStatusBytes(IOReq *ior)
{
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];
	uint32 oldints;

	DBUG(("xbus:CmdSnarfStatusBytes\n"));
	
	ior->io_Flags &= ~IO_QUICK;	/* clear the quick bit */
	oldints = Disable();
	if (ed->ed_StatusWorking) {
		AddTail(&ed->ed_StatusPackets,(Node *)ior);
	} else {
		PrimeStatus(ior);
	}
	Enable(oldints);
	return 0;
}

static int
CmdSetXferSpeed(IOReq *ior)
{
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];
	uint32 oldints;
	uint32 speedRate, speedCode, ticks;

	DBUG(("xbus:CmdSetXferSpeed\n"));
	
	oldints = Disable();
	speedRate = ior->io_Info.ioi_Offset;
	if (speedRate == 0) {
	  ed->ed_XferSpeed = 0;         /* default speed */
	  ed->ed_XferRegister = XTYPE1(0,1,0)|XTYPE2(0,1,0)|XTYPE3(1,2,1)|XTYPE4(1,2,1);
	  Enable(oldints);
	  DBUG(("xbus:  set xfer speed of unit %d to default\n", unit));
	} else if (speedRate >= XBUS_MIN_PERIOD &&
		   speedRate <= XBUS_MAX_PERIOD) {
	  ed->ed_XferSpeed = speedRate;
	  ticks = (speedRate + 39) / 40;
	  speedCode = xbus_speed_codes[ticks];
	  ed->ed_XferRegister = XTYPE1(0,1,0) | XTYPE3(1,2,1) |
	    (speedCode << XB_TYPE2Shift) | (speedCode << XB_TYPE4Shift);
	  Enable(oldints);
	  DBUG(("xbus:  set xfer speed of unit %d to %d ns, %d ticks\n",
	       unit, speedRate, ticks));
	} else {
	  ior->io_Error = BADIOARG;
	}
	Enable(oldints);
	return 1;
}

static int
CmdSetBurstSize(IOReq *ior)
{
#ifdef BURSTSUPPORTED
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];
	ed->ed_BurstSize = ior->io_Info.ioi_Offset;
#else
	ior->io_Error = BADCOMMAND;
#endif
	return 1;
}

static int
CmdSetHogTime(IOReq *ior)
{
#ifdef HOGTIMESUPPORTED
	int unit = ior->io_Info.ioi_Unit;
	ExpDev *ed = Dev_table[unit];
	ed->ed_HogTime = ior->io_Info.ioi_Offset;
#else
	ior->io_Error = BADCOMMAND;
#endif
	return 1;
}

static int
CmdWaitDipirStart(IOReq *ior)
{
	uint32 oldints;

	if (!DipirWaitInitDone) 
		DipirWaitInit();
	ior->io_Flags &= ~IO_QUICK;	/* clear the quick bit */
	oldints = Disable();
	AddTail(&WaitDipirStartList, (Node *)ior);
	Enable(oldints);
	return 0;
}

static int
CmdWaitDipirEnd(IOReq *ior)
{
	uint32 oldints;

	if (!DipirWaitInitDone) 
		DipirWaitInit();
	ior->io_Flags &= ~IO_QUICK;	/* clear the quick bit */
	oldints = Disable();
	AddTail(&WaitDipirEndList, (Node *)ior);
	Enable(oldints);
	return 0;
}

vuint32 dummyVar = 0;
#define	LoopsPerMillisec 650 /* This depends on the processor/memory speed! */

static void
Spin(int msec)
{
	uint32 loops;

	while (msec-- > 0) 
		for (loops = 0;  loops < LoopsPerMillisec;  loops++) 
			dummyVar++;
}

static int
CmdResetUnit(IOReq *ior)
{
	ExpansionBus *xb = XBUS_ADDR;

	/*
	 * For now, reset the entire XBus.
	 * Since the Creative/Soundblaster system is the only one
	 * that calls this, and Creative can have only one XBus device,
	 * this is ok for now.
	 */
	xb->xb_SetExpCtl = XB_EXPRESET;
	Spin(100);
	xb->xb_ClrExpCtl = XB_EXPRESET;
	Spin(600);
	return 0;
}

#ifdef XCREATEIO
int
XcreateIOR(ior)
IOReq *ior;
{
	Task *ct = KernelBase->kb_CurrentTask;

	if ((ct->t.n_Flags & TASK_SUPER) == 0)
		return BADPRIV; 
	return 0;
}
#endif

#define MAXCOMMANDS 12
static int (*CmdTable[])() =
{
	CmdWrite,
	CmdRead,
	CmdStatus,
	CmdCommand,
	CmdDrainDataFifo,
	CmdSnarfStatusBytes,
	CmdSetXferSpeed,
	CmdSetBurstSize,
	CmdSetHogTime,
	CmdWaitDipirStart,
	CmdWaitDipirEnd,
	CmdResetUnit
};


int32
XDispatch( IOReq *ior )
{
  Task *t;
  int32 ret, cmd;
  
  cmd = ior->io_Info.ioi_Command;

  /* if the callback is set, then the task must be priv'd */
  /* otherwise, check the IOReq owner's task */
  if( ior->io_CallBack == NULL ) {
    t = (Task *)LookupItem( ior->io.n_Owner );
    if( !t || !(t->t.n_Flags & TASK_SUPER) ) {
      INFO(("ERROR: non-priv task attempting access to xbus!!\n"));
      ior->io_Error = BADPRIV;
      ret = 1;
      goto done;
    }
  }
   
  if ( cmd >= MAXCOMMANDS ) {
    ior->io_Error = BADCOMMAND;
    ret = 1;
  } else 
    ret = (CmdTable[cmd])(ior);
  
  if (ret == 0) {
    /*ior->io_Flags &= ~IO_QUICK;*/
    return 0;
  }
  
 done:
  if ( (ior->io_Flags & IO_INTERNAL) == 0)
    SuperCompleteIO(ior);
  return 1;
}


static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	TAG_ITEM_NAME,	"xbus",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)myAbortIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)MAXCOMMANDS,
	CREATEDRIVER_TAG_DISPATCH,	(void *)((int)(XDispatch)),
	TAG_END,		0,
};

static TagArg devArgs[] =
{
	TAG_ITEM_PRI,	(void *)3,
	CREATEDEVICE_TAG_DRVR,	(void *)1,
	TAG_ITEM_NAME,	"xbus",
	CREATEDEVICE_TAG_INIT,	(void *)((long)expansionInit),
#ifdef XCREATEIO
	CREATEDEVICE_TAG_CRIO,	(void *)((int)(XcreateIOR)),
#endif
	TAG_END,		0,
};

Item
createExpansionDriver(void)
{
	Item devItem = -1;
	Item drvrItem;

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
	KDBUG(("Creating Expansion driver returns drvrItem=%d\n",drvrItem));
	if (drvrItem >= 0) {
		devArgs[1].ta_Arg = (void *)drvrItem;
		devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),devArgs);
		if (devItem < 0)
			DeleteItem(drvrItem);
	}
	return devItem;
}
