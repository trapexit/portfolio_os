/*	$Id: meidev.c,v 1.23 1994/08/02 20:13:47 markn Exp $
**
**	Dipir device driver for the MEI CDROM drive.
**
**	Copyright 1993 by The 3DO Company Inc.
*/
#ifdef MEICD_DRIVER /* Skip whole source file if MEICD_DRIVER not defined. */

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "rom.h"
#include "dipir.h"
#include "meidev_rev.h"


#define POLL                    xb->xb_Poll[0]
#define SELECTXBUS(u)           xb->xb_Sel[0] = u;
#define READPOLL(poll,unit)     xb->xb_Sel[0] = unit; poll = (uint8)POLL

#define CD_READY	0x01	/* drive is ready */
#define CD_DBLSPEED	0x02
#define CD_ERROR	0x10	/* error */
#define CD_SPINNING	0x20
#define CD_DISCIN	0x40	/* disc in the drive */
#define CD_DOORCLOSED	0x80	/* door is closed */

#define TESTREAD(s)     (s & XBUS_POLL_READ_VALID)
#define TESTWRITE(s)    (s & XBUS_POLL_WRIT_VALID)
#define TESTSTAT(s)     (s & XBUS_POLL_STAT_VALID)

#define	DriveIsReady(s)	(s & CD_READY)
#define DoorIsClosed(s)	(s & CD_DOORCLOSED)
#define	DoorIsOpen(s)	(DoorIsClosed(s) == 0)
#define DiscIsIn(s)	(s & CD_DISCIN)
#define DiscIsSpinning(s)	(s & CD_SPINNING)
#define DriveIsDoubleSpeed(s)	(s & CD_DBLSPEED)

#define MEI_BLOCKSIZE	2048

static DipirEnv *de;
static DipirRoutines *dipr;


static uint32 OffsetToMSF(int32 offset)
{
  uint32 minutes, seconds, fields;
  minutes = offset / (FRAMEperSEC * SECperMIN);
  offset -= minutes * (FRAMEperSEC * SECperMIN);
  seconds = offset / FRAMEperSEC;
  fields = offset - seconds * FRAMEperSEC;
  return (minutes << 16) + (seconds << 8) + fields;
}

static 
void
WaitMills(num)
int num;
{
	RESETTIMER();
	while (READTIMER() < num) ;
}

#ifdef	UNDEF
static void
ResetDevice(void)
{
	ExpansionBus *xb = XBUS_ADDR;
	xb->xb_CmdStat[0] = 0x0a;
	xb->xb_CmdStat[0] = 0;
	xb->xb_CmdStat[0] = 0;
	xb->xb_CmdStat[0] = 0;
	xb->xb_CmdStat[0] = 0;
	xb->xb_CmdStat[0] = 0;
	xb->xb_CmdStat[0] = 0;
	WaitMills(600);
}

static void
AbortCommand()
{
	Do7Cmd((int)0x08000000,0);
}

static int
DriveReady(void)
{
	char errstat;
	ReadDevError();
	errstat = de->statbuff[de->statcnt-1];
	return errstat & CD_READY;
}

static void
SpinDown(void)
{
	Do7Cmd((int)0x03000000,0);
}

static void
DumpStat(void)
{
	int i;
	PUTS("stat[");
	for (i = 0; i < de->statcnt; i++)
	{
	    PUTHEX(de->statbuff[i]);
	    PUTC(' ');
	}
	PUTC(']');
}

static void
CloseDrawer(void)
{
	Do7Cmd((int)0x07000000,0);
}
#endif

static
void
AsyncDo7Cmd(a,b)
int a,b;
{
	int cmd[2];
	cmd[0] = a; cmd[1] = b;
	ASYNCSQUIRTOUTCMD((char *)cmd,7);
}

static void
Do7Cmd(a,b)
int a,b;
{
	int cmd[2];
	cmd[0] = a; cmd[1] = b;
	SQUIRTOUTCMD((char *)cmd,7);
}

static void
ReadDevError()
{
	Do7Cmd((int)0x82000000,0);
}

static int
DoubleSpeed(void)
{
	int errstat;
	Do7Cmd((int)0x09038000,0);
	errstat = de->statbuff[de->statcnt-1];
	return DriveIsDoubleSpeed(errstat);
}


static void
SpinUp(void)
{
	Do7Cmd((int)0x02000000,0);
}

/*
 *	Ok.  Them tricky dudes could *hold* the drive door closed,
 *	and prevent us from reentering Dipir.  Or, wire a disable
 *	to the door close switch.  We nail 'em.  On Eject, wait
 *	for the door to slide open.  MooooHahahahahahahahah!!!!
 *	Clever idea, Dale!
 */
static void
EjectDisc(void)
{
	char errstat;

	PUTS("Belch!");
	Do7Cmd((int)0x06000000,0);
	PUTS("Burp!");

	while( 1 )
	{
		ReadDevError();
		errstat = de->statbuff[de->statcnt-1];
		if( DoorIsOpen(errstat) )
			break;
		PUTC('o');
		WaitMills(50);		/* Avoid swamping drive */
	}
}

static void
AsyncReadBlock(b)
int b;
{
    de->DataExpected = de->BlockSize;
    AsyncDo7Cmd((int)0x10000000|(int)OffsetToMSF(b),(int)0x00000100);
}

static int
WaitReadBlock(void)
{
	return WAITXBUS();
}

static int
ReadBlock(b)
int b;
{
	PUTHEX(b);
	AsyncReadBlock(b);
	return WAITXBUS();
}

static int
ReadTOC(a)
int a;
{
	uint8 errstat;
	Do7Cmd((int)0x8c000000|(a<<8),0);
	errstat = de->statbuff[de->statcnt-1];
	if (errstat & 0x10) return -1;
	return 0;
}

static int
ReadSessionInfo(void)
{
	char errstat;
	Do7Cmd((int)0x8d000000,0);
	errstat = de->statbuff[de->statcnt-1];
	if (errstat & 0x10) return -1;
	return 0;
}

static int
ReadDiscInfo()
{
	uint8 errstat;
	Do7Cmd((int)0x8b000000,0);
	errstat = de->statbuff[de->statcnt-1];
	if (errstat & 0x10) return -1;
	return 0;
}


static int32 CheckGoldDisc(void) {
    return 0;
}

static int32 GetMfgPlant(void) {
    return 0;
}

static uint32
GetDriverInfo()
{
	return( DIPIR_DEVICE_ID_MEICDROM | MEIDEV_VERREV );
}

static DeviceRoutines dvr =
{
	ReadBlock,
	ReadDiscInfo,
	ReadSessionInfo,
	ReadTOC,
	EjectDisc,
	AsyncReadBlock,
	GetDriverInfo,
	CheckGoldDisc,
	GetMfgPlant,
	DEVICE_ROUTINES_VERSION_0+1,
	WaitReadBlock,
};


#define	MEI4000	0x306b	/* CDROM Drive revision 4000, July 1993 */
#define LOOP_DELAY 75	/* Delay to keep from swamping poor 8-bit drive micro */

int
MEICD_InitDisc(lde)
struct DipirEnv *lde;
{
    uchar errstat;
    uchar lier=0;	/* Moved Dale's retry counter to automatic storage */
    uchar NewMEI=0;	/* Set for revisions >4000 (July 1993) */
    uint32 retrycnt=0;	/* Only for old drives. !! remove soon !! */


    de = lde;
    dipr = de->DipirRoutines;

    PUTS(" Enter InitDisc");

    if ( de->ManuRevNum > MEI4000 )	/* 16 bits, per expansion spec */
	NewMEI = 1;

	goto	skipdelay;	/* Sorry 'bout the goto! */
/***************************************************************************/
again:
	WaitMills(LOOP_DELAY);	/* Avoid swamping drive */
	retrycnt++;		/* Only for old drives !! remove soon !! */

skipdelay:

	/* Get device status */
	ReadDevError();
	errstat = de->statbuff[de->statcnt-1];

	if (DoorIsOpen(errstat))
	{
		/*
		 * After much debate, we no longer wait here for the 
		 * door to close.
		 * That had meant 3DO boxes appeared "dead" (no video)
		 * if you ejected a disk during Dipir.
		 * We exit and "pray" for another Dipir.
		 */
	    	PUTS("***Door Open,return***");
		return 0;
	}

	/*
	 * Note: the drive hardware can't really tell the difference
	 * between "no disk" and "unfocusable disk".  95% of upside-down
	 * discs, for example, come back as "no disk".
	 */
	if (DiscIsIn(errstat) == 0)
	{   /* Disk in not in drive, we think */
	    /* Trying to do a SpinUp command at this time */
	    /* yielded what looked like a hung drive (and still does) */
	  if (!NewMEI)
	  {
	    PUTC(';');

	    if (retrycnt < (2000/LOOP_DELAY))	/* Should be 1 sec.  Wait 2 */
		goto again;

	    PUTS("***NoDisc***");
	    return 0;
	  }
	  else
	  {
	    PUTC(':');
	    if (DriveIsReady(errstat) == 0) goto again; /* still checking */
	    PUTS("***NewMEI NoDisc,return***");
	    return 0;	/* really no disk in drive! */
	  }
	}

	if (DriveIsReady(errstat) == 0)
	{ /* Disk is present in the drive! */

	  if (!NewMEI)
	  {
	    /* We give'em 6 seconds to try and read the TOC */
	    /* We give'em 16 seconds (photocd gold)to try and read the TOC */
	    /* Nope, now 31 seconds (photocd gold)to try and read the TOC */
#if 0
	    if (READTIMER() > 31000)
#endif

	    if (retrycnt > (31000/LOOP_DELAY))
	    {
		/* try mei reset command (it is bogus - hard reset 'em) */
		/*ResetDevice();*/	/* This also resets the timer */
		PUTS("***ResetDevAndExit***");
		RESETDEVANDEXIT();
	    }
	    PUTC('r');
	    goto again;
	  }

	    /* drive not ready yet disk present! */
	    /* must be a bad disk */
	    PUTS("+++Unreadable Disc+++"); PUTHEX(errstat);
	    PUTS("=stat ");

	/* Note: I don;t think this is needed anymore -Bryce */
	    lier++;
	    if (lier == 1) goto again;

	/*
	 * At this point drive says the disk is unreadable.  We wanted
	 * to kick it out, but had several reasons not to.  We can't
	 * tell the difference between "no disk" and "disk upside down".
	 * We don't want to open the door of a store demo unit, or bonk
	 * the glass doors of a user's cabinet.  Also, 95% of pizza discs
	 * come back as "no disk" - we'd seem inconsistent.  Also, the
	 * drive is buggy.
	 * 
	 * So, we just exit.  We want the drive to NEVER AGAIN accept
	 * read commands for this unit (!!!).  Action: Bug Hedley.
	 * This is a security hole!!!
	 */

#if 0
	PUTS("*** Ick! Waiting ***");
	WaitMills(1000);	/* MEI5000 drive buggy, must wait !! */
#endif
	PUTS("*** Ick! Ejecting Disk ***");
	EjectDisc();		/* MEI5000 drive may crash here !! */

	    return 0;
	}

	/* the door is closed, there is a disc in there */
	PUTS("Door closed, disk in place");
	if (DiscIsSpinning(errstat) == 0)
	{
		PUTS("Spin up, go back");
		SpinUp();
		goto again;
	}

	if (DoubleSpeed())
	{
		PUTS("DOUBLE SPEED!");
	}
	else
	{
		PUTS("SINGLE SPEED!");
	}

	/* initialize our device routines */
	de->dvr = &dvr;
	de->BlockSize = MEI_BLOCKSIZE;

	PUTS("Happy camper");
	return 1;	/* disc ready to be interrogated */
}

#else /* MEICD_DRIVER */

int MEICD_InitDisc(struct DipirEnv *lde) { return 0; }

#endif /* MEICD_DRIVER */
