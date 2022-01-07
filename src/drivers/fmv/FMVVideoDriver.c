/*
	File:		FMVVideoDriver.c

	Contains:	xxx put contents here xxx

	Written by:	xxx put writers here xxx

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<41>	11/17/94	GDW		Pause tag arg added.
		<40>	10/13/94	DM		Added flag and check when MPEGVideoDevOpen and disable/enable
									slow bus interrupt when installing the interrupt handler.
		<39>	 9/12/94	GDW
		<38>	  9/7/94	GM		Modified handling of scan complete interrupt to send a play
									command if we are not in scan mode. This seems to fix a not well
									understood bug in which the CL450 is left in pause state after
									completing a scan command.
		<37>	 8/16/94	GM		Added code to not return old picture output from the CL450 after
									a video input stream change (this is a hack, but it works).
									Added tossPicture flag.
		<36>	  8/4/94	GDW		Check for Rev 2 verses Rev A silicon.
		<35>	 7/29/94	GM		Moved flush after abort code and modified to prevent multiple
									executions. Added flushCount status variable. Fixed increment of
									read and write completion status variables.
		<34>	 7/18/94	GM		Removed start of next DMA write from abort code to reduce
									interrupt latency. Added flush flag and moved flush code to
									FMVVideoCmdWrite.
		<33>	 7/12/94	GM		Removed unused FMVVideoCmdSetSize. Fixed interupt disable/enable
									in FMVVideoCmdWrite. Fixed general handling of interrupt
									disables/enables (changed global interrupts variable to be stack
									based). Removed interrupts disabling from abort code. Fixed
									spelling of bufEmptyInterruptPending. Added (empty) handlers for
									pic decoded and start code detect interrupts. Fixed check for
									even byte boundaries of dma addresses and lengths. Removed some
									debugging code.
		<32>	  7/7/94	GDW		Read and writes clear IO_QUICK bit.
		<31>	  6/2/94	GDW		Added SCR tag argument.
		<30>	  5/3/94	GDW
		<29>	 4/21/94	GDW		Added some MIABuild checks.
		<28>	 4/15/94	GDW		Added PlayThrough mode.
		<27>	  4/8/94	GM		Removed bug fix from <26>. Must use microcode no 96 or greater.
		<26>	  4/8/94	GM		Goofed on change history <25>. Also changed PTSOffset to return
									video read retry count. Worked around a bug in the cl450 no 95
									microcode which did not generate a new picture interrupt while
									in I frame search mode. This bug should be fixed in future
									microcode revs. The change is in the handling of the scan
									complete interrupt.
		<25>	  4/8/94	GM		Modified checking of frame skip count.
		<24>	  4/5/94	BCK		Changed from FMVTimeStamps to FMVIOReqOptions structure.

		<23>	  4/4/94	GM		Modified debugging info structure so that it's easier to read in
									the debugger.
		<22>	 3/30/94	GM		Added checks to be sure reads are pending before starting
									writes. This compensates for a Woody reset condition which
									continuously provide h and v syncs to the CL450 until Woody
									receives a CLIO DMA end code.
		<21>	 3/27/94	GM		Added more debugging information including logging of PTS queue
									activity.
		<20>	 3/18/94	GM		Changed handling of video PTSs. The CL450 now generates an
									interrupt when a new PTS is available. This is kept in a queue
									and matched with the corresponding new picture.
		<19>	  3/8/94	GM		Debug version sets the PTS offset field of video read requests
									to the decoding time (in audio ticks).
		<18>	  3/7/94	GM		Fixed initialization and reset of forcePicture flag.
		<17>	  3/1/94	GM		Cleaned up code for transition to new API.
		<16>	 2/28/94	GM		Fixed bugs in Jed's cmdcontrol code.
		<15>	 2/25/94	GM		Added support for I frame decode and B picture skipping. Fixed
									abort and close routines. Added & cleaned up debugging code.
		<14>	 2/25/94	GDW		Global variable drill.
		<13>	 2/14/94	GDW		New header file with low level interfaces.
		<12>	 2/14/94	GM		Added support for handling bitstream discontinuities
		<11>	 2/10/94	GM		Added "FMVDriver: " prefix to debugging printouts.
		<10>	 2/10/94	GDW		Changed some defines.
		 <9>	 2/10/94	GM		Changed video decoder public routine name prefix to FMVVidDec
									from FMV450 to assist in development of Thomson version driver.
		 <8>	  2/8/94	GDW		Added control and new size function.
		 <7>	  2/7/94	GDW		Added Status and Control routine changes for new API.
		 <6>	  2/2/94	GM		Fixed handling of buffer underflow interrupt from CL450.
		 <5>	  2/1/94	GM		Cleaned up old, unused code. Now enables and handles CL450
									interrupts including new picture and buffer underflow. Only
									outputs real pictures using the new picture interrupt. Now reads
									PTSs from the CL450 in mid picture. Added more debugging
									support. Fixed PTS related bugs - was sending wrong size to
									NewPacket command.
		 <4>	 1/14/94	GM		Added support for video PTSs
		 <3>	11/19/93	GM		Did some George things.
		 <2>	11/18/93	GM		reconciled with latest pre-projector version

*/

/* file: FMVVideoDriver.c */
/* MPEG video decoder initialization, interrupt, control, and DMA routines */
/* 4/28/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#include "types.h"
#include "audio.h"
#include "debug.h"
#include "io.h"
#include "interrupts.h"
#include "inthard.h"
#include "super.h"
#include "operror.h"

#include "CL450.h"
#include "FMV.h"
#include "FMVDriver.h"
#include "FMVVideoDriver.h"
#include "Hollywood.h"
#include "Woody.h"
#include "FMVVideoDriverImp.h"

#define DBUG(x)		 DEBUGP(x)

FMVVidDebugInfoRecord FMVVidDebugInfo = { 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L, 0L };

#define PTSBUFFERSIZE 512

int32 FMVVidPTSIn[ PTSBUFFERSIZE ], gPTSInIndex = 0L;
int32 FMVVidPTSOut[ PTSBUFFERSIZE ], gPTSOutIndex = 0L;

int32 gPixelMode = 16L, gResampleMode = kCODEC_SQUARE_RESAMPLE;
int32 gXOffset=WDYXOFFSET,gYOffset=WDYYOFFSET;
int32 gXSize=WDYWINDOWWIDTH,gYSize=WDYWINDOWHEIGHT;
int32 gLastVideoPTS = -1, gFMVVidFramesToSkip = 0L;
int32 gDitherMode = 0L;

#if MIABUILD == 1
uint32	gVidPlayThroughMode = false;					// DMA is the default
//int32	gSCR;											// Global SCR for playthrough mode
//Boolean gNewSCR;										// Goes true when user supplies SCR
bool	gMPEGVideoDevOpen ;

#endif

struct
{
	unsigned ditherMode:8, playMode: 8;
	unsigned newPicture:1, tossPicture:1, abortPicture:1, forcePicture:1, flushFlag:1 ;
}
gFlags;

/* pending io requests */

List gMPEGVideoReadQueue, gMPEGVideoWriteQueue;
IOReq *gCurrentMPEGVideoReadReq = (IOReq *) NULL;
IOReq *gCurrentMPEGVideoWriteReq = (IOReq *) NULL;

/* DMA read channel */
static vuint32 *gDMAFromVideoDecoderDst = UNCLEtoRAM;
static vuint32 *gDMAFromVideoDecoderLen = UNCLEtoRAM + 1;
// uint32 gNextLen = 0L;

/* DMA write channel */
static vuint32 *gDMAToVideoDecoderSrc = RAMtoUNCLE;
static vuint32 *gDMAToVideoDecoderLen = RAMtoUNCLE + 1;

#define GetAudioTimeHack() GetAudioTime()

static void QueueRead( IOReq *ior )
{
	int32 interrupts;

	FMVVidDebugInfo.readsReceived++;

	// clear retry count
	ior->io_Extension[1] = 0L;

	interrupts = Disable();
	AddTail( &gMPEGVideoReadQueue, (Node *) ior );
	Enable( interrupts );
}

static IOReq *NextReadReq()
{
	return( (IOReq *) RemHead( &gMPEGVideoReadQueue ) );
}

static void DoDMARead()
{
	// if no read is in progress and a request is on the queue, start dma read

	if( gCurrentMPEGVideoReadReq )
		return;
#if MIABUILD == 1
	if ( !gVidPlayThroughMode ) {									// No reads in playthrough mode
#endif
		gCurrentMPEGVideoReadReq = NextReadReq();
		if( gCurrentMPEGVideoReadReq )
		{
			uint32 address, length;

			// record starting time
	//		gCurrentMPEGVideoReadReq->io_Extension[1] = (int32) GetAudioTimeHack();

			// disable dma prior to writing dma registers
			*DMAREQDIS = EN_UNCLEtoDMA;

			// write dma control registers
			address = (uint32) gCurrentMPEGVideoReadReq->io_Info.ioi_Recv.iob_Buffer;
			length = (uint32) gCurrentMPEGVideoReadReq->io_Info.ioi_Recv.iob_Len - 4L;
	//		length = gNextLen = length >> 1;

			*gDMAFromVideoDecoderDst = (vuint32) address;
			*gDMAFromVideoDecoderLen = (vuint32) length;

			// enable dma completion interrupt
			EnableInterrupt( INT_UNCLEIN );

			// enable opera dma
			*DMAREQEN = EN_UNCLEtoDMA;

			// enable woody dma
			FMVEnableVideoDMAOut();
		}
#if MIABUILD == 1
	} // If Not PlayThrough mode
#endif
}

/* this interrupt handler is called when a DMA read (picture output) completes */

#define FMVInternalSetPTSValue( ior, pts )		((ior)->io_Extension[FMV_PTS_INDEX] = (pts))
#define FMVInternalSetPTSOffset( ior, offset )	((ior)->io_Extension[FMV_PTS_OFFSET] = (offset))

#define PTS_QUEUE_SIZE 64L

struct
{
	int32 readIndex;
	int32 writeIndex;
	int32 count;
	int32 valid[ PTS_QUEUE_SIZE ];
	int32 value[ PTS_QUEUE_SIZE ];
}
gPTSQueue = { 0L, 0L, 0L };

static void FlushPTSQueue()
{
	// clear the PTS queue
	gPTSQueue.writeIndex = 0L;		// write ptr
	gPTSQueue.readIndex = 0L;		// read ptr
	gPTSQueue.count = 0L;			// count
}

static void StartPTSQueue()
{
	// clear the PTS queue
	gPTSQueue.writeIndex = 0L;		// write ptr
	gPTSQueue.readIndex = 1L;		// first pts is bogus
	gPTSQueue.count = -1L;			// first pts is bogus

	gPTSInIndex = 0L;				// debugging
	gPTSOutIndex = 0L;				// debugging
}

// read the pts queue
// return 1 if the pts is valid
// return 0 if the pts is invalid
static int32 ReadPTSQueue( int32 *thePTS)
{
	int32 result;

	// any entries?
	if( gPTSQueue.count > 0L )
	{
		result  = gPTSQueue.valid[ gPTSQueue.readIndex ];
		*thePTS = gPTSQueue.value[ gPTSQueue.readIndex++ ];
		if( gPTSQueue.readIndex >= PTS_QUEUE_SIZE )
			gPTSQueue.readIndex = 0L;
		gPTSQueue.count--;
	}
	else
		result = 0L;

	if( gPTSOutIndex < PTSBUFFERSIZE )
		FMVVidPTSOut[ gPTSOutIndex++ ] = *thePTS | (result << 31) |(gPTSQueue.count << 24);

	return( result );
}

static int32 WritePTSQueue( int32 valid, int32 value )
{
	FMVVidDebugInfo.PTSWrites++;

	if( gPTSInIndex < PTSBUFFERSIZE )
		FMVVidPTSIn[ gPTSInIndex++ ] = value | (valid << 31) | (gPTSQueue.count << 24);

	if( ++gPTSQueue.count > PTS_QUEUE_SIZE )
	{
		if( gPTSInIndex < PTSBUFFERSIZE )
			FMVVidPTSIn[ gPTSInIndex++ ] = 0x0bad0badL;
		gPTSQueue.count--;
		return( -1L );
	}
	gPTSQueue.valid[ gPTSQueue.writeIndex ] = valid;
	gPTSQueue.value[ gPTSQueue.writeIndex++ ] = value;
	if( gPTSQueue.writeIndex >= PTS_QUEUE_SIZE )
		gPTSQueue.writeIndex = 0L;

	return( 0L );
}

int32 FMVVideoDMAtoRAMIntr( void )
{
	IOReq *savedIOReq;
	int32 thePTS;

#if MIABUILD == 1
	if(!gMPEGVideoDevOpen) return (0) ;
#endif

	FMVVidDebugInfo.readDMAInterrupts++;

	// is this the second half of a two part dma? (need to read PTS in middle)
	if( gCurrentMPEGVideoReadReq )
	{
		// if we are skipping pictures, do it here
		if( gFMVVidFramesToSkip )
			gFMVVidFramesToSkip = FMVVidDecSkipFrames( gFMVVidFramesToSkip );

		/* the following has to occur in order. see Dale Luck */
		savedIOReq = gCurrentMPEGVideoReadReq;
		gCurrentMPEGVideoReadReq = (IOReq *) NULL;

		// is the picture we just dma'd a real one?
		if( gFlags.abortPicture )
		{
			// the picture has been aborted due to a flush
			gFlags.abortPicture = 0;			// clear the flag
			ReadPTSQueue( &thePTS );			// read unused PTS from the PTS queue

			// restart the request
			AddHead( &gMPEGVideoReadQueue, (Node *) savedIOReq );
			DoDMARead();
			return( 0L );
		}
		if( gFlags.tossPicture )
		{
			// the CL450 will output a picture from the previous stream
			// this is a hack to get rid of it
			gFlags.tossPicture = 0;				// clear the flag
			ReadPTSQueue( &thePTS );			// read unused PTS from the PTS queue

			// restart the request
			AddHead( &gMPEGVideoReadQueue, (Node *) savedIOReq );
			DoDMARead();
			return( 0L );
		}
		if( gFlags.newPicture | gFlags.forcePicture )
		{
			// clear PTS valid flag
			savedIOReq->io_Flags &= ~FMVValidPTS;	// pts is invalid until proven valid
			if( gFlags.newPicture )
			{
				// picture readout has completed, get timestamp info
				if( ReadPTSQueue( &thePTS ) )
				{
					savedIOReq->io_Flags |= FMVValidPTS;	// pts is valid
					FMVInternalSetPTSValue( savedIOReq, thePTS );
				}
			}
			gFlags.newPicture = 0; gFlags.forcePicture = 0;
		}
		else
		{
			// the picture we just read wasn't valid, restart the read request
			// increment retry count
			savedIOReq->io_Extension[1]++;
			AddHead( &gMPEGVideoReadQueue, (Node *) savedIOReq );
			DoDMARead();
			return( 0L );
		}
		// set up next read if there is one
		DoDMARead();

		savedIOReq->io_Actual = savedIOReq->io_Info.ioi_Recv.iob_Len;

		SuperCompleteIO( savedIOReq );
		FMVVidDebugInfo.readsCompleted++;
	}
	return( 0 );
}

static void QueueWrite( IOReq *ior )
{
	int32 interrupts;

	FMVVidDebugInfo.writesReceived++;

	interrupts = Disable();
	AddTail( &gMPEGVideoWriteQueue, (Node *) ior );
	Enable( interrupts );
}

static IOReq *NextWriteReq()
{
	return( (IOReq *) RemHead( &gMPEGVideoWriteQueue ) );
}

static uint32 bufEmptyInterruptPending = 0L;

static void DoDMAWrite()
{
	if( gCurrentMPEGVideoWriteReq )
		return;
	gCurrentMPEGVideoWriteReq = NextWriteReq();
	if( gCurrentMPEGVideoWriteReq )
	{
		vuint32 src = (vuint32) gCurrentMPEGVideoWriteReq->io_Info.ioi_Send.iob_Buffer;
		vuint32 len = (vuint32) gCurrentMPEGVideoWriteReq->io_Info.ioi_Send.iob_Len;

		// a transfer length of zero indicates a discontinuity
		// in the video bitstream
		if( !len )
		{
			IOReq *savedIOReq = gCurrentMPEGVideoWriteReq;

			// don't return the next picture
			gFlags.abortPicture = 1;
			gFlags.newPicture = 0;

			// signal the video decoder to expect a new bitstream
			gFlags.flushFlag = 0;
			FMVVidDebugInfo.flushCount++;
			FMVVidDecNewBitstream();
			FlushPTSQueue();

			// the CL450 will output a picture from the old stream
			// get rid of it at the next dma completion (hack)
			gFlags.tossPicture = 1;

			// complete this IOReq
			gCurrentMPEGVideoWriteReq = (IOReq *) NULL;
			SuperCompleteIO( savedIOReq );
			FMVVidDebugInfo.writesCompleted++;

			// fire up next write request
			DoDMAWrite();
			FMVVidDebugInfo.discontinuities++;

			return;
		}
		else if( gFlags.flushFlag )
		{
			gFlags.flushFlag = 0;
			FMVVidDebugInfo.flushCount++;
			FMVVidDecNewBitstream();
			FlushPTSQueue();

			// the CL450 will output a picture from the old stream
			// get rid of it at the next dma completion (hack)
			gFlags.tossPicture = 1;
		}

		// send a new packet command to associate a timestamp with the encoded data
		// is there an FMVTimeStamps struct associated with this io request?
		if( gCurrentMPEGVideoWriteReq->io_Info.ioi_CmdOptions )
		{
			FMVIOReqOptionsPtr TS;

			TS = (FMVIOReqOptionsPtr) gCurrentMPEGVideoWriteReq->io_Info.ioi_CmdOptions;

			if( FMVFlags(TS) & FMVValidPTS )
			{
				// pts in io request is valid, send it to the decoder
				FMVVidDecNewPacket( len, 1L, TS->FMVOpt_PTS );
			}
			else
				// no pts in current packet, but send count so the decoder can keep track
				FMVVidDecNewPacket( len, 0L, 0L );
		}
		else
			// no pts in current packet, but send count so the decoder can keep track
			FMVVidDecNewPacket( len, 0L, 0L );

		if( src & 2L )					// is the source on an odd halfword?
		{
			src = src & ~3L;			// if yes, start dma at lower long word boundary
			len += 2L;					// increase dma length by two bytes
			WdySetControlSet( WDYSFVH );// skip first video halfword
		}
		if( len & 2L )					// is the length not a multiple of four?
		{
			len += 2L;					// if yes, transfer an extra two bytes
			WdySetControlSet( WDYSLVH );// skip last video halfword
		}
		// disable dma
		*DMAREQDIS = EN_DMAtoUNCLE;

		// set up next write dma
		*gDMAToVideoDecoderSrc = src;
		*gDMAToVideoDecoderLen = len - 4L;

		// enable dma completion interrupt */
		EnableInterrupt( INT_UNCLEOUT );

		// enable dma
		*DMAREQEN = EN_DMAtoUNCLE;

		// enable woody
		FMVEnableVideoDMAIn();

		if( bufEmptyInterruptPending )
		{
			// clear the CL450 interrupt status register, this
			// enables the CL450 buffer empty interrupt
			FMV450ClearInterruptStatus();
			bufEmptyInterruptPending = 0L;
		}
	}
}

Item IntrVidDMAtoRAM, IntrVidDMAfrRAM, IntrVideo;

/* this interrupt handler is called when a DMA write (data into woody) completes */

int32 FMVVideoDMAfrRAMIntr( void )
{
	IOReq *savedIOReq;

#if MIABUILD == 1
	if(!gMPEGVideoDevOpen) return (0) ;
#endif
	FMVVidDebugInfo.writeDMAInterrupts++;

	if( gCurrentMPEGVideoWriteReq )
	{
		savedIOReq = gCurrentMPEGVideoWriteReq;
		gCurrentMPEGVideoWriteReq = (IOReq *) NULL;

		/* set up next write if there is one */
		DoDMAWrite();

		savedIOReq->io_Actual = savedIOReq->io_Info.ioi_Send.iob_Len;

		SuperCompleteIO( savedIOReq );
		FMVVidDebugInfo.writesCompleted++;

		if ( FMVVidDebugInfo.writesCompleted == FMVVidDebugInfo.writesReceived ) {
			FMVVidDebugInfo.possibleUnderflow++;
		}
	}
	return( 0 );
}

/* this interrupt handler is called when the FMV board generates an interrupt */
/* handle only video interrupts */

int32 FMVVideoIntr( void )
{
	uint32 intStatus;

#if MIABUILD == 1
	if(!gMPEGVideoDevOpen) return (0) ;
#endif
	intStatus = FMVVidDecCheckInterrupt();
	if( intStatus )
	{
		// cause the video decoder to deassert the interrupt line
		FMVVidDecClearInterrupt();

		// check each possible video decoder interrupt
		if( intStatus & VINT_PICSTART )		// picture start code detected
		{
//			extern int32 logPTSOffset;
//			uint32 dummy;

//			logPTSOffset = 1L;
//			// read the current pts
//			FMVVidDecReadPTS( &dummy );
		}
		if( intStatus & VINT_NEWPTS )		// new pts available
		{
			uint32 valid,pts;

			// read the current pts
			valid = FMVVidDecReadPTS( &pts );
			WritePTSQueue( valid, pts );
			FMVVidDebugInfo.newPTS++;
		}
		if( intStatus & VINT_SCOMPLETE )	// scan complete
		{
			// if still in the scan state, start next scan
			if( gFlags.playMode == FMVVIDDECMODEKEYFRAMES )
				FMVVidDecKeyFrames();
			else if( gFlags.playMode == FMVVIDDECMODEPLAY )
				FMVVidDecPlay();

			// write the pts queue to keep it in sync
			WritePTSQueue( 0L, 0L );
			FMVVidDebugInfo.scanCompletes++;
		}
		if( intStatus & VINT_NEWPICTURE )	// new picture display
		{
			FMVVidDebugInfo.newPicture++;
			gFlags.newPicture = 1;
		}
		if( intStatus & VINT_DATAERROR )	// data error detected
		{
			FMVVidDebugInfo.errorInterrupts++;
		}
		if( intStatus & VINT_PICDECODED )	// new picture decoded
		{
		}
		// the buffer empty interrupt reoccurs as long as the condition is true
		// so we need to handle it in a special way
		if( intStatus & VINT_BUFFEMPTY )	// buffer underflow
		{
			bufEmptyInterruptPending++;
			FMVVidDebugInfo.bufferUnderflow++;
		}
		else
		{
			// for all other interrupts clear interrupt status register
			// in the CL450, this re-enables interrupts
			FMV450ClearInterruptStatus();
		}
	}
	return( 0 );
}

int32 FMVVideoSetSize( int32 hSize, int32 vSize )
{
	/* x size and y size */
	gXSize = hSize;
	gYSize = vSize;

	/* check ranges */
	if( (gXSize < 0L) || (gXSize > 0x1ffL) ||
		(gYSize < 0L) || (gYSize > 0x1ffL) )
		return(-1L);

	/* check alignment */
	if( (gXSize & 0x3fL) || (gYSize & 0x1L) )
		return(-1L);
	return( 1L );

}

//-----------------------------------------------------------------------------

int32 FMVVideoInit( void )
{
	DBUG(("FMVVideoInit: &FMVVidDebugInfo = %08lx\n",&FMVVidDebugInfo));

	gPixelMode = 16L;
	gResampleMode = kCODEC_SQUARE_RESAMPLE;
	gXOffset=WDYXOFFSET,gYOffset=WDYYOFFSET;
	gXSize=WDYWINDOWWIDTH,gYSize=WDYWINDOWHEIGHT;

	WdySetControlSet( WDYRESETVIDEO );
//	WdySetControlClear( WDYRESETVIDEO );

	*DMAREQDIS = EN_UNCLEtoDMA;
	DisableInterrupt( INT_UNCLEIN );

	*DMAREQDIS = EN_DMAtoUNCLE;					/* stop Opera DMA to video decoder */
	DisableInterrupt( INT_UNCLEOUT );

	/* install interrupt routines */
	IntrVidDMAtoRAM = SuperCreateFIRQ("FMVVideoDMAtoRAMIntr",150,FMVVideoDMAtoRAMIntr,INT_UNCLEIN);
	IntrVidDMAfrRAM = SuperCreateFIRQ("FMVVideoDMAfrRAMIntr",150,FMVVideoDMAfrRAMIntr,INT_UNCLEOUT);
	DisableInterrupt( INT_PD ) ;
	IntrVideo       = SuperCreateFIRQ("FMVVideoIntr",150,FMVVideoIntr,INT_PD);
	EnableInterrupt( INT_PD ) ;
	if( IntrVidDMAtoRAM < 0 )
		return( IntrVidDMAtoRAM );

	if( IntrVidDMAfrRAM < 0 )
		return( IntrVidDMAfrRAM );

	if( IntrVideo < 0 )
		return( IntrVideo );

	/* initialize read/write queues */
	InitList( &gMPEGVideoReadQueue, "MPEG Video Read Queue" );

	InitList( &gMPEGVideoWriteQueue, "MPEG Video Write Queue" );

	return( 0 );
}

void FMVVideoDevOpen( Device *dev )
{
	DBUG(("FMVVideoDevOpen"));

	StartPTSQueue();
	gFlags.newPicture = 0; gFlags.abortPicture = 0; gFlags.tossPicture = 0;
	gFlags.forcePicture = 0; gFlags.flushFlag = 0;
	gLastVideoPTS = -1L, gFMVVidFramesToSkip = 0L;
	gCurrentMPEGVideoReadReq = (IOReq *) NULL;
	gCurrentMPEGVideoWriteReq = (IOReq *) NULL;

#if MIABUILD == 1
//	gSCR = 0L;
//	gNewSCR = false;								// Default is we generate SCRs
	gMPEGVideoDevOpen = TRUE ;
#endif

	/* enable woody dma (bug fix) */
	if( gWoodyRevision < 3L )
		WdySetControlSet( WDYVDMAOUTENABLE );

	/* reset video subsystem */
	DEBUGP(("video reset\n"));
	WdySetControlSet( WDYRESETVIDEO );
	FMVVidDecOpen(gPixelMode, gResampleMode, gXOffset, gYOffset, gXSize, gYSize);
}

void FMVVideoDevClose( Device *dev )
{
	IOReq *ior;

	FMVVidDecClose();								// Give low level a chance

	/* clear the queues */
	// just delete the IOReqs
	// abortio will automatically get called and remove them
	while( !IsEmptyList( &gMPEGVideoReadQueue ) )
	{
		ior = (IOReq *) LastNode( &gMPEGVideoReadQueue );
		SuperInternalDeleteItem( ior->io.n_Item );
	}
	while( !IsEmptyList( &gMPEGVideoWriteQueue ) )
	{
		ior = (IOReq *) LastNode( &gMPEGVideoWriteQueue );
		SuperInternalDeleteItem( ior->io.n_Item );
	}
	ior = gCurrentMPEGVideoReadReq;
	if( ior )
	{
		SuperInternalDeleteItem( ior->io.n_Item );
	}
	ior = gCurrentMPEGVideoWriteReq;
	if( ior )
	{
		SuperInternalDeleteItem( ior->io.n_Item );
	}
#if MIABUILD == 1
	gMPEGVideoDevOpen = FALSE ;
#endif


}

void FMVVideoAbortIO( IOReq *ior )
{
	FMVVidDebugInfo.aborts++;
//	DBUG(("FMVVideoAbortIO"));

	if( ior == gCurrentMPEGVideoReadReq )
	{
		FMVVidDebugInfo.inProgressReadAborts++;
		// force read to complete
		gFlags.forcePicture = 1;
		ior->io_Error = ABORTED;
	}
	else if( ior == gCurrentMPEGVideoWriteReq )
	{
		FMVVidDebugInfo.inProgressWriteAborts++;
		/* cancel current request */
		FMVDisableVideoDMAIn();

		/* be sure dma hasn't just finished */
		if( *gDMAToVideoDecoderLen == 0xfffffffcL )
		{
			return;
		}
		ior->io_Error = ABORTED;
		ior->io_Actual = 0L;
		gCurrentMPEGVideoWriteReq = (IOReq *) NULL;

		// flag a flush of the decoder
		gFlags.flushFlag = 1;

		// don't return the next picture
		gFlags.abortPicture = 1;

		/* set up next write if there is one */
//		DoDMAWrite();
		SuperCompleteIO( ior );
		FMVVidDebugInfo.writesCompleted++;
	}
	else
	{
		RemNode( (Node *) ior );
		ior->io_Error = ABORTED;
		ior->io_Actual = 0L;
		SuperCompleteIO( ior );
		if(ior->io_Info.ioi_Command == CMD_WRITE)
			FMVVidDebugInfo.writesCompleted++;
		else
			FMVVidDebugInfo.readsCompleted++;
	}
}

//-----------------------------------------------------------------------------

int32 FMVVideoCmdRead( IOReq *ior )
{
	int32 interrupts;

	/* queue the read request. the real read won't take place until the */
	/* video decoder has actually decoded a picture */
	ior->io_Flags &= ~IO_QUICK;
	QueueRead( ior );

	/* do we need to kick start the reads? */
	if( !gCurrentMPEGVideoReadReq )
	{
		/* kick start the read */
		interrupts = Disable();
		DoDMARead();
		Enable( interrupts );
	}
	/* do we need to kick start the writes? */
	/* we need to do this here because we must have reads queued before */
	/* we start writes otherwise we'll lose the first pictures out of */
	/* the CL450 thanks to Mr. Woody */
	if( !gCurrentMPEGVideoWriteReq )
	{
		interrupts = Disable();
		DoDMAWrite();
		Enable( interrupts );
	}
	return( 0L );
}

int32 FMVVideoCmdStatus( IOReq *ior )
{
	CODECDeviceStatus		*dst;
	int32					len;
	TagArgP					codecStatPtr;
	int						i;
	uint32					capFlags;

	dst = (CODECDeviceStatusPtr) ior->io_Info.ioi_Recv.iob_Buffer;
	len = ior->io_Info.ioi_Recv.iob_Len;

	if ( dst == NULL ) return(1L);					// Nothing passed leave

	capFlags = (kVideoCODECOutDoesDither |
				kVideoCODECOutDoesMPEGI |
				kVideoCODECOutDoesDMA |
				kVideoCODECOutDoesVariableSize |
				kVideoCODECOutDoesSquarePixel |
				kVideoCODECOutDoesNTSC |
				kVideoCODECOutDoesPAL |
				kVideoCODECOutDoes16BPP |
#if MIABUILD == 1
				kVideoCODECOutDoesPlayThrough |
#endif
				kVideoCODECOutDoes24BPP);
	codecStatPtr = dst->codec_TagArg;				// Get a temporary pointer

	i = 0;

	if ( codecStatPtr[i].ta_Tag == TAG_END ) {		// Did they pass an empty struct?
		if ( len >= sizeof(CODECDeviceStatus) ) {
			codecStatPtr[0].ta_Tag = VID_CODEC_TAG_CAPABILITIES;
			codecStatPtr[0].ta_Arg = (void *) capFlags;
			codecStatPtr[1].ta_Tag = VID_CODEC_TAG_HSIZE;
			codecStatPtr[1].ta_Arg = (void *) gXSize;
			codecStatPtr[2].ta_Tag = VID_CODEC_TAG_VSIZE;
			codecStatPtr[2].ta_Arg = (void *) gYSize;
			codecStatPtr[3].ta_Tag = VID_CODEC_TAG_DEPTH;
			codecStatPtr[3].ta_Arg = (void *) gPixelMode;
			codecStatPtr[4].ta_Tag = VID_CODEC_TAG_DITHER;
			codecStatPtr[4].ta_Arg = (void *) gDitherMode;
			codecStatPtr[5].ta_Tag = VID_CODEC_TAG_STANDARD;
			codecStatPtr[5].ta_Arg = (void *) gResampleMode;
#if MIABUILD == 1
			codecStatPtr[6].ta_Tag = VID_CODEC_TAG_PLAYTHROUGH;
			codecStatPtr[6].ta_Arg = (void *) gVidPlayThroughMode;
			codecStatPtr[7].ta_Tag = VID_CODEC_TAG_SCR;
			codecStatPtr[7].ta_Arg = (void *) FMVVidDecGetSCR();
#endif
			return(1L);								// We filled it all
		}
	}

	while ( codecStatPtr[i].ta_Tag != TAG_END ) {	// Have we reached the end yet?
		switch ( codecStatPtr[i].ta_Tag ) {
			case VID_CODEC_TAG_CAPABILITIES:
				codecStatPtr[i].ta_Arg = (void *) capFlags;
				break;
			case VID_CODEC_TAG_HSIZE:
				codecStatPtr[i].ta_Arg = (void *) gXSize;
				break;
			case VID_CODEC_TAG_VSIZE:
				codecStatPtr[i].ta_Arg = (void *) gYSize;
				break;
			case VID_CODEC_TAG_DEPTH:
				codecStatPtr[i].ta_Arg = (void *) gPixelMode;
				break;
			case VID_CODEC_TAG_DITHER:
				codecStatPtr[i].ta_Arg = (void *) gDitherMode;
				break;
			case VID_CODEC_TAG_STANDARD:
				codecStatPtr[i].ta_Arg = (void *) gResampleMode;
				break;
#if MIABUILD == 1
			case VID_CODEC_TAG_PLAYTHROUGH:
				codecStatPtr[i].ta_Arg = (void *) gVidPlayThroughMode;
				break;
			case VID_CODEC_TAG_SCR:
				codecStatPtr[i].ta_Arg = (void *) FMVVidDecGetSCR();
				break;
#endif
		}
		i++;										// Next entry
	}
	return( 1L );
}

int32 FMVVideoCmdControl( IOReq *ior )
{
	CODECDeviceStatus		*dst;
	int32					len;
	TagArgP					codecStatPtr;
	int						i;
	int32					err;
	int32					setPixelDepth;
	Boolean					deviceChange;

	deviceChange = false;
	dst = (CODECDeviceStatusPtr) ior->io_Info.ioi_Send.iob_Buffer;
	len = ior->io_Info.ioi_Send.iob_Len;

	err = 0L;										// Default return code
	setPixelDepth = 0L;

	if ( dst == NULL )
		return(1L);									// Nothing passed exit
	codecStatPtr = dst->codec_TagArg;				// Get a temporary pointer

	// loop through tag args
	for( i = 0; codecStatPtr[i].ta_Tag != TAG_END; i++ )
	{
		switch ( codecStatPtr[i].ta_Tag )
		{
			case VID_CODEC_TAG_HSIZE:
				err = FMVVideoSetSize((int32) codecStatPtr[i].ta_Arg,gYSize);
				deviceChange = true;
				break;
			case VID_CODEC_TAG_VSIZE:
				err = FMVVideoSetSize(gXSize,(int32) codecStatPtr[i].ta_Arg);
				deviceChange = true;
				break;
			case VID_CODEC_TAG_DEPTH:
				setPixelDepth = (uint32) codecStatPtr[i].ta_Arg;
				deviceChange = true;
				break;
			case VID_CODEC_TAG_DITHER:
				if ( (uint32) codecStatPtr[i].ta_Arg == kCODEC_RANDOM_DITHER )
					err = FMVVideoCmdSetRandomDither(ior);
				else if ( (uint32) codecStatPtr[i].ta_Arg == kCODEC_MATRIX_DITHER )
					err = FMVVideoCmdSetMatrixDither(ior);
				deviceChange = true;
				break;
			case VID_CODEC_TAG_STANDARD:
				if ( (uint32) codecStatPtr[i].ta_Arg == kCODEC_SQUARE_RESAMPLE )
					err = FMVVideoCmdModeSquare();
				else if ( (uint32) codecStatPtr[i].ta_Arg == kCODEC_NTSC_RESAMPLE )
					err = FMVVideoCmdModeNTSC();
				if ( (uint32) codecStatPtr[i].ta_Arg == kCODEC_PAL_RESAMPLE )
					err = FMVVideoCmdModePAL();
				deviceChange = true;
				break;
			case VID_CODEC_TAG_PLAY:
				deviceChange = true;
				err = FMVVideoCmdPlay();
				break;
			case VID_CODEC_TAG_SKIPFRAMES:
				err = FMVVideoCmdSkipFrames( (int32 ) codecStatPtr[i].ta_Arg );
				break;
			case VID_CODEC_TAG_KEYFRAMES:
				err = FMVVideoCmdKeyFrames();
				break;
#if MIABUILD == 1
			case VID_CODEC_TAG_PLAYTHROUGH:
				gVidPlayThroughMode = (uint32) codecStatPtr[i].ta_Arg;
				deviceChange = true;
				break;
			case VID_CODEC_TAG_SCR:
				FMVVidDecSetSCR((uint32) codecStatPtr[i].ta_Arg);
				break;
			case VID_CODEC_TAG_PAUSE:
				err = FMVVideoCmdPause((uint32) codecStatPtr[i].ta_Arg);
				break;
#endif
			default:
				break;
		}	// end switch
	}		// end for
	if ( deviceChange ) {
		if ( setPixelDepth == 16 )
			err = FMVVideoCmdPixelMode16();
		else if ( setPixelDepth == 24L )
			err = FMVVideoCmdPixelMode24();
		FMVVidDecDeviceChange(gPixelMode, gResampleMode, gXOffset, gYOffset, gXSize, gYSize);
//		FMVVidDecOpen(gPixelMode, gResampleMode, gXOffset, gYOffset, gXSize, gYSize);
	}
	if( err  < 0L )
		ior->io_Error = MakeKErr(ER_SEVERE,ER_C_STND,ER_DeviceError);
	return( 1L );
}

int32 FMVVideoCmdWrite( IOReq *ior )
{
	int32 interrupts;

	/* queue the write request */
	ior->io_Flags &= ~IO_QUICK;
	QueueWrite( ior );
	/* should we kick start the write */
#if MIABUILD == 1
	if ( gVidPlayThroughMode ) {							// OK to have reads outstanding in this mode
		interrupts = Disable();
		DoDMAWrite();
		Enable( interrupts );
	} else {
#endif
		if( gCurrentMPEGVideoReadReq && !gCurrentMPEGVideoWriteReq )
		{
			interrupts = Disable();
			DoDMAWrite();
			Enable( interrupts );
		}
#if MIABUILD == 1
	}
#endif
	Enable( interrupts );

	return( 0L );
}

int32 FMVVideoCmdPixelMode16( void )
{
	gPixelMode = 16L;
	return( FMVVideoSetPixelMode() );
}

int32 FMVVideoCmdPixelMode24( void )
{
	gPixelMode = 24L;
	return( FMVVideoSetPixelMode() );
}

int32 FMVVideoSetPixelMode( void )
{
	int32 tryCount;

	*DMAREQDIS = EN_UNCLEtoDMA;
	DisableInterrupt( INT_UNCLEIN );

	*DMAREQDIS = EN_DMAtoUNCLE;					/* stop Opera DMA to video decoder */
	DisableInterrupt( INT_UNCLEOUT );

	/* set up woody */
	FMVWoodyVideoInit( gPixelMode, gResampleMode, gXOffset, gYOffset, gXSize, gYSize );

	/* initialize the video decoder */
	for( tryCount = 0; tryCount < 20; tryCount++ )
	{
		if( FMVVidDecInit( gPixelMode, gResampleMode, gXOffset, gYOffset, gXSize, gYSize ) == 0L )
			break;
		DBUG(("Video Decoder Initialization failed %d\n", tryCount+1L));
	}
	FMVVidDecPlay();

	return( 1L );
}

int32 FMVVideoCmdStep( IOReq *ior )
{
	return( 1L );
}

int32 FMVVideoCmdPlay( void )
{
	gFlags.playMode = FMVVIDDECMODEPLAY;
	FMVVidDecPlay();

	return( 1L );
}

#if MIABUILD == 1
int32 FMVVideoCmdPause( uint32 frameType )
{
	FMVVidDecPause(frameType);
	return( 1L );
}
#endif

int32 FMVVideoCmdStop( IOReq *ior )
{
	return( 1L );
}

#if MIABUILD == 1
/* set the window size */

int32 FMVVideoCmdSetSize( IOReq *ior )
{
	int32		err;

	err = 1L;										// Default value
	if( ior->io_Info.ioi_Send.iob_Len >= 8L ) {
		err = FMVVideoSetSize( ((int32 *) ior->io_Info.ioi_Send.iob_Buffer)[0],
							   ((int32 *) ior->io_Info.ioi_Send.iob_Buffer)[1] );
		if ( err == -1L ) {
			ior->io_Error = BADIOARG;
		}
	} else {
		ior->io_Error = BADIOARG;
		return (1);
	}
	return(err);
}
#endif

int32 FMVVideoCmdModeSquare( void )
{
	gResampleMode = kCODEC_SQUARE_RESAMPLE;

	return( 1L );
}

int32 FMVVideoCmdModeNTSC( void )
{
	gResampleMode = kCODEC_NTSC_RESAMPLE;

	return( 1L );
}

int32 FMVVideoCmdModePAL( void )
{
	gResampleMode = kCODEC_PAL_RESAMPLE;

	return( 1L );
}

int32 FMVVideoCmdSetRandomDither( IOReq *ior )
{
	return( 1L );
}

int32 FMVVideoCmdSetMatrixDither( IOReq *ior )
{
	return( 1L );
}

int32 FMVVideoCmdSkipFrames( int32 count )
{
	int32 interrupts;

	if( count < 0 )
		return( -1L );

	interrupts = Disable();			// update variable atomically
	gFMVVidFramesToSkip += count;	// add count to number of frames to skip
	// make sure we don't exceed limit
	if( gFMVVidFramesToSkip > MAXFRAMESKIP )
		gFMVVidFramesToSkip = MAXFRAMESKIP;
	Enable( interrupts );			// actual skip command will be issued
									// at picture completion time
	return( 1L );
}

int32 FMVVideoCmdKeyFrames( void )
{
	gFlags.playMode = FMVVIDDECMODEKEYFRAMES;
	FMVVidDecKeyFrames();

	return( 1L );
}
