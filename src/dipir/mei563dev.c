/*	$Id: mei563dev.c,v 1.5 1994/08/02 20:13:47 markn Exp $
**
**	Dipir device driver for the Soundblaster CDROM drive.
**
**	Copyright 1994 by The 3DO Company Inc.
*/
#ifdef MEICD563_DRIVER /* Skip whole source file if MEICD563_DRIVER not defined. */

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "rom.h"
#include "dipir.h"
#include "mei563dev_rev.h"


#define CD_READY	0x01	/* drive is ready */
#define CD_DBLSPEED	0x02
#define CD_ERROR	0x10	/* error */
#define CD_SPINNING	0x20
#define CD_DISCIN	0x40	/* disc in the drive */
#define CD_DOORCLOSED	0x80	/* door is closed */

#define	DriveIsReady(s)		(s & CD_READY)
#define DoorIsClosed(s)		(s & CD_DOORCLOSED)
#define	DoorIsOpen(s)		(DoorIsClosed(s) == 0)
#define DiscIsIn(s)		(s & CD_DISCIN)
#define DiscIsSpinning(s)	(s & CD_SPINNING)
#define DriveIsDoubleSpeed(s)	(s & CD_DBLSPEED)

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
AsyncDo7Cmd(int a,int b)
{
	int cmd[2];
	cmd[0] = a; cmd[1] = b;
	ASYNCSQUIRTOUTCMD((char *)cmd,7);
}

static void
Do7Cmd(int a, int b)
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
AsyncReadBlock(int b)
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
ReadBlock(int b)
{
	PUTHEX(b);
	AsyncReadBlock(b);
	return WAITXBUS();
}

static int
ReadTOC(int a)
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


static int32 CheckGoldDisc(void) 
{
    return 0;
}

static int32 GetMfgPlant(void) 
{
    return 0;
}

static uint32
GetDriverInfo()
{
	return( DIPIR_DEVICE_ID_MEICD563ROM | MEICD563DEV_VERREV );
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


#define LOOP_DELAY 75	/* Delay to keep from swamping poor 8-bit drive micro */

int
MEICD563_InitDisc(struct DipirEnv *lde)
{
	uchar errstat;
	uint32 retrycnt = 0;

	de = lde;
	dipr = de->DipirRoutines;

	PUTS(" Enter MEICD563_InitDisc");
	de->DeviceFlags |= DEV_NO_TAG_BYTE;
	SETXBUSSPEED(640);

again:
	if (retrycnt++ > 0)
		WaitMills(LOOP_DELAY);	/* Avoid swamping drive */

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
	if (!DiscIsIn(errstat))
	{
		/* Disk in not in drive, we think */
		/* Trying to do a SpinUp command at this time */
		/* yielded what looked like a hung drive (and still does) */
		PUTC(';');
		if (retrycnt < (2000/LOOP_DELAY)) /* Should be 1 sec.  Wait 2 */
			goto again;
		PUTS("***NoDisc***");
		return 0;
	}

	if (!DriveIsReady(errstat))
	{
		/* Disk is present in the drive! */
		/* We give'em 31 seconds to try and read the TOC */
		PUTC('r');
		if (retrycnt < (31000/LOOP_DELAY))
			goto again;
		EjectDisc();
		return 0;
	}

	/* the door is closed, there is a disc in there */
	PUTS("Door closed, disk in place");
	if (!DiscIsSpinning(errstat))
	{
		PUTS("Spin up, go back");
		SpinUp();
		goto again;
	}

	if (DoubleSpeed()) {
		PUTS("DOUBLE SPEED!");
	} else {
		PUTS("SINGLE SPEED!");
	}

	/* initialize our device routines */
	de->dvr = &dvr;
	de->BlockSize = DISC_BLOCK_SIZE;

	PUTS("Happy camper");
	return 1;	/* disc ready to be interrogated */
}

#else /* MEICD563_DRIVER */

int MEICD563_InitDisc(struct DipirEnv *lde) { return 0; }

#endif /* MEICD563_DRIVER */
