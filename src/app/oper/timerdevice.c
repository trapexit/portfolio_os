/* $Id: timerdevice.c,v 1.43 1994/10/20 17:06:16 vertex Exp $ */

#include "types.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "kernelnodes.h"
#include "debug.h"
#include "strings.h"
#include "interrupts.h"
#include "kernel.h"
#include "timer.h"
#include "time.h"
#include "operror.h"
#include "mem.h"
#include "super.h"


#define KDBUG(x)	 /*kprintf x*/
#define DBUG(x)	 /* Superkprintf x */
#define DBUGM(x)	 /* Superkprintf x */

extern uint32	USecToTicks(uint32);


/*****************************************************************************/


/* used in io_Flags of IO requests to mark them as metronome requests */
#define TIMERIOFLAGS_METRONOME 0x80000000

typedef struct TimerIOReq
{
    IOReq   timer_IOReq;

    /* amount of time between ticks of a metronome */
    TimeVal timer_Interval;
} TimerIOReq;


/*****************************************************************************/


/* Where we put ioreq waiting for time delay */
List DelayList;	/* vertical blanks */
List MDelayList; /* microsecond delays */

Item DelayClock;
Timer *timer;

static void
TimerAbortIO(ior)
IOReq *ior;
{
	/* If we get to here, the ior is in progress */
	/* and interrupts are disabled */
	/* All we have to do is take it off the list */
	/* and complete it */
	RemNode((Node *)ior);
	ior->io_Error = ABORTED;
	/* We let the timer alone since all it does is */
	/* wake us up and cause us to recheck to queue of */
	/* pending requests. */
	SuperCompleteIO(ior);
}

Item firqItem,timItem;
TimerDevice *td;

int32 ctimerInt(void);
int32 cMtimerInt(void);

int
tvTst(tv)
struct timeval *tv;
{
	int32 secs = tv->tv_sec;
	if (secs < 0)	return -1;
	if (secs > 0) return 1;
	if (tv->tv_usec) return 1;
	return 0;
}

void
tvSub(tv1,tv2)
struct timeval *tv1,*tv2;
{
	/* subtract tv2 from tv1 */
	int32 secs = tv1->tv_sec - tv2->tv_sec;
	int32 usecs = tv1->tv_usec - tv2->tv_usec;
	if (usecs < 0)
	{
	    secs--;
	    usecs += 1000000;
	}
	tv1->tv_sec = secs;
	tv1->tv_usec = usecs;
}

void
tvAdd(tv1,tv2)
struct timeval *tv1,*tv2;
{
	/* add tv2 to tv1 */
	uint32 secs = tv1->tv_sec + tv2->tv_sec;
	uint32 usecs = tv1->tv_usec + tv2->tv_usec;
	if (usecs >= 1000000)
	{
	    secs++;
	    usecs -= 1000000;
	}
	tv1->tv_sec = secs;
	tv1->tv_usec = usecs;
}

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
bool
tvCmpF(mm,nm)
/* mm is the new node, nm is already in the list */
Node *mm,*nm;
{
	/* mm is already in the list */
	/* nm is the node we want to insert */
	IOReq *m,*n;
	int i;
	DBUG(("mm=%lx nm=%lx\n",mm,nm));
	m = (IOReq *)mm; n = (IOReq *)nm;
	i = tvCmp((struct timeval *)m->io_Extension,
		  (struct timeval *)n->io_Extension);
	DBUG(("tvCmp = %d\n",i));
	if (i <= 0)	return 1;
	else return 0;
}

TagArg htTags[] =
{
	TAG_ITEM_NAME,  "utimer",
	CREATETIMER_TAG_NUM,	(void *)2,
	CREATETIMER_TAG_FLAGS,	(void *)TIMER_MUST_INTERRUPT,
	TAG_END,	0
};

int32 lastticks[2];
struct timeval lasttv;

int
reprocess()
/*	compute new target for system timer */
{
	IOReq *ior;
	int retval = 0;
	struct timeval tv;
	Err ret;

	/* stop the timer */
	/*(*timer->tm_Control)(timer,0,TIMER_DECREMENT|TIMER_RELOAD|TIMER_INTREQ);*/
	(*timer->tm_Control)(timer,0,TIMER_DECREMENT|TIMER_RELOAD|TIMER_INTEN);
again:
	ior = (IOReq *)FIRSTNODE(&MDelayList);
	if (ISNODE(&MDelayList,ior) )
	{
	    struct timeval target;
	    uint32 ticks;

	    target.tv_sec = ior->io_Extension[0];
	    target.tv_usec = ior->io_Extension[1];

	    TimeStamp(&tv);
	    tvSub(&target,&tv);	/* How far to go yet ? */
	    lasttv = target;
	    if (tvTst(&target) <= 0)
	    {
		RemNode((Node *)ior);

	        if (ior->io_Flags & TIMERIOFLAGS_METRONOME)
	        {
	            tvAdd((TimeVal *)ior->io_Extension,
                          &((TimerIOReq *)ior)->timer_Interval);

		    /* ping our client */
                    ret = SuperInternalSignal((Task *)LookupItem(ior->io.n_Owner),
                                              ior->io_Info.ioi_CmdOptions);
                    if (ret >= 0)
                    {
                        UniversalInsertNode(&MDelayList,(Node *)ior,tvCmpF);
                        goto again;
                    }

                    /* couldn't ping the client, we're done for... */
                    ior->io_Error = ret;
	        }

		if (ior->io_Flags & IO_INTERNAL) retval = 1;
		else    SuperCompleteIO(ior);
		goto again;
	    }
	    if (target.tv_sec)
	    {	/* if at least 1 second? */
		/* set to low 32bit mask, about 3 hours */
		target.tv_usec = 1000000;
	    }
	    ticks = USecToTicks(target.tv_usec);
	    if(ticks > 0)ticks--;
	    /* now set up timer */
	    /* load 32 bits */
	    lastticks[0] = (ticks>>16);
	    lastticks[1] = ticks;
	    (*timer->tm_Load)(timer->tm_ID,(ticks>>16),0);
	    (*timer->tm_Load)((int32)timer->tm_ID-1,ticks,0xffff);

	    (*timer->tm_Control)(timer, TIMER_RELOAD|TIMER_DECREMENT|TIMER_INTEN,0);
	    /* Enable timer interrupts */
	}
	return retval;
}

int32
ctimerInt()
	/* called by interrupt routine */
{
	IOReq *ior;
	IOReq *nior;
	Err ret;


	td->timerdev_VBlankCount++;
	if (td->timerdev_VBlankCount == 0)td->timerdev_VBlankCountOverFlow++;
	for (ior = (IOReq *)FIRSTNODE(&DelayList); ISNODE(&DelayList,ior) ;
			ior = nior)
	{
		int32 cnt;
		cnt = ior->io_Actual;
		cnt++;
		ior->io_Actual = cnt;
		nior = (IOReq *)NEXTNODE(ior);
		if ((uint32)cnt >= (uint32)ior->io_Info.ioi_Offset)
		{
		        if (ior->io_Flags & TIMERIOFLAGS_METRONOME)
		        {
                            /* resume the countdown... */
		            ior->io_Actual = 0;

		            /* ping our client */
		            ret = SuperInternalSignal((Task *)LookupItem(ior->io.n_Owner),
                                                      ior->io_Info.ioi_CmdOptions);
		            if (ret < 0)
		            {
		                /* couldn't ping the client, we're done for... */
		                ior->io_Error = ret;
		                RemNode((Node *)ior);
		                SuperCompleteIO(ior);
		            }
		        }
		        else
		        {
		            RemNode((Node *)ior);
                            SuperCompleteIO(ior);
                        }
		}
	}
	return 0;
}

int32
cMtimerInt()
	/* called by interrupt routine */
{
	reprocess();	/* ignore returns, this should always ret 0 */
	return 0;
}

static Item
timerInit(drv)
Driver *drv;
{
	InitList(&DelayList,"timerdelaylist");
	InitList(&MDelayList,"microtimerdelaylist");
	DBUG(("DelayList=%lx MDelayList=%lx\n",&DelayList,&MDelayList));

	/* use real programmable timers */
	DelayClock = SuperCreateItem(MKNODEID(KERNELNODE,TIMERNODE), htTags);
	if (DelayClock < 0)	return DelayClock;
	timer = (Timer *)LookupItem(DelayClock);

	DBUG(("timer=%lx int=%d\n",timer,timer->tm_IntNum));
	timItem = SuperCreateFIRQ("microtimer",200,cMtimerInt,timer->tm_IntNum);
	if (timItem<0)	return timItem;

	/* create vblank watcher */
	/* firqItem = SuperCreateItem(MKNODEID(KERNELNODE,FIRQNODE),FirqTags); */
	firqItem = SuperCreateFIRQ("vsynctimer",200,ctimerInt,INT_V1);
	if (firqItem < 0)	return firqItem;

	return drv->drv.n_Item;
}

static Item
devInit(dev)
Device *dev;
{
	dev->dev_MaxUnitNum = 1;	/* two units */
	return dev->dev.n_Item;
}

static int
CmdRead(ior)
IOReq *ior;
{
	/* Command Read */
	/* Read current time in ticks */
	int unit = (int)ior->io_Info.ioi_Unit;
	struct timeval *dst =
		(struct timeval *)ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;

	DBUGM(("timerdevice: CmdRead u %ld\n",unit));
	/* make sure long word aligned */
	if ((int)dst & 0x3) goto abort;

	if (len < 8) goto abort; /* get highest resolution */

	if (unit == TIMER_UNIT_USEC)
	{
	    TimeStamp(dst);	/* get current 48 bit tick value */
	}
	else
	{	/* unit 0 = vblank */
	    /* Read vblank counter */
	    dst->tv_sec = td->timerdev_VBlankCountOverFlow;
	    dst->tv_usec = td->timerdev_VBlankCount;
	}
	DBUGM(("sec=%ld usec=%ld\n",dst->tv_sec,dst->tv_usec));

	ior->io_Actual = 8;

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

	KDBUG(("CmdStatus dst=%lx len=%d\n",dst,len));
	if (len < 8)	goto abort;
	memset(&mystat,0,sizeof(DeviceStatus));

	mystat.ds_DriverIdentity = DI_TIMER;
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

static int
CmdWrite(ior)
IOReq *ior;
{
	ior->io_Error = BADCOMMAND;
	return 1;
}

bool
CmpF(mm,nm)
Node *mm,*nm;
{
	IOReq *m,*n;
	m = (IOReq *)mm; n = (IOReq *)nm;
	return m->io_Info.ioi_Offset < n->io_Info.ioi_Offset;
}

#ifdef undef
bool
DCmpF(mm,nm)
Node *mm,*nm;
{
	IOReq *m,*n;
	m = (IOReq *)mm; n = (IOReq *)nm;
	if (DCmp((uint32 *)m->io_Extension,(uint32 *)n->io_Extension) < 0)
		return 1;
	else return 0;
}
#endif

static int
CmdDelay(ior)
IOReq *ior;
{
	int unit = (int)ior->io_Info.ioi_Unit;
	struct timeval *tv;
	int32 size;
	ulong oldints;
	int retval = 0;

	if (unit == TIMER_UNIT_VBLANK)
	{
	    DBUG((" CmdDelay: unit %ld for %ld\n",unit,(uint32)ior->io_Info.ioi_Offset));
	    oldints = Disable();
	    UniversalInsertNode(&DelayList,(Node *)ior,CmpF);
	    ior->io_Flags &= ~IO_QUICK;
	    Enable(oldints);
	    return 0;
	}

	/* microsecond timer */

	size = ior->io_Info.ioi_Send.iob_Len;
	if (size != 8) goto abort;

	tv = (struct timeval *)ior->io_Info.ioi_Send.iob_Buffer;
	if ((int)tv & 3) goto abort;
	DBUGM((" CmdDelay:%lx.%lx\n",tv->tv_sec,tv->tv_usec));

	if (tv->tv_usec > 1000000) goto abort;

	/* Compute target timeval */
	TimeStamp((TimeVal *)ior->io_Extension);	/* Get current time */
	tvAdd((TimeVal *)ior->io_Extension,tv);
	DBUGM(("CmdMDelay target:ex=%lx.%lx\n", ior->io_Extension[0], ior->io_Extension[1]));

#if 0
	/* This is uneccessary overhead for the vast majority of cases.
	 * This case will be handled just fine by reprocess() above.
	 * And beyond that, this case will only occur if the time between
	 * the call to TimeStamp() above, and this call to TimeStamp()
	 * is greater than the waiting time interval requested. Quite
	 * unlikely.
	 */
	{
	TimeVal ltv;

            TimeStamp(&ltv);        /* Get current time */
            if (tvCmp((struct timeval *)ior->io_Extension,&ltv) > 0)
            {
                /* time already passed! */
                DBUGM(("Time already passed %lx.%lx, return now\n", \
                                    ltv.tv_sec,ltv.tv_usec));
                return 1;
            }
	}
#endif

	/* Now have target */

	oldints = Disable();
	UniversalInsertNode(&MDelayList,(Node *)ior,tvCmpF);
	if (FIRSTNODE(&MDelayList) == (Node *)ior)
	{
		/* This call may complete now */
		retval = reprocess();
	}
	if(retval == 0)ior->io_Flags &= ~IO_QUICK;
	Enable(oldints);
	return retval;
abort:
	ior->io_Error = BADIOARG;
	return 1;
}

static int
CmdDelayUntil(ior)
IOReq *ior;
{
	int unit = (int)ior->io_Info.ioi_Unit;
	struct timeval *tv;
	struct timeval ltv;
	int32 size;
	ulong oldints;
	int retval = 0;

	if (unit == TIMER_UNIT_VBLANK)
	{
	    oldints = Disable();
		/* limit of 1 overflow period for VBlank timer */
	    ior->io_Info.ioi_Offset -= td->timerdev_VBlankCount;
	    UniversalInsertNode(&DelayList,(Node *)ior,CmpF);
	    ior->io_Flags &= ~IO_QUICK;
	    Enable(oldints);
	    return 0;
	}

	/* microsecond timer */
	size = ior->io_Info.ioi_Send.iob_Len;
	tv = (struct timeval *)ior->io_Info.ioi_Send.iob_Buffer;
	DBUG((" CmdDelayUntil:%lx.%lx\n",tv->tv_sec,tv->tv_usec));
	if (size != 8) goto abort;
	if ((int)tv & 3) goto abort;

	if (tv->tv_usec > 1000000) goto abort;

	ior->io_Extension[0] = tv->tv_sec;
	ior->io_Extension[1] = tv->tv_usec;
	TimeStamp(&ltv);	/* Get current time */
	if (tvCmp((struct timeval *)ior->io_Extension,&ltv) <= 0)
	{
	    /* time already passed! */
	    DBUGM(("Time already passed %lx.%lx, return now\n", \
				ltv.tv_sec,ltv.tv_usec));
	    return 1;
	}
	/* Now have target */

	oldints = Disable();
	UniversalInsertNode(&MDelayList,(Node *)ior,tvCmpF);
	if (FIRSTNODE(&MDelayList) == (Node *)ior)
	{
		/* This call may complete now */
		retval = reprocess();
	}
	if(retval == 0)ior->io_Flags &= ~IO_QUICK;
	Enable(oldints);
	return retval;
abort:
	ior->io_Error = BADIOARG;
	return 1;
}


/*****************************************************************************/


static int CmdMetronome(IOReq *ior)
{
    if (ior->io_Info.ioi_Unit == TIMER_UNIT_USEC)
    {
        /* do we have the right amount of data */
	if (ior->io_Info.ioi_Send.iob_Len != sizeof(TimeVal))
	    goto abort;

        /* is the data correctly long-aligned */
        if ((uint32)ior->io_Info.ioi_Send.iob_Buffer & 3)
            goto abort;

        /* copy the supplied interval into the IOReq for later reference */
        memcpy(&((TimerIOReq *)ior)->timer_Interval,ior->io_Info.ioi_Send.iob_Buffer,sizeof(TimeVal));

        /* point the IO to the interval within the IOReq, so that it can't
         * change while the IO is being serviced
         */
        ior->io_Info.ioi_Send.iob_Buffer = &((TimerIOReq *)ior)->timer_Interval;
    }

    /* do we have a valid signal mask? */
    if ((int32)ior->io_Info.ioi_CmdOptions <= 0)
        goto abort;

    /* mark this puppy as a repeating metronome request */
    ior->io_Flags |= TIMERIOFLAGS_METRONOME;

    return CmdDelay(ior);

abort:
    ior->io_Error = BADIOARG;
    return 1;
}


/*****************************************************************************/


static int (*CmdTable[])() =
{
	CmdWrite,
	CmdRead,
	CmdStatus,
	CmdDelay,
	CmdDelayUntil,
	CmdMetronome
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	(void *)1,
	TAG_ITEM_NAME,	"timer",
	CREATEDRIVER_TAG_ABORTIO,	(void *)((long)TimerAbortIO),
	CREATEDRIVER_TAG_MAXCMDS,	(void *)6,
	CREATEDRIVER_TAG_CMDTABLE,	(void *)CmdTable,
	CREATEDRIVER_TAG_INIT,		(void *)((long)timerInit),
	TAG_END,		0,
};

static TagArg devArgs[] =
{
	TAG_ITEM_PRI,   (void *)3,
	CREATEDEVICE_TAG_DRVR,  (void *)1,
	TAG_ITEM_NAME,  "timer",
	CREATEDEVICE_TAG_INIT,  (void *)((long)devInit),
	CREATEDEVICE_TAG_IOREQSZ, (void *)sizeof(TimerIOReq),
	TAG_END,                0,
};

Item
createTimerDriver(void)
{
	Item devItem = -1;
	Item drvrItem;

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
	KDBUG(("Creating Timer driver returns drvrItem=%d\n",drvrItem));
	if (drvrItem >= 0)
	{
		devArgs[1].ta_Arg = (void *)drvrItem;
		devItem = (Item)CreateSizedItem(MKNODEID(KERNELNODE,DEVICENODE),
						devArgs,sizeof(TimerDevice));
		/*kprintf("Creating Timer device returns devItem=%d\n",devItem);*/
	}
	td = (TimerDevice *)LookupItem(devItem);
	return devItem;
}
