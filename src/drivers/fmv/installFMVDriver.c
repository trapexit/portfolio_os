/*
	File:		installFMVDriver.c

	Contains:	xxx put contents here xxx

	Written by:	George Mitsuoka

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<43>	  1/3/95	GM		Major changes/additions to support demand-loading. Default
									microcode directory is now $devices/FMV.
		<42>	 12/5/94	GM		Fixed handling of signals and failures when loading microcode
									and/or patch files.
		<41>	11/17/94	GM		Added handling of failure to create FMV driver. Creation may
									fail if decoder hardware isn't available.
		<40>	11/17/94	GDW		New EC/EA handling
		<39>	10/12/94	GDW		Change handling of bit buffer parameter.
		<38>	 9/14/94	DM		Added the vbl count used in the Mia driver to delay for writing
									to the hardware.
		<37>	  9/6/94	GDW		WatchDog now takes a value.
		<36>	  9/1/94	GDW		Added switch for SCR handling.
		<35>	 8/26/94	GM		Changed some prints to reduce startup time.
		<34>	 8/24/94	DM		Changed the gVBL from boolean to integer
		<33>	 8/23/94	GDW		Added lots o options to control error handling.
		<32>	 8/15/94	DM		Changed the type of vbl from boolean to int32 so that we can
									control how many vbl to wait before continuing.
		<31>	  8/3/94	GDW		Removed SCR and SCRInc from this level.
		<30>	 7/29/94	GM		Moved first debugging printf to first executable line in main()
									at Drew's request. This allows him to see if the driver is being
									successfully launched.
		<29>	 7/20/94	DM		Used conditional check for ~Mia to remove the debug statement
									indicating the CL450 download code.
		<28>	 7/12/94	GM		Fixed handling and response to signals. Added ScavengeMem()
									calls after freeing microcode buffers.
		<27>	 6/10/94	GDW		Added SCR increment value.
		<26>	  6/2/94	GDW		Removed vbl option and added frameRate option.
		<25>	 5/27/94	GM		Changed default pts mechanism from hw to sw.
		<24>	 5/17/94	GDW		Added YDOXDO options flag.
		<23>	 5/11/94	DM		Add condition build flag for Mia build.
		<22>	  5/6/94	GDW		Audio clock 8 option is now generally supported.
		<21>	  5/4/94	GDW		Added mode Bill options.
		<20>	  5/4/94	BCK		Included FMVAudioDriver.h for FMVAudioSetSWPTSMechanism().
		<19>	  5/3/94	GDW		Added vbl option for Thomson debugging.
		<18>	 4/25/94	BCK		Added switch for using SW vs HW PTS mechanism (-swpts).
		<17>	 4/20/94	GDW		Added new option for switching audio clock.
		<16>	 4/15/94	GDW		Added new arguments for MIA
		<15>	 3/27/94	GM		Changed default timing parameters back to 13Mhz because of
									changed CL450 specifications.
		<14>	 3/18/94	GM		Changed default timing parameters for 20 Mhz CL450 pixel clock
									due to clock mode switching bug in Woody.
		<13>	 3/17/94	BCK		Added usage printout.
		<12>	 3/10/94	GM		Increased default values for HPWidth and frontBorder. This hides
									dropped hsyncs which are due to CL450 microcode problems.
		<11>	  3/8/94	GM		Opens Audio Folio so we can read the audio clock for performance
									analysis.
		<10>	  3/5/94	GM		Added support for loading TI audio decoder patch files.
		 <9>	  3/1/94	GM		Fixed bugs introduced by previous clean up.
		 <8>	 2/28/94	GM		Cleaned up code to avoid compiler warnings.
		 <6>	 2/10/94	GM		Modified kprintfs to use DEBUG macro which prints "FMVDriver: "
									prefix.
		 <5>	 1/17/94	GDW		Prints version information to console.
		 <4>	 1/14/94	GM		
		 <3>	11/30/93	GM		Major code additions to allow download of CL450 microcode from
									Opera filesystem.
		 <2>	11/19/93	GM		Did some George stuff.
		 <2>	11/19/93	GM		Did more George stuff.
*/

/* file: installFMVDriver.c */
/* test installation of FMV driver */
/* 4/23/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifdef THINK_C
#define __swi(a)
#endif

#include "portfolio.h"
#include "types.h"
#include "audio.h"
#include "debug.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "operror.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"

#include "FMV.h"
#include "FMVDriver.h"
#if MIABUILD == 1
#include "FMVAnalogDriver.h"
#endif
#include "FMVAudioDriver.h"
#include "Woody.h"

#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG( args )	{ kprintf("FMVDriver: "); kprintf args ; }

#define DISKSTREAMBUFFERSIZE 2048L

// default settings

#if MIABUILD == 1
#define DEFAULT_HORIZONTAL_PULSE_WIDTH	3
#define DEFAULT_VERTICAL_SYNC_PULSE_WIDTH	32	
#define DEFAULT_FRONT_BORDER			10L
#define DEFAULT_TOP_BORDER				2L
#define DEFAULT_BOTTOM_BORDER			2L
#define DEFAULT_FLAG_13					0L
#define DEFAULTAUDIOCLK					0L				/* Enables audio clock for Mia II */
#define DEFAULTREVAFIX					0L
#define DEFAULTSCRINC					(90000 / 60)
#else
#define DEFAULT_HORIZONTAL_PULSE_WIDTH	240L
#define DEFAULT_VERTICAL_SYNC_PULSE_WIDTH	255L	
#define DEFAULT_FRONT_BORDER			252L
#define DEFAULT_TOP_BORDER				10L
#define DEFAULT_BOTTOM_BORDER			10L
#define DEFAULT_FLAG_13					1L
#define DEFAULTAUDIOCLK					1L				/* Enables audio clock for Mia II */
#endif

#define DEFAULT_BACK_BORDER				10L
#define DEFAULT_UCODE_BOOT_FILENAME		"$devices/FMV/CL450Boot.UCode"
#define DEFAULT_UCODE_16_FILENAME		"$devices/FMV/CL45016Bit.UCode"
#define DEFAULT_UCODE_24_FILENAME		"$devices/FMV/CL45024Bit.UCode"
#define DEFAULT_AUDIO_PATCH_1_FILENAME	"TIAV110Patch1"
#define DEFAULT_AUDIO_PATCH_2_FILENAME	"TIAV110Patch2"
#define DEFAULT_FUDGE_FACTOR			1L
#define DEFAULT_DEBUG					0L

//==============================================================================
// Special things for debugging Thomson
//==============================================================================

#if MIABUILD == 1
#define DEFAULT_INIT7194				1L
#define DEFAULT_NODISPLAY				0L
#define DEFAULT_BITBUFFER_SIZE			0x1FF			// Default size of the Thomson bit buffer
#define DEFAULT_PIXCLK					1L				// Enables one clock delay
#define DEFAULTVBL						0L				// Disable VTop/VBottom interrupts
#define DEFAULTSTOPMODE					0L				// Disable Thomson stop mode
#define DEFAULTPERERROR					1L				// Enable Thomson PER error recovery SW
#define DEFAULTSERERROR					0L				// Enable Thomson SER error recovery
#define DEFAULTYDOXDO					1L				// Use the default YDOXDO value
#define DEFAULTWATCHDOG					0L				// Use video watch dog reset
#define DEFAULTWATCHDOGFRAMES			4				// Number of frames to wait before reset
#define DEFAULTNOSCR					0L				// Start the SCR clock
#define DEFAULTFIXEDBITBUFF				0L				// Compute bit buffer size from sequence header
#endif

int32 debugFlag = 0L;
int32 HPWidth,frontBorder,topBorder,bottomBorder;
int32 VSWidth,backBorder,flag13,fudge;
int32 gSignalVideoMode16, gSignalVideoMode24;
int32 gSignalAudioPatch1, gSignalAudioPatch2;
int32 gSignalLoaded, gSignalLoadError, gSignalDone, *gUcodeBuffer;
char *ucodeBootFilename;
char *ucode16Filename;
char *ucode24Filename;
char *audioPatch1Filename;
char *audioPatch2Filename;
struct Task *gMainTask;
Item gClientTaskItem;
Item gAudioDeviceItem, gVideoDeviceItem;
int32		gAudioClk;

//==============================================================================
// Special things for debugging Thomson
//==============================================================================

#if MIABUILD == 1
int32 		gInit7194;
int32 		gNoDisplay;
uint16		gBitBuffSize;
uint16		gBitBuffThreshold;
int32		gPixClkDelay;
int32		gVBL;
int32		gStopMode;
uint32		gStopPictureCnt;
int32		gSWPERError;
int32		gSERError;
int32		gWatchDogEnabled; 
int32		gWatchDogFrames;
int32		gNoSCREnabled;
int32		gDefaultYDOXDO;
uint16		gYDOXDO;
int32		gREVAFixEnabled;
int32		gFixedBitBitBuffSize;
int32		gFixedBitBitBuffThreshold;
#endif

static int32 LoadMicrocodeFile( char *filename )
{
	Stream *ucodeStream;
	int32 length, waitResult, status = 0L;
	
	/* open microcode file */
	if( (ucodeStream = OpenDiskStream( filename,DISKSTREAMBUFFERSIZE)) == NULL )
	{
		status = -1L;
		goto abort;
	}
	/* read length word */
	if( ReadDiskStream( ucodeStream, (char *) &length, sizeof( length ) ) != sizeof( length ) )
	{
		DEBUG(("Couldn't read %s\n",filename));
		status = -1L;
		goto abortCloseStream;
	}
	/* allocate microcode buffer memory */
	length *= sizeof( int32 );
	if( (gUcodeBuffer = (int32 *) malloc( length )) == NULL )
	{
		DEBUG(("Couldn't allocate microcode buffer\n"));
		status = -1L;
		goto abortCloseStream;
	}
	/* read microcode into buffer */
	if( ReadDiskStream( ucodeStream, (char *) gUcodeBuffer, length ) != length )
	{
		DEBUG(("Couldn't read microcode from %s\n",filename));
		status = -1L;
		goto abortFreeBuffer;
	}
	/* signal that microcode has been loaded */
	SendSignal( gClientTaskItem, gSignalLoaded );
	
	/* wait for signal that buffer has been used */
	waitResult = WaitSignal( gSignalDone );
	if( waitResult & SIGF_ABORT )
		status = -2L;
		
abortFreeBuffer:
	free( gUcodeBuffer );
	ScavengeMem();
abortCloseStream:
	CloseDiskStream( ucodeStream );	
abort:
	if( status == -1L )
		SendSignal( gClientTaskItem, gSignalLoadError );
		
	return( status );
}

#define DAEMON_STACK_SIZE 2000
static char daemonStack[DAEMON_STACK_SIZE];

static void DaemonThread(void)
{
	int32 waitResult, signalMask;

	/* this task must access microcode in the filesystem on behalf of the driver */
	/* set up signalling mechanism */
	gMainTask = CURRENTTASK;

	if( ((gSignalVideoMode16 = AllocSignal( 0 )) == 0L) ||
		((gSignalVideoMode24 = AllocSignal( 0 )) == 0L) ||
		((gSignalAudioPatch1 = AllocSignal( 0 )) == 0L) ||
		((gSignalAudioPatch2 = AllocSignal( 0 )) == 0L) ||
		((gSignalDone = AllocSignal( 0 )) == 0L) )
	{
		DEBUG(("couldn't allocate signals\n"));
		goto abort;
	}
	signalMask = gSignalVideoMode16 | gSignalVideoMode24 |
				 gSignalAudioPatch1 | gSignalAudioPatch2;
	while( 1 )
	{
		DEBUG(("Waiting for mode signal\n"));
		waitResult = WaitSignal( signalMask );
		if( waitResult & SIGF_ABORT )
		{
			DEBUG(("received SIGF_ABORT\n"));
			goto abort;
		}
		if( waitResult & gSignalAudioPatch1 )
		{
			DEBUG(("Got audio patch1 signal\n"));
			if( !LoadMicrocodeFile( audioPatch1Filename ) )
				DEBUG(("loaded %s\n",audioPatch1Filename));
			continue;
		}
		if( waitResult & gSignalAudioPatch2 )
		{
			DEBUG(("Got audio patch2 signal\n"));
			if( !LoadMicrocodeFile( audioPatch2Filename ) )
				DEBUG(("loaded %s\n",audioPatch2Filename));
			continue;
		}
		if( waitResult & (gSignalVideoMode16 | gSignalVideoMode24 ) )
		{
			DEBUG(("loading %s\n",ucodeBootFilename));
			if( LoadMicrocodeFile( ucodeBootFilename ) )
			{
				DEBUG(("error loading %s\n",ucodeBootFilename));
				continue;
			}
			if( waitResult & gSignalVideoMode16 )
			{
				DEBUG(("loading %s\n",ucode16Filename));
				if( LoadMicrocodeFile( ucode16Filename ) )
				{
					DEBUG(("error loading %s\n",ucode16Filename));
					continue;
				}
			}
			else if( waitResult & gSignalVideoMode24 )
			{
				DEBUG(("loading %s\n",ucode24Filename));
				if( LoadMicrocodeFile( ucode24Filename ) )
				{
					DEBUG(("error loading %s\n",ucode24Filename));
					continue;
				}
			}
		}
	}
abort:
	DEBUG(("aborting\n"));
}

int main( int32 argc, char *argv[] )
{
	Item child;
	Item driverItem;
	int32 arg, demandLoaded = 0;

	if (argc == DEMANDLOAD_MAIN_CREATE)
		demandLoaded = 1;
	else if( argc == DEMANDLOAD_MAIN_DELETE )
		return( 0L );

#ifdef MAC_BUILD

#else
    print_vinfo();
#endif
	// default settings
	HPWidth = DEFAULT_HORIZONTAL_PULSE_WIDTH;
	frontBorder = DEFAULT_FRONT_BORDER;
	topBorder = DEFAULT_TOP_BORDER;
	bottomBorder = DEFAULT_BOTTOM_BORDER;
	backBorder = DEFAULT_BACK_BORDER;
	VSWidth = DEFAULT_VERTICAL_SYNC_PULSE_WIDTH;
	ucodeBootFilename = DEFAULT_UCODE_BOOT_FILENAME;
	ucode16Filename = DEFAULT_UCODE_16_FILENAME;
	ucode24Filename = DEFAULT_UCODE_24_FILENAME;
	audioPatch1Filename = DEFAULT_AUDIO_PATCH_1_FILENAME;
	audioPatch2Filename = DEFAULT_AUDIO_PATCH_2_FILENAME;
	flag13 = DEFAULT_FLAG_13;
	fudge = DEFAULT_FUDGE_FACTOR;
	debugFlag = DEFAULT_DEBUG;
	gAudioClk = DEFAULTAUDIOCLK;
	FMVAudioSetSWPTSMechanism(true);

#if MIABUILD == 1
	gInit7194 = DEFAULT_INIT7194;
	gNoDisplay = DEFAULT_NODISPLAY;
	gBitBuffSize = DEFAULT_BITBUFFER_SIZE;
	gBitBuffThreshold = DEFAULT_BITBUFFER_SIZE - 4;
	gFixedBitBitBuffSize = DEFAULTFIXEDBITBUFF;
	gFixedBitBitBuffThreshold = DEFAULTFIXEDBITBUFF;
	gPixClkDelay = DEFAULT_PIXCLK;
	gVBL = DEFAULTVBL;
	gStopMode = DEFAULTSTOPMODE;
	gStopPictureCnt = 0L;
	gSWPERError = DEFAULTPERERROR;
	gSERError = DEFAULTSERERROR;
	gWatchDogEnabled = DEFAULTWATCHDOG;
	gWatchDogFrames = DEFAULTWATCHDOGFRAMES;
	gNoSCREnabled = DEFAULTNOSCR;
	gDefaultYDOXDO = DEFAULTYDOXDO;
	gYDOXDO = 0x115B;
	gREVAFixEnabled = DEFAULTREVAFIX;
#endif

	if( !demandLoaded )
	{
		for(arg = 1; arg < argc; arg++)
		{
			if( strcasecmp(argv[ arg ],"-debug") == 0 )
			{
				debugFlag = 1L;
				Debug();
			}
			else if( (strcasecmp(argv[ arg ],"-hpw") == 0) && (arg+1 < argc) )
				HPWidth = atoi( argv[ ++arg ] );
			else if( (strcasecmp(argv[ arg ],"-front") == 0) && (arg+1 < argc) )
				frontBorder = atoi( argv[ ++arg ] );
			else if( (strcasecmp(argv[ arg ],"-top") == 0) && (arg+1 < argc) )
				topBorder = atoi( argv[ ++arg ] );
			else if( (strcasecmp(argv[ arg ],"-bottom") == 0) && (arg+1 < argc) )
				bottomBorder = atoi( argv[ ++arg ] );
			else if( (strcasecmp(argv[ arg ],"-back") == 0) && (arg+1 < argc) )
				backBorder = atoi( argv[ ++arg ] );
			else if( (strcasecmp(argv[ arg ],"-vsw") == 0) && (arg+1 < argc) )
				VSWidth = atoi( argv[ ++arg ] );
			else if( (strcasecmp(argv[ arg ],"-ucodeBoot") == 0) && (arg+1 < argc) )
				ucodeBootFilename = argv[ ++arg ];
			else if( (strcasecmp(argv[ arg ],"-ucode16") == 0) && (arg+1 < argc) )
				ucode16Filename = argv[ ++arg ];
			else if( (strcasecmp(argv[ arg ],"-ucode24") == 0) && (arg+1 < argc) )
				ucode24Filename = argv[ ++arg ];
			else if( (strcasecmp(argv[ arg ],"-apatch1") == 0) && (arg+1 < argc) )
				audioPatch1Filename = argv[ ++arg ];
			else if( (strcasecmp(argv[ arg ],"-apatch2") == 0) && (arg+1 < argc) )
				audioPatch2Filename = argv[ ++arg ];
			else if( (strcasecmp(argv[ arg ],"-f") == 0) && (arg+1 < argc) )
				fudge = atoi( argv[ ++arg ] );
			else if( strcasecmp(argv[ arg ],"-20") == 0)
				flag13 = 0L;
			else if( strcasecmp(argv[ arg ],"-13") == 0)
				flag13 = 1L;
			else if( strcasecmp(argv[ arg ],"-audioClk8") == 0)
				gAudioClk = false;
			else if( strcasecmp(argv[ arg ],"-swpts") == 0)
				FMVAudioSetSWPTSMechanism(true);
			else if( strcasecmp(argv[ arg ],"-hwpts") == 0)
				FMVAudioSetSWPTSMechanism(false);
#if MIABUILD == 1
			else if( strcasecmp(argv[ arg ],"-no7194Init") == 0)
				gInit7194 = false;
			else if( strcasecmp(argv[ arg ],"-noDisplay") == 0)
				gNoDisplay = false;
			else if( strcasecmp(argv[ arg ],"-bitBuffSize") == 0) {
				gFixedBitBitBuffSize = true;
				gBitBuffSize = atoi( argv[ ++arg ] );
			} else if( strcasecmp(argv[ arg ],"-bitBuffThres") == 0) {
				gFixedBitBitBuffThreshold = true;
				gBitBuffThreshold = atoi( argv[ ++arg ] );
			} else if( strcasecmp(argv[ arg ],"-noPixClkDelay") == 0)
				gPixClkDelay = false;
			else if( strcasecmp(argv[ arg ],"-vbl") == 0)
				gVBL = atoi( argv[ ++arg ] );
			else if( strcasecmp(argv[ arg ],"-noSCR") == 0)
				gNoSCREnabled = true;
			else if( strcasecmp(argv[ arg ],"-PERHWHandles") == 0)
				gSWPERError = false;
			else if( strcasecmp(argv[ arg ],"-SERHandling") == 0)
				gSERError = true;
			else if( strcasecmp(argv[ arg ],"-enableRevAFix") == 0)
				gREVAFixEnabled = true;
			else if( strcasecmp(argv[ arg ],"-stopMode") == 0) {
				gStopMode = true;
				gStopPictureCnt = atoi( argv[ ++arg ] ); 
			} else if ( strcasecmp(argv[ arg ],"-YDOXDO") == 0) {
				gDefaultYDOXDO = false;
				gYDOXDO = atoi( argv[ ++arg ] ); 
			} else if ( strcasecmp(argv[ arg ],"-enableWatchDog") == 0) {
				gWatchDogEnabled = true;
				gWatchDogFrames = atoi( argv[ ++arg ] ); 
			}
#endif
			else
			{
				printf("usage: installFMVDriver - Version %s - Installs the FMV Driver.\n");
				printf("    [-hpw size]         - Horizontal sync pulse width (DEFAULT %ld).\n", DEFAULT_HORIZONTAL_PULSE_WIDTH); 
				printf("    [-front size]       - Front border width (DEFAULT %ld).\n", DEFAULT_FRONT_BORDER); 
				printf("    [-top size]         - Top border width (DEFAULT %ld).\n", DEFAULT_TOP_BORDER); 
				printf("    [-bottom size]      - Bottom border width (DEFAULT %ld).\n", DEFAULT_BOTTOM_BORDER); 
				printf("    [-back size]        - Back border width (DEFAULT %ld).\n", DEFAULT_BACK_BORDER); 
				printf("    [-vsw size]         - Vertical sync pulse width (DEFAULT %ld).\n", DEFAULT_VERTICAL_SYNC_PULSE_WIDTH); 
				printf("    [-ucodeBoot file]   - CL450 Microcode Boot filename (DEFAULT %s).\n", DEFAULT_UCODE_BOOT_FILENAME); 
				printf("    [-ucode16 file]     - CL450 Microcode 16 Bit filename (DEFAULT %s).\n", DEFAULT_UCODE_16_FILENAME); 
				printf("    [-ucode24 file]     - CL450 Microcode 24 Bit filename (DEFAULT %s).\n", DEFAULT_UCODE_24_FILENAME); 
				printf("    [-apatch1 file]     - AV110 Patch 1 filename (DEFAULT %s).\n", DEFAULT_AUDIO_PATCH_1_FILENAME); 
				printf("    [-apatch2 file]     - AV110 Patch 2 filename (DEFAULT %s).\n", DEFAULT_AUDIO_PATCH_2_FILENAME); 
				printf("    [-f fudgeFactor]    - Fudge factor (DEFAULT %ld).\n", DEFAULT_FUDGE_FACTOR); 
				printf("    [-20]               - Flag 13 off (DEFAULT %ld).\n", DEFAULT_FLAG_13); 
				printf("    [-13]               - Flag 13 on (DEFAULT %ld).\n", DEFAULT_FLAG_13); 
				printf("    [-swpts]            - Use software PTS mechanism instead of the HW mechanism (DEFAULT SW mechanism).\n");
				printf("    [-hwpts]            - Use hardware PTS mechanism instead of the SW mechanism (DEFAULT SW mechanism).\n");
				printf("    [-debug]            - Turns on debugging help (DEFAULT %ld).\n", DEFAULT_DEBUG); 
				printf("    [-audioClk8]        - Set audio PCM divider to 8 (DEFAULT %ld).\n", DEFAULTAUDIOCLK); 
#if MIABUILD == 1
				printf("    [-no7194Init]       - Does not initialize 7194 (DEFAULT %ld).\n", DEFAULT_INIT7194); 
				printf("    [-noDisplay]        - Does not turn on the 3500 display (DEFAULT %ld).\n", DEFAULT_NODISPLAY); 
				printf("    [-bitBuffSize nn]   - Sets the bit buffer size for 3500 (DEFAULT %ld).\n", DEFAULT_BITBUFFER_SIZE); 
				printf("    [-bitBuffThres nn]  - Sets the bit buffer threshold for 3500 (DEFAULT %ld).\n", DEFAULT_BITBUFFER_SIZE); 
				printf("    [-noPixClkDelay]    - Disables pixel clock delay for 3500 (DEFAULT %ld).\n", DEFAULT_PIXCLK); 
	//			printf("    [-frameRate]        - Set the increment amount of SCR (DEFAULT %ld).\n", DEFAULTSCRINC); 
				printf("    [-stopMode]         - Stops decoding on frame number (DEFAULT %ld).\n", 0L); 
				printf("    [-noSCR]            - Doesn't start the SCR clock (DEFAULT %ld).\n", DEFAULTNOSCR); 
				printf("    [-enableWatchDog nn] - Reset video if decode stalls (DEFAULT %ld).\n", DEFAULTWATCHDOG); 
				printf("    [-PERHWHandles]     - Hardware handles pipeline errors (DEFAULT %ld).\n", DEFAULTPERERROR); 
				printf("    [-SERHandling]      - Do handle serious errors (DEFAULT %ld).\n", DEFAULTSERERROR); 
				printf("    [-enableRevAFix]    - Do not handle serious errors (DEFAULT %ld).\n", DEFAULTREVAFIX); 
				printf("    [-YDOXDO]           - Use specified YDOXDO value (DEFAULT %ld).\n", DEFAULTYDOXDO); 
				printf("    [-vbl number]     	- Sets the how many vbls to wait (DEFAULT %ld).\n", DEFAULTVBL); 
#endif
				printf("    [-?]                - Display this message.\n");
				exit(0);
			}
		}
	}
	DEBUG(("hpw = %ld front = %ld top = %ld ",HPWidth,frontBorder,topBorder));
	DEBUG(("bottom = %ld back = %ld flag13 = %ld ",bottomBorder,backBorder,flag13));
	DEBUG(("vsw = %ld\n",VSWidth));
#if MIABUILD == 0
	DEBUG(("ucodeBoot = '%s' ucode16 = '%s' ucode24 = '%s'\n",
			ucodeBootFilename,ucode16Filename,ucode24Filename));
#endif
	DEBUG(("apatch1 = '%s' apatch2 = %s\n",audioPatch1Filename,audioPatch2Filename));
	DEBUG(("creating driver\n"));
	
	driverItem = CreateFMVDriver();
	if (driverItem < 0)
	{
		DEBUG(("Error creating FMVDriver %d\n",driverItem));
		PrintfSysErr( driverItem );
		return (int)driverItem;
	}

	DEBUG(("creating daemon thread\n"));

	child = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
						 TAG_ITEM_NAME,                 "FMV Daemon",
						 CREATETASK_TAG_PC,             DaemonThread,
						 CREATETASK_TAG_STACKSIZE,      DAEMON_STACK_SIZE,
						 CREATETASK_TAG_SP,             &daemonStack[DAEMON_STACK_SIZE],
						 CREATETASK_TAG_SUPER,          0,
						 TAG_END);
	if (child < 0)
	{
	    DeleteItem(driverItem);
	    DEBUG(("Error creating daemon thread: "));
	    PrintfSysErr(child);
	    return (int)child;
	}
	if( !demandLoaded )
		WaitSignal( 0L );

	return( (int) driverItem );
}
