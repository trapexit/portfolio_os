/*
	File:		CL450.c

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<28>	 12/5/94	GM		Added handling of SIGF_ABORT after all calls to SuperWaitSignal.
		<27>	11/17/94	GDW		Changed args sent to Pause routine.
		<26>	  9/7/94	GM		Modified interrupt protection in FMVDo450Command() so interrupts
									will not be locked out during waits.
		<25>	  9/7/94	GM		Added interrupt protection in FMVDo450Command().
		<24>	  9/1/94	GM		Cleaned up code to get rid of compiler warnings. Allowed CL450
									microcode initialization to time out and retry in case of
									failure.
		<23>	 8/30/94	GM		Changed CL450DebugInfo structure name to CL450StatusInfo and
									added flag to work around a CL450 bug (the CL450 occasionally
									hangs if flushed before any data has been sent. We'll work
									around this (until C-Cube fixes it) by not flushing unless we
									have sent a couple of new packet commands). Removed unnecessary
									spins and fixed a race condition during CL450 initialization.
									Added a delay in FMVReset450.
		<22>	 8/10/94	GM		Re-enabled FMVVidDecPlay and FMVVidDecPause. Added PTS debugging
									code. Added pause before flush and play after flush to prevent
									output of old pictures when transitioning to a new stream.
		<21>	  8/3/94	GDW		Changed play and pause commands to not call 450 low level.
		<20>	 7/12/94	GM		Removed unused args to FMVDo450Command. Added check for CL450
									acceptance of NewPacket command to avoid race condition. Enabled
									Picture decoded interrupt. Added some PTS debugging code.
		<19>	  7/6/94	GDW		Added device change function.
		<18>	  5/3/94	GDW
		<17>	  4/5/94	GM		Removed extraneous setting of HOST_CONTROL register to
									HCTRL_NOTINT at Glen Ivey's (C-Cube) instruction.
		<16>	  3/5/94	GM		Added freeing of allocated signal when no longer used.
		<15>	  3/1/94	GM		Cleaned up code to avoid compiler warnings.
		<14>	 2/25/94	GM		Added I frame search and skip B picture code.
		<13>	 2/25/94	GDW		Did some global variable clean up and removed  some warnings.
		<12>	 2/14/94	GDW		New header file with low level interfaces.
		<11>	 2/14/94	GM		Cleaned up code. Added support for stream discontinuities using
									flushbitstream command.
		<10>	 2/10/94	GM		Changed public routine name prefix to FMVVidDec from FMV450 to
									assist in development of Thomson version driver.
		 <9>	  2/7/94	GDW		Changed resample defines.
		 <8>	  2/7/94	GM		Added debugging code for reading SCRs from the CL450. This only
									works with diagnostic microcode and should be removed in future
									versions.
		 <7>	  2/2/94	GM		Fixed handling of buffer underflow interrupt in coordination
									with changes to FMVVideoDriver.c
		 <6>	  2/1/94	GM		Modified interrupt handling routines. Updated to reflect latest
									CL450 interrupt specifications. Now handles numerous CL450
									interrupts.
		 <5>	 1/14/94	GM		Added PTS support.
		 <4>	11/30/93	GM		Major code changes to allow download of CL450 microcode from the
									Opera filesystem instead of directly from the FMV ROM.
		 <3>	11/19/93	GM		modified cmem_dmactrl initialization to 1qe from 2qe
		 <2>	11/18/93	GM		Removed hsize <= 128 bug fix no longer necessary with new
									microcode

*/

/* file: CL450.c */
/* C-Cube CL450 initialization and control */
/* 4/8/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#include "types.h"
#include "kernel.h"
#include "task.h"

#include "CL450.h"
#include "FMV.h"
#include "FMVErrors.h"
#include "FMVROM.h"
#include "Hollywood.h"
#include "Woody.h"
#include "FMVDriver.h"
#include "FMVVideoDriverImp.h"

uint32 gIMEMImage[ IMEM_SIZE ];
#define PTS_DEBUG 1
#define VIDPTSLOGSIZE 256
struct
{
	int32 vidSwitchCount, vidPTSInCount, vidPTSOutCount, lastVidPTS, cflevel;
	int32 dontFlush, reserved1, reserved2, reserved3, reserved4;
	struct { uint32 inPTS, inLength, vidStreamOffset, outPTS, delta; } vidPTSLog[ VIDPTSLOGSIZE ];
}
CL450StatusInfo = { 0L,0L,0L,0L,0xffffffffL, 0L,0L,0L,0L,0L };

#define TIMEOUTLIMIT	50000

extern struct Task *gMainTask;
extern Item gClientTaskItem;
extern int32 gSignalVideoMode16, gSignalVideoMode24;
extern int32 gSignalLoaded, gSignalDone, *gUcodeBuffer;

/* reset the CL450 */

void FMVReset450()
{
	int32 i;
	/* toggle reset bits in CMEM_control register */
	FMVWriteVideoRegister( CMEM_CONTROL, CMCTRL_RESET );

	/* need to hold for 1.25 microseconds? */
	for( i = 0; i < 100; i++ )
		FMVReadVideoRegister( CMEM_CONTROL );

	FMVWriteVideoRegister( CMEM_CONTROL, CMCTRL_START );
}

/* execute a CL450 µcode command */

int32 FMVDo450Command( uint32 arg0, uint32 arg1, uint32 arg2, uint32 arg3, uint32 arg4 )
{
	int32   timeout, interrupts;

	/* wait for completion of previous command */
	/* DANGER, DANGER, busy wait, ACK, ACK */

	/* disable interrupts so that another 450 command can't break us */
	interrupts = Disable();

	timeout = 0;
	while( FMVReadVideoRegister( HOST_NEWCMD ) & 0x01 )
	{
		/* don't keep interrupts disabled for the duration of the loop */
		Enable( interrupts );
		// SuperSwitch();
		CL450StatusInfo.vidSwitchCount++;
		if( timeout++ > TIMEOUTLIMIT )
		{
			DEBUGP(("FMVDo450Command: timed out\n"));
			return( -1 );
		}
		/* interrupts must be disabled before the check and until the command is sent */
		interrupts = Disable();
	}
	/* set up initial address to write to command registers */
	FMVWriteVideoRegister( HOST_RADDR, 0 );

	/* address auto-increments after each write */
	/* write the command and parameters */
	FMVWriteVideoRegister( HOST_RDATA, arg0 );
	FMVWriteVideoRegister( HOST_RDATA, arg1 );
	FMVWriteVideoRegister( HOST_RDATA, arg2 );
	FMVWriteVideoRegister( HOST_RDATA, arg3 );
	FMVWriteVideoRegister( HOST_RDATA, arg4 );

	/* execute the command */
	FMVWriteVideoRegister( HOST_NEWCMD, H_NEWCOMMAND );

	Enable( interrupts );
	return( 0L );
}

/* write block of words to the CL450 IMEM */

int32 FMVWrite450IMEM( uint32 startAddress, uint32 length, uint32 *data )
{
	long count, timeout;

	DEBUGP(("FMVWrite450IMEM\n"));

	timeout = 0L;
	while( FMVReadVideoRegister( CPU_CONTROL ) & CPU_RUNENABLE )
		if( timeout++ > TIMEOUTLIMIT )
		{
			DEBUGP(("FMVWrite450IMEM: timeout waiting for CL450 to stop\n"));
			return( -1L );
		}

	/* write IMEM */
	startAddress <<= 1;

	for( count = 0L; count < length; count++ )
	{
		uint32 temp;

		FMVWriteVideoRegister( CPU_IADDR, startAddress++ );
		temp = (*data >> 16) & 0xffff;			/* hi 16 bits */
		FMVWriteVideoRegister( CPU_IMEM, temp );

		FMVWriteVideoRegister( CPU_IADDR, startAddress++ );
		temp = *data++ & 0xffff;				/* lo 16 bits */
		FMVWriteVideoRegister( CPU_IMEM, temp );
	}
	/* no way to verify write takes place */
	return( 0L );
}

/* FMV450RunBootCode() loads and begins execution of the CL450 boot code.
   This code is used to load decoder microcode into CL450 DRAM from CL450
   TMEM. This is necessary because we do not have enough address lines to
   access all of CL450 DRAM from the host */

FMVError FMV450RunBootCode( uint32 *bootCode )
{
	uint32 IMEMStartingAddress, entryPoint, codeIndex, codeSize;
	uint32 codeWordCount, IMEMAddress;
	int32 timeout;

	/* bootCode image layout is as follows:
		image[ 0 ] = CL450 IMEM starting address
		image[ 1 ] = CL450 execution starting address
		image[ 2 ] = code size (# of longwords)
		image[ 3 ] -> image[ 3 + code size - 1 ] = CL450 microinstructions */

	DEBUGP(("FMV450RunBootCode\n"));

	/* read boot code header information */
	IMEMStartingAddress = bootCode[ 0L ];
	entryPoint = bootCode[ 1L ];
	codeSize = bootCode[ 2L ];

	/* read µcode into gIMEMImage buffer */
	codeIndex = 3L;
	IMEMAddress = IMEMStartingAddress;
	for( codeWordCount = 0; codeWordCount < codeSize; codeWordCount++ )
	{
		if( IMEMAddress >= IMEM_SIZE )
			return( kErrBadBootCodeAddress );

		gIMEMImage[ IMEMAddress++ ] = bootCode[ codeIndex++ ];
	}
	/* wait for completion of previous command */
	/* DANGER, DANGER, busy wait, ACK, ACK */

	timeout = 0;
	while( FMVReadVideoRegister( HOST_NEWCMD ) & 0x01 )
		if( timeout++ > TIMEOUTLIMIT )
		{
			DEBUGP(("FMV450RunBootCode: timed out\n"));
			return( kErrFMVCL450CommandTimeout );
		}

	/* disable CL450 execution */
	FMVWriteVideoRegister( CPU_CONTROL, CPU_RUNDISABLE );

	/* dump µcode to CL450 IMEM */
	FMVWrite450IMEM( 0L, IMEM_SIZE, gIMEMImage );

	/* set the initial pc value */
	FMVWriteVideoRegister( CPU_PC, entryPoint );

	/* set run enable */
	FMVWriteVideoRegister( CPU_CONTROL, CPU_RUNENABLE );

	/* now ready to load DRAM from TMEM */

	return( kNoErr );
}

/* FMV450RunCode() loads and begins execution of CL450 microcode.
   The microcode is loaded into CL450 DRAM using special boot microcode
   as described above */

FMVError FMV450RunCode( uint32 *code )
{
	uint32 codeIndex, IMEMStartingAddress, entryPoint;
	uint32 segmentCount, segment, segmentAddress, segmentSize;
	uint32 codeWordCount, codeWordAddress, codeWord, TMEMAddress, savedTMEMAddress;
	int32 segmentIsInIMEM,i;
	uint32 timeout;
	uint32 TMEMImage[ TMEM_SIZE / 2 ];

	DEBUGP(("FMV450RunCode\n"));

	/* read microcode header information */
	IMEMStartingAddress = code[ 0L ];
	IMEMStartingAddress >>= 2L;				/* convert byte to word address */
	entryPoint = code[ 1L ];
	segmentCount = code[ 2L ];

	/* load segments */
	codeIndex = 3L;

	/* clear IMEM image (to ease debugging) */
	for( i = 0; i < IMEM_SIZE; i++)
		gIMEMImage[ i ] = 0L;

	for( segment = 0; segment < segmentCount; segment++ )
	{
		segmentAddress = code[ codeIndex++ ];
		segmentSize = code[ codeIndex++ ];

		segmentAddress >>= 2;				/* convert byte to word address */
		codeWordAddress = segmentAddress;
		codeWordCount = 0;

		if( (IMEMStartingAddress <= segmentAddress) && (segmentSize <= CACHE_END) )
			segmentIsInIMEM = 1L;
		else
			segmentIsInIMEM = 0L;

		while( codeWordCount < segmentSize )
		{
			uint32 temp, addrHi, addrLo, addr;

			/* wait for completion of previous command */
			/* DANGER, DANGER, busy wait, ACK, ACK */

			timeout = 0;
			while( FMVReadVideoRegister( HOST_NEWCMD ) & 0x01 )
				if( timeout++ > TIMEOUTLIMIT )
				{
					DEBUGP(("FMV450RunCode: timed out\n"));
					return( kErrFMVCL450CommandTimeout );
				}

			/* start a read-modify write on DRAM */
			/* execute command which copies DRAM to right place in TMEM */
			/* address is in short words, segmentAddress is in long words */
			/* address is split over two arguments 3:16 */
			TMEMAddress = savedTMEMAddress = (segmentAddress % 32L) << 1L;
			segmentAddress &= 0xffffffe0;	/* !@#$ C-Cube */
			addr = segmentAddress << 1;		/* convert to short word address */
			addrLo = addr & 0xffff;			/* extract low 16 bits */
			addrHi = addr >> 16;			/* extract high 3 bits */

			temp = 0L;
			FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_READDRAM, temp );

			/* load code into TMEM, the boot code then moves it to DRAM */
			/* initialize TMEMAddress, it autoincrements with each write */
			FMVWriteVideoRegister( CPU_TADDR, TMEMAddress );

			while( (TMEMAddress < TMEM_SIZE) && (codeWordCount < segmentSize) )
			{
				/* get the microcode word */
				codeWord = code[ codeIndex++ ];
				codeWordCount++ ;

				/* store TMEM Image for later write verification */
				TMEMImage[ TMEMAddress / 2 ] = codeWord;

				/* write to TMEM in two 16 bit chunks */
				temp = (codeWord >> 16) & 0xffff;			/* hi 16 bits */
				FMVWriteVideoRegister( CPU_TMEM, temp );
				temp = codeWord & 0xffff;					/* lo 16 bits */
				FMVWriteVideoRegister( CPU_TMEM, temp );

				/* if address is in IMEM, write it there as well */
				if( segmentIsInIMEM )
				{
					gIMEMImage[ codeWordAddress - IMEMStartingAddress ] = codeWord;
				}
				TMEMAddress += 2L;
				codeWordAddress++;
			}
			/* TMEM is now loaded with microcode */
			/* execute command which copies TMEM to right place in DRAM */
			temp = 0L;
			FMVDo450Command( addrLo, addrHi, TMEM_SIZE, BOOT_WRITEDRAM, temp );

			segmentAddress += TMEM_SIZE >> 1;
		}
		/* microcode segment loaded */
	}
	/* all segments loaded */
	DEBUGP(("    loaded %ld segments\n",segment));

	/* now we no longer need the boot code */
	/* we can overwrite it with the initial cached microcode */

	/* wait for completion of previous command */
	/* DANGER, DANGER, busy wait, ACK, ACK */

	timeout = 0;
	while( FMVReadVideoRegister( HOST_NEWCMD ) & 0x01 )
		if( timeout++ > TIMEOUTLIMIT )
		{
			DEBUGP(("FMV450RunCode: timed out\n"));
			return( kErrFMVCL450CommandTimeout );
		}

	/* disable CL450 execution */
	FMVWriteVideoRegister( CPU_CONTROL, CPU_RUNDISABLE );

	/* dump µcode to CL450 IMEM */
	FMVWrite450IMEM( 0L, IMEM_SIZE, gIMEMImage );

	/* set the initial pc value */
	FMVWriteVideoRegister( CPU_PC, entryPoint & 0x1ff);
	i = FMVReadVideoRegister( CPU_PC );

	/* clear TMEM */
	FMVWriteVideoRegister( CPU_TADDR, 0L );
	for( TMEMAddress = 0; TMEMAddress < TMEM_SIZE; TMEMAddress++ )
		FMVWriteVideoRegister( CPU_TMEM, 0L );

	/* write HMEM location 15, the CL450 clears this reg when done initializing */
	FMVWriteVideoRegister( HOST_RADDR, 15L );
	FMVWriteVideoRegister( HOST_RDATA, 0xffffL );

	/* set run enable */
	FMVWriteVideoRegister( CPU_CONTROL, CPU_RUNENABLE );

	/* wait for the CL450 to complete initialization */
	timeout = 0;
	do
	{
		FMVWriteVideoRegister( HOST_RADDR, 15L );
		if( timeout++ > TIMEOUTLIMIT )
		{
			DEBUGP(("FMV450RunCode: timed out\n"));
			return( kErrFMVCL450CommandTimeout );
		}
	}
	while( FMVReadVideoRegister( HOST_RDATA ) );

	DEBUGP(("    microcode downloaded and enabled after %ld\n",timeout));

	/* ready to rock 'n roll! */
	return( kNoErr );
}

// coordinate filesystem access with the driver task to
// read in the microcode

FMVError FMV450RunMPEGCode( int32 codeSignal )
{
	int32 waitResult, status = 0L;

	/* the driver installer task accesses the filesystem on behalf of the driver */
	/* set up signalling mechanism */
	gClientTaskItem = CURRENTTASK->t.n_Item;
	gSignalLoaded = SuperAllocSignal( 0L );
	if( !gSignalLoaded )
	{
		DEBUGP((" couldn't allocate gSignalLoaded\n"));
		return( (FMVError) -1L );
	}
	/* signal the installer task to load the microcode */
	SuperInternalSignal( gMainTask, codeSignal );

	/* wait for the installer task to complete the load */
	waitResult = SuperWaitSignal( gSignalLoaded );
	if( waitResult & SIGF_ABORT )
		goto abortCleanUp;

	/* boot code is loaded into gUcodeBuffer, run it */
	if( FMV450RunBootCode( (uint32 *) gUcodeBuffer ) < 0L )
		goto abortCleanUp;

	/* signal installer task to load real microcode */
	SuperInternalSignal( gMainTask, gSignalDone );

	/* wait for the installer task to complete the load */
	waitResult = SuperWaitSignal( gSignalLoaded );
	if( waitResult & SIGF_ABORT )
		goto abortCleanUp;

	/* real microcode is loaded, run it */
	if( FMV450RunCode( (uint32 *) gUcodeBuffer ) < 0L )
		goto abortCleanUp;

	/* signal installer task that we are done with buffer */
	SuperInternalSignal( gMainTask, gSignalDone );

	SuperFreeSignal( gSignalLoaded );

	return( (FMVError) status );

abortCleanUp:
	/* something bad happened, tell the installer task to clean up */
	SuperInternalSignal( gMainTask, SIGF_ABORT );
	SuperFreeSignal( gSignalLoaded );
	return( (FMVError) -1L );
}

/* FMV450RunMPEG16BitCode( ) loads and runs the 16 bit per pixel 3DO microcode */

FMVError FMV450RunMPEG16BitCode( )
{
	DEBUGP(("FMV450RunMPEG16BitCode\n"));

	return( FMV450RunMPEGCode( gSignalVideoMode16 ) );
}

/* FMV450RunMPEG16BitCode( ) loads and runs the standard 24 bit per pixel microcode */

FMVError FMV450RunMPEG24BitCode( )
{
	DEBUGP(("FMV450RunMPEG24BitCode\n"));

	return( FMV450RunMPEGCode( gSignalVideoMode24 ) );
}

void FMV450SetSystemClockDivisor( uint32 clockFrequency )
{
	uint32 scr2, divisor;

	/* calculate divisor from clockFrequency */
	divisor = clockFrequency / MPEGSYSTEMCLOCKFREQUENCY;

	/* put the right bits in the right place */
	scr2 = ((divisor << 3) & SCR2_DIVISORMSK) | SCR2_GLCKSELECT;

	FMVWriteVideoRegister( HOST_SCR2, scr2 );
}

uint32 FMV450ReadSystemClockReference( )
{
	uint32 result;

	/* these reads have to occur in this order */
	result = FMVReadVideoRegister( HOST_SCR0 ) & SCR0_SYSCLKLMSK;
	result |= (FMVReadVideoRegister( HOST_SCR1 ) & SCR1_SYSCLKMMSK) << 15;
	result |= (FMVReadVideoRegister( HOST_SCR2 ) & SCR2_SYSCLKHMSK) << 30;

	return( result );
}

void FMV450WriteSystemClockReference( uint32 currentSCR )
{
	uint32 scr2;

	/* save the state of the scr2 register */
	scr2 = FMVReadVideoRegister( HOST_SCR2 );

	/* put the right bits in the right place */
	scr2 = (scr2 & ~SCR2_SYSCLKHMSK) | ((currentSCR >> 30) & SCR2_SYSCLKHMSK);

	/* these writes have to occur in this order */
	FMVWriteVideoRegister( HOST_SCR2, scr2 );
	FMVWriteVideoRegister( HOST_SCR1, ((currentSCR >> 15) & SCR1_SYSCLKMMSK) );
	FMVWriteVideoRegister( HOST_SCR0, currentSCR & SCR0_SYSCLKLMSK );
}

// read presentation time stamp from CL450
// bit 15 of the CL450 HMEM register 6 is set if the pts is valid
// HMEM[6] bits 1:0		=		PTS bits 31:30
// HMEM[5] bits 14:0	=		PTS bits 29:15
// HMEM[4] bits 14:0	=		PTS bits 14:0
// the high bits of HMEM[5] and HMEM[4] must be masked

int32 FMVVidDecReadPTS( uint32 *resultPTS )
{
	uint32 pts3130, pts2915, pts1400, i;

	// read pts
	FMVWriteVideoRegister( HOST_RADDR, 6 );
	pts3130 = FMVReadVideoRegister( HOST_RDATA );

	// see if a valid pts is available
	if( pts3130 & 0x8000L )					// bit 15 is set if pts is valid
	{
		FMVWriteVideoRegister( HOST_RADDR, 7 );
		pts2915 = FMVReadVideoRegister( HOST_RDATA ) & 0x7fffL;
		FMVWriteVideoRegister( HOST_RADDR, 8 );
		pts1400 = FMVReadVideoRegister( HOST_RDATA ) & 0x7fffL;

		*resultPTS = (pts3130 << 30) | (pts2915 << 15) | pts1400;

#ifdef PTS_DEBUG
		// log PTSs for debugging
		CL450StatusInfo.lastVidPTS = *resultPTS;
		i = CL450StatusInfo.vidPTSOutCount;
		CL450StatusInfo.vidPTSLog[ i ].outPTS = *resultPTS;
		CL450StatusInfo.vidPTSLog[ i ].delta = *resultPTS - CL450StatusInfo.vidPTSLog[ i - 1 ].outPTS;
		CL450StatusInfo.lastVidPTS = *resultPTS;
		if( ++CL450StatusInfo.vidPTSOutCount >= VIDPTSLOGSIZE )
			CL450StatusInfo.vidPTSOutCount = 0L;
#endif
		return( 1L );						// indicate pts is valid
	}
	return( 0L );							// indicate pts is not valid
}

void FMVVidDecReadSCR( uint32 *resultSCR, uint32 *count )
{
	uint32 scr3130, scr2915, scr1400;

	// read scr
	FMVWriteVideoRegister( HOST_RADDR, 0xcL );
	scr3130 = FMVReadVideoRegister( HOST_RDATA );
	FMVWriteVideoRegister( HOST_RADDR, 0xdL );
	scr2915 = FMVReadVideoRegister( HOST_RDATA ) & 0x7fffL;
	FMVWriteVideoRegister( HOST_RADDR, 0xeL );
	scr1400 = FMVReadVideoRegister( HOST_RDATA ) & 0x7fffL;

	*resultSCR = (scr3130 << 30) | (scr2915 << 15) | scr1400;

	// read count
	FMVWriteVideoRegister( HOST_RADDR, 0xfL );
	*count = FMVReadVideoRegister( HOST_RDATA );
}

void FMV450WriteIndirectVideoRegister( uint32 vreg, uint32 value )
{
	FMVWriteVideoRegister( VID_CONTROL, (vreg << 1) & VID_VRIDMSK );
	FMVWriteVideoRegister( VID_REGDATA, value);
}

uint32 FMV450ReadIndirectVideoRegister( uint32 vreg )
{
	FMVWriteVideoRegister( VID_CONTROL, (vreg << 1) & 0x1eL );
	return( FMVReadVideoRegister( VID_REGDATA ) & 0xffffL );
}

void FMV450SetRGBConversionCoefficients( uint32 k0, uint32 k1, uint32 k2, uint32 k3 )
{
	uint32 temp;

	temp = k0 << 8 | k1;
	FMV450WriteIndirectVideoRegister( VID_SELA, temp );
	temp = k2 << 8 | k3;
	FMV450WriteIndirectVideoRegister( VID_SELB, temp );
}

int32 FMVVidDecInit( int32 pixelMode, int32 resampleMode, int32 xOffset, int32 yOffset, int32 xSize, int32 ySize )
{
	uint32 temp;
	extern int32 HPWidth, frontBorder, topBorder, fudge;

	DEBUGP(("FMVVidDecInit, &CL450StatusInfo = 0x%08lx\n",&CL450StatusInfo));

	/* work around a CL450 bug */
	/* the CL450 occasionally hangs if flushed before any data has been */
	/* sent. we'll work around this (until C-Cube fixes it) by not flushing */
	/* unless we have sent a couple of new packet commands */
	CL450StatusInfo.dontFlush = 2L;

	FMVReset450();											/* soft CL450 reset */
	FMVWriteVideoRegister( HOST_NEWCMD, 0L );				/* clear new command */
//	FMVWriteVideoRegister( HOST_CONTROL, HCTRL_NOTINT );	/* clear interrupt */

	/* set registers to correct initial state */
	FMVWriteVideoRegister( CMEM_DMACTRL, CMDMA_1QE );		/* assert CFLEVEL on 4+ empty entries */
	FMV450SetSystemClockDivisor( HLWD_GCLK );				/* derives MPEG clock from global clock */
	temp = DRAM_RFRSHCNT & DRAM_REFCNTMSK;
	FMVWriteVideoRegister( DRAM_REFCNT, temp );				/* DRAM refresh count */

	if( pixelMode == 16L )
	{
		if( FMV450RunMPEG16BitCode() )						/* load microcode and begin execution */
			return( -1 );
	}
	else if( pixelMode == 24L )
	{
		if( FMV450RunMPEG24BitCode() )						/* load microcode and begin execution */
			return( -1 );
	}
	if( FMV450SetInterruptMask(0x0L) )
		return( -1 );

	temp = FMVReadVideoRegister( CMEM_DMACTRL );
	DEBUGP((" CMEM_DMACTRL = %08lx\n",temp));

	if( FMVVidDecSetEnabledInterrupts( VINT_SCOMPLETE | VINT_BUFFEMPTY |
									   VINT_NEWPICTURE | VINT_DATAERROR |
									   VINT_SCOMPLETE | VINT_PICDECODED ) )
		return( -1 );

	if( pixelMode == 16L )
	{
		if( FMVVidDecSetWindow( xOffset, yOffset,
							 xSize << 1, ySize ) )
			return( -1 );
		if( FMV450SetBorderRGB( frontBorder - HPWidth - 2, topBorder * (xSize >> 6L) + fudge,
								CL450RBORDERCOLOR, CL450GBORDERCOLOR, CL450BBORDERCOLOR ) )
			return( -1 );
	}
	else if( pixelMode == 24L )
	{
		switch( resampleMode )
		{
			case kCODEC_SQUARE_RESAMPLE:
				break;
			case kCODEC_NTSC_RESAMPLE:
			case kCODEC_PAL_RESAMPLE:
				xSize = WDYPPHSYNC24REN >> 1;
				break;
		}
		if( FMVVidDecSetWindow( xOffset, yOffset,
							 xSize << 1, ySize /* + fudge */ ) )
			return( -1 );
		if( FMV450SetBorderRGB( frontBorder - HPWidth - 2, topBorder,
								CL450RBORDERCOLOR, CL450GBORDERCOLOR, CL450BBORDERCOLOR ) )
			return( -1 );
	}
	if( FMVVidDecSetVideoRate50() )
		return( -1 );
	if( FMV450SetRGBColorMode() )
		return( -1 );

	return( 0L );
}

int32 FMVVidDecClose(void)
{
	return( 0L );
}

int32 FMVVidDecOpen(int32 pixelMode, int32 resampleMode, int32 xOffset, int32 yOffset, int32 xSize, int32 ySize)
{
	return( 0L );
}

int32 FMVVidDecDeviceChange(int32 pixelMode, int32 resampleMode, int32 xOffset, int32 yOffset, int32 xSize, int32 ySize)
{
	return( 0L );
}

void FMVVidDecSetSCR(uint32 theSCR)
{
}

uint32 FMVVidDecGetSCR(void)
{
	return( 0L );
}

/* sugar routines for CL450 commands */

/* inquirebufferfullness command */
int32 FMVVidDecInquireBufferFullness()
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_INQBUFFER,
							 dummy, dummy, dummy, dummy ) );
}

/* setthreshold command */
int32 FMV450SetThreshold( int32 threshold )
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_SETTHRESH, threshold,
							 dummy, dummy, dummy ) );
}

/* setinterruptmask command */
int32 FMV450SetInterruptMask( int32 mask )
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_SETINTMASK, mask,
							 dummy, dummy, dummy ) );
}

/* setvideorate PAL command */
int32 FMVVidDecSetVideoRate50()
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_SETVIDRATE, 3L,
							 dummy, dummy, dummy ) );
}

/* setvideorate NTSC command */
int32 FMVVidDecSetVideoRate60()
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_SETVIDRATE, 4L,
							 dummy, dummy, dummy ) );
}

/* setwindow command */
int32 FMVVidDecSetWindow( uint32 xoff, uint32 yoff, uint32 width, uint32 height )
{
	return( FMVDo450Command( MPGV_SETWINDOW, xoff, yoff, width, height ) );
}

/* setborder command */
int32 FMV450SetBorderRGB( uint32 left, uint32 top, uint32 r, uint32 g, uint32 b )
{
	uint32 gb;
	gb = g << 8 | b;
	return( FMVDo450Command( MPGV_SETBORDER, left, top, r, gb ) );
}

// flushbitstream command
int32 FMV450FlushBitstream()
{
	uint32 dummy = 0L;

	return( FMVDo450Command( MPGV_FLUSHBS,
							 dummy, dummy, dummy, dummy ) );
}

// new bitstream
int32 FMVVidDecNewBitstream( void )
{
	/* work around a CL450 bug */
	/* the CL450 occasionally hangs if flushed before any data has been */
	/* sent. we'll work around this (until C-Cube fixes it) by not flushing */
	/* unless we have sent a couple of new packet commands */
	if( CL450StatusInfo.dontFlush )
		return( 0L );

	// flush bitstream twice to transition to a new stream
	FMV450FlushBitstream();
	return( FMV450FlushBitstream() );
}

/* newpacket command */
int32 FMVVidDecNewPacket( uint32 length, uint32 isValid, uint32 ts )
{
	int32 timeout = 0L, i;
	uint32 ts2 = 0L, ts1 = 0L, ts0 = 0L;

	/* work around a CL450 bug */
	/* the CL450 occasionally hangs if flushed before any data has been */
	/* sent. we'll work around this (until C-Cube fixes it) by not flushing */
	/* unless we have sent a couple of new packet commands */
	if( CL450StatusInfo.dontFlush )
		CL450StatusInfo.dontFlush--;

	if( isValid )
	{
		ts2 = ((ts >> 30) & 0x03L) | 0x8000L;
		ts1 = (ts >> 15) & 0x7fffL;
		ts0 = ts & 0x7fffL;

#ifdef PTS_DEBUG
		// log input timestamps for debugging
		i = CL450StatusInfo.vidPTSInCount;
		CL450StatusInfo.vidPTSLog[ i ].inPTS = ts;
		CL450StatusInfo.vidPTSLog[ i ].inLength = length;
		CL450StatusInfo.vidPTSLog[ i ].vidStreamOffset =
			CL450StatusInfo.vidPTSLog[ i - 1 ].vidStreamOffset + length;
		if( ++CL450StatusInfo.vidPTSInCount >= VIDPTSLOGSIZE )
			CL450StatusInfo.vidPTSInCount = 0L;
#endif
	}
	else
		ts2 = 0L;

	FMVDo450Command( MPGV_NEWPACKET, length, ts2, ts1, ts0 );
	while( FMVReadVideoRegister( HOST_NEWCMD ) & 0x01 )
	{
		// SuperSwitch();
		CL450StatusInfo.vidSwitchCount++;
		if( timeout++ > TIMEOUTLIMIT )
		{
			DEBUGP(("FMVVidDecNewPacket: timed out\n"));
			return( -1 );
		}
	}
	return( 0L );
}

/* play command */
int32 FMVVidDecPlay()
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_PLAY,
							 dummy, dummy, dummy, dummy ) );
}

/* pause command */
int32 FMVVidDecPause( uint32 frameType )
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_PAUSE,
							 dummy, dummy, dummy, dummy ) );
}

/* single step command */
int32 FMVVidDecSingleStep()
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_SINGLESTEP,
							 dummy, dummy, dummy, dummy ) );
}

/* setcolormode( RGB ) command */
int32 FMV450SetRGBColorMode()
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_SETCLRMODE, VID_MODERGB,
							 dummy, dummy, dummy ) );
}

/* setcolormode( YCrCb ) command */
int32 FMV450SetYCrCbColorMode()
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_SETCLRMODE, VID_MODEYCRCB,
							 dummy, dummy, dummy ) );
}

// skip B-pictures command
// skips count B-pictures, returns 0 if command is accepted, count if deferred
// if non-zero is returned, this routine should be called at a later time
int32 FMVVidDecSkipFrames( int32 count )
{
	// it is only legal to write HMEM[5] if it is currently 0
	FMVWriteVideoRegister( HOST_RADDR, 5 );
	if( FMVReadVideoRegister( HOST_RDATA ) )
		return( count );

	// write the count to HMEM[5]
	FMVWriteVideoRegister( HOST_RADDR, 5 );
	FMVWriteVideoRegister( HOST_RDATA, count );

	return( 0L );
}

// decode I pictures command
int32	FMVVidDecKeyFrames( void )
{
	uint32 dummy = 0L;
	return( FMVDo450Command( MPGV_SCAN,
							 dummy, dummy, dummy, dummy ) );
}

/* get status register value */
uint32 FMV450GetStatusRegister( uint32 statusRegister )
{
	FMVWriteVideoRegister( HOST_RADDR, statusRegister );
	return( FMVReadVideoRegister( HOST_RDATA ) );
}

/* clear interrupt status register to enable interrupts */
void FMV450ClearInterruptStatus( void )
{
	FMVWriteVideoRegister( HOST_RADDR, CSTAT_INTSTAT );
	FMVWriteVideoRegister( HOST_RDATA, 0L );
}

// clear interrupt
void FMVVidDecClearInterrupt( void )
{
	uint32 hcntrlreg;

	// clear NOT INT bit in host control register
	hcntrlreg = FMVReadVideoRegister( HOST_CONTROL );
	hcntrlreg |= HCTRL_NOTINT;
	FMVWriteVideoRegister( HOST_CONTROL, hcntrlreg );
}

// Handle interrupt stuff

uint32 gCL450InterruptMask = 0L;

// enable a set of interrupts
// disables others

int32 FMVVidDecSetEnabledInterrupts( uint32 interruptMask )
{
	// clear interrupt bit in host control register
	FMVVidDecClearInterrupt();

	// clear interrupt status register
	FMV450ClearInterruptStatus();

	// set interrupt mask
	gCL450InterruptMask = interruptMask;

	return( FMV450SetInterruptMask( interruptMask ) );
}

// enable interrupts, maintain current enabled interrupts
int32 FMVVidDecEnableInterrupts( uint32 interruptsToEnable )
{
	gCL450InterruptMask |= interruptsToEnable;
	return( FMV450SetInterruptMask( gCL450InterruptMask ) );
}

// disable interrupts, maintain other currently enabled interrupts
int32 FMVVidDecDisableInterrupts( uint32 interruptsToDisable )
{
	gCL450InterruptMask &= ~interruptsToDisable;
	return( FMV450SetInterruptMask( gCL450InterruptMask ) );
}

// check interrupts
uint32 FMVVidDecCheckInterrupt( void )
{
	return( FMV450GetStatusRegister( CSTAT_INTSTAT ) );
}

/* enable vsync interrupt */
void FMVVidDecEnableVSInterrupt( void )
{
	WdySetControlSet( WDYGPIO0 );		/* enables vsync interrupt rework */
}

/* check vsync interrupt */
int32 FMVVidDecCheckVSInterrupt( void )
{
	return( WdyGetControlSet() & WDYGPIO1 );
}

/* clear vsync interrupt */
void FMVVidDecClearVSInterrupt( void )
{
	WdySetControlClear( WDYGPIO0 );		/* disables vsync interrupt rework */
}
