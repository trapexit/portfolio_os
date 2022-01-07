/*
	File:		FMVAudioDriver.c

	Contains:	FMV Audio Driver interrupt and I/O Request handler.

	  Public Routines
		FMVAudioInit - Initializes the audio device.
		FMVAudioDevOpen - Opens the audio device.
		FMVAudioDevClose - Closes the audio device.
		FMVAudioAbortIO - Result of an I/O Request abort command.
		FMVAudioCmdStatus - Result of an I/O Request status command.
		FMVAudioCmdControl - Result of an I/O Request control command.
		FMVAudioCmdRead - Result of an I/O Request read command.
		FMVAudioCmdWrite - Result of an I/O Request write command.
		FMVAudioCmdPlay - Initiates write and read DMAs and starts decompression.

	  Private Routines

		InitializePTSSaving - Initialize the PTS saving mechanism.
		SavePTSForNextRead - Save the PTS for the next Read.
		SetPTSFromSaved - Sets the PTS fields of the I/O Request from the saved PTS values.
		SetPTS - Sets the PTS fields of the I/O Request directly.

		QueueRead - Queues a read request.
		NextReadReq - Removes the request from the head of the read queue and returns it.
		FMVAudioSetUpNextRead - Sets up and starts the next read DMA.
		FMVAudioDMAtoRAMIntr - Interrupt handler for DMA read completes.

		QueueWrite - Queues a write request.
		NextWriteReq - Removes the request from the head of the write queue and returns it.
		FMVAudioSetUpNextWrite - Sets up and starts the next read DMA.
		FMVAudioDMAfrRAMIntr - Interrupt handler for DMA write completes.

		FMVAudioIntr - Interrupt handler for FMV board audio interrupts.


	Written by:	George Mitsuoka

	Copyright:	й 1993, 1994, by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<57>	 12/6/94	GM		Removed #include of L64111.h
		<56>	10/27/94	DM		Changed the context of calling the forage driver to be local to
									the open call rather than at driver init time.
		<55>	10/13/94	DM		Add disable/enable to slow bus interrupt when interrupt handler
									is installed.
		<54>	10/12/94	GDW		Add an initialize of the audio sample rate.
		<53>	 10/4/94	DM		Added global for gMPEGAudioDevOpen so we can filter when some of
									the interrupt activity should be executing.  Possible conflict
									with forage device driver since we share the INT_PD line.
		<52>	 9/27/94	DM		Added global interrupt flags to detect PCM underflow, buffer
									almost empty, and buffer almost full, but I disabled them from
									being activated at this time.  Only sampling frequency change is
									detected.  The audio interrupt in Kelly is now enabled.
		<51>	 9/14/94	DM		Fixed the audio clock selection on the dev station.  0 for 48kHz
									and 1 for 44.1kHz.
		<50>	 9/12/94	DM		Added support for 48kHz streams.  Detected in the Audio
									Interrupt and the current io read request is terminated and the
									ioreq flags now contain a flag indicating the sampling
									frequency.  The application must now send a cmd control to
									change the sampling frequency.
		<49>	 8/10/94	GM		Enabled and now handle the AV110 PCM output underflow interrupt.
									This is used in an attempt to detect and correct (by reset) an
									AV110 bug which causes channel swapping and audio distortion.
									Added reset after discontinuity to help prevent audio glitches.
									Added new status fields.
		<48>	  8/3/94	GDW		Added code to activate audio pts interrupt for MIA hardware.
		<47>	 7/29/94	GM		Added increment of writes completed to abort routine.
		<46>	 7/18/94	GM		Removed set up of next DMA write from abort code to reduce
									interrupt latency.
		<45>	 7/12/94	GM		Added some debugging code.
		<44>	  7/6/94	GDW		Clears the IO_CHICK flag on reads and writes.
		<43>	 6/21/94	GDW		Put marker back in.
		<42>	 4/25/94	BCK		Using PTS interrupt control routines now in AV110.c and moved
									initialization of the interrupt (in Woody.c) to be initiated
									from FMVAudioInit.
		<41>	 4/25/94	BCK		Added mods identified in Design Review. Made a set of routines
									for accessing the PTS info.
		<40>	 4/15/94	GDW		PlayThrough variable name changed.
		<39>	 4/11/94	BCK		Added comments and made interrupt routines static.
		<38>	  4/8/94	BCK		Streamlined HW PTS routine. Added even more comments.
									Conditionally compiled out resuming reads when the last write
									request is aborted.
		<37>	  4/7/94	BCK		Added more comments and fixed conditionally compiled out some
									currently unused code.
		<36>	  4/7/94	BCK		Fixed some HW PTS bugs. Added lots of comments, including a
									Functional Overview.
		<35>	  4/5/94	BCK		Changed from FMVTimestamps to FMVIOReqOptions structure.
									Added additional handling of abort case during read halting.
		<34>	  4/4/94	GM		Fixed disabling/enabling of interrupts to be matched and not
									extraneous. Corrects a hanging bug.
		<33>	 3/31/94	BCK		Fixed a remotely possible problem that could hang the driver if
									a write completes within a few instructions after DMA enabled.
									Added comments about assumptions.
		<32>	 3/30/94	BCK		Added disable-enable pair around usage of
									gCurrentMPEGAudioReadReq in FMVAudioSetupNextWrite(). Fixed PTS
									setting in QueueRead().
		<31>	 3/30/94	GM		Removed stopping of audio read dma when no write buffers are
									pending. This was causing hangs.
		<30>	 3/30/94	BCK		Fixed hang problem by adding Disable-Enable interrupts around
									RemHead calls and the associated check for NULL. Removed
									div9Table.
		<29>	 3/29/94	BCK		Fixed read abort problem by enabling DMA if the read is active
									and halted.
		<28>	 3/28/94	BCK		Added EOS flag handling and fixed Abort bug with read
									throttling. Switched to simpler PTS mechanism.
		<27>	 3/27/94	GM		Modified reset behavior to work around a Woody reset bug.
		<26>	 3/17/94	BCK		Added more "Paranoid" checks. Added count of PTS values out of
									range.
		<25>	 3/14/94	BCK		Added sanity checks that can be turned on during debugging.
									FMVIntrDMAfromRAM now checks for the PTS Valid flag (as well as
									null Timcode struct). QueueRead properly sets PTS info when read
									queue is empty.
		<24>	 3/10/94	BCK		Added read DMA halting when no writes are active. Fixed
									completedWrites count. Added two tiered PTS logging.
		<23>	  3/8/94	BCK		Updated SW PTS mechanism to be exact.
		<22>	 2/28/94	GDW		Removed old API.
		<21>	 2/25/94	GM		Added structure to assist in debugging. Fixed bugs in abort and
									close routines.
		<20>	 2/25/94	BCK		Turned off PTS logging and removed an unnecessary check for
									deltaPTS wrap.
		<19>	 2/25/94	GDW		Played the name game.
		<18>	 2/23/94	BCK		Replaced divides in SW PTS byte offset calculation with table
									lookups and shifts.
		<17>	 2/16/94	BCK		Improved SW PTS accuracy by using PTS to calculate the byte
									offsets returned in the IOReq.
		<16>	 2/14/94	GM		Changed DBUG macro to print FMVDriver prefix.
		<15>	 2/10/94	GDW		Added Status and Control routine changes         for new API.
		<14>	  2/7/94	GDW		Added Status and Control routine changes for new API.
		<13>	  2/3/94	GDW		Added sw/hw build flags for PTS code.
		<12>	  2/2/94	GDW		Fixed lost PTS bug.
		<11>	  2/2/94	GDW		Fixed some Audio PTS problems.
		<10>	 1/20/94	GDW		Fixed some Audio PTS problems.
		 <9>	 12/8/93	GDW		Added PTS code for audio interrupts not working.
		 <8>	 12/3/93	GDW		Added interrupt code for PTS support.
		 <7>	 12/1/93	GDW		Conditionalized MIA code.
		 <6>	11/23/93	GDW		Fixed default mode bug.
		 <5>	11/22/93	GDW		Changed audioDataFormat to no static variable.
		 <4>	11/22/93	GDW		Added audio bypass mode.
		 <3>	11/19/93	GM		Cleaned up code and removed extra reset clears.
		 <2>	11/19/93	GM		removed extraneous reset clears and audio dma disable

*/

/* file: FMVAudioDriver.c */
/* MPEG audio decoder initialization, interrupt, control and DMA routines */
/* 5/14/93 George Mitsuoka */
/* The 3DO Company Copyright й 1993 */

/*
Functional Overview
April 11, 1994

The FMV audio driver receives I/O requests for read DMA, write DMA, and
control commands. The driver processes DMA completion interrupts and completes
the read and write DMA requests. It acts like a FIFO: data is DMAed to and from
it and decompressed in the middle. If not enough data is written to it (the FIFO
is empty), then it stalls until more writes occur. If data is not read (the FIFO
is full), then it backs up and stops processing writes. No data is lost, the
queues just get longer.

The number of requests issued is arbitrary since the queues are implemented as
linked lists. New requests are added to the tail of the queue, and requests are
processed in order and removed from the head of the queue.

The Woody interface chip has one 16 byte FIFO on input from the system bus, and
another on the output. Write DMAs happen when the input FIFO is empty, and read
DMAs happen when the output FIFO is full. In either case 4 Opera system bus
transactions occur (4 bus transactions of 4 bytes each to transfer the 16 bytes
to/from the FIFO).

The FMV module is currently the lowest device on the Opera DMA priority list, and
only above the ARM CPU memory accesses. This is generally not a problem since the
DMA bursts on the system bus transfer the 16 bytes in or out of Woody at speeds
approaching 50M bytes per second, but to the TI AV110 at only 2M bytes per second
and from the TI AV110 at about 52K bytes per second.


Timestamps and Audio Packets

In the system stream or packet stream modes, each write request may have an
associated timestamp, called a Presentation Time Stamp (PTS). The system
stream is parsed and the audio packet stream is extracted. The PTS refers
to the first sample of the first audio access unit (1152 sample group (Layer 2))
in a packet.

When the hardware handles PTSes, an interrupt is produced when the PTS reaches
the Woody chip. The decompressed data then has an additional 16 bytes of delay
before it is DMAed to memory, so the offset associated with the timestamp could
be as much as 16 bytes early. The PTS value is read from a register and placed
in the current read request structure or saved for the next one.

When PTSes are handled by software, the PTS in the current write request is
placed in the current read request structure or saved for the next one. There
is easy no way to tell exactly which sample it is associated with, so the
error range is a bit more than plus or minus one audio access unit.


VideoCD

One important use of this driver is for VideoCD, so it is relevant to talk about
some of the specifics of dealing with it.  Typical VideoCD packets contain about
3 and 1/8 audio access units each and have the following sequence of deltas
between their PTS values in audio access units:

4   3   3   3   3   3   3   3   4   3   3   3   3   3   3   3   3   4   3   3 ...

In other words, they come in groups of 8 or 9 packets, where the first one has a
delta of 4 audio access units, and the subsequent ones have a delta of 3 audio
access units. Other MPEG audio sequences will be likely have similar patterns
depending upon compressed bit rate and packet size. The VideoCD packets are 2324
bytes which corresponds to a CD-ROM Mode 2 Form 2 sector size.

The PTS clock is 90KHz and each audio access unit is about 2351.020408163 ticks.

*/

#include "types.h"
#include "audio.h"
#include "debug.h"
#include "io.h"
#include "interrupts.h"
#include "inthard.h"
#include "super.h"
#include "operror.h"

#include "FMV.h"
#include "FMVDriver.h"
#include "FMVAudioDriver.h"
#include "Hollywood.h"
#include "AV110.h"
#include "Woody.h"

#if MIABUILD == 1
#include "kernel.h"
#include "task.h"
#include "nodes.h"

#include "Kelly.h"
#include "FMVAnalogDriver.h"
#include "FMVHWAccessUtilities.h"
#endif

/* #if(def) Switches
 *
 * DBUG - Used for Debug printfs.
 *
 * PTS_LOGGING - Turns on loggin of PTSes to memory.
 */

#define DBUG(x)			DEBUGP( x )

/*
 *The FMVAudioDriverStatus structure is useful for debugging the driver or simply
 * determining its current state. The things to look for are listed below.
 *
 * The driver should never stall in a state with uncompleted reads and writes
 * outstanding. This indicates a driver malfunction.
 *
 * The readsHalted count should be equal to or one greater than the readsResumed
 * count. In the latter case, the FIFO is empty and waiting for more writes.
 */
struct
{
	int32 readsReceived, readsCompleted;	// Read requests received and completed.
	int32 writesReceived, writesCompleted;	// Write requests received and completed.
	int32 lastPTS, lastOffset;				// last pts value and offset
	int32 errorInterrupts, bufferUnderflow;	// Error interrupts and buffer underflow counts.
	int32 maxAbortTime, discontinuities;	// Reserved location and discontinuity count.
	int32 aborts, inProgressWriteAborts;	// Total abort and write abort counts.
	int32 inProgressReadAborts, pcmUFlow;	// Read abort count and PCM underflow count
	int32 readsHalted,readsResumed;			// Reads halted and resumed (for FIFO-like operation).
	int32 ptsOutOfRange, ptsOffsetOutOfRange;// For SW PTSes: PTS and offset out of range counts.
	int32 ptsInterrupts, resets;			// count pts interrupts, decoder resets
#if MIABUILD == 1
	int32 bufferAlmostEmpty, bufferAlmostFull;	//
	int32 sampleFreqChanges, currentFreq;	// count number of sampling freq changes, current frequency
}
FMVAudioDriverStatus = { 0L,0L, 0L,0L, 0L,0L, 0L,0L, -1L,0L, 0L,0L, 0L,0L, 0L,0L, 0L,0L, 0L,0L,0L,0L };
#else
}
FMVAudioDriverStatus = { 0L,0L, 0L,0L, 0L,0L, 0L,0L, -1L,0L, 0L,0L, 0L,0L, 0L,0L, 0L,0L, 0L,0L };
#endif
// [TBD] For SW PTSes, if we support Layer 1, then "SAMPLES_PER_AUDIO_ACCESS_UNIT" needs
// to be a variable.
#define SAMPLES_PER_AUDIO_ACCESS_UNIT	1152
#define BYTES_PER_AUDIO_ACCESS_UNIT		((SAMPLES_PER_AUDIO_ACCESS_UNIT) * 4)
#define PTS_PER_AUDIO_ACCESS_UNIT		2351
#define PTS_BYTE_OFFSET_MARGIN			((BYTES_PER_AUDIO_ACCESS_UNIT) * 4)
#define READ_DONE_MARGIN				10

Item gIntrAudDMAtoRAM, gIntrAudDMAfrRAM, gIntrAudio;

/* Pending I/O request queue and current request pointers for read and write DMA. */
List gMPEGAudioReadQueue, gMPEGAudioWriteQueue;
IOReq *gCurrentMPEGAudioReadReq = (IOReq *) NULL;
IOReq *gCurrentMPEGAudioWriteReq = (IOReq *) NULL;

/* DMA read channel pointers -- These point to memory-mapped hardware registers for
 * the read DMA in progress.
 */
static vuint32 *gDMAFromAudioDecoderDst = EXTERNALtoRAM;
static vuint32 *gDMAFromAudioDecoderLen = EXTERNALtoRAM + 1;

/* DMA write channel pointers -- These point to memory-mapped hardware registers for
 * the write DMA in progress.
 */
static vuint32 *gDMAToAudioDecoderSrc = RAMtoEXTERNAL;
static vuint32 *gDMAToAudioDecoderLen = RAMtoEXTERNAL + 1;

// static uint32 audioInterrupt = 0;

/* Application selectable or query-able values.
 *
 * Audio stream format -- System and packet streams return PTSes, raw audio data
 *		does not have PTS information embedded. This is both selectable and query-able.
 * Sample rate -- We only support 44.1KHz currently.
 * Attenuation -- Currently always zero (no attenuation).
 * Play through mode -- The FMV does not support a play-through mode.
 *
 * ([TBD] This should probably be a structure rather than a set of variables.
 */
uint32 gAudioDataFormat = FMV_AUDIO_STREAM;
static Boolean gReturnPTS = false;
uint32	gSampleRate = FMV_AUDIO_SAMPLERATE_44;
uint32	gLeftAtten = 0L;
uint32	gRightAtten = 0L;
uint32	gAudPlayThroughMode = false;


/* PTS Saving - Sometimes the PTS value must be saved until the appropriate read
 * is in progress. This is where the values are saved.
 * ([TBD] This should probably be a structure rather than a set of variables.
 */
uint32	gLastPTS = 0;
uint32	gLastPTSHigh = 0;
uint32	gLastPTSOffset = 0;
uint32	gUseLastPTS = false;

/* The FIFO full flag and aborted read completion state. gWaitForWrites is true
 * in normal play state. When the EOS flag is set in a write or the current
 * read is aborted it is set to false. It is set back to true in the former,
 * when the next write is initiated, and in the latter when the read completes.
 *
 * When the current read is aborted, the gLettingAbortedReadComplete and
 * gWaitForWrites flags are set to make sure the aborted read completes.
 */
uint32	gWaitForWrites = true;
uint32	gLettingAbortedReadComplete = false;

/* By default HW interrupts are used to generate PTSes -- This flag changes the
 * the mechanism to SW determined PTSes.
 */
int32	gUsingSoftwarePTS = false;
#if MIABUILD == 1
int32	gDetectFSModify = false ;
//ееint32	gDetectPCMUFlow = false ;
//ееint32	gDetectBufferAlmostEmpty = false ;
//ееint32	gDetectBufferAlmostFull = false ;
bool	gMPEGAudioDevOpen ;
#endif
/*********************************************************************
 * Logging of DMA Writes and Reads for debug.
 * This capability is used for debugging and should be turned off by
 * default by commenting out the define for PTS_LOGGING.
 * High bits are used to flag the path through the code.
 */
#define PTS_LOGGING
#ifdef PTS_LOGGING
typedef struct PTSLog
{
	int32	timecode;
	int32	readOffset;
	int32	readTotal;
} PTSLog;
#define MAX_LOG_ENTRIES 500
static int32 gEntries = 0;
PTSLog	ptsLog[MAX_LOG_ENTRIES];

static int32 LogPTS(int32 timecode, int32 readOffset, int32 readTotal)
{
	ptsLog[gEntries].timecode = timecode;
	ptsLog[gEntries].readOffset = readOffset;
	ptsLog[gEntries].readTotal = readTotal;

	gEntries++;
	if(gEntries >= MAX_LOG_ENTRIES)
		gEntries = 0;

	ptsLog[gEntries].timecode = 0;

	return(gEntries);
}

#else
#define LogPTS(a,b,c)
#endif // PTS_LOGGING

/*********************************************************************
 * InitializePTSSaving - Initialize the PTS saving mechanism.
 */
static void InitializePTSSaving()
{
	gLastPTS = 0;
	gLastPTSOffset = 0;
	gLastPTSHigh = 0;
	gUseLastPTS = false;

	return;
}
#if MIABUILD == 1
/************************************************************************
 * SetSampleRate - Sets the audio sample rate.
 ************************************************************************/
static int32 SetSampleRate( uint32 theSampleRate )
{
	extern	uint8 gStationType ;
	extern	uint32	gSampleRate ;
	uint32	readData ;
	uint8	taskprivs ;
	Err audioErr ;
	TagArg audioTags[2];

		switch (theSampleRate)
		{
			default:
			case FMV_AUDIO_SAMPLERATE_32:
				DEBUGP(("Invalid Sample Rate = %d\n",theSampleRate)) ;
				gSampleRate = FMV_AUDIO_SAMPLERATE_32 ;
			if(gStationType == kSETTOPTERMINAL)
				SelectAudioClock(FMV_AUDIO_SAMPLERATE_32) ;	/* send a command thru forage device */
				audioTags[0].ta_Arg = (void *)Convert32_F16((uint32)32000) ;
				break ;
			case FMV_AUDIO_SAMPLERATE_44:
			if(gStationType == kDEVSTATION)
			{
				readData = FMVReadKellyReg(kRegGpIO) ;
				FMVWriteKellyReg(kRegGpIO,readData | kGpIOG0OEnable | kGpIOG0Enable) ;
				gSampleRate = FMV_AUDIO_SAMPLERATE_44 ;
			}
			if(gStationType == kSETTOPTERMINAL)
				SelectAudioClock(FMV_AUDIO_SAMPLERATE_44) ;	/* send a command thru forage device */
				audioTags[0].ta_Arg = (void *)Convert32_F16((uint32)44100) ;
				break ;
			case FMV_AUDIO_SAMPLERATE_48:
			if(gStationType == kDEVSTATION)
			{
				readData = FMVReadKellyReg(kRegGpIO) ;
				FMVWriteKellyReg(kRegGpIO,(readData | kGpIOG0OEnable) & ~kGpIOG0Enable ) ;
				gSampleRate = FMV_AUDIO_SAMPLERATE_48 ;
			}
			if(gStationType == kSETTOPTERMINAL)
				SelectAudioClock(FMV_AUDIO_SAMPLERATE_48) ;	/* send a command thru forage device */
				audioTags[0].ta_Arg = (void *)Convert32_F16((uint32)48000) ;
				break ;
		}

	audioTags[0].ta_Tag = AF_TAG_SAMPLE_RATE ;
	audioTags[1].ta_Tag = TAG_END;
	audioTags[1].ta_Arg = 0;

 	taskprivs = KernelBase->kb_CurrentTask->t.n_Flags;
	KernelBase->kb_CurrentTask->t.n_Flags |= TASK_SUPER;

	if((audioErr = SuperSetAudioFolioInfo(audioTags)) < 0)
	{
		DBUG((" ERROR: Setting Audio Folio Sample Rate failed = %lx\n",audioErr)) ;
	}

	KernelBase->kb_CurrentTask->t.n_Flags =
			(KernelBase->kb_CurrentTask->t.n_Flags & ~TASK_SUPER) |
			(taskprivs & TASK_SUPER);

	return( 1L );

} // End of SetSampleRate
/************************************************************************
 * Determine Sample Rate - Determine the audio sample rate.
 ************************************************************************/
static uint32 DetermineSampleRate( void )
{
	extern	uint32	gSampleRate ;
	uint32	readData ;
	uint32	ioSampleRateFlag ;

	readData = FMVReadAudioRegister(PCM_FS) & 0x00000003; /* determine new sampling frequency */
	switch (readData)
	{
		case PCM44:
			gSampleRate = FMV_AUDIO_SAMPLERATE_44 ;
			ioSampleRateFlag = IO_SAMPLERATE_44100;
			break ;
		case PCM48:
			gSampleRate = FMV_AUDIO_SAMPLERATE_48 ;
			ioSampleRateFlag = IO_SAMPLERATE_48000;
			break ;
		case PCM32:
			gSampleRate = FMV_AUDIO_SAMPLERATE_32 ;
			ioSampleRateFlag = IO_SAMPLERATE_32000 ;
			break ;
		default:
			ioSampleRateFlag = IO_SAMPLERATE_RSRVD ;
			break ;
	}

	return( ioSampleRateFlag );

} // End of DetermineSampleRate
#endif

/*********************************************************************
 * SavePTSForNextRead - Save the PTS for the next Read.
 */
static void SavePTSForNextRead(uint32 pts, uint32 offset, uint32 ptsHigh)
{
	if ( !gUseLastPTS )
	{	/* There is no saved PTS yet, so save this one. */
		gLastPTS = pts;
		gLastPTSOffset = offset;
		gLastPTSHigh = ptsHigh;
		gUseLastPTS = true;
		LogPTS(pts, offset, 0x30000000L | ptsHigh);
	}

	return;
}

/*********************************************************************
 * SetPTSFromSaved - Sets the PTS fields of the I/O Request from the
 * saved PTS values (if one has been saved).
 */
static void SetPTSFromSaved(IOReq *ioReq)
{
	if( gUseLastPTS )
	{	/* There is a saved PTS, so load it into the request. */
		ioReq->io_Flags |= FMVValidPTS;
		ioReq->io_Flags |= gLastPTSHigh;
		ioReq->io_Extension[FMV_PTS_INDEX] = gLastPTS;
		ioReq->io_Extension[FMV_PTS_OFFSET] = gLastPTSOffset;
		gUseLastPTS = false;
		LogPTS(0xc0000000L + gLastPTS, gLastPTSOffset,
			(ioReq->io_Flags & FMVValidPTS) | gLastPTSOffset);
	}

	return;
}

/*********************************************************************
 * SetPTS - Sets the PTS fields of the I/O Request directly.
 */
static void SetPTS(IOReq *ioReq, uint32 pts, uint32 offset, uint32 ptsHigh)
{
	if ( (gCurrentMPEGAudioReadReq->io_Flags & FMVValidPTS) == 0 )
	{	/* There isn't a valid PTS yet, so load the PTS into the request. */
		ioReq->io_Flags |= FMVValidPTS;
		ioReq->io_Flags |= ptsHigh;
		ioReq->io_Extension[FMV_PTS_INDEX] = pts;
		ioReq->io_Extension[FMV_PTS_OFFSET] = offset;
		LogPTS(pts, offset, ptsHigh);
	}
	else
		LogPTS(0xf0000000, pts, offset);

	return;
}


/*********************************************************************
 * QueueRead - Queues a read request.
 */
static void QueueRead( IOReq *ior )
{
	uint32 interrupts;

	FMVAudioDriverStatus.readsReceived++;
	ior->io_Flags = 0;

	/* We only access the saved PTS in interrupt mode if gUseLastPTS is false or if a read in
	 * progress terminates. We only access them in non-interrupt mode if gUseLastPTS
	 * is true and the read queue is empty, or while interrupts are off.
	 */
	if( IsEmptyList(&gMPEGAudioReadQueue) )
		{	/* No other requests in the queue, so load it into the IOReq */
			SetPTSFromSaved(ior);
		}

	{	/* Add to the tail of the queue while interrupts are off. */
		interrupts = Disable();
		AddTail( &gMPEGAudioReadQueue, (Node *) ior );
		Enable( interrupts );
	}
}

/*********************************************************************
 * NextReadReq - Removes the request from the head of the read queue
 * and returns it. This is only called while interrupts are disabled.
 */
static IOReq *NextReadReq()
{
	return( (IOReq *) RemHead( &gMPEGAudioReadQueue ) );
}

/************************************************************************
 * FMVAudioSetUpNextRead - Sets up and starts the next read DMA.
 * This routine executes in interrupt mode after a read completes, or
 * in non-interrupt mode when there are no reads queued and a new read
 * is queued.
 ************************************************************************/
static void FMVAudioSetUpNextRead()
{
	uint32 interrupts;

	/* gCurrentMPEGAudioReadReq can only be set to non-NULL if it is NULL.
	 * The check for NULL and the setting to the new value must happen as a pair
	 * while interrupts are off.
	 *
	 * We access it from non-interrupt mode with one of these two methods:
	 *	1)	When there are no other reads in the queue, and therefore none in progress.
	 *	2)	We turn interrupts off while we are using the value, and begin usage by
	 *		first checking for non-NULL then grabbing the next from the queue.
	 */

	{	/* If no read is in progress, set up the next read while interrupts are off. */
		interrupts = Disable();
		if( gCurrentMPEGAudioReadReq )
		{
			Enable( interrupts );
			return;
		}

		gCurrentMPEGAudioReadReq = NextReadReq();
		Enable( interrupts );
	}

	if( gCurrentMPEGAudioReadReq )
	{
		/* disable DMA */
		*DMAREQDIS = EN_EXTERNALtoDMA;

		/* set up next transfer */
		*gDMAFromAudioDecoderDst = (vuint32) gCurrentMPEGAudioReadReq->io_Info.ioi_Recv.iob_Buffer;
		*gDMAFromAudioDecoderLen = (vuint32) gCurrentMPEGAudioReadReq->io_Info.ioi_Recv.iob_Len - 4L;

		/* enable interrupt */
		EnableInterrupt( INT_DMAEXTIN );

		/* enable DMA */
		*DMAREQEN = EN_EXTERNALtoDMA;
		FMVEnableAudioDMAOut();
	}
	else	/* no more reads in the queue */
	{
		/* disable DMA */
		*DMAREQDIS = EN_EXTERNALtoDMA;

		/* disable DMA completion interrupt */
		DisableInterrupt( INT_DMAEXTIN );
	}
}

/****************************************************************************
 * FMVAudioDMAtoRAMIntr - Interrupt handler is called when a DMA read
 * completes to notify completion of the IOReq and start next DMA.
 */

static int32 FMVAudioDMAtoRAMIntr( void )
{
	IOReq *savedIOReq;
#if MIABUILD == 1
	uint32	ioSampleRateFlag ;


	if(!gMPEGAudioDevOpen) return (0);
#endif

	/* We don't disable woody here, because the audio decoder will still
	 * be streaming data, and we want woody to buffer it.
	 * This is a woody feature
	 */

	/* this stuff is really time and execution order critical, be careful
	 * when messing with this. The next write must be set up before
	 * SuperCompleteIO() is called. [TBD] Why?
	 */

	/* Notify completion of IORequest. (There should always be one.) */
	if( gCurrentMPEGAudioReadReq )
	{
		savedIOReq = gCurrentMPEGAudioReadReq;
		gCurrentMPEGAudioReadReq = (IOReq *) NULL;

		/* set up next read if there is one */
		FMVAudioSetUpNextRead();

		savedIOReq->io_Actual = savedIOReq->io_Info.ioi_Recv.iob_Len;

#if MIABUILD == 1
		/* return with the io request the sample rate value in the io_flags */
		ioSampleRateFlag = DetermineSampleRate() ;
		savedIOReq->io_Flags &= ~IO_SAMPLERATE_MASK ;	// strip current samplerate flag
		savedIOReq->io_Flags |= ioSampleRateFlag ;		// load new samplerate flag
		FMVAudioDriverStatus.currentFreq = gSampleRate ;
#endif
		LogPTS(0xa0000000L, savedIOReq->io_Extension[FMV_PTS_INDEX],
			(savedIOReq->io_Flags & FMVValidPTS) | savedIOReq->io_Extension[FMV_PTS_OFFSET]);

		if( gCurrentMPEGAudioReadReq ) {
			/* There is a request in the queue, so load the it into the IOReq */
			SetPTSFromSaved(gCurrentMPEGAudioReadReq);
		}
		FMVAudioDriverStatus.lastPTS = savedIOReq->io_Extension[FMV_PTS_INDEX];
		FMVAudioDriverStatus.lastOffset = savedIOReq->io_Extension[FMV_PTS_OFFSET];

		SuperCompleteIO( savedIOReq );
		FMVAudioDriverStatus.readsCompleted++;
	} else {
		FMVAudioSetUpNextRead();
	}

	if(gLettingAbortedReadComplete)
	{	/* Aborted read has completed, so turn off this mode. */
		gLettingAbortedReadComplete = false;
		gWaitForWrites = true;
	}

	return( 0 );
}

/******************************************************************************
 * QueueWrite - Queues a write request. This routine is called every time the
 * app has a write request, and it simply queues it.
 */

static void QueueWrite( IOReq *ior )
{
	uint32 interrupts;

	FMVAudioDriverStatus.writesReceived++;

	{	/* Add to the tail of the queue while interrupts are off. */
		interrupts = Disable();
		AddTail( &gMPEGAudioWriteQueue, (Node *) ior );
		Enable( interrupts );
	}
}

/************************************************************************
 * NextWriteReq - Removes the request from the head of the write queue
 * and returns it. This routine is called when we are actually doing a DMA
 * write (i.e. from memory into the woodman).  It returns either the
 * head of the write queue or null if the write queue is empty.
 * This is only called while interrupts are disabled.
 */

static IOReq *NextWriteReq()
{
	return( (IOReq *) RemHead( &gMPEGAudioWriteQueue ) );
}

/************************************************************************
 * FMVAudioSetUpNextWrite - Sets up and starts the next read DMA.
 * This routine executes in interrupt mode after a write completes, or
 * in non-interrupt mode when there are no writes queued and a new write
 * is queued.
 ************************************************************************/
static void FMVAudioSetUpNextWrite()
{
	uint32 interrupts;

	/* gCurrentMPEGAudioWriteReq can only be set to non-NULL if it is NULL.
	 * The check for NULL and the setting to the new value must happen as a pair
	 * while interrupts are off.
	 *
	 * We access it from non-interrupt mode with one of these two methods:
	 *	1)	When there are no other writes in the queue, and therefore none in progress.
	 *	2)	We turn interrupts off while we are using the value, and begin usage by
	 *		first checking for non-NULL.
	 */

	// If no write is in progress, set up the next one.
	{	/* Interrupts are disabled in this code block. */
		interrupts = Disable();
		if( gCurrentMPEGAudioWriteReq )
		{
			Enable( interrupts );
			return;
		}
		gCurrentMPEGAudioWriteReq = NextWriteReq();
		Enable( interrupts );
	}

	if( gCurrentMPEGAudioWriteReq )
	{	// Set up the DMA
		vuint32 src = (vuint32) gCurrentMPEGAudioWriteReq->io_Info.ioi_Send.iob_Buffer;
		vuint32 len = (vuint32) gCurrentMPEGAudioWriteReq->io_Info.ioi_Send.iob_Len - 4L;
		IOReq *ioReq;

		if( gCurrentMPEGAudioWriteReq->io_Info.ioi_Send.iob_Len == 0 )
		{
			// The write has zero length -- check the flags and complete it here.
			if( (FMVFlags(gCurrentMPEGAudioWriteReq->io_Info.ioi_CmdOptions) & FMV_END_OF_STREAM_FLAG) != 0 )
			{
				// End of Stream indicated -- resume the reads if halted.
				if(FMVAudioDriverStatus.readsHalted > FMVAudioDriverStatus.readsResumed)
				{
					FMVEnableAudioDMAOut();
					FMVAudioDriverStatus.readsResumed++;
				}
				// Turn read halting off since we won't get any more writes
				gWaitForWrites = false;
			}
			/* hack to prevent AV110 audio glitches */
			FMVWriteAudioRegister( RESET, 1L );
			FMVAudioDriverStatus.resets++;

			/* Complete the zero length write request. SuperCompleteIO() can initiate
			 * another I/O request, so we must set gCurrentMPEGAudioWriteReq to NULL
			 * beforehand.
			 */
			gCurrentMPEGAudioWriteReq->io_Actual = 0;
			ioReq = gCurrentMPEGAudioWriteReq;
			gCurrentMPEGAudioWriteReq = NULL;
			SuperCompleteIO( ioReq );
			FMVAudioDriverStatus.writesCompleted++;

			FMVAudioSetUpNextWrite();

			return;
		}
		else if(!gLettingAbortedReadComplete)
			// Turn read halting back on since we have a write.
			gWaitForWrites = true;

		if(gUsingSoftwarePTS)
		{	/* Using Software PTS Mechanism. */
			if ( gCurrentMPEGAudioWriteReq->io_Info.ioi_CmdOptions &&
				((FMVFlags(gCurrentMPEGAudioWriteReq->io_Info.ioi_CmdOptions) & FMVValidPTS) != 0) ) {
				// The write has PTS info.
				if ( (gAudioDataFormat == FMV_AUDIO_PACKET) ||				// We only return PTS's
					 (gAudioDataFormat == FMV_SYSTEM_STREAM) ) {			// for these data formats

					vint32 curRdDMAAddr;
					int32 curPTSOffset, curPTS;
					FMVIOReqOptionsPtr		wrTsPtr;						// Pointer to write request FMV options structure
					int32					curLen;

					wrTsPtr = (FMVIOReqOptionsPtr) gCurrentMPEGAudioWriteReq->io_Info.ioi_CmdOptions;
					curPTS = wrTsPtr->FMVOpt_PTS;

					{	/* Turn off interrupts while we look at the read in progress saved PTS. */
						interrupts = Disable();
						if ( gCurrentMPEGAudioReadReq ){				// Is there a read in progress?
							curRdDMAAddr = *gDMAFromAudioDecoderDst;						// Get the current DMA pointer
							curPTSOffset = curRdDMAAddr - (uint32) (gCurrentMPEGAudioReadReq->io_Info.ioi_Recv.iob_Buffer);
							curLen = gCurrentMPEGAudioReadReq->io_Info.ioi_Recv.iob_Len;

							SetPTS(gCurrentMPEGAudioReadReq, curPTS, curPTSOffset, FMVFlags(wrTsPtr) & FMVPTSHIGHBIT);
						} else {
							/* The PTS is for the next Read DMA */
								SavePTSForNextRead(curPTS, 0L, FMVFlags(wrTsPtr) & FMVPTSHIGHBIT);
						}
						Enable( interrupts );
					}
				}
			}
			else
				LogPTS(0xffffffff, 0, 0);
		}

		// Force DMA transfer to be on long word boundaries.
		if( src & 3L )					// is the source on an odd halfword?
		{
			src = src & ~3L;			// if yes, start DMA at lower long word boundary
			len += 2L;					// increase DMA length by two bytes
			WdySetControlSet( WDYSFAH );// skip first audio halfword
		}
		if( len & 3L )					// is the length not a multiple of four?
		{
			len += 2L;					// if yes, transfer an extra two bytes
			WdySetControlSet( WDYSLAH );// skip last audio halfword
		}

		{	// While DMAs are disabled, set up next DMA transfer.
			// disable DMA
			*DMAREQDIS = EN_DMAtoEXTERNAL;

			// modify hardware DMA registers.
			*gDMAToAudioDecoderSrc = src;
			*gDMAToAudioDecoderLen = len;

			// Enable DMA completion interrupt.
			EnableInterrupt( INT_DMAEXTOUT );

			/* Enable DMA */
			*DMAREQEN = EN_DMAtoEXTERNAL;
		}

		if(FMVAudioDriverStatus.readsHalted > FMVAudioDriverStatus.readsResumed)
		{	// Resume read by enabling DMA.
			FMVEnableAudioDMAOut();
			FMVAudioDriverStatus.readsResumed++;
		}

		/* Enable Woody - This should be the last thing done to make sure that
		 * when this routine is executed in non-interrupt mode that a write
		 * cannot complete and interrupt this routine before it is done.
		 */
		FMVEnableAudioDMAIn();
	}
	return;
}

/************************************************************************
 * FMVAudioDMAfrRAMIntr -
 * This interrupt handler is called when a DMA write completes.
 */
static int32 FMVAudioDMAfrRAMIntr( void )
{
	IOReq *savedIOReq;

	/* This stuff is really time and execution order critical, be careful
	 * when messing with this. The next write must be set up before
	 * SuperCompleteIO() is called. [TBD] Why?
	 */

#if MIABUILD == 1

	if(!gMPEGAudioDevOpen) return (0);
#endif

	/* Notify completion of IORequest. (There should always be one.) */
	if( gCurrentMPEGAudioWriteReq )
	{	// There is a write in progress - complete the I/O request.
		savedIOReq = gCurrentMPEGAudioWriteReq;
		gCurrentMPEGAudioWriteReq = (IOReq *) NULL;

		/* Set up next write if there is one */
		FMVAudioSetUpNextWrite();

		if( gCurrentMPEGAudioWriteReq == NULL)
		{ /* All queued write requests have completed. */
			if( (gCurrentMPEGAudioReadReq != NULL) && (gWaitForWrites) )
			{	// There is a Read DMA we are in the "Wait for writes" mode - so stop the Read DMA.
				FMVDisableAudioDMAOut();
				FMVAudioDriverStatus.readsHalted++;
			}
		}

		savedIOReq->io_Actual = savedIOReq->io_Info.ioi_Send.iob_Len;

		SuperCompleteIO( savedIOReq );
		FMVAudioDriverStatus.writesCompleted++;
	}
	else
	{
		/* Set up next write if there is one */
		FMVAudioSetUpNextWrite();
	}

	return( 0 );
}

/************************************************************************
 * FMVAudioIntr - This interrupt handler is called when the FMV board
 * generates an interrupt. It only handles audio interrupts.
 *
 * The interrupts handled are:
 *	INT_PTS - The PTS arrived interrupt.
 *	INT_FS_MOD - if there is a change in sampling frequency
 */

int32 FMVAudioIntr( void )
{
	uint32			interruptBits;
	uint32			thePTS;
	int32			highBit;
	uint32			byteIndex;
	uint32			dmaAddress;
	int32			curLen;
#if MIABUILD == 1
	uint32			theInt;
	IOReq 			*savedIOReq;
	uint32			ioSampleRateFlag ;


	if(!gMPEGAudioDevOpen) return(0) ;
#endif

	// Get the DMA address of the current read DMA. The DMA will continue to operate
	// while this code is executing, so we grab it now to get the most accurate value
	// possible. We are already late in getting it by the latency from the hardware
	// interrupt till the execution of this line completes. The application must take
	// into account some inaccuracy in this value, typically less than or equal to 16 bytes.
	dmaAddress = ((uint32) *gDMAFromAudioDecoderDst);

	interruptBits = FMVAV110ReadIntStatus();/* Get Audio interrupt registers value & clear them */
#if MIABUILD == 1
	if( (interruptBits & INT_FS_MOD) != 0 )	/* Is this a sampling frequency change ? */
	{
		theInt = FMVReadKellyReg(kRegIntr);
		if ( (theInt & (kKellyIntrEnChipEN | kKellyIntrAudIE)) != 0 )
			FMVWriteKellyReg(kRegIntr, (kKellyIntrEnChipEN | kKellyIntrAudIE));	// clear interrupt
		if(gCurrentMPEGAudioReadReq)		/* terminate the current Read and flag change */
		{
			savedIOReq = gCurrentMPEGAudioReadReq;
			gCurrentMPEGAudioReadReq = (IOReq *) NULL;
			savedIOReq->io_Actual = savedIOReq->io_Info.ioi_Recv.iob_Len;
			ioSampleRateFlag = DetermineSampleRate() ;		// figure out what the new sample rate is
			savedIOReq->io_Flags &= ~IO_SAMPLERATE_MASK ;	// strip current samplerate flag
			savedIOReq->io_Flags |= ioSampleRateFlag ;		// load new sample rate flag
			SuperCompleteIO( savedIOReq );
			FMVAudioDriverStatus.sampleFreqChanges++;
			FMVAudioDriverStatus.currentFreq = gSampleRate ;
		}
	}	else
#endif
	if ( (interruptBits & INT_PTS) != 0 )				// Is this a PTS interrupt.
	{
		FMVAudioDriverStatus.ptsInterrupts++;

		FMVAV110ClearInterrupt();

		if(!gUsingSoftwarePTS)
		{
#if MIABUILD == 1
			theInt = FMVReadKellyReg(kRegIntr);
			if ( (theInt & (kKellyIntrEnChipEN | kKellyIntrAudIE)) != 0 ) {
				FMVWriteKellyReg(kRegIntr, (kKellyIntrEnChipEN | kKellyIntrAudIE));		// clear interrupt

#endif
			thePTS = FMVAV110ReadPTS(&highBit);			// Read the PTS value from the register(s).

			if ( gCurrentMPEGAudioReadReq == NULL )
			{	// The read is not actually in progress, we're filling the Woody Video Read FIFO.
				SavePTSForNextRead(thePTS, 0L, highBit ? FMVPTSHIGHBIT : 0);
			}
			else if ( (gCurrentMPEGAudioReadReq->io_Flags & FMVValidPTS) == 0 )
			{	// There is no PTS stored in the IOReq yet, then store this one.
				byteIndex = dmaAddress - (uint32)(gCurrentMPEGAudioReadReq->io_Info.ioi_Recv.iob_Buffer);
				SetPTS(gCurrentMPEGAudioReadReq, thePTS, byteIndex, highBit ? FMVPTSHIGHBIT : 0);
			}
			else
			{	// There is already a PTS stored in the IOReq.
				curLen = gCurrentMPEGAudioReadReq->io_Info.ioi_Recv.iob_Len;
				byteIndex = dmaAddress - (uint32) (gCurrentMPEGAudioReadReq->io_Info.ioi_Recv.iob_Buffer);
				if(curLen - byteIndex <= WDYAUDIOREADFIFOSIZE)
				{
					/* We're within the Woody audio read FIFO size of completing the DMA,
					 * so save the PTS for the next Read DMA.
					 */
					SavePTSForNextRead(thePTS, 0L, highBit ? FMVPTSHIGHBIT : 0);
				}
				else
				{	// We have no use for this PTS.
					byteIndex = dmaAddress - (uint32) (gCurrentMPEGAudioReadReq->io_Info.ioi_Recv.iob_Buffer);
					LogPTS(thePTS, byteIndex, 0x50000000L);
				}
			}
		}
#if MIABUILD == 1
		}
#endif

	}
	else if ( (interruptBits & INT_OUT_UFLOW) != 0 )	/* Is this a Output Underflow interrupt. */
	{
		FMVAV110ClearInterrupt();

		FMVAudioDriverStatus.pcmUFlow++;
		/* work around the start/stop bug in v2.2 AV110 silicon */
		FMVWriteAudioRegister( RESET, 1L );
		FMVAudioDriverStatus.resets++;
	}
#if MIABUILD == 1
//ее	if ( (interruptBits & INT_BALE) != 0 )
//ее	{
//ее		theInt = FMVReadKellyReg(kRegIntr);
//ее		if ( (theInt & (kKellyIntrEnChipEN | kKellyIntrAudIE)) != 0 )
//ее			FMVWriteKellyReg(kRegIntr, (kKellyIntrEnChipEN | kKellyIntrAudIE));	// clear interrupt
//ее		FMVAudioDriverStatus.bufferAlmostEmpty++;
//ее
//ее	}
//ее	if ( (interruptBits & INT_BALF) != 0 )
//ее	{
//ее		theInt = FMVReadKellyReg(kRegIntr);
//ее		if ( (theInt & (kKellyIntrEnChipEN | kKellyIntrAudIE)) != 0 )
//ее			FMVWriteKellyReg(kRegIntr, (kKellyIntrEnChipEN | kKellyIntrAudIE));	// clear interrupt
//ее		FMVAudioDriverStatus.bufferAlmostFull++;
//ее
//ее	}
#endif
	return( 0 );
}

/************************************************************************
 * SetSndDataFormat - Sets the audio stream format.
 ************************************************************************/
static int32 SetSndDataFormat( uint32 theDataType )
{
	switch ( theDataType ) {
		case FMV_AUDIO_STREAM:
			FMVAV110SetAudioStream();
			gReturnPTS = false;
			break;
		case FMV_AUDIO_PACKET:
			FMVAV110SetPacketStream();
			gReturnPTS = true;
			break;
		case FMV_SYSTEM_STREAM:
			FMVAV110SetSystemStream();
			gReturnPTS = true;
			break;
#if MIABUILD == 1
		case FMV_AUDIO_BYPASS:
			FMVAV110SetByPass();
			gReturnPTS = false;
			break;
#endif
	}
	gAudioDataFormat = theDataType;

	if(!gUsingSoftwarePTS)
	{
		if ( (gAudioDataFormat == FMV_AUDIO_PACKET) ||
			 (gAudioDataFormat == FMV_SYSTEM_STREAM) )
			FMVAV110SetupPTSInterrupt(true);		/* Enable the PTS interrupt */
		else
			FMVAV110SetupPTSInterrupt(false);		/* Disable the PTS interrupt */
	}
#if MIABUILD == 1
	if( gDetectFSModify )	FMVAV110EnableFSModInterrupt() ;
//ее	if( gDetectPCMUFlow )	FMVAV110EnableOutputUnderflowInterrupt() ;
//ее	if( gDetectBufferAlmostEmpty )	FMVAV110EnableInputAlmostEmptyInterrupt() ;
//ее	if( gDetectBufferAlmostFull )	FMVAV110EnableInputAlmostFullInterrupt() ;
//ее	if ( gDetectFSModify || gDetectPCMUFlow || gDetectBufferAlmostEmpty || gDetectBufferAlmostFull)
	if ( gDetectFSModify )
		FMVWriteKellyReg(kRegIntrEn, (kKellyIntrEnChipEN | kKellyIntrAudIE));	// Enable TI interrupt
#endif
	return( 1L );

} // End of SetSndDataFormat

//------------------------------------------------------------------------
//	Public Routines
//------------------------------------------------------------------------

/************************************************************************
 * FMVAudioInit - Initializes the audio device.
 */
int32 FMVAudioInit( void )
{

	DBUG(("FMVAudioInit: &FMVAudioDriverStatus = 0x%08lx\n",(uint32)&FMVAudioDriverStatus));
#ifdef PTS_LOGGING
	DBUG(("FMVDriver: *********** PTS Log is at %lx ********\n", (uint32)ptsLog));
#endif

	FMVAV110SetOutputSampleFrequency( HLWD_AUDIOFREQ );

	*DMAREQDIS = EN_EXTERNALtoDMA;			/* Stop Opera DMA from the audio decoder. */
	DisableInterrupt( INT_DMAEXTIN );

	*DMAREQDIS = EN_DMAtoEXTERNAL;			/* Stop Opera DMA to the audio decoder. */
	DisableInterrupt( INT_DMAEXTOUT );
#if MIABUILD == 1
	DisableInterrupt( INT_PD );
#endif
	DBUG(("Installing audio interrupt routines.\n"));

	/* Initialize read & write queues. */
	InitList( &gMPEGAudioReadQueue, "MPEG Audio Read Queue" );
	InitList( &gMPEGAudioWriteQueue, "MPEG Audio Write Queue" );

	/* Initialize some status fields. */
	FMVAudioDriverStatus.readsHalted = 0;
	FMVAudioDriverStatus.readsResumed = 0;
	FMVAudioDriverStatus.ptsOutOfRange = 0;
	FMVAudioDriverStatus.ptsOffsetOutOfRange = 0;

	/* Install interrupt routines. */
	gIntrAudDMAtoRAM = SuperCreateFIRQ("FMVAudioDMAtoRAMIntr",150,FMVAudioDMAtoRAMIntr,INT_DMAEXTIN);
	gIntrAudDMAfrRAM = SuperCreateFIRQ("FMVAudioDMAfrRAMIntr",150,FMVAudioDMAfrRAMIntr,INT_DMAEXTOUT);
	gIntrAudio       = SuperCreateFIRQ("FMVAudioIntr",150,FMVAudioIntr,INT_PD);
	DBUG(("dma->ram = %ld, dma<-ram = %ld, audio = %ld\n",gIntrAudDMAtoRAM,gIntrAudDMAfrRAM,gIntrAudio));

#if MIABUILD == 1
	EnableInterrupt( INT_PD );
	if ( !gUsingSoftwarePTS ) {
		FMVWriteKellyReg(kRegIntrEn, (kKellyIntrEnChipEN | kKellyIntrAudIE));		// Enable TI interrupt
	}
	if ( gDetectFSModify ) {
//еее	if ( gDetectFSModify || gDetectPCMUFlow || gDetectBufferAlmostEmpty || gDetectBufferAlmostFull) {
		FMVWriteKellyReg(kRegIntrEn, (kKellyIntrEnChipEN | kKellyIntrAudIE));	// Enable TI interrupt
	}
#endif
	if( gIntrAudDMAtoRAM < 0 )
		return( gIntrAudDMAtoRAM );
	if( gIntrAudDMAfrRAM < 0 )
		return( gIntrAudDMAfrRAM );
	if( gIntrAudio < 0 )
		return( gIntrAudio );

//	if(!gUsingSoftwarePTS)
		FMVAV110InitPTSInterrupt();

	return( 0 );
}

/************************************************************************
 * FMVAudioDevOpen - Opens the audio device.
 */
void FMVAudioDevOpen( Device *dev )
{
#if MIABUILD == 1
	extern uint8 gStationType ;
#endif


	DBUG(("FMV Audio Device Open"));

	FMVAV110StopDecoder();					/* Make sure the decoder is stopped. */

	*DMAREQDIS = EN_EXTERNALtoDMA;			/* Stop Opera DMA from the audio decoder. */
	DisableInterrupt( INT_DMAEXTIN );

	*DMAREQDIS = EN_DMAtoEXTERNAL;			/* Stop Opera DMA to the audio decoder. */
	DisableInterrupt( INT_DMAEXTOUT );

	// Initialize PTS saving .
	InitializePTSSaving();

	gAudioDataFormat = FMV_AUDIO_STREAM;		// Default mode is audio streams
	FMVAV110Init(gAudioDataFormat);				// INIT now takes the format of the data еееJW1

	if(!gUsingSoftwarePTS)
	{
		if ( (gAudioDataFormat == FMV_AUDIO_PACKET) ||
			 (gAudioDataFormat == FMV_SYSTEM_STREAM) )
			FMVAV110SetupPTSInterrupt(true);		/* Enable the PTS interrupt */
		else
			FMVAV110SetupPTSInterrupt(false);		/* Disable the PTS interrupt */
	}
	else
		FMVAV110SetupPTSInterrupt(false);		/* Disable the PTS interrupt */

	FMVAV110EnableOutputUnderflowInterrupt();

#if MIABUILD == 1
	gMPEGAudioDevOpen= TRUE ;
	gDetectFSModify = true ;
	FMVAV110EnableFSModInterrupt() ; /* enable the Sampling Freq change interrupt */
//ее	gDetectPCMUFlow = true ;
//ее	FMVAV110EnableOutputUnderflowInterrupt() ;
//ее	gDetectBufferAlmostEmpty = true ;
//ее	FMVAV110EnableInputAlmostEmptyInterrupt() ;
//ее	gDetectBufferAlmostFull = true ;
//ее	FMVAV110EnableInputAlmostFullInterrupt() ;

	if(gStationType == kSETTOPTERMINAL)
	{
		SetupForage() ;
	}
#endif


#ifdef PTS_LOGGING
	gEntries = 0;		// Reset PTS Logging.
#endif

/* Initialize waiting for writes (aka read halting).
 * Initially, make reads wait for writes if not enough are queued.
 * When EOS is set on a write, then this will be set to false;
 * When another write is queued, then this will be set back to true;
 */
	gWaitForWrites = true;
#if MIABUILD == 1
//	SetSampleRate(FMV_AUDIO_SAMPLERATE_44);
//	SetSampleRate(FMV_AUDIO_SAMPLERATE_48);							// еее Hack for testing
#endif
}

/************************************************************************
 * FMVAudioDevClose - Closes the audio device.
 */
void FMVAudioDevClose( Device *dev )
{
	IOReq *ior;
#if MIABUILD == 1
	uint32	readData ;
	extern uint8 gStationType ;
#endif

	/* clear the queues */
	// just delete the IOReqs
	// abortio will automatically get called and remove them
	while( !IsEmptyList( &gMPEGAudioReadQueue ) )
	{
		ior = (IOReq *) LastNode( &gMPEGAudioReadQueue );
		SuperInternalDeleteItem( ior->io.n_Item );
	}
	while( !IsEmptyList( &gMPEGAudioWriteQueue ) )
	{
		ior = (IOReq *) LastNode( &gMPEGAudioWriteQueue );
		SuperInternalDeleteItem( ior->io.n_Item );
	}
	ior = gCurrentMPEGAudioReadReq;
	if( ior )
	{
		SuperInternalDeleteItem( ior->io.n_Item );
	}
	ior = gCurrentMPEGAudioWriteReq;
	if( ior )
	{
		SuperInternalDeleteItem( ior->io.n_Item );
	}
#if MIABUILD == 1
	/* Disable Audio (AV110 & Kelly) Interrupts */
	readData = FMVAV110ReadIntStatus() ;	// clear any pending audio interrupts
	FMVWriteAudioRegister(INTR_ENH, 0);		// clear all interrupt enable bits
	FMVWriteAudioRegister(INTR_ENL, 0);		// clear all interrupt enable bits
	FMVWriteKellyReg(kRegIntrEn, 0);		// clear all interrupt enable bits
	gDetectFSModify = false ;
//ее	gDetectPCMUFlow = false ;
//ее	gDetectBufferAlmostEmpty = false ;
//ее	gDetectBufferAlmostFull = false ;
	gMPEGAudioDevOpen= FALSE ;

	if(gStationType == kSETTOPTERMINAL)
		CleanupForage() ;

#endif

}

/************************************************************************
 * FMVAudioAbortIO - Result of an I/O Request abort command. (This routine
 * is called by the system with interrupts disabled.)
 */
void FMVAudioAbortIO( IOReq *ior )
{
	FMVAudioDriverStatus.aborts++;

//	DBUG(("FMVAudioAbortIO"));

	if( ior == gCurrentMPEGAudioReadReq )
	{	// This read request is active.
		FMVAudioDriverStatus.inProgressReadAborts++;

		// Let read complete, but flag as aborted.
		ior->io_Error = ABORTED;

		if(FMVAudioDriverStatus.readsHalted > FMVAudioDriverStatus.readsResumed)
		{	// The read has been halted, so enable the Read DMA to let it finish.
			FMVEnableAudioDMAOut();
			FMVAudioDriverStatus.readsResumed++;
		}

		// Don't wait for writes until the aborted read completes.
		gLettingAbortedReadComplete = true;
		gWaitForWrites = false;
	}
	else if( ior == gCurrentMPEGAudioWriteReq )
	{	// This write request is active.
		FMVAudioDriverStatus.inProgressWriteAborts++;
		/* Cancel current request. */
		FMVDisableAudioDMAIn();

		/* Make sure DMA hasn't just finished */
		if( *gDMAToAudioDecoderLen == 0xfffffffcL )
		{
			return;
		}
		ior->io_Error = ABORTED;
		ior->io_Actual = 0L;
		gCurrentMPEGAudioWriteReq = (IOReq *) NULL;

		/* Set up the next write if there is one. */
//		FMVAudioSetUpNextWrite();

		SuperCompleteIO( ior );
		FMVAudioDriverStatus.writesCompleted++;
	}
	else
	{	// Remove the request from the appropriate list.
		RemNode( (Node *) ior );
		ior->io_Error = ABORTED;
		ior->io_Actual = 0L;
		SuperCompleteIO( ior );
		if(ior->io_Info.ioi_Command == CMD_WRITE)
			FMVAudioDriverStatus.writesCompleted++;
		else
			FMVAudioDriverStatus.readsCompleted++;
	}
}

/************************************************************************
 * FMVAudioCmdStatus - Result of an I/O Request status command.
 */
int32 FMVAudioCmdStatus( IOReq *ior )
{
	CODECDeviceStatus		*dst;
	int32					len;
	TagArgP					codecStatPtr;
	int						i;
	uint32					capFlags;

	dst = (CODECDeviceStatusPtr) ior->io_Info.ioi_Recv.iob_Buffer;
	len = ior->io_Info.ioi_Recv.iob_Len;

	if ( dst == NULL ) return(1L);					// Nothing passed leave

	capFlags = (kAudioCODECOutDoesStereo |
				kAudioCODECOutDoesDMA |
				kAudioCODECOutDoesMPEGSystemStream |
				kAudioCODECOutDoesMPEGAudioPacket |
				kAudioCODECOutDoesMPEGAudioStream |
				kAudioCODECOutDoesAttenuation );
	codecStatPtr = dst->codec_TagArg;				// Get a temporary pointer

	i = 0;

	if ( codecStatPtr[i].ta_Tag == TAG_END ) {		// Did they pass an empty struct?
		if ( len >= sizeof(CODECDeviceStatus) ) {
			codecStatPtr[0].ta_Tag = AUD_CODEC_TAG_CAPABILITIES;
			codecStatPtr[0].ta_Arg = (void *) capFlags;
			codecStatPtr[1].ta_Tag = AUD_CODEC_TAG_ATTENUATION;
			codecStatPtr[1].ta_Arg = (void *) ((gLeftAtten << 16) | gRightAtten);
			codecStatPtr[2].ta_Tag = AUD_CODEC_TAG_SAMPLERATE;
			codecStatPtr[2].ta_Arg = (void *) gSampleRate;
			codecStatPtr[3].ta_Tag = AUD_CODEC_TAG_DATA_FORMAT;
			codecStatPtr[3].ta_Arg = (void *) gAudioDataFormat;
			codecStatPtr[4].ta_Tag = AUD_CODEC_TAG_PLAYTHROUGH;
			codecStatPtr[4].ta_Arg = (void *) gAudPlayThroughMode;
			return(1L);								// We filled it all
		}
	}

	while ( codecStatPtr[i].ta_Tag != TAG_END ) {	// Have we reached the end yet?
		switch ( codecStatPtr[i].ta_Tag ) {
			case AUD_CODEC_TAG_CAPABILITIES:
				codecStatPtr[i].ta_Arg = (void *) capFlags;
				break;
			case AUD_CODEC_TAG_ATTENUATION:
				codecStatPtr[i].ta_Arg = (void *) ((gLeftAtten << 16) | gRightAtten);
				break;
			case AUD_CODEC_TAG_SAMPLERATE:
				codecStatPtr[i].ta_Arg = (void *) gSampleRate;
				break;
			case AUD_CODEC_TAG_DATA_FORMAT:
				codecStatPtr[i].ta_Arg = (void *) gAudioDataFormat;
				break;
			case AUD_CODEC_TAG_PLAYTHROUGH:
				codecStatPtr[i].ta_Arg = (void *) gAudPlayThroughMode;
				break;
		}
		i++;										// Next entry
	}

	return( 1L );
}

/************************************************************************
 * FMVAudioCmdControl - Result of an I/O Request control command.
 */
int32 FMVAudioCmdControl( IOReq *ior )
{
	CODECDeviceStatus		*dst;
	int32					len;
	TagArgP					codecStatPtr;
	int						i;
	int32					err;
	Boolean					startDecoding;

	dst = (CODECDeviceStatusPtr) ior->io_Info.ioi_Send.iob_Buffer;
	len = ior->io_Info.ioi_Send.iob_Len;

	err = 1L;										// Default return code
	startDecoding = false;

	if ( dst == NULL ) return(1L);					// Nothing passed leave
	codecStatPtr = dst->codec_TagArg;				// Get a temporary pointer
	i = 0;											// Start at first tag arg

	while ( codecStatPtr[i].ta_Tag != TAG_END ) {	// Have we reached the end yet?
		switch ( codecStatPtr[i].ta_Tag ) {
//			case AUD_CODEC_TAG_ATTENUATION:
//				break;
			case AUD_CODEC_TAG_DATA_FORMAT:
				err = SetSndDataFormat((int32) codecStatPtr[i].ta_Arg);
				break;
			case AUD_CODEC_TAG_PLAY:
				startDecoding = true;
				break;
#if MIABUILD == 1
			case AUD_CODEC_TAG_PLAYTHROUGH:
				if ( (uint32) codecStatPtr[i].ta_Arg ) {
					SetSndDataFormat(FMV_AUDIO_BYPASS);
				} else {
					SetSndDataFormat(FMV_AUDIO_STREAM);
				}
				break;
			case AUD_CODEC_TAG_SAMPLERATE:
				err = SetSampleRate((uint32) codecStatPtr[i].ta_Arg);
				break;
			default:
				DEBUGP(("Invalid Control TagArg\n")) ;
				break ;
#endif
		}
		i++;										// Next entry
	}
	if ( startDecoding ) {
		FMVAudioCmdPlay();
	}
	return( 1L );
}

/************************************************************************
 * FMVAudioCmdRead - Result of an I/O Request read command.
 */
int32 FMVAudioCmdRead( IOReq *ior )
{
	uint32 interrupts;

	/* queue the read request */
	ior->io_Flags &= ~IO_QUICK;
	QueueRead( ior );

	{	/* [TBD] We should not need to disable interrupts here, but we're
	 	 * doing it now to be safe.
		 */
		interrupts = Disable();
		/* get it going in case there weren't any queued */
		FMVAudioSetUpNextRead();
		Enable( interrupts );
	}

	return( 0 );
}

/************************************************************************
 * FMVAudioCmdWrite - Result of an I/O Request write command.
 */
int32 FMVAudioCmdWrite( IOReq *ior )
{
	uint32 interrupts;

	/* queue the write request */
	ior->io_Flags &= ~IO_QUICK;
	QueueWrite( ior );

	{	/* [TBD] We should not need to disable interrupts here, but we're
		 * doing it now to be safe.
		 */
		interrupts = Disable();
		/* get it going in case there weren't any queued */
		FMVAudioSetUpNextWrite();
		Enable( interrupts );
	}

	return( 0 );
}

/************************************************************************
 * FMVAudioCmdPlay - Initiates write and read DMAs and starts decompression.
 */
int32 FMVAudioCmdPlay()
{
	uint32 interrupts;

	{	/* [TBD] We should not need to disable interrupts here, but we're
		 * doing it now to be safe.
		 */
		interrupts = Disable();

		/* Start the first write and the first read. */
		FMVAudioSetUpNextWrite();
		FMVAudioSetUpNextRead();

		Enable( interrupts );
	}

	FMVAV110StartDecoder();

	return( 1L );
}

/************************************************************************
 * FMVAudioSetSWPTSMechanism - Sets the PTS Mechanism to hardware (false) or
 * software (true).
 */
void FMVAudioSetSWPTSMechanism(int32 useSWPTS)
{
	gUsingSoftwarePTS = useSWPTS;
}
