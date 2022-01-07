/*	$Id: lccddev.c,v 1.26.1.9 1995/02/15 19:11:12 bungee Exp $
**
**	Dipir device driver for the Low-Cost CD-ROM drive.
**
**	Copyright 1994 by The 3DO Company Inc.
*/
#ifdef LCCD_DRIVER /* Skip whole source file if LCCD_DRIVER not defined. */

#define	APPSPLASH 1

#include "types.h"
#include "inthard.h"
#include "clio.h"
#include "discdata.h"
#include "rom.h"
#include "dipir.h"
#include "lccddev.h"
#include "lccddev_rev.h"

#define	MAX_UINT32		(0xFFFFFFFF)
#define	NOTIMEOUT		(0)
#define	READ_RETRIES		12
#define	TOC_RETRIES		1

#undef	USE_SWITCH_REPORTS

#ifndef DEBUG
#define	DEBUGFLAG(n,action)
#else
#define	DEBUGFLAG(n,action)	if (dipirDebugControl & (n)) { action }
#define	dbDetail		0x01
#define	dbCmds			0x02
#define	dbMajor			0x04
#define	dbQReports		0x08
#define	dbIO			0x10
#define	dbIODetail		0x20
static uint32 dipirDebugControl = 0;
#endif

#define BCD2BIN(bcd)		((((bcd) >> 4) * 10) + ((bcd) & 0x0F))
#define BIN2BCD(bin)		((((bin) / 10) << 4) | ((bin) % 10))
#define	SwapNibbles(x)		((((x) & 0xF) << 4) | (((x) >> 4) & 0xF))

#define	MILLISEC		1
#define	OK			0
#define	TAG_ERROR		0x80

#define READPOLL(poll,unit) \
	xb->xb_Sel[0] = (unit);  poll = (uint8)(xb->xb_Poll[0]);

#define	DMA_CH0			0x1
#define	DMA_CH1			0x2

// These should be somewhere else ???
#define NEIL 0
#if NEIL
#undef EN_CD0toDMA
#undef EN_CD1toDMA
#undef INT1_DMAfrCD0
#undef INT1_DMAfrCD1
#define EN_CD0toDMA		0x20000000
#define EN_CD1toDMA		0x40000000
#define	INT1_DMAfrCD0		0x0020
#define	INT1_DMAfrCD1		0x0080
#define NeilFeatureReg		0x0340C000
#define	HWFeatureReg		*((vuint32*)NeilFeatureReg)

#define	DMAZeroCurPtr		((Madam *)MADAM)->UncleToRam.Address
#define	DMAZeroCurLen		((Madam *)MADAM)->UncleToRam.Length
#define	DMAZeroNextPtr		((Madam *)MADAM)->UncleToRam.NextAddress
#define	DMAZeroNextLen		((Madam *)MADAM)->UncleToRam.NextLength
#define	DMAOneCurPtr		((Madam *)MADAM)->ExternalToRam.Address
#define	DMAOneCurLen		((Madam *)MADAM)->ExternalToRam.Length
#define	DMAOneNextPtr		((Madam *)MADAM)->ExternalToRam.NextAddress
#define	DMAOneNextLen		((Madam *)MADAM)->ExternalToRam.NextLength
#define DMANeilEnable		0x0340C004
#define DMANeilDisable		0x0340C308
#else /* NEIL */
#define	HWFeatureReg		((Madam *)MADAM)->AnvilFeature
#define	DMAZeroCurPtr		((Madam *)MADAM)->CD0ToRam.Address
#define	DMAZeroCurLen		((Madam *)MADAM)->CD0ToRam.Length
#define	DMAZeroNextPtr		((Madam *)MADAM)->CD0ToRam.NextAddress
#define	DMAZeroNextLen		((Madam *)MADAM)->CD0ToRam.NextLength
#define	DMAOneCurPtr		((Madam *)MADAM)->CD1ToRam.Address
#define	DMAOneCurLen		((Madam *)MADAM)->CD1ToRam.Length
#define	DMAOneNextPtr		((Madam *)MADAM)->CD1ToRam.NextAddress
#define	DMAOneNextLen		((Madam *)MADAM)->CD1ToRam.NextLength
#endif /* NEIL */

/* Command packets */
static uchar PlayCmd[] =	{ CMD_PPSO, CMD_PPSO_PLAY };
static uchar PauseCmd[] =	{ CMD_PPSO, CMD_PPSO_PAUSE };
static uchar EjectCmd[] =	{ CMD_PPSO, CMD_PPSO_OPEN };
static uchar SingleSpeedCmd[] =	{ CMD_SET_SPEED, CMD_SET_SPEED_SINGLE, 0 };
static uchar DoubleSpeedCmd[] =	{ CMD_SET_SPEED, CMD_SET_SPEED_DOUBLE, 0 };
static uchar CheckWOCmd[] =	{ CMD_CHECK_WO };
static uchar CopyProtThreshCmd[] = { CMD_COPY_PROT_THRESH, 0, 0, 0 };
static uchar AudioFormatCmd[] =	{ CMD_SECTOR_FORMAT, FMT_AUDIO, 0 };
static uchar DataFormatCmd[] =	{ CMD_SECTOR_FORMAT, 
			  FMT_MODE1_AUXCH0 | FMT_HEADER_CH0 | FMT_COMP_CH0, 0 };

static DipirEnv *de;
static DipirRoutines *dipr;

#ifdef USE_SWITCH_REPORTS
static Boolean SwitchPressed;
#endif


/***************************************************************************
 Cache of TOC-type stuff.
 Upper levels of dipir read this stuff four or five times, so we cache it.
*/
static Boolean gotCachedInfo;
static TOCInfo cachedTOC1;
static DiscInfo cachedDiscInfo;
static Boolean cachedMultiSession;

static Boolean IsGoldDisc = FALSE; // TRUE?
static Boolean LongOpWorking = FALSE;
static Boolean ks_good = FALSE;
static uint32 k11, k22;
static int32 CorrectedECC;

/***************************************************************************
 Cache of buffers which hold pre-fetched data from the disc.
*/
struct DiscBuffer {
	struct DiscBuffer *	Next;
	uint32			BlockNumber;
	uint8			Intransit;
	uint8			unused[3];
	struct DiscBlock	DiscBlock;
};

static struct DiscBuffer *FirstBuffer;
static struct DiscBuffer *IntransitBuffer = NULL;
static uint32 ReqBlock;

/* There are exactly two disc buffers */
static struct DiscBuffer DiscBuffer1, DiscBuffer2;

/***************************************************************************
 One-time setup of the buffers.
*/
static void
InitBuffers(void)
{ 
	FirstBuffer = &DiscBuffer1;
	DiscBuffer1.Next = &DiscBuffer2;
	DiscBuffer2.Next = &DiscBuffer1;
	gotCachedInfo = FALSE;
}

/***************************************************************************
 Set all disc buffers to the "empty" state.
*/
static void
EmptyBuffers(void)
{
	struct DiscBuffer *bp;

	bp = FirstBuffer;
	do {
		bp->BlockNumber = 0;
		bp->Intransit = FALSE;
	} while ((bp = bp->Next) != FirstBuffer);
}

/***************************************************************************
 Find the buffer which holds a specific disc block.
*/
static struct DiscBuffer *
FindBuffer(uint32 block)
{
	struct DiscBuffer *bp;

	bp = FirstBuffer;  
	do {
		if (bp->BlockNumber == block)
			return bp;
	} while ((bp = bp->Next) != FirstBuffer);
	return NULL;
}

/***************************************************************************
 Find the latest (largest) block number currently in a buffer.
*/
static uint32
LatestBlock(void)
{
	struct DiscBuffer *bp;
	uint32 latest = 0;

	bp = FirstBuffer;  
	do {
		if (bp->BlockNumber > latest)
			latest = bp->BlockNumber;
	} while ((bp = bp->Next) != FirstBuffer);
	return latest;
}

/***************************************************************************
 Delay (spin) for the specified number of milliseconds.
*/
static void
Delay(int32 millisecs)
{
	RESETTIMER();
	while (READTIMER() < millisecs)
		continue;
}

/***************************************************************************
 Return the size of a specified report type.
*/
static int
ReportSize(uchar tagByte)
{
	switch (tagByte &~ TAG_ERROR)
	{
	case CMD_READ_ID	&~ TAG_ERROR:	return READ_ID_REPORT_SIZE;
	case CMD_READ_ERROR	&~ TAG_ERROR:	return READ_ERROR_REPORT_SIZE;
	case CMD_CHECK_WO	&~ TAG_ERROR:	return CHECK_WO_REPORT_SIZE;
	case CMD_READ_FIRMWARE	&~ TAG_ERROR:	return READ_FIRMWARE_RESP_SIZE;
	case CMD_COPY_PROT_THRESH &~ TAG_ERROR:	return COPY_PROT_THRESH_RESP_SIZE;
	case REPORT_DRIVE_STATE	&~ TAG_ERROR:	return DRIVE_STATE_REPORT_SIZE;
	case REPORT_QCODE	&~ TAG_ERROR:	return QCODE_REPORT_SIZE;
	case REPORT_SWITCH	&~ TAG_ERROR:	return SWITCH_REPORT_SIZE;
	}
	/* Anything else is just a standard one-byte command response. */
	return 1;
}

/***************************************************************************
 Read a response packet from the disc drive.
 We look for a packet of a specified type, discarding any we may get
 which are not of that type.
*/
static int
GetResponse(uchar *resp, int respLen, uchar tagByte, uint32 timeout)
{
	int expectLen = 0;
	int gotLen;
	int err = OK;
	uint32 tm;
	uchar stat;
	uchar poll;
	Boolean skip;
#ifdef USE_SWITCH_REPORTS
	Boolean switchReport;
#endif
	ExpansionBus *xb = XBUS_ADDR;

	do { /* Keep getting packets till we get the right type (tagByte). */
		skip = FALSE;
#ifdef USE_SWITCH_REPORTS
		switchReport = FALSE;
#endif
		gotLen = 0;
		do { /* Keep getting bytes till we get a full packet. */
			tm = timeout;
		  	for (;;) {
				/* Wait for a byte. */
				if (timeout != NOTIMEOUT && --tm == 0)
				{
					PUTS("GetResponse timeout\n");
					return -1;
				}
				READPOLL(poll, de->device);
				if (poll & XBUS_POLL_STAT_VALID) break;
				Delay(1*MILLISEC); /* don't swamp the bus */
			}
			stat = (uint8)(xb->xb_CmdStat[0]);
			if (gotLen == 0)
			{
				/* "stat" is the first byte, the tag byte.
				 * Figure out how long the packet will be. */
				expectLen = ReportSize(stat);
				DEBUGFLAG(dbDetail, PUTS("GetResponse: len="); 
						PUTHEX(expectLen);)
				if ((stat &~ TAG_ERROR) != (tagByte &~ TAG_ERROR))
				{
					/* Not the response we want.
					 * Discard it and keep looking. */
					skip = TRUE;
#ifdef USE_SWITCH_REPORTS
					if ((stat &~ TAG_ERROR) == REPORT_SWITCH)
						switchReport = TRUE;
					else
#endif
						PUTS("Discard unexpected resp\n"); 
				} else if (expectLen > respLen)
				{
					/* Should not happen: caller's
					 * buffer is not big enough. */
					PUTS("***ERROR: expectLen > respLen ");
					PUTHEX(respLen); 
					skip = TRUE;
					err = -1;
				}
				DEBUGFLAG(dbCmds, PUTS("R-tag="); )
			}
			DEBUGFLAG(dbCmds, PUTHEX(stat);) 
			if (!skip)
				resp[gotLen] = stat;
#ifdef USE_SWITCH_REPORTS
			else if (switchReport && gotLen == 2)
			{
				/* Third byte of a switch report tells us
				 * which switch has changed */
				if (stat & USER_SWITCH) 
				{
					SwitchPressed = TRUE;
					DEBUGFLAG(dbMajor,
						PUTS("=== SWITCH! ===\n"); )
				}
			}
#endif
			gotLen++;
		} while (gotLen < expectLen);
	} while (skip && err == OK);
	DEBUGFLAG(dbDetail, PUTS("GetResponse: ret="); PUTHEX(err);)
	return err;
}

/***************************************************************************
 Send a command to the drive and get a response packet.
*/
static int
SendDiscCmdWithResponse(uchar *cmd, int cmdLen, uchar *resp, int respLen, uint32 timeout)
{
	DEBUGFLAG(dbCmds, PUTS("SendDiscCmd: cmdLen="); PUTHEX(cmdLen);
		PUTS("cmd="); 
		{ int i; for (i=0; i<cmdLen; i++) { PUTHEX(cmd[i]); } } )
	ASYNCSQUIRTOUTCMD(cmd, cmdLen);
	DEBUGFLAG(dbDetail, PUTS("SendDiscCmd: get response"); )
	/* Response should have same type (tag byte) as the command we sent. */
	return GetResponse(resp, respLen, cmd[0], timeout);
}

/***************************************************************************
 Send a command to the drive and get a standard (one byte) response.
*/
static int
SendDiscCmd(uchar *cmd, int cmdLen)
{
	uchar resp[1];
	int err;

	err = SendDiscCmdWithResponse(cmd, cmdLen, resp, sizeof(resp), NOTIMEOUT);
	if (err < OK) return err;
	return OK;
}

/***************************************************************************
 Ask for a single report, right now.
*/
static int
QueryReport(uchar *report, int reportLen, uchar reportType)
{
	int err;
	uchar GetReportCmd[2];

	DEBUGFLAG(dbDetail, PUTS("QueryReport: type="); PUTHEX(reportType); )
	/* Request one immediate report. */
	GetReportCmd[0] = reportType &~ REPORT_TAG;
	GetReportCmd[1] = REPORT_NOW;
	err = SendDiscCmd(GetReportCmd, sizeof(GetReportCmd));
	if (err < OK) return err;
	/* Get the report. */
	return GetResponse(report, reportLen, reportType, NOTIMEOUT);
}

/***************************************************************************
 Get the current state of the drive (PLAY, PAUSE, STOP, OPEN, etc.)
*/
static int
GetDriveState(void)
{
	int err;
	uchar report[DRIVE_STATE_REPORT_SIZE];

	err = QueryReport(report, sizeof(report), REPORT_DRIVE_STATE);
	if (err < OK) return err;
	DEBUGFLAG(dbDetail, PUTS("GetDriveState: state="); PUTHEX(report[1]); )
	return report[1];
}

/***************************************************************************
 Enable/disable a specific type of report from the drive.
*/
static int
EnableReports(uchar reportType, int what)
{
	uchar EnableReportCmd[2];

	DEBUGFLAG(dbDetail, PUTS("EnableReports: type="); PUTHEX(reportType); 
			  PUTS("what="); PUTHEX(what); )

	/* Enable/disable this type of report. */
	EnableReportCmd[0] = reportType &~ REPORT_TAG;
	EnableReportCmd[1] = what;
	return SendDiscCmd(EnableReportCmd, sizeof(EnableReportCmd));
}

/***************************************************************************
 Wait for the drive to enter a specified state.
*/
static int
WaitDriveState(int needState)
{
	int state;

	/* Wait for the drive to enter the specified state. */
	for (;;)
	{
		state = GetDriveState();
		if (state < OK) return state;
		if (state == needState)
			break;
		if (DRV_ERROR_STATE(state))
			return -1;
#ifndef USE_SWITCH_REPORTS
		if (DRV_OPEN_STATE(state) && needState != DRV_OPEN)
			return -1;
#endif
		Delay(100*MILLISEC);
	}
	return OK;
}

/***************************************************************************
 Seek to a specified block number on the disc.
*/
static int
SeekDisc(int32 block)
{
	uchar SeekCmd[4];

	/* Issue the seek command to the drive. */
	SeekCmd[0] = CMD_SEEK;
	SeekCmd[1] = (uchar) (block >> 16);
	SeekCmd[2] = (uchar) (block >> 8);
	SeekCmd[3] = (uchar) (block);

	PUTS("SeekDisc:"); PUTHEX(*(uint32 *)SeekCmd);

	return SendDiscCmd(SeekCmd, sizeof(SeekCmd));
}

/***************************************************************************
 Eject the disc.
*/
static void
EjectDisc(void)
{
	int err;

	DEBUGFLAG(dbMajor, PUTS("EjectDisc:");)
	/* Send the OPEN-DRAWER command to the drive. */
	for (;;)
	{
		err = SendDiscCmd(EjectCmd, sizeof(EjectCmd));
		if (err >= OK) break;
		PUTS("EjectDisc: error in SendDiscCmd\n");
		Delay(100*MILLISEC);
	}
	/* Must wait for drive to open (or stuck, etc.) in order to make sure
	 * than any EjectCmd that was sent while the drive is in the process
	 * of returning to DoubleSpeed [see end of EndLongOp()] takes effect.
	 */
	WaitDriveState(DRV_OPEN);

}

#ifdef USE_SWITCH_REPORTS
#define	DRIVE_OPEN()	CheckSwitch()
/***************************************************************************
 Check to see if the user switch (disc eject button) has been pressed.
*/
static int
CheckSwitch(void)
{
	if (!SwitchPressed)
		return 0;
	EjectDisc();
	return -1;
}
#else
#define	DRIVE_OPEN()	DriveOpen()
/***************************************************************************
 Check to see if the drive is open (or opening).
*/
static Boolean
DriveOpen(void)
{
	int state;

	state = GetDriveState();
	if (DRV_ERROR_STATE(state))
	{
		/* If the drive is in some kind of error state,
		 * just say that it is OPEN. */
		PUTS("DriveOpen: error state "); PUTHEX(state);
		return TRUE;
	}
	return DRV_OPEN_STATE(state);
}
#endif /* USE_SWITCH_REPORTS */

/***************************************************************************
 Enable DMA from the disc.
*/
static void 
EnableDMA(uint8 channels)
{
	uint32	curBits;

        /* reset the CD DMA hardware */
        curBits = HWFeatureReg;
        curBits &= ~MADAM_CDDMA;
        HWFeatureReg = curBits;
	
	/* Enable the host DMA channel(s) first. */
	if (channels & DMA_CH0)
	{
		*(vuint32*)ClrInt1Bits = INT1_DMAfrCD0;
#if NEIL
		/* turn on Neil for now, too */
		*(vuint32 *)DMANeilEnable = EN_CD0toDMA;
#endif
		/* enable Anvil cd0 DMA channel */
		*(vuint32 *)DMAREQEN = EN_CD0toDMA;
	}
	if (channels & DMA_CH1)
	{
		*(vuint32*)ClrInt1Bits = INT1_DMAfrCD1;
#if NEIL
		/* turn on Neil for now, too */
		*(vuint32 *)DMANeilEnable = EN_CD1toDMA;
#endif
		/* enable Anvil cd1 DMA channel */
		*(vuint32 *)DMAREQEN = EN_CD1toDMA;
	}

	/* Enable the CD DMA hardware */
	curBits = HWFeatureReg;
	curBits |= MADAM_CDDMA;
	HWFeatureReg = curBits;
}

/***************************************************************************
 Disable DMA from the disc.
*/
static void 
DisableDMA(uint8 channels)
{
	uint32	curBits;

	/* Disable the CD DMA hardware first. */
	curBits = HWFeatureReg;
	curBits &= ~MADAM_CDDMA;
	HWFeatureReg = curBits;

	/* Then disable the host DMA channel(s) */
	if (channels & DMA_CH0)
	{
#if NEIL
		/* turn off Neil for now, too */
		*(vuint32 *)DMANeilDisable = EN_CD0toDMA;			
#endif
		/* disable Anvil cd0 DMA channel */
		*(vuint32 *)DMAREQDIS = EN_CD0toDMA;
	}
	if (channels & DMA_CH1)
	{
#if NEIL
		/* turn off Neil for now, too */
		*(vuint32 *)DMANeilDisable = EN_CD1toDMA;			
#endif
		/* disable Anvil cd1 DMA channel */
		*(vuint32 *)DMAREQDIS = EN_CD1toDMA;
	}
}

/***************************************************************************
 Set up DMA memory address & length (Current).
*/
static void 
ConfigDMA(uint8 channel, uint8 *dst, int32 len)
{
	if (channel == DMA_CH0)
	{
		DEBUGFLAG(dbIODetail,
			PUTS("ConfigDMA ch0: ptr/len="); 
			PUTHEX(dst); PUTHEX(len); )
		DMAZeroCurPtr = (uint32)dst;
		DMAZeroCurLen = len - sizeof(uint32);
	} else
	{
		DEBUGFLAG(dbIODetail,
			PUTS("ConfigDMA ch1: ptr/len="); 
			PUTHEX(dst); PUTHEX(len); )
		DMAOneCurPtr = (uint32)dst;
		DMAOneCurLen = len - sizeof(uint32);
	}
}

/***************************************************************************
 Set up DMA memory address & length (Next).
*/
static void 
ConfigNextDMA(uint8 channel, uint8 *dst, int32 len)
{
	if (channel == DMA_CH0)
	{
		DEBUGFLAG(dbIODetail,
			PUTS("ConfigDMA next ch0: ptr/len="); 
			PUTHEX(dst); PUTHEX(len); )
		DMAZeroNextPtr = (uint32)dst;
		DMAZeroNextLen = len - sizeof(uint32);
	} else
	{
		DEBUGFLAG(dbIODetail,
			PUTS("ConfigDMA next ch1: ptr/len="); 
			PUTHEX(dst); PUTHEX(len); )
		DMAOneNextPtr = (uint32)dst;
		DMAOneNextLen = len - sizeof(uint32);
	}
}

/***************************************************************************
 Seek to a specified block number and start playing.
 Any data currently in the DiscBuffers is discarded.
*/
static int
StartPlaying(uint32 block)
{
	int err;
	int state;
	struct DiscBuffer *bp;
	uchar report[DRIVE_STATE_REPORT_SIZE];

	DisableDMA(DMA_CH0|DMA_CH1);

	/* Enable CIP reports and disard the first one. */
	EnableReports(REPORT_DRIVE_STATE, REPORT_ENABLE_DETAIL);
	err = GetResponse(report, sizeof(report), REPORT_DRIVE_STATE, NOTIMEOUT);
	if (err < OK)
		goto Error;
	state = report[1];
	if (DRV_ERROR_STATE(state) || DRV_OPEN_STATE(state))
		goto Error;
	/* Seek to the block. */
	err = SeekDisc((int32)block);
	if (err < OK) return err;
	/* Wait for drive to enter the PAUSE state.
	 * It is not safe to enable DMA before the drive is in PAUSE,
	 * because garbage gets spit out on the DMA channels when 
	 * the drive enters PAUSE. */
	do {
		err = GetResponse(report, sizeof(report), 
				REPORT_DRIVE_STATE, NOTIMEOUT);
		if (err < OK)
			goto Error;
		state = report[1];
		DEBUGFLAG(dbIO, if (state != DRV_PAUSE) { 
			PUTS("CIP="); PUTHEX(state); } )
		if (DRV_ERROR_STATE(state))
			goto Error;
#ifdef USE_SWITCH_REPORTS
		if (CheckSwitch())
			goto Error;
#endif /* USE_SWITCH_REPORTS */
		if (DRV_OPEN_STATE(state))
			goto Error;
	} while (state != DRV_PAUSE);
	EnableReports(REPORT_DRIVE_STATE, REPORT_NEVER);

	/* Set up buffers and DMA. */
	EmptyBuffers();
	IntransitBuffer = FirstBuffer;
	IntransitBuffer->BlockNumber = block;
	IntransitBuffer->Intransit = TRUE;
	ConfigDMA(DMA_CH0, 
		(uint8*)&IntransitBuffer->DiscBlock, sizeof(struct DiscBlock));
	/* Set up "next" DMA pointers so data for the next block 
	   (after the current one) has someplace to go. */
	/* Note a weirdness here: we point the next DMA pointers to a
	   buffer which potentially already has good data in it.
	   But we don't update that buffer's BlockNumber or Intransit status
	   because we HOPE we're going to get the current data out of that
	   buffer before data starts DMAing into it.
	   We don't use the safer method of updating the BlockNumber here,
	   because then we would need at least three buffers.   With this
	   algorithm, we only need two buffers.  The cost is that after
	   after getting data out of a buffer, we have to check again to
	   make sure the buffer hasn't changed (see WaitReadBlock). */
	bp = IntransitBuffer->Next;
	ConfigNextDMA(DMA_CH0, 
		(uint8*)&bp->DiscBlock, sizeof(struct DiscBlock));
	EnableDMA(DMA_CH0);

	/* Start playing */
	err = SendDiscCmd(PlayCmd, sizeof(PlayCmd));
	if (err < OK) return err;
	return OK;

Error:
	EnableReports(REPORT_DRIVE_STATE, REPORT_NEVER);
	return -1;
}

/***************************************************************************
 Is DMA done?
*/
static int
DMADone(void)
{
	uint32 bits;

	/* See if we've gotten an interrupt for DMA completion. 
	   Dipir has interrupts disabled, so we must poll the intr bit. */
	bits = *(vuint32*)SetInt1Bits;
	if ((bits & INT1_DMAfrCD0) == 0)
		return 0;
	/* DMA is done: clear the interrupt. */
	*(vuint32*)ClrInt1Bits = INT1_DMAfrCD0;
	return 1;
}

/***************************************************************************
 Check DMA hardware and update software state.
*/
static void
UpdateDMA(void)
{
	struct DiscBuffer *bp;

	if (IntransitBuffer == NULL || !DMADone())
		return;

	/* Update status of current buffer (just finished)
	   and next buffer (now receiving data). */
	bp = IntransitBuffer;
	IntransitBuffer = IntransitBuffer->Next;
	bp->Intransit = FALSE;
	IntransitBuffer->Intransit = TRUE;
	IntransitBuffer->BlockNumber = bp->BlockNumber + 1;

	/* Set up "next" DMA pointers so data for the next block 
	   (after the current one) has someplace to go. */
	bp = IntransitBuffer->Next;
	ConfigNextDMA(DMA_CH0, (uint8*)&bp->DiscBlock, sizeof(struct DiscBlock));
}

/***************************************************************************
 Convert block number to Minutes/Seconds/Frames.
*/
static uint32
BlockToMSF(uint32 block)
{
	uint32 m, s, f;
	m = block / (FRAMEperSEC*SECperMIN);
	s = (block % (FRAMEperSEC*SECperMIN)) / FRAMEperSEC;
	f = block % FRAMEperSEC;
	return (BIN2BCD(m) << 16) | (BIN2BCD(s) << 8) | BIN2BCD(f);
}

/***************************************************************************
 Start reading a specified block from the disc into de->CurrentBuff.
*/
static void
AsyncReadBlock(int block)
{
	uint32 latest;
#define	WAITLIMIT	(4) /* blocks */

	DEBUGFLAG(dbIO, PUTS("ReadBlock:"); PUTHEX(block); )
	if (DRIVE_OPEN())
		return;
	ReqBlock = block;
	UpdateDMA();
	if (FindBuffer(block) != NULL)
	{
		/* The block is already in a buffer
		   (it may be Intransit, but that's ok). */
		return;
	}
	latest = LatestBlock();
	if (latest != 0 && block > latest && block < latest + WAITLIMIT)
	{
		/* The block is close enough that we won't bother 
		   to seek; we'll just wait until we play to it. */
		return;
	}
	(void) StartPlaying(block);
}

/***************************************************************************
 Copy a block of data.
 Source & destination must be uint32-aligned, len is a multiple of uint32.
*/
static void
memcpy32(uint32 *d, uint32 *s, int len)
{
	while (len > 0) 
	{
		*d++ = *s++;
		len -= sizeof(uint32);
	}
}

/***************************************************************************
 Wait for a disc read initiated by AsyncReadBlock to complete.
*/
static int
WaitReadBlock(void)
{
	int err;
	uint32 msf;
	uint32 retries;
	uint32 timeInLoop;
	struct DiscBuffer *bp;

	retries = 0;
Start:
	if (IntransitBuffer == NULL)
	{
		PUTS("WaitReadBlock: no intransit\n");
		goto Restart;
	}

	/* Wait for DMA completion.  When DMA completes, the buffer
	   will be available via FindBuffer. */
	RESETTIMER();
	timeInLoop = 0;
	for (;;) 
	{
		if (READTIMER() > 100*MILLISEC)
		{
			timeInLoop += 100;
			if (DRIVE_OPEN())
				return -1;
			RESETTIMER();
			if (timeInLoop > 1000*MILLISEC)
			{
				PUTS("Read timeout\n");
				goto Restart;
			}
		}
		UpdateDMA();
		/* See if the buffer is available and not in-transit.
		 * If so, break out of this loop. */
		bp = FindBuffer(ReqBlock);
		if (bp != NULL && !bp->Intransit)
			break;
		if (IntransitBuffer->BlockNumber > ReqBlock)
		{
			/* Oops, we are past the block we want. */
			PUTS("Missed block "); PUTHEX(ReqBlock);
			PUTS(" Now reading "); 
				PUTHEX(IntransitBuffer->BlockNumber);
			goto Restart;
		}
	}

	if ((bp->DiscBlock.db_Header >> 8) != 
	    (bp->DiscBlock.db_Completion >> 8))
	{
		PUTS("MSF mismatch (1): hdr/comp="); 
		PUTHEX(bp->DiscBlock.db_Header);
		PUTHEX(bp->DiscBlock.db_Completion);
		goto Restart;
	}

	CorrectedECC = 0;
	if (bp->DiscBlock.db_Completion & COMPL_ECC)
	{
		/* Try ECC correction. */
		PUTS("WaitReadBlock: ECC error");
		CorrectedECC = SECTORECC((uint8*)&bp->DiscBlock);
		PUTS("ECC="); PUTHEX(CorrectedECC);
		if (CorrectedECC < 0)
			goto Restart;
	}
	memcpy32((uint32*)(de->CurrentBuff), (uint32*)(&bp->DiscBlock.db_Data), 
		de->BlockSize);

	UpdateDMA();
	if (bp->BlockNumber != ReqBlock || bp->Intransit)
	{
		/* Yikes! It changed!
		   This is the weird case.  The DMA caught up with us,
		   and we didn't get the data out in time.  It may have
		   been corrupted by the DMA before we copied it.
		   Start over. */
		goto Restart;
	}

	/* Make sure the header word and completion word match the block
	   number we requested. */
	msf = BlockToMSF(ReqBlock);
	if ((bp->DiscBlock.db_Header >> 8) != msf)
	{
		PUTS("MSF mismatch (2): exp/hdr/comp="); PUTHEX(msf); 
		PUTHEX(bp->DiscBlock.db_Header);
		PUTHEX(bp->DiscBlock.db_Completion);
		goto Restart;
	}
	return OK;

Restart:
	if (++retries > READ_RETRIES)
	{
		PUTS("Max restarts exceeded\n");
		return -1;
	}
	PUTS("WaitReadBlock: restarting seek/read\n");
	err = StartPlaying(ReqBlock);
	if (err < OK) return err;
	goto Start;
}

/***************************************************************************
 Read a specified block (synchronously) from the disc into de->CurrentBuff.
*/
static int
ReadBlock(int b)
{

	AsyncReadBlock(b);
	return WaitReadBlock();
}

/***************************************************************************
*/
static int32
LastECC(void)
{
	return CorrectedECC;
}

/***************************************************************************
 Clear out any junk in the XBUS status FIFO.
*/
static void
FlushStatusFifo(void)
{
	uint8 poll;
	uint8 junk;
	ExpansionBus *xb = XBUS_ADDR;

	for (;;)
	{
		READPOLL(poll, de->device);
		if (!(poll & XBUS_POLL_STAT_VALID)) 
		{
			/* No more status bytes.
			 * Make sure there are no more coming. */
			Delay(100*MILLISEC);
			READPOLL(poll, de->device);
			if (!(poll & XBUS_POLL_STAT_VALID)) 
				break;
		}
		junk = (uint8)(xb->xb_CmdStat[0]);
		PUTS("Flush status byte "); PUTHEX(junk);  
	}
}

/***************************************************************************
 Extract information from various types of Q reports.
*/
static void
SetDiscInfoA0(DiscInfo *di, uchar *report)
{
	di->di_FirstTrackNumber = BCD2BIN(report[QR_PMIN]);
	di->di_DiscId = report[QR_PSEC]; 
	DEBUGFLAG(dbQReports, 
		PUTS("POINT=A0: FirstTrack="); PUTHEX(di->di_FirstTrackNumber);
		PUTS("DiscId="); PUTHEX(di->di_DiscId); )
}

static void
SetDiscInfoA1(DiscInfo *di, uchar *report)
{
	di->di_LastTrackNumber = BCD2BIN(report[QR_PMIN]); 
	DEBUGFLAG(dbQReports, 
		PUTS("POINT=A1: LastTrack="); PUTHEX(di->di_LastTrackNumber); )
}

static void
SetDiscInfoA2(DiscInfo *di, uchar *report)
{
	di->di_MSFEndAddr_Min = BCD2BIN(report[QR_PMIN]);
	di->di_MSFEndAddr_Sec = BCD2BIN(report[QR_PSEC]);
	di->di_MSFEndAddr_Frm = BCD2BIN(report[QR_PFRAME]);
	DEBUGFLAG(dbQReports,
		PUTS("POINT=A2: Min/Sec/Frm ");
		PUTHEX(di->di_MSFEndAddr_Min);
		PUTHEX(di->di_MSFEndAddr_Sec);
		PUTHEX(di->di_MSFEndAddr_Frm); )
}

static void
SetTOCInfo(TOCInfo *toc, uchar *report)
{
	/* toc_AddrCntrl has ADR in high nibble and CTL in low nibble, 
	 * which is backwards from what we read off the disc. */
	toc->toc_AddrCntrl = SwapNibbles(report[QR_ADRCTL]);
	toc->toc_CDROMAddr_Min = BCD2BIN(report[QR_PMIN]);
	toc->toc_CDROMAddr_Sec = BCD2BIN(report[QR_PSEC]);
	toc->toc_CDROMAddr_Frm = BCD2BIN(report[QR_PFRAME]);
	DEBUGFLAG(dbQReports,
		PUTS("TOC entry:"); 
		PUTS("AdrCtl="); PUTHEX(toc->toc_AddrCntrl);
		PUTS("Min/Sec/Frm ");
		PUTHEX(toc->toc_CDROMAddr_Min);
		PUTHEX(toc->toc_CDROMAddr_Sec);
		PUTHEX(toc->toc_CDROMAddr_Frm); )
}

/***************************************************************************
 Read the table of contents and disc info from the disc.
*/
static int
ReadTOCAndDiscInfo(TOCInfo *toc, DiscInfo *di, int track, Boolean *pMultiSession)
{
	int err;
	uint32 got;
	uint32 retries;
	uchar point;
	uchar report[QCODE_REPORT_SIZE];

#define	GOT_A0		0x1
#define	GOT_A1		0x2
#define	GOT_A2		0x4
#define	GOT_TOC		0x8
#define	GOT_TOC1	0x10

	DEBUGFLAG(dbMajor, PUTS("ReadTOCAndDiscInfo:"); )
	if (DRIVE_OPEN())
		return -1;

	EmptyBuffers();

	/* Check to see if the info we need is already cached. */
	if (gotCachedInfo)
	{
		if (toc != NULL)
			*toc = cachedTOC1;
		if (di != NULL)
			*di = cachedDiscInfo;
		if (pMultiSession != NULL)
			*pMultiSession = cachedMultiSession;
		DEBUGFLAG(dbMajor, PUTS(" Found all info in cache!\n"); )
		return OK;
	}

	/* Pause the drive and change to AUDIO mode, so we can read the TOC. */
	err = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
	if (err < OK)
	{
		PUTS("ReadTOC: error in pause");
		return err;
	}
	err = WaitDriveState(DRV_PAUSE);
	if (err < OK)
	{
		PUTS("ReadTOC: error waiting for PAUSE");
		return err;
	}
	err = SendDiscCmd(AudioFormatCmd, sizeof(AudioFormatCmd));
	if (err < OK)
	{
		PUTS("ReadTOC: error in sector format");
		return err;
	}
	retries = 0;
Again:
	/* Disable all reports (except switch reports). */
	EnableReports(REPORT_DRIVE_STATE, REPORT_NEVER);
	EnableReports(REPORT_QCODE, REPORT_NEVER);
	/* Flush all status bytes (is this necessary?) */
	FlushStatusFifo();

	/* Seek to beginning of TOC. */
	err = SeekDisc((int32)0);
	if (err < OK) 
	{
		PUTS("ReadTOC: error in seek(0)"); 
		return err;
	}

	/* Start playing the data. 
	 * Watch the QCODEs and parse them to get the TOC and disc info. */
	err = SendDiscCmd(PlayCmd, sizeof(PlayCmd));
	if (err < OK) 
	{
		PUTS("ReadTOC: error in play");
		return err;
	}
	err = WaitDriveState(DRV_PLAY);
	if (err < OK)
	{
		PUTS("ReadTOC: error waiting for PLAY");
		return err;
	}

	EnableReports(REPORT_QCODE, REPORT_ENABLE);

	got = 0;
	for (;;)
	{
		if (DRIVE_OPEN())
			return -1;

		if ((got & (GOT_A0|GOT_A1|GOT_A2|GOT_TOC1)) == 
			   (GOT_A0|GOT_A1|GOT_A2|GOT_TOC1))
			gotCachedInfo = TRUE;

		if (gotCachedInfo && ((got & GOT_TOC) || toc == NULL))
		{
			/* Got everything we need. */
			break;
		}

		err = GetResponse(report, sizeof(report), REPORT_QCODE, 
				100*MILLISEC);
		if (err < OK)
			return -1;
		switch (report[QR_ADRCTL] & 0xF)
		{
		case 0: /* Is 0 the same as 1? */
		case 1:
			if (report[QR_TNO] != 0)
			{
				/* This is a normal data-area entry, 
				 * not a lead-in (TOC) entry.
				 * We've run off the end of the TOC. */
				DEBUGFLAG(dbMajor, PUTS("****** TNO != 0; "
						"return *****"); )
				goto Retry;
			}

			switch (report[QR_POINT])
			{
			case 0xA0:
				got |= GOT_A0;
				if (!gotCachedInfo)
				{
					SetDiscInfoA0(&cachedDiscInfo, report);
					/* If this is a CD-I disc, return
					 * error.  A CD-I TOC is different
					 * than normal TOCs, and we don't have
					 * the code here to interpret it. */
					if (cachedDiscInfo.di_DiscId == 0x10)
					{
						PUTS("CD-I disc!"); 
						return -1;
					}
				}
				if (di != NULL) 
					SetDiscInfoA0(di, report);
				break;
			case 0xA1:
				got |= GOT_A1;
				if (!gotCachedInfo)
					SetDiscInfoA1(&cachedDiscInfo, report);
				if (di != NULL) 
					SetDiscInfoA1(di, report);
				break;
			case 0xA2:
				got |= GOT_A2;
				if (!gotCachedInfo)
					SetDiscInfoA2(&cachedDiscInfo, report);
				if (di != NULL) 
					SetDiscInfoA2(di, report);
				break;
			default:
				point = BCD2BIN(report[QR_POINT]);
				if (toc != NULL && point == track)
				{
					/* This is the track we want. */
					got |= GOT_TOC;
					SetTOCInfo(toc, report);
				}
				if (point == 1)
				{
					/* This is track 1; cache it */
					got |= GOT_TOC1;
					SetTOCInfo(&cachedTOC1, report);
				}
				break;
			}
			break;
		case 2: /* UPC/EAN */
		case 3: /* ISRC */
			break;
		case 5: /* multisession */
			if (!gotCachedInfo)
				cachedMultiSession = TRUE;
			if (pMultiSession != NULL)
				*pMultiSession = TRUE;
			break;
		default:
			  PUTS("Adr?="); 
			  PUTHEX(report[QR_ADRCTL]); 
			break;
		}
	}

	/* Disable Qcode reports. */
	EnableReports(REPORT_QCODE, REPORT_NEVER);
	/* Pause the drive and put it back in DATA mode. */
	err = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
	if (err < OK)
	{
		PUTS("ReadTOC: error in PAUSE");
		return err;
	}
	err = WaitDriveState(DRV_PAUSE);
	if (err < OK)
	{
		PUTS("ReadTOC: error waiting for PAUSE");
		return err;
	}
	err = SendDiscCmd(DataFormatCmd, sizeof(DataFormatCmd));
	if (err < OK)
	{
		PUTS("ReadTOC: error in FORMAT");
		return err;
	}
	return OK;

Retry:
	if (++retries <= TOC_RETRIES)
		goto Again;
	PUTS("FAIL");
	return -1;
}

/***************************************************************************
 Read the table of contents from the disc.
*/
static int
ReadTOC(int track)
{
	int err;
	err = ReadTOCAndDiscInfo((TOCInfo*)de->statbuff, (DiscInfo*)NULL, 
					track, (Boolean*)NULL);
	DEBUGFLAG(dbMajor, 
		{ TOCInfo *ti = (TOCInfo*)de->statbuff;
		  PUTS("ReadTOC done, track "); PUTHEX(track);
		  PUTS("AdrCtl="); PUTHEX(ti->toc_AddrCntrl);
		  PUTS("TrackN="); PUTHEX(ti->toc_TrackNumber);
		  PUTS("M/S/F="); PUTHEX(ti->toc_CDROMAddr_Min); 
		  PUTHEX(ti->toc_CDROMAddr_Sec); PUTHEX(ti->toc_CDROMAddr_Frm); 
		} )
	return err;
}

/***************************************************************************
 Read the disc info from the disc.
*/
static int
ReadDiscInfo(void)
{
	int err;
	err = ReadTOCAndDiscInfo((TOCInfo*)NULL, (DiscInfo*)de->statbuff, 
					-1, (Boolean*)NULL);
	DEBUGFLAG(dbMajor, 
		{ DiscInfo *di = (DiscInfo*)de->statbuff; 
		  PUTS("ReadDiscInfo done @"); PUTHEX(di);
		  PUTS("DiscId="); PUTHEX(di->di_DiscId);
		  PUTS("FirstTrack="); PUTHEX(di->di_FirstTrackNumber);
		  PUTS("LastTrack="); PUTHEX(di->di_LastTrackNumber);
		  PUTS("EndAddr="); PUTHEX(di->di_MSFEndAddr_Min);
		  PUTHEX(di->di_MSFEndAddr_Sec); PUTHEX(di->di_MSFEndAddr_Frm);
		} )
	return err;
}

/***************************************************************************
 Read the session info from the disc (is it a multisession disc?).
*/
static int
ReadSessionInfo(void)
{
	int err;
	Boolean multiSession;

	err = ReadTOCAndDiscInfo((TOCInfo*)NULL, (DiscInfo*)NULL,
					-1, &multiSession);
	de->statbuff[1] = 0;
	de->statcnt = 2;
	if (err < OK) return err;
	if (multiSession)
		de->statbuff[1] = 0x80;
	return OK;
}

/***************************************************************************
 Determine whether the disc is a "gold" disc (write-once).
*/
static int32 
CheckGoldDisc(void)
{
	return IsGoldDisc;
}

static void
BeginLongOp(void)
{
	int err;

	if (DRIVE_OPEN())
		return;
	DisableDMA(DMA_CH0|DMA_CH1);
	err = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
	if (err < OK) return;
	err = WaitDriveState(DRV_PAUSE);
	if (err < OK) return;
	err = SendDiscCmd(SingleSpeedCmd, sizeof(SingleSpeedCmd));
	if (err < OK) return;
	err = SendDiscCmd(PlayCmd, sizeof(PlayCmd));
	if (err < OK) return;
	err = WaitDriveState(DRV_PLAY);
	if (err < OK) return;
	ASYNCSQUIRTOUTCMD(CheckWOCmd, sizeof(CheckWOCmd));
	LongOpWorking = TRUE;
}

static void
EndLongOp(void)
{
	int err;
	uint32 ratio_int, ratio_frac;
	uint32 ratio256;
	uchar resp[CHECK_WO_REPORT_SIZE];

	if (!LongOpWorking)
		err = -1;
	else
	{
		/* CheckWO takes around 750ms to complete.  The 1 sec timeout
		 * is designed to handle the case of us never getting the
		 * CheckWO response back AT ALL.  This timeout begins after
		 * we've completed the RSA check(s).  If the RSA completes
		 * early (say, when it fails), we must guarantee that the
		 * CheckWO has had time to complete BEFORE we send any
		 * EjectCmd. */
		err = GetResponse(resp, sizeof(resp), CMD_CHECK_WO, 1000*MILLISEC);
	}
	if (err == OK)
	{
		/* Get k11 (11KHz energy), k22 (22KHz energy), 
		 * and ratio (8.8 binary point ratio).
		 * It is a gold (write-once) disc if k22/k11 > ratio. */
		k11 = ((uint32)resp[1] << 16) + 
			((uint32)resp[2] << 8) + 
			((uint32)resp[3]);
		k22 = ((uint32)resp[4] << 16) + 
			((uint32)resp[5] << 8) + 
			((uint32)resp[6]);
		ks_good = TRUE;
		ratio_int = resp[7];
		ratio_frac = resp[8];
		PUTS("k11="); PUTHEX(k11);
		PUTS("k22="); PUTHEX(k22);
		PUTS("ratio int="); PUTHEX(ratio_int);
		PUTS("     frac="); PUTHEX(ratio_frac);
		/*
		 * Rearrange expression to avoid need for floating point.
		 * So instead of:
		 *	(k22 / k11) > (ratio)
		 * we use:
		 *	(k22 * 256) > (k11 * ratio * 256)
		 */
		ratio256 = (256 * ratio_int) + ratio_frac;
		if (ratio256 >= MAX_UINT32 / k11)
		{
			/*
			 * The (k11*ratio256) below would overflow.
			 * But since k22 is 3 bytes, k22*256 <= MAXUINT32.
			 * And from above "if", (k11*ratio256) >= MAXUINT32.
			 * Therefore (k22 * 256) <= (k11 * ratio256).
			 */
			IsGoldDisc = FALSE;
		} else
		{
			IsGoldDisc = (k22 * 256 > k11 * ratio256);
		}
	}
	if (DRIVE_OPEN())
		return;
	err = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
	if (err < OK) return;
	err = WaitDriveState(DRV_PAUSE);
	if (err < OK) return;
	(void) SendDiscCmd(DoubleSpeedCmd, sizeof(DoubleSpeedCmd));
}

/***************************************************************************
 Do we have a valid 3DO disc?
 Must be called after BeginLongOp/EndLongOp.
*/
static int32
CheckCopyProtect(uint32 arg)
{
	int err;
	int32 IsOkDisc;
	uint32 ratio256;
	uchar resp[COPY_PROT_THRESH_RESP_SIZE];

	if (!ks_good)
		return TRUE;
	err = SendDiscCmdWithResponse(CopyProtThreshCmd, 
		sizeof(CopyProtThreshCmd), resp, sizeof(resp), NOTIMEOUT);
	if (err < 0)
		return FALSE;
	if (resp[1] == 0 && resp[2] == 0)
		ratio256 = arg;
	else
		ratio256 = (256 * (uint32)resp[1]) + (uint32)resp[2];
	if (ratio256 >= MAX_UINT32 / k11)
	{
		IsOkDisc = TRUE;
	} else
	{
		IsOkDisc = (k22 * 256 < k11 * ratio256);
	}
	return IsOkDisc;
}

/***************************************************************************
*/
static void 
memset(uint8 *p, uint8 v, uint32 count)
{
	while (count > 0)
	{
		*p++ = v;
		count--;
	}
}

/***************************************************************************
 We don't provide a default DisplayImage routine; it must be in system ROM.
*/
int32
DefaultDisplayImage(void *image0, char *pattern)
{
	return 0;
}

/***************************************************************************
 Display error message if RSA check of firmware fails.
*/
static void
DisplayFirmwareError(void)
{
	/* This pile of bits is a VideoImage structure which 
	 * displays the "SELF TEST FAILED" message. */
	static uint32 firmwareMessage[] = {
		0x01424f4f, 0x54534352, 0x00000108, 0x00160060,
		0x01000000, 0x00000000, 0xeaaaaaaa, 0xaaaaaaaa,
		0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaab,
		0xc0000015, 0x50555440, 0x00555400, 0x00555455,
		0x54155055, 0x54000003, 0xc00000c0, 0x08c000c0,
		0x00c00000, 0x000300c0, 0x00c00803, 0x00000003,
		0xc000002a, 0xa4ea80c0, 0x00ea8000, 0x000300ea,
		0x802aa403, 0x00000003, 0xc0000095, 0x58d554d5,
		0x54c00000, 0x000300d5, 0x54955803, 0x00000003,
		0xc0000000, 0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000003, 0xc0000000, 0x00eaa806,
		0x400b80c0, 0x00eaa8ea, 0xa40fc000, 0x00000003,
		0xc0000000, 0x00d54060, 0x240300c0, 0x00d540c0,
		0x0c0b8000, 0x00000003, 0xc0000000, 0x00c000ea,
		0xac0300c0, 0x00c000c0, 0x0c054000, 0x00000003,
		0xc0000000, 0x00800080, 0x080a80aa, 0xa8aaa8aa,
		0xa00a8000, 0x00000003, 0xd5555555, 0x55555555,
		0x55555555, 0x55555555, 0x55555555, 0x55555557,
	};

	DISPLAYIMAGE((void*)firmwareMessage, HW_SPLASH_PATTERN);
}

/***************************************************************************
 Read a block of the firmware.
*/
static int 
ReadFirmwareBlock(uint32 block, uint8 *buffer, 
		uint32 *pNumBlocks, uint32 *pSigOffset)
{
	int err;
	uchar ReadFirmwareCmd[4];
	uchar resp[READ_FIRMWARE_RESP_SIZE];

	DisableDMA(DMA_CH0|DMA_CH1);
	ConfigDMA(DMA_CH0, buffer, FIRMWARE_BLOCK_SIZE);
	EnableDMA(DMA_CH0);

	ReadFirmwareCmd[0] = CMD_READ_FIRMWARE;
	ReadFirmwareCmd[1] = (uint8)block;
	ReadFirmwareCmd[2] = 0;
	ReadFirmwareCmd[3] = 0;
	err = SendDiscCmdWithResponse(ReadFirmwareCmd, sizeof(ReadFirmwareCmd),
			resp, sizeof(resp), 2000*MILLISEC);
	if (err < OK)
	{
		PUTS("ReadFirmware: SendCmd error\n");
		DisableDMA(DMA_CH0|DMA_CH1);
		return err;
	}
	*pNumBlocks = (uint32) resp[1];
	*pSigOffset = (uint32) MakeInt16(resp[2], resp[3]);
	RESETTIMER();
	while (!DMADone())
	{
		if (READTIMER() > 2000*MILLISEC)
		{
			PUTS("ReadFirmware: DMA timeout\n");
			DisableDMA(DMA_CH0|DMA_CH1);
			return -1;
		}
	}
	DisableDMA(DMA_CH0|DMA_CH1);
	return OK;
}

/***************************************************************************
 Check validity of the firmware.
 This is a crude attempt to prevent hardware licencees from modifying
 the firmware without approval from 3DO.
*/
static void
CheckFirmware(void)
{
	uint32 block;
	uint8 *buffer;
	uint32 numBlocks;
	uint32 sigOffset;
	uint8 sig[SIG_LEN];

	buffer = DiscBuffer1.DiscBlock.db_Data;
	block = 0;
	DIGESTINIT();
	do {
		if (ReadFirmwareBlock(block, buffer, 
				&numBlocks, &sigOffset) < OK)
		{
			PUTS("CheckFirmware: read error\n");
			goto Bogus;
		}
		if (numBlocks < MIN_FIRMWARE_SIZE / FIRMWARE_BLOCK_SIZE)
		{
			PUTS("CheckFirmware: too small "); PUTHEX(numBlocks);
			goto Bogus;
		}
		if (block == sigOffset / FIRMWARE_BLOCK_SIZE)
		{
			sigOffset %= FIRMWARE_BLOCK_SIZE;
			MEM_MOVE(sig, buffer + sigOffset, SIG_LEN);
			memset(buffer + sigOffset, 0, SIG_LEN);
		}
		UPDATEDIGEST(buffer, FIRMWARE_BLOCK_SIZE);
	} while (++block < numBlocks);
	FINALDIGEST();
	if (RSAFINALTHDO(sig, SIG_LEN) == 0)
	{
		PUTS("CheckFirmware: RSA fail\n");
		goto Bogus;
	}
	return;

Bogus:
	/* Firmware check failed.  Display error message and hang. */
	PUTS("===== CheckFirmware: BOGUS!");
	DisplayFirmwareError();
	for (;;) ;
}

/***************************************************************************
 Determine the ID of the disc manufacturing plant.
*/
static int32 
GetMfgPlant(void)
{
	return 0;
}

/***************************************************************************
 Return the ID of this driver.
*/
static uint32
GetDriverInfo()
{
	return DIPIR_DEVICE_ID_LOWCOSTCDROM | LCCDDEV_VERREV;
}

/***************************************************************************
 Clean up device after we're done using it.
*/
static void
LCCD_DeinitDisc(void)
{
	int state;

	EnableReports(REPORT_DRIVE_STATE, REPORT_NEVER);
	EnableReports(REPORT_SWITCH, REPORT_NEVER);
	EnableReports(REPORT_QCODE, REPORT_NEVER);
	state = GetDriveState();
	if (state == DRV_PLAY)
	{
		(void) SendDiscCmd(PauseCmd, sizeof(PauseCmd));
	}
	FlushStatusFifo();
}

/***************************************************************************
 Standard table of driver routines for dipir to use.
*/
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
	DEVICE_ROUTINES_VERSION_0+4,
	WaitReadBlock,
	LCCD_DeinitDisc,
	BeginLongOp,
	EndLongOp,
	CheckCopyProtect,
	LastECC,
};

/***************************************************************************
 Initialize the driver and the disc drive.
*/
int
LCCD_InitDisc(struct DipirEnv *lde)
{
	int state;
	uint32 timeout;
	Boolean sentPause = FALSE;

	de = lde;
	dipr = de->DipirRoutines;

	DEBUGFLAG(dbMajor, PUTS("LCCD_InitDisc"); )
#ifdef USE_SWITCH_REPORTS
	SwitchPressed = FALSE;
	EnableReports(REPORT_SWITCH, REPORT_ENABLE);
#else
	EnableReports(REPORT_SWITCH, REPORT_NEVER);
#endif
	
	timeout = 0;
	for (;;)
	{
		if (timeout > 10000*MILLISEC)
			return 0;
		state = GetDriveState();
		if (state < OK)
			return 0;
#ifdef USE_SWITCH_REPORTS
		if (CheckSwitch())
			return 0;
#endif /* USE_SWITCH_REPORTS */
		if (state == DRV_PAUSE)
			break;
		switch (state) 
		{
		case DRV_STOPPING:
		case DRV_CLOSING:
		case DRV_FOCUSING:
		case DRV_SPINNINGUP:
		case DRV_SEEKING:
		case DRV_LATENCY:
			/* Wait for stable state */
			break;
		case DRV_STOP:
		case DRV_STOP_FOCUSED:
			/* The only way we can be in STOP state is as
			 * a transition to OPEN.  Just wait for OPEN. */
			break;
		case DRV_OPEN:
		case DRV_OPENING:
			PUTS("InitDisc: door open");
			CheckFirmware();
			return 0;
		case DRV_UNREADABLE:
		case DRV_SEEKFAILURE:
			EjectDisc();
			/* Fall thru */
		case DRV_FOCUSERROR:
		case DRV_STUCK:
			PUTS("InitDisc: drive error "); PUTHEX(state);
			CheckFirmware();
			return 0;
		case DRV_PLAY:
		default:
			/* Keep waiting... */
			if (!sentPause)
			{
				/* Try to go into PAUSE state. */
				state = SendDiscCmd(PauseCmd, sizeof(PauseCmd));
				if (state < OK)
				{
					PUTS("InitDisc: error in pause");
					return 0;
				}
				sentPause = TRUE;
			}
			break;
		}
		Delay(100*MILLISEC);
		timeout += 100*MILLISEC;
	}

	if (SendDiscCmd(DoubleSpeedCmd, sizeof(DoubleSpeedCmd)) < OK)
	{
		PUTS("error in Speed cmd");
	}
	/* Initialize our device routines */
	de->dvr = &dvr;
	de->BlockSize = DISC_BLOCK_SIZE;
	/* Initialize buffers */
	InitBuffers();
	EmptyBuffers();
	DEBUGFLAG(dbMajor, PUTS("ready"); )
	return 1;
}

#else /* LCCD_DRIVER */

int LCCD_InitDisc(struct DipirEnv *lde) { return 0; }

#endif /* LCCD_DRIVER */
