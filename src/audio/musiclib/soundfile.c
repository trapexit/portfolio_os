/* $Id: soundfile.c,v 1.45 1994/11/16 23:00:27 phil Exp $ */
/***************************************************************
**
** Play Sound File from Disk.
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

/*
** 921116 PLB Modified for explicit mixer connect.
** 921118 PLB Added ChangeDirectory("/remote") for filesystem.
** 921202 PLB Converted to LoadInstrument and GrabKnob.
** 921203 PLB Use AUDIODATADIR instead of /remote.
** 930501 PLB Convert to use linked attachments.
** 930505 PLB Wait on SignalMask to avoid double hits.
** 930524 PLB Added CreateSoundFilePLayer
** 930614 PLB Read integral number of blocks at end.
** 930721 PLB Changed MEMTYPE_DMA to MEMTYPE_AUDIO
** 930730 PLB Added ioi_Flags2 = 0
** 930809 PLB Added SFP_NO_DIRECTOUT
** 931110 PLB Protect against writing past end of buffer if DataOffset is Past BufferEnd
** 931110 PLB Prevent hang at end by only unlinking last buffer.
** 931110 PLB In LoadSoundFile(), rewind after loading instruments so that
**			drive head is left in soundfile track.
** 931111 PLB Clear signals before and after run.
** 931213 PLB Converted to use new SoundSpooler, not released until 3/94.
** 940224 PLB Use Async IO
** 940415 PLB UnloadSoundFile was unloading sfp->sfp_Spooler->sspl_SamplerIns
**            which had already been cleared.  Thus the instrument was never
**            getting unloaded. New bug arrived with new spooler.
** 940415 PLB Removed dead static code, LoopSoundFile().
** 940421 PLB Do not play buffers past end of short files.
**            Reset Spooler in RewindSoundFile() to handle looping and reduce
**            hiccup in Start/Stop.
**            Return SignalsNeeded from StartSoundFile() to reduce loop errors.
** 940516 PLB Fixed minor warnings.
** 940727 WJB Added autodocs.
** 941013 WJB Now reads block size from file system.
** 941114 PLB Change directout.dsp priority from 0 to 100 for capture_audio.c.
** 941116 PLB Delete IOReq before file device to prevent WARNING
*/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "stdarg.h"
#include "strings.h"
#include "operror.h"
#include "stdio.h"

/* Include this when using the Audio Folio */
#include "audio.h"
#include "music.h"

#include "music_internal.h"     /* package id */

DEFINE_MUSICLIB_PACKAGE_ID(soundfile)

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define DBUGCLK(x) /* PRT(x) */
#define DEBUG_LoadSoundFile     0   /* prints out info during LoadSoundFile() (941013) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto error; \
	}

#define FINISHIO \
		if(sfp->sfp_PrivateFlags & SFP_PRIV_IO_PENDING) /* 940224 */ \
		{ \
			DBUG(("WaitIO\n")); \
			WaitIO( sfp->sfp_IOReqItem ); \
			sfp->sfp_PrivateFlags &= ~SFP_PRIV_IO_PENDING; \
		}

#define SNDF_IF_ASYNC (TRUE)

/* Debug info must be written to arrays cuz printing would mess up timing. */
/* #define DEBUG */
#ifdef DEBUG
#define MAXDATA 1000
static int32 logCurAddrA[MAXDATA];
static int32 logCurAddrB[MAXDATA];
static int32 logSignal[MAXDATA];
static int32 logCurSignal[MAXDATA];
static int32 logBufIndex[MAXDATA];
static int32 logCursor[MAXDATA];
static int32 logNumRead[MAXDATA];
static int32 logUnlink[MAXDATA];
static int32 dbgindx;
#define LOGDATA(val,arr)  { if (dbgindx < MAXDATA ) { arr[dbgindx]=val; } }
#endif


/* -------------------- Local functions */

static int32 GetFileBlockSize (Item fileioreq);


/* -------------------- Code */

/**************************************************************************/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/createsoundfileplayer
 |||	CreateSoundFilePlayer - Creates a SoundFilePlayer data structure to play
 |||	                        sound files.
 |||
 |||	  Synopsis
 |||
 |||	    SoundFilePlayer *CreateSoundFilePlayer( int32 NumBuffers,
 |||	    int32 BufSize, void *Buffers[] )
 |||
 |||	  Description
 |||
 |||	    This procedure creates a SoundFilePlayer data structure and initializes
 |||	    it.  A SoundFilePlayer can be used to play several sound files, one at a
 |||	    time.  The Buffers argument is a pointer to an array of pointers to
 |||	    preallocated audio data buffers, all the same size.  If Buffers is NULL,
 |||	    then this procedure allocates the memory for you.  The memory for this
 |||	    must be of type MEMTYPE_AUDIO.
 |||
 |||	  Arguments
 |||
 |||	    NumBuffers                   A value indicating the number of audio data
 |||	                                 buffers.
 |||
 |||	    BufSize                      A value indicating the size of the audio
 |||	                                 data buffers.
 |||
 |||	    Buffers                      A pointer to an array of pointers to
 |||	                                 preallocated audio data buffers.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns a pointer to the SoundFilePlayer data structure if
 |||	    all went well or a NULL if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Caveats
 |||
 |||	    Buffer size must be a multiple of the file's block size (see LoadSoundFile() for complete reason).
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), CloseSoundFile(), DeleteSoundFile(), LoadSoundFile(),
 |||	    ReadSoundFile(), RewindSoundFile(), ServiceSoundFile(), StartSoundFile(),
 |||	    StopSoundFile(), UnloadSoundFile()
 |||
**/
SoundFilePlayer *CreateSoundFilePlayer ( int32 NumBuffers, int32 BufSize, void *Buffers[] )
{
	SoundFilePlayer *sfp;
	SoundSpooler *sspl;

	void *MemAddr;
	int32 i;

	PULL_MUSICLIB_PACKAGE_ID(soundfile);

	if (NumBuffers < 2)
	{
		ERR(("CreateSoundFilePlayer: too few buffers requested, %d < 2\n", NumBuffers));
		return NULL;
	}
	if (NumBuffers > MAX_SOUNDFILE_BUFS)
	{
		ERR(("CreateSoundFilePlayer: too many buffers requested, %d > %d\n",
			NumBuffers, MAX_SOUNDFILE_BUFS));
		return NULL;
	}

	sfp = (SoundFilePlayer *) zalloc(sizeof(SoundFilePlayer));
	if (sfp == NULL) return NULL;

/* Create SoundSpooler data structure. */
	sspl = ssplCreateSoundSpooler( NumBuffers, 0 );
	if( sspl == NULL )
	{
		ERR(("CreateSoundFilePlayer: could not create spooler.\n"));
		goto error;
	}

	sfp->sfp_Spooler = sspl;
	sfp->sfp_BufSize = BufSize;
	sfp->sfp_NumBuffers = NumBuffers;

/* Assign memory to buffers. */
/* Create desired buffers for spooler. */
	for (i=0; i<NumBuffers; i++)
	{
		if (Buffers)
		{
			MemAddr = Buffers[i];
		}
		else
		{
			MemAddr = EZMemAlloc( BufSize, MEMTYPE_AUDIO );
			if (MemAddr == NULL)
			{
				ERR(("CreateSoundFilePlayer: Buffer allocation failed!\n"));
				goto error;
			}
		}
		sfp->sfp_BufferAddrs[i] = (char *) MemAddr;
	}

	if (Buffers == NULL) sfp->sfp_PrivateFlags |= SFP_PRIV_OWN_BUFFERS;

error:
	return sfp;
}
/**************************************************************************/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/deletesoundfileplayer
 |||	DeleteSoundFilePlayer - Deletes a SoundFilePlayer data structure.
 |||
 |||	  Synopsis
 |||
 |||	    int32 DeleteSoundFilePlayer( SoundFilePlayer *sfp )
 |||
 |||	  Description
 |||
 |||	    This procedure deletes a SoundFilePlayer data structure and cleans up
 |||	    after CreateSoundFilePlayer().  If the memory for the SoundFilePlayer was
 |||	    allocated by CreateSoundFilePlayer(), then the memory is returned to the
 |||	    system.  If you allocated your own buffers, DeleteSoundFilePlayer()
 |||	    won't free the buffers; you must do it yourself.
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), CloseSoundFile(), LoadSoundFile(), ReadSoundFile(),
 |||	    RewindSoundFile(), ServiceSoundFile(), StartSoundFile(), StopSoundFile(),
 |||	    UnloadSoundFile(), CreateSoundFilePlayer()
 |||
**/
int32 DeleteSoundFilePlayer ( SoundFilePlayer *sfp )
{
	int32 i, Result = 0;

	if (sfp)
	{
		for (i=0; i<sfp->sfp_NumBuffers; i++)
		{
			if (sfp->sfp_PrivateFlags & SFP_PRIV_OWN_BUFFERS)
			{
				EZMemFree(sfp->sfp_BufferAddrs[i]);
			}
		}
		ssplDeleteSoundSpooler( sfp->sfp_Spooler );
		free (sfp);
	}

	return Result;
}

/**************************************************************************
** Load beginning of a soundfile into the player.
**************************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/loadsoundfile
 |||	LoadSoundFile - Preloads the first part of a sound file.
 |||
 |||	  Synopsis
 |||
 |||	    Err LoadSoundFile( SoundFilePlayer *sfp, char *FileName )
 |||
 |||	  Description
 |||
 |||	    This procedure preloads the first part of a sound file to be played.  The
 |||	    file should be an AIFF file.  The appropriate DSP instrument to play this
 |||	    sound file is loaded for you at the same time.
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer.
 |||
 |||	    FileName                     A pointer to the sound file.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Caveats
 |||
 |||	    Prior to V24 this function failed to check the file's block size. It always
 |||	    assumed that it was 2048 bytes. As of V24 it does check. However, there has
 |||	    always been an implicit requirement that buffer size be a multiple of block
 |||	    size. Not having buffers be a multiple of block size could
 |||	    result in memory overruns. The sound file player now checks buffer size against block size and returns
 |||	    an error if the buffer size is not a multiple of block size.
 |||
 |||	    This means that an app using 10K buffers will work fine on a 2K block file and
 |||	    will simply fail on a 4K block file system. To prevent this choose a buffer size
 |||	    that is a multiple of all of the block sizes that your application is expected to support.
 |||
 |||	    If this restriction is too severe, use the advanced sound player instead.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), CloseSoundFile(), DeleteSoundFile(), ReadSoundFile(),
 |||	    RewindSoundFile(), ServiceSoundFile(), StartSoundFile(), StopSoundFile(),
 |||	    UnloadSoundFile(), CreateSoundFilePlayer()
 |||
**/
int32 LoadSoundFile (  SoundFilePlayer *sfp, char *FileName )
{
	int32 Result;
	Item OpenFileItem, ioReqItem;
	char *InstrumentName;
	int32 Channels;
	int32 DataOffset;
	int32 DataSize;
	TagArg Tags[4];
	Item TempSample;

#ifdef DEBUG
	int32 i;
/* Clear debug arrays. */
	for (i=0; i<MAXDATA; i++)
	{
		logCurAddrA[i] = 0;
		logCurAddrB[i] = 0;
		logSignal[i] = 0;
		logCurSignal[i] = 0;
		logBufIndex[i] = 0;
		logCursor[i] = 0;
		logNumRead[i] = 0;
	}
	dbgindx = 0;
#endif

/* Scan sample buffer to get sample info */
	TempSample = ScanSample(FileName, 0 );  /* from command line? */
	CHECKRESULT(TempSample,"ScanSample");

/* Get information from sample. */
	Tags[0].ta_Tag = AF_TAG_CHANNELS;
	Tags[1].ta_Tag = AF_TAG_DATA_OFFSET;
	Tags[2].ta_Tag = AF_TAG_DATA_SIZE;
	Tags[3].ta_Tag = TAG_END;
	Result = GetAudioItemInfo( TempSample, Tags );
	if( Result < 0) return Result;

	Channels = (int32) Tags[0].ta_Arg;
	DataOffset = (uint32) Tags[1].ta_Arg;
	DataSize = (int32) Tags[2].ta_Arg;


	sfp->sfp_NumToRead = DataSize + DataOffset;
	sfp->sfp_BuffersToPlay = (sfp->sfp_NumToRead + sfp->sfp_BufSize -1 )/ sfp->sfp_BufSize;
	sfp->sfp_DataOffset = DataOffset;

DBUG(("sfp->sfp_BuffersToPlay = %d\n", sfp->sfp_BuffersToPlay));

/* Now reopen file as a block oriented sound file. */
	OpenFileItem = OpenDiskFile(FileName);
	if (OpenFileItem < 0)
	{
		Result = OpenFileItem;
		goto error;
	}
	sfp->sfp_FileItem = OpenFileItem;

/* Create an IO Request for reading file. */
	Tags[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	Tags[0].ta_Arg = (void *) OpenFileItem;
	Tags[1].ta_Tag = TAG_END;
	ioReqItem = CreateItem(MKNODEID(KERNELNODE,IOREQNODE), Tags);
	if (ioReqItem < 0)
	{
		Result = ioReqItem;
		goto error;
	}
	sfp->sfp_IOReqItem = ioReqItem;

/* Get the file's block size (941013) */
    {
        int32 tresult;

        if ( (tresult = GetFileBlockSize (sfp->sfp_IOReqItem)) < 0 ) {
            Result = tresult;
            goto error;
        }
        sfp->sfp_BlockSize = (uint32)tresult;

      #if DEBUG_LoadSoundFile
        PRT (("soundfile: Block size: %lu bytes.\n", sfp->sfp_BlockSize));
      #endif
    }

/* Make sure buffer size is a multiple of block size */
    if (sfp->sfp_BufSize % sfp->sfp_BlockSize) {
        ERR (("soundfile: buffer size (%lu) not a multiple of file's block size (%lu)\n", sfp->sfp_BufSize, sfp->sfp_BlockSize));
        Result = ML_ERR_BUFFER_TOO_SMALL;
        goto error;
    }

/* Load an instrument if the user has not supplied one. */
	if((sfp->sfp_Flags & SFP_NO_SAMPLER) == 0)
	{
		InstrumentName = SelectSamplePlayer( TempSample , FALSE );
		if (InstrumentName == NULL)
		{
			ERR(("No instrument to play that sample.\n"));
			Result = ML_ERR_UNSUPPORTED_SAMPLE;
			goto error; /* %Q cleanup */
		}
		else
		{
          #if DEBUG_LoadSoundFile
			PRT(("soundfile: Use '%s' instrument.\n", InstrumentName));
          #endif
		}
		sfp->sfp_SamplerIns = LoadInstrument( InstrumentName, 0, 100 );
		CHECKRESULT(sfp->sfp_SamplerIns,"LoadInstrument");
	}

/* Attach buffers to sampler. */
	if( sfp->sfp_SamplerIns )
	{
		Result = ssplAttachInstrument( sfp->sfp_Spooler, sfp->sfp_SamplerIns );
		CHECKRESULT(Result,"ssplAttachInstrument");
	}

	if((sfp->sfp_Flags & SFP_NO_DIRECTOUT) == 0)
	{
		sfp->sfp_OutputIns = LoadInstrument("directout.dsp",  0, 100); /* 941114 */
DBUG(("Open: OutputIns = 0x%x\n", sfp->sfp_OutputIns));
		CHECKRESULT(sfp->sfp_OutputIns,"LoadInstrument");

		if (Channels == 1)
		{
/* Connect Sampler to Direct Out */
			Result = ConnectInstruments (sfp->sfp_SamplerIns, "Output", sfp->sfp_OutputIns, "InputLeft");
			CHECKRESULT(Result,"ConnectInstruments");
			Result = ConnectInstruments (sfp->sfp_SamplerIns, "Output", sfp->sfp_OutputIns, "InputRight");
			CHECKRESULT(Result,"ConnectInstruments");
		}
		else if (Channels == 2)
		{

/* Connect Sampler to Direct Out */
			Result = ConnectInstruments (sfp->sfp_SamplerIns, "LeftOutput", sfp->sfp_OutputIns, "InputLeft");
			CHECKRESULT(Result,"ConnectInstruments");
			Result = ConnectInstruments (sfp->sfp_SamplerIns, "RightOutput", sfp->sfp_OutputIns, "InputRight");
			CHECKRESULT(Result,"ConnectInstruments");
		}
	}


	UnloadSample( TempSample );

/* Rewind after loading instruments so that drive head is left in soundfile track. 931110 */
	Result = RewindSoundFile(sfp);
	CHECKRESULT(Result,"RewindSoundFile");

	return Result;

#ifdef DEBUG
	dbgindx = 0;
#endif

error:
	return Result;
}

/*****************************************************************/

static int32 ReadSoundFileAddr( SoundFilePlayer *sfp, int32 Cursor, uint8 *Address, int32 IfAsync)
{
/* Read sound file data into buffer. */
	int32 LeftToRead, NumToRead, NumToClear = 0, Result = 0;
	int32 err, i;
	IOInfo theInfo;
	uint8 *bp;

DBUG(("ReadSoundFileAddr( 0x%x, 0x%lx, 0x%lx )\n", sfp, Cursor, Address));
	LeftToRead = sfp->sfp_NumToRead - Cursor;
	if (LeftToRead <= 0)
	{
ERR(("ReadSoundFile nothing left to read.\n"));
		return ML_ERR_END_OF_FILE;
	}

	if (sfp->sfp_IOReqItem > 0)
	{
            /* @@@ NumToRead is set in both cases assuming that the buffer size is a multiple of block size */
		if (LeftToRead < sfp->sfp_BufSize)
		{
/* Adjust NumToRead to get an integral number of BLOCKs 930614 */
		 /* NumToRead = (LeftToRead + (BLOCKSIZE-1)) & (~(BLOCKSIZE-1));   941013: replaced w/ block size from file */
			NumToRead = (LeftToRead + (sfp->sfp_BlockSize-1)) / sfp->sfp_BlockSize * sfp->sfp_BlockSize;
DBUG(("NumToRead = 0x%x, LeftToRead = 0x%x\n", NumToRead, LeftToRead));
			NumToClear = sfp->sfp_BufSize - LeftToRead;
		}
		else
		{
			NumToRead = sfp->sfp_BufSize;
			NumToClear = 0;
		}

    	theInfo.ioi_Command = CMD_READ;
    	theInfo.ioi_Flags = 0;
 #ifndef dragon2
   		theInfo.ioi_Flags2 = 0;
 #endif
    	theInfo.ioi_Unit = 0;
    	theInfo.ioi_Offset = (int32) (Cursor / sfp->sfp_BlockSize); /* 941013: replaced w/ block size from file */
    	theInfo.ioi_Send.iob_Buffer = NULL;
    	theInfo.ioi_Send.iob_Len = 0;
    	theInfo.ioi_Recv.iob_Buffer = Address;
    	theInfo.ioi_Recv.iob_Len = NumToRead;
#ifdef DEBUG
	LOGDATA(NumToRead,logNumRead);
#endif
/* Wait for old IO to complete.  940224 */

		DBUG(("ReadSoundFileAS: IfAsync = %d\n", IfAsync));
		FINISHIO;
		if( IfAsync )
		{
			DBUG(("ReadSoundFileAS: SendIO\n"));
    		err = SendIO(sfp->sfp_IOReqItem , &theInfo);
			if( err >= 0) sfp->sfp_PrivateFlags |= SFP_PRIV_IO_PENDING;

		}
		else
		{
    		err = DoIO(sfp->sfp_IOReqItem , &theInfo);
		}

    	if (err < 0)
    	{
    		ERR(("ReadSoundFIle: DoIO error = 0x%x\n", err));
    		Result = err;
    	}
/* Clear any data left at end. */
		if (NumToClear)
		{
/* 940224 Must finish IO because it is rounded up to a block and has crud at end. */
			FINISHIO;
			bp = Address + LeftToRead;
			for (i=0; i<NumToClear; i++) *bp++ = 0;
DBUG(("Clearing %d bytes in buf 0x%x\n", NumToClear, Address ));
		}
	}
	else
	{
		Result = ML_ERR_NOT_OPEN;
	}

	return Result;
}


/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/readsoundfile
 |||	ReadSoundFile - Reads a sound file.
 |||
 |||	  !!! hide this function?
 |||
 |||	  Synopsis
 |||
 |||	    int32 ReadSoundFile( SoundFilePlayer *sfp, int32 Cursor,
 |||	    int32 BufIndex )
 |||
 |||	  Description
 |||
 |||	    This procedure is used internally to read part of the sound file into a
 |||	    buffer.
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer.
 |||
 |||	    Cursor                       A value indicating where in the sound file
 |||	                                 the procedure is reading from.
 |||
 |||	    BufIndex                     A value indicating which of the audio data
 |||	                                 buffers the procedure is reading from.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ServiceSoundFile()
 |||
**/
int32 ReadSoundFile( SoundFilePlayer *sfp, int32 Cursor, int32 BufIndex)
{
DBUG(("ReadSoundFile( ...,..., %d )\n", BufIndex ));
	return ReadSoundFileAddr( sfp, Cursor, sfp->sfp_BufferAddrs[BufIndex], SNDF_IF_ASYNC );
}

/**************************************************************************/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/opensoundfile
 |||	OpenSoundFile - Creates a SoundFilePlayer data structure and loads a sound
 |||	                file.
 |||
 |||	  Synopsis
 |||
 |||	    SoundFilePlayer *OpenSoundFile( char *FileName,
 |||	    int32 NumBuffers, int32 BufSize )
 |||
 |||	  Description
 |||
 |||	    This is a convenience procedure that creates a SoundFilePlayer data
 |||	    structure and loads the sound file.  It calls CreateSoundFilePlayer() and
 |||	    LoadSoundFile() for you.
 |||
 |||	  Arguments
 |||
 |||	    FileName                     A pointer to the name of the sound file.
 |||
 |||	    NumBuffers                   A value indicating the number of audio data
 |||	                                 buffers in the SoundFilePlayer.
 |||
 |||	    BufSize                      A value indicating the size of the audio
 |||	                                 data buffers.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns a pointer to the SoundFilePlayer data structure if
 |||	    successful or NULL if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in music.lib V20.
 |||
 |||	  Caveats
 |||
 |||	    Buffer size must be a multiple of the file's block size (see LoadSoundFile() for complete reason).
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CloseSoundFile(), DeleteSoundFile(), LoadSoundFile(), ReadSoundFile(),
 |||	    RewindSoundFile(), ServiceSoundFile(), StartSoundFile(), StopSoundFile(),
 |||	    UnloadSoundFile(), CreateSoundFilePlayer()
 |||
**/
SoundFilePlayer *OpenSoundFile (char *FileName, int32 NumBuffers, int32 BufSize )
{

	SoundFilePlayer *sfp;
	int32 Result;

	sfp = CreateSoundFilePlayer ( NumBuffers, BufSize, NULL );
	if (sfp)
	{
		Result = LoadSoundFile( sfp, FileName );
		if (Result < 0) return NULL;
	}
	return sfp;
}

/***************************************************************************
** Loop back to beginning of file.
** Preload buffers so that spooler is ready to start immediately.
***************************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/rewindsoundfile
 |||	RewindSoundFile - Rewinds a sound file player.
 |||
 |||	  Synopsis
 |||
 |||	    int32 RewindSoundFile( SoundFilePlayer *sfp )
 |||
 |||	  Description
 |||
 |||	    This procedure rewinds a SoundFilePlayer data structure so that sound file
 |||	    playback will start from the beginning when the sound file is activated
 |||	    with StartSoundFile().
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer data
 |||	                                 structure.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), CloseSoundFile(), DeleteSoundFile(), LoadSoundFile(),
 |||	    ReadSoundFile(), ServiceSoundFile(), StartSoundFile(), StopSoundFile(),
 |||	    UnloadSoundFile(), CreateSoundFilePlayer()
 |||
**/
int32 RewindSoundFile ( SoundFilePlayer *sfp)
{
	int32 Result, i;
	char *bp;
	int32 LastPos;
	SoundSpooler *sspl;
	SoundBufferNode *sbn;

	sfp->sfp_Cursor = 0;
	sfp->sfp_LastPos = 0;
	sfp->sfp_BufIndex = 0;
	sfp->sfp_BuffersPlayed = 0;

	sspl = sfp->sfp_Spooler;

/* 940419 Reset spooler so that buffers are free. */
	Result = ssplAbort( sspl, NULL );
	if (Result < 0) return Result;

/* Preload the initial buffers. */
	for (i=0; i<sfp->sfp_NumBuffers; i++)
	{
		if (sfp->sfp_Cursor < sfp->sfp_NumToRead)
		{
			Result = ReadSoundFile( sfp, sfp->sfp_Cursor, sfp->sfp_BufIndex);
			if (Result < 0) return Result;

			sfp->sfp_Cursor += sfp->sfp_BufSize;
			sfp->sfp_BufIndex += 1;

/*  Send buffers during Rewind so that Start/Stop works Properly. */
			if( (sbn = ssplRequestBuffer( sspl )) != NULL)
			{

/* Set address and length of buffer. */
				Result = ssplSetBufferAddressLength( sspl, sbn,
					sfp->sfp_BufferAddrs[i], sfp->sfp_BufSize );
				if( Result < 0 ) return Result;

/* Send that buffer off to be played eventually. */
				Result = ssplSendBuffer( sspl, sbn );
				if( Result < 0 ) return Result;
			}
			else
			{
				ERR(("RewindSoundFile: Could not send all buffers!\n"));
			}
		}
		else
		{
			break;
		}
	}

	sfp->sfp_BufIndex = 0;

/* Zero out part of file that contains header info which should be small. */
	bp = sfp->sfp_BufferAddrs[0];
/* 931110 Protect against writing past end of buffer if DataOffset is past BufferEnd */
	LastPos = (sfp->sfp_DataOffset < sfp->sfp_BufSize) ?
				sfp->sfp_DataOffset :  sfp->sfp_BufSize ;
	for (i=0; i<LastPos; i++)
	{
		*bp++ = 0;
	}

	return 0;
}

/***************************************************************************
** Start playback. Returns initial SignalsNeeded if successful. 940421
***************************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/startsoundfile
 |||	StartSoundFile - Begins playing a sound file.
 |||
 |||	  Synopsis
 |||
 |||	    int32 StartSoundFile( SoundFilePlayer *sfp, int32
 |||	    Amplitude )
 |||
 |||	  Description
 |||
 |||	    This procedure begins playing a sound file.
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer data
 |||	                                 structure controlling playback.
 |||
 |||	    Amplitude                    A value indicating the amplitude at which to
 |||	                                 play the sound file; the value is from 0 to
 |||	                                 $7FFF.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), CloseSoundFile(), DeleteSoundFile(), LoadSoundFile(),
 |||	    ReadSoundFile(), RewindSoundFile(), ServiceSoundFile(), StopSoundFile(),
 |||	    UnloadSoundFile(), CreateSoundFilePlayer()
 |||
**/
int32 StartSoundFile ( SoundFilePlayer *sfp , int32 Amplitude)
{
	int32 Result;

/* DirectOut must be started */
	if(sfp->sfp_OutputIns)
	{
		Result = StartInstrument( sfp->sfp_OutputIns, NULL );
		if( Result < 0 ) return Result;
	}

	Result = ssplStartSpooler( sfp->sfp_Spooler, Amplitude );
	if( Result < 0 ) return Result;

	return (sfp->sfp_Spooler->sspl_SignalMask);
}

/**************************************************************************
** ServiceSoundFile - read a buffer of sound if the DMA is ready.
** Returns the number of bytes left to be heard.
**************************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/servicesoundfile
 |||	ServiceSoundFile - Spools a sound file from disk to a sound buffer.
 |||
 |||	  Synopsis
 |||
 |||	    int32 ServiceSoundFile( SoundFilePlayer *sfp, int32
 |||	    SignalIn, int32 *SignalNeeded )
 |||
 |||	  Description
 |||
 |||	    This procedure is called repeatedly to spool the sound file from disk to a
 |||	    sound buffer played by StartSoundFile() and controlled by the specified
 |||	    SoundFilePlayer data structure.  It returns, in the SignalNeeded mask, the
 |||	    signal for the next sound buffer to finish playing.   The task should use
 |||	    that signal in WaitSignal() to enter wait state where it can wait for the
 |||	    buffer to finish playing.  When it exits wait state, it passes the signal
 |||	    mask returned to ServiceSoundFile() as the next buffer to write to.
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer data
 |||	                                 structure.
 |||
 |||	    SignalIn                     A signal mask containing bits set
 |||	                                 corresponding to the cue bits of sound
 |||	                                 buffers to be filled.
 |||
 |||	    SignalNeeded                 A pointer to a signal mask where the
 |||	                                 procedure writes the cue bit of the sound
 |||	                                 buffers for which the task should wait.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), CloseSoundFile(), DeleteSoundFile(), LoadSoundFile(),
 |||	    ReadSoundFile(), RewindSoundFile(), StartSoundFile(), StopSoundFile(),
 |||	    UnloadSoundFile(), CreateSoundFilePlayer()
 |||
**/
int32 ServiceSoundFile ( SoundFilePlayer *sfp, int32 SignalIn, int32 *SignalNeeded)
{
	int32 Result = 0;
	SoundSpooler *sspl;
	SoundBufferNode *sbn;
	int32 NumBuffers;

DBUG(("ServiceSoundFile: Played = %d, BI=%d\n", sfp->sfp_BuffersPlayed, sfp->sfp_BufIndex));
	sspl = sfp->sfp_Spooler;

	if ( SignalIn )
	{

/* Tell sound spooler that some buffers have completed. */
		NumBuffers = ssplProcessSignals( sspl, SignalIn, NULL );
		CHECKRESULT(NumBuffers,"ssplProcessSignals");
		sfp->sfp_BuffersPlayed += NumBuffers;

#ifdef DEBUG
	{
		char *CurAddr;
		CurAddr = DSPWhereDMA(0);
		LOGDATA((int32)CurAddr,logCurAddrA);
		LOGDATA(sfp->sfp_Cursor,logCursor);
		LOGDATA(SignalIn,logSignal);
		LOGDATA(GetCurrentSignals(),logCurSignal);
		LOGDATA(sfp->sfp_BufIndex,logBufIndex);
		LOGDATA(-1,logUnlink);
	}
#endif
/* If there is anything left to read, go get it. */
		do
		{
			sbn = NULL;

			if (sfp->sfp_Cursor < sfp->sfp_NumToRead)
			{

/* Fill up the buffers */
				if( (sbn = ssplRequestBuffer( sspl )) != NULL)
				{

DBUG(("ServiceSoundFile: sbn=0x%08x, Addr = 0x%08x\n", sbn, sbn->sbn_Address ));
					Result = ReadSoundFileAddr( sfp, sfp->sfp_Cursor, sbn->sbn_Address, SNDF_IF_ASYNC);
					if (Result < 0) return Result;
					sfp->sfp_Cursor += sfp->sfp_BufSize;

					Result = ssplSendBuffer( sspl, sbn );

					sfp->sfp_LastPos += sfp->sfp_BufSize;
					if (sfp->sfp_LastPos > sfp->sfp_NumToRead)
					{
						sfp->sfp_LastPos = sfp->sfp_NumToRead;
					}

/* Index to next buffer, wrap at end to zero. */
					sfp->sfp_BufIndex = (sfp->sfp_BufIndex < (sfp->sfp_NumBuffers-1))
						 ? sfp->sfp_BufIndex+1 : 0;
				}
			}
		} while( sbn != NULL );

#ifdef DEBUG
		{
			char *CurAddr;
			CurAddr = DSPWhereDMA(0);
			LOGDATA((int32)CurAddr,logCurAddrB);
		}
#endif

	}

/* Determine which signal to wait for next. */
	if (sfp->sfp_BuffersPlayed < sfp->sfp_BuffersToPlay)
	{
		*SignalNeeded = sfp->sfp_Spooler->sspl_SignalMask;
	}
	else
	{
		*SignalNeeded = 0;   /* All Done. */
	}

	Result = sfp->sfp_NumToRead - sfp->sfp_LastPos;

#ifdef DEBUG
	dbgindx++;
#endif
error:
	return Result;
}

/**************************************************************************
** StopSoundFile - Stop the sample playing instrument.
**************************************************************************/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/stopsoundfile
 |||	StopSoundFile - Stops playing a sound file.
 |||
 |||	  Synopsis
 |||
 |||	    int32 StopSoundFile( SoundFilePlayer *sfp )
 |||
 |||	  Description
 |||
 |||	    This procedure stops playing a sound file.
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer data
 |||	                                 structure.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), CloseSoundFile(), DeleteSoundFile(), LoadSoundFile(),
 |||	    ReadSoundFile(), RewindSoundFile(), ServiceSoundFile(), StartSoundFile(),
 |||	    UnloadSoundFile(), CreateSoundFilePlayer()
 |||
**/
int32 StopSoundFile ( SoundFilePlayer *sfp )
{
	int32 Result;


	Result = ssplStopSpooler ( sfp->sfp_Spooler );
	if (Result < 0) return Result;

	if(sfp->sfp_OutputIns)
	{
		Result = StopInstrument ( sfp->sfp_OutputIns , NULL );
	}

	return Result;
}

/**************************************************************************
** UnloadSoundFile - Unload items related to this file.
**************************************************************************/

#ifdef DEBUG
static char DebugHeader[] =
	"  i     Cursor BufIndex Signal   CurSigal  NumRead   CurAddrA  CurAddrB  UL\n";
#endif

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/unloadsoundfile
 |||	UnloadSoundFile - Unloads a sound file.
 |||
 |||	  Synopsis
 |||
 |||	    int32 UnloadSoundFile( SoundFilePlayer *sfp )
 |||
 |||	  Description
 |||
 |||	    This procedure unloads a sound file that has been played.  This process
 |||	    closes the AIFF file, deletes the IOReq item used to read the file,
 |||	    detaches the buffers from the sampled-sound instrument, and frees the
 |||	    sampled-sound instrument.  The task must delete the SoundFilePlayer data
 |||	    structure and the sound buffers on its own.
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer data
 |||	                                 structure.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), CloseSoundFile(), DeleteSoundFile(), LoadSoundFile(),
 |||	    ReadSoundFile(), RewindSoundFile(), ServiceSoundFile(), StartSoundFile(),
 |||	    StopSoundFile(), CreateSoundFilePlayer()
 |||
**/
int32 UnloadSoundFile ( SoundFilePlayer *sfp )
{

#ifdef DEBUG
	int32 b,i;
	PRT((DebugHeader));
	b = dbgindx-100;
	if (b<0) b=0;
	for (i=b; i<dbgindx; i++)
	{
		PRT(("%4d  ", i));
		PRT(("%8x   %4x  %8x %8x", logCursor[i], logBufIndex[i], logSignal[i], logCurSignal[i]));
		PRT(("%8x   %8x  %8x %2d\n", logNumRead[i], logCurAddrA[i], logCurAddrB[i], logUnlink[i]));
	}
	PRT((DebugHeader));
	PRT(("NumToRead = %x\n", sfp->sfp_NumToRead));
#endif

/* Delete IOReq before deleting device to prevent warnings. */
	if (sfp->sfp_IOReqItem)
	{
		DeleteItem( sfp->sfp_IOReqItem );
		sfp->sfp_IOReqItem = 0;
		sfp->sfp_PrivateFlags &= ~SFP_PRIV_IO_PENDING;  /* 940224 */
	}

	if (sfp->sfp_FileItem)
	{
		CloseDiskFile ( sfp->sfp_FileItem );
		sfp->sfp_FileItem = 0;
	}

	if((sfp->sfp_SamplerIns) && ((sfp->sfp_Flags & SFP_NO_SAMPLER) == 0))
	{
		ssplDetachInstrument( sfp->sfp_Spooler );
/* 940415 Was unloading sfp->sfp_Spooler->sspl_SamplerIns */
		UnloadInstrument( sfp->sfp_SamplerIns );
		sfp->sfp_SamplerIns = 0;
	}
	if(sfp->sfp_OutputIns)
	{
		UnloadInstrument( sfp->sfp_OutputIns );
		sfp->sfp_OutputIns = 0;
	}

	return( 0);
}

/**************************************************************************
** CloseSoundFile - Free everything and UnloadSample.
**************************************************************************/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/soundfileplayer/closesoundfile
 |||	CloseSoundFile - Closes a SoundFilePlayer data structure and unloads a
 |||	                 sound file.
 |||
 |||	  Synopsis
 |||
 |||	    int32 CloseSoundFile( SoundFilePlayer *sfp )
 |||
 |||	  Description
 |||
 |||	    This procedure is a convenience procedure that closes the specified
 |||	    SoundFilePlayer data structure and also cleans up any items associated
 |||	    with the player.  This procedure is the inverse of OpenSoundFile(), and is
 |||	    in essence acombination of UnloadSoundFile() and DeleteSoundFilePlayer().
 |||
 |||	  Arguments
 |||
 |||	    sfp                          A pointer to the SoundFilePlayer.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    soundfile.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    OpenSoundFile(), DeleteSoundFile(), LoadSoundFile(), ReadSoundFile(),
 |||	    RewindSoundFile(), ServiceSoundFile(), StartSoundFile(), StopSoundFile(),
 |||	    UnloadSoundFile(), CreateSoundFilePlayer()
 |||
**/
int32 CloseSoundFile ( SoundFilePlayer *sfp )
{
	UnloadSoundFile( sfp );
	return DeleteSoundFilePlayer( sfp );
}


/* -------------------- misc support functions */

/*
    This function returns an error instead of returning a block size of 0.
    (941013)
*/
static int32 GetFileBlockSize (Item fileioreq)
{
    IOInfo ioinfo;
    FileStatus stat;
    Err errcode;

    memset (&ioinfo, 0, sizeof ioinfo);
    ioinfo.ioi_Command         = CMD_STATUS;
    ioinfo.ioi_Recv.iob_Buffer = &stat;
    ioinfo.ioi_Recv.iob_Len    = sizeof stat;

        /* @@@ this relies on V24 DoIO() */
    return (errcode = DoIO (fileioreq, &ioinfo)) < 0 ? errcode
         : stat.fs.ds_DeviceBlockSize ? stat.fs.ds_DeviceBlockSize
         : ML_ERR_BADITEM;      /* @@@ could have a more suitable error code, but this should be fine */
}

