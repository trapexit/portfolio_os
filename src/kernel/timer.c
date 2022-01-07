/* $Id: timer.c,v 1.37 1994/09/27 20:03:23 vertex Exp $ */

/* file: timer.c */

#include "types.h"
#include "nodes.h"
#include "item.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "timer.h"
#include "interrupts.h"

#include "inthard.h"
#include "clio.h"
#include "time.h"

#include "operror.h"
#include "stdio.h"
#include "internalf.h"

extern void Panic(int halt,char *);

#define DBUG(x)	/*printf x*/

Timer *realtimeclock;
Timer *quantaclock;

/* manage allocation and deallocation of hardware timers */

void
ControlTimer(t,set,clr)
Timer *t;
int32 set,clr;
{
    uint32 *p;
    int32 ints = set&(TIMER_INTEN|TIMER_INTREQ);
    int32 intc = clr&(TIMER_INTEN|TIMER_INTREQ);
    int32 c = t->tm_ID;
    int32 i = t->tm_Size;

    DBUG(("TimerControl(t=%lx set=%lx clr=%lx\n",t,set,clr));
    DBUG(("size=%d num=%d\n",i,c));

    set &= 0xf;
    clr &= 0xf;

    while (i--)
    {
	int32 q;
	if (c > 7)	p = (uint32 *)(SetTm1);
	else	p = (uint32 *)(SetTm0);

	q = c&7;
	q <<= 2;

	/* never set CASCADE on last timer in chain */
	if (i == 0)	set &= ~TIMER_CASCADE;

	DBUG(("c=%d q=%d ",c,q));
	DBUG(("set=%lx clr=%lx\n",set,clr));

	if (clr)	p[1] = clr<<q;
	if (set)	p[0] = set<<q;
	c--;	/* next timer */
    }
    if (ints&TIMER_INTEN)	EnableInterrupt(t->tm_IntNum);
    if (intc&TIMER_INTEN)	DisableInterrupt(t->tm_IntNum);
    if (intc&TIMER_INTREQ)	ClearInterrupt(t->tm_IntNum);
	/*DBUG(("return from Control Timer\n"))*/
}

void
LoadTimer(c,v,r)
int32 c,v,r;
{
	HardTimer *ht;
	v &= 0xffff;
	ht = (HardTimer *)(Timer0);
	ht += c;
	DBUG(("LoadTimer ht=%lx v=%lx r=%d\n",ht,v,r));
	ht->ht_cnt = v;
	ht->ht_cntBack = r;
}

int32
ReadTimer(n)
int32 n;
{
	HardTimer *ht;
	ht = (HardTimer *)(Timer0);
	ht += n;
	return ht->ht_cnt;
}

void
CascadeEnable(t)
Timer *t;
{
	ControlTimer(t,(int32)TIMER_CASCADE,(int32)0);
}

void
TimerInit(t)
Timer *t;
{
	ControlTimer(t,(int32)0,(int32)TIMER_ALLBITS);
}

uint8 IntTbl[16] =
{	0,INT_TIM1,
	0,INT_TIM3,
	0,INT_TIM5,
	0,INT_TIM7,
	0,INT_TIM9,
	0,INT_TIM11,
	0,INT_TIM13,
	0,INT_TIM15
};

static int32
ict_c(tm, p, tag, arg)
Timer *tm;
uint32 *p;
uint32 tag;
uint32 arg;
{
    switch (tag)
    {
	case CREATETIMER_TAG_NUM: tm->tm_Size = (uint8)arg;
			if (tm->tm_Size > 15) return BADTAGVAL;
				  break;
	case CREATETIMER_TAG_FLAGS: *p = arg;	/* flags */
				  break;
	default:	return BADTAG;
    }
    return 0;
}

Item
internalCreateTimer(tm,a)
Timer *tm;
TagArg *a;
{
	/* num = number of contiguous timers */
	/* flags, interrupt = 1, if must generate an interrupt */
	uint32 n;
	uint32 flags = 0;
	int32 m;
	int32 tb;
	uint32 c;
	Item ret;

	DBUG(("internalCreateTimer: enter\n"));

        if (CURRENTTASK)
        {
            if ((CURRENTTASK->t.n_Flags & TASK_SUPER) == 0)
                return BADPRIV;
        }

	ret = TagProcessor(tm, a, ict_c, &flags);
	if (ret < 0)	return ret;

	n = tm->tm_Size;
	/* compute bit mask, 1 bit for each timer 1->1 2->3 3->7... */
	m  = (uint32)(1<<n) - 1;
	c = n-1;
again:
	tb = KernelBase->kb_TimerBits;
	DBUG(("timer loop c=%d m=%lx tb=%lx\n",c,m,tb));
	while (c<16)
	{
	    if ( (tb & m) == m)	break;
	    c++;
	    m <<= 1;
	}
	if (c == 16)
	{
		ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_NoTimer);
		goto err;
	}
	if (flags & TIMER_MUST_INTERRUPT)
	{
	    if ( (c & 1) == 0)
	    {
		c++;
		m <<= 1;
		goto again;
	    }
	}
	KernelBase->kb_TimerBits &= ~m;
	DBUG(("Got timers c=%d\n",c));
	tm->tm_ID = (uint8)c;
	tm->tm_IntNum = IntTbl[c];
	/* link em together */
	DBUG(("link em together\n"));
	TimerInit(tm);
	CascadeEnable(tm);
	DBUG(("after last TimerInit\n"));
	tm->tm_Control = ControlTimer;
	tm->tm_Load = LoadTimer;
	tm->tm_Read = ReadTimer;
	DBUG(("CreateTime now returning\n"));
	return tm->tm.n_Item;
err:
	return ret;
}

int32
internalDeleteTimer(tm,t)
Timer *tm;
Task *t;
{
	uint32 m;
	DisableInterrupt(tm->tm_IntNum);
	m = (uint32)(1<<tm->tm_Size)-1;
	m <<= tm->tm_ID;
	KernelBase->kb_TimerBits |= m;
	return 0;
}

TagArg CntrTags[] =
{
	CREATETIMER_TAG_NUM,	(void *)3,
	0,0
};

TagArg TaskCntrTags[] =
{
	CREATETIMER_TAG_NUM,	(void *)1,
	CREATETIMER_TAG_FLAGS,	(void *)TIMER_MUST_INTERRUPT,
	0,0
};

int32
USec2Ticks(usecs)
int32 usecs;
{	/* convert usecs to ticks (1 second or less) */
	int32 ticks;
	DBUG(("USec2Ticks(%ld)\n",usecs));
	DBUG(("Using Green conversion\n"));
	ticks = usecs >> KernelBase->kb_numticks;
	return ticks;
}

#ifdef undef
void
Ticks2TimeVal(ticks,tv)
uint32 *ticks;	/* ptr to 64 bit timer value */
struct timeval *tv;
{	/* convert ticks to usecs */
	uint32 usecs;
	uint32 secs;
	/*DShiftL(ticks, KernelBase->kb_numticks);*/
	usecs = ticks[1] << KernelBase->kb_numticks;
	secs = usecs/1000000;
	tv->tv_sec = secs;
	if (secs)
	{
	    usecs -= secs*1000000;
	}
	tv->tv_usec = usecs;
}
#endif

int32
SimpleTicksToUsecs(int32 ticks)
{
	/* ticks must be at most a 16 bit value */
	int32 usecs;

	usecs = ticks << KernelBase->kb_numticks;
	return usecs;
}


void
TimeStamp(tv)
struct timeval *tv;
{
    uint32 ticks[2];
    uint32 qwe;

	read3ctrs(ticks);
	tv->tv_sec = ticks[0];
	qwe = 62500 - (ticks[1]+1);
	qwe <<= KernelBase->kb_numticks;
	tv->tv_usec = qwe;
}

void
start_timers(void)
{
	Item ti;
	int nsecs;
	int ticks;

	DBUG(("starting timers\n"));
	/* realtimeclock is a 1 sec interrupting timer */
	realtimeclock = (Timer *)AllocateNode((Folio *)KernelBase,TIMERNODE);
	if (!realtimeclock) goto abort;
	/* set up slack timer */

	    /* set up slack timer for redww/green */

	    /* Compute nsecs*100 per tick */
	    nsecs = (int)((100*2*1000000)/KernelBase->kb_numticks);
	    DBUG(("nsecs*100=%ld\n",nsecs));
	    /* we want 16 usecs of these so */
	    ticks = (16000*100)/nsecs;
	    /* pick closer error */
	    if (16000*100 - ticks*nsecs > nsecs/2) ticks++;
	    DBUG(("totalslackticks=%ld dticks=%ld\n",ticks,ticks-64));
	    if (ticks < 64)
	    {
#ifdef DEVELOPMENT
		printf("Error in slack tick calculation, time > 16 usec\n");
		while (1);
#else
		Panic(1,"Bad Slack Calc\n");
#endif

	    }
	    *Slack = (uint32)ticks - 64;	/* 16 usec per clock tick */
	    /**Slack = 80;*/	/* grins */

	ti = internalCreateTimer(realtimeclock,CntrTags);
	if (ti < 0) goto abort;

	/* load the second ctrs (2 16 bit cascaded counters) */
	LoadTimer((int32)realtimeclock->tm_ID ,
				(int32)0xffff, (int32)0xffff);
	LoadTimer((int32)realtimeclock->tm_ID-1 ,
				(int32)0xffff, (int32)0xffff);
	/* 1sec = 62500 16usec ticks */
	LoadTimer((int32)realtimeclock->tm_ID-2 ,(int32)62500-1, (int32)62500-1);
	KernelBase->kb_numticks = 4;

	ControlTimer(realtimeclock,(int32)TIMER_DECREMENT|TIMER_RELOAD, (int32)0);
	DBUG(("1 second timer started\n"));
	quantaclock = (Timer *)AllocateNode((Folio *)KernelBase,TIMERNODE);
	if (!quantaclock) goto abort;
	ti = internalCreateTimer(quantaclock,TaskCntrTags);
	if (ti < 0) goto abort;
#ifdef undef
	ticks = USec2Ticks((int32)20000);	/* 20 msecs */
	DBUG(("ticks=$%lx\n",ticks));
	LoadTimer((int32)quantaclock->tm_ID,ticks,ticks); /* 20msec tick */
	ControlTimer(quantaclock,
			(int32)TIMER_DECREMENT|TIMER_RELOAD,(int32)0);
#endif
	return;
abort:
#ifdef DEVELOPMENT
	printf("Error starting timers\n");
	while (1);
#else
	Panic(1,"Error starting timers\n");
#endif

}
