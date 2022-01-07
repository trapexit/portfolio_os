/*
	File:		FMVDriver.c

	Contains:	xxx put contents here xxx

	Written by:	George Mitsuoka

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<35>	  1/3/95	GM		removed unused functions/includes. changed return value of
									CreateFMVDriver to return the video device item to help support
									demand loading.
		<34>	12/13/94	DM		Add #define DEBUG around the kprintf stuff so when ROM version
									is built, it doesn't include the strings.
		<33>	 12/6/94	GM		Removed #include "L64111.h"
		<32>	 12/2/94	GDW		Fixed len bug in status call.
		<31>	11/17/94	GM		Added setjmp mechanism for handling non-existent hardware. Fixed
									error handling.
		<30>	10/27/94	DM		Changed the context of the forage calls  to be local to the open
									device rather than from the driver init.  Created SetupForage
									and CleanupForage in FMVHWAccessUtility.c.
		<29>	10/21/94	DM		Add support for whether forage is present in Set Top Boxes and
									pass that info on to other routines.
		<28>	10/18/94	DM		Add setup for forage so we can determine which version of the
									MPEG video decoder chip we have.
		<27>	10/13/94	DM		Add call to open Audio Folio, so the MPEG Audio driver can
									adjust the audio folio when there is a sampling frequency rate
									change.
		<26>	 9/12/94	DM		Added routine in driver init to determine what type of setup
									we're in (dev station or set top terminal) and added call to
									open up the forage device driver if we're on a stt.
		<25>	  9/8/94	DM		Renamed the gGrafBase to GrafBase because the graphics folio
									expects to find it by that name.
		<24>	 8/26/94	DM		Cleaned up some warning messages
		<23>	 8/24/94	DM		Added timer io stuff in driver init
		<22>	 8/15/94	DM		Add graphics folio call so we can control vbl and
									setvblattributes
		<21>	 5/25/94	GDW		Moved I2C driver open into Init code.
		<20>	 5/12/94	DM		Removed semicolons on device open flags and added check to
									device open conditions.
		<19>	 5/11/94	DM		Add condition build flag for Mia build.
		<18>	 3/27/94	GM		Removed reinitialization of Woody when both         devices are
									closed. That fixed uncovered yet another Woody bug.
		<17>	  3/7/94	GM		Added reinitialization of Woody when both devices are closed.
									This partially fixes a bug in Woody's aud and vid resets.
		<16>	 2/28/94	GDW		Removed old API.
		<15>	 2/25/94	GM		Added support for skipping frames and I frame search mode.
									Cleaned up code a little.
		<14>	 2/14/94	GM		Write buffers with length zero are now allowed. This will be
									interpreted as the beginning of a new stream by the driver.
		<13>	 2/10/94	GM		Modified KDBUG and DBUG macros to print "FMVDriver:
		<12>	  2/8/94	GDW		Moved CmdControl so it would be called.
		<11>	  2/7/94	GDW		Added Status and Control routine changes for new API.
		<10>	 12/3/93	GDW		Added interrupt code for PTS support.
		 <9>	 12/1/93	GDW		Conditionalized MIA code.
		 <8>	11/23/93	GM		Added more stringent checks on buffer alignment and sizes
		 <7>	11/23/93	GDW		Prints the value sent to SetSndDataFormat.
		 <6>	11/19/93	GDW		Add new sound stream functions
		 <5>	11/18/93	GM		
		 <4>	 11/9/93	GM		Remove some comments.
		 <3>	 11/5/93	GDW		
		 <2>	 11/5/93	GDW		Added returns to reduce warnings.

*/

/* file: FMVDriver.c */
/* Opera driver wrapper for FMV code */
/* 4/22/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifdef THINK_C
#define __swi(arg)
#endif

#include "types.h"
#include "item.h"
#include "device.h"
#include "debug.h"
#include "driver.h"
#include "io.h"
#include "interrupts.h"
#include "inthard.h"
#include "kernel.h"
#include "operror.h"
#include "super.h"
#include "string.h"

#include "FMV.h"
#include "FMVAudioDriver.h"
#include "FMVDriver.h"
#include "FMVROM.h"
#include "FMVVideoDriver.h"
#include "Hollywood.h"
#include "Woody.h"
#include "FMVUtilities.h"
#if MIABUILD == 1
#include "folio.h"
#include "audio.h"

#include "FMVAnalogDriver.h"
#include "FMVAnalogVideoDriver.h"
#include "FMVAnalogAudioDriver.h"
#include "I2CDriver.h"
#include "graphics.h"
#include "FMVHWAccessUtilities.h"
#endif

#ifdef THINK_C
#define KDBUG(x)
#define DBUG(x)
#else
#ifdef DEBUG
#define KDBUG(x)	{ kprintf("FMVDriver: "); kprintf x; }
#define DBUG(x)		{ Superkprintf("FMVDriver: "); Superkprintf x; }
#else
#define KDBUG(x)	/*	*/
#define DBUG(x)		/* */
#endif
#endif

#if MIABUILD == 1
#define SuperFindDevice(n)     SuperFindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),(n))
#define SuperOpenNamedDevice(x,a)	SuperOpenItem(SuperFindDevice(x),a)

Item		gI2CDeviceItem;				// I2C device driver
Item		gI2CWrCmdItem;				// I2C ioReq item
IOReq		*gI2CioReq;

Item		GrafBaseItem ;
GrafFolio 	*GrafBase;

Item	gAudioFolioItem ;

uint8	gStationType ;		// 1 = Dev Station 0 = Set Top Terminal
Item	gForageItem ;		// Forage Device Driver
Item	gForageCmdItem ;	// Forage IOReq Item
IOReq	*gForageIOReq ;		// Forage IOReq
Item	gForageRdCmdItem ;	// Forage Read IOReq Item
IOReq	*gForageRdIOReq ;	// Forage Read IOReq
bool	gForageDetected ;	// indicates whether forage has been detected
#endif

/* function prototypes */

static void FMVAbortIO( IOReq *ior );
static Item FMVDriverInit( Driver *drv );
static Item AudioDevInit( Device *dev );
static Item VideoDevInit( Device *dev );
#if MIABUILD == 1
static Item AnalogAudioDevInit( Device *dev );
static Item AnalogVideoDevInit( Device *dev );
#endif
static Item devOpen( Device *dev );
static int32 CmdRead( IOReq *ior );
static int32 CmdStatus( IOReq *ior );
static int32 CmdWrite( IOReq *ior );
static int32 CmdControl( IOReq *ior );


static Item audioDeviceItem = 0, videoDeviceItem = 0;

#if MIABUILD == 1
static Item analogAudioDeviceItem = 0, analogVideoDeviceItem = 0;
#endif

static void FMVAbortIO( IOReq *ior )
{
	DBUG(("FMVDevice: FMVAbortIO\n"));

	if( ior->io_Dev->dev.n_Item == audioDeviceItem )
		FMVAudioAbortIO( ior );
	else if( ior->io_Dev->dev.n_Item == videoDeviceItem )
		FMVVideoAbortIO( ior );
		
#if MIABUILD == 1
	else if( ior->io_Dev->dev.n_Item == analogVideoDeviceItem ) 
			FMVAnalogVideoAbortIO(ior) ;
	else if ( ior->io_Dev->dev.n_Item == analogAudioDeviceItem ) 
			FMVAnalogAudioAbortIO(ior) ;
#endif
}

static Item FMVDriverInit( Driver *drv )
{
#if MIABUILD == 1
extern Item gTimerItem;
extern Item gTimerIOItem;
extern IOReq *gTimerIOReq;
uint32	readData ;
extern Folio *AudioBase ;
extern int32 gREVAFixEnabled ;
Err	returnCode ;
#endif

	volatile uint32        *pointer;
	jmp_buf                 jb, *old_co;
	uint32			old_quiet;

	DBUG(("FMVDevice: FMVDriverInit\n"));

	/* make sure we've got some hardware */

	/* save kernel state */
	old_co = KernelBase->kb_CatchDataAborts;
	old_quiet = KernelBase->kb_QuietAborts;

	/* set up new state */
	KernelBase->kb_CatchDataAborts = &jb;
	KernelBase->kb_QuietAborts = ABT_CLIOT;

	/* jump to here if we can't access the hardware */
	if (setjmp (jb))
	{
		DBUG(("\tData Abort while initializing fmv driver\n"));

		/* restore the kernel state */
		KernelBase->kb_CatchDataAborts = old_co;
		KernelBase->kb_QuietAborts = old_quiet;
		return( MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceOffline) );
	}
	/* restore kernel state at the end of this routine */

#if MIABUILD == 1	
	if( (gI2CDeviceItem = SuperOpenNamedDevice( I2C_DEVICE_NAME, 0 )) < 0 ) {
		DBUG(("FMVDriverInit: Unable to open I2C driver\n"));
		goto abort;
	} else {
	gI2CWrCmdItem = SuperCreateIOReq(0,0,gI2CDeviceItem,0);			// Get the I2C device req
	gI2CioReq = (IOReq *) LookupItem(gI2CWrCmdItem);
	}

	GrafBaseItem =
		SuperOpenItem(SuperFindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),"Graphics"), 0);
	if (GrafBaseItem<0) {
		DBUG(("FMVDriverInit: Unable to open Graphics Folio\n"));
		goto abort;
	}	else {
		GrafBase = (GrafFolio *)LookupItem (GrafBaseItem);
		DEBUGP (("GrafBase located at %lx\n", (uint32)GrafBase));
	}

	if( (gTimerItem = SuperOpenNamedDevice( "timer", 0 )) < 0 ) {
		DBUG(("FMVDriverInit: Unable to open timer driver\n"));
		goto abort;
	} else {
		DBUG(("FMVDriverInit: Timer Device Item = $%lx\n",gTimerItem));
		gTimerIOItem = SuperCreateIOReq(0,0,gTimerItem,0);		// Get the timer device req
//		DBUG(("FMVDriverInit: Timer IO Item = $%lx\n",gTimerIOItem));
		gTimerIOReq = (IOReq *) LookupItem(gTimerIOItem);
//		DBUG(("FMVDriverInit: Timer IOReq  = $%lx\n",(uint32)gTimerIOReq));
	}
//
//	if we're in a STT, then GPIO 1 in Woody will be high.
//	if we're in a Network Dev Station, then GPIO 1 in Woody will be low.
//	
//	In the STT, we have to open the forage driver which will be used to adjust the
//	48kHz and 44.1kHz switching and to allow us to read which version of 
//	MPEG Video decoder we have installed.
//
	readData = WdyGetControlClear() ;
	gForageDetected = FALSE ;
	if((readData & WDYGPIO1) == 0)
	{
		gStationType = kDEVSTATION ;
		DBUG(("Detected Network Development Station \n"));
	}	else {
		gStationType = kSETTOPTERMINAL ;
		DBUG(("Detected Set Top Terminal \n"));
		SetupForage() ;
//
//	if the gREVAFixEnabled flag is set (true), then ignore the following determination code for the STT.
//	When it is true, then the REV A Fix will be enabled which overrides any other condition.  
//
		if(!gREVAFixEnabled)
		{
			
			returnCode = WhichMPEGVideoVersion() ;
			if(returnCode < 0)
				DBUG(("FMVDriverInit: MPEG Video Decoder Failed = %08lx \n", returnCode));
		} 
	}
	if(gREVAFixEnabled)
		DBUG(("FMVDriverInit: Enabling MPEG Video Decoder Rev A Fix \n"));
//
//	Setup the audio folio so when audio MPEG switches between 48kHz 44.1kHz streams, the appropriate
//	sample rate changes can occur in the audio folio to adjust the pitch.
//
	gAudioFolioItem =
		SuperOpenItem(SuperFindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),AUDIOFOLIONAME), 0);
	if (gAudioFolioItem<0) {
		DBUG(("FMVDriverInit: Unable to open Audio Folio\n"));
		goto abort;
	}	else {
		AudioBase = (Folio *)LookupItem (gAudioFolioItem);
		DEBUGP (("AudioBase located at %lx\n", (uint32)AudioBase));
	}

#endif

	/* disable board interrupt */
	DisableInterrupt( INT_PD );

	/* set up ROM accesses */
	FMVROMAccessInit();
	
	if( FMVWoodyInit() < 0 )
	{
		DBUG(("FMVWoodyInit Failed\n"));
		goto abort;
	}
	
#if MIABUILD == 1	
/*	Determine versions */
	if( FMVAnalogInit() < 0 )
	{
		DBUG(("MiaInit Failed\n"));
		goto abort;
	}

	if(	gStationType == kSETTOPTERMINAL)
		CleanupForage() ;
	
#endif

	/* enable board interrupt */
	EnableInterrupt( INT_PD );

	/* restore kernel state */
	KernelBase->kb_CatchDataAborts = old_co;
	KernelBase->kb_QuietAborts = old_quiet;

	return drv->drv.n_Item;

abort:
	/* restore kernel state */
	KernelBase->kb_CatchDataAborts = old_co;
	KernelBase->kb_QuietAborts = old_quiet;

	return( MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceError) );
}

//------------------------------------------------------------------------------

static Item AudioDevInit( Device *dev )
{
	DBUG(("FMVDevice: AudioDevInit\n"));

	if( FMVAudioInit() < 0 )
	{
		DBUG(("FMVAudioInit Failed\n"));
		return( MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceError) );
	}
	dev->dev_MaxUnitNum = 0;			/* one unit */

	return dev->dev.n_Item;
}

static Item VideoDevInit( Device *dev )
{
	DBUG(("FMVDevice: VideoDevInit\n"));

	if( FMVVideoInit() < 0 )
	{
		DBUG(("FMVVideoInit Failed\n"));
		return( MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceError) );
	}
	dev->dev_MaxUnitNum = 0;			/* one unit */

	return dev->dev.n_Item;
}
#if MIABUILD == 1
//-------------------------------------------------------------------------
// VideoDevInit
//-------------------------------------------------------------------------
static Item AnalogVideoDevInit( Device *dev )
{
	DBUG(("FMVDriver: AnalogVideoDevInit\n"));


/*	Initialize and setup the video section */

	if( FMVAnalogVideoInit() < 0 )
	{
		DBUG(("FMVAnalogVideoInit Failed\n"));
		return( -1 );
	}

	dev->dev_MaxUnitNum = 0;			/* one unit */
	return dev->dev.n_Item;
}
//-------------------------------------------------------------------------
// AudioDevInit
//-------------------------------------------------------------------------
static Item AnalogAudioDevInit( Device *dev )
{
	DBUG(("FMVDriver: AnalogAudioDevInit\n"));

	/* Initialize and setup the audio section */
	if( FMVAnalogAudioInit() < 0 )
	{
		DBUG(("FMVAnalogAudioInit Failed\n"));
		return( -1 );
	}

	dev->dev_MaxUnitNum = 0;			/* one unit */
	return dev->dev.n_Item;
}

#endif

static int32 openFlags = 0L;
#define AUDIODEVOPEN	0x1L
#define VIDEODEVOPEN	0x2L

#if MIABUILD == 1
#define ANALOGAUDIODEVOPEN	0x4L
#define ANALOGVIDEODEVOPEN	0x8L
#endif
		
static Item devOpen( Device *dev )
{
	DBUG(("FMVDevice: devOpen %ld\n",dev->dev_OpenCnt));
	
	if( dev->dev_OpenCnt > 0 )
		return( MakeKErr(ER_SEVERE,ER_C_STND,ER_NotSupported) );

	if( !openFlags )
	{
	}
	if( dev->dev.n_Item == audioDeviceItem )
	{
#if MIABUILD == 1
		if((openFlags & ANALOGAUDIODEVOPEN) > 0)
		{
			DBUG(("ERROR:  Analog Audio already open\n"));
			return -1 ;
		} else 
#endif
		{
			FMVAudioDevOpen( dev );
			openFlags |= AUDIODEVOPEN;
		} 
	}
	if( dev->dev.n_Item == videoDeviceItem )
	{
#if MIABUILD == 1
		if((openFlags & ANALOGVIDEODEVOPEN) > 0)
		{
			DBUG(("ERROR:  Analog Video already open\n"));
			return -1 ;
		} else 
#endif
		{
			FMVVideoDevOpen( dev );
			openFlags |= VIDEODEVOPEN;
		} 
	}
#if MIABUILD == 1
	if( dev->dev.n_Item == analogVideoDeviceItem )
	{
		if((openFlags & VIDEODEVOPEN) > 0)
		{
			DBUG(("ERROR:  MPEG Video already open\n"));
			return -1 ;
		} 	else {
			FMVAnalogVideoDevOpen( dev );
			openFlags |= ANALOGVIDEODEVOPEN;
		}
	}
	if( dev->dev.n_Item == analogAudioDeviceItem )
	{
		if((openFlags & AUDIODEVOPEN) > 0)
		{
			DBUG(("ERROR:  MPEG Audio already open\n"));
			return -1 ;
		} else {
			FMVAnalogAudioDevOpen( dev );
			openFlags |= ANALOGAUDIODEVOPEN;
		}
	}
#endif

	dev->dev_MaxUnitNum = 0;			/* one unit */
	return dev->dev.n_Item;
}

static void devClose( Device *dev )
{
	DBUG(("FMVDevice: devClose %ld\n",dev->dev_OpenCnt));
	
	if( dev->dev.n_Item == audioDeviceItem )
	{
		DBUG(("Closing audio device\n"));
		FMVAudioDevClose( dev );
		openFlags &= ~AUDIODEVOPEN;
	}
	else if( dev->dev.n_Item == videoDeviceItem )
	{
		DBUG(("Closing video device\n"));
		FMVVideoDevClose( dev );
		openFlags &= ~VIDEODEVOPEN;
	}
	
#if MIABUILD == 1
	else if( dev->dev.n_Item == analogVideoDeviceItem )
	{
		DBUG(("Closing analog video device\n"));
		FMVAnalogVideoDevClose( dev );
		openFlags &= ~ANALOGVIDEODEVOPEN;
	}
	if( dev->dev.n_Item == analogAudioDeviceItem )
	{
		DBUG(("Closing analog audio device\n"));
		FMVAnalogAudioDevClose( dev );
		openFlags &= ~ANALOGAUDIODEVOPEN;
	}
#endif

	if( !openFlags )
	{
	}
}

static int32 CheckOptions( IOReq *ior )
{
	FMVIOReqOptions *optionsPtr;

	optionsPtr = (FMVIOReqOptions *) ior->io_Info.ioi_CmdOptions;

	/* is there a FMVIOReqOptions struct? */
	if( !optionsPtr )
		return( 0L );

	/* verify alignment */
	if( (uint32) optionsPtr & 0x3L )
	{
		DBUG(("FMVDevice: CmdOptions not long aligned\n"));
		return( -1L );
	}
	/* verify addresses */
	if( !SuperIsRamAddr( (void *) optionsPtr, sizeof(FMVIOReqOptions) ) )
	{
		DBUG(("FMVDevice: CmdOptions doesn't point to FMVIOReqOptions\n"));
		return( -1L );
	}
	/* verify reserved fields */
    if( optionsPtr->Reserved1 | optionsPtr->Reserved2 | optionsPtr->Reserved3 )
	{
		DBUG(("FMVDevice: FMVIOReqOptions reserved fields non-zero\n"));
		return( -1L );
	}
	return( 0L );
}

//------------------------------------------------------------------------------

static int32 CmdRead( IOReq *ior )
{
	/* Command Read */
	uint32 *dst = (uint32 *)ior->io_Info.ioi_Recv.iob_Buffer;
	int32 len = ior->io_Info.ioi_Recv.iob_Len;

	/* check options */
	if( CheckOptions( ior ) )
	{
		DBUG(("FMVDevice: CmdRead: bad options\n"));
		goto abort;
	}
	/* make sure long word aligned */
	if ( ((int32) dst & 0x3L) || ( len & 0x3L ) )
	{
		DBUG(("FMVDevice: CmdRead dst = %08lx, len = %ld\n",(int32) dst,len));
		DBUG(("non-aligned read\n"));
		goto abort;
	}
	if (len < 4L)					/* don't do tiny io */
	{
		DBUG(("FMVDevice: CmdRead dst = %08lx, len = %ld\n",(int32) dst,len));
		DBUG(("bad length\n"));
		goto abort;
	}
	/* dispatch to appropriate device */
	if( ior->io_Dev->dev.n_Item == audioDeviceItem )
		return( FMVAudioCmdRead( ior ) );
	else if( ior->io_Dev->dev.n_Item == videoDeviceItem )
		return( FMVVideoCmdRead( ior ) );
		
#if MIABUILD == 1
	else if( ior->io_Dev->dev.n_Item == analogAudioDeviceItem )
		return( FMVAnalogAudioCmdRead( ior ) );
	else if( ior->io_Dev->dev.n_Item == analogVideoDeviceItem )
		return(FMVAnalogVideoCmdRead( ior ) );
#endif

abort:
	DBUG(("CmdRead aborting\n"));
	ior->io_Error = BADIOARG;
	return( 1L );
}

static int32 CmdControl( IOReq *ior )
{
	if( ior->io_Dev->dev.n_Item == audioDeviceItem )
		return( FMVAudioCmdControl( ior ) );
	else if( ior->io_Dev->dev.n_Item == videoDeviceItem )
		return( FMVVideoCmdControl( ior ) );
#if MIABUILD == 1
	else if( ior->io_Dev->dev.n_Item == analogAudioDeviceItem )
		return( FMVAnalogAudioCmdControl( ior ) );
	else if( ior->io_Dev->dev.n_Item == analogVideoDeviceItem )
		return( FMVAnalogVideoCmdControl( ior ) );
#endif

	return( 1L );
}

static int32 CmdStatus( IOReq *ior )
{
	CODECDeviceStatus	*dst;
	int32				len;
	
#if MIABUILD == 1
	DIGITIZERDeviceStatus	*dstDigitizer;
#endif

	len = ior->io_Info.ioi_Recv.iob_Len;	// Get the length of data structure
	dst = (CODECDeviceStatusPtr) ior->io_Info.ioi_Recv.iob_Buffer;

	/* check size, alignment */
	if( (len < 8) || ((uint32) dst & 0x3L) )
	{
		ior->io_Error = BADIOARG;
		return( 1L );
	}
	ior->io_Actual = len;				// Tell them amount returned
	dst->codec_ds.ds_DriverIdentity = DI_HOLLYWOOD;
	dst->codec_ds.ds_MaximumStatusSize = sizeof( CODECDeviceStatus );

	if( len < sizeof( CODECDeviceStatus ) )
	{
		return( 1L );
	}
	DBUG(("FMVDevice: CmdStatus dst=%lx len=%ld\n",(uint32)dst,len));
#if MIABUILD == 1
	dstDigitizer = (DIGITIZERDeviceStatusPtr) ior->io_Info.ioi_Recv.iob_Buffer;
#endif

	ior->io_Actual = len;				// Tell them amount returned

	dst->codec_ds.ds_DeviceFlagWord = DS_DEVTYPE_CODEC;

#if MIABUILD == 1
	dstDigitizer->digitizer_ds.ds_DriverIdentity = DI_HOLLYWOOD;
	dstDigitizer->digitizer_ds.ds_MaximumStatusSize = sizeof( DIGITIZERDeviceStatus );
	dstDigitizer->digitizer_ds.ds_DeviceFlagWord = DS_DEVTYPE_CODEC;  // DS_DEVTYPE_DIGITIZER not defined
#endif

	if( ior->io_Dev->dev.n_Item == audioDeviceItem )
		return( FMVAudioCmdStatus( ior ) );
	else if( ior->io_Dev->dev.n_Item == videoDeviceItem )
		return( FMVVideoCmdStatus( ior ) );
#if MIABUILD == 1
	else if( ior->io_Dev->dev.n_Item == analogAudioDeviceItem )
		return( FMVAnalogAudioCmdStatus( ior ) );
	else if( ior->io_Dev->dev.n_Item == analogVideoDeviceItem )
		return( FMVAnalogVideoCmdStatus( ior ) );
#endif


	return( 1L );
}

static int32 CmdWrite( IOReq *ior )
{
	/* Command Write */
	uint32 *src = (uint32 *)ior->io_Info.ioi_Send.iob_Buffer;
	int32 len = ior->io_Info.ioi_Send.iob_Len;

	/* check options */
	if( CheckOptions( ior ) )
	{
		DBUG(("FMVDevice: CmdWrite: bad options\n"));
		goto abort;
	}
	if( len )	// if len is 0 we don't need these checks
	{
		// make sure start & end of buffer are short word aligned
		if( ((int32) src & 0x1L ) || ( len & 0x1L ) )
		{
			DBUG(("FMVDevice: CmdWrite src = %08lx, len = %ld\n",(int32) src,len));
			DBUG(("non-aligned write\n"));
			goto abort;
		}
		if( len < 0L )
		{
			DBUG(("FMVDevice: CmdWrite src = %08lx, len = %ld\n",(int32) src,len));
			DBUG(("bad length\n"));
			goto abort;
		}
	}
	/* dispatch to appropriate device */
	if( ior->io_Dev->dev.n_Item == audioDeviceItem )
		return( FMVAudioCmdWrite( ior ) );
	else if( ior->io_Dev->dev.n_Item == videoDeviceItem )
		return( FMVVideoCmdWrite( ior ) );

#if MIABUILD == 1
	else if( ior->io_Dev->dev.n_Item == analogAudioDeviceItem )
		return( FMVAnalogAudioCmdWrite( ior ) );
	else if( ior->io_Dev->dev.n_Item == analogVideoDeviceItem )
		return( FMVAnalogVideoCmdWrite( ior ) );
#endif

abort:
	DBUG(("CmdWrite aborting\n"));
	ior->io_Error = BADIOARG;
	return( 1L ) ;
}

//================================================================================
//
//================================================================================
static int32 (*CmdTable[])() =
{
	CmdWrite,
	CmdRead,
	CmdStatus,
	CmdControl
};

static TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,				(void *) 1,
	TAG_ITEM_NAME,				FMV_DRIVER_NAME,
	CREATEDRIVER_TAG_ABORTIO,	(void *) FMVAbortIO,
	CREATEDRIVER_TAG_MAXCMDS,	(void *) (sizeof( CmdTable ) / sizeof( int (*)() )),
	CREATEDRIVER_TAG_CMDTABLE,	(void *) CmdTable,
	CREATEDRIVER_TAG_INIT,		(void *) FMVDriverInit,
	TAG_END,					0,
};

static TagArg audioDevArgs[] =
{
	TAG_ITEM_PRI,				(void *) 150,
	CREATEDEVICE_TAG_DRVR,		(void *) 1,
	TAG_ITEM_NAME,				FMV_AUDIO_DEVICE_NAME,
	CREATEDEVICE_TAG_INIT,		(void *) AudioDevInit,
	CREATEDEVICE_TAG_OPEN,		(void *) devOpen,
	CREATEDEVICE_TAG_CLOSE,		(void *) devClose,
	TAG_END,					0,
};

static TagArg videoDevArgs[] =
{
	TAG_ITEM_PRI,				(void *) 100,
	CREATEDEVICE_TAG_DRVR,		(void *) 1,
	TAG_ITEM_NAME,				FMV_VIDEO_DEVICE_NAME,
	CREATEDEVICE_TAG_INIT,		(void *) VideoDevInit,
	CREATEDEVICE_TAG_OPEN,		(void *) devOpen,
	CREATEDEVICE_TAG_CLOSE,		(void *) devClose,
	TAG_END,					0,
};
#if MIABUILD == 1 
//-------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------
static TagArg analogVideoDevArgs[] =
{
	TAG_ITEM_PRI,				(void *) 100,
	CREATEDEVICE_TAG_DRVR,		(void *) 1,
	TAG_ITEM_NAME,				FMV_ANALOG_VIDEO_DEVICE_NAME,
	CREATEDEVICE_TAG_INIT,		(void *) AnalogVideoDevInit,
	CREATEDEVICE_TAG_OPEN,		(void *) devOpen,
	CREATEDEVICE_TAG_CLOSE,		(void *) devClose,
	TAG_END,					0,
};
//-------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------
static TagArg analogAudioDevArgs[] =
{
	TAG_ITEM_PRI,				(void *) 100,
	CREATEDEVICE_TAG_DRVR,		(void *) 1,
	TAG_ITEM_NAME,				FMV_ANALOG_AUDIO_DEVICE_NAME,
	CREATEDEVICE_TAG_INIT,		(void *) AnalogAudioDevInit,
	CREATEDEVICE_TAG_OPEN,		(void *) devOpen,
	CREATEDEVICE_TAG_CLOSE,		(void *) devClose,
	TAG_END,					0,
};
#endif


Item CreateFMVDriver( void )
{
	Item drvrItem;

	drvrItem = CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),drvrArgs);
	KDBUG(("Creating driver returns drvrItem=%ld\n",drvrItem));
			
	if( drvrItem < 0 )
		return( drvrItem );
		
	/* create audio device */
	audioDevArgs[1].ta_Arg = (void *) drvrItem;
	audioDeviceItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),audioDevArgs);
	KDBUG(("Creating FMV audio device returns %ld\n",audioDeviceItem));

	if( audioDeviceItem < 0 )
		return( audioDeviceItem );

	/* create video device */
	videoDevArgs[1].ta_Arg = (void *) drvrItem;
	videoDeviceItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),videoDevArgs);
	KDBUG(("Creating FMV video device returns %ld\n",videoDeviceItem));
	
	if( videoDeviceItem < 0 )
		return( videoDeviceItem );

#if MIABUILD == 1
	/* create analog audio device */
	analogAudioDevArgs[1].ta_Arg = (void *) drvrItem;
	analogAudioDeviceItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),analogAudioDevArgs);
	KDBUG(("Creating analog FMV audio device returns %ld\n",analogAudioDeviceItem));

	if( analogAudioDeviceItem < 0 )
		return( analogAudioDeviceItem );

	/* create analog video device */
	analogVideoDevArgs[1].ta_Arg = (void *) drvrItem;
	analogVideoDeviceItem = CreateItem(MKNODEID(KERNELNODE,DEVICENODE),analogVideoDevArgs);
	KDBUG(("Creating analog FMV video device returns %ld\n",analogVideoDeviceItem));

	if( analogVideoDeviceItem < 0 )
		return( analogVideoDeviceItem );
#endif
	return( videoDeviceItem );
}

