/* $Id: mac.c,v 1.34 1994/09/27 18:55:13 shawn Exp $ */
/* file: macdriver.c */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "strings.h"
#include "task.h"
#include "mem.h"
#include "kernel.h"
#include "interrupts.h"
#include "operror.h"
#include "super.h"

#include "debug.h"

extern Superkprintf(const char *fmt, ... );
extern SuperDebugTrigger(void);
extern SuperPause(void);

#define DBUG(x)	/*Superkprintf x*/
#define KBUG(x)	/*kprintf x*/

#define PCKTSIZE	sizeof (debugio)

/* Where we construct the packets to send to the mac */
static Item IOReqItem;
static Item DeviceItem;	/* for lomac device */
static IOReq *lomacior;
static Item firqItem;

static IOReq *iorWorking;
static List iorqs;

static List iorAsking;

/* extra stuff needed in ioreq block */
typedef struct macstuff
{
	int32 len;
}  macstuff;

static void
myAbortIO(ior)
IOReq *ior;
{
}

extern void macInt(void);

int32 cmacInt(void);

/*#define OLDCREATE*/

#ifdef OLDCREATE
static
TagArg FirqTags[] =
{
	TAG_ITEM_PRI,	(void *)5,
	TAG_ITEM_NAME,	"macask",
	CREATEFIRQ_CODE, (void *)(int)cmacInt,
	CREATEFIRQ_NUM, (void *)INT_V1,
	TAG_END,	(void *)0,
};
#endif

char lmacname[]="lomac";

#ifdef undef
static TagArg IOArgs[] =
{
	CREATEIOREQ_TAG_DEVICE, 0,
	TAG_END, 0,
};
#endif

static Item
macInit(dev)
Driver *dev;
{
	InitList(&iorqs,"mac.iorqlist");
	InitList(&iorAsking,"mac.Askiorqlist");
	DeviceItem = SuperOpenItem(SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),lmacname),0);
        if (DeviceItem < 0)
	{
#ifdef DEBUG
		Superkprintf("Error openning:%s\n",lmacname);
#endif
		return DeviceItem;
	}
#ifdef undef
	IOArgs[0].ta_Arg = (void *)DeviceItem;
	IOReqItem = SuperCreateItem(MKNODEID(KERNELNODE,IOREQNODE),
					(void *)IOArgs);
#endif
	IOReqItem = SuperCreateIOReq(0,0,DeviceItem,0);
	if (IOReqItem < 0)
	{
		Superkprintf("Error creating ioreq\n");
		SuperCloseItem(DeviceItem);
		return IOReqItem;
	}
#ifdef OLDCREATE
	firqItem = SuperCreateItem(MKNODEID(KERNELNODE,FIRQNODE),FirqTags);
#else
	firqItem = SuperCreateFIRQ("macask",5,cmacInt,INT_V1);
#endif
	if (firqItem < 0)
	{
		Superkprintf("Error creating macfirq\n");
		SuperDeleteItem(IOReqItem);
		SuperCloseItem(DeviceItem);
		return firqItem;
	}
	lomacior = (struct IOReq *)LookupItem(IOReqItem);
	return dev->drv.n_Item;
}

static int
CmdStatus(ior)
IOReq *ior;
{
	DeviceStatus *dst = (DeviceStatus *)ior->io_Info.ioi_Recv.iob_Buffer;
	DeviceStatus mystat;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;

	KBUG(("CmdStatus dst=%lx len=%d\n",dst,len));
	if (len < 8)	goto abort;
	memset(&mystat,0,sizeof(DeviceStatus));

	mystat.ds_DriverIdentity = DI_OTHER;
	mystat.ds_MaximumStatusSize = sizeof(DeviceStatus);
	mystat.ds_DeviceFlagWord = DS_DEVTYPE_OTHER;

	if (len > sizeof(DeviceStatus)) len = sizeof(DeviceStatus);
	memcpy(dst,&mystat,len);
	ior->io_Actual = len;

	return 1;

abort:
	ior->io_Error = BADIOARG;
	return 1;


}

void
InitPacket(ior,len)
IOReq *ior;
int32 len;
{
	debugio *dbg = (debugio *)ior->io_Extension[0];
	int *p = (int *)ior->io_Extension[0];
	macstuff *q = (macstuff *)&p[PCKTSIZE/sizeof(int)];
	ior->io_Extension[1] = 1; /* illegal Mac error */
	q->len = len;	/* save here for later */
	dbg->reqOwner = -1;	/* packet for the debugger */
	dbg->reqStatusPtr = &ior->io_Extension[1];
}

IOReq * macCallBack(IOReq *ior);

void ioerror(void);

int32
cmacInt(void)
{
	/* Only used for the Mac Ask command */
	IOReq *m,*n;
	for ( m = (IOReq *)FIRSTNODE(&iorAsking) ; ISNODE(&iorAsking,m);
		m = n)
	{	/* currently have an outstanding Ask Io pending */
		int stat = (int)m->io_Extension[1];
		n = (IOReq *)NEXTNODE(m);
		if (stat != 1)
		{
		    RemNode((Node *)m);
		    if (stat == 0)	/* Now really done and no errors */
			m->io_Actual = (int32)strlen((char *)m->io_Info.ioi_Recv.iob_Buffer);
		    m->io_Error = stat;
		    SuperCompleteIO(m);
		    /* Now duplicate code that is in the callback */
#ifdef undef
	/* The following code is being ifdefed out since I think */
	/* it is wrong and it would never be used in a call back manner */
		    if (iorWorking == 0)
		    {
			m = (IOReq *)RemHead(&iorqs);
			if (m)
				StartNextPacket();	/* is this cool? */
		    }
#endif
		}
	}
	return 0;
}

void
PreparePacket(IOReq *ior)
{
	lomacior->io_Info.ioi_Command = CMD_WRITE;
	lomacior->io_Info.ioi_Send.iob_Buffer = (void *)ior->io_Extension[0];
	lomacior->io_Info.ioi_Send.iob_Len = 32;
	lomacior->io_Info.ioi_Offset = 0;
	lomacior->io_Info.ioi_Flags = IO_QUICK;
	lomacior->io_CallBack = macCallBack;
}

IOReq *
StartNextPacket(callback)
int callback;
{
    iorWorking = (IOReq *)RemHead(&iorqs);
    if (iorWorking)
    {
 	/*
 	 *	on ASK cmd, we enable printf, which may have been
 	 *	disabled by some titles (RoadRash).
 	 */
 	if (iorWorking->io_Info.ioi_Command == MACCMD_ASK)
 		Superkprintf("%c",2);
	PreparePacket(iorWorking);
	if (callback == 0) SuperinternalSendIO(lomacior);
	return lomacior;
    }
    return 0;
}

void
SendPacket(IOReq *ior)
{
	/* We are in Super mode */
	int32 ret;
	uint32 oldints;
	oldints = Disable();

	ior->io_Flags &= ~IO_QUICK;
	InsertNodeFromTail(&iorqs,(Node *)ior);
	if (iorWorking)
	{
	    Enable(oldints);
	    return;
	}
	StartNextPacket(0);
	Enable(oldints);
}


IOReq *
macCallBack(ior)
IOReq *ior;
{
	IOReq *myio;
	/* called back when lomac finished with io request */
	IOReq *nextior = 0;

	/*Enable(0x40);*/
	/* DBUG(("macCallBack:ior=%lx\n",ior));*/
	myio = iorWorking;
	if (myio == 0)
	{
		ioerror();
		while (1);
		return 0;
	}
	myio->io_Error = myio->io_Extension[1];
	/*DBUG(("io_Error=%ld\n",myio->io_Error));*/
	if (myio->io_Error == 0)
	{
		int *p = (int *)myio->io_Extension[0];
		macstuff *q = (macstuff *)&p[PCKTSIZE/sizeof(int)];
		myio->io_Actual = q->len;
	}
	/* Special code for Ask Command */
	if (myio->io_Info.ioi_Command == MACCMD_ASK)
	{
		/* can't complete it right now */
		IOReq *n;
		/* kill all pending Asks */
		for (n = (IOReq *)FIRSTNODE(&iorAsking);
			ISNODE(&iorAsking,n);
			n = (IOReq *)NEXTNODE(n))
		{
		    n->io_Extension[1] = -1;
		}
		AddTail(&iorAsking,(Node *)myio);
	}
	else SuperCompleteIO(myio);

	iorWorking = 0;
	return StartNextPacket(1);
}

static int
CmdRead(ior)
IOReq *ior;
{
	/* Command Read */
	int32  offset = ior->io_Info.ioi_Offset;
	void *dst = ior->io_Info.ioi_Recv.iob_Buffer;

	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	debugio *dbg = (debugio *)(ior->io_Extension[0]);
	char *fname, *cpoint;
	int i;
	int32 llen;


	DBUG(("CmdREAD len=%d fname=%s\n",len,fname));
	DBUG(("buffer=%lx\n",dst));
	DBUG(("offset=%lx\n",offset));

	/*if (offset < 0) goto abort;*/
	if (len == 0) return 1;
	if (len < 0) goto abort;

	cpoint = (char *)ior->io_Info.ioi_Send.iob_Buffer;
	llen = strlen(cpoint) < MACNAMEBUFBYTES ? strlen(cpoint) : (MACNAMEBUFBYTES-1);
	fname = dbg->namebuf;
	memset(fname,0,MACNAMEBUFBYTES);
	for(i=0; i<llen; i++) {
	    fname[i] = *cpoint != '/' ? *cpoint : ':';
	    cpoint++;
	}

	InitPacket(ior,len);
	dbg->reqCommand = 2;	/* Read File */
	dbg->ptrs[2] = len;
	dbg->ptrs[3] = offset;
	dbg->ptrs[1] = (ulong)dst;

	dbg->ptrs[0] = (ulong)fname;

#ifdef undef
	DBUG(("pck buff=%lx\n",(ulong)dbg));
#endif

	SendPacket(ior);

	return 0;
abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdWrite(ior)
IOReq *ior;
{
	/* Command Write */
	void *src = ior->io_Info.ioi_Send.iob_Buffer;
	int32 len = ior->io_Info.ioi_Send.iob_Len;

	debugio *dbg = (debugio *)(ior->io_Extension[0]);
	int32  offset = ior->io_Info.ioi_Offset;
	char *fname, *cpoint;
	int i;
	int32 llen;

	DBUG(("CmdWrite len=%d filename=%s\n",len,fname));
	DBUG(("buffer=%lx offset=%lx\n",src,offset));


	if (offset < 0) goto abort;
	if (len == 0) return 1;
	if (len < 0) goto abort;

	cpoint = (char *)ior->io_Info.ioi_Recv.iob_Buffer;
	llen = strlen(cpoint) < MACNAMEBUFBYTES ? strlen(cpoint) : (MACNAMEBUFBYTES-1);
	fname = dbg->namebuf;
	memset(fname,0,MACNAMEBUFBYTES);
	for(i=0; i<llen; i++) {
	    fname[i] = *cpoint != '/' ? *cpoint : ':';
	    cpoint++;
	}

	InitPacket(ior,len);
	dbg->reqCommand = 7;	/* Write File */
	dbg->ptrs[2] = len;
	dbg->ptrs[1] = (ulong)src;
	dbg->ptrs[0] = (ulong)fname;
	dbg->ptrs[3] = offset;

	DBUG(("pkt.buff=%lx\n",(ulong)dbg));

	SendPacket(ior);

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

void
ioerror()
{
}

static int
CmdAppend(ior)
IOReq *ior;
{
	/* Command Write */
	void *src = ior->io_Info.ioi_Send.iob_Buffer;
	int32 len = ior->io_Info.ioi_Send.iob_Len;
	debugio *dbg = (debugio *)(ior->io_Extension[0]);
	int32  offset = ior->io_Info.ioi_Offset;
	char *fname, *cpoint;
	int i;
	int32 llen;

	DBUG(("CmdAppend len=%d filename=%s\n",len,fname));
	DBUG(("buffer=%lx offset=%lx\n",src,offset));


	if (offset < 0) goto abort;
	if (len == 0) return 1;
	if (len < 0) goto abort;

	cpoint = (char *)ior->io_Info.ioi_Recv.iob_Buffer;
	llen = strlen(cpoint) < MACNAMEBUFBYTES ? strlen(cpoint) : (MACNAMEBUFBYTES-1);
	fname = dbg->namebuf;
	memset(fname,0,MACNAMEBUFBYTES);
	for(i=0; i<llen; i++) {
	    fname[i] = *cpoint != '/' ? *cpoint : ':';
	    cpoint++;
	}
	InitPacket(ior,len);
	dbg->reqCommand = 4;	/* Write File */
	dbg->ptrs[2] = len;
	dbg->ptrs[1] = (ulong)src;
	dbg->ptrs[0] = (ulong)fname;
	dbg->ptrs[3] = offset;

	DBUG(("pkt.buff=%lx\n",(ulong)dbg));

	SendPacket(ior);

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdPrint(ior)
IOReq *ior;
{
	/* printf action request hack */
	/* Command Write */
	int32  offset = ior->io_Info.ioi_Offset;
	char *dst;
	void *src = ior->io_Info.ioi_Send.iob_Buffer;
	int32 xferlen;
	int32 len = ior->io_Info.ioi_Send.iob_Len;
	debugio *dbg = (debugio *)(ior->io_Extension[0]);

	DBUG(("CmdPrint len=%d\n",len));
	DBUG(("buffer=%lx\n",src));
	DBUG(("offset=%lx\n",offset));

	if (len == 0) return 1;

	if (offset < 0) goto abort;
	if (len < 0) goto abort;

	if (len > 200) len = 200;
	xferlen = len;

	dst = (char *)(dbg + 1);

	InitPacket(ior,len);
	dbg->ptrs[0] = (ulong)src;

	dbg->reqCommand = 1;

	SendPacket(ior);

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdAsk(ior)
IOReq *ior;
{
	int32  offset = ior->io_Info.ioi_Offset;
	void *src = ior->io_Info.ioi_Send.iob_Buffer;
	int32 xferlen;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	debugio *dbg = (debugio *)(ior->io_Extension[0]);
	
	DBUG(("CmdAsk len=%d\n",len));
	DBUG(("buffer=%lx\n",src));
	DBUG(("offset=%lx\n",offset));
	DBUG(("ior=%lx\n",ior));
	DBUG(("&iorWorking=%lx\n",&iorWorking));

	if (len == 0) return 1;

	if (offset < 0) goto abort;
	if (len < 0) goto abort;

	if (len > 96) len = 96;
	xferlen = len;

	dbg->ptrs[0] = (ulong)src;
	dbg->ptrs[1] = xferlen;
	dbg->ptrs[2] = (int)ior->io_Info.ioi_Recv.iob_Buffer;

	InitPacket(ior,len);
	dbg->reqCommand = 5;

	SendPacket(ior);
	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdFileLen(ior)
IOReq *ior;
{
	/* Command FileLen */
	void *dst = ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	debugio *dbg = (debugio *)(ior->io_Extension[0]);
	int32  offset = ior->io_Info.ioi_Offset;

	char *fname, *cpoint;
	int i;
	int32 llen;

	DBUG(("CmdFileLen len=%d filename=%s\n",len,fname));
	DBUG(("buffer=%lx offset=%lx\n",dst,offset));

	DBUG(("iorWorking=%lx\n",iorWorking));


	/*if ((int)(fname) & 3) goto abort;*/


	if (offset < 0) goto abort;
	if (len == 0)	return 1;
	if (len < 4) goto abort;

	cpoint = (char *)ior->io_Info.ioi_Send.iob_Buffer;
	llen = strlen(cpoint) < MACNAMEBUFBYTES ? strlen(cpoint) : (MACNAMEBUFBYTES-1);
	fname = dbg->namebuf;
	memset(fname,0,MACNAMEBUFBYTES);
	for(i=0; i<llen; i++) {
	    fname[i] = *cpoint != '/' ? *cpoint : ':';
	    cpoint++;
	}
	DBUG(("CMDFileLen filename=%s\n",fname));

	InitPacket(ior,len);
	dbg->reqCommand = 6;
	dbg->ptrs[2] = len;
	dbg->ptrs[1] = (ulong)dst;
	dbg->ptrs[0] = (ulong)fname;
	dbg->ptrs[3] = offset;

	DBUG(("pkt.buff=%lx\n",(ulong)dbg));
	/*DBUG(("FileLen:iorWorking=%lx\n:",iorWorking));*/

	SendPacket(ior);

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

#ifdef undef
static int
CmdFileInfo(ior)
IOReq *ior;
{
	/* Command FileInfo */
	void *dst = ior->io_Info.ioi_Recv.iob_Buffer;
	int len = ior->io_Info.ioi_Recv.iob_Len;
	char *fname = (char *)ior->io_Info.ioi_Send.iob_Buffer;
	debugio *dbg = (debugio *)(ior->io_Extension[0]);
	int  offset = ior->io_Info.ioi_Offset;

	DBUG(("CmdFileInfo len=%d filename=%s\n",len,fname));
	DBUG(("buffer=%lx offset=%lx\n",dst,offset));

	if (offset < 0) goto abort;
	if (len == 0) return 1;
	if (len < 4) goto abort;

	InitPacket(ior,len);
	dbg->reqCommand = 9;
	dbg->ptrs[1] = (ulong)dst;
	dbg->ptrs[0] = (ulong)fname;

	DBUG(("pkt.buff=%lx\n",(ulong)dbg));
	/*DBUG(("FileLen:iorWorking=%lx\n:",iorWorking));*/

	SendPacket(ior);

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}
#endif

static int
CmdReadDir(ior)
IOReq *ior;
{
	/* Command ReadDir */
	void *dst = ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	debugio *dbg = (debugio *)(ior->io_Extension[0]);
	int32  offset = ior->io_Info.ioi_Offset;

	char *fname, *cpoint;
	int i;
	int32 llen;

	DBUG(("CmdReadDir len=%d filename=%s\n",len,fname));
	DBUG(("buffer=%lx offset=%lx\n",dst,offset));


	if (offset < 0) goto abort;
	if (len == 0) return 1;
	if (len < 96) goto abort;

	cpoint = (char *)ior->io_Info.ioi_Send.iob_Buffer;
	llen = strlen(cpoint) < MACNAMEBUFBYTES ? strlen(cpoint) : (MACNAMEBUFBYTES-1);
	fname = dbg->namebuf;
	memset(fname,0,MACNAMEBUFBYTES);
	for(i=0; i<llen; i++) {
	    fname[i] = *cpoint != '/' ? *cpoint : ':';
	    cpoint++;
	}

	InitPacket(ior,len);
	dbg->reqCommand = 10;
	dbg->ptrs[1] = (ulong)dst;
	dbg->ptrs[0] = (ulong)fname;
	dbg->ptrs[2] = len;
	dbg->ptrs[3] = offset;

	DBUG(("pkt.buff=%lx\n",(ulong)dbg));
	/*DBUG(("FileLen:iorWorking=%lx\n:",iorWorking));*/

	SendPacket(ior);

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdWriteCr(ior)
IOReq *ior;
{
	/* Command Write */
	void *src = ior->io_Info.ioi_Send.iob_Buffer;
	int32 len = ior->io_Info.ioi_Send.iob_Len;
/*	char *fname = (char *)ior->io_Info.ioi_Recv.iob_Buffer; */
	debugio *dbg = (debugio *)(ior->io_Extension[0]);
	int32  offset = ior->io_Info.ioi_Offset;
	char *fname, *cpoint;
	int i;
	int32 llen;

	DBUG(("CmdWriteCR len=%d filename=%s\n",len,fname));
	DBUG(("buffer=%lx offset=%lx\n",src,offset));

	if (offset < 0) goto abort;
	if (len == 0) return 1;
	if (len < 0) goto abort;

	cpoint = (char *)ior->io_Info.ioi_Recv.iob_Buffer;
	llen = strlen(cpoint) < MACNAMEBUFBYTES ? strlen(cpoint) : (MACNAMEBUFBYTES-1);
	fname = dbg->namebuf;
	memset(fname,0,MACNAMEBUFBYTES);
	for(i=0; i<llen; i++) {
	    fname[i] = *cpoint != '/' ? *cpoint : ':';
	    cpoint++;
	}
	InitPacket(ior,len);
	dbg->reqCommand = 3;	/* Write File */
	dbg->ptrs[2] = len;
	dbg->ptrs[1] = (ulong)src;
	dbg->ptrs[0] = (ulong)fname;
	dbg->ptrs[3] = offset;

	DBUG(("pkt.buff=%lx\n",(ulong)dbg));

	SendPacket(ior);

	return 0;

abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdReadCDDelay(ior)
IOReq *ior;
{
	/* Command Read */
	int32  offset = ior->io_Info.ioi_Offset;
	void *dst = ior->io_Info.ioi_Recv.iob_Buffer;
/*	char *fname = (char *)ior->io_Info.ioi_Send.iob_Buffer; */
	int32 len = ior->io_Info.ioi_Recv.iob_Len;
	debugio *dbg = (debugio *)(ior->io_Extension[0]);

	char *fname, *cpoint;
	int i;
	int32 llen;

	DBUG(("CmdREAD len=%d fname=%s\n",len,fname));
	DBUG(("buffer=%lx\n",dst));
	DBUG(("offset=%lx\n",offset));

	/*if (offset < 0) goto abort;*/
	if (len == 0) return 1;
	if (len < 0) goto abort;

	cpoint = (char *)ior->io_Info.ioi_Send.iob_Buffer;
	llen = strlen(cpoint) < MACNAMEBUFBYTES ? strlen(cpoint) : (MACNAMEBUFBYTES-1);
	fname = dbg->namebuf;
	memset(fname,0,MACNAMEBUFBYTES);
	for(i=0; i<llen; i++) {
	    fname[i] = *cpoint != '/' ? *cpoint : ':';
	    cpoint++;
	}

	InitPacket(ior,len);
	dbg->reqCommand = 11;	/* Read File */
	dbg->ptrs[2] = len;
	dbg->ptrs[3] = offset;
	dbg->ptrs[1] = (ulong)dst;

	dbg->ptrs[0] = (ulong)fname;

#ifdef undef
	DBUG(("pck buff=%lx\n",(ulong)dbg));
#endif

	SendPacket(ior);

	return 0;
abort:
	ior->io_Error = BADIOARG;
	return 1;
}

extern struct KernelBase *KernelBase;

int
createIO(ior)
IOReq *ior;
{
	/* Need an extra (packet) buffer per io request */
	long *p = (long *)AllocMemFromMemLists(KernelBase->kb_MemFreeLists,
				PCKTSIZE+sizeof(macstuff),0);
	DBUG(("allocating extra memory for IOReq ior=%lx\n",ior));
	if (!p)	return -1;
	ior->io_Extension[0] = (long)p;
	return 0;
}

void
deleteIO(ior)
IOReq *ior;
{
#ifdef undef
	FREEMEM((void *)ior->io_Extension[0],PCKTSIZE+sizeof(macstuff));
#else
	FreeMemToMemLists(KernelBase->kb_MemFreeLists,
		(void *)ior->io_Extension[0],PCKTSIZE+sizeof(macstuff));
#endif
}

static int (*CmdTable[])() =
{
	CmdWrite,	/* 0 */
	CmdRead,	/* 1 */
	CmdStatus,	/* 2 */
	CmdPrint,	/* 3 */
	CmdAsk,		/* 4 */
	CmdAppend,	/* 5 */
	CmdFileLen,	/* 6 */
	CmdWriteCr,	/* 7 */
	CmdWriteCr,	/* 8 */
	CmdReadDir,	/* 9 */
	CmdReadCDDelay	/* 10 */
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	TAG_ITEM_NAME,	"mac",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)myAbortIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)11,
	CREATEDRIVER_TAG_CMDTABLE,	(void *)CmdTable,
	CREATEDRIVER_TAG_INIT,		(void *)((long)macInit),
	TAG_END,		0,
};

static TagArg devArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	CREATEDEVICE_TAG_DRVR,	0,
	CREATEDEVICE_TAG_CRIO,	(void *)((int)(createIO)),
	CREATEDEVICE_TAG_DLIO,	(void *)((int)(deleteIO)),
	TAG_ITEM_NAME,	"mac",
	TAG_END,		0,
};

extern void createLoMacDriver(void);

Item
createMacDriver(void)
{
	Item drvrItem;
	Item devItem = -1;

	createLoMacDriver();
	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
	KBUG(("Creating Mac driver returns devItem=%lx\n",drvrItem));
	if (drvrItem>=0)
	{
		devArgs[1].ta_Arg = (void *)drvrItem;
		devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),devArgs);
		KBUG(("Creating mac device returns devItem=%d\n",devItem));
	}
	else PrintError(0,"create mac driver item",0,drvrItem);
	return devItem;
}

