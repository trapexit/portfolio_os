/* $Id: main.c,v 1.132 1994/11/23 01:54:48 sdas Exp $ */

/* If you add debugging to this file, please use the MASTERDEBUG facility. */

#ifndef	ROMBUILD
/*
 * We still need this define to represent the address that
 * the misc code gets loaded at in development systems where
 * the dipir does not hand us the address. UGH!
 */
#define	MISCSTART	0x10000
#endif

#ifdef DEVELOPMENT
#define INFO(x)		printf x
#else
#define INFO(x)		/* printf x */
#endif

#define FIND_FREQ

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "item.h"

#include "folio.h"
#include "list.h"

#include "kernel.h"
#include "mem.h"
#include "strings.h"
#include "aif.h"

#include "semaphore.h"
#include "inthard.h"
#include "discdata.h"
#include "setjmp.h"

#include "stdio.h"			/* for printf() */
#include "debug.h"			/* for print_vinfo() */

#include "internalf.h"
#include "operror.h"			/* for PrintError */

#include "sysinfo.h"
#include "super.h"
#include "bootdata.h"

extern void             InitKB (_3DOBinHeader *);
extern void             ErrorInit (void);
extern void             TellChipsVersion (int8 version);
extern void             VBDelay (int);

extern Item             KernelItem;

uint32                 *InitItemTable (void);
void                    start_timers (void);

extern void             InitIO (void);
extern void             InitMem (void);

#ifdef ROMBUILD
extern void             InitDirectoryBuffer(void);
#endif

#ifdef ARM600
extern void             InitMMU (int32);
#endif

extern void             Panic (int halt, char *fmt, ...);
extern void             start_firq (void);

extern BootData        *DipirBootData;

char                    VolumeName[32];

extern AIFHeader        __my_AIFHeader;
extern _3DOBinHeader    __my_3DOBinHeader;

uint32			DevicePermissions;
PlatformID		pfID = { 0, };

void
startOperator()
{
	char		*oper_name = "Operator";
	AIFHeader	*aifHdr;

	/*
	 * The operator is loaded above the kernel.
	 * see documentation in mem.c for details.
	 */
	aifHdr = FindImage(0, 0, oper_name);

	if (aifHdr == (AIFHeader *)0)
	    printf("sherry: unable to locate %s\n", oper_name);
	else
	{
	    Item   t = -1;
	    Task   *task;

	    if ((task= (Task *)AllocateNode((Folio *)KernelBase, TASKNODE)) != NULL)
	    {
		char   *oper_argp[3];
		int32  oper_argc = 2;

		oper_argp[0] = oper_name;
		oper_argp[1] = (char *)&__my_AIFHeader;
		oper_argp[2] = NULL;

		t = internalCreateTaskVA
			(task,
			 TAG_ITEM_NAME,		 oper_name,
			 CREATETASK_TAG_AIF,	 aifHdr,
			 CREATETASK_TAG_IMAGESZ, (aifHdr->aif_ImageROsize +
						  aifHdr->aif_ImageRWsize +
						  aifHdr->aif_ZeroInitSize),
			 CREATETASK_TAG_ARGC,	 oper_argc,
			 CREATETASK_TAG_ARGP,	 oper_argp,
			 TAG_END,		 0
			);
	    }
#ifdef DEVELOPMENT
	    if (t < 0)
		PrintError ("startOperator", "internalCreateTask", task, t);
#endif /* DEVELOPMENT */
	}
}

extern uint32           CPUFlags;
#ifdef	ARM600
extern uint32           Arm600;
#endif

int                     clock_freq;	/* multiply by 100 to get actual hz */

#ifdef MULTIT
static TagArg                  stags[2] =
{
	TAG_ITEM_NAME, (void *) "Swi",
	TAG_END, 0,
};

Item                    SwiSemaphoreI;
extern Semaphore       *SwiSemaphore;
#endif

void
ResetHardware (void)
{
	*AbortBits = 0;
}

extern void            *callaif (int, int, int, void *);

#ifdef	EXT_MISC
extern vpfunc_t		MiscCode;
extern struct MiscFuncs *MiscFuncs;	/* table of misc functions */
#endif

int
main ()
{
	AIFHeader              *aifp;
	DiscLabel              *DipirBootVolumeLabel;

#ifdef	EXT_MISC
	/*
	 * If MiscCode doesn't point sanely, point it at where we
	 * expect the MiscCode to really be. It would be nice if there
	 * were some way for us to scan memory looking for misc code.
	 */
	if ((MiscCode == 0) || (Make_Int (unsigned, MiscCode) >=16 * 1024 * 1024))
	{
		MiscCode = Make_Func (void *, MISCSTART);
	}

	/* Initialize misc code read in from disc */
	MiscFuncs = (struct MiscFuncs *) callaif (0, 0, 0, Make_Ptr (void, MiscCode));
#endif

#ifdef	PEBBLE_BEACH_GOLF_WORKAROUND
	/*
	 * Pebble Beach Golf (Japanise version) wedges after accessing
	 * memory at zero; placing a zero there seems to help.
	 */
	*(vuint32 *)0 = 0;
#endif

#ifdef ARM600
	if (cpuspeed () < 77)
		Arm600 = 1;
#endif

	KernelBase = 0;
	ResetHardware ();		/* can't trust state of hardware */
	clock_freq = 500000;

	aifp = &__my_AIFHeader;

	InitIO ();
	Enable (0);			/* Must reenable interrupts at top level */

	if (DipirBootData)
	{
		DipirBootVolumeLabel = DipirBootData->bd_BootVolumeLabel;
		DevicePermissions = DipirBootData->bd_DevicePerms;
	}
	else
	{
		DipirBootVolumeLabel = 0;
		DevicePermissions = -1;
	}

	if ((DipirBootVolumeLabel == 0) ||
	    ((long) DipirBootVolumeLabel & 3) ||
	    ((long) DipirBootVolumeLabel >= (long)aifp))
	{
		/* must be coming up from rom */
		memset (VolumeName, 0, 32);
		strcpy (VolumeName, "rom");
	}
	else
		memcpy (VolumeName, DipirBootVolumeLabel->dl_VolumeIdentifier, 32);

	InitKB (&__my_3DOBinHeader);

	{
/*
 * derived from print_vinfo() ... which we must not use
 * because it dives through the Print3DOHeader SWI.
 */
	    extern char	whatstring[];
	    extern char	copyright[];
	    char	       *wsp = whatstring;
	    if ((wsp[0] == '@') &&
		(wsp[1] == '(') &&
		(wsp[2] == '#') &&
		(wsp[3] == ')'))
		wsp += 4;
	    while (*wsp == ' ')
		wsp ++;
	    internalPrint3DOHeader (&__my_3DOBinHeader, wsp, copyright);
	}

/* NOTE: Don't call SysInfo before InitKB. SysInfo hooks to Kernel */
/*       using KernelBase to handle aborts reading ROM     */
/*
 * Get the Platform ID, for use by debugging code.
 * NOTE: This is only for diagnostics, debug, and display purposes;
 * it should not be used to determine any feature of the IM.
 */
	if (pfID.chip == 0)
	        SuperQuerySysInfo(SYSINFO_TAG_PLATFORMID, (void *)&pfID, sizeof(PlatformID));
	{
	    char *mfg,  umfg[32];
	    char *chip, uchip[32];
	    char *rev,  urev[32];
	    char *xmsg;

	    xmsg = "";

	    switch (pfID.mfgr)
	    {
	        case SYSINFO_MFGR_MEC:		mfg = "MEC";	  break;
	        case SYSINFO_MFGR_SANYO:	mfg = "SANYO";	  break;
	        case SYSINFO_MFGR_ATT:		mfg = "ATT";	  break;
	        case SYSINFO_MFGR_GOLDSTAR:	mfg = "GOLDSTAR"; break;
	        case SYSINFO_MFGR_SAMSUNG:	mfg = "SAMSUNG";  break;
	        case SYSINFO_MFGR_CREATIVE:	mfg = "CREATIVE"; break;
	        case SYSINFO_MFGR_SA:		mfg = "SA";	  break;
		default: sprintf(mfg = umfg, "UNKNOWN-MFGR<0x%x>",  pfID.mfgr);
	    }

	    switch (pfID.chip)
	    {
	        case SYSINFO_CHIP_RED:		chip = "RED";	break;
	        case SYSINFO_CHIP_GREEN:	chip = "GREEN";	break;
	        case SYSINFO_CHIP_ANVIL:	chip = "ANVIL";	break;
		default: sprintf(chip = uchip, "UNKNOWN-CHIP<0x%x>", pfID.chip);
	    }

	    sprintf(rev = urev, "Upgrade %d.%d", pfID.ver, pfID.rev);

	    INFO(("Platform ID: %s %s %s %s\n", mfg, chip, rev, xmsg));
	}

	TellChipsVersion (0);		/* Tell the custom chips who we are */

	InitMem ();

#ifdef ARM600
	if (Arm600)
		InitMMU (aifp->aif_ImageROsize);
#endif

	if (InitItemTable () == 0)	/* Side affects in this routine */
	{
		Panic (1, "Could not allocate item table\n");
	}

	/* and kernel is item #1 */
	/* finish initializing KernelBase */
	KernelItem = AssignItem (KernelBase, 1);
	KernelBase->kb.fn.n_Item = KernelItem;
	KernelBase->kb_BootVolumeName = VolumeName;

	ErrorInit ();

#ifdef ROMBUILD
	InitDirectoryBuffer();
#endif

#ifdef MULTIT
	/* allocate swi semaphore */
	SwiSemaphore = (Semaphore *) AllocateNode ((Folio *) KernelBase, SEMA4NODE);
	if (!SwiSemaphore)
	{
		Panic (1, "Could not allocate SwiSemaphore\n");
	}
	SwiSemaphoreI = internalCreateSemaphore (SwiSemaphore, stags);
	if (SwiSemaphoreI < 0)
	{
		Panic (1, "Could not create SwiSemaphore\n");
	}
#endif

	if (init_dev_sem () < 0)
		Panic (1, "Could not create DevSemaphore\n");

	start_timers ();

	/* take over interrupts now */
	start_firq ();
	EnableFence (FENCEEN);

	startOperator();
	/* We must never get here */
	INFO (("Failure - Operator did not start\n"));
	while (1);
}


void
TellChipsVersion (int8 version)
{
	uint32                  tmp, shiftversion;
	volatile uint32        *pointer;
	jmp_buf                 jb, *old_co;
	uint32			old_quiet;

	old_co = KernelBase->kb_CatchDataAborts;
	old_quiet = KernelBase->kb_QuietAborts;

	if (setjmp (jb))
	{			/* Probably could not write to UNCLE */
		KernelBase->kb_CatchDataAborts = old_co;
		KernelBase->kb_QuietAborts = old_quiet;
		return;
	}
	KernelBase->kb_CatchDataAborts = &jb;
	KernelBase->kb_QuietAborts = ABT_CLIOT;

	shiftversion = (uint32) ((uint32)version << 24);

	/* first Madam */
	pointer = (uint32 *) (MADAM) + 1;
	tmp = (*pointer & 0x00FFFFFF) | shiftversion;
	*pointer = tmp;

	/* now CLIO */
	pointer = (uint32 *) (CLIO) + 1;
	tmp = (*pointer & 0x00FFFFFF) | shiftversion;
	*pointer = tmp;

	/* now UNCLE */
	pointer = (uint32 *) (UNCLE) + 1;
	tmp = (*pointer & 0x00FFFFFF) | shiftversion;
	*pointer = tmp;

	KernelBase->kb_CatchDataAborts = old_co;
	KernelBase->kb_QuietAborts = old_quiet;
}
