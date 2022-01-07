/*
	File:		FMVVideoDriverImp.h

	Contains:	xxx put contents here xxx

	Written by:	George D. Wilson Jr.

	Copyright:	© 1993 by 3DO, Inc., all rights reserved.

	Change History (most recent first):

		<11>	11/17/94	GDW		Changed arguments passed to pause routine.
		<10>	 9/12/94	GDW		Added some 3500 error fields in debug structure.
		 <9>	 8/23/94	GDW		Added some 3500 error fields in debug structure.
		 <8>	  8/3/94	GDW		Fixed the debug structure.
		 <7>	  8/3/94	GDW		Added new low level primitives.
		 <6>	 7/12/94	GM		Added definition of VINT_PICSTART for new start code detection
									interrupt.
		 <5>	  7/6/94	GDW		Added device change routine.
		 <4>	  5/3/94	GDW		Added FMVVidDecClose.
		 <3>	 2/28/94	GM		Added prototyope for FMVVideNewBitstream
		 <2>	 2/25/94	GM		Added prototypes for FMVVidDecSkipFrames and FMVVidDecKeyFrames.
		 <1>	 2/14/94	GDW		first checked in
									support download of microcode from Opera filesystem.

*/

#define VINT_PICSTART	0x1000L	/* picture start code detect */
#define VINT_SCOMPLETE	0x0800L	/* scan complete interrupt */
#define VINT_RDYFORDATA	0x0400L	/* ready for data interrupt */
#define VINT_SEQHDRCODE	0x0200L	/* sequence header code found */
#define VINT_BUFFEMPTY	0x0100L	/* input buffer empty interrupt */
#define VINT_NEWPTS		0x0080L	/* new pts available */
#define VINT_PICDECODED	0x0040L	/* picture decoded interrupt */
#define VINT_SEQENDCODE	0x0020L	/* sequence end code found */
#define VINT_ENDSEQ		0x0010L	/* end of sequence interrupt */
#define VINT_STARTSEQ	0x0008L	/* start of sequence interrupt */
#define VINT_NEWGROUP	0x0004L	/* new group of pictures interrupt */
#define VINT_NEWPICTURE	0x0002L	/* new picture interrupt */
#define VINT_DATAERROR	0x0001L	/* data error interrupt */

// debugging stuff
typedef struct {
	int32 readsReceived, readsCompleted, readDMAInterrupts;
	int32 writesReceived, writesCompleted, writeDMAInterrupts;
	int32 newPicture, newPTS, PTSWrites;
	int32 scanCompletes,flushCount,maxAbortTime;
	int32 errorInterrupts,bufferUnderflow,discontinuities;
	int32 aborts,inProgressReadAborts,inProgressWriteAborts;
	int32 seriousError, pipeLineErrors,watchDog;
	int32 possibleUnderflow, reserved1, reserved2;
} FMVVidDebugInfoRecord, *FMVVidDebugInfoRecordPtr;


int32	FMVVidDecReadPTS( uint32 *resultPTS );
void	FMVVidDecReadSCR( uint32 *resultSCR, uint32 *count );
int32	FMVVidDecInit( int32 pixelMode, int32 resampleMode, int32 xOffset, int32 yOffset, int32 xSize, int32 ySize );
int32	FMVVidDecClose(void);
int32	FMVVidDecOpen( int32 pixelMode, int32 resampleMode, int32 xOffset, int32 yOffset, int32 xSize, int32 ySize );
int32	FMVVidDecInquireBufferFullness( void );
int32	FMVVidDecSetVideoRate50( void );
int32	FMVVidDecSetVideoRate60( void );
int32	FMVVidDecSetWindow( uint32 xoff, uint32 yoff, uint32 width, uint32 height );
int32	FMVVidDecNewPacket( uint32 length, uint32 isValid, uint32 ts );
int32	FMVVidDecPlay( void );
int32	FMVVidDecPause( uint32 frameType );
int32	FMVVidDecSingleStep( void );
void	FMVVidDecClearInterrupt( void );
int32	FMVVidDecSetEnabledInterrupts( uint32 interruptMask );
int32	FMVVidDecEnableInterrupts( uint32 interruptToEnable );
int32	FMVVidDecDisableInterrupts( uint32 interruptsToDisable );
uint32	FMVVidDecCheckInterrupt( void );
int32	FMVVidDecNewBitstream( void );
int32	FMVVidDecSkipFrames( int32 count );
int32	FMVVidDecKeyFrames( void );
int32	FMVVidDecDeviceChange(int32 pixelMode, int32 resampleMode, int32 xOffset, int32 yOffset, int32 xSize, int32 ySize);
void	FMVVidDecSetSCR( uint32 theSCR );
uint32	FMVVidDecGetSCR( void );
