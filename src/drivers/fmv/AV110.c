/*
	File:		AV110.c

	Contains:	xxx put contents here xxx

	Written by:	George Mitsuoka

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<36>	 12/6/94	GM		Delmar conditionalized some Mia stuff.
		<35>	 12/5/94	GM		Added handling of SIGF_ABORT after all calls to SuperWaitSignal.
		<34>	 10/3/94	DM		Changed the pcm_div value from 6 to 7 based on inputs from Bill
									Dawson concerning the 60kHz sustained rate limitation on the
									current 2.3 silicon.
		<33>	 9/14/94	DM		Speed up the output sample frequency divisor.
		<32>	 9/12/94	DM		Fixed the way enables are enabled and added the sampling
									frequency change interrupt.
		<31>	 8/26/94	GM		Stopped printing a message if the audio patch files where not
									loaded.
		<30>	 8/24/94	DM		Modified a separate reset routine for analog and adjusted the
									init so I don't affect the mpeg.
		<29>	 8/16/94	DM		Changed  Reset sequence in FMVResetAV110 for Mia stuff only.
		<28>	 8/10/94	GM		Fixed enabling of high byte interrupts (was setting bits greater
									than bit number 7).
		<27>	 7/12/94	GM		Fixed allocation/freeing of signals.
		<26>	 5/27/94	DM		Add some code for Mia to wait for a RESET and RESTART to finish
									before continuing.  The flag should zero out when done.
		<25>	 5/11/94	DM		Add condition build flag for Mia build.
		<24>	  5/6/94	GDW		Audio clk 8 is now generally supported.
		<23>	 4/25/94	BCK		Added PTS interrupt control routines and moved initialization of
									the interrupt to be initiated from FMVAudioInit.
		<22>	 4/20/94	GDW		Added new option for switching audio clock.
		<21>	 4/19/94	BCK		Changed error handling mode from repeat, which is not valid if
									not using DRAM, to Mute mode.
		<20>	  4/5/94	BCK		Removed invisble characters introduced during the check-in
									process.
		<19>	  4/5/94	BCK		Using FMVReadAV110Register(register) to mask the register reads
									to 8 bits. Added FMVAV110ReadPTS() to read the PTS value.
		<18>	  4/4/94	GM		Fixed FMVAV110Init to initialize the INTR_ENL register even if
									HWINTPTS is not #defined.
		<17>	 3/27/94	GM		Changed intialization code to work around a Woody reset bug. The
									SBOF feature cannot be used.
		<16>	  3/8/94	BCK		Changed FMV_AUDIO_SYSTEM_STREAM to FMV_SYSTEM_STREAM in a chunk
									of code for HW PTSes only.
		<15>	  3/7/94	GM		Freed allocated signal in patch code.
		<14>	  3/5/94	GM		Added support for loading AV110 microcode patch files.
		<13>	  3/3/94	GDW		Fixed Kelly defines.
		<12>	 2/25/94	GDW		Played the name game.
		<11>	  2/3/94	GDW		Added sw/hw build flags for PTS code.
		<10>	  2/2/94	GDW		Fixed lost of first PTS bug.
		 <9>	 12/8/93	GDW		Removed interrupt code.
		 <8>	 12/3/93	GDW		Added code to enable TI PTS interrupt.
		 <7>	 12/1/93	GDW		Conditionalized MIA code.
		 <6>	11/23/93	JW		Added restart to Init routine.
		 <5>	11/22/93	GDW		Hits registers in Kelly for ByPass mode.
		 <4>	11/22/93	DM		Fix serial/paral bug in Init routine.
		 <3>	11/22/93	GDW		Added audio bypass mode.
		 <2>	11/19/93	GDW		Added new functions for additional stream support.
*/

/* file: AV110.c */
/* TI TMS320AV110 initialization and control */
/* 4/24/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#include "types.h"
#include "kernel.h"
#include "task.h"

#include "FMV.h"
#include "Hollywood.h"
#include "AV110.h"
#include "Woody.h"
#if MIABUILD == 1
#include "Kelly.h"
#endif
#include "FMVDriver.h"


extern uint32 gAudioDataFormat;
void FMVAnalogResetAV110()
{
#if MIABUILD == 1
	int32	i ;
	uint32	readData ;
	bool	waitResetDone ;

//	Let the clocks thru
	FMVWriteAudioRegister( PLAY, 1L);					// AV110  Play on
	FMVWriteAudioRegister( MUTE, 0L);					// AV110  Mute off

//	Flush the bit buffer in Kelly
	FMVWriteAudioRegister( kRegReset, kDebugAudReset);	// Pulse Kelly reset audio set
	FMVWriteAudioRegister( kRegReset, 0L);				// Pulse Kelly reset audio clear
	FMVWriteAudioRegister( kRegAudCntrl, 0L);			// Kelly Audio Play off
	for(i=0;i<10;i++) 									// wait 2 microseconds to let transition occur
		readData =FMVReadAudioRegister(kRegKellyVersion) ;

//	Force a pin reset
	WdySetControlSet( WDYRESETAUDIO );
	waitResetDone = FALSE ;
	while(!waitResetDone)
	{
		readData = FMVReadAudioRegister(RESET) ;
		if((readData & 0x00000001) == 0)
			waitResetDone = TRUE ;
	}
//	Turn the clocks back on
	FMVWriteAudioRegister( PLAY, 1L);					// AV110  Play on
	FMVWriteAudioRegister( MUTE, 0L);					// AV110  Mute off

	FMVWriteAudioRegister( kRegReset, kDebugAudReset);	// Kelly reset audio set
	for(i=0;i<10;i++) 									// wait awhile
		readData =FMVReadAudioRegister(kRegKellyVersion) ;
	FMVWriteAudioRegister( kRegReset, 0L);				// Kelly reset audio clear
#endif
}

void FMVResetAV110()
{
	/* hard reset */
	WdySetControlSet( WDYRESETAUDIO );
	/* soft reset */
	FMVWriteAudioRegister( RESET, 1L );
}

/* load TI patch code from buffer */
int32 FMVAV110PatchDecoder( uint32 *buffer )
{
	int32 i, temp;

	/* load the patch by repeatedly writing the patch register */
	for( i = 0; i < PATCH_SIZE; i++ )
		FMVWriteAudioRegister( PATCH, buffer[ i ] );

	/* activate the patch by repeatedly reading the patch register (great, huh?) */
	for( i = 0; i < PATCH_SIZE; i++ )
		temp = FMVReadAV110Register( PATCH );

	return( 0L );
}

/* load TI patch code from file */
int32 FMVAV110LoadPatch( int32 patchNumber )
{
	int32 waitResult,status=0L;

	/* the driver installer task accesses the filesystem on behalf of the driver */
	/* set up signalling mechanism */
	gClientTaskItem = CURRENTTASK->t.n_Item;
	if( ((gSignalLoaded = SuperAllocSignal( 0 )) == 0L) ||
		((gSignalLoadError = SuperAllocSignal( 0 )) == 0L) )
	{
		DEBUGP((" couldn't allocate audioSignals\n"));
		goto abortFreeSignals;
	}
	/* signal the installer task to load the microcode patch */
	if( patchNumber == 1 )
		SuperInternalSignal( gMainTask, gSignalAudioPatch1 );
	else if( patchNumber == 2 )
		SuperInternalSignal( gMainTask, gSignalAudioPatch2 );

	/* wait for the installer task to complete the load */
	waitResult = SuperWaitSignal( gSignalLoaded | gSignalLoadError );

	if( waitResult & gSignalLoaded )
	{
		/* patch microcode is loaded, load it in the AV110 */
		if( FMVAV110PatchDecoder( (uint32 *) gUcodeBuffer ) < 0L )
			status = -1L;

		/* signal installer task that we are done with buffer */
		SuperInternalSignal( gMainTask, gSignalDone );
	}
	/* if we get a gSignalLoadError, the microcode file wasn't loaded */
	if( waitResult & (gSignalLoadError | SIGF_ABORT) )
	{
		status = -1L;
		goto abortFreeSignals;
	}
abortFreeSignals:
	if( gSignalLoaded )
		SuperFreeSignal( gSignalLoaded );
	if( gSignalLoadError )
		SuperFreeSignal( gSignalLoadError );

	return( status );
}

/* set up av110 registers as specified by TI */

//==============================================================================
// Note:	If the default audio data ever changes make sure to change the
//			assignment of the global variable "gAudioDataFormat" located in
//			"FMVVideoDriver.c" to match.
//==============================================================================

// DON'T MUCK WITH THE INITIALIZATION CODE UNLESS YOU ARE SURE YOU KNOW
// WHAT YOU ARE DOING!
// IN PARTICULAR, DO NOT CHANGE THE ORDER OF REGISTER WRITES AND DO NOT
// USE THE SBOF FEATURE IN WOODY AS IT IS BROKEN.

void FMVAV110Init(uint32 dataFormat)
{
#if MIABUILD == 1
	if ( dataFormat != FMV_AUDIO_BYPASS ) {
#endif
	FMVResetAV110();						/* reset TI audio decoder */
	FMVAV110LoadPatch( 1L );				/* load patches */
	FMVAV110LoadPatch( 2L );				/* max of two patches */
#if MIABUILD == 1
	} else {
		FMVAnalogResetAV110() ;					/* for analog mode */
	}
#endif

	FMVWriteAudioRegister( INTR_ENH, 0L);	/* disable interrupts */
	FMVWriteAudioRegister( INTR_ENL, 0L);	/* disable interrupts */

	FMVAV110SetOutputSampleFrequency( HLWD_AUDIOFREQ );
	FMVWriteAudioRegister( PCM_ORD, 0L);	/* MSB first */
	FMVWriteAudioRegister( PCM_18, 0L);		/* 16 bit audio data */
	FMVWriteAudioRegister( STR_SEL, dataFormat);	/* Except the specified stream */
	FMVWriteAudioRegister( CRC_ECM, 0L);	/* ignore CRC errors */
	FMVWriteAudioRegister( SYNC_ECM, ERR_MUTE);	/* mute on sync loss */

#if MIABUILD == 1
	if ( dataFormat != FMV_AUDIO_BYPASS ) {
#endif
	FMVWriteAudioRegister( MUTE, 1L);		/* mute output, enable pcm clock */
	FMVWriteAudioRegister( PLAY, 0L);		/* disable output */
#if MIABUILD == 1
	}
#endif

	FMVWriteAudioRegister( SKIP, 0L);		/* don't skip next frame */
	FMVWriteAudioRegister( REPEAT, 0L);		/* don't repeat next frame */
	FMVWriteAudioRegister( RESTART, 0L);	/* don't flush buffers */

	FMVWriteAudioRegister( LATENCY, 0L);	/* low latency */

	FMVWriteAudioRegister( IRC_LOAD, 0L);	/* don't load clock */
	FMVWriteAudioRegister( DRAM_EXT, 1L);	/* external memory */

	FMVWriteAudioRegister( SYNC_LOCK, 0L);	/* in sync after first good sync word found */
	if ( dataFormat == FMV_AUDIO_BYPASS ) {
		FMVWriteAudioRegister( SIN_EN, 1L);	/* serial data input */
	} else {
		FMVWriteAudioRegister( SIN_EN, 0L);	/* parallel data input */
	}

	FMVWriteAudioRegister( ATTEN_L, 0L);	/* don't attenuate left channel */
	FMVWriteAudioRegister( ATTEN_R, 0L);	/* don't attenuate right channel */
	FMVWriteAudioRegister( AUD_ID_EN, 0L);	/* ignore audio stream id */
	FMVWriteAudioRegister( FREE_FORMH, 0L);	/* not free format */
	FMVWriteAudioRegister( FREE_FORML, 0L);	/* not free format */

	FMVWriteAudioRegister( RESET, 1L );			/* @#$ TI */
}

/* set the output pcm clock divider based on the Hollywood audio clock */

void FMVAV110SetOutputSampleFrequency( uint32 outputFrequency )
{
	int32 TCR;
	extern uint32  gAudioClk;

	if ( gAudioClk ) {
		TCR = 12L;					// Some Mia I and II boards have new clocks
	} else {
//		TCR = 8L;
//		TCR = 6L;
		TCR = 7L;	// changed value due to 60kHz limitation on 2.3 silicon
	}
//	/* calculate timer countdown register value (16.16 format) */
//	TCR = HLWD_ACLCK / ( outputFrequency * HLWD_ASYSCLKRATIO );

//	TCR = 12L;
	FMVWriteAudioRegister( PCM_DIV, TCR );
}

void FMVAV110StartDecoder()
{
	FMVWriteAudioRegister( PLAY, 1L);		/* enable output */
	FMVWriteAudioRegister( MUTE, 0L);		/* don't mute */
#if MIABUILD == 1
	if ( gAudioDataFormat == FMV_AUDIO_BYPASS )
	{
		FMVWriteAudioRegister( kRegAudCntrl, 1L);	// Turn Kelly ADC play thru on
	}
#endif
}

void FMVAV110StopDecoder()
{
//	FMVWriteAudioRegister( MUTE, 1L);		/* mute */
	FMVWriteAudioRegister( PLAY, 0L);		/* disable output */
#if MIABUILD == 1
	if ( gAudioDataFormat == FMV_AUDIO_BYPASS )
	{
		FMVWriteAudioRegister( kRegAudCntrl, 0L);	// Turn Kelly ADC play thru on
	}
	FMVWriteAudioRegister( RESET, 1L );	// reset av110
#endif
}

void FMVAV110MuteDecoder()
{
	FMVWriteAudioRegister( MUTE, 1L);		/* mute */
}

void FMVAV110EnableInputAlmostEmptyInterrupt()
{
#if MIABUILD == 1
	uint32	readData ;
	readData = FMVReadAudioRegister(INTR_ENL) & 0x000000FF ;
	readData |= INT_BALE ;
	FMVWriteAudioRegister( INTR_ENL, readData );
#else
	FMVWriteAudioRegister( INTR_ENL, INT_BALE );
#endif
}

void FMVAV110EnableInputAlmostFullInterrupt()
{
#if MIABUILD == 1
	uint32	readData ;
	readData = FMVReadAudioRegister(INTR_ENL) & 0x000000FF ;
	readData |= INT_BALF ;
	FMVWriteAudioRegister( INTR_ENL, readData );
#else
	FMVWriteAudioRegister( INTR_ENL, INT_BALF );
#endif
}

void FMVAV110EnableOutputUnderflowInterrupt()
{
#if MIABUILD == 1
	uint32	readData ;
	readData = FMVReadAudioRegister(INTR_ENH) & 0x000000FF ;
	readData |= (INT_OUT_UFLOW >> 8) ;
	FMVWriteAudioRegister( INTR_ENH, readData );
#else
	FMVWriteAudioRegister( INTR_ENH, INT_OUT_UFLOW >> 8);
#endif
}

void FMVAV110EnableEndOfStreamInterrupt()
{
#if MIABUILD == 1
	uint32	readData ;
	readData = FMVReadAudioRegister(INTR_ENH) & 0x000000FF ;
	readData |= (INT_EOS_FOUND >> 8) ;
	FMVWriteAudioRegister( INTR_ENH, readData );
#else
	FMVWriteAudioRegister( INTR_ENH, INT_EOS_FOUND >> 8);
#endif
}

void FMVAV110EnableValidHeaderInterrupt()
{
#if MIABUILD == 1
	uint32	readData ;
	readData = FMVReadAudioRegister(INTR_ENL) & 0x000000FF ;
	readData |= INT_HEADER ;
	FMVWriteAudioRegister( INTR_ENL, readData );
#else
	FMVWriteAudioRegister( INTR_ENL, INT_HEADER );
#endif
}

void FMVAV110InitPTSInterrupt()
{
	WdySetControlSet( WDYGPIODIR0 );			// Enable GPIO 0 for PTS interrupts
	WdySetControlSet(WDYGPIO0);					// Initially clear the interrupt
	WdySetControlClear(WDYGPIO0);
}

void FMVAV110SetupPTSInterrupt(int32 enable)
{
#if MIABUILD == 1
	uint32 readData ;
#endif
	if(enable)
#if MIABUILD == 1
	{
		readData = FMVReadAudioRegister(INTR_ENL) ;
		readData |= INT_PTS ;
		FMVWriteAudioRegister( INTR_ENL, readData) ;
	}
#else
		FMVWriteAudioRegister( INTR_ENL, INT_PTS);	/* Enable the PTS interrupt */
#endif
	else
#if MIABUILD == 1
	{
		readData = FMVReadAudioRegister(INTR_ENL) ;
		readData &= (~INT_PTS) ;
		FMVWriteAudioRegister( INTR_ENL, readData) ;
	}
#else
		FMVWriteAudioRegister( INTR_ENL, 0L);		/* Disable the PTS interrupt */
#endif
	return;
}

#if MIABUILD == 1
void FMVAV110EnableFSModInterrupt()
{
	uint32	readData ;
	readData = FMVReadAudioRegister(INTR_ENH) & 0x000000FF;
	readData |= (INT_FS_MOD >> 8) ;
	FMVWriteAudioRegister( INTR_ENH, readData);
}
#endif

void FMVAV110ClearInterrupt()
{
	WdySetControlSet(WDYGPIO0);					// Clear the interrupt in woody.
	WdySetControlClear(WDYGPIO0);
}

int32 FMVAV110ReadIntStatus()
{
	int32 result = 0L;

	result = (FMVReadAV110Register( INTRH ) & 0xff) << 8L;
	result |= FMVReadAV110Register( INTRL ) & 0xff;

	return( result );
}

int32 FMVAV110ReadInBuffCount( )
{
	int32 result = 0L;

	result = (FMVReadAV110Register( BUFFH ) & 0xff) << 8L;
	result |= FMVReadAV110Register( BUFFL ) & 0xff;

	return( result );
}

uint32	FMVAV110ReadPTS(int32* highBit)
{
	uint32 thePTS;
	uint32 temp;

	thePTS = FMVReadAV110Register(APTS0);				// Read bits 0:7
	thePTS |= FMVReadAV110Register(APTS1) << 8;			// Read bits 8:15
	thePTS |= FMVReadAV110Register(APTS2) << 16;		// Read bits 16:23
	thePTS |= FMVReadAV110Register(APTS3) << 24;		// Read bits 24:31
	temp = FMVReadAV110Register(APTS4);					// Read bit 32
	if( (temp & 0x1) != 0)
		*highBit = true;
	else
		*highBit = false;

	return(thePTS);
}

void FMVAV110SendEOS()
{
	FMVWriteAudioRegister( EOS, 1L );
}


void FMVAV110SetAudioStream()
{
	FMVAV110Init( STREAM_AUDIO );
}

void FMVAV110SetPacketStream()
{
	FMVAV110Init( STREAM_PACKET );
}

void FMVAV110SetSystemStream()
{
	FMVAV110Init( STREAM_SYSTEM );
}

#if MIABUILD == 1
void FMVAV110SetByPass()
{
	FMVAV110Init( STREAM_BYPASS );
}
#endif
