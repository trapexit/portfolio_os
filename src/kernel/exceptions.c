/* $Id: exceptions.c,v 1.60 1994/09/28 01:42:05 sdas Exp $ */

#include "types.h"
#include "kernel.h"
#include "internalf.h"
#include "mem.h"
#include "task.h"
#include "interrupts.h"
#include "clio.h"
#include "inthard.h"
#include "varargs.h"
#include "debug.h"

/* Bone Yard */
/* Come here to pick up the remains */

extern void printf(char *fmt, ...);
extern int vsprintf(char *buff, const char *fmt, va_list a);

#ifdef DEVELOPMENT
#define INFO(x)	printf x
#define DBUG(x)	/*printf x*/
#else
#define INFO(x)	/*printf x*/
#define DBUG(x)	/*printf x*/
#define DumpRegs(x) /* */
#define DumpTask(x) /* */
#endif

#ifdef ARM600
extern void dumpmmu(void);
extern int32 Arm600;
#endif

/* optional error number */
volatile uint32 PanicErr;
volatile uint32 PanicAddr;
/* Where the last panic message is placed */
volatile long boneyardmsg[128/sizeof(long)];


#define	WATCHDOG_NEEDLES	9	/* Count time in units of "Needles" */
#define	TEJU_MAGICOUNT		10

void SuperHardReboot(void)
{
	Clio *clio = (Clio *)CLIO;
	vuint32 *pointer = ADBIO;
	int i;

	INFO(("Enter HardReboot\n"));

	Disable();	/* no return out of this routine */
			/* except by an act of god */

	/* Mute the audio kludge ADBIO 1 */
	*pointer &= (vuint32)(~(ADBIO_AUDIO_MUTE | ADBIO_AUDIO_MUTE_EN));

	clio->WatchDog	   = 0x0b;	/* Trigger the addiction */
	clio->clio_ExpansionBus.xb_SetExpCtl = XB_EXPRESET;
					/* Hammer the expansion bus also */

	/* Wait 9 NEEDLES.  This is at least 100ms, no matter what. */
	for( i=0; i<WATCHDOG_NEEDLES; i++ ) {
		while( !(*VCNT&VCNT_MASK) > TEJU_MAGICOUNT )
			;
		while( !(*VCNT&VCNT_MASK) < TEJU_MAGICOUNT )
			;
	}

	/* Hit the Doggie-accelerator */
	*pointer |= (ADBIO_WATCHDOG | ADBIO_WATCHDOG_EN);

	while (1); /* Wait for Armageddon */
}

void
SuperHalt(void)
{
	/* This forces memory transfers so we can look at the
	   state analyzer and get some info when a machine with
	   no other way of getting info out has paniced */

	uint32 qwe;
	int i;
	while (1) {
		qwe = PanicErr;
		qwe = PanicAddr;
		for (i = 0; i < sizeof(boneyardmsg)/sizeof(long) ; i++)
		   qwe = boneyardmsg[i];
	}
}

void
SetPanicError(uint32 val,uint32 addr)
{
	PanicErr = val;
	PanicAddr = addr;
}

void
Panic(int halt, const char *fmt, ...)
{
    va_list a;
    int charcount;
    *DSPPGW = 0;		/* stop dspp */
    if (fmt)
    {
	va_start(a, fmt);
	charcount = vsprintf((char *)boneyardmsg, fmt, a);
	printf("%s",boneyardmsg);
    }
#ifdef DEVELOPMENT
    if (halt) SuperHalt();
#else
    if (halt) SuperHardReboot();
#endif
}

extern void EnableIrq(void);

#include "setjmp.h"

#ifdef DEVELOPMENT
void
DumpRegs(sp)
uint32 *sp;
{
	int32 i;
	INFO(("r0-r4 : "));
	for (i = 0; i < 5; i++)
	    INFO(("$%08x ",*sp++));
	INFO(("\nr5-r8 : "));
	for (i = 0; i < 4; i++)
	    INFO(("$%08x ",*sp++));
	INFO(("\nr9-r12: "));
	for (i = 0; i < 4; i++)
	    INFO(("$%08x ",*sp++));
	INFO(("\n"));
}

void
DumpTask(t)
Task *t;
{
    if (t)
    {
        if (t->t_ThreadTask)
        {
            INFO(("Thread: $%06lx, item $%06x, parent '%s', %s\n",t,t->t.n_Item,t->t_ThreadTask->t.n_Name,t->t.n_Name));
        }
        else
        {
            INFO(("Task  : $%06lx, item $%06x, %s\n",t,t->t.n_Item,t->t.n_Name));
        }
        INFO(("        t_FreeMemoryLists $%06x, t_StackBase $%06x, t_StackSize %d\n",t->t_FreeMemoryLists,t->t_StackBase,t->t_StackSize));
        INFO(("        t_SuperStackBase $%06x, t_SuperStackSize %d\n",t->t_SuperStackBase,t->t_SuperStackSize));
    }
    else INFO(("No task!\n"));
}
#endif

#ifdef undef
static volatile int32 qwe;
#endif

extern uint32 CPUFlags;
extern char kill_kprintf;

uint32
dumpbits ()
{
	vuint32		       *p;
	uint32                  abtbits;


	p = AbortBits;
	abtbits = *p;
	*p = 0;			/* rearm aborts for blue */

	if (abtbits & KernelBase->kb_QuietAborts)
		return abtbits;

	SetPanicError (abtbits, (uint32) p);

	if ((CPUFlags & (KB_NODBGR | KB_NODBGRPOOF)) == KB_NODBGR)
	{
		if (kill_kprintf)
			Panic (1, 0);
	}

#ifdef DEVELOPMENT

	/*
	 * If we are just going to catch this and return, do not print anything.
	 */
	if (KernelBase->kb_CatchDataAborts)
		return abtbits;

	INFO (("AbortBits=$%lx, ", abtbits));
	if (abtbits & ABT_ROMF)
		INFO (("Rom Failure\n"));
	if (abtbits & ABT_ROMW)
		INFO (("Rom Write\n"));
	if (abtbits & ABT_CLIOT)
		INFO (("CLIO timeout\n"));
	if (abtbits & ABT_HARDU)
		INFO (("User access to hardware\n"));
	if (abtbits & ABT_SYSRAMU)
		INFO (("User access to SYSRAM\n"));

	if (abtbits & ABT_FENCEV)
	{
		INFO(("Fence Violation\n"));
		INFO(("Fence Bits: "));
		p = FENCESTACK;
		INFO (("0L=%lx ", p[12]));
		INFO (("0R=%lx ", p[13]));
		INFO (("1L=%lx ", p[14]));
		INFO (("1R=%lx ", p[15]));
		INFO (("2L=%lx ", p[28]));
		INFO (("2R=%lx ", p[29]));
		INFO (("3L=%lx ", p[30]));
		INFO (("3R=%lx \n", p[31]));
	}
	if (abtbits & ABT_VPR)
		INFO (("Virtual Page Error\n"));
	if (abtbits & ABT_R26E)
		INFO (("Out of 26 bit address range\n"));
	if (abtbits & ABT_SPSC)
		INFO (("SPORT while SC\n"));
	if (abtbits & ABT_BITE)
		INFO (("Byte access to hardware\n"));
	if (abtbits & ABT_BADDEC)
		INFO (("*A detect\n"));
	if (abtbits & ABT_ARPS)
		INFO (("ARM to Regis/PIP while SIP\n"));
	if (abtbits & ABT_BWACC)
		INFO (("Byte address, Word access\n"));
	if (abtbits == 0)
	{
		INFO (("This abort is most likely from a 'lowmem abort' card.\n"));
		INFO (("The actual offending instruction may be up to two\n"));
		INFO (("instructions before the indicated PC.\n"));
	}
#endif
	return abtbits;
}

int
debugabort(frame,what)
uint32 *frame;
int what;
{
#ifdef HALT
	while (1);
#endif
    if ((CPUFlags & (KB_NODBGR|KB_NODBGRPOOF)) == KB_NODBGR)  return 0;
    return DebugAbortTrigger((uint32)frame,(uint32)what,0,0);
}

int32
cprefetchabort(sp)
uint32 *sp;
{
	/* incoming frame: */
	/* int32: r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r14 */
	/*        0  1  2  3  4  5  6  7  8  9  10  11  12  13 */
	EnableIrq();
	Panic(0,"Prefetch abort PC: $%lx\n",sp[13]-4);
	DumpRegs(sp);
	DumpTask(CURRENTTASK);
#ifdef ARM600
	if (Arm600)	dumpmmu();
#endif
	dumpbits();
	if (getSPSR() & 0x3)
	{
	    Panic(1,"Opus system error, prefetch abort in supervisor mode\n");
	}

	if (debugabort(sp,PREFETCH_ABORT)) return 1;	/* return to process */
	return AbortCurrentTask(sp);
}

extern int32 getSPSR(void);

int32
cdataabort(sp)
uint32 *sp;
{
	/* incoming frame: */
	/* int32: r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r14 */
	/*        0  1  2  3  4  5  6  7  8  9  10  11  12  13 */
	uint32 ret = 1;

	if ( (getSPSR() & 0x3) && (KernelBase->kb_CatchDataAborts == 0))
	{
	    if (CPUFlags & KB_NODBGR)
	    {
	        SetPanicError((uint32)0x12345678,0);
#ifdef DEVELOPMENT
		INFO(("Opus system error, data abort in supervisor mode\n"));
	        while (1);
#else
		Panic(1,"Opus system error, data abort in supervisor mode\n");
#endif
	    }
	}
#ifdef undef
	{
		volatile int32 *p = (int32 *)0x3300200;
		int i;
		p += 12;
		for (i = 0; i < 32-12 ; i++) qwe += *p++;
	}
#endif
	/* its just a user mode data abort */
	ret = dumpbits();
	if ( KernelBase->kb_CatchDataAborts == 0)
	{
	    INFO(("Data abort PC: $%lx\n",sp[13]-8));
	    DumpRegs(sp);
	    DumpTask(CURRENTTASK);
#ifdef ARM600
	    if (Arm600)	dumpmmu();
#endif
	}
	if (getSPSR() & 0x3)
	{
	    /* we were in supervisor mode! */
	    if (KernelBase->kb_CatchDataAborts == 0)
	    {
		Panic(1,"Panic: serious unexpected data supervisor abort\n");
	    }
	    if ( (ret & KernelBase->kb_QuietAborts) == 0)
	        DBUG(("Supervisor data abort, longjumping out\n"));
	    sp[0] = (int)KernelBase->kb_CatchDataAborts;
	    KernelBase->kb_CatchDataAborts = 0;	/* oneshot */
	    sp[1] = ret;
	    sp[13] = ((int32)(&longjmp));
	    return 1;
	}
	if (debugabort(sp,DATA_ABORT)) return 1;	/* return to process */
	return AbortCurrentTask(sp);
}

#ifdef	undef
extern int32 illins_check;
#endif

int32
cillins(sp)
uint32 *sp;
{
	/* incoming frame: */
	/* int32: r0,...,r12,r14 */
	/*        0  ...  12  13 */
#ifdef undef
	/* weed out startup check for mmu */
	if (sp[13] == (int32)(&illins_check))	return 1;
#endif

	/* unexpected Illegal instruction */
	EnableIrq();
	SetPanicError((uint32)(sp[6]-4),(uint32)cillins);
	Panic(0,"Illegal Instruction task=%lx addr=%lx\n",(uint32)CURRENTTASK,sp[13]-4);

#ifdef ARM600
	if (Arm600)	dumpmmu();
#endif
	if (debugabort(sp,ILLINS_ABORT)) return 1;	/* return to process */
	return AbortCurrentTask(sp);
}

void
cirq(sp)
int32 *sp;
{
	if (CPUFlags & KB_NODBGR)
	{
#ifdef undef
	    setSPSR(getSPSR() | 0x80);	/* disable and return */
#endif
	    return;
	}
	Panic(1,"Unexpected IRQ\n");
}

#if 0
void
cswi(sp)
int32 *sp;
{
	uint32 *pc = (uint32 *)sp[6];
	pc--;
	SetPanicError(*pc,(uint32)pc);
	Panic(1,"Unexpected SWI, no debugger\n");
}
#endif

int badbitcnt;

int
BadBitHandler()
{
	int32 b = *((vint32 *)(BADBITS));
#ifdef DEVELOPMENT
	int32 pb = *((vint32 *)(PrivBits));
#endif
	int32 q;
	*BADBITS = 0;
#ifdef DEVELOPMENT
	EnableIrq();
#endif
	INFO(("BadBit Interrupt bits=%lx\n",b));
	q = b & 0x7fff;
	b >>= 16;
	if (q)	INFO(("UnderFlow=%lx\n",q));
	q = b & 0xf;
	b >>= 4;
	if (q)	INFO(("OverFlow=%lx\n",q));

	q = b & 0x1;
	b >>= 1;
	if (q)
	{
#ifdef DEVELOPMENT
		INFO(("DMA Priv Violation PrivBits=%lx\n",pb));
		if (pb & PRIV_DMAtoSYSRAM) INFO(("DMA access to SYSRAM\n"));
		if (pb & PRIV_SPORTtoSYSRAM) INFO(("SPORT access to SYSRAM\n"));
		if (pb & PRIV_REGIStoSYSRAM) INFO(("REGIS access to SYSRAM\n"));
		if (pb & PRIV_DMA_VRAMSIZE) INFO(("DMA access over VRAMSIZE\n"));
		if (pb & PRIV_SPORT_VRAMSIZE)
		{
		    INFO(("SPORT access over VRAMSIZE:%lx\n",*CLUTMIDctl));
		}
		if (pb & PRIV_REGIS_VRAMSIZE) INFO(("REGIS access over VRAMSIZE\n"));
		if (pb & PRIV_REGIS_MATH) INFO(("REGIS math failure\n"));
#endif
		*((int32 *)(PrivBits)) = 0;
	}

#ifdef DEVELOPMENT
	q = b & 0x1;
	b >>= 1;
	if (q)
	{
		INFO(("DMA NFW=%lx ",q));
		INFO(("addr=%lx\n",b&0x3f));
	}
#endif
	badbitcnt++;
	if (badbitcnt > 3) {
#ifdef DEVELOPMENT
	    INFO(("Bad bits abort\n"));
	    while (1);
#else
	    Panic(1,"Bad bits abort\n");
#endif
	}
	return 1;
}

void swiabort(ti,sp)
Item ti;
uint32 *sp;
{
	INFO(("swi aborted taskitem=%lx\n",ti));
}

extern long GetUserSP(void);

void cswi_overrun(r0,r1,r2,r3,psr,r8,r9,r10,r11,r14)
int32 r0,r1,r2,r3;
int32 psr,r8,r9,r10,r11;
uint32 *r14;
{
	r14--;
	Panic(1,"SWI OVERRUN PC=$%lx  instruction=$%lx usp=$%lx\n",(ulong)r14,*r14,GetUserSP);
}

void
SoftIntHandler()
{
	/* ignore for now */
	return;
}

#ifdef undef
void
AbortTest(void)
{
	jmp_buf	my_env;
	int32 *p = (int32*)0x2500000;
	KernelBase->kb_CatchDataAborts = &my_env;
	if (setjmp(my_env))	return;
	*p = 1;
	KernelBase->kb_CatchDataAborts = 0;
}
#endif
