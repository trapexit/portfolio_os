/*
$Id: lccddriver.c,v 1.15 1995/02/08 22:10:43 bungee Exp $

$Log: lccddriver.c,v $
 * Revision 1.15  1995/02/08  22:10:43  bungee
 * Modified to use +/- 1% for variable pitch (to match MKE), instead of 5%.
 * Also modified status byte that is returned in the clamshell case when it's
 * stuck shut (and there was previously a disc present).  If we're playing and
 * then try to eject the disc in s/w (ie, CDROMCMD_OPEN_DRAWER), and we land
 * in the stuck state (ie, clamshell), we now clear the DiscIn bit in the
 * status byte.
 *
 * Revision 1.14  1995/02/01  23:22:55  bungee
 * Make sure that, if we've just pressed the eject button (and we're in the
 * process of opening the drawer), that we make sure that the CloseSwitch has
 * disengaged before we allow a separate button press to close the drawer.
 * This insures that we dipir, and don't get hung waiting for a never-occuring
 * event.
 *
 * Revision 1.13  1995/01/27  02:29:44  bungee
 * Fix needed to handle opening and closing the drawer mid-dipir.
 *
 * Revision 1.12  1995/01/25  10:07:18  bungee
 * Very minor modification to handle the situation where the initial state of
 * the drawer (when using a drawer mechanism) is STUCK.  Previously the button
 * would be unresponsive...now the drawer is opened.
 *
 * Revision 1.11  1995/01/20  02:44:44  bungee
 * Fixed clamshell support (would hang if booted with clamshell open).
 * Changed/removed some DBUG()'s.  Fixed VERIFY_CRC_CHECK and bufcmp()...(both
 * are dead code).  Removed KICK_START_PREFETCHING (dead code).
 *
 * Revision 1.10  1994/12/21  18:03:36  bungee
 * Removed some double-slash comments that were "accidentally" left in.  Oh
 * God!  :^o
 *
 * Revision 1.9  1994/12/15  02:10:38  bungee
 * Added TOC retry support.
 * Moved 'killDMA' label in an attempt to prevent possible DMA hardware lockup.
 *
 * Revision 1.8  1994/12/08  19:47:38  bungee
 * Several changes and enhancements:
 *   - Added CD-i support.
 *   - Fixed problem in CopySectorData() for Mode2 sectors.
 *   - Added kludge to deal with errors in Mode2 headers.
 *   - Made header/completion-word sanity checking more robust.  (Also helps to
 *     weed out those nasty hardware flakies!)
 *   - Changed some DBUG()'s.
 *   - Modified LCCDDriverInit() to only allow communication with the first
 *     LCCD device seen on xbus.
 *   - Modified the use of retryShift to match MKE functionality (previously
 *     functionality was a superset of MKE).
 *   - Removed unneeded DMAREQDIS line from EnableLCCDDMA().
 *
 * Revision 1.7  1994/11/29  19:54:06  bungee
 * Changed highly-intuitive, extremely-readable double-slash comments to the
 * extremely-illegible, poorly-designed carry-over-from-weakling-pascal
 * slash-star comment pairs...because on January 23, 2091 someone may actually
 * be convoluted enough to try and compile this code with some 1930's "C"
 * compiler that doesn't support compiler extensions, one of which being C++
 * style comments.
 *
 * Revision 1.6  1994/11/17  20:25:25  bungee
 * Several itty-bitty tweaks made here:
 *  - Updated CDROMCMD_CLOSE_DRAWER support.
 *  - Modified CompleteWorkingIOReq() so it can be called from the driver's
 *    dispatch routine.
 *  - Changed DEBUG defines to DEVELOPMENT.
 *  - Now omits "retry count expired" if errorRecovery=CIRC_RETRIES_ONLY and
 *    retry count is zero.
 *  - Updated DataAvailNow() to handle failing EDC/CRC on Mode2Form2 discs
 *    when they have not implemented it (EDC on M2F2 is _optional_).
 *  - Now allow unexpected state after dipir (during INIT_REPORTS) in
 *    DEVELOPMENT mode.
 *  - Updated LCCDDriverInit() to properly teardown in event of an error, by
 *    adding CHK4ERRwithACTION() macro.  Also removed redundant io_Error check.
 *  - Added support for new system errors, ER_NoHardware and ER_Kr_TaskKilled.
 *
 * Revision 1.5  1994/11/09  23:58:45  bungee
 * Updated killing-daemon support.  Updated clamshell support.  Updated
 * DiscData() to use less stack; and removed potential bug.
 *
 * Revision 1.4  1994/11/02  20:39:23  bungee
 * Removed DEBUG.
 *
 * Revision 1.3  1994/11/02  16:31:31  bungee
 * Removed internal memcmp() now that it's been added to string.h.
 *
 * Revision 1.2  1994/11/02  14:50:17  bungee
 * Updated CallBackSuper() to the new calling convention.
 *
 * Revision 1.1  1994/11/02  13:17:06  bungee
 * Initial revision
 *

	File:		LCCDDriver.c
	
	Contains:	Kernel driver support for the Low Cost CD-ROM drive.
	
*/


#include "types.h"
#include "item.h"
#include "mem.h"
#include "nodes.h"
#include "list.h"
#include "device.h"
#include "driver.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "io.h"
#include "super.h"
#include "operror.h"
#include "strings.h"
#include "interrupts.h"
#include "inthard.h"

#include "lccddriver.h"

/* This only used to verify that hardware CRC (or software ECC) works */
#define VERIFY_CRC_CHECK	0
#define DEBUG_ECC			0

/* DBUG statements compiled in? */
#define DEBUG				0

#define kPrintGeneralStatusStuff	0x00000001
#define kPrintQtyNPosOfSectorReqs	0x00000002
#define kPrintSendCmdToDevStuff		0x00000004
#define kPrintDataAvailNowResponse	0x00000008
#define kPrintECCStats				0x00000010
#define kPrintDescrambledSubcode	0x00000020
#define kPrintSendCompleteIO		0x00000100
#define kPrintActionMachineStates	0x00000200
#define kPrintSignalAwakeSleep		0x00000400
#define kPrintDeviceStates			0x00000800
#define kPrintStatusResponse		0x00001000
#define	kPrintTOCAddrs				0x00002000
#define kPrintDipirStuff			0x00004000
#define kPrintVariPitch				0x00008000
#define kPrintMegaECCStats			0x00010000
#define	kPrintCmdOptions			0x00020000
#define kPrintWarnings				0x00040000
#define kPrintDeath					0x00080000
#define kPrintStatusByte			0x00100000
#define kPrintClamshell				0x00200000
#if DEBUG
	#define DBUG(a,x)	if (gPrintTheStuff & (a)) Superkprintf x
#else
	#define DBUG(a,x)
#endif

#define DBUG2(a,x)	if (gPrintTheStuff & (a)) Superkprintf x

/* Error statements always enabled */
#define DBUGERR(x)	Superkprintf x

/* macros */
#define	CHK4ERR(x,str)	if ((x)<0) { DBUGERR(str); return (x); }
#define	CHK4ERRwithACTION(x,action,str)	if ((x)<0) { action; DBUGERR(str); return (x); }
#define MIN(a,b)		(((a) < (b)) ? (a) : (b))
#define SwapNybble(x)	(((x) << 4) | (((x) >> 4) & 0x0F))

#define Check4ErrorOrNeededSleep(response)   		\
			if ((response) == kCmdNotYetComplete)	\
				break;       						\
			if ((response) < kCDNoErr)				\
				return (response)

/* prototype definitions */
/* utilities */
uint8	BCD2BIN(uint8 bcd);
uint8	BIN2BCD(uint8 bin);
uint32	Offset2BCDMSF(int32 offset);
uint32	MSF2Offset(uint32 MSF, uint8 selector);
uint32	bufcmp(uint8 *buf1, uint8 *buf2);
uint32	GenChecksum(uint32 *buf);

/* kernel mantra */
Item	createLCCDDriver(void);
int32	LCCDDriverInit(struct Driver *theDrvr);
uint32	DeviceIsLCCD(struct XBusDeviceStatus *xbus);
Err		LCCDNewDevice(int8 unit);
int32	LCCDDeviceInit(cdrom *cd);
void	LCCDDaemonStub(void);
Err		LCCDDriverDaemon(void);
Err		LCCDDriverDispatch(IOReq *theReq);
void	LCCDDriverAbortIO(IOReq *theReq);

/* internal buffer support routines */
Err		AllocateStandardBuffers(cdrom *cd);
Err		ResizeLinkedBuffers(cdrom *cd, uint32 size);

/* cmd (front-end) stuff */
Err		SendCmdWithWaitState(cdrom *cd, uint8 *cmd, uint8 cmdLen, uint8 desiredCIP, uint8 *errCode);
Err		SendCmdToDev(cdrom *cd, uint8 *cmdBuf, uint8 cmdLen);
IOReq	*CmdCallBack(IOReq *theReq);

/* snarfer (back-end) stuff */
Err		SetupSnarfer(cdrom *cd);
IOReq	*SnarfCallBack(IOReq *theReq);
void	ProcessSnarfBuffers(cdrom *cd);
void	HandleSwReport(cdrom *cd, uint8 *statusBytes, int32 x);
void	ParseTOCEntries(cdrom *cd, uint8 *statusBytes, int32 x);

/* dipir stuff */
Err		PrepareDuckAndCover(cdrom *cd);
IOReq	*DipirRecoverCallBack(IOReq *theReq);

/* misc. support routines */
void	InitDataSpace(cdrom *cd);
void	InitSubcodeSpace(cdrom *cd);
void	InitPrefetchEngine(cdrom *cd);
void	PrepareCachedInfo(cdrom *cd);
void	TakeDeviceOffline(cdrom *cd);
void	AbortCurrentIOReqs(cdrom *cd);
void	CompleteWorkingIOReq(IOReq *theReq, Err theErr);

/* daemon-state/cmd-action machines */
Err		OpenDrawer(cdrom *cd);
Err		CloseDrawer(cdrom *cd);
Err		InitReports(cdrom *cd);
Err		BuildDiscData(cdrom *cd);
Err		RecoverFromDipir(cdrom *cd);
Err		StopPrefetching(cdrom *cd);
void	ProcessClientIOReqs(cdrom *cd);
int32	CmdStatus(cdrom *cd, IOReq *theReq);
int32	DiscData(cdrom *cd, IOReq *theReq);
Err		Read(cdrom *cd);
Err		ReadSubQ(cdrom *cd);
Err		SetDefaults(cdrom *cd);
Err		ResizeBufferSpace(cdrom *cd);

/* read support routines */
Err		VerifyCmdOptions(uint32 opts);
uint8	MKE2BOB(cdrom *cd, int32 densityCode);
uint32	ValidBlockLengthFormat(uint8 format, int32 len);
uint32	DataFormatDontJive(cdrom *cd, uint32 theMSF, uint8 format);
int8	DataAvailNow(cdrom *cd, int32 sector, uint8 format, uint32 opts);
uint32	DataAvailRSN(cdrom *cd, int32 sector, uint8 format);
uint32	SanityCheckSector(cdrom *cd, int8 blk);
void	CopySectorData(cdrom *cd, uint8 *dst, uint8 *src, int32 blkLen, uint8 format);

/* subcode support */
uint32	SubcodeSyncedUp(cdrom *cd);
void	CopyDescrambledSubcode(cdrom *cd, uint8 *dst, uint8 *src);
void	CopySyncedSubcode(cdrom *cd, uint8 *dst);

/* dma-related */
void	EnableLCCDDMA(uint8 channels);
void	DisableLCCDDMA(uint8 channels);
void	ConfigLCCDDMA(uint8 channel, uint8 curORnext, uint8 *dst, int32 len);
int32	ChannelZeroFIRQ(void);
int32	ChannelOneFIRQ(void);

/* stuff required for daemon */
#define DAEMONUSERSTACK 256
static uint8	daemonStack[DAEMONUSERSTACK];

/* globals */
static TagArg	cdRomDriverTags[] =
{
	TAG_ITEM_NAME,				(void *)"LCCD-ROM",
	TAG_ITEM_PRI,				(void *)3,
	CREATEDRIVER_TAG_INIT,		(void *)(uint32)LCCDDriverInit,
	CREATEDRIVER_TAG_ABORTIO,	(void *)(uint32)LCCDDriverAbortIO,
	CREATEDRIVER_TAG_MAXCMDS,	(void *)9,
	CREATEDRIVER_TAG_DISPATCH,	(void *)(uint32)LCCDDriverDispatch,
	TAG_END,					NULL
};

static TagArg	cdRomDeviceTags[] =
{
	TAG_ITEM_NAME,				(void *)"CD-ROM",
	TAG_ITEM_PRI,				(void *)50,
	CREATEDEVICE_TAG_INIT,		(void *)(uint32)LCCDDeviceInit,
	CREATEDEVICE_TAG_DRVR,		NULL,
	TAG_END,					NULL
};

static TagArg   cdRomDaemonTags[] =
{
    TAG_ITEM_NAME,				(void *)"LCCD daemon",
    TAG_ITEM_PRI,				(void *)220,
    CREATETASK_TAG_PC,			(void *)(uint32)LCCDDaemonStub,
    CREATETASK_TAG_SP,			(void *)(daemonStack + DAEMONUSERSTACK),
    CREATETASK_TAG_STACKSIZE,	(void *)DAEMONUSERSTACK,
    CREATETASK_TAG_SUPER,		(void *)TRUE,
    TAG_END,					NULL
};

static cdrom	*gLCCDDevices[LCCD_MAX_NUM_DEV];	/* array of LCCD devices (used in FIRQs) */
static uint8	gNumLCCDs;							/* number of LCCD devices present */
static Item		gXBUS;								/* global xbus item */
static uint32	gDaemonSignal;						/* global daemon signal */
static Task		*gDaemon;							/* daemon task pointer */
static List		gLCCDDeviceList;					/* list of LCCD devices */
static uint32	gNumDataBlksInUse;					/* current number of data blocks marked VALID */
static uint32	gNumSnarfBlksInUse;					/* current number of snarfer buffer blocks in use */
static uint32	gPrintTheStuff;						/* kPrintf DBUG() bits */

static SubQInfo	gSavedQCode;						/* used to make sure that we got the QRpt back in ReadSubQ() */

static uint32	gSubcodeSyncMissCount;				/* count of missed subcode sync marks...used to determine if the subcode is no longer synced up */

static uint32	gHighWaterMark;
static uint32	gMaxHWM;

static uint32	gECCTimer;
#ifdef DEVELOPMENT
static uint32	gBaddCode[4];
static uint32	gDeadBabe[4];
static uint8	gECCBuf[2352];
#endif


/*******************************************************************************
* BCD2BIN() and BIN2BCD()                                                      *
*                                                                              *
*      Purpose: Utility routines used to convert a byte from binary to BCD.    *
*                                                                              *
*******************************************************************************/
uint8 BCD2BIN(uint8 bcd)
{
	return (((bcd >> 4) * 10) + (bcd & 0x0F));
}

uint8 BIN2BCD(uint8 bin)
{
	uint8	quo = 0;
	uint8	rem = bin;
	
	while (rem >= 10)
	{
		quo++;
		rem -= 10;
	}

	return ((quo << 4) | rem);
}

/*******************************************************************************
* Offset2BCDMSF()                                                              *
*                                                                              *
*      Purpose: Utility routine to convert a 32-bit binary offset to a 24-bit  *
*               BCD min, sec, frm.                                             *
*                                                                              *
*   Parameters: offset - 32-bit binary offset                                  *
*                                                                              *
*      Returns: uint32 - 0x00MMSSFF in BCD                                     *
*                                                                              *
*******************************************************************************/
uint32 Offset2BCDMSF(int32 offset)
{
	uint8	theMSF[4];
	
	theMSF[0] = 0;
	theMSF[1] = (uint8)((offset / (75L * 60L)) & 0xFF);			/* binary min */
	theMSF[2] = (uint8)(((offset % (75L * 60L)) / 75L) & 0xFF);	/* binary sec */
	theMSF[3] = (uint8)(((offset % (75L * 60L)) % 75L) & 0xFF);	/* binary frm */

	theMSF[1] = BIN2BCD(theMSF[1]);								/* BCD min */
	theMSF[2] = BIN2BCD(theMSF[2]);								/* BCD sec */
	theMSF[3] = BIN2BCD(theMSF[3]);								/* BCD frm */

	return (*(uint32 *)theMSF);
}

#define kBinary	0
#define kBCD	1
#define BCDMSF2Offset(x)	MSF2Offset((x), kBCD)
#define BinMSF2Offset(x)	MSF2Offset((x),	kBinary)

/*******************************************************************************
* MSF2Offset()                                                                 *
*                                                                              *
*      Purpose: Utility routine to convert a 24-bit MSFs to a 32-bit binary    *
*               offsets.  The MSF can be in either BCD or Binary.              *
*                                                                              *
*   Parameters: MSF      - 0x00MMSSFF, minutes, seconds, frames of sector      *
*                          address.                                            *
*               selector - denotes either BCD MSF or Binary MSF                *
*                                                                              *
*      Returns: uint32 - 32-bit binary offset                                  *
*                                                                              *
*******************************************************************************/
uint32 MSF2Offset(uint32 MSF, uint8 selector)
{
	uint32	minutes, seconds, frames;
	
	minutes = (MSF >> 16) & 0xFF;							/* BCD min */
	seconds = (MSF >> 8) & 0xFF;							/* BCD sec */
	frames = MSF & 0xFF;									/* BCD frm */
	
	if (selector == kBCD)
	{
		minutes = BCD2BIN((uint8)minutes);					/* binary min */
		seconds = BCD2BIN((uint8)seconds);					/* binary sec */
		frames = BCD2BIN((uint8)frames);					/* binary frm */
	}
	
	return ((((minutes * 60L) + seconds) * 75L) + frames);
}



/*******************************************************************************
* createLCCDDriver()                                                           *
*                                                                              *
*      Purpose: Creates the LCCD driver                                        *
*                                                                              *
*      Returns: Item number of LCCD driver                                     *
*                                                                              *
*    Called by: operator.c                                                     *
*                                                                              *
*******************************************************************************/
Item createLCCDDriver(void)
{
	return (CreateItem(MKNODEID(KERNELNODE, DRIVERNODE), cdRomDriverTags));
}

/*******************************************************************************
* LCCDDriverInit()                                                             *
*                                                                              *
*      Purpose: Initialization routine for LCCD driver.  Checks for LCCD       *
*               device(s) on xbus -- creating a new device/structure for each. *
*               Then spawns LCCD driver daemon task to handle ioReq processing.*
*                                                                              *
*   Parameters: theDrvr - pointer to LCCD driver, structure supplied by OS.    *
*                                                                              *
*      Returns: Item number of LCCD driver                                     *
*                                                                              *
*    Called by: OS, upon driver item creation -- during createLCCDDriver()     *
*                                                                              *
*******************************************************************************/
int32 LCCDDriverInit(struct Driver *theDrvr)
{
	struct XBusDeviceStatus	xbusStat;
	Item	xbusItem, xbusIORItem, daemonItem;
	IOReq	*xbusIOR;
	int8	unit;
	Err		err;
	uint8	maxUnit;

	/* look for LCCD devices on xbus */
	xbusItem = SuperFindNamedItem(MKNODEID(KERNELNODE, DEVICENODE), "xbus");
	CHK4ERR(xbusItem, ("LCCD: >ERROR<  Could not find xbus device\n"));

	gXBUS = SuperOpenItem(xbusItem, 0);
	CHK4ERR(gXBUS, ("LCCD: >ERROR<  Could not open xbus device\n"));

	xbusIORItem = SuperCreateIOReq("lccdxbus", 0, gXBUS, 0);
	CHK4ERR(xbusIORItem, ("LCCD: >ERROR<  Could not create xbus ioreq\n"));
	
	
	/* initialize ior and its ioinfo */
	xbusIOR = (IOReq *)LookupItem(xbusIORItem);
	
	memset(&xbusIOR->io_Info,0,sizeof(IOInfo));

	/* XBUS converts this CMD_STATUS to a READ_ID */
	xbusIOR->io_Info.ioi_Command = CMD_STATUS;
	xbusIOR->io_Info.ioi_Recv.iob_Buffer = &xbusStat;
	xbusIOR->io_Info.ioi_Recv.iob_Len = sizeof(xbusStat);

	/* initialize device list */
	InitList(&gLCCDDeviceList,"lccd_devices");
	
	/* set CREATEDEVICE_TAG_DRVR arg to driver's item number */
	cdRomDeviceTags[3].ta_Arg = (void *)theDrvr->drv.n_Item;
	
	gNumLCCDs = 0;
	maxUnit = ((Device *)LookupItem(gXBUS))->dev_MaxUnitNum;	/* last valid unit number */
	for (unit = 0; unit <= maxUnit; unit++)
	{
		xbusIOR->io_Info.ioi_Unit = unit;
		err = SuperinternalSendIO(xbusIOR);						/* send STATUS to this unit to get device info */
		if (err >= 0)
		{
			err = SuperWaitIO(xbusIORItem);
			if (err >= 0)
			{
				/* the status command we just sent has returned,
				 * determine if the device is LCCD
				 */
				if (DeviceIsLCCD(&xbusStat))
				{
					err = LCCDNewDevice(unit);
					
					CHK4ERRwithACTION(err, SuperDeleteItem(xbusIORItem), ("LCCD:  >ERROR<  Could not create new LCCD device\n"));

					break;		/* only allow one (first) device (for now) */
				}
			}
		}
	}
	
	/* throw away unnecessary IOReqItem */
	SuperDeleteItem(xbusIORItem);	

	/* where any LCCD devices found? */
	if (!gNumLCCDs)
	{
		DBUG(kPrintGeneralStatusStuff, ("LCCD:  >WARNING<  No LCCD devices found on xbus.\n"));
		SuperCloseItem(gXBUS);
		return (kCDErrNoLCCDDevices);
	}
	
#if DEBUG_ECC
	gPrintTheStuff = kPrintDeath | kPrintWarnings |
		 kPrintQtyNPosOfSectorReqs;
#else
	gPrintTheStuff = kPrintDeath | kPrintWarnings;
#endif
	
	/* for debugging */
	DBUG(kPrintWarnings, ("LCCD:  gECCTimer @ %08lx\n", &gECCTimer));
	DBUG(kPrintWarnings, ("LCCD:  gPTS @ %08lx\n", &gPrintTheStuff));
	DBUG(kPrintWarnings, ("LCCD:  gHWM @ %08lx\n", &gHighWaterMark));
	DBUG(kPrintWarnings, ("LCCD:  Data block headers @ %08lx\n", gLCCDDevices[0]->cd_DataBlkHdr));
	DBUG(kPrintWarnings, ("LCCD:  Snarf buffer       @ %08lx\n", gLCCDDevices[0]->cd_SnarfBuffer));

	/* create LCCD driver daemon */
	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Creating the driver daemon\n"));
	daemonItem = SuperCreateItem(MKNODEID(KERNELNODE, TASKNODE), cdRomDaemonTags);
	CHK4ERRwithACTION(daemonItem, SuperDeleteItem(xbusIORItem), ("LCCD:  >ERROR<  Could not create driver daemon\n"));
	
	return (theDrvr->drv.n_Item);
}

/*******************************************************************************
* DeviceIsLCCD()                                                               *
*                                                                              *
*      Purpose: Determines if the specified device is LCCD-like.               *
*                                                                              *
*      Returns: non-zero if LCCD; zero otherwise.                              *
*                                                                              *
*    Called by: LCCDDriverInit()                                               *
*                                                                              *
*******************************************************************************/
uint32 DeviceIsLCCD(struct XBusDeviceStatus *xbus)
{
	return ((xbus->xbus_ManuIdNum == LCCD_MANU_ID) && (xbus->xbus_ManuDevNum == LCCD_MANU_DEV_NUM));
}

/*******************************************************************************
* LCCDNewDevice()                                                              *
*                                                                              *
*      Purpose: Creates new LCCD device and associated structure.              *
*                                                                              *
*   Parameters: unit - unit number of the xbus device in question              *
*                                                                              *
*      Returns: Err, if one occured                                            *
*                                                                              *
*    Called by: LCCDDriverInit()                                               *
*                                                                              *
*******************************************************************************/
Err LCCDNewDevice(int8 unit)
{
	LCCDDeviceEntry	*devEntry;
	Item			cdRomItem;
	cdrom			*cd;
	Err				err;
	char			*devName = "CD-ROM#";
	
	/* does a LCCD device _already_ exist? */
	if (gNumLCCDs > 0)
	{
		devName[6] = '0' + gNumLCCDs;					/* gNumLCCDs is the number (NOT unit) of the device being created */
		cdRomDeviceTags[0].ta_Arg = devName;
	}

	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Creating LCCD device\n"));
	cdRomItem = SuperCreateSizedItem(MKNODEID(KERNELNODE, DEVICENODE), cdRomDeviceTags, sizeof(cdrom));
	CHK4ERR(cdRomItem, ("LCCD:  >ERROR<  Could not create CD-ROM device\n"));

	cd = (cdrom *)LookupItem(cdRomItem);
	
	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Allocating memory for device and its globals\n"));
	devEntry = (LCCDDeviceEntry *)SUPER_ALLOCMEM(sizeof(LCCDDeviceEntry), MEMTYPE_FILL);
	if (!devEntry)
		return (kCDErrNoMemAvail);
	devEntry->cd_Device = cd;
	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Adding device to device list\n"));
	AddTail(&gLCCDDeviceList, (Node *)devEntry);
	
	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Added new CD-ROM device 0x%lx at 0x%lx\n", cdRomItem, cd));

	cd->cd_unit = unit;

	err = SetupSnarfer(cd);
	CHK4ERR(err, ("LCCD:  >ERROR<  Could not setup snarfer\n"));

	err = PrepareDuckAndCover(cd);
	CHK4ERR(err, ("LCCD:  >ERROR<  Could not prepare duck-n-cover\n"));

	return (kCDNoErr);
}

/*******************************************************************************
* LCCDDeviceInit()                                                             *
*                                                                              *
*      Purpose: Initializes new LCCD device structure.  Creates generic        *
*               ioReq's to use for general xbus/etc. communication.            *
*                                                                              *
*   Parameters: cd - pointer to cdrom device structure in question             *
*                                                                              *
*      Returns: int32 - misc. info (err, item #, etc.)                         *
*                                                                              *
*    Called by: OS, upon device item creation -- during LCCDNewDevice()        *
*                                                                              *
*******************************************************************************/
int32 LCCDDeviceInit(cdrom *cd)
{
	int8	x;
	Item	theItem;
	Err		err;
	
	/* clear all internal state bits */
	cd->cd_State = 0;

	/* initialize all cdrom device fields */
	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Initializing LCCD device\n"));

	/* allocate standard buffer space (subcode space [2K] + 6 data buffers [6 x 2352] = one 16K segment) */
	err = AllocateStandardBuffers(cd);
	CHK4ERR(err, ("LCCD:  >ERROR<  Could not allocate standard buffer space\n"));

	/* allocate extended buffer space (13 data buffers [13 x 2352] = one 32K DRAM page) */
	err = ResizeLinkedBuffers(cd, CDROM_STANDARD_BUFFERS);
	CHK4ERR(err, ("LCCD:  >ERROR<  Could not allocate extended buffer space\n"));

	theItem = SuperCreateIOReq("lccdcmd", 0, gXBUS, 0);
	CHK4ERR(theItem, ("LCCD:  >ERROR<  Could not create cmd ioReq\n"));
	cd->cd_cmdReq = (IOReq *)LookupItem(theItem);
		
	theItem = SuperCreateIOReq("lccdsnarf", 0, gXBUS, 0);
	CHK4ERR(theItem, ("LCCD:  >ERROR<  Could not create snarf ioReq\n"));
	cd->cd_snarfReq = (IOReq *)LookupItem(theItem);
	
	theItem = SuperCreateIOReq("lccdrecover", 0, gXBUS, 0);
	CHK4ERR(theItem, ("LCCD:  >ERROR<  Could not create recover ioReq\n"));
	cd->cd_recoverReq = (IOReq *)LookupItem(theItem);

	InitList(&cd->cd_pendingIORs,"pending_iors");	/* create list for ior's waiting to be processed */
	cd->cd_workingIOR = NULL;						/* not currently processing any IOR */

	gLCCDDevices[gNumLCCDs++] = cd;					/* save off ptr to this unit's device structure */

	gSubcodeSyncMissCount = 0;

	/* initialize snarf buffer & indeces */
	cd->cd_SnarfReadIndex = 0;
	cd->cd_SnarfWriteIndex = 0;
	for (x = 0; x < MAX_NUM_SNARF_BLKS; x++)
		cd->cd_SnarfBuffer[x].blkhdr.state = BUFFER_BLK_FREE;

	cd->cd_DevState = DS_INIT_REPORTS;				/* initial device state */
	
	cd->cd_PrefetchStartOffset = 0;
	cd->cd_PrefetchEndOffset = 0;
	cd->cd_PrefetchCurMSF = 0;
	cd->cd_PrefetchSectorFormat = INVALID_SECTOR;

	cd->cd_DefaultOptions.asFields.densityCode =	CDROM_DEFAULT_DENSITY;
	cd->cd_DefaultOptions.asFields.addressFormat =	CDROM_Address_Blocks;
	cd->cd_DefaultOptions.asFields.speed =			CDROM_DOUBLE_SPEED;
	cd->cd_DefaultOptions.asFields.pitch =			CDROM_PITCH_NORMAL;
	cd->cd_DefaultOptions.asFields.blockLength =	CDROM_M1_D;
	cd->cd_DefaultOptions.asFields.errorRecovery =	CDROM_DEFAULT_RECOVERY;
	
	/* MKE spec. gives 8 as the default.  
	 * The closest we can get given the current cd-rom driver API is 7.
	 * NOTE:  retryShift gets "(1 << retryShift) - 1" applied to it.
	 */
	cd->cd_DefaultOptions.asFields.retryShift =		3;

	DisableLCCDDMA(DMA_CH0 | DMA_CH1);				/* just for kicks... */
	
	cd->cd_Ch0FIRQ = SuperCreateFIRQ("LCCD data channel", CH0_DMA_FIRQ_PRIORITY, ChannelZeroFIRQ, INT_CD0);
	CHK4ERR(cd->cd_Ch0FIRQ, ("LCCD:  >ERROR<  Problem registering FIRQ routine for DMA Channel 0\n"));

	cd->cd_Ch1FIRQ = SuperCreateFIRQ("LCCD subcode channel", CH1_DMA_FIRQ_PRIORITY, ChannelOneFIRQ, INT_CD1);
	CHK4ERR(cd->cd_Ch1FIRQ, ("LCCD:  >ERROR<  Problem registering FIRQ routine for DMA Channel 1\n"));

	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Successfully completed LCCD device initialization\n"));
	return (kCDNoErr);
}

/*******************************************************************************
* LCCDDaemonStub()                                                             *
*                                                                              *
*      Purpose: Stub routine used to create LCCD driver daemon.  This routine  *
*               is necessary in order to execute the daemon in supervisor      *
*               mode.  In addition, it allows us to tell if we exit the daemon *
*               for any reason.                                                *
*                                                                              *
*    Called by: OS, upon daemon task item creation -- during LCCDDriverInit(). *
*                                                                              *
*******************************************************************************/
void LCCDDaemonStub(void)
{
	CallBackSuper(LCCDDriverDaemon, 0, 0, 0);		/* start the daemon */
	
	DeleteItem(CURRENTTASK->t.n_Item);				/* kill ourselves */
}

/*******************************************************************************
* LCCDDriverDaemon()                                                           *
*                                                                              *
*      Purpose: Performs device monitoring tasks and handles ioReq processing. *
*                                                                              *
*      Returns: Any error that occured -- should only happen if daemon is      *
*               killed.                                                        *
*                                                                              *
*    Called by: LCCDDaemonStub()                                               *
*                                                                              *
*******************************************************************************/
Err LCCDDriverDaemon(void)
{
	cdrom	*cd;
	uint32	awakenedMask = 0;
	Err		err;

	gDaemon = CURRENTTASK;
	gDaemonSignal = SuperAllocSignal(0L);
	
	/* grab first (currently, only) device */
	cd = ((LCCDDeviceEntry *)FirstNode(&gLCCDDeviceList))->cd_Device;

	/* initialize this to keep daemon alive throughout device initialization */
	awakenedMask = gDaemonSignal;

	/* gotta make one pass thru the daemon BEFORE going to sleep
	 * in order to turn on reports, read the TOC, etc. for device init
	 */
	SuperInternalSignal(gDaemon, gDaemonSignal);

	while (TRUE)
	{
		DBUG(kPrintSignalAwakeSleep, ("LCCD:  Daemon going to sleep...\n"));
		awakenedMask = SuperWaitSignal(gDaemonSignal);		
		DBUG(kPrintSignalAwakeSleep, ("LCCD:  The Daemon has awaken\n"));
		
		/* somebody killed us...gotta bail */
		if (awakenedMask & SIGF_ABORT)
		{
			/* indicate internally that the daemon has gone away */
			gDaemon = NULL;

			/* abort any currently active or pending client ioReq's */
			AbortCurrentIOReqs(cd);

			/* turn off any DMA */
			DisableLCCDDMA(DMA_CH0 | DMA_CH1);
			
			/* unregister the firq handlers */
			SuperDeleteItem(cd->cd_Ch0FIRQ);
			SuperDeleteItem(cd->cd_Ch1FIRQ);

			/* delete utility ioReqs */
			SuperInternalDeleteItem(cd->cd_cmdReq->io.n_Item);
			SuperInternalDeleteItem(cd->cd_snarfReq->io.n_Item);
			SuperInternalDeleteItem(cd->cd_recoverReq->io.n_Item);

			/* free up Extended and Standard prefetch space(s) */
			SUPER_FREEMEM(cd->cd_ExtBuffers, kExtendedSpaceSize);
			SUPER_FREEMEM(cd->cd_SubcodeBuffer, kStandardSpaceSize);

			DBUG(kPrintDeath, ("LCCD:  >DEATH<  We've been killed!\n"));
			break;
		}
				
		if (awakenedMask & gDaemonSignal)
		{
			/* Currently, this assumes one LCCD device. */
			cd = ((LCCDDeviceEntry *)FirstNode(&gLCCDDeviceList))->cd_Device;	
			
			/* handle all async status coming from LCCD device */
			ProcessSnarfBuffers(cd);

			if (cd->cd_State & CD_SNARF_OVERRUN)
			{
				DBUG(kPrintWarnings, ("LCCD:  >WARNING<  Snarf Buffer Overrun!  (probable loss of status info)\n"));
				err = SetupSnarfer(cd);
				if (err < 0)
				{
					DBUG(kPrintDeath, ("LCCD:  >DEATH<  Error setting-up snarfer\n"));
				}
			}
			
			if ((cd->cd_State & CD_JUST_DIPIRED) &&
				((cd->cd_DevState & kMajorDevStateMask) != DS_CLOSE_DRAWER) &&
				((cd->cd_DevState & kMajorDevStateMask) != DS_RECOVER_FROM_DIPIR))
			{
				/* recover from a dipir of a device other than us */
				DBUG(kPrintDipirStuff, ("LCCD:  dipir of other device detected...saving current state...%02lx\n", cd->cd_DevState));
				cd->cd_SavedDevState = cd->cd_DevState;
				cd->cd_DevState = DS_RECOVER_FROM_DIPIR;
			}
			else if ((cd->cd_State & CD_PREFETCH_OVERRUN) && 
				((cd->cd_DevState & kMajorDevStateMask) != DS_STOP_PREFETCHING))
			{
				/* did the prefetch buffers get filled up? */
				cd->cd_SavedDevState = cd->cd_DevState;
				cd->cd_DevState = DS_STOP_PREFETCHING;
			}

			/* The lack of a 'break's in the cases in this switch statement is
			 * INTENTIONAL.  We want to fall thru and begin the next state
			 * without falling asleep.
			 */
			switch (cd->cd_DevState & kMajorDevStateMask)
			{
				case DS_OPEN_DRAWER:
					DBUG(kPrintDeviceStates, ("LCCD:  DS_OPEN_DRAWER\n"));
					if (!OpenDrawer(cd))
						break;
						
					goto processReqs;
					
				case DS_CLOSE_DRAWER:
					DBUG(kPrintDeviceStates, ("LCCD:  DS_CLOSE_DRAWER\n"));
					err = CloseDrawer(cd);
					if (!err)
						break;

					if (cd->cd_workingIOR)
						if (cd->cd_workingIOR->io_Info.ioi_Command == CDROMCMD_CLOSE_DRAWER)
							CompleteWorkingIOReq(cd->cd_workingIOR, err);

					cd->cd_State &= ~(CD_JUST_DIPIRED | CD_CACHED_INFO_AVAIL);
					cd->cd_DevState = DS_INIT_REPORTS;

				case DS_INIT_REPORTS:
					DBUG(kPrintDeviceStates, ("LCCD:  DS_INIT_REPORTS\n"));
					if (!InitReports(cd))
						break;

					switch (cd->cd_CIPState)
					{
#ifdef DEVELOPMENT
						default:
							DBUG(kPrintDipirStuff, ("LCCD:  >WARNING<  Unexpected CIPState (%02x) after dipir\n", cd->cd_CIPState));
							/* no break here is intentional */
						case CIPPlay:
						case CIPPause:
							/* disc present, device on-line */
							cd->cd_State |= CD_DEVICE_ONLINE;
							break;
#else
						case CIPPlay:
						case CIPPause:
							/* disc present, device on-line */
							cd->cd_State |= CD_DEVICE_ONLINE;
							break;
						default:
							DBUG(kPrintDipirStuff, ("LCCD:  >WARNING<  Unexpected CIPState (%02x) after dipir\n", cd->cd_CIPState));
							/* no break here is intentional */
#endif
						case CIPOpen:
						case CIPOpening:
						case CIPStuck:
						case CIPFocusError:
							cd->cd_State |= CD_NO_DISC_PRESENT;
						case CIPUnreadable:
						case CIPSeekFailure:
							cd->cd_State |= CD_UNREADABLE;

							/* clear this bit if set by no disc being present */
							cd->cd_State &= ~CD_DEVICE_ERROR;

							PrepareCachedInfo(cd);
							AbortCurrentIOReqs(cd);
							goto processReqs;
					}
					cd->cd_DevState = DS_BUILDING_DISCDATA;
					
				case DS_BUILDING_DISCDATA:
					DBUG(kPrintDeviceStates, ("LCCD:  DS_BUILDING_DISCDATA\n"));
					err = BuildDiscData(cd);
					if (!err)
						break;
					if (err < 0)
					{
						cd->cd_State |= CD_UNREADABLE;
						PrepareCachedInfo(cd);
						AbortCurrentIOReqs(cd);
					}
processReqs:
					cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;
					
				case DS_PROCESS_CLIENT_IOREQS:
					DBUG(kPrintDeviceStates, ("LCCD:  DS_PROCESS_CLIENT_IOREQS\n"));
					ProcessClientIOReqs(cd);
					break;
				
				case DS_RECOVER_FROM_DIPIR:
					DBUG(kPrintDeviceStates, ("LCCD:  DS_RECOVER_FROM_DIPIR\n"));
					if (RecoverFromDipir(cd))
					{
						/* go to the start of current dev (major) state (this
						 * also RESTARTS any current working ioReq!)
						 */
						cd->cd_DevState = (cd->cd_SavedDevState & kMajorDevStateMask);
						DBUG(kPrintDipirStuff, ("LCCD:  new dev state...%02lx\n", cd->cd_DevState));
					}
					break;
					
				case DS_STOP_PREFETCHING:
					DBUG(kPrintDeviceStates, ("LCCD:  DS_STOP_PREFETCHING\n"));
					if (StopPrefetching(cd))	/* how to handle errors here? */
						cd->cd_DevState = cd->cd_SavedDevState;		/* restore saved state */
					break;
			}
		}
	}
	
	return (kCDErrDaemonKilled);				/* should never get here! */
}

/*******************************************************************************
* LCCDDriverDispatch()                                                         *
*                                                                              *
*      Purpose: Sticks the ioReq onto the pendingIORs queue and then signals   *
*               the daemon to process the ioReq.  Attempts to process          *
*               CMD_STATUS and CDROMCMD_DISCDATA calls immediately, if         *
*               possible.                                                      *
*                                                                              *
*   Parameters: theReq - pointer to incoming ioRequest                         *
*                                                                              *
*      Returns: Err - Error, if one occured                                    *
*                                                                              *
*    Called by: OS, upon receiving a SendIO() to this driver                   *
*                                                                              *
*******************************************************************************/
Err LCCDDriverDispatch(IOReq *theReq)
{
	cdrom	*cd = (cdrom *)theReq->io_Dev;
	uint32	interrupts;
	Err		cmdDone = FALSE;
	
	DBUG(kPrintSendCompleteIO, ("LCCD:**Received SendIO(%02x)\n", theReq->io_Info.ioi_Command));

	switch (theReq->io_Info.ioi_Command)
	{
		case CDROMCMD_RESIZE_BUFFERS:
			/* only allow RESIZE_BUFFERS for privileged tasks */
			if (CURRENTTASK->t.n_Flags & TASK_SUPER)
				goto queueItUp;

			theReq->io_Error = kCDErrNotPrivileged;
			return (theReq->io_Error);
		case CMD_STATUS:
		case CDROMCMD_DISCDATA:
			/* CMD_STATUS, _DISCDATA can be called anytime after we have at
			 * least _attempted_ to read the TOC.
			 */
			cmdDone = (theReq->io_Info.ioi_Command == CMD_STATUS) ? CmdStatus(cd, theReq) : DiscData(cd, theReq);
			if (cmdDone)
			{
				CompleteWorkingIOReq(theReq, kCDNoErr);
				break;
			}
			/* no break is intentional here */
		case CMD_READ:
		case CDROMCMD_READ_SUBQ:
		case CDROMCMD_OPEN_DRAWER:
		case CDROMCMD_CLOSE_DRAWER:
		case CDROMCMD_SETDEFAULTS:
queueItUp:
			theReq->io_Flags &= ~IO_QUICK;

			/* insert ioReq in pending queue (using InsertNodeFromTail() places
			 * it in priority order)
			 */
			interrupts = Disable();
			InsertNodeFromTail(&cd->cd_pendingIORs, (Node *)theReq);
			Enable(interrupts);
			
			/* signal daemon to process ioRequest */
			if (gDaemon)
			{
				DBUG(kPrintSignalAwakeSleep, ("LCCD:  Signalling daemon\n"));
				SuperInternalSignal(gDaemon, gDaemonSignal);
			}
			break;
#ifdef DEVELOPMENT
		case CDROMCMD_DIAG_INFO:
			if (theReq->io_Info.ioi_Recv.iob_Buffer)
			{
				memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, cd, sizeof(cdrom));
				theReq->io_Actual = sizeof(cdrom);
				SuperCompleteIO(theReq);
				break;
			}
			else
			{
				uint8	*diagCmd = (uint8 *)theReq->io_Info.ioi_Send.iob_Buffer;
				
				switch (diagCmd[0])
				{
					case 'p':
						gPrintTheStuff ^= theReq->io_Info.ioi_CmdOptions;
						DBUGERR(("LCCD:  gPTS now set to %08lx\n", gPrintTheStuff));
						break;
					case 'h':
						gMaxHWM = theReq->io_Info.ioi_CmdOptions;
						DBUGERR(("LCCD:  gMaxHWM now set to %ld\n", gMaxHWM));
						break;
				}
				theReq->io_Actual = 0;
				SuperCompleteIO(theReq);
				break;
			}
#endif
		case CDROMCMD_READ_DISC_CODE:
		case CDROMCMD_PASSTHROUGH:
		default:
			theReq->io_Error = kCDErrBadCommand;
			return (theReq->io_Error);
	}

	return (kCDNoErr);
}

/*******************************************************************************
* LCCDDriverAbortIO()                                                          *
*                                                                              *
*      Purpose: Aborts an ioReq.  If it's currently processing this ioReq, it  *
*               bails.  If this ioReq is still pending, it marks it as         *
*               'aborted' and completes it.                                    *
*                                                                              *
*   Parameters: theReq - the ioReq to be aborted                               *
*                                                                              *
*    Called by: OS, during an AbortIO()                                        *
*                                                                              *
*******************************************************************************/
void LCCDDriverAbortIO(IOReq *theReq)
{
	cdrom	*cd = (cdrom *)theReq->io_Dev;
	
	theReq->io_Error = kCDErrAborted;

	/* if it's not currently active, AND it's one of ours */
	if ((theReq != cd->cd_workingIOR) && 
		IsNode(&cd->cd_pendingIORs, (Node *)theReq))
	{
		RemNode((Node *)theReq);		/* just remove it from cd_pendingIORs */
		SuperCompleteIO(theReq);		/* ...and complete the ioReq */
	}
}



/*******************************************************************************
* AllocateStandardBuffers()                                                    *
*                                                                              *
*      Purpose: Allocates the 16K which is required for minimum operation.     *
*                                                                              *
*   Parameters: cd - ptr to cdrom struct for this device                       *
*                                                                              *
*      Returns: Err - if no memory was available.                              *
*                                                                              *
*    Called by: LCCDDeviceInit()                                               *
*                                                                              *
*******************************************************************************/
Err	AllocateStandardBuffers(cdrom *cd)
{
	uint8	x;
	uint8	*dataPtr;
	uint8	*StdBufSpace;
	
	/* initialize these to "minimum" configuration */
	cd->cd_NumDataBlks = MIN_NUM_DATA_BLKS;
	cd->cd_NumSubcodeBlks = MIN_NUM_SUBCODE_BLKS;

	StdBufSpace = (uint8 *)SUPER_ALLOCMEM(kStandardSpaceSize, MEMTYPE_DRAM);
	if (!StdBufSpace)
		return (kCDErrNoMemAvail);
		
	cd->cd_SubcodeBuffer = (subcodeBlock *)StdBufSpace;
	
	/* init assuming that all subcode blocks are available/used */
	for (x = 0, dataPtr = StdBufSpace; x < MAX_NUM_SUBCODE_BLKS; x++, dataPtr += kSubcodeBlkSize)
		cd->cd_SubcodeBlkHdr[x].buffer = dataPtr;

	/* initialize data buffer & indeces */
	for (x = 0, dataPtr = (StdBufSpace + kSubcodeBufSizePlusSome); x < MIN_NUM_DATA_BLKS; x++, dataPtr += kDataBlkSize)
		cd->cd_DataBlkHdr[x].buffer = dataPtr;

	return (kCDNoErr);
}	

/*******************************************************************************
* ResizeLinkedBuffers()                                                        *
*                                                                              *
*      Purpose: Resizes the internal prefetch buffer space between Standard    *
*               (MAX) mode & Minimum (MIN) mode.  Standard mode provides uses  *
*               48K of memory for the combined (data + subcode) prefetch       *
*               space; whereas Minimum mode uses only 16K for both (limiting   *
*               the data to 6 sectors of prefetch).  This allows the driver to *
*               free-up a 32K page to the application, if requested via        *
*               CDROMCMD_RESIZE_BUFFERS.  Currently, this is only allowed if   *
*               the caller is a privileged task (ie, the system).              *
*                                                                              *
*   Parameters: cd   - ptr to cdrom device struct                              *
*               size - the size/mode to switch to (0 = default, 1 = min,       *
*                      2 = std)                                                *
*                                                                              *
*      Returns: Err, if one occured                                            *
*                                                                              *
*    Called by: LCCDDeviceInit() and ResizeBufferSpace()                       *
*                                                                              *
*******************************************************************************/
Err ResizeLinkedBuffers(cdrom *cd, uint32 size)
{
	int8	x;
	uint8	*dataPtr;
	uint8	*ExtBufSpace;
	int32	scavengedBytes;
		
	switch (size)
	{
		case CDROM_MINIMUM_BUFFERS:
			if (cd->cd_NumDataBlks == MIN_NUM_DATA_BLKS)
			{
				/* we're already in this mode 
				 * (do we really want to return an error in this case?)
				 */
/*
				return (kCDErrBadIOArg);
*/
				break;
			}
	
			cd->cd_NumDataBlks = MIN_NUM_DATA_BLKS;
			cd->cd_NumSubcodeBlks = MIN_NUM_SUBCODE_BLKS;
			cd->cd_State |= CD_USING_MIN_BUF_SPACE;
			
			/* return one DRAM page */
			SUPER_FREEMEM(cd->cd_ExtBuffers, kExtendedSpaceSize);
			
			scavengedBytes = ScavengeMem();
			break;
		case CDROM_STANDARD_BUFFERS:
		case CDROM_DEFAULT_BUFFERS:			 /* the default is to use the max */
			if (cd->cd_NumDataBlks == MAX_NUM_DATA_BLKS)
			{
				/* we're already in this mode 
				 * (do we really want to return an error in this case?)
				 */
/*
				return (kCDErrBadIOArg);
*/
				break;
			}

			cd->cd_NumDataBlks = MAX_NUM_DATA_BLKS;
			cd->cd_NumSubcodeBlks = MAX_NUM_SUBCODE_BLKS;
			cd->cd_State &= ~CD_USING_MIN_BUF_SPACE;
			
			ExtBufSpace = (uint8 *)SUPER_ALLOCMEM(kExtendedSpaceSize, MEMTYPE_STARTPAGE | MEMTYPE_DRAM);
			if (!ExtBufSpace)
				return (kCDErrNoMemAvail);

			cd->cd_ExtBuffers = ExtBufSpace;
			for (x = MIN_NUM_DATA_BLKS, dataPtr = ExtBufSpace; x < MAX_NUM_DATA_BLKS; x++, dataPtr += kDataBlkSize)
				cd->cd_DataBlkHdr[x].buffer = dataPtr;
				
			break;
		default:
			return (kCDErrBadArg);
	}
	
	/* update max HighWaterMark for new buffer space */
	gMaxHWM = (uint32)cd->cd_NumDataBlks - kHighWaterBufferZone;
	
	InitDataSpace(cd);
	InitSubcodeSpace(cd);

	return (kCmdComplete);
}



/*******************************************************************************
* SendCmdWithWaitState()                                                       *
*                                                                              *
*      Purpose: Provides the mini state machine needed to walk commands thru   *
*               the send-command, wait-for-command-tag, wait-for-state process.*
*                                                                              *
*   Parameters: cd         - ptr to the cdrom device in question               *
*               cmd        - address of byte string containing command to send *
*               cmdLen     - length of command to send                         *
*               desiredCIP - the state to wait for (CIPNone if you only want   *
*                            to wait for the command tag)                      *
*               errCode    - address to hold any errored CIP state in case of  *
*                            dev error                                         *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: StopPrefetching(), RecoverFromDipir(), OpenDrawer(),           *
*               CloseDrawer(), InitReports(), BuildDiscData(), Read(),         *
*               ReadSubQ()                                                     *
*                                                                              *
*******************************************************************************/
Err SendCmdWithWaitState(cdrom *cd, uint8 *cmd, uint8 cmdLen, uint8 desiredCIP, uint8 *errCode)
{
	switch (cd->cd_DevState & kCmdStateMask)
	{
		case kSendCmd:
			SendCmdToDev(cd, cmd, cmdLen);
			cd->cd_DevState = (cd->cd_DevState & ~kCmdStateMask) | kWait4Tag;
		case kWait4Tag:
			/* wait for our cmd tag */
			if (cd->cd_CmdByteReceived != cmd[0])
				break;
			cd->cd_DevState = (cd->cd_DevState & ~kCmdStateMask) | kWait4CIPState;
		case kWait4CIPState:
			/* look for possible error */
			if (cd->cd_State & CD_DEVICE_ERROR)
			{
				cd->cd_State &= ~CD_DEVICE_ERROR;
				if (errCode)
					*errCode = cd->cd_CIPState;
				return (kCDErrDeviceError);
			}
			else if ((desiredCIP == CIPNone) || (cd->cd_CIPState == desiredCIP))
				return (kCmdComplete);
			break;
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* SendCmdToDev()                                                               *
*                                                                              *
*      Purpose: Sends the requested command to the LCCD device via xbus.       *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               cmdBuf - pointer to command byte string to send across xbus    *
*               cmdLen - length of command byte string                         *
*                                                                              *
*      Returns: Err - Any error received from SendIO()                         *
*                                                                              *
*    Called by: Various internal action machines upon needing to send a cmd to *
*               the LCCD device.                                               *
*                                                                              *
*******************************************************************************/
Err SendCmdToDev(cdrom *cd, uint8 *cmdBuf, uint8 cmdLen)
{
	Err		err;
#ifdef DEVELOPMENT
	uint8	x;
	uint8	hackblk;
#endif

	/* clear previous cmd byte in case we wish to send 2 like cmds in a row */
	cd->cd_CmdByteReceived = 0;

	/* store cmd byte string in static mem */
	memset(cd->cd_cmdBytes, 0, sizeof(cd->cd_cmdBytes));
	memcpy(cd->cd_cmdBytes, cmdBuf, cmdLen);

	/* setup cmd_cmd ior */
	memset(&cd->cd_cmdReq->io_Info,0,sizeof(IOInfo));
	cd->cd_cmdReq->io_Info.ioi_Command = XBUSCMD_Command;
	cd->cd_cmdReq->io_Info.ioi_Unit = cd->cd_unit;					/* restore unit #, since it was just cleared */
	cd->cd_cmdReq->io_Info.ioi_User = (uint32)cd;					/* save reference to device */
	cd->cd_cmdReq->io_Info.ioi_Send.iob_Buffer = cd->cd_cmdBytes;	/* send cmd string */
	cd->cd_cmdReq->io_Info.ioi_Send.iob_Len = cmdLen;
	cd->cd_cmdReq->io_Info.ioi_Recv.iob_Buffer = NULL;				/* null ptr/len causes async xbus to complete this ioReq immediately */
	cd->cd_cmdReq->io_Info.ioi_Recv.iob_Len = 0;					/* tag byte (only) returned */
	cd->cd_cmdReq->io_CallBack = CmdCallBack;						/* set to callback routine stub */

#ifdef DEVELOPMENT
	DBUG(kPrintSendCmdToDevStuff, ("LCCD:  SendCmd..."));
	for (x = 0; x < cmdLen; x++)
	{
		DBUG(kPrintSendCmdToDevStuff, ("%02x ", cmdBuf[x]));
	}
	DBUG(kPrintSendCmdToDevStuff, ("\n"));

	hackblk = cd->cd_SnarfWriteIndex;
	if (cd->cd_SnarfBuffer[hackblk].dbgdata[0] | cd->cd_SnarfBuffer[hackblk].dbgdata[1] | 
		cd->cd_SnarfBuffer[hackblk].dbgdata[2] | cd->cd_SnarfBuffer[hackblk].dbgdata[3])
	{
		gBaddCode[0] = cd->cd_SnarfBuffer[hackblk].dbgdata[0];
		gBaddCode[1] = cd->cd_SnarfBuffer[hackblk].dbgdata[1];
		gBaddCode[2] = cd->cd_SnarfBuffer[hackblk].dbgdata[2];
		gBaddCode[3] = cd->cd_SnarfBuffer[hackblk].dbgdata[3];
		
		gDeadBabe[0] = *(uint32 *)cd->cd_cmdBytes;	
		gDeadBabe[1] = cd->cd_DevState;
		gDeadBabe[2] = cd->cd_State;
		gDeadBabe[3] = cd->cd_CIPState;
	
		cd->cd_SnarfBuffer[hackblk].dbgdata[0] = 0xFFFFFFFF;	
		cd->cd_SnarfBuffer[hackblk].dbgdata[1] = 0xBADDC0DE;
		cd->cd_SnarfBuffer[hackblk].dbgdata[2] = 0xDEADBABE;
		cd->cd_SnarfBuffer[hackblk].dbgdata[3] = 0xFFFFFFFF;

		DBUG(kPrintWarnings, ("LCCD:  >WARNING<  Sent back-to-back commands to drive without waiting on cmd tag!\n"));
		DBUG(kPrintWarnings, ("LCCD:  cmd1:  %08lx %08lx %08lx %08lx\n", gBaddCode[0], gBaddCode[1], gBaddCode[2], gBaddCode[3]));
		DBUG(kPrintWarnings, ("LCCD:  cmd1:  %08lx %08lx %08lx %08lx\n", gDeadBabe[0], gDeadBabe[1], gDeadBabe[2], gDeadBabe[3]));
	}
	else
	{
		cd->cd_SnarfBuffer[hackblk].dbgdata[0] = *(uint32 *)cd->cd_cmdBytes;	
		cd->cd_SnarfBuffer[hackblk].dbgdata[1] = cd->cd_DevState;
		cd->cd_SnarfBuffer[hackblk].dbgdata[2] = cd->cd_State;
		cd->cd_SnarfBuffer[hackblk].dbgdata[3] = cd->cd_CIPState;
	}
#endif

	err = SuperInternalSendIO(cd->cd_cmdReq);		/* send cmd to drive */

	CHK4ERR(err, ("LCCD:  >ERROR<  Problem sending command ioReq to xbus\n"));

	return (kCDNoErr);
}

/*******************************************************************************
* CmdCallBack()                                                                *
*                                                                              *
*      Purpose: Does absolutely nothing.  It's only here to satisfy xbus       *
*               protocol.  This comment is even longer than the routine.       *
*                                                                              *
*******************************************************************************/
IOReq *CmdCallBack(IOReq *theReq)
{
	return (0);
}



/*******************************************************************************
* SetupSnarfer()                                                               *
*                                                                              *
*      Purpose: Sends a "snarfer" to xbus to retreive any status being         *
*               returned from the LCCD device.  This mechanism is required in  *
*               order to support async. communication with the device          *
*               (enabling the use of Reports, etc.).  This is primarily req.   *
*               due to the architecture of the firmware.                       *
*                                                                              *
*               The receive buffer for the snarfReq is the next FREE 'snarf    *
*               buffer block', indicated by the SnarfWriteIndex.               *
*                                                                              *
*         NOTE: ALL status (from cmds, reports, etc.) must be returned via a   *
*               snarfReq due to the fact that sync/async device communication  *
*               cannot be mixed.                                               *
*                                                                              *
*   Parameters: cd - pointer to cdrom device to snarf status from              *
*                                                                              *
*      Returns: Err - if one occurs when sending the snarfer to xbus           *
*                                                                              *
*    Called by: LCCDNewDevice(), during device initialization                  *
*                                                                              *
*******************************************************************************/
Err SetupSnarfer(cdrom *cd)
{
	uint8	blk = cd->cd_SnarfWriteIndex;
	Err		err;
	
	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Setting up snarfer for async. status responses\n"));
	
	if (cd->cd_SnarfBuffer[blk].blkhdr.state == BUFFER_BLK_FREE)
	{
		cd->cd_State &= ~CD_SNARF_OVERRUN;
	
		memset(&cd->cd_snarfReq->io_Info, 0, sizeof(IOInfo));

		/* formerly known as 'XBUSCMD_SnarfStatusBytes' this sucks all status
		 * out of the fifo
		 */
		cd->cd_snarfReq->io_CallBack = SnarfCallBack;
		cd->cd_snarfReq->io_Info.ioi_Command = XBUSCMD_CommandSyncStat;
		cd->cd_snarfReq->io_Info.ioi_Unit = cd->cd_unit;
		cd->cd_snarfReq->io_Info.ioi_User = (uint32)cd;
		cd->cd_snarfReq->io_Info.ioi_Recv.iob_Buffer = cd->cd_SnarfBuffer[blk].blkdata;
		cd->cd_snarfReq->io_Info.ioi_Recv.iob_Len = kLCCDMaxStatusLength;
		
		/* mark block as in use */
		cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_INUSE;
		
		err = SuperInternalSendIO(cd->cd_snarfReq);
		CHK4ERR(err, ("LCCD:  >ERROR<  Problem setting up snarfer ioReq\n"));
	}
	else
	{
		DBUG(kPrintDeath, ("LCCD:  >DEATH<  No snarf buffer available...couldn't send snarfer\n"));
		
		return (kCDErrSnarfBufferOverrun);
	}
	
	return (kCDNoErr);
}

/*******************************************************************************
* SnarfCallBack()                                                              *
*                                                                              *
*      Purpose: Intercepts incoming snarfReq's upon an interrupt to xbus,      *
*               preserves the response in the snarf status buffer, redirects   *
*               the snarfReq to the next available snarf buffer block, and     *
*               signals the daemon to process the buf.                         *
*                                                                              *
*   Parameters: theReq - pointer to the snarfReq containing status             *
*                                                                              *
*      Returns: IOReq - if the next snarf buffer block is FREE, the snarfReq's *
*                       io_Recv buffer is redirected to this bufblk, and the   *
*                       snarfReq is returned to the caller.  This resends the  *
*                       snarfReq to xbus.                                      *
*                                                                              *
*    Called by: OS, upon receiving a CompleteIO() for a snarfReq.              *
*                                                                              *
*******************************************************************************/
IOReq *SnarfCallBack(IOReq *theReq)
{
	cdrom	*cd = (cdrom *)theReq->io_Info.ioi_User;	/* what device? */
	uint8	blk = cd->cd_SnarfWriteIndex;				/* which buffer are we currently snarfing into? */

	/* update current snarfBuffer block header, mark as complete, etc. */
	cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_VALID;
	cd->cd_SnarfBuffer[blk].blkhdr.size = (uint8)theReq->io_Actual;

#ifdef DEVELOPMENT
	{
		uint8	x;
		for (x = cd->cd_SnarfBuffer[blk].blkhdr.size; x < 12; x++)
			cd->cd_SnarfBuffer[blk].blkdata[x] = 0x00;
			
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkhdr &= 0xFFFF0000;
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkhdr |= (0x0000FFFF & cd->cd_DevState);
	}
#endif
	
	/* point to 'next' snarf buffer block */
	blk = cd->cd_SnarfWriteIndex = (blk == (MAX_NUM_SNARF_BLKS-1)) ? 0 : (blk + 1);
	
	/* is the next buffer block free? */
	if (cd->cd_SnarfBuffer[blk].blkhdr.state == BUFFER_BLK_FREE)
	{
		/* if so, mark it as now being in-use; and re-direct snarfReq's buf */
		cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_INUSE;
		theReq->io_Info.ioi_Recv.iob_Buffer = cd->cd_SnarfBuffer[blk].blkdata;
		theReq->io_Info.ioi_Recv.iob_Len = 32;	/* insure we get max amt back */
		
#ifdef DEVELOPMENT
		/* write over old data (provides a "sync mark" to indicate where 
		 * we're currently writing to)
		 */
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkhdr |= 0x00FFFFFF;
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkdata[0] = 0xFFFFFFFF;
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkdata[4] = 0xFFFFFFFF;
		*(uint32 *)&cd->cd_SnarfBuffer[blk].blkdata[8] = 0xFFFFFFFF;
		cd->cd_SnarfBuffer[blk].dbgdata[0] = 0x00000000;
		cd->cd_SnarfBuffer[blk].dbgdata[1] = 0x00000000;
		cd->cd_SnarfBuffer[blk].dbgdata[2] = 0x00000000;
		cd->cd_SnarfBuffer[blk].dbgdata[3] = 0x00000000;
#endif

		/* update number of snarfers in use (and max needed) */
		gNumSnarfBlksInUse++;
	}
	else
	{
		theReq = 0;			/* make sure we return a NULL ioReq* in this case */
		
		cd->cd_State |= CD_SNARF_OVERRUN;				/* uh oh!...crap out! */
	}

	/* notify daemon of newly received response */
	if (gDaemon)
		SuperInternalSignal(gDaemon, gDaemonSignal);
	
	/* don't return (ie, resend) the ioReq if it's being aborted
	 * by the daemon (this only occurs if the daemon's been killed)
	 */
	if (theReq->io_Error == ABORTED)
		return (NULL);
	
	/* cause kernel to resend this req to xbus */
	return (theReq);
}

/*******************************************************************************
* ProcessSnarfBuffers()                                                        *
*                                                                              *
*      Purpose: Parses the status returned in the snarf buffer blocks, dealing *
*               with each status appropriately, and then makes the snarf       *
*               buffer block available for re-use by a snarfReq.               *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon being signaled by SnarfCallBack().    *
*                                                                              *
*******************************************************************************/
void ProcessSnarfBuffers(cdrom *cd)
{
	uint8	blk = cd->cd_SnarfReadIndex;
	uint8	x, len;
	uint8	*blockData;
	
	/* process all valid snarfed blocks */
	while (cd->cd_SnarfBuffer[blk].blkhdr.state == BUFFER_BLK_VALID)
	{
		/* how much data was returned in this block?  and were is it located? */
		len = cd->cd_SnarfBuffer[blk].blkhdr.size;
		blockData = cd->cd_SnarfBuffer[blk].blkdata;
		
		for (x = 0; x < len; )
		{
			switch (blockData[x])
			{
				case CD_LED:
				case CD_SETSPEED:
				case CD_SPINDOWNTIME:
				case CD_SECTORFORMAT:
				case CD_CIPREPORTEN:
				case CD_QREPORTEN:
				case CD_SWREPORTEN:
				case CD_SENDBYTES:
				case CD_PPSO:
				case CD_SEEK:
					DBUG(kPrintStatusResponse, ("LCCD:  %02x\n", blockData[x]));
					
					/* save last received cmd tag byte */
					cd->cd_CmdByteReceived = blockData[x];

					x += 1;					/* only cmd tag byte is returned */
					break;
				case CD_MECHTYPE:
					DBUG(kPrintStatusResponse, ("LCCD:  %02x\n", blockData[x]));
					
					/* save last received cmd tag byte */
					cd->cd_CmdByteReceived = blockData[x];
					
					/* is it a drawer or clamshell mechanism? */
					if (blockData[x+1] == kDrawerMechanism)
						cd->cd_State |= CD_DRAWER_MECHANISM;

					DBUG(kPrintClamshell, ("LCCD:  MechType=%02x\n", blockData[x+1]));
					x += 12;				/* MechType cmd returns 12 bytes */
					break;
				case CD_CIPREPORT:
					DBUG(kPrintStatusResponse, ("LCCD:  %02x %02x\n", blockData[x], blockData[x+1]));
					
					/* save latest known device CIP state */
					cd->cd_CIPState = blockData[x+1];
					
					if ((cd->cd_CIPState == CIPStop) || (cd->cd_CIPState == CIPStopAndFocused))
					{
						/* clear this bit in case of auto spin-down */
						cd->cd_StatusByte &= ~SB_SPINUP;
						DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));
					}						
					
					switch (cd->cd_CIPState)
					{
						case CIPStuck:
							if (!(cd->cd_State & CD_DRAWER_MECHANISM))
							{
								DBUG2(kPrintWarnings, ("LCCD:  Someone tried to swap discs in the clamshell drive!\n"));
								cd->cd_StatusByte &= ~(SB_DISCIN | SB_SPINUP | SB_DBLSPD | SB_READY);
								TakeDeviceOffline(cd);
								AbortCurrentIOReqs(cd);
							}
						case CIPFocusError:
						case CIPUnreadable:
						case CIPSeekFailure:
							cd->cd_State |= CD_DEVICE_ERROR;
							DBUG2(kPrintDeath, ("LCCD:  >DEATH<  Got error CIP state (%02x)\n", cd->cd_CIPState));
							break;
					}
	
					x += 2;				/* CIPReport response is 2 bytes long */
					break;
				case CD_QREPORT:
					/* cmd tag for MKE readsubq (required for MKE emulation) */
					cd->cd_LastQCode0.cmdTag = 0x87;

					/* indicate valid Qcode being returned (required for MKE emulation) */
					cd->cd_LastQCode0.validByte = 0x80;

					/* cached latest Qcode */
					memcpy(&cd->cd_LastQCode0.addressAndControl, &blockData[x+1], sizeof(SubQInfo)-2);
					
					/* if we're still reading the TOC... */
					if (cd->cd_State & CD_READING_TOC_INFO)
						ParseTOCEntries(cd, blockData, x);

					x += 11;			/* QReport response is 11 bytes long */
					break;
				case CD_SWREPORT:
					DBUG(kPrintStatusResponse, ("LCCD:  %02x %02x %02x\n", blockData[x], blockData[x+1], blockData[x+2]));
					
					HandleSwReport(cd, blockData, x);
	
					x += 3;				/* SwReport response is 3 bytes long */
					break;
				default:
					DBUG(kPrintWarnings, ("LCCD:  >WARNING<  Unknown status response received!\n"));
					DBUG(kPrintWarnings, ("LCCD:  "));
					for (; x < len; x++)
					{
						DBUG(kPrintWarnings, ("%02x ",blockData[x]));
					}
					DBUG(kPrintWarnings, ("\n"));
					break;
			}
		}

		/* mark block as now available for use */
		cd->cd_SnarfBuffer[blk].blkhdr.state = BUFFER_BLK_FREE;

		/* update number of snarfers in use */
		gNumSnarfBlksInUse--;	

		/* point to 'next' snarf buffer block */
		blk = cd->cd_SnarfReadIndex = (blk == (MAX_NUM_SNARF_BLKS-1)) ? 0 : (blk + 1);
	}
}

/*******************************************************************************
* HandleSwReport()                                                             *
*                                                                              *
*      Purpose: Handles SwReports.  Saves the latest (and previous) switch     *
*               states in device globals.  Determines if user switch was       *
*               pressed or if someone actually pressed on the drawer itself,   *
*               and initiates appropriate action to open/close the drawer.     *
*               Also updates status byte.  Upon receiving a close switch       *
*               report, we set CD_DIPIR_PENDING.                               *
*                                                                              *
*   Parameters: cd          - pointer to the cdrom device in question          *
*               statusBytes - pointer to incoming status byte buffer           *
*               x           - index into buffer where SwReport status begins   *
*                                                                              *
*    Called by: ProcessSnarfBuffers(), upon parsing a SwReport.                *
*                                                                              *
*******************************************************************************/
void HandleSwReport(cdrom *cd, uint8 *statusBytes, int32 x)
{
	uint8	curState = statusBytes[x+1];
	uint8	delta = statusBytes[x+2];
	
	DBUG(kPrintStatusResponse, ("LCCD:  CurState: %02x\n", curState));
	DBUG(kPrintStatusResponse, ("LCCD:    Change: %02x\n", delta));

	if (curState & CD_DOOR_CLOSED_SWITCH)
	{
		cd->cd_StatusByte |= SB_DOOR;				/* door is now closed */
		DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));
		
		if (delta & CD_DOOR_CLOSED_SWITCH)
		{
			/* a dipir is pending on our device (we must induce it) */
			cd->cd_State |= CD_DIPIR_PENDING;
			
			/* support needed for clamshell drives, and for handling the
			 * case of being in the CIPClosing state when leaving dipir.
			 */
			if ((cd->cd_DevState & kMajorDevStateMask) != DS_CLOSE_DRAWER)
			{
				/* go directly to the DIPIR (inducing) state of the 
				 * CloseDrawer machine
				 */
				DBUG(kPrintClamshell, ("LCCD:  Clamshell closed.\n"));
				cd->cd_DevState = CL_DIPIR;
			}
		}

		if (delta & CD_DOOR_OPEN_SWITCH)
		{
			DBUG(kPrintWarnings, ("LCCD:  >WARNING<  Got spurious OPEN SwRpt\n"));
			return;
		}
	}
	else
	{
		cd->cd_StatusByte &= ~(SB_DOOR | SB_DISCIN | SB_SPINUP | SB_DBLSPD | SB_READY);
		DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));
		
		/* support needed for clamshell drives */
		if ((delta & CD_DOOR_CLOSED_SWITCH) && !(cd->cd_State & CD_DRAWER_MECHANISM))
		{
			DBUG(kPrintClamshell, ("LCCD:  Clamshell opened!\n"));
			TakeDeviceOffline(cd);
			AbortCurrentIOReqs(cd);
			cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;
		}
	}
	
	cd->cd_State &= ~(CD_DOOR_OPEN_SWITCH | CD_DOOR_CLOSED_SWITCH | CD_USER_SWITCH);
	cd->cd_State |= (uint32)curState;
	
	/* was the user switch pressed? */
	if ((curState & CD_USER_SWITCH) && (delta & CD_USER_SWITCH))
	{
		/* indicate that we're Drawer (ie, not Clamshell) */
		cd->cd_State |= CD_DRAWER_MECHANISM;

		/* if (open) or (attempting to open, AND the CloseSwitch has
		 * disengaged)
		 */
		if (((curState & CD_DOOR_OPEN_SWITCH) || 
			((cd->cd_DevState & kMajorDevStateMask) == DS_OPEN_DRAWER)) &&
			((cd->cd_CIPState != CIPStopping) && (cd->cd_CIPState != 
			CIPStopAndFocused) && (cd->cd_CIPState != CIPStop)))
		{
			/* place device machine in CloseDrawer state */
			cd->cd_DevState = DS_CLOSE_DRAWER;
		}			
		/* else...it's either closed, attempting to close, or stuck */
		else
		{
			TakeDeviceOffline(cd);
			AbortCurrentIOReqs(cd);
			cd->cd_DevState = DS_OPEN_DRAWER;	/* go to OpenDrawer state */
		}			
	}
	/* else if (not fully open) and (open switch is now off)
	 * (ie, the openSw was on and now it's off)
	 */
	else if ((~curState & CD_DOOR_OPEN_SWITCH) && (delta & CD_DOOR_OPEN_SWITCH))
	{
		/* this could occur when someone presses on the door itself, 
		 * not the user switch
		 */
		if ((cd->cd_DevState & kMajorDevStateMask) != DS_CLOSE_DRAWER)
			cd->cd_DevState = DS_CLOSE_DRAWER;	/* go to CloseDrawer state */
	}
}

/*******************************************************************************
* ParseTOCEntries()                                                            *
*                                                                              *
*      Purpose: Interprets Qcode in order to build the Disc, TOC, and Session  *
*               Info.                                                          *
*                                                                              *
*   Parameters: cd          - pointer to the cdrom device in question          *
*               statusBytes - pointer to incoming status byte buffer           *
*               x           - index into buffer where QReport/Qcode status     *
*                             begins                                           *
*                                                                              *
*    Called by: ProcessSnarfBuffers(), upon receiving a QReport while we're    *
*               still building the discdata info.                              *
*                                                                              *
*******************************************************************************/
void ParseTOCEntries(cdrom *cd, uint8 *statusBytes, int32 x)
{
	uint8 adrctl = statusBytes[x+1];
	uint8 tracknum = statusBytes[x+2];
	uint8 point = statusBytes[x+3];
	uint8 min = statusBytes[x+4];
	uint8 sec = statusBytes[x+5];
	uint8 frame = statusBytes[x+6];
	uint8 pmin = statusBytes[x+8];
	uint8 psec = statusBytes[x+9];
	uint8 pframe = statusBytes[x+10];

	uint8	track;
	uint8	GotAllTracks;
	uint32	discInfoMSF;

	DBUG(kPrintGeneralStatusStuff, ("LCCD:  %02x\n", point));

	switch (adrctl & 0x0F)					/* mask out the control nybble */
	{
		case 0x01:							/* adr mode 1  (track time info) */
			/* if we're in the lead-out area (just prior to the next TOC) */
			if (tracknum == 0xAA)
				break;

			/* make sure that we've gotten an entry for A0, A1, and A2 */
			if (cd->cd_BuildingTOC == (TOC_GOT_A0 | TOC_GOT_A1 | TOC_GOT_A2))
			{
				GotAllTracks = TRUE;
				
				/* NOTE: The .firstTrackNumber should not ever change
				 * ...but the .lastTrackNumber can change for MULTISESSION discs
				 */
				if (!(cd->cd_State & CD_DISC_IS_CDI) ||
					(cd->cd_State & CD_DISC_IS_CDI_WITH_AUDIO))
				{
					for (track = cd->cd_DiscInfo.firstTrackNumber;
						(track <= cd->cd_DiscInfo.lastTrackNumber);
						track++)
					{
						if (!cd->cd_TOC_Entry[track].trackNumber)
						{
							GotAllTracks = FALSE;
							break;		/* break out of existence-test loop */
						}
					}
				}
				
				/* done reading toc (we've started to repeat entries) */
				if (GotAllTracks)
				{
					cd->cd_State &= ~CD_READING_TOC_INFO;
					cd->cd_State |= CD_GOT_ALL_TRACKS;
	
					/* the DiscInfo.MSF can change for MULTISESSION discs */
					cd->cd_MediumBlockCount = ((uint32)cd->cd_DiscInfo.minutes * 60L +
											  (uint32)cd->cd_DiscInfo.seconds) * 75L +
											  (uint32)cd->cd_DiscInfo.frames;
				}
			}

			if ((cd->cd_State & CD_READING_TOC_INFO) && !(cd->cd_State & CD_GOT_ALL_TRACKS))
			{
				/* NOTE:  For CD-i discs (discID = 0x10), the TOC is
				 * constructed slightly differently.  The PMIN(A0) entry is
				 * the first AUDIO track# (if the disc contains audio tracks);
				 * and PMIN(A1) is the last audio track#.  The AdrCtrl field
				 * of A1 will have it's 0x04 bit clear if the disc contains
				 * audio tracks; and therefore, audio track TOC entries.  The
				 * CD-i tracks do NOT have TOC entries.
				 */
				switch (point)
				{
					case 0xA0:
						/* PMIN(A0) = first track number (BCD) */
						/* PSEC(A0) = DiscID (0x00, 0x10, 0x20) */
						if (cd->cd_State & CD_READING_INITIAL_TOC)
						{
							cd->cd_DiscInfo.discID = psec;

							if (psec == 0x10)
							{
								DBUG(kPrintGeneralStatusStuff, ("LCCD:  Disc is CD-i\n"));
								cd->cd_DiscInfo.firstTrackNumber = 1;
								cd->cd_State |= CD_DISC_IS_CDI;

								/* initialize TOC entries for CD-i tracks as
								 * they have no "standard" entries.
								 */
								for (track = 1; track < BCD2BIN(pmin); track++)
								{
									/* mark it as a data track at 00:02:00 */
									cd->cd_TOC_Entry[track].addressAndControl = 0x14;
									cd->cd_TOC_Entry[track].trackNumber = track;
									cd->cd_TOC_Entry[track].minutes = 0x00;
									cd->cd_TOC_Entry[track].seconds = 0x02;
									cd->cd_TOC_Entry[track].frames = 0x00;
								}
							}
							else
								cd->cd_DiscInfo.firstTrackNumber = BCD2BIN(pmin);
						}
						else
							cd->cd_NextSessionTrack = BCD2BIN(pmin);
						cd->cd_BuildingTOC |= TOC_GOT_A0;
						break;
					case 0xA1:
						/* make sure that we've already received an A0 entry
						 * so we know the proper context (WRT CD-i, etc.)
						 */
						if (!(cd->cd_BuildingTOC & TOC_GOT_A0))
							break;

						if (cd->cd_State & CD_DISC_IS_CDI)
						{
							/* Note that the context of adrctl here
					 	 	* assumes that the nybbles are NOT swapped.
					 	 	*/
							if (!(adrctl & 0x40))
							{
    							cd->cd_State |= CD_DISC_IS_CDI_WITH_AUDIO;
								DBUG(kPrintWarnings, ("LCCD:  Disc is CD-i with audio\n"));
							}
						}

						if (!(cd->cd_State & CD_DISC_IS_CDI) ||
							(cd->cd_State & CD_DISC_IS_CDI_WITH_AUDIO))
						{
							/* PMIN = last track number (BCD) */
							cd->cd_DiscInfo.lastTrackNumber = BCD2BIN(pmin);
						}
						else
						{
							/* PMIN = number of CD-i tracks plus 1 */
							cd->cd_DiscInfo.lastTrackNumber = BCD2BIN(pmin) - 1;
						}
						cd->cd_BuildingTOC |= TOC_GOT_A1;
						break;
					case 0xA2:
						/* PMIN, PSEC, PFRM = MSF of lead-out area (BCD) */

						/* "minus 1" to match the MKE spec (ie, MKE gives the
						 * MSF of the last _valid_ sector)
						 */
						discInfoMSF = Offset2BCDMSF(BCDMSF2Offset(((uint32)pmin << 16) | ((uint32)psec << 8) | pframe) - 1);
						cd->cd_DiscInfo.minutes = BCD2BIN((uint8)((discInfoMSF >> 16) & 0xFF));
						cd->cd_DiscInfo.seconds = BCD2BIN((uint8)((discInfoMSF >> 8) & 0xFF));
						cd->cd_DiscInfo.frames = BCD2BIN((uint8)(discInfoMSF & 0xFF));

						cd->cd_BuildingTOC |= TOC_GOT_A2;
						break;
					default:
						/* hmmm...it must be a TOC entry for a track */
						point = BCD2BIN(point);

						/* must be swapped to match current MKE spec */
						cd->cd_TOC_Entry[point].addressAndControl = SwapNybble(adrctl);
						cd->cd_TOC_Entry[point].trackNumber = point;
						cd->cd_TOC_Entry[point].minutes = BCD2BIN(pmin);
						cd->cd_TOC_Entry[point].seconds = BCD2BIN(psec);
						cd->cd_TOC_Entry[point].frames = BCD2BIN(pframe);
						break;
				}
			}
			break;
		case 0x02:							/* adr mode 2  (UPC/EAN info) */
			memcpy(&cd->cd_LastQCode2, &cd->cd_LastQCode0, sizeof(SubQInfo));
			break;
		case 0x03:							/* adr mode 3  (ISRC info) */
			memcpy(&cd->cd_LastQCode3, &cd->cd_LastQCode0, sizeof(SubQInfo));
			break;
		case 0x05:							/* adr mode 5  (Hybrid/MultiSess) */
			/* only accept one mode 5, point 0xB0 per TOC */
			if ((point == 0xB0) && !(cd->cd_State & CD_READ_NEXT_SESSION))
			{
				/* indicate that we need to read the next session's TOC */
				cd->cd_State |= CD_READ_NEXT_SESSION;

				/* back up 01:00:00 from the MSF in the 05-entry (skip 
				 * ahead 1 sec to prevent f/w from landing in latency)
				 */
				cd->cd_TOC = BCDMSF2Offset(((uint32)min << 16) | ((uint32)sec << 8) | (uint32)frame) - BCDMSF2Offset(0x005900);
			}
			break;
		default:
			DBUG(kPrintGeneralStatusStuff, ("LCCD:  Unknown Qcode ADR! (CtlAdr = %02x)\n", adrctl));
			break;
	}
}



/*******************************************************************************
* PrepareDuckAndCover()                                                        *
*                                                                              *
*      Purpose: Sends ioReq to notify xbus that we wish to be notified when    *
*               some (any) device was just dipired.  This notification is      *
*               necessary because we have been playing (prefetching) when the  *
*               dipir of another device took place; and we need this hook to   *
*               be able to call RecoverFromDipir().                            *
*                                                                              *
*   Parameters: cd - pointer to cdrom device struct                            *
*                                                                              *
*      Returns: Err - if one occurs when sending ioReq to xbus                 *
*                                                                              *
*    Called by: LCCDNewDevice(), during device initialization                  *
*                                                                              *
*******************************************************************************/
Err PrepareDuckAndCover(cdrom *cd)
{
	Err err;
	
	DBUG(kPrintGeneralStatusStuff, ("LCCD:  Preparing for Duck-n-Cover\n"));
	
	memset(&cd->cd_recoverReq->io_Info, 0, sizeof(IOInfo));
	
	cd->cd_recoverReq->io_CallBack = DipirRecoverCallBack;
	cd->cd_recoverReq->io_Info.ioi_Command = XBUSCMD_WaitDipirEnd;
	cd->cd_recoverReq->io_Info.ioi_User = (uint32)cd;
		
	err = SuperInternalSendIO(cd->cd_recoverReq);
	CHK4ERR(err, ("LCCD:  >ERROR<  Problem setting up dipir recover ioReq\n"));
		
	return (kCDNoErr);
}

/*******************************************************************************
* DipirRecoverCallBack()                                                       *
*                                                                              *
*      Purpose: Notify daemon that we've returned from dipir; so it's ok to    *
*               proceed with psuedo-restart code:  InitReports(),              *
*               BuildDiscData(), etc. (in the case that it was our device that *
*               was dipired); or to call RecoverFromDipir() (in the case that  *
*               it was another device).                                        *
*                                                                              *
*   Parameters: theReq - pointer to the incoming dipirReq                      *
*                                                                              *
*      Returns: IOReq - This automagically resends the recoverReq to xbus.     *
*                                                                              *
*    Called by: OS, just after dipir.                                          *
*                                                                              *
*******************************************************************************/
IOReq *DipirRecoverCallBack(IOReq *theReq)
{
	cdrom	*cd = (cdrom *)theReq->io_Info.ioi_User;		/* what device? */
	
	/* indicate that we just returned from dipir-land */
	cd->cd_State |= CD_JUST_DIPIRED;
	cd->cd_State &= ~CD_DIPIR_PENDING;			 /* dipir no longer pending */

	/* notify daemon of newly received response */
	if (gDaemon)
		SuperInternalSignal(gDaemon, gDaemonSignal);

	/* don't return (ie, resend) the ioReq if it's being aborted
	 * by the daemon (this only occurs if the daemon's been killed)
	 */
	if (theReq->io_Error == ABORTED)
		return (NULL);
	
	/* cause kernel to resend this req to xbus */
	return (theReq);
}



/*******************************************************************************
* InitDataSpace()                                                              *
*                                                                              *
*      Purpose: Initializes data prefetch space (actually just the headers     *
*               associated with the data space).                               *
*                                                                              *
*   Parameters: cd - ptr to cdrom struct for this device                       *
*                                                                              *
*    Called by: ResizeLinkedBuffers(), RecoverFromDipir(), and Read().         *
*                                                                              *
*******************************************************************************/
void InitDataSpace(cdrom *cd)
{
	int32 x;
	
	/* initialize data buffer & indeces */
	cd->cd_DataReadIndex = 0;
	cd->cd_CurDataWriteIndex = 0;
	cd->cd_NextDataWriteIndex = 1;
	for (x = 0; x < cd->cd_NumDataBlks; x++)
	{
		cd->cd_DataBlkHdr[x].state = BUFFER_BLK_FREE;
		cd->cd_DataBlkHdr[x].format = INVALID_SECTOR;
		cd->cd_DataBlkHdr[x].MSF = 0;
	}
}

/*******************************************************************************
* InitSubcodeSpace()                                                           *
*                                                                              *
*      Purpose: Initializes subcode prefetch space (actually just the headers  *
*               associated with the subcode space).                            *
*                                                                              *
*   Parameters: cd - ptr to cdrom struct for this device                       *
*                                                                              *
*    Called by: ResizeLinkedBuffers(), RecoverFromDipir(), and Read().         *
*                                                                              *
*******************************************************************************/
void InitSubcodeSpace(cdrom *cd)
{
	int32 x;
	
	/* initialize subcode buffer & indeces */
	cd->cd_SubcodeReadIndex = 0;
	cd->cd_CurSubcodeWriteIndex = 0;
	cd->cd_NextSubcodeWriteIndex = 1;
	for (x = 0; x < cd->cd_NumSubcodeBlks; x++)
		cd->cd_SubcodeBlkHdr[x].state = BUFFER_BLK_FREE;
}

/*******************************************************************************
* InitPrefetchEngine()                                                         *
*                                                                              *
*      Purpose: Initializes the Prefetch and Subcode Engine(s) by marking the  *
*               first two buffer blocks (cur & next) as "in use", then         *
*               configuring the DMA registers to point to those buffer blocks, *
*               and then enabling the interrupts for channels 0 and 1.         *
*               Channel 1 will only be enabled if we have been requested to    *
*               return the subcode info with the sector data.                  *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*    Called by: Read() and RecoverFromDipir().                                 *
*                                                                              *
*******************************************************************************/
void InitPrefetchEngine(cdrom *cd)
{
	uint8	curBlk = cd->cd_CurDataWriteIndex;
	uint8	nextBlk = cd->cd_NextDataWriteIndex;
	
	/* at this point, nextBlk = (curBlk + 1) */
	cd->cd_DataBlkHdr[curBlk].state = BUFFER_BLK_INUSE;	
	cd->cd_DataBlkHdr[nextBlk].state = BUFFER_BLK_INUSE;

	/* if we're reading an audio sector (and subcode was requested) */
	if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
	{
		uint8	curSubBlk = cd->cd_CurSubcodeWriteIndex;
		uint8	nextSubBlk = cd->cd_NextSubcodeWriteIndex;
		
		/* at this point, nextBlk = (curBlk + 1) */
		cd->cd_SubcodeBlkHdr[curSubBlk*2].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[(curSubBlk*2)+1].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[nextSubBlk*2].state = BUFFER_BLK_INUSE;
		cd->cd_SubcodeBlkHdr[(nextSubBlk*2)+1].state = BUFFER_BLK_INUSE;
	
		ConfigLCCDDMA(DMA_CH0, CUR_DMA, cd->cd_DataBlkHdr[curBlk].buffer, cd->cd_BlockLength);
		ConfigLCCDDMA(DMA_CH0, NEXT_DMA, cd->cd_DataBlkHdr[nextBlk].buffer, cd->cd_BlockLength);

		/* note that we use lengths of 192 (196-4) because the DMA h/w 
		 * requires lengths/ptrs to be multiples of 4 
		 * ...so we cram 2 logical buffer blocks (98 bytes each) into 1 
		 * physical block.  Note use of 196 "minus 4" for dma funkiness
		 */
		ConfigLCCDDMA(DMA_CH1, CUR_DMA, cd->cd_SubcodeBuffer[curSubBlk], 192);
		ConfigLCCDDMA(DMA_CH1, NEXT_DMA, cd->cd_SubcodeBuffer[nextSubBlk], 192);

		/* Enable cur/next DMA for channels 0 and 1 */
		EnableLCCDDMA(DMA_CH0 | DMA_CH1);
	}
	else
	{
		ConfigLCCDDMA(DMA_CH0, CUR_DMA, cd->cd_DataBlkHdr[curBlk].buffer, cd->cd_BlockLength);
		ConfigLCCDDMA(DMA_CH0, NEXT_DMA, cd->cd_DataBlkHdr[nextBlk].buffer, cd->cd_BlockLength);
	
		/* Enable cur/next DMA for (data) channel 0 only */
		EnableLCCDDMA(DMA_CH0);
	}
}

/*******************************************************************************
* PrepareCachedInfo()                                                          *
*                                                                              *
*      Purpose: Prepares the initial value of the StatusByte.  Also takes the  *
*               device offline (clearing the TOC info, too), if appropriate.   *
*               Sets the flag CD_CACHED_INFO_AVAIL to indicate that we at      *
*               least tried to read the TOC and allows CmdStatus() and         *
*               DiscData() to complete.   However, DiscData() will return all  *
*               zeros if the device is offline...as desired.                   *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon() and BuildDiscData().                        *
*                                                                              *
*******************************************************************************/
void PrepareCachedInfo(cdrom *cd)
{
	if (cd->cd_State & CD_DOOR_CLOSED_SWITCH)	/* is the drive door closed? */
	{
		/* door closed */
		cd->cd_StatusByte |= SB_DOOR;
	}
	else
	{
		/* door open */
		cd->cd_StatusByte &= ~(SB_DOOR | SB_DISCIN | SB_SPINUP | SB_DBLSPD | SB_READY);

		TakeDeviceOffline(cd);

		goto done;
	}

	if (cd->cd_State & CD_NO_DISC_PRESENT)
	{
		/* drive "ready", but no disc present */
		cd->cd_StatusByte |= SB_READY;
		cd->cd_StatusByte &= ~(SB_DISCIN | SB_SPINUP | SB_DBLSPD);

		TakeDeviceOffline(cd);
		
		goto done;
	}
	
	if (cd->cd_State & CD_UNREADABLE)
	{
		/* drive not ready...because unreadable disc is present, TOC not read */
		cd->cd_StatusByte &= ~(SB_READY);
		cd->cd_StatusByte |= SB_DISCIN;

		TakeDeviceOffline(cd);
		
		goto done;
	}

	/* drive ready, readable disc present, TOC read */
	cd->cd_StatusByte |= (SB_READY | SB_DISCIN);
	
	switch (cd->cd_CIPState)		/* what state is the drive currently in? */
	{
		case CIPPause:
		case CIPPlay:
		case CIPSeeking:
		case CIPSpinningUp:
		case CIPLatency:
			cd->cd_StatusByte |= SB_SPINUP;				/* disc is spinning */
			break;
		default:
			cd->cd_StatusByte &= ~SB_SPINUP;			/* disc is stopped */
			break;
	}

done:
	DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));

	/* indicate that STATUS and DISCDATA cmds can now be completed */
	cd->cd_State |= CD_CACHED_INFO_AVAIL;
}

/*******************************************************************************
* TakeDeviceOffline()                                                          *
*                                                                              *
*      Purpose: Takes the device off-line by clearing CD_DEVICE_ONLINE, and    *
*               clears any current Disc, TOC, and Session Info.                *
*                                                                              *
*   Parameters: cd          - pointer to the cdrom device in question          *
*               statusBytes - pointer to incoming status byte buffer           *
*               x           - index into buffer where QReport/Qcode status     *
*                             begins                                           *
*                                                                              *
*    Called by: PrepareCachedInfo() and HandleSwReport().                      *
*                                                                              *
*******************************************************************************/
void TakeDeviceOffline(cdrom *cd)
{
	/* take device "off-line" */
	cd->cd_State &= ~CD_DEVICE_ONLINE;

	/* clear disc, TOC, and session info */
	memset(&cd->cd_DiscInfo, 0, sizeof(CDDiscInfo));
	memset(cd->cd_TOC_Entry, 0, sizeof(CDTOCInfo));
	memset(&cd->cd_SessionInfo, 0, sizeof(CDSessionInfo));
}

/*******************************************************************************
* AbortCurrentIOReqs()                                                         *
*                                                                              *
*      Purpose: Aborts all currently active/pending ioReqs, except for         *
*               CMD_STATUS.                                                    *
*                                                                              *
*   Parameters: cd - pointer to cdrom device structure in question             *
*                                                                              *
*    Called by: LCCDDriverDaemon() and HandleSwReport().                       *
*                                                                              *
*******************************************************************************/
void AbortCurrentIOReqs(cdrom *cd)
{
	List	tmp;
	uint32	interrupts;
	IOReq	*theNode, *theNextNode;
	
	/* abort any workingIOR */
	if (cd->cd_workingIOR)
		CompleteWorkingIOReq(cd->cd_workingIOR, kCDErrAborted);	
	
	InitList(&tmp,"temporary list");
	
	/* abort all ioReqs except CMD_STATUS */
	interrupts = Disable();
	for(theNode = (IOReq *)FirstNode(&cd->cd_pendingIORs);
		IsNode(&cd->cd_pendingIORs, theNode);
		theNode = theNextNode)
	{
		theNextNode = (IOReq *)NextNode(theNode);
		if (theNode->io_Info.ioi_Command != CMD_STATUS)
		{
			RemNode((Node *)theNode);		/* remove ioReq from pending list */
			AddTail(&tmp, (Node *)theNode);	/* ...add it to temporary list */
		}
	}
	Enable(interrupts);

	/* abort items on temporary list */
	while (!IsEmptyList(&tmp))
		CompleteWorkingIOReq((IOReq *)RemHead(&tmp), kCDErrAborted);
}

/*******************************************************************************
* CompleteWorkingIOReq()                                                       *
*                                                                              *
*      Purpose: Completes the currently working ioReq; allowing the daemon to  *
*               process the next pending ioReq.                                *
*                                                                              *
*   Parameters: theReq - pointer to ioRequest to complete                      *
*               theErr - any error to return in io_Error                       *
*                                                                              *
*      Returns: none                                                           *
*                                                                              *
*    Called by: OS, upon receiving a SendIO() to this driver                   *
*                                                                              *
*******************************************************************************/
void CompleteWorkingIOReq(IOReq *theReq, Err theErr)
{
	cdrom	*cd = (cdrom *)theReq->io_Dev;
	
	DBUG(kPrintSendCompleteIO, ("LCCD:  Completing the working ioReq (%02x, err:%0ld)\n", theReq->io_Info.ioi_Command, theErr));

	if (theErr < kCDNoErr)
	{
		DBUG2(kPrintWarnings, ("LCCD:  Completing ioReq with error (%02x, err:%0ld)\n", theReq->io_Info.ioi_Command, theErr));
		theReq->io_Error = theErr;					/* update for any error */
	}
	
	SuperCompleteIO(theReq);						/* complete the request */
	
	if (theReq == cd->cd_workingIOR)
		cd->cd_workingIOR = NULL;					/* not working any longer */
}



/*******************************************************************************
* OpenDrawer()                                                                 *
*                                                                              *
*      Purpose: Provides the state machine (ha!) needed to open the drawer.    *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_OPEN_DRAWER; or ProcessClientIOReqs(), upon receiving a     *
*               CDROM_OPEN_DRAWER command.                                     *
*                                                                              *
*******************************************************************************/
Err OpenDrawer(cdrom *cd)
{
	uint8 ppsO[] = { CD_PPSO, 0x00 };					/* open the drawer */
	
	return (SendCmdWithWaitState(cd, ppsO, sizeof(ppsO), CIPOpen, NULL));
}

/*******************************************************************************
* CloseDrawer()                                                                *
*                                                                              *
*      Purpose: Provides the state machine needed to close the drawer.  Also   *
*               hand-holds this system thru the dipir process, by 'inducing'   *
*               dipir.  This is done by pinging the drive (with "give me a CIP *
*               report now") after media is known to be set (after the switch  *
*               report is sent indicating that the close switch is engaged).   *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_CLOSE_DRAWER; or ProcessClientIOReqs(), upon receiving a    *
*               CDROM_CLOSE_DRAWER command.                                    *
*                                                                              *
*******************************************************************************/
Err CloseDrawer(cdrom *cd)
{
	uint8	ppSo[] = { CD_PPSO, 0x01 };					/* close the drawer */
	uint8	CIPRptNow[] = { CD_CIPREPORTEN, 0x01};		/* get one now */
	Err		err;
	
	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case CL_START:
			err = SendCmdWithWaitState(cd, ppSo, sizeof(ppSo), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = CL_CLOSE;
		case CL_CLOSE:
			/* if dipir not indicated as "pending" yet, then we have yet to 
			 * receive the SwRprt indicating that the drawer has closed fully
			 */
			if (!(cd->cd_State & CD_DIPIR_PENDING))
				break;
			cd->cd_DevState = CL_DIPIR;
		case CL_DIPIR:
			err = SendCmdWithWaitState(cd, CIPRptNow, sizeof(CIPRptNow), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			return (kCmdComplete);
	}
		
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* InitReports()                                                                *
*                                                                              *
*      Purpose: Provides the state machine needed to enable all reports as     *
*               needed for proper driver/user interaction.                     *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_INIT_REPORTS.                                               *
*                                                                              *
*******************************************************************************/
Err InitReports(cdrom *cd)
{
	uint8 LED[] = { CD_LED, 0x02 };							/* LED is on in focusing, spinning, seeking, latency, and play states */
	uint8 SwRptEn[] = { CD_SWREPORTEN, 0x02};				/* turn on switch reports (we take control back from the firmware) */
	uint8 CIPRptEn[] = { CD_CIPREPORTEN, 0x03};				/* turn on ALL CIP reports (f/w automatically sends one NOW) */
	uint8 QRptDis[] = { CD_QREPORTEN, 0x00 };				/* make sure QReports are disabled */
	uint8 MechType[] = { CD_MECHTYPE, 0x00, 0x00, 0x00 };	/* check for drawer/clamshell type */
	Err	err;
	
	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case IR_START:
			err = SendCmdWithWaitState(cd, LED, sizeof(LED), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = IR_SWEN;
		case IR_SWEN:
			err = SendCmdWithWaitState(cd, SwRptEn, sizeof(SwRptEn), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = IR_QDIS;
		case IR_QDIS:
			err = SendCmdWithWaitState(cd, QRptDis, sizeof(QRptDis), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = IR_MECH;
		case IR_MECH:
			err = SendCmdWithWaitState(cd, MechType, sizeof(MechType), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = IR_CIPEN;
		case IR_CIPEN:
			err = SendCmdWithWaitState(cd, CIPRptEn, sizeof(CIPRptEn), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			return (kCmdComplete);
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* BuildDiscData()                                                              *
*                                                                              *
*      Purpose: Provides the state machine needed to read the TOC, build the   *
*               disc info, and read the session info.  If an 0x05/0xB0 entry   *
*               is observed in the TOC by ParseTOCEntries(),                   *
*               CD_READ_NEXT_SESSION is set to indicate that this is           *
*               potentially a multisession disc.  We then jump out to the next *
*               TOC should be located, and attempt to read the next session.   *
*               If there is no next session located there, then the firmware   *
*               returns CIPSeekFailure.                                        *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_BUILDING_DISCDATA.                                          *
*                                                                              *
*******************************************************************************/
Err BuildDiscData(cdrom *cd)
{
	uint8	pPso[] = { CD_PPSO, 0x02 };									/* stop (drive will automatically seek to start of TOC) */
	uint8	driveSpeed[] = { CD_SETSPEED, CDROM_DOUBLE_SPEED, 0x00 };	/* single/double speed + pitch */
	uint8	bobFormat[] = { CD_SECTORFORMAT, DA_SECTOR, 0x00 };			/* sector format + subcode enable */
	uint8	seek2toc[] = { CD_SEEK, 0x00, 0x00, 0x00 };					/* seek to 00:00:00 (f/w will automatically start PLAYing) */
	uint8	Ppso[] = { CD_PPSO, 0x03 };									/* play */
	uint8	QRptEn[] = { CD_QREPORTEN, 0x02 };							/* turn on QReports */
	uint8	QRptDis[] = { CD_QREPORTEN, 0x00 };							/* turn off QReports */
	uint8	seek2pause[] = { CD_SEEK, 0x00, 0x00, 0x96 };				/* seek somewhere in order to pause (seek is supplied with a binary offset) */
	Err		err;
	uint8	errCode;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case BDD_START:
			cd->cd_State &= ~CD_ALREADY_RETRIED_TOC_READ;
retryTOC:
			memset(&cd->cd_DiscInfo, 0, sizeof(CDDiscInfo));
			memset(&cd->cd_TOC_Entry, 0, sizeof(cd->cd_TOC_Entry));
			memset(&cd->cd_SessionInfo, 0, sizeof(CDSessionInfo));

			/* initial TOC starts at 00:00:00 */
			cd->cd_TOC = 0;

			cd->cd_BuildingTOC = FALSE;
			cd->cd_MediumBlockCount = 0;
			cd->cd_NextSessionTrack = 0;			/* no multi-session, yet */
			cd->cd_State &= ~(CD_READING_TOC_INFO | CD_READ_NEXT_SESSION | CD_GOT_ALL_TRACKS);
			cd->cd_State |= CD_READING_INITIAL_TOC;

			/* if dipir left us playing...we need to pause */
			if (cd->cd_CIPState == CIPPlay)
				cd->cd_DevState = BDD_PAUSE;
			else
				goto speedjump;
		case BDD_PAUSE:
			/* pause the drive */
			err = SendCmdWithWaitState(cd, pPso, sizeof(pPso), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
speedjump:
			DisableLCCDDMA(DMA_CH0 | DMA_CH1);			/* disable any DMA */
			cd->cd_DevState = BDD_SPEED;
		case BDD_SPEED:
			/* read the TOC in double speed */
			err = SendCmdWithWaitState(cd, driveSpeed, sizeof(driveSpeed), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_StatusByte |= SB_DBLSPD;				/* update status byte */
			DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));

			/* save current settings */
			cd->cd_CurrentSettings.speed = driveSpeed[1];
			cd->cd_CurrentSettings.pitch = CDROM_PITCH_NORMAL;

			cd->cd_DevState = BDD_FORMAT;
		case BDD_FORMAT:
			/* read the TOC in audio mode */
			err = SendCmdWithWaitState(cd, bobFormat, sizeof(bobFormat), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);

			/* save current settings */
			cd->cd_CurrentSettings.format = bobFormat[1];
			cd->cd_CurrentSettings.subcode = bobFormat[2];

			cd->cd_DevState = BDD_SEEK;
multiseek:
			DBUG(kPrintTOCAddrs, ("TOC @ %06lX\n", Offset2BCDMSF(cd->cd_TOC)));
		case BDD_SEEK:
			/* this flag interpreted upon receiving a QRpt */
			cd->cd_State |= CD_READING_TOC_INFO;

			/* seek to beginning of TOC.  NOTE: addr is a binary offset */
			*(uint32 *)seek2toc |= cd->cd_TOC;
			err = SendCmdWithWaitState(cd, seek2toc, sizeof(seek2toc), CIPPause, &errCode);
			if (errCode == CIPSeekFailure)
				goto multidone;
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = BDD_PLAY;
		case BDD_PLAY:
			err = SendCmdWithWaitState(cd, Ppso, sizeof(Ppso), CIPPlay, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = BDD_QEN;
		case BDD_QEN:
			/* start listening to QReports to build the TOC */
			err = SendCmdWithWaitState(cd, QRptEn, sizeof(QRptEn), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = BDD_QDIS;
		case BDD_QDIS:
			/* still reading the TOC? */
			if (cd->cd_State & CD_READING_TOC_INFO)
			{
				/* if Trk# != 0, we've read beyond the TOC area */
				if (cd->cd_LastQCode0.trackNumber && (cd->cd_LastQCode0.trackNumber != 0xAA))
				{
					if (cd->cd_State & CD_ALREADY_RETRIED_TOC_READ)
					{
						DBUG2(kPrintDeath, ("LCCD:  >DEATH<  Unable to read complete TOC...FAILED\n"));
					
						cd->cd_State |= CD_UNREADABLE;
					
						/* clear these so tests in BDD_CHK4MULTI will fail */
						cd->cd_State &= ~(CD_READING_TOC_INFO | CD_READ_NEXT_SESSION);
						cd->cd_NextSessionTrack = 0;
					}
					else
					{
						DBUG2(kPrintWarnings, ("LCCD:  >WARNING<  Unable to read complete TOC...ATTEMPTING RETRY\n"));
						cd->cd_State |= CD_ALREADY_RETRIED_TOC_READ;
						goto retryTOC;
					}
				}
				else
					break;
			}
			/* stop listening to QReports */
			err = SendCmdWithWaitState(cd, QRptDis, sizeof(QRptDis), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
multidone:
			cd->cd_DevState = BDD_CHK4MULTI;
		case BDD_CHK4MULTI:
			if (cd->cd_State & CD_READ_NEXT_SESSION)
			{
				cd->cd_BuildingTOC = FALSE;
				
				cd->cd_State &= ~(CD_READ_NEXT_SESSION | CD_READING_INITIAL_TOC | CD_GOT_ALL_TRACKS);
				cd->cd_DevState = BDD_SEEK;
				goto multiseek;
			}
			
			if (cd->cd_NextSessionTrack)
			{
				cd->cd_SessionInfo.valid = 0x80;	/* session info valid bit */
				cd->cd_SessionInfo.minutes = cd->cd_TOC_Entry[cd->cd_NextSessionTrack].minutes;
				cd->cd_SessionInfo.seconds = cd->cd_TOC_Entry[cd->cd_NextSessionTrack].seconds;
				cd->cd_SessionInfo.frames = cd->cd_TOC_Entry[cd->cd_NextSessionTrack].frames;
			}

			PrepareCachedInfo(cd);
			cd->cd_DevState = BDD_SEEK2PAUSE;
		case BDD_SEEK2PAUSE:
			/* seek out of the TOC to pause */
			err = SendCmdWithWaitState(cd, seek2pause, sizeof(seek2pause), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			return (kCmdComplete);
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* RecoverFromDipir()                                                           *
*                                                                              *
*      Purpose: Provides the state machine needed to recover from a            *
*               "recoverable" dipir.  A "recoverable" dipir is one in which    *
*               the system doesn't get rebooted, and the OS re-loaded (such as *
*               with "data discs").                                            *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon seeing that the JUST_DIPIRED flag was *
*               set by DipirRecoverCallBack().                                 *
*                                                                              *
*******************************************************************************/
Err RecoverFromDipir(cdrom *cd)
{
	uint8	pPso[] = { CD_PPSO, 0x02 };					/* pause the drive */
	IOReq	*currentIOReq = cd->cd_workingIOR;
	Err		err;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case RFD_START:
			/* were playing when the dipir (of another device) happened? */
			if ((cd->cd_CIPState != CIPSeeking) && (cd->cd_CIPState != CIPLatency) && (cd->cd_CIPState != CIPPlay))
				goto recover;
			cd->cd_DevState = RFD_PAUSE;
		case RFD_PAUSE:
			DBUG(kPrintDipirStuff, ("LCCD:  Sending pause to recover from dipir\n"));
			err = SendCmdWithWaitState(cd, pPso, sizeof(pPso), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
recover:
			cd->cd_DevState = RFD_RECOVER;
		case RFD_RECOVER:
			DBUG(kPrintDipirStuff, ("LCCD:  Disabling DMA, etc. after dipir\n"));
			DisableLCCDDMA(DMA_CH0 | DMA_CH1);			/* disable any DMA */

			cd->cd_State &= ~CD_PREFETCH_OVERRUN;		/* reset overrun flag */
			cd->cd_State &= ~CD_CURRENTLY_PREFETCHING;	/* reset pref. flag */
			cd->cd_State &= ~CD_GONNA_HAVE_TO_STOP;		/* reset the bit */
			cd->cd_State &= ~CD_JUST_DIPIRED;			/* clear this bit */
			
			InitDataSpace(cd);
			InitSubcodeSpace(cd);
			InitPrefetchEngine(cd);
			
			/* if currently working on an IOReq
			 * ...we need to reset io_Actual (we'll retry this ioReq later)
			 */
			if (currentIOReq)
				currentIOReq->io_Actual = 0;
			
			DBUG(kPrintDipirStuff, ("LCCD:  Dipir recovery completed\n"));
			
			/* make sure the daemon doesn't go to sleep w/o sending a command
			 * (in the SavedDevState machine)...otherwise we'd never wake up
			 */
			if (gDaemon)
				SuperInternalSignal(gDaemon, gDaemonSignal);
				
			return (kCmdComplete);
	}
	
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* StopPrefetching()                                                            *
*                                                                              *
*      Purpose: Provides the state machine needed to disable the Prefetch      *
*               Engine.  We first seek to the 'next sector' (the last sector   *
*               in the prefetch space plus 1) anticipating sequential reads,   *
*               wait for the drive to enter the paused state, then disable     *
*               DMA, and clear CD_PREFETCH_OVERRUN.                            *
*                                                                              *
*               If we are already in the process of opening the drawer or      *
*               restarting a read to go get data from somewhere else on the    *
*               disc, then we do not seek (and pause), we simply disable DMA,  *
*               etc.                                                           *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon seeing that CD_PREFETCH_OVERRUN was   *
*               set by the FIRQ handler.                                       *
*                                                                              *
*******************************************************************************/
Err StopPrefetching(cdrom *cd)
{
	/* seek to next (sequential) sector */
	uint8	seek2next[] = { CD_SEEK, 0x00, 0x00, 0x00 };
	Err		err;

	/* NOTE:  These conditionals are here to prevent the possibility of
	 *        sending this SEEK before we actually got the cmd tag back for a
	 *        just-sent command; or in the case of Read(), we're already in the
	 *        process of trying to PAUSE, so just disable the DMA and return.
	 */
	if (cd->cd_workingIOR)
	{
		if (((cd->cd_SavedDevState & ~kCmdStateMask) == RD_RESTART) ||
			((cd->cd_workingIOR->io_Info.ioi_Command == CDROMCMD_READ_SUBQ) &&
			((cd->cd_SavedDevState & ~kCmdStateMask) == SQ_QNOW)))
		{
			goto killDMA;
		}
	}
	
	if ((cd->cd_SavedDevState & kMajorDevStateMask) == DS_OPEN_DRAWER)
		goto killDMA;

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case PE_START:
			/* "pre-seek" to what would be the next sequential sector if we
			 * got a sequential read AFTER the prefetch space filled up
			 */
			*(uint32 *)seek2next |= BCDMSF2Offset(cd->cd_PrefetchCurMSF);
			err = SendCmdWithWaitState(cd, seek2next, sizeof(seek2next), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = PE_PAUSE;
		case PE_PAUSE:
			DisableLCCDDMA(DMA_CH0 | DMA_CH1);			/* disable any DMA */
killDMA:
			cd->cd_State &= ~CD_PREFETCH_OVERRUN;		/* reset overrun flag */
			cd->cd_State &= ~CD_CURRENTLY_PREFETCHING;	/* reset pref. flag */
			cd->cd_State &= ~CD_GONNA_HAVE_TO_STOP;		/* reset the bit */
			
			DBUG(kPrintGeneralStatusStuff, ("LCCD:  Prefetch Engine disabled\n"));
			
			/* make sure the daemon doesn't go to sleep w/o sending a command
			 * (in the SavedDevState machine)...otherwise we'd never wake up
			 */
			if (gDaemon)
				SuperInternalSignal(gDaemon, gDaemonSignal);
				
			return (kCmdComplete);
	}
	
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* ProcessClientIOReqs()                                                        *
*                                                                              *
*      Purpose: If we're not currently working on an ioReq, pulls the next     *
*               pendingIOR off and dispatches it to the appropriate command    *
*               action machine.  When the command action machine completes, we *
*               call CompleteWorkingIOReq() and signal the daemon so we can    *
*               process any ioReq(s) left pending.                             *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*    Called by: LCCDDriverDaemon(), upon arriving in device state              *
*               DS_PROCESS_CLIENT_IOREQS.                                      *
*                                                                              *
*******************************************************************************/
void ProcessClientIOReqs(cdrom *cd)
{
	uint32	interrupts;
	Err		err = FALSE;

	/* if not currently working, get the next ioReq (if any) */
	if (!cd->cd_workingIOR)
	{	
		interrupts = Disable();
		cd->cd_workingIOR = (IOReq *)RemHead(&cd->cd_pendingIORs);
		Enable(interrupts);

		/* reset the action machine(s) */
		cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;
	}
	
	/* if currently working, or got new ioReq, deal with it... */
	if (cd->cd_workingIOR)
	{
		/* jump into the appropriate action machines */
		switch (cd->cd_workingIOR->io_Info.ioi_Command)
		{
			case CMD_READ:
			case CDROMCMD_READ_SUBQ:
				if (cd->cd_State & CD_DEVICE_ONLINE)
					err = (cd->cd_workingIOR->io_Info.ioi_Command == CMD_READ) ? Read(cd) : ReadSubQ(cd);
				else
				{
					/* reset the action machine(s) */
					cd->cd_DevState = DS_PROCESS_CLIENT_IOREQS;
					err = kCDErrDeviceOffline;
				}
				break;
			case CMD_STATUS:				err = CmdStatus(cd, cd->cd_workingIOR);		break;
			case CDROMCMD_DISCDATA:			err = DiscData(cd, cd->cd_workingIOR);		break;
			case CDROMCMD_OPEN_DRAWER:		err = OpenDrawer(cd);						break;
			case CDROMCMD_CLOSE_DRAWER:
				cd->cd_DevState = DS_CLOSE_DRAWER;
				err = CloseDrawer(cd);
				break;
			case CDROMCMD_SETDEFAULTS:		err = SetDefaults(cd);						break;
			case CDROMCMD_RESIZE_BUFFERS:	err = ResizeBufferSpace(cd);				break;
		}
		
		/* if the cmd we are working on just finished... */
		if (err)
		{
			CompleteWorkingIOReq(cd->cd_workingIOR, err);

			/* signal daemon to make sure we process any pending ioReq */
			if (gDaemon)
				SuperInternalSignal(gDaemon, gDaemonSignal);
		}

	}
}

/*******************************************************************************
* CmdStatus()                                                                  *
*                                                                              *
*      Purpose: Support for CMD_STATUS driver API call.                        *
*                                                                              *
*   Parameters: cd     - ptr todrom struct for this device                     *
*               theReq - the ioReq associated with the CMD_STATUS              *
*                                                                              *
*    Called by: LCCDDriverDispatch() and ProcessClientIOReqs().                *
*                                                                              *
*******************************************************************************/
int32 CmdStatus(cdrom *cd, IOReq *theReq)
{
	DeviceStatus	status;
	int32			actual;

	/* this provides an "initial holdoff" so that we can at least attempt to
	 * read the TOC, etc.  This is required so that the filesystem waits for
	 * for valid data (in the DeviceStatus struct) to be returned
	 */
	if (!(cd->cd_State & CD_CACHED_INFO_AVAIL))
		return (kCmdNotYetComplete);
	
	status.ds_DriverIdentity = DI_LCCD_CDROM;
	status.ds_DriverStatusVersion = 0;
	status.ds_MaximumStatusSize = sizeof(status);
	status.ds_DeviceFlagWord = (uint32)cd->cd_StatusByte;
	status.ds_FamilyCode = DS_DEVTYPE_CDROM;
	status.ds_DeviceUsageFlags = DS_USAGE_READONLY;
	status.ds_DeviceBlockCount = cd->cd_MediumBlockCount;
	/* this is determined from DiscInfo */
	status.ds_DeviceLastErrorCode = 0;				/* unused in MKE driver */
	status.ds_DeviceMediaChangeCntr = 0;			/* unused in MKE driver */

	/* this assumes that all data is of one type */
	if (cd->cd_TOC_Entry[1].addressAndControl & kDataTrackTOCEntry)
	{
		DBUG(kPrintGeneralStatusStuff, ("LCCD:  Track 1 is data, assuming blocksize is 2048\n"));
		status.ds_DeviceBlockSize = 2048;		/* CDROM_M1_D (mode 1 data) */
		status.ds_DeviceUsageFlags |= DS_USAGE_FILESYSTEM;
	}
	else
	{
		DBUG(kPrintGeneralStatusStuff, ("LCCD:  Track 1 is audio, assuming blocksize is 2352\n"));
		status.ds_DeviceBlockSize = 2352;		/* CDROM_DA (digital audio) */
	}
	
	actual = (int32) ((theReq->io_Info.ioi_Recv.iob_Len < sizeof(status)) ?
			  theReq->io_Info.ioi_Recv.iob_Len : sizeof(status));
	memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, &status, actual);
	theReq->io_Actual = actual;
	
	DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));
	
	return (kCmdComplete);
}

/*******************************************************************************
* DiscData()                                                                   *
*                                                                              *
*      Purpose: Support for CDROMCMD_DISCDATA driver API call.                 *
*                                                                              *
*   Parameters: cd     - ptr todrom struct for this device                     *
*               theReq - the ioReq associated with the CDROMCMD_DISCDATA       *
*                                                                              *
*    Called by: LCCDDriverDispatch() and ProcessClientIOReqs().                *
*                                                                              *
*******************************************************************************/
int32 DiscData(cdrom *cd, IOReq *theReq)
{
	CDROM_Disc_Data	*data;
	int32			actual;

	if (!(cd->cd_State & CD_CACHED_INFO_AVAIL))
		return (kCmdNotYetComplete);

	/* NOTE:  assumes order of internal structs:  cd_DiscInfo, cd_TOC_Entry, 
	 *        cd_SessionInfo in cdrom struct does not change
	 */
	data = (CDROM_Disc_Data *)&cd->cd_DiscInfo;
	actual = theReq->io_Info.ioi_Recv.iob_Len;
	if (actual > sizeof(CDROM_Disc_Data))
		actual = sizeof(CDROM_Disc_Data);
	memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, data, actual);
	theReq->io_Actual = actual;

	return (kCmdComplete);
}

/*******************************************************************************
* Read()                                                                       *
*                                                                              *
*      Purpose: Provides the state machine needed for CMD_READ requests.       *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device                               *
*                                                                              *
*      Returns: Err - if any occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err Read(cdrom *cd)
{
	CDROMCommandOptions	options;
	IOReq				*theReq = cd->cd_workingIOR;
	uint8				sectorFormat;
	uint32				bufLen;
	uint32				blockLen;
	int8				bufBlk;
	Err					err;

	uint8 pPso[] = { CD_PPSO, 0x02 };						/* pause (close the data valve, stop any dma) */
	uint8 driveSpeed[] = { CD_SETSPEED, 0x01, 0x00 };		/* single/double speed + pitch */
	uint8 bobFormat[] = { CD_SECTORFORMAT, 0x00, 0x00 };	/* sector format + subcode enable */
	uint8 seek2sector[] = { CD_SEEK, 0x00, 0x00, 0x00 };	/* seek */
	uint8 Ppso[] = { CD_PPSO, 0x03 };						/* play (open data valve) */
	
	/* calc stuff that used in multiple states
	 * (NOTE:  blockLen, sectorFormat are not valid on the first pass...
	 * because options gets updated in RD_START)
	 */
	options.asLongword = theReq->io_Info.ioi_CmdOptions;
	blockLen = options.asFields.blockLength;
	sectorFormat = MKE2BOB(cd, options.asFields.densityCode);

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case RD_START:
			DBUG(kPrintActionMachineStates, ("LCCD:  RD_START\n"));

			err = VerifyCmdOptions(options.asLongword);
			if (err)
				return (err);
			
			if (!options.asFields.densityCode)		options.asFields.densityCode = 		cd->cd_DefaultOptions.asFields.densityCode;
			if (!options.asFields.addressFormat)	options.asFields.addressFormat =	cd->cd_DefaultOptions.asFields.addressFormat;
			if (!options.asFields.speed)			options.asFields.speed =			cd->cd_DefaultOptions.asFields.speed;
			if (!options.asFields.pitch)			options.asFields.pitch =			cd->cd_DefaultOptions.asFields.pitch;
			if (!options.asFields.blockLength)		options.asFields.blockLength =		cd->cd_DefaultOptions.asFields.blockLength;
		
			/* Note that this was originally coded to allow someone to be able
			 * to chose the default errorRecovery (by setting it to zero);
			 * while also allowing them to use a different retryShift (by 
			 * setting it to non-zero...if they wanted "no retries", then they
			 * were required to specify the errorRecovery value).  This was a
			 * superset of the MKE functionality.
			 *
			 * As it is, we had to revert back to only allowing a different
			 * retryShift when the errorRecovery was specified (for
			 * compatibility reasons).  Joy.
			 */
			if (!options.asFields.errorRecovery)
			{
				options.asFields.errorRecovery =	cd->cd_DefaultOptions.asFields.errorRecovery;
				options.asFields.retryShift =		cd->cd_DefaultOptions.asFields.retryShift;
			}
		
			/* convert binary MSF to block offset */
			if (options.asFields.addressFormat == CDROM_Address_Abs_MSF)
			{
				theReq->io_Info.ioi_Offset = BinMSF2Offset(theReq->io_Info.ioi_Offset);
				options.asFields.addressFormat = CDROM_Address_Blocks;
			}
			
			/* save any defaults back into CmdOptions */
			theReq->io_Info.ioi_CmdOptions = options.asLongword;
			DBUG(kPrintCmdOptions, ("LCCD:  ioi_CmdOptions=%08lx\n", theReq->io_Info.ioi_CmdOptions));
			
			/* init these during first pass thru Read() */
			blockLen = options.asFields.blockLength;
			sectorFormat = MKE2BOB(cd, options.asFields.densityCode);

			cd->cd_CurRetryCount = (1 << options.asFields.retryShift) - 1;
			cd->cd_SavedRecvPtr = (uint8 *)theReq->io_Info.ioi_Recv.iob_Buffer;

			/* original length of user's buffer */
			bufLen = theReq->io_Info.ioi_Recv.iob_Len;

			/* offset of the next sector to transfer, and number of sectors */
			cd->cd_SectorOffset = theReq->io_Info.ioi_Offset;
			cd->cd_SectorsRemaining = bufLen / blockLen;

			/* range check the incoming block address based on TOC */
			if (cd->cd_SectorOffset > (BinMSF2Offset(((uint32)cd->cd_DiscInfo.minutes << 16) | 
										((uint32)cd->cd_DiscInfo.seconds << 8) | 
										((uint32)cd->cd_DiscInfo.frames))))
			{
				DBUG(kPrintWarnings, ("LCCD:  >WARNING<  Block address (%08lx) out-of-range (discEnd=%08lx)...aborting request\n", Offset2BCDMSF(cd->cd_SectorOffset), 
						Offset2BCDMSF(BinMSF2Offset(((uint32)cd->cd_DiscInfo.minutes << 16) | 
													((uint32)cd->cd_DiscInfo.seconds << 8) | 
													((uint32)cd->cd_DiscInfo.frames)))));
				return (kCDErrEndOfMedium);
			}

			/* make sure that bufLen is a multiple of blockLen
			 * also, make sure blkLen specified is cool for this format
			 * also, make sure sectorFormat jives with the track they're reading
			 */
			if ((bufLen != (cd->cd_SectorsRemaining * blockLen)) ||
				!ValidBlockLengthFormat(sectorFormat, blockLen) ||
				DataFormatDontJive(cd, Offset2BCDMSF(cd->cd_SectorOffset), sectorFormat))
			{
				DBUG(kPrintWarnings, ("LCCD:  >WARNING<  Bad ioRequest buffer size (%08lx), blockLen (%08lx), or sectorFormat (%02x)...aborting request\n", bufLen, blockLen, sectorFormat));
				return (kCDErrBadArg);
			}

			DBUG(kPrintQtyNPosOfSectorReqs, ("LCCD: %ld @ %06lx\n", cd->cd_SectorsRemaining, Offset2BCDMSF(cd->cd_SectorOffset)));
			theReq->io_Actual = 0;		/* initialize current transfer count */
			
			/* is the speed different?  if so, gotta pause */
			if (cd->cd_CurrentSettings.speed != options.asFields.speed)
			{
				cd->cd_DevState = RD_RESTART;
				goto ReadRestart;
			}
			cd->cd_DevState = RD_VARIABLE_PITCH;

		case RD_VARIABLE_PITCH:
			/* is the pitch different? */
			if (cd->cd_CurrentSettings.pitch != options.asFields.pitch)
			{
				/* bail, if we're currently in double speed */
				if (cd->cd_CurrentSettings.speed == CDROM_DOUBLE_SPEED)
					return (kCDErrBadArg);

				/* NOTE:  fine pitch always enabled for single speed */
				driveSpeed[1] = kFineEn | kSingleSpeed;
				switch (options.asFields.pitch)
				{
					case CDROM_PITCH_SLOW:		driveSpeed[2] = kNPct010;	break;		/* - 1% of normal */
					case CDROM_PITCH_NORMAL:	driveSpeed[2] = kPPct000;	break;		/* + 0% of normal */
					case CDROM_PITCH_FAST:		driveSpeed[2] = kPPct010;	break;		/* + 1% of normal */
				}
				err = SendCmdWithWaitState(cd, driveSpeed, sizeof(driveSpeed), CIPNone, NULL);
				Check4ErrorOrNeededSleep(err);
				DBUG(kPrintVariPitch,("RD_VARI_PITCH:  (%d) %02X %02X\n", options.asFields.pitch, driveSpeed[1], driveSpeed[2]));
				cd->cd_CurrentSettings.pitch = options.asFields.pitch;
			}
			cd->cd_DevState = RD_LOOP;

		case RD_LOOP:
loopDloop:
			DBUG(kPrintActionMachineStates, ("LCCD:  RD_LOOP (%ld sectors left)\n", cd->cd_SectorsRemaining));
						
			/* is this sector available in the prefetch buffer? */
			bufBlk = DataAvailNow(cd, cd->cd_SectorOffset, sectorFormat, options.asLongword);
			switch (bufBlk)
			{
				case kNoData:
					DBUG(kPrintDataAvailNowResponse, ("LCCD: kNoData (%08lx)\n", Offset2BCDMSF(cd->cd_SectorOffset)));
					if (DataAvailRSN(cd, cd->cd_SectorOffset, sectorFormat))
					{
						/* look for possible (seek) error */
						if (cd->cd_State & CD_DEVICE_ERROR)
						{
							cd->cd_State &= ~CD_DEVICE_ERROR;
							return (kCDErrDeviceError);
						}
						gHighWaterMark = MIN(gMaxHWM, cd->cd_SectorsRemaining);
						cd->cd_State |= CD_READ_IOREQ_BUSY;
						break;
					}
					DBUG(kPrintDataAvailNowResponse, ("LCCD: Not avail RSN...S/E = %08lx - %08lx\n", Offset2BCDMSF(cd->cd_PrefetchStartOffset), Offset2BCDMSF(cd->cd_PrefetchEndOffset)));
					cd->cd_State &= ~CD_READ_IOREQ_BUSY;
					cd->cd_DevState = RD_RESTART;
					break;
				case kBadData:
					DBUG(kPrintDataAvailNowResponse, ("LCCD: kBadData (%08lx)\n", Offset2BCDMSF(cd->cd_SectorOffset)));
					/* has the retry count expired? */
					if (!cd->cd_CurRetryCount)
					{
						/* if requested, return the data even if unrecoverable
						 * via ECC or via RETRIES, respectively
						 */
						if ((options.asFields.errorRecovery == CDROM_BEST_ATTEMPT_RECOVERY) ||
							(options.asFields.errorRecovery == CDROM_CIRC_RETRIES_ONLY))
						{
							CopySectorData(cd, (uint8 *)cd->cd_SavedRecvPtr,
								cd->cd_DataBlkHdr[bufBlk].buffer, blockLen, sectorFormat);

							/* mark block as avail to prefetch engine */
							cd->cd_DataBlkHdr[bufBlk].state = BUFFER_BLK_FREE;

							/* now that we have another free block available to
							 * prefetch into, we need to update the EndOffset
							 * so DataAvailRSN() works properly.  also, make
							 * sure we're still prefetching (so we don't screw
							 * up the DataAvailRSN test).
							 */
							if (cd->cd_State & CD_CURRENTLY_PREFETCHING)
								cd->cd_PrefetchEndOffset++;

							/* move read index to next VALID block */
							cd->cd_DataReadIndex = (bufBlk == (cd->cd_NumDataBlks-1)) ? 0 : (bufBlk + 1);

							/* update starting offset of prefetch space to 
							 * exclude this freed block
							 */
							cd->cd_PrefetchStartOffset++;
		
							/* increment user length count */
							theReq->io_Actual += blockLen;
						}
						else
						{
							DBUG2(kPrintWarnings, ("LCCD:  Retry count expired!  (sector %06lx)\n", Offset2BCDMSF(cd->cd_SectorOffset)));
						}
						
						/* complete this ioRequest */
						return (kCDErrMediaError);
					}
					else
						cd->cd_CurRetryCount--;
				case kNoSubcode:
					DBUG(kPrintDataAvailNowResponse, ("LCCD: kNoSubcode\n"));
					cd->cd_DevState = RD_RESTART;
					break;
				default:	
					CopySectorData(cd, (uint8 *)cd->cd_SavedRecvPtr, cd->cd_DataBlkHdr[bufBlk].buffer, blockLen, sectorFormat);
					
#if DEBUG_ECC
					DBUG(kPrintQtyNPosOfSectorReqs, ("LCCD:  buf %08x, chksum %08x\n", cd->cd_SavedRecvPtr, GenChecksum((uint32 *)cd->cd_SavedRecvPtr)));
#endif
					
					if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
					{
						uint8 subBlk = cd->cd_SubcodeReadIndex;

						/* mark blk as avail to subcode engine, update index */
						cd->cd_SubcodeBlkHdr[subBlk].state = BUFFER_BLK_FREE;
						cd->cd_SubcodeReadIndex = (subBlk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (subBlk + 1);
					}

					/* mark block as avail to prefetch engine */
					cd->cd_DataBlkHdr[bufBlk].state = BUFFER_BLK_FREE;

					/* now that we have another free block available to
					 * prefetch into, we need to update the EndOffset so 
					 * DataAvailRSN() works properly.  also, make sure we're
					 * still prefetching (so we don't screw up the
					 * DataAvailRSN test).
					 */
					if (cd->cd_State & CD_CURRENTLY_PREFETCHING)
						cd->cd_PrefetchEndOffset++;

					/* move read index to next VALID block, update index */
					cd->cd_DataReadIndex = (bufBlk == (cd->cd_NumDataBlks-1)) ? 0 : (bufBlk + 1);
					cd->cd_PrefetchStartOffset++;

					cd->cd_SavedRecvPtr += blockLen;	/* update local copy of recv buf ptr */
					theReq->io_Actual += blockLen;		/* increment user length count */
					cd->cd_SectorOffset++;				/* increment sector offset (to read next sector) */
					cd->cd_SectorsRemaining--;			/* update remaining sector count */

					/* reset retry count for next sector */
					cd->cd_CurRetryCount = (1 << options.asFields.retryShift) - 1;
	
					/* any sectors remaining? */
					if (!cd->cd_SectorsRemaining)
					{
						cd->cd_State &= ~CD_READ_IOREQ_BUSY;
						return (kCmdComplete);		/* return "cmd completed" */
					}
					
					goto loopDloop;						/* nasty; but fast */
			}

			/* if the data is gonna be avail RSN,
			 * ...break, and wait for it to show up
			 */
			if ((bufBlk == kNoData) && (cd->cd_DevState == RD_LOOP))
				break;

		case RD_RESTART:
ReadRestart:
			/* pause (close the data valve, stop any dma) */
			DBUG(kPrintActionMachineStates, ("LCCD:  RD_RESTART\n"));
			err = SendCmdWithWaitState(cd, pPso, sizeof(pPso), CIPPause, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_StatusByte |= SB_SPINUP;
			DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));
			cd->cd_DevState = RD_SPEED;
		case RD_SPEED:
			DBUG(kPrintActionMachineStates, ("LCCD:  RD_SPEED\n"));
			/* is the speed already set? */
			if (cd->cd_CurrentSettings.speed != options.asFields.speed)
			{
				switch (options.asFields.speed)
				{
					case CDROM_SINGLE_SPEED:
						/* single speed, fine pitch disabled... */
						driveSpeed[1] = kSingleSpeed;

						/* fine pitch always enabled for single-speed, audio */
						if (sectorFormat == DA_SECTOR)
							driveSpeed[1] |= kFineEn;
						cd->cd_StatusByte &= ~SB_DBLSPD;
						DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));
						break;
					case CDROM_DOUBLE_SPEED:		
						/* double speed, fine pitch disabled... */
						driveSpeed[1] = kDoubleSpeed;
						cd->cd_StatusByte |= SB_DBLSPD;
						DBUG(kPrintStatusByte, ("LCCD: Device status byte currently...0x%02x\n", cd->cd_StatusByte));
						break;
				}
				if ((driveSpeed[1] == kDoubleSpeed) || (sectorFormat != DA_SECTOR))
				{
					if (options.asFields.pitch != CDROM_PITCH_NORMAL)
						return (kCDErrBadArg);
				}
				else
				{
					switch (options.asFields.pitch)
					{
						case CDROM_PITCH_SLOW:		driveSpeed[2] = kNPct010;	break;		/* - 1% of normal */
						case CDROM_PITCH_NORMAL:	driveSpeed[2] = kPPct000;	break;		/* + 0% of normal */
						case CDROM_PITCH_FAST:		driveSpeed[2] = kPPct010;	break;		/* + 1% of normal */
					}
				}
	
				err = SendCmdWithWaitState(cd, driveSpeed, sizeof(driveSpeed), CIPNone, NULL);
				Check4ErrorOrNeededSleep(err);
				DBUG(kPrintVariPitch,("RD_SPEED:  (%d) %02X %02X\n", options.asFields.pitch, driveSpeed[1], driveSpeed[2]));
				cd->cd_CurrentSettings.speed = options.asFields.speed;
				cd->cd_CurrentSettings.pitch = options.asFields.pitch;
			}
			cd->cd_DevState = RD_FORMAT;
			
		case RD_FORMAT:
			DBUG(kPrintActionMachineStates, ("LCCD:  RD_FORMAT\n"));
			bobFormat[1] = cd->cd_PrefetchSectorFormat = sectorFormat;
			
			/* are they asking for subcode? */
			if (options.asFields.blockLength >= 2436)
			{
				cd->cd_State |= CD_PREFETCH_SUBCODE_ENABLED;
				bobFormat[2] = 0x01;
			}
			else
				cd->cd_State &= ~CD_PREFETCH_SUBCODE_ENABLED;

 			/* are the format, subcode already set? */
			if ((cd->cd_CurrentSettings.format != sectorFormat) ||
				(cd->cd_CurrentSettings.subcode != bobFormat[2]))
			{
				err = SendCmdWithWaitState(cd, bobFormat, sizeof(bobFormat), CIPNone, NULL);
				Check4ErrorOrNeededSleep(err);

				/* save current settings */
				cd->cd_CurrentSettings.format = sectorFormat;
				cd->cd_CurrentSettings.subcode = bobFormat[2];
			}
			cd->cd_DevState = RD_SEEK;
			
		case RD_SEEK:
			DBUG(kPrintActionMachineStates, ("LCCD:  RD_SEEK\n"));
			/* firmware wants binary frame # (not BCD...NOTE: seek2sector[0] 
			 * is safe because MSB of cd_SectorOffset is 0x00)
			 */
			*(uint32 *)seek2sector |= cd->cd_SectorOffset;
			
			err = SendCmdWithWaitState(cd, seek2sector, sizeof(seek2sector), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = RD_PREPARE;
			
		case RD_PREPARE:
			DBUG(kPrintActionMachineStates, ("LCCD:  RD_PREPARE\n"));
			/* reset the prefetch space */
			InitDataSpace(cd);

			/* reset the subcode space, if reading subcode too */
			if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
				InitSubcodeSpace(cd);

			/* Prefetch Engine now enabled */
			cd->cd_State |= (CD_CURRENTLY_PREFETCHING | CD_GONNA_HAVE_TO_STOP);
			cd->cd_PrefetchStartOffset = cd->cd_SectorOffset;

			/* NOTE:  There will be N-1 VALID blocks */
			cd->cd_PrefetchEndOffset = cd->cd_SectorOffset + cd->cd_NumDataBlks - 1;
			cd->cd_PrefetchCurMSF = Offset2BCDMSF(cd->cd_SectorOffset);

			/* audio or cd-rom?  (remember, minus 4 for dma len funkiness) */
			cd->cd_BlockLength = (sectorFormat == DA_SECTOR) ? 2348 : 2340;

			/* init blks, dma, etc. */
			InitPrefetchEngine(cd);

			gHighWaterMark = MIN(gMaxHWM, cd->cd_SectorsRemaining);
			cd->cd_State |= CD_READ_IOREQ_BUSY;
			
			cd->cd_DevState = RD_PLAY;
			
		case RD_PLAY:
			DBUG(kPrintActionMachineStates, ("LCCD:  RD_PLAY\n"));
			/* CIPNone, because we want to allow the CIPState response(s) to 
			 * wake the daemon up
			 */
			err = SendCmdWithWaitState(cd, Ppso, sizeof(Ppso), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = RD_LOOP;
			break;
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* ReadSubQ()                                                                   *
*                                                                              *
*      Purpose: Provides the state machine needed for CDROMCMD_READ_SUBQ reqs. *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device                               *
*                                                                              *
*      Returns: Err - if any occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err ReadSubQ(cdrom *cd)
{
	uint8	QRptNow[] = { CD_QREPORTEN, 0x01 };		/* get a QRpt immediately */
	IOReq	*theReq = cd->cd_workingIOR;
	int32	actual;
	Err		err;

#define READ_SUBQ_ZEROED_OUT 0
#if READ_SUBQ_ZEROED_OUT	
	/* return zero for now (only the audio app uses this; and then only to 
	 * determine if de-emphasis is enabled)
	 */
	actual = (int32) ((theReq->io_Info.ioi_Recv.iob_Len < sizeof(SubQInfo)) ?
		 	 theReq->io_Info.ioi_Recv.iob_Len : sizeof(SubQInfo));
	memset(theReq->io_Info.ioi_Recv.iob_Buffer, 0, sizeof(SubQInfo));
	theReq->io_Actual = actual;
	
	return (kCmdComplete);
#endif

	switch (cd->cd_DevState & ~kCmdStateMask)
	{
		case SQ_START:
			memcpy(&gSavedQCode, &cd->cd_LastQCode0, sizeof(SubQInfo));
			cd->cd_DevState = SQ_QNOW;
		case SQ_QNOW:
			/* get one Qcode */
			err = SendCmdWithWaitState(cd, QRptNow, sizeof(QRptNow), CIPNone, NULL);
			Check4ErrorOrNeededSleep(err);
			cd->cd_DevState = SQ_UNIQUE_QRPT;
		case SQ_UNIQUE_QRPT:
			/* wait until we get a NEW Qcode */
			if (memcmp((uint8 *)&gSavedQCode, (uint8 *)&cd->cd_LastQCode0, sizeof(SubQInfo)))
			{
				actual = (int32) ((theReq->io_Info.ioi_Recv.iob_Len < sizeof(SubQInfo)) ?
						  theReq->io_Info.ioi_Recv.iob_Len : sizeof(SubQInfo));
				memcpy(theReq->io_Info.ioi_Recv.iob_Buffer, &cd->cd_LastQCode0, actual);
				theReq->io_Actual = actual;
				
				return(kCmdComplete);			/* indicate cmd completed */
			}
			break;
	}
	return (kCmdNotYetComplete);
}

/*******************************************************************************
* SetDefaults()                                                                *
*                                                                              *
*      Purpose: Updates the defaults for the LCCD device based on input from   *
*               the client's ioRequest.                                        *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if any occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err SetDefaults(cdrom *cd)
{
	CDROMCommandOptions	options;
	IOReq				*theReq = cd->cd_workingIOR;
	Err					err;
	
	options.asLongword = theReq->io_Info.ioi_CmdOptions;
	
	err = VerifyCmdOptions(options.asLongword);
	if (err)
		return (err);

	/* set new defaults based on ioi_CmdOptions */
	if ((uint8)options.asFields.densityCode)		cd->cd_DefaultOptions.asFields.densityCode =	options.asFields.densityCode;
	if ((uint8)options.asFields.speed)				cd->cd_DefaultOptions.asFields.speed =			options.asFields.speed;
	if ((uint8)options.asFields.pitch)				cd->cd_DefaultOptions.asFields.pitch =			options.asFields.pitch;
	if ((uint32)options.asFields.blockLength)		cd->cd_DefaultOptions.asFields.blockLength =	options.asFields.blockLength;

	if ((uint8)options.asFields.errorRecovery)		/* new default setting */
	{
		cd->cd_DefaultOptions.asFields.errorRecovery =	options.asFields.errorRecovery;

		/* store whatever's there (ie, allow ZERO retries) */
		cd->cd_DefaultOptions.asFields.retryShift =		options.asFields.retryShift;
	}
	else if ((uint8)options.asFields.retryShift)
	{
		/* if they just wanna update the retry count only */
		cd->cd_DefaultOptions.asFields.retryShift =		options.asFields.retryShift;
	}

	/* NOTE:  There's a fault in the original CD-ROM driver API due to the 
	 *        fact that specifying zero for a CmdOption means "use the 
	 *        default".  But the CDROM_Address_Blocks is defined to be zero. 
	 *        So if someone was to set the default to something other than
	 *        CDROM_Address_Blocks; they could never set it back.  Therefore,
	 *        we must simply store whatever's there.
	 */
	cd->cd_DefaultOptions.asFields.addressFormat =	options.asFields.addressFormat;

	return (kCmdComplete);
}

/*******************************************************************************
* ResizeBufferSpace()                                                          *
*                                                                              *
*      Purpose: Checks to make sure that we're not prefetching, etc. before    *
*               calling ResizeLinkedBuffers() to reconfigure the prefetch      *
*               space to the desired mode.                                     *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: Err - if one occured.                                          *
*                                                                              *
*    Called by: ProcessClientIOReqs().                                         *
*                                                                              *
*******************************************************************************/
Err ResizeBufferSpace(cdrom *cd)
{
	if (cd->cd_State & CD_GONNA_HAVE_TO_STOP)
		return (kCmdNotYetComplete);

	return (ResizeLinkedBuffers(cd, cd->cd_workingIOR->io_Info.ioi_CmdOptions));
}



/*******************************************************************************
* VerifyCmdOptions()                                                           *
*                                                                              *
*      Purpose: Utility to verify that the incoming ioi_CmdOptions are valid.  *
*                                                                              *
*   Parameters: opts - the ioReq's ioi_CmdOptions                              *
*                                                                              *
*      Returns: Err - if any of the options are invalid.                       *
*                                                                              *
*    Called by: Read() and SetDefaults().                                      *
*                                                                              *
*******************************************************************************/
Err	VerifyCmdOptions(uint32 opts)
{
	CDROMCommandOptions options;
	
	options.asLongword = opts;

	if (((uint8)options.asFields.densityCode > CDROM_DIGITAL_AUDIO) ||
		((uint8)options.asFields.addressFormat > CDROM_Address_Abs_MSF) ||
		((uint8)options.asFields.errorRecovery > CDROM_BEST_ATTEMPT_RECOVERY) ||
		((uint8)options.asFields.speed > CDROM_DOUBLE_SPEED) ||
		((uint8)options.asFields.pitch > CDROM_PITCH_FAST))
	{
		return (kCDErrBadArg);
	}
	return (kCDNoErr);
}

/*******************************************************************************
* MKE2BOB()                                                                    *
*                                                                              *
*      Purpose: Converts an MKE 'density code' to its equiv. Bob 'format code'.*
*                                                                              *
*   Parameters: cd          - pointer to the cdrom device in question          *
*               densityCode - the MKE density code to convert                  *
*                                                                              *
*      Returns: uint8 - Bob format code                                        *
*                                                                              *
*    Called by: Read()                                                         *
*                                                                              *
*******************************************************************************/
uint8 MKE2BOB(cdrom *cd, int32 densityCode)
{
	switch (densityCode)
	{
		case CDROM_DEFAULT_DENSITY:		return ((cd->cd_DiscInfo.discID) ? XA_SECTOR : M1_SECTOR);
		case CDROM_DATA:				return (M1_SECTOR);
		case CDROM_MODE2_XA:			return (XA_SECTOR);
		case CDROM_DIGITAL_AUDIO:		return (DA_SECTOR);
		default:						return (INVALID_SECTOR);
	}
}

/*******************************************************************************
* ValidBlockLengthFormat()                                                     *
*                                                                              *
*      Purpose: Performs a sanity check to insure that the blockLength         *
*               specified in the ioReq's ioi_CmdOptions is valid for the       *
*               requested format.                                              *
*                                                                              *
*   Parameters: format - the Bob sector format specified in the ioReq          *
*               len    - the blockLength specified in the ioReq                *
*                                                                              *
*      Returns: uint32 - non-zero if the format is INVALID, zero otherwise.    *
*                                                                              *
*    Called by: Read().                                                        *
*                                                                              *
*******************************************************************************/
uint32 ValidBlockLengthFormat(uint8 format, int32 len)
{
	int8	valid = FALSE;
	
	switch (format)
	{
		case DA_SECTOR:
			switch (len)
			{
				case 2352:			/* data                                   */
				case 2353:			/* data + errorbyte (?)                   */
				case 2448:			/* data + subcode                         */
				case 2449:			/* data + subcode + errorbyte (?)         */
					valid = TRUE;
					break;
			}
			break;
		case M1_SECTOR:
			switch (len)
			{
				case 2048:			/* data                                   */
				case 2052:			/* hdr + data                             */
				case 2336:			/* data + aux/ecc                         */
				case 2340:			/* hdr + data + aux/ecc                   */
				case 2352:			/* sync + header + data + aux/ecc         */
				case 2436:			/* hdr + data + aux/ecc + subcode         */
				case 2440:			/* hdr + data + aux/ecc + cmpwrd + subcode*/
				case 2448:			/* sync + hdr + data + aux/ecc + subcode  */
					valid = TRUE;
					break;
			}
			break;
		case XA_SECTOR:
			switch (len)
			{
				case 2048:			/* Form1:   data                          */
				case 2060:			/* Form1:   header + data                 */
				case 2324:			/* Form2:   data                          */
				case 2328:			/* Form1/2: data + aux/ecc                */
				case 2336:			/* Form2:   hdr + data                    */
				case 2340:			/* Form1/2: hdr + data + aux/ecc          */
				case 2352:			/* Form1/2: sync + hdr + data + aux/ecc   */
				case 2436:			/* hdr + data + aux/ecc + subcode         */
				case 2440:			/* hdr + data + aux/ecc + cmpwrd + subcode*/
				case 2448:			/* sync + hdr + data + aux/ecc + subcode  */
					valid = TRUE;
					break;
			}
			break;
	}
	return (valid);
}

/*******************************************************************************
* DataFormatDontJive()                                                         *
*                                                                              *
*      Purpose: Performs a sanity check to insure that the data on the disc is *
*               being requested in the correct format.  IE, don't allow        *
*               reading audio in data, or vice-versa.                          *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               theMSF - the BCD MSF of the desired sector                     *
*               format - the Bob sector format specified in the ioReq (ie, the *
*                        attempted mode)                                       *
*                                                                              *
*      Returns: uint32 - non-zero if the format is INVALID, zero otherwise.    *
*                                                                              *
*    Called by: Read().                                                        *
*                                                                              *
*******************************************************************************/
uint32 DataFormatDontJive(cdrom *cd, uint32	theMSF, uint8 format)
{
	uint8	track = cd->cd_DiscInfo.lastTrackNumber;
	uint8	found = FALSE;
	uint32	curTrackStart;
	
	/* determine current track */
	do {

		/* calculate start of track 'x' */
		curTrackStart = ((uint32)BIN2BCD(cd->cd_TOC_Entry[track].minutes)<<16) +
						((uint32)BIN2BCD(cd->cd_TOC_Entry[track].seconds)<<8) +
						(uint32)BIN2BCD(cd->cd_TOC_Entry[track].frames);
			
		if (curTrackStart > theMSF)
			track--;
		else
			found = TRUE;
	} while (!found && (track >= cd->cd_DiscInfo.firstTrackNumber));
	
	/* this should always be true (as long as we _do_ perform range-checking 
	 * on incoming ioi_Offset values)
     */
	if (found)
	{
		if (format == DA_SECTOR)
		{
			if (cd->cd_TOC_Entry[track].addressAndControl & kDataTrackTOCEntry)
				return (TRUE);				/* data requested in wrong mode */
		}
		else
		{
			if (!(cd->cd_TOC_Entry[track].addressAndControl & kDataTrackTOCEntry))
				return (TRUE);				/* data requested in wrong mode */
		}
	}
	else
	{
		DBUG(kPrintWarnings, ("LCCD:  >WARNING<  Track match not found for sector %06lx\n", theMSF));
	}
	
	return (FALSE);							/* data requested in valid mode */
}

#ifdef DEVELOPMENT
	void PrintHdrCompWords(cdrom *cd, uint8 curBlk)
	{
		uint8	blk;
		uint8	curRead = cd->cd_DataReadIndex;
		uint8	curWrite = cd->cd_CurDataWriteIndex;
		uint8	nextWrite = cd->cd_NextDataWriteIndex;
		uint8	Bchr, Rchr, Cchr, Nchr;
	
		DBUG(kPrintDeath, ("LCCD:  HDR      HDR+1    CW-1     CW\n"));
		for(blk = 0; blk < cd->cd_NumDataBlks; blk++)
		{
			DBUG(kPrintDeath, ("LCCD:  %08lX %08lX ",
				*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[0],
				*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[4]));
			DBUG(kPrintDeath, ("%08lX %08lX  ",
				*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2336],
				*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2340]));
			Bchr = (blk == curBlk) ? 'B' : ' ';
			Rchr = (blk == curRead) ? 'R' : ' ';
			Cchr = (blk == curWrite) ? 'C' : ' ';
			Nchr = (blk == nextWrite) ? 'N' : ' ';
			DBUG(kPrintDeath, ("%c %c ", Bchr, Rchr));
			DBUG(kPrintDeath, ("%c %c\n", Cchr, Nchr));
		}
	}
#else
	#define PrintHdrCompWords(cd,curBlk)
#endif

/*******************************************************************************
* DataAvailNow()                                                               *
*                                                                              *
*      Purpose: Determines if the requested sector is currently available in   *
*               the prefetch space (in a block marked as VALID).  This is      *
*               accomplished by starting at the current beginning of the       *
*               prefetch space (DataReadIndex) and scanning forward until the  *
*               sector is found...or the prefetch buffer is completely         *
*               digested.  As the prefetch buffer is scanned, the unneeded     *
*               blocks are marked as FREE so they can be re-used by the        *
*               Prefetch Engine.                                               *
*                                                                              *
*        NOTE:  This means that if sectors 1,2,3,4,5 are available, and we get *
*               the following requests 1,2,5,3; sectors 1,2,5 will be returned *
*               immediately; but sector 3 will have to be re-fetched.  If the  *
*               requested sector is found, its block number is returned;       *
*               otherwise, we return -1.  This also has the effect of          *
*               freeing-up the entire prefetch buffer.  We also insure that if *
*               the data has been prefetched, that it was read using the same  *
*               format as is requested for that sector.  In the case where     *
*               subcode is also requested with audio sector data, we must have *
*               at least 4 (NUM_SYNCS_NEEDED_2_PASS+1) _valid_ subcode blocks  *
*               in order to return the block number (ie, the data is avail     *
*               now); otherwise, we return -1.                                 *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               sector - requested sector (in an absolute block number...not   *
*                        MSF)                                                  *
*               format - requested format of mode to read sector in            *
*                                                                              *
*      Returns: int8 - index of the prefetch buf blk which contains the sector *
*                                                                              *
*    Called by: Read()                                                         *
*                                                                              *
*******************************************************************************/
int8 DataAvailNow(cdrom *cd, int32 sector, uint8 format, uint32 opts)
{
	CDROMCommandOptions options;
	uint32	theMSFAddr;
	uint8	x;
	uint8	blk;
	uint8	subBlk, nextBlk;
	int32	numErrs;
	uint32	savedHeader;

	options.asLongword = opts;
	theMSFAddr = Offset2BCDMSF(sector);				/* MSF of sector we want */

	/* start with the current data, subcode blks */
	blk = cd->cd_DataReadIndex;
	subBlk = cd->cd_SubcodeReadIndex;

	/* continue until we run out of valid blocks */
	while(cd->cd_DataBlkHdr[blk].state == BUFFER_BLK_VALID)
	{
		/* does the sector read match the one we want? */
		if (cd->cd_DataBlkHdr[blk].MSF == theMSFAddr)
		{
			/* ...and does the format match what we want? */
			if (cd->cd_DataBlkHdr[blk].format == format)
			{
				/* ...and is subcode being requested for this sector? */
				if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
				{
					nextBlk = subBlk;
					/* "plus 1" because the sync has a 98% chance of falling
					 * in the middle of the buffer block which means we'll need
					 * one additional VALID block
					 */
					for (x = 0; x < (NUM_SYNCS_NEEDED_2_PASS+1); x++)
					{
						if (cd->cd_SubcodeBlkHdr[nextBlk].state != BUFFER_BLK_VALID)
						{
							if (cd->cd_SubcodeBlkHdr[nextBlk].state == BUFFER_BLK_BUCKET)
								return (kNoSubcode);	/* we will never have enough subcode because the buffer filled up (and we stopped prefetching) */
							else
								return (kNoData);		/* data not yet available due to lack of enough subcode */
						}
							
						nextBlk = (nextBlk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (nextBlk + 1);
					}
				}
				
				/* if we've gotten this far then we have enough subcode to
				 * return this sector
				 * note: upon returning, cd->cd_DataReadIndex = blk
 				 */
				if (format == DA_SECTOR)
					return (blk);					

				/* Verify header[31:8] == compword[31:8] for M1,XA sectors */
				if ((*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[0] ^
					*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2340]) & 0xFFFFFF00)
				{
					DBUG2(kPrintDeath, ("LCCD:  >DEATH<  Compword does NOT match header! (exp = %06lx)\n\n", theMSFAddr));
					PrintHdrCompWords(cd, blk);
					return (kBadData);
				}

				if (cd->cd_DataBlkHdr[blk].buffer[2343] & CRC_ERROR_BIT)
				{
					DBUG(kPrintECCStats, ("LCCD:  Must perform ECC on sector %06lx...", theMSFAddr));

					/* deal with mode 1 sectors */
					if (format == M1_SECTOR)
					{
						/* if requested, return the data even if unrecoverable 
						 * via retries (do not perform ECC in this case)
						 */
						if (options.asFields.errorRecovery == CDROM_CIRC_RETRIES_ONLY)
						{
							DBUG(kPrintECCStats,("CIRC_RETRIES_ONLY\n"));
							return (kBadData);
						}

#if DEBUG_ECC
						gECCTimer = 0xbad42ecc;
						gECCTimer = cd->cd_DataBlkHdr[blk].MSF;
#endif
						gECCTimer = SectorECC(cd->cd_DataBlkHdr[blk].buffer);
						numErrs = gECCTimer;
						DBUG(kPrintECCStats, ("%08lx fixed\n", numErrs));
						
						if (numErrs < 0)
							return (kBadData);
					}
					else						/* deal with mode 2 sectors */
					{
						/* is it Form2? */
						if (cd->cd_DataBlkHdr[blk].buffer[6] & 0x20)
						{
							/* is the optional EDC implemented? */
							if (*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2336])
							{
								/* return an error (no ECC to perform here) */
								DBUG2(kPrintWarnings, ("LCCD:  >ERROR<  Got CRC error for Form2 data on sector %06lx %08lx\n", 
									Offset2BCDMSF(cd->cd_SectorOffset), 
									*(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2336]));

								return (kBadData);
							}
							else
								return (blk);
						}
						else
						{
							/* if requested, return the data even if 
							 * unrecoverable via retries (do not perform ECC
							 * in this case)
							 */
							if (options.asFields.errorRecovery == CDROM_CIRC_RETRIES_ONLY)
								return (kBadData);
							
							savedHeader = *(uint32 *)cd->cd_DataBlkHdr[blk].buffer;
							*(uint32 *)cd->cd_DataBlkHdr[blk].buffer = 0x00000000;

#if DEBUG_ECC
							gECCTimer = 0xbad42ecc;
							gECCTimer = cd->cd_DataBlkHdr[blk].MSF;
#endif
							gECCTimer = SectorECC(cd->cd_DataBlkHdr[blk].buffer);
							numErrs = gECCTimer;

							*(uint32 *)cd->cd_DataBlkHdr[blk].buffer = savedHeader;
							DBUG(kPrintECCStats, ("%08lx fixed\n", numErrs));
							if (numErrs < 0)
								return (kBadData);
						}
					}
				}
#if VERIFY_CRC_CHECK
				else 
				{
					if (format == XA_SECTOR)
					{
						savedHeader = *(uint32 *)cd->cd_DataBlkHdr[blk].buffer;
						*(uint32 *)cd->cd_DataBlkHdr[blk].buffer = 0x00000000;
					}
					gECCTimer = 0xbad42ecc;
					gECCTimer = cd->cd_DataBlkHdr[blk].MSF;
					memcpy(gECCBuf, cd->cd_DataBlkHdr[blk].buffer, 2344);
					/* gECCTimer = SectorECC(cd->cd_DataBlkHdr[blk].buffer); */
					gECCTimer = SectorECC(gECCBuf);
					numErrs = gECCTimer;
flagLoop:
					DBUG(kPrintECCStats, ("%08x fixed\n", numErrs));
					if (format == XA_SECTOR)
						*(uint32 *)cd->cd_DataBlkHdr[blk].buffer = savedHeader;
					if (numErrs & 0x0000FFFF)		/* only look at the error count */
					{
						uint32 x;
						uint8 *buf = cd->cd_DataBlkHdr[blk].buffer;
						vuint32 flag;
						DBUGERR(("LCCD:  >ERROR<  CRC Failure @ %06x!  (ECC stat: %08x)\n", cd->cd_DataBlkHdr[blk].MSF, numErrs));
#if 1
						bufcmp(gECCBuf, cd->cd_DataBlkHdr[blk].buffer);
						for (x = 0; x < 2336; x += 16)
						{
							Superkprintf("%08x %08x ",
								*(uint32 *)&buf[x], *(uint32 *)&buf[x+4]);
							Superkprintf("%08x %08x\n",
								*(uint32 *)&buf[x+8], *(uint32 *)&buf[x+12]);
						}
						Superkprintf("%08x %08x\n", *(uint32 *)&buf[x], *(uint32 *)&buf[x+4]);
						Superkprintf("sector @ %08x\n", buf);
						Superkprintf("gECCbuf @ %08x\n", gECCBuf);
						Superkprintf("flag @ %08x\n", &flag);
						flag = 0xDEADC0DE;
						while (flag && (flag != 1))
							x++;
						memcpy(gECCBuf, buf, 2344);
						numErrs = SectorECC(gECCBuf);
						if (flag != 1)
							goto flagLoop;
#endif
					}
				}
#endif
				/* verify that the header/sector/etc is what we think it is */
				if (SanityCheckSector(cd, blk))
				{
					PrintHdrCompWords(cd, blk);
					return (kBadData);
				}

				/* note:  upon returning cd->cd_DataReadIndex = blk */
				return (blk);
			}
			else			/* data found in WRONG format, must re-read it */
				return (kBadData);
		}
		else
		{
			/* this is not the sector we're looking for, so free up this block 
			 * (ie, we won't allow people to read data that has been previously
			 * prefetched but read after) meaning...if we prefetch 1,2,3,4,5
			 * and get a request to return 4, we cannot go back and return 2
			 * without re-reading it.  tough nuegies...
			 */

			/* is subcode being requested for this sector? */
			if (cd->cd_State & CD_PREFETCH_SUBCODE_ENABLED)
			{
				/* update subcode index to start at next block */
				cd->cd_SubcodeReadIndex = (subBlk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (subBlk + 1);

				/* mark block as available to Prefetch Engine */
				cd->cd_SubcodeBlkHdr[subBlk].state = BUFFER_BLK_FREE;
				
				subBlk = cd->cd_SubcodeReadIndex;
			}
			
			/* update startOffset of pref space to exclude this freed block.
			 * ...mark update data index to start at next block.
			 * ...mark sector as prefetched but skipped over.
			 * ...mark block as available to Prefetch Engine.
			 */
			cd->cd_PrefetchStartOffset++;
			cd->cd_DataReadIndex = (blk == (cd->cd_NumDataBlks-1)) ? 0 : (blk + 1);
			cd->cd_DataBlkHdr[blk].MSF |= 0xFF000000;
			cd->cd_DataBlkHdr[blk].state = BUFFER_BLK_FREE;

			/* update the endOffset (last sector that will be prefetch-able) */
			if (cd->cd_State & CD_CURRENTLY_PREFETCHING)
				cd->cd_PrefetchEndOffset++;
						
			blk = cd->cd_DataReadIndex;
		}
	}

	/* requested sector not found in prefetch buffer */
	return (kNoData);
}

/*******************************************************************************
* DataAvailRSN()                                                               *
*                                                                              *
*      Purpose: Determines if the requested sector is going to be available if *
*               we simply allow the drive to prefetch it (ie, the data WILL be *
*               available before prefetch space fills up.  We also make sure   *
*               that the sector was prefetched in the correct mode (this is    *
*               necessary for discs that mix data/audio tracks.                *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               sector - requested sector (in an abs block number...not MSF)   *
*               format - requested format of mode to read sector in            *
*                                                                              *
*      Returns: int8 - TRUE if the data will be available RSN; FALSE otherwise *
*                                                                              *
*    Called by: Read()                                                         *
*                                                                              *
*******************************************************************************/
uint32 DataAvailRSN(cdrom *cd, int32 sector, uint8 format)
{
	/* does the sector we want fall in the range of what we're prefetching?
	 * if so, is it the same format that it will be prefetched in?
	 * and are we currently prefetching? (this was needed to support on-the-fly
	 * ResizeBuffers while we're still prefetching)
     */
	return ((cd->cd_PrefetchStartOffset <= sector) &&
			(cd->cd_PrefetchEndOffset >= sector) &&
			(cd->cd_PrefetchSectorFormat == format) &&
			(cd->cd_State & CD_CURRENTLY_PREFETCHING));
}

/*******************************************************************************
* SanityCheckSector()                                                          *
*                                                                              *
*      Purpose: Verify that the sector header (and Bob's completion word)      *
*               match the MSF that we expect to see.  In theory, the only time *
*               that they should differ is when bit errors occur in the header *
*               data.                                                          *
*                                                                              *
*   Parameters: cd  - pointer to the cdrom device in question                  *
*               blk - index into the data buffer for the sector in question    *
*                                                                              *
*      Returns: uint8 - TRUE if there is an unrecoverable error in the header  *
*                       (ie, the ECC was unable to correct it).                *
*                                                                              *
*    Called by: CopyDAData()                                                   *
*                                                                              *
*******************************************************************************/
uint32 SanityCheckSector(cdrom *cd, int8 blk)
{
	uint32	expected = cd->cd_DataBlkHdr[blk].MSF;
	uint32	header = *(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[0] >> 8;
	
	/* verify that the header MSF matches what we thought we read */
	if (expected != header)
	{
		DBUG2(kPrintDeath, ("LCCD:  >DEATH<  Header (%06lx) does not match expected (%06lx)! (c=%08lx)\n", 
			header, expected, *(uint32 *)&cd->cd_DataBlkHdr[blk].buffer[2340]));

		/* Do not return an error if it's a Mode2 sector.  This is due to the
		 * fact that data errors can occur in the header.  These are neither
		 * detectable, nor correctable.  Therefore, we must trust the firmware
		 * to return the correct sector (ie, land at the right place after a
		 * seek).
		 */
		if (cd->cd_DataBlkHdr[blk].format != XA_SECTOR)
			return (TRUE);
	}
	
	return (FALSE);
}

/*******************************************************************************
* CopySectorData()                                                             *
*                                                                              *
*      Purpose: Copies the requested data to the users buffer; taking into     *
*               account the specified blockLength and the lengths of the sync, *
*               header, data, aux/ecc, compword, and subcode.  NOTE:  Returns  *
*               zero for the MKE error byte, if requested.                     *
*                                                                              *
*   Parameters: cd     - pointer to the cdrom device in question               *
*               dst    - address of user's buffer                              *
*               src    - address in prefetch buffer to copy from               *
*               blkLen - block length as specified in ioi_CmdOptions.          *
*               format - the Bob sector format                                 *
*                                                                              *
*    Called by: Read().                                                        *
*                                                                              *
*******************************************************************************/
void CopySectorData(cdrom *cd, uint8 *dst, uint8 *src, int32 blkLen, uint8 format)
{
	uint8 syncMark[12] = {	0x00, 0xFF, 0xFF, 0xFF,
							0xFF, 0xFF, 0xFF, 0xFF,
							0xFF, 0xFF, 0xFF, 0x00 };	/* generic sync mark */
	uint32 compword = 0L;

	switch (blkLen)
	{
		case 2048:
			if (format == M1_SECTOR)
				memcpy(dst, src+4L, 2048);		/* M1:		D            */
			else
				memcpy(dst, src+12L, 2048);		/* M2F1:	D            */
			break;
		case 2052:
			memcpy(dst, src, 2052);				/* M1:		H+D          */
			break;
		case 2060:
			memcpy(dst, src, 2060);				/* M2F1:	H+D          */
			break;
		case 2324:
			memcpy(dst, src+12L, 2324);			/* M2F2:	D            */
			break;
		case 2328:
			memcpy(dst, src+12L, 2328);			/* M2F1(or2):	D+A      */
			break;
		case 2336:
			if (format == M1_SECTOR)
				memcpy(dst, src+4L, 2336);		/* M1:		D+A          */
			else
				memcpy(dst, src, 2336);			/* M2F2:	H(s)+D       */
			break;
		case 2340:
			memcpy(dst, src, 2340);				/* M1,M2F1,M2F2:	H+D+A */
			break;
		case 2449:
			*(uint8 *)(dst + 2448L) = 0;		/* old MKE error byte */
			/* no break intentional */
		case 2448:
			/* subcode for M1, M2F1, M2F2, and Digital Audio */
			CopySyncedSubcode(cd, dst + 2352L);
			/* no break intentional */
		case 2352:
		case 2353:
			if (format == DA_SECTOR)
			{
				memcpy(dst, src, 2352);				/* Digital Audio */
				if (blkLen == 2353)
					*(uint8 *)(dst + 2448L) = 0;	/* old MKE error byte */
			}
			else
			{
				/* M1,M2F1,M2F2:	sync + header + data + aux/ecc */
				memcpy(dst, syncMark, sizeof(syncMark));
				memcpy(dst+12L, src, 2340);
			}
			break;
		case 2440:
			compword = 4L;
			/* no break intentional */
		case 2436:
			/* M1,M1F1,M1F2:	hdr + data + aux/ecc (+ compword?) + subcode */
			memcpy(dst, src, 2340L + compword);
			CopySyncedSubcode(cd, dst + 2340L + compword);
			break;
	}
}



/*******************************************************************************
* SubcodeSyncedUp()                                                            *
*                                                                              *
*      Purpose: Determines if the subcode sync marks are currently "locked-in";*
*               and where the true start of the logical subcode blocks is.     *
*               The algorithm requires a minimum number of consecutive syncs   *
*               to be present (NUM_SYNCS_NEEDED_2_PASS) in order to decide     *
*               that the subcode is locked in.  In addition, there is a        *
*               maximum number of allowable consecutive missed syncs           *
*               (NUM_NOSYNCS_NEEDED_2_FAIL) before we decide that we've lost   *
*               sync.                                                          *
*                                                                              *
*   Parameters: cd - pointer to the cdrom device in question                   *
*                                                                              *
*      Returns: uint32 - non-zero if we're sync'd-up; zero otherwise           *
*                                                                              *
*    Called by: CopyDAData()                                                   *
*                                                                              *
*******************************************************************************/
uint32 SubcodeSyncedUp(cdrom *cd)
{
	uint8	blk;
	uint8	*syncPtr, *nextPtr;
	uint32	x;
	
	/* if we're not sync'd-up, try to get sync'd */
	if (!(cd->cd_State & CD_SUBCODE_SYNCED_UP))
	{
		/* start with the current "pseudo-block",
		 * start looking from beginning of 98-byte "pseudo-block"
		 * location just beyond this subcode buffer block
		 */
		blk = cd->cd_SubcodeReadIndex;
		syncPtr = cd->cd_SubcodeBlkHdr[blk].buffer;
		nextPtr = syncPtr + kSubcodeBlkSize;

		/* look for sync mark in this block */
		while(!(*syncPtr & SYNC_MARK) && (syncPtr < nextPtr))
			syncPtr++;
		
		/* have read into the next block without finding a sync mark? */
		if (syncPtr == nextPtr)
		{
			/* free up this block (so it can be used by the subcode engine)
			 * ...and wrap around to top of buffer if needed
			 */
			cd->cd_SubcodeBlkHdr[blk].state = BUFFER_BLK_FREE;
			cd->cd_SubcodeReadIndex = (blk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (blk + 1);
			
			/* sync not found in first block (need to return here so we 
			 * don't digest too much subcode and get out of sync)
			 */
			return (FALSE);
		}
		else
		{
			/* look for the first 1-to-0 transition of the P-bit */
			while (*syncPtr & SYNC_MARK)
				syncPtr++;

			/* backup one so we actually point to SyncByte2 (S1 of "S0,S1") */
			syncPtr--;
			
			/* at this point syncPtr points to byte 2 of a REAL 98-byte
			 * subcode packet (_potentially_)
			 */
			
			/* if this is true, then the "true start" of the subcode is in
			 * the next buffer block
			 */
			if (syncPtr >= nextPtr)
			{
				/* free up this blk (so it's avail to the subcode engine) */
				cd->cd_SubcodeBlkHdr[blk].state = BUFFER_BLK_FREE;
				cd->cd_SubcodeReadIndex = (blk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (blk + 1);
									
				/* sync not found in first block (need to return here so we
				 * don't digest too much subcode and get out of sync)
				 */
				return (FALSE);
			}
			
			/* start of true subcode block */
			cd->cd_SubcodeTrueStart = syncPtr;

			/* start looking one-block-out 
 			 * ...only look in blks we'd actually descramble the subcode from
			 * ...and bump nextPtr to next block each time
			 */
			for (x = 1, nextPtr = syncPtr + kSubcodeBlkSize;
				x < NUM_SYNCS_NEEDED_2_PASS;
				x++, nextPtr += kSubcodeBlkSize)
			{
				/* check to make sure to didn't loop in the buffer pool */
				if (nextPtr >= (cd->cd_SubcodeBlkHdr[cd->cd_NumSubcodeBlks-1].buffer + kSubcodeBlkSize))
					nextPtr -= cd->cd_NumSubcodeBlks*kSubcodeBlkSize;
				
				/* free up this block, and update the index */
				if (!(*nextPtr & SYNC_MARK))
				{
					cd->cd_SubcodeBlkHdr[blk].state = BUFFER_BLK_FREE;
					cd->cd_SubcodeReadIndex = (blk == (cd->cd_NumSubcodeBlks-1)) ? 0 : (blk + 1);

					/* subcode sync not locked-in yet */
					return (FALSE);
				}
			}
			
			/* clear the "missed sync mark" count */
			gSubcodeSyncMissCount = 0;

			/* apparently we detected enough syncs to pass */
			cd->cd_State |= CD_SUBCODE_SYNCED_UP;
		}
	}
	else							/* try to determine if we've lost sync */
	{
		/* start looking one-block-out
		 * ...only look in blks that we'd actually descramble the subcode from
		 * ...and bump nextPtr to next block each time
		 */
		for (x = 1, nextPtr = cd->cd_SubcodeTrueStart + kSubcodeBlkSize;
			x < NUM_SYNCS_NEEDED_2_PASS;
			x++, nextPtr += kSubcodeBlkSize)
		{
			/* check to make sure to didn't loop in the buffer pool */
			if (nextPtr >= (cd->cd_SubcodeBlkHdr[cd->cd_NumSubcodeBlks-1].buffer + kSubcodeBlkSize))
				nextPtr -= cd->cd_NumSubcodeBlks*kSubcodeBlkSize;
			
			/* did we miss a sync mark? */
			gSubcodeSyncMissCount = (*nextPtr & SYNC_MARK) ? 0 : (gSubcodeSyncMissCount + 1);
			
			/* did we meet/exceed the # of consecutive missed syncs? */
			if (gSubcodeSyncMissCount >= NUM_NOSYNCS_NEEDED_2_FAIL)
			{
				/* subcode is now assumed to be "out-of-sync" */
				cd->cd_State &= ~CD_SUBCODE_SYNCED_UP;
				return (FALSE);
			}
		}
		/* subcode is still synced up... */
	}
	
	return (cd->cd_State & CD_SUBCODE_SYNCED_UP);
}

/*******************************************************************************
* Subcode support...                                                           *
*                                                                              *
*   The #define's below cause the compiler to build of table of offsets used   *
*   for subcode descrambling.  The final table is SubcodeOffsetTable[], a      *
*   table of 96 bytes which is used to lookup descrambled entries within three *
*   sequential (scrambled) subcode blocks.                                     *
*                                                                              *
*   The actually descrambling occurs when the requested subcode is copied out  *
*   to the client's buffer within CopyDescrambledSubcode().                    *
*                                                                              *
*******************************************************************************/

#define SUBCODE_BLOCK_OFFSET(x)			( ((x) < 96) ? (x) : ( ((x)<(96+96)) ? (x)+2 : (x)+4 ))

#define PACKDELAY( Pack, Byte ) 		Byte+(24*(7-Pack))

#define ONEPACKBYTE( IP, Pack, Byte )   SUBCODE_BLOCK_OFFSET( 24 * IP + PACKDELAY( Pack, Byte ))

#define ONEFULLPACK( IP ) \
			ONEPACKBYTE( IP, 7, 0 ), \
			ONEPACKBYTE( IP, 5,18 ), \
			ONEPACKBYTE( IP, 2, 5 ), \
			ONEPACKBYTE( IP, 0,23 ), \
			ONEPACKBYTE( IP, 3, 4 ), \
			ONEPACKBYTE( IP, 5, 2 ), \
			ONEPACKBYTE( IP, 1, 6 ), \
			ONEPACKBYTE( IP, 0, 7 ), \
			ONEPACKBYTE( IP, 7, 8 ), \
			ONEPACKBYTE( IP, 6, 9 ), \
			ONEPACKBYTE( IP, 5,10 ), \
			ONEPACKBYTE( IP, 4,11 ), \
			ONEPACKBYTE( IP, 3,12 ), \
			ONEPACKBYTE( IP, 2,13 ), \
			ONEPACKBYTE( IP, 1,14 ), \
			ONEPACKBYTE( IP, 0,15 ), \
			ONEPACKBYTE( IP, 7,16), \
			ONEPACKBYTE( IP, 6,17), \
			ONEPACKBYTE( IP, 6, 1), \
			ONEPACKBYTE( IP, 4,19), \
			ONEPACKBYTE( IP, 3,20), \
			ONEPACKBYTE( IP, 2,21), \
			ONEPACKBYTE( IP, 1,22), \
			ONEPACKBYTE( IP, 4, 3)

static uint32 SubcodeOffsetTable[96] = { ONEFULLPACK(0), ONEFULLPACK(1), ONEFULLPACK(2), ONEFULLPACK(3) } ;

#define SubcodeBitsQRSTUVW	0x7F

/*******************************************************************************
* CopyDescrambledSubcode()                                                     *
*                                                                              *
*      Purpose: Utility routine to copy the descrambled subcode to a specified *
*               location in the user's buffer.                                 *
*                                                                              *
*   Parameters: cd  - pointer to cdrom device in question                      *
*               dst - address to copy 96 bytes of subcode to.                  *
*               src - start of current subcode area                            *
*                                                                              *
*    Called by: CopySyncedSubcode().                                           *
*                                                                              *
*******************************************************************************/
void CopyDescrambledSubcode(cdrom *cd, uint8 *dst, uint8 *src)
{
	uint8	*offset;
	int8	x;
	
	for (x = 0 ; x < 96 ; x++)
	{
		/* "plus one" to skip over sync byte */
		offset = src + SubcodeOffsetTable[x] + 1;
		
		/* check to make sure to didn't loop in the buffer pool */
		if (offset >= (cd->cd_SubcodeBlkHdr[cd->cd_NumSubcodeBlks-1].buffer + kSubcodeBlkSize))
			offset -= cd->cd_NumSubcodeBlks*kSubcodeBlkSize;
		
		dst[x] = SubcodeBitsQRSTUVW & *offset;
	}
}

/*******************************************************************************
* CopySyncedSubcode()                                                          *
*                                                                              *
*      Purpose: Utility routine to copy the synced-up (and descrambled)        *
*               subcode to a specified location in the user's buffer.  If the  *
*               subcode has not been synced-up (ie, locked-in), then we simply *
*               zero-fill the 96 bytes.                                        *
*                                                                              *
*   Parameters: cd  - pointer to cdrom device in question                      *
*               dst - address to copy 96 bytes of subcode to.                  *
*                                                                              *
*    Called by: CopyDAData(), CopySectorData().                                *
*                                                                              *
*******************************************************************************/
void CopySyncedSubcode(cdrom *cd, uint8 *dst)
{
	/* if we're locked-in and synchronized on subcode */
	if (SubcodeSyncedUp(cd))
	{
		/* SubcodeTrueStart gets set initially in SubcodeSyncedUp() */
		CopyDescrambledSubcode(cd, dst, cd->cd_SubcodeTrueStart);
		
		/* update start (2nd sync byte) of next subcode packet */
		cd->cd_SubcodeTrueStart += kSubcodeBlkSize;
		
		/* check to make sure to didn't loop in the buffer pool */
		if (cd->cd_SubcodeTrueStart >= (cd->cd_SubcodeBlkHdr[cd->cd_NumSubcodeBlks-1].buffer + kSubcodeBlkSize))
			cd->cd_SubcodeTrueStart -= cd->cd_NumSubcodeBlks*kSubcodeBlkSize;
	}
	else									/* clear all 96 bytes of subcode */
		memset(dst, 0, 96);
}



/*******************************************************************************
* EnableLCCDDMA()                                                              *
*                                                                              *
*      Purpose: Enables DMA channel(s) 0 and/or 1 for the 'current' and/or     *
*               'next' reg's.                                                  *
*                                                                              *
*   Parameters: channels  - which DMA channel(s) to affect?                    *
*               curORnext - which buf/len register pair(s) to affect?          *
*                                                                              *
*    Called by: InitPrefetchEngine()                                           *
*                                                                              *
*******************************************************************************/
void EnableLCCDDMA(uint8 channels)
{
	uint32	curBits;

	/* reset host by clearing the CDDMAEN bit (this also soft resets the LCCD
	 * interface logic within Anvil)
	 */
	curBits = AnvilFeatureReg & ~MADAM_CDDMA;
	AnvilFeatureReg = curBits;
	
	/* enable the host DMA channel(s) first... */
	if (channels & DMA_CH0)
	{
		/* clear the interrupt in case one snuck thru, and is now pending
		 * ...then enable the cd0 FIRQ handler
		 * ...and then the Anvil cd0 DMA channel
		 */
		ClearInterrupt(INT_CD0);
		EnableInterrupt(INT_CD0);
		*(uint32 *)DMAREQEN = EN_CD0toDMA;
	}
	if (channels & DMA_CH1)
	{
		ClearInterrupt(INT_CD1);
		EnableInterrupt(INT_CD1);
		*(uint32 *)DMAREQEN = EN_CD1toDMA;
	}

	/* ...then enable the CD DMA hardware */
	curBits = AnvilFeatureReg | MADAM_CDDMA;
	AnvilFeatureReg = curBits;
}

/*******************************************************************************
* DisableLCCDDMA()                                                             *
*                                                                              *
*      Purpose: Disables DMA channel(s) 0 and/or 1 for the 'current' and/or    *
*               'next' reg's.                                                  *
*                                                                              *
*   Parameters: channels  - which DMA channel(s) to affect?                    *
*               curORnext - which buf/len register pair(s) to affect?          *
*                                                                              *
*    Called by: LCCDDeviceInit(), LCCDDriverDaemon(), StopPrefetching(),       *
*               RecoverFromDipir(), BuildDiscData(), ChannelZeroFIRQ().        *
*                                                                              *
*******************************************************************************/
void DisableLCCDDMA(uint8 channels)
{
	uint32	curBits;

	/* disable the CD DMA hardware first... */
	curBits = AnvilFeatureReg & ~MADAM_CDDMA;
	AnvilFeatureReg = curBits;

	/* ...then disable the host DMA channel(s) */
	if (channels & DMA_CH0)
	{
		/* disable Anvil cd0 DMA channel
		 * ...then disable the cd0 FIRQ handler
		 */
		*(uint32 *)DMAREQDIS = EN_CD0toDMA;
		DisableInterrupt(INT_CD0);
	}
	if (channels & DMA_CH1)
	{
		*(uint32 *)DMAREQDIS = EN_CD1toDMA;
		DisableInterrupt(INT_CD1);
	}
}

/*******************************************************************************
* ConfigLCCDDMA()                                                              *
*                                                                              *
*      Purpose: Configures the 'current' or 'next' DMA registers for channel 0 *
*               or 1 with the specified buffer pointer and length.             *
*                                                                              *
*   Parameters: channel   - which DMA channel?                                 *
*               curORnext - which buf/len register pair?                       *
*               dst       - pointer to input buffer                            *
*               len       - length of input DMA transaction/buffer             *
*                                                                              *
*    Called by: InitPrefetchEngine(), ChannelZeroFIRQ(), ChannelOneFIRQ()      *
*                                                                              *
*******************************************************************************/
void ConfigLCCDDMA(uint8 channel, uint8 curORnext, uint8 *dst, int32 len)
{
	if (channel == DMA_CH0)
		if (curORnext == CUR_DMA)	
		{
			DMAZeroCurPtr = (uint32)dst;
			DMAZeroCurLen = len;
		}
		else
		{
			DMAZeroNextPtr = (uint32)dst;
			DMAZeroNextLen = len;
		}
	else
		if (curORnext == CUR_DMA)	
		{
			DMAOneCurPtr = (uint32)dst;
			DMAOneCurLen = len;
		}
		else
		{
			DMAOneNextPtr = (uint32)dst;
			DMAOneNextLen = len;
		}
}

/*******************************************************************************
* ChannelZeroFIRQ()                                                            *
*                                                                              *
*      Purpose: This routine provides the actual Prefetch Engine for sector    *
*               data.  Data is read off of the disc (when the driver envokes a *
*               Ppso) and this FIRQ routine handles all the prefetch buffer    *
*               block (and DMA register) handling needed to spool the data     *
*               'continuously' until the prefetch buffer pool fills up.        *
*                                                                              *
*    Called by: Called upon receiving a DMA EndOfLength interrupt for LCCD     *
*               channel 0.                                                     *
*                                                                              *
*******************************************************************************/
int32 ChannelZeroFIRQ(void)
{
	/* must cheat to get our device globals 
	 * (Note that this won't work for multiple drives)
	 */
	cdrom	*cd = gLCCDDevices[0];
	uint8	curBlk = cd->cd_CurDataWriteIndex;
	uint8	nextBlk = cd->cd_NextDataWriteIndex;
	uint8	crcError = 0;
	
	/* a "Ppso" happens (in the daemon), then...	
	 * an EOL interrupt occurs for the block pointed to by "curBlk"
	 */
	
	/* if these are the same, then we were out of buffer space the previous
	 * pass thru this FIRQ
	 */
	if (curBlk != nextBlk)
	{
		/* mark the previously filled sector buffer as valid */
		cd->cd_DataBlkHdr[curBlk].state = BUFFER_BLK_VALID;
		
		/* update the block header's MSF and format fields */
		cd->cd_DataBlkHdr[curBlk].MSF = cd->cd_PrefetchCurMSF;
		cd->cd_DataBlkHdr[curBlk].format = cd->cd_PrefetchSectorFormat;
		
		/* update the MSF 'counter' */
		cd->cd_PrefetchCurMSF = Offset2BCDMSF(BCDMSF2Offset(cd->cd_PrefetchCurMSF) + 1);
		
		/* if we're below the high water mark, check to see if this sector is
		 * errored...if so, wake up the daemon early
		 */
		crcError = (cd->cd_DataBlkHdr[curBlk].buffer[2343] & CRC_ERROR_BIT);
	
		/* set curBlk, so that the next time we're in here, we point to 
		 * the right block
		 */
		curBlk = cd->cd_CurDataWriteIndex = nextBlk;

		/* point to 'next' data buffer block */
		nextBlk = cd->cd_NextDataWriteIndex = (nextBlk == (cd->cd_NumDataBlks-1)) ? 0 : (nextBlk + 1);
		
		/* is the next block available? */
		if (cd->cd_DataBlkHdr[nextBlk].state == BUFFER_BLK_FREE)
		{
			/* mark it as used, and re-direct the dma ptrs to this blk */
			cd->cd_DataBlkHdr[nextBlk].state = BUFFER_BLK_INUSE;
			ConfigLCCDDMA(DMA_CH0, NEXT_DMA, cd->cd_DataBlkHdr[nextBlk].buffer, cd->cd_BlockLength);
		}
		else
		{
			/* since we adjusted nextBlk in order to test for a free block, 
			 * we need to...point nextBlk back to the current block to "slip
			 * the bucket under the waterfall"
			 */
			nextBlk = cd->cd_NextDataWriteIndex = curBlk;
			
			/* update the MSF 'counter'
			 * NOTE:  This gets used in StopPrefetching() to pre-seek to the
			 *        next sector (in the case of sequential reads)
			 */
			cd->cd_PrefetchCurMSF = Offset2BCDMSF(BCDMSF2Offset(cd->cd_PrefetchCurMSF) + 1);
	
			/* HALT DAMMIT!  NOTE: This will cause a dma overflow, ignore it */
			ConfigLCCDDMA(DMA_CH0, NEXT_DMA, NULL, 0);

			/* set the overrun flag, reset the prefetching flag */
			cd->cd_State |= CD_PREFETCH_OVERRUN;
			cd->cd_State &= ~CD_CURRENTLY_PREFETCHING;
			
			/* notify daemon that prefetch buffers have filled */
			if (gDaemon)
				SuperInternalSignal(gDaemon, gDaemonSignal);
			
			return (0);
		}
	}
	else
	{
		/* Since "bucket mode" is disabled, then we'll only make one more 
		 * pass thru this FIRQ after curBlk == nextBlk...since we cleared
		 * out NEXT_DMA ptr/len when that occured.  The Cur/Next
		 * DataWriteIndeces get updated to point to the first 2 blocks that
		 * will be freed-up upon _any_ data digestion.  
		 *
		 * Optionally, in addition to this, in the digest code, upon detecting
		 * that there are 2 free blocks, we mark the cur/next blocks to 
		 * INUSE, InitPrefetchEngine(), and then restart the engine.
		 */

		/* disable channel zero dma now that we've locked in the last buf blk */
		DisableLCCDDMA(DMA_CH0);

		/* mark the last sector block in the prefetch buffer as valid */
		cd->cd_DataBlkHdr[curBlk].state = BUFFER_BLK_VALID;

		/* update the block header's MSF and format fields */
		cd->cd_DataBlkHdr[curBlk].MSF = Offset2BCDMSF(BCDMSF2Offset(cd->cd_PrefetchCurMSF) - 1);
		cd->cd_DataBlkHdr[curBlk].format = cd->cd_PrefetchSectorFormat;
		
		/* update cur/next DataWriteIndeces */
		/* ...first block containing valid data (first to be digested) */
		cd->cd_CurDataWriteIndex = (curBlk == (cd->cd_NumDataBlks-1)) ? 0 : (curBlk + 1);

		/* point to 'next' data buffer block (ie, the new CurWriteIndex + 1) */
		cd->cd_NextDataWriteIndex = (cd->cd_CurDataWriteIndex == (MAX_NUM_DATA_BLKS-1)) ? 0 : (cd->cd_CurDataWriteIndex + 1);
	}
	
	gNumDataBlksInUse = (cd->cd_DataReadIndex < cd->cd_CurDataWriteIndex) ? 
						((uint32)cd->cd_CurDataWriteIndex - (uint32)cd->cd_DataReadIndex) :
						(((uint32)cd->cd_CurDataWriteIndex + (uint32)cd->cd_NumDataBlks) - (uint32)cd->cd_DataReadIndex);
	
	if (gDaemon && 
		((gNumDataBlksInUse > gHighWaterMark) || 
		(crcError && (gNumDataBlksInUse <= gHighWaterMark))) && 
		(cd->cd_State & CD_READ_IOREQ_BUSY))
	{
		/* notify daemon that we need to copy back data (or an error occured) */
		SuperInternalSignal(gDaemon, gDaemonSignal);
	}

	return (0);
}

/*******************************************************************************
* ChannelOneFIRQ()                                                             *
*                                                                              *
*      Purpose: This routine provides the actual Subcode Engine for sector     *
*               subcode data; and handles all the subcode buffer block (and    *
*               DMA register) handling needed to spool the subcode info        *
*               'continuously' until the subcode buffer fills up.              *
*                                                                              *
*               NOTE:  The Subcode Engine gets stopped by the same mechanism   *
*                      that stops the Prefetch Engine; and the Subcode Engine  *
*                      can "overflow" into the BUCKET block to its heart's     *
*                      content until a stop/pause/etc. is issued to close the  *
*                      valve (w/no problems).                                  *
*                                                                              *
*    Called by: Called upon receiving a DMA EndOfLength interrupt for LCCD     *
*               channel 1.                                                     *
*                                                                              *
*******************************************************************************/
int32 ChannelOneFIRQ(void)
{
	/* must cheat to get our device globals 
	 * (Note that this won't work for multiple drives)
	 */
	cdrom	*cd = gLCCDDevices[0];
	uint8	curBlk = cd->cd_CurSubcodeWriteIndex;
	uint8	nextBlk = cd->cd_NextSubcodeWriteIndex;
	
	/* a "Ppso" happens (in the daemon), then...	
	 * an EOL interrupt occurs for the block pointed to by "curBlk"
	 */

	/* if these are the same, then we were out of buffer space the
	 * previous pass thru this FIRQ
	 */
	if (curBlk != nextBlk)
	{
		/* mark the previously filled sector buffer as valid */
		cd->cd_SubcodeBlkHdr[curBlk*2].state = BUFFER_BLK_VALID;
		cd->cd_SubcodeBlkHdr[(curBlk*2)+1].state = BUFFER_BLK_VALID;
		
		/* set curBlk, so that the next time we're in here, we point to 
		 * the right block
		 */
		curBlk = cd->cd_CurSubcodeWriteIndex = nextBlk;

		/* point to 'next' subcode buffer block */
		nextBlk = cd->cd_NextSubcodeWriteIndex = (nextBlk == ((cd->cd_NumSubcodeBlks/2)-1)) ? 0 : (nextBlk + 1);
		
		/* is the next block available? */
		if ((cd->cd_SubcodeBlkHdr[nextBlk*2].state == BUFFER_BLK_FREE) &&
			(cd->cd_SubcodeBlkHdr[(nextBlk*2)+1].state == BUFFER_BLK_FREE))
		{
			/* mark the next pair as in-use */
			cd->cd_SubcodeBlkHdr[nextBlk*2].state = BUFFER_BLK_INUSE;
			cd->cd_SubcodeBlkHdr[(nextBlk*2)+1].state = BUFFER_BLK_INUSE;
			
			/* re-direct the dma ptrs to this block
			 * note that we use lengths of 192 (196-4) because the DMA h/w
			 * requires lengths/ptrs to be multiples of 4 ...so we cram 2
			 * logical buffer blocks (98 bytes each) into 1 physical block
			 */
			ConfigLCCDDMA(DMA_CH1, NEXT_DMA, cd->cd_SubcodeBuffer[nextBlk], 192);
		}
		else
		{
			/* since we adjusted nextBlk in order to test for a free block,
			 * we need to...point nextBlk back to the current block to "slip
			 * the bucket under the waterfall"
			 */
			nextBlk = cd->cd_NextSubcodeWriteIndex = curBlk;
			
			/* indicate that the bucket has been used */
			cd->cd_SubcodeBlkHdr[nextBlk*2].state = BUFFER_BLK_BUCKET;
			cd->cd_SubcodeBlkHdr[(nextBlk*2)+1].state = BUFFER_BLK_BUCKET;
			
			/* next block unavailable
			 * leave nextDMA as is (this will cause any incoming DMA to
			 * loop around to the same buffer).  Anvil leaves the NextPtr/Len
			 * alone when copying them to the CurPtr/Len.
			 */
		}
	}
	else
	{
		/* we don't need to do anything here, do we? */
	}
	
	return (0);
}

#if VERIFY_CRC_CHECK
uint32 bufcmp(uint8 *buf1, uint8 *buf2)
{
	uint32	p = 0;
	uint32	q = 0;
	uint32	qx;
	uint32	row, col;
	uint32	err = 0;
	
	Superkprintf("P  0.........1.........2.........3.........4.........5.........6.........7.........8.....\n");
	Superkprintf("   01234567890123456789012345678901234567890123456789012345678901234567890123456789012345\n");
	
	for (row = 0; row < 24; row++)
	{
		Superkprintf("%2ld ", row);
		for (col = 0; col < 86; col++)
		{
			if (buf1[p] == buf2[p])
				Superkprintf(".");
			else
				Superkprintf("*");
			p++;
		}
		Superkprintf("\n");
	}

	Superkprintf("   +---------+---------+---------+---------+---------+---------+---------+---------+-----\n");

	for (; row < 26; row++)
	{
		Superkprintf("%2ld ", row);
		for (col = 0; col < 86; col++)
		{
			if (buf1[p] == buf2[p])
				Superkprintf(".");
			else
				Superkprintf("*");
			p++;
		}
		Superkprintf("\n");
	}
	
	
	Superkprintf("\n\nQ  0.........1.........2.........3.........4.........5.........6.........7.........8..... ....\n");
	Superkprintf("   01234567890123456789012345678901234567890123456789012345678901234567890123456789012345 6789\n");
	
	for (row = 0; row < 26; row++)
	{
		q = 86*row;
		Superkprintf("%2ld ", row);
		for (col = 0; col < 86; col += 2)	/* columns done in pairs */
		{
			if (buf1[q] == buf2[q])			/* The even columns */
				Superkprintf(".");
			else
				Superkprintf("*");
			q++;
			if (buf1[q] == buf2[q])			/* The odd columns */
				Superkprintf(".");
			else
				Superkprintf("*");
			q += 87;						/* bump to next even col index */
			if (q >= 2236)					/* did we wrap? */
				q -= 2236;
		}
		if ((row/10)*10 == row)
			Superkprintf("+");
		else
			Superkprintf("|");
		for (; col < 90; col++)
		{
			qx = (col>>1)*52 + (col & 0x01) + (row << 1);
			if (buf1[qx] == buf2[qx])
				Superkprintf(".");
			else
			{
				Superkprintf("*");
				err++;
			}
		}
		Superkprintf("\n");
	}
	return (err);
}

uint32 GenChecksum(uint32 *buf)
{
	int32	x;
	uint32	checksum = 0;
	
	for (x = 0; x < 512; x++)
		checksum ^= buf[x];
		
	return (checksum);
}
#endif

