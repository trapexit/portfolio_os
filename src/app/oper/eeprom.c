/* $Id: eeprom.c,v 1.10 1994/11/19 03:26:58 deborah Exp $ */

/*
 * Source for the eeprom device driver. In normal systems, linked
 * into the operator. If compiled with EEPROMTASK defined (in GNUMakefile),
 * then becomes a separately launchable task. 
 * The separate task version was made as a proof-of-concept that
 * a device driver could be demand-loaded. THIS IS NOT COMPLETE.
 * The task version requires setFunction support in the kernel,
 * and will only launch with a leaky or extrarich dipir.
 * To use eepromtask, copy it into the $DEVICES directory,
 * change its name to "eeprom", and then use the normal eeprom
 * utility to access it. You do not need a version of the operator
 * with EEPROM defined, in fact, they will conflict.
 * See deborah for fuller detail.
 */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "strings.h"
#include "kernel.h"
#include "operror.h"
#include "super.h"
#include "mem.h"
#include "inthard.h"
#include "setjmp.h"
#include "time.h"

extern Superkprintf(const char *fmt, ... );
#define DEBUG

#define DBUG(x)	 /*kprintf x*/
#define KBUG(x)	 /*Superkprintf x*/
#define RRBDBUG(x)	 /*Superkprintf x*/

extern void	TimeStamp(struct timeval *tv);

#ifdef	EEPROMTASK
/*
 * This is a hack for now. When eeprom is part of operator,
 * tvCmp is accessible. Must be duplicated here because eepromtask
 * is separately launchable.
 */
int
tvCmp(tv1,tv2)
struct timeval *tv1,*tv2;
{
	/* return -1 if tv1 is less than tv2 */
	/* return 0 if tv1 == tv2 */
	/* return 1 if tv1 > tv2 */
	int d1;
	d1 = (int)tv1->tv_sec - (int)tv2->tv_sec;
	if (d1<0) return -1;
	if (d1>0) return 1;
	d1 = (int)tv1->tv_usec - (int)tv2->tv_usec;
	if (d1<0) return -1;
	if (d1>0) return 1;
	return 0;
}
#else	/* EEPROMTASK */
extern int	tvCmp(struct timeval *tv1, struct timeval *tv2);
#endif	/* EEPROMTASK */

static void
myAbortIO(ior)
IOReq *ior;
{
}

#define BLOCKSIZE	1

#define ONEMEG	(1024*1024)

#define EEPROM_ADDR	0x3600000
#define EEPROM_SIZE	ONEMEG

extern struct KernelBase *KernelBase;

Item
eepromInit(dev)
Device *dev;
{
	jmp_buf *oldjmpbuf;
	jmp_buf jb;

	volatile uint8 *p = (volatile uint8 *)EEPROM_ADDR;

	KBUG(("eepromInit\n"));
	KBUG(("flags=%lx\n",KernelBase->kb_CPUFlags));
	if (KernelBase->kb_CPUFlags & KB_NODBGR)
		return MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_NoHardware);

	oldjmpbuf = KernelBase->kb_CatchDataAborts;
	KernelBase->kb_CatchDataAborts = &jb;
	if (setjmp(jb))
	{
	    /* No eeprom there */
	    KBUG(("got an abort reading from eeprom space\n"));
	    KernelBase->kb_CatchDataAborts = oldjmpbuf;
	    return MAKEKERR(ER_SEVERE,ER_C_NSTND,ER_NoHardware);
	}
	KBUG(("Sample a byte in eeprom space\n"));
	if (*p) ;	/* read a byte to see if hardware exists */
	KernelBase->kb_CatchDataAborts = oldjmpbuf;
	/* it exists, I think */

	return dev->dev.n_Item;
}

static int
CmdRead(ior)
IOReq *ior;
{
    return 1;
}

void
xfer(char *src,char *dst)
{
    uint32 oldints;
    int i = 128/8;
    oldints = Disable();	/* no interrupts while this is going on */
    while (i--)
    {
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
	*dst++ = *src++;
    }
    Enable(oldints);
}

void
WaitFor(sec,usec)
int sec,usec;
{
    struct timeval tv,ctv;
    TimeStamp(&tv);
    tv.tv_usec += usec;
    if (tv.tv_usec > 1000000)
    {
	tv.tv_usec -= 1000000;
	tv.tv_sec++;
    }
    tv.tv_sec += sec;
    while (1)
    {
    	TimeStamp(&ctv);
	if (tvCmp(&tv,&ctv) <= 0) return;
    }
}

static int
CmdWrite(ior)
IOReq *ior;
{
	/* Command Write */
	int32  offset = ior->io_Info.ioi_Offset;
	char *dst = (char *)EEPROM_ADDR;
	char *src = (char *)ior->io_Info.ioi_Send.iob_Buffer;
	int32 len = ior->io_Info.ioi_Send.iob_Len;
	int i;

#ifdef DEBUG
	Superkprintf("eeprom:CmdWrite len=%d\n",len);
	Superkprintf("buffer=0x%lx len=%d\n",src,len);
	Superkprintf("offset=%lx\n",offset);
#endif
	if (len == 0)	return 1;
	if (offset != 0) goto abort;
	if (len > EEPROM_SIZE) goto abort;

	/* split this into 8 banks of 128k each */
	for (i = 0; i < 128*1024/128 ; i++)
	{
	    xfer(src+offset,dst+offset);
	    xfer(src+offset+128*1024,dst+offset+128*1024);
	    xfer(src+offset+2*128*1024,dst+offset+2*128*1024);
	    xfer(src+offset+3*128*1024,dst+offset+3*128*1024);
	    xfer(src+offset+4*128*1024,dst+offset+4*128*1024);
	    xfer(src+offset+5*128*1024,dst+offset+5*128*1024);
	    xfer(src+offset+6*128*1024,dst+offset+6*128*1024);
	    xfer(src+offset+7*128*1024,dst+offset+7*128*1024);
	    WaitFor(0,150);	/* wait for 150 usecs */
	    /* programming has started */
	    WaitFor(0,12000);	/* now wait for 15 msec */
	    offset += 128;
	}

	ior->io_Actual = len;
	return 1;

abort:
	ior->io_Error = BADIOARG;
	return 1;
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

static int (*CmdTable[])() =
{
	CmdWrite,
	CmdRead,
	CmdStatus,
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	TAG_ITEM_NAME,	"eeprom",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)myAbortIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)3,
	CREATEDRIVER_TAG_CMDTABLE,	(void *)CmdTable,
	TAG_END,		0,
};

static TagArg devArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	CREATEDEVICE_TAG_DRVR,		0,
	TAG_ITEM_NAME,	"eeprom",
	CREATEDEVICE_TAG_INIT,	(void *)((long)eepromInit),
	TAG_END,		0,
};

Item
createEEPROMDriver(void)
{
	Item drvrItem;
	Item devItem = 0;

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
	DBUG(("Creating EEPROM driver returns drvrItem=%d\n",drvrItem));
	if (drvrItem >= 0)
	{
		devArgs[1].ta_Arg = (void *)drvrItem;
		devItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),devArgs);
		kprintf("eeprom-devItem=%lx\n",devItem);
	}
	return devItem;
}

#ifdef	EEPROMTASK
int
main()
{
    if (createEEPROMDriver() > 0) WaitSignal(0);
}
#endif	/* EEPROMTASK */
