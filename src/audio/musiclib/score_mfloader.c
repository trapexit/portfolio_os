/* $Id: score_mfloader.c,v 1.9 1994/09/09 23:42:11 peabody Exp $ */
/****************************************************************
**
** MIDI File loader for score player.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 940412 WJB Split off from score.c
** 940727 WJB Added autodocs.
** 940812 WJB Tweaked includes.
****************************************************************/

#include <audio.h>
#include <handy_tools.h>        /* EZMemAlloc() */
#include <mem.h>                /* MEMTYPE_ */
#include <midifile.h>
#include <score.h>
#include <stdio.h>              /* printf() */


/* -------------------- Macros */

#define	PRT(x)	       printf x
#define	ERR(x)         PRT(x)               /* !!! only define in DEVELOPMENT mode? */
#define	DBUG(x)        /* PRT(x) */


/* -------------------- Local functions */

static Err MFTrackToSequence ( MIDIFileParser *mfpptr, Sequence *SeqPtr);
static int32 MFReadCollection ( MIDIFileParser *mfpptr, Collection *ColPtr);
static int32 PrepareScoreSequence( Sequence *SeqPtr, char *EventDataPtr, int32 MaxEvents );
static int32 LoadScoreEvent( MIDIFileParser *mfpptr, unsigned char *Data, int32 NumBytes);


/* -------------------- MIDI File loader */

/******************************************************************
** Load a Data from a Track into Sequence
** Allocate enough memory for max events then copy it into smaller memory.
******************************************************************/
static Err MFTrackToSequence ( MIDIFileParser *mfpptr, Sequence *SeqPtr)
{
	int32 DataSize;
	char *TempEvents, *Events;
	int32 MaxEvents, NumBytes;
	int32 Result;

	TempEvents = NULL;
	Events = NULL;

	mfpptr->mfp_UserData = (void *) SeqPtr;

/* Verify header and get number of bytes. */
	NumBytes = ReadMFTrackHeader( mfpptr );
	if( NumBytes < 0 )
	{
		Result = NumBytes;
		goto error;
	}

/* Allocate enough for all notes. Assume minimum of 1 time byte, 2 data in file. */
	MaxEvents = (NumBytes/3) +1;
	DataSize = MaxEvents * sizeof(MIDIEvent);

/* To avoid fragmantation, allocate space for smaller event memory now. */
	TempEvents = (char *)EZMemAlloc( DataSize, MEMTYPE_FILL );
	if( TempEvents == NULL )
	{
		ERR(("MFLoadCollection: Could not allocate track memory.\n"));
		Result = AF_ERR_NOMEM;
		goto error;
	}
	Events = (char *)EZMemAlloc( DataSize, MEMTYPE_FILL );
	if( Events == NULL )
	{
/* Just use TempEvents. At least it runs! */
		Events = TempEvents;
		TempEvents = NULL;
	}

	PrepareScoreSequence( SeqPtr, Events, MaxEvents );

/* Set function pointer. */
	mfpptr->mfp_HandleEvent = LoadScoreEvent;
	if ((Result = ScanMFTrack( mfpptr, NumBytes )) < 0)
	{
		ERR(("MFLoadCollection: Error parsing track = %d\n", Result));
		goto error;
	}

DBUG(("%d events in sequence, room for %d.\n", SeqPtr->jglr_Many,  SeqPtr->seq_Max ));
	if (SeqPtr->jglr_Many > 0)
	{
/* Reallocate sequence data now that we know how many there are. */
		if( TempEvents )
		{
/* By freeing and reallocating, we avoid fragmentation. */
			EZMemFree( TempEvents );
			DataSize = SeqPtr->jglr_Many * SeqPtr->seq_EventSize;
			TempEvents = (char *)EZMemAlloc( DataSize, MEMTYPE_ANY );
			if( TempEvents )
			{
				memcpy( TempEvents, Events, DataSize );
				SeqPtr->seq_Max = SeqPtr->jglr_Many;
				SeqPtr->seq_Events = TempEvents;
				EZMemFree( Events );
			}
		}
	}
	else
	{
		DBUG(("Freeing TempEvents = 0x%x\n", TempEvents));
		if( TempEvents ) EZMemFree( TempEvents );  /* 931208 Fix leak. */
	}

	return Result;

error:
	if( TempEvents ) EZMemFree( TempEvents );
	if( Events ) EZMemFree( Events );
	return Result;
}


/******************************************************************
** Load a Sequence for a MIDI File
** Assume the file is a Format0
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/mfloadsequence
 |||	MFLoadSequence - Loads a sequence from a MIDI file.
 |||
 |||	  Synopsis
 |||
 |||	    Err MFLoadSequence( MIDIFileParser *mfpptr, char
 |||	    *filename, Sequence *SeqPtr )
 |||
 |||	  Description
 |||
 |||	    This procedure loads a single sequence from a format 0 MIDI file and turns
 |||	    it into a juggler sequence for playback using the juggler.  To use this
 |||	    procedure, you must first create the juggler sequence using CreateObject().
 |||	    The procedure translates the MIDI messages in the sequence into MIDI
 |||	    events in the juggler sequence.  To work, the procedure must have a
 |||	    MIDIFileParser data structure to keep track of the MIDI sequence's
 |||	    original settings.
 |||
 |||	    Note that this procedure will accept a format 1 MIDI file, in which case
 |||	    it converts only the first track (sequence) of the file.  That track
 |||	    typically contains no notes.
 |||
 |||	  Arguments
 |||
 |||	    mfpptr                       Pointer to a MIDIFileParser data structure.
 |||
 |||	    filename                     Pointer to a character string containing the
 |||	                                 name of the format 0 MIDI file from which to
 |||	                                 load the sequence.
 |||
 |||	    SeqPtr                       Pointer to a juggler sequence in which to
 |||	                                 store the converted MIDI sequence.
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
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CreateObject(), MFLoadCollection(), MFUnloadCollection()
 |||
**/
int32 MFLoadSequence ( MIDIFileParser *mfpptr, char *filename, Sequence *SeqPtr)
{
	int32 Result;

	Result = OpenFlexStreamFile(&mfpptr->mfp_FlexStream, filename);
	if (Result < 0) return(Result);
	PRT(("%s opened\n", filename));

	if ((Result = ParseMFHeader(mfpptr)) != 0)
	{
		ERR(("Error parsing header: $%x\n", Result));
		goto error;
	}
	PRT(("LoadSequence: Format = %d, NumTracks = %d, Division = %d\n",
			mfpptr->mfp_Format, mfpptr->mfp_NumTracks, mfpptr->mfp_Division));


	if ((Result = MFTrackToSequence(mfpptr, SeqPtr)) < 0)
	{
		ERR(("Error parsing track = %d\n", Result));
		goto error;
	}

	CloseFlexStreamFile(&mfpptr->mfp_FlexStream);
	return(0);

error:
	ERR(("MFP Error %d\n", Result));
	CloseFlexStreamFile(&mfpptr->mfp_FlexStream);
	return(Result);
}

/******************************************************************
** Read a Collection from an Open FlexStream
** Assume the file is a Format1 or 0
******************************************************************/
static int32 MFReadCollection ( MIDIFileParser *mfpptr, Collection *ColPtr)
{
	int32 Result;
	Sequence *SeqPtr;
	int32 i;

	SeqPtr = NULL;

	if ((Result = ParseMFHeader(mfpptr)) != 0)
	{
		ERR(("Error parsing header: $%x\n", Result));
		goto error;
	}
	PRT(("MFReadCollection: Format = %d, NumTracks = %d, Division = %d\n",
			mfpptr->mfp_Format, mfpptr->mfp_NumTracks, mfpptr->mfp_Division));

/* Set function pointer. */
	mfpptr->mfp_HandleTrack = NULL;
	mfpptr->mfp_HandleEvent = LoadScoreEvent;

	for ( i=0; i < mfpptr->mfp_NumTracks; i++ )
	{
		SeqPtr = (Sequence *) CreateObject( &SequenceClass ); /* SC_OC%00001 */
		if (SeqPtr == NULL)
		{
			ERR(("MFReadCollection: Failure to create Sequence\n"));
			goto error;
		}

/* Inherit Collections context. */
		SeqPtr->jglr_UserContext = ColPtr->jglr_UserContext;

		if ((Result = MFTrackToSequence(mfpptr, SeqPtr)) < 0)
		{
			ERR(("MFReadCollection: Error parsing track = %d\n", Result));
			goto error;
		}

		if (SeqPtr->jglr_Many > 0)
		{
			Result = ColPtr->Class->Add(ColPtr, SeqPtr, 1); /* SC_LA%00003 */
			if (Result)
			{
				ERR(("MFReadCollection: Add returned 0x%x\n", Result));
				goto error;
			}
		}
		else
		{
/* No notes so track ignored. */
			FreeObject( SeqPtr );                  /* SC_OF%00002 */
			DestroyObject( (COBObject *) SeqPtr ); /* SC_OD%00001 */
		}

	}
	return(0);

error:
	ERR(("MFP Error %d\n", Result));
	if( SeqPtr ) DestroyObject( (COBObject *) SeqPtr );

	return(Result);
}

/******************************************************************
** Load a Collection for a MIDI File
** Assume the file is a Format1 or 0
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/mfloadcollection
 |||	MFLoadCollection - Loads a set of sequences from a MIDI file.
 |||
 |||	  Synopsis
 |||
 |||	    Err MFLoadCollection( MIDIFileParser *mfpptr, char
 |||	    *filename, Collection *ColPtr )
 |||
 |||	  Description
 |||
 |||	    This procedure loads a set of sequences from a format 1 MIDI file and
 |||	    turns them into a juggler collection for playback using the juggler.  To
 |||	    use this procedure, you must first create a juggler collection using
 |||	    CreateObject().  The procedure then creates an appropriate number of
 |||	    juggler sequences to contain the sequences found in the MIDI file.
 |||
 |||	    The procedure translates the MIDI messages in each file sequence into MIDI
 |||	    events in appropriate juggler sequences within the collection.  To work,
 |||	    the procedure must have a MIDIFileParser data structure to keep track of
 |||	    the MIDI sequence's original settings.
 |||
 |||	    Note that this procedure also accept format 0 MIDI files, treating them as
 |||	    a one-track format 1 MIDI file.
 |||
 |||	  Arguments
 |||
 |||	    mfpptr                       Pointer to a MIDIFileParser data structure.
 |||
 |||	    filename                     Pointer to a character string containing the
 |||	                                 name of the format 1 MIDI file from which to
 |||	                                 load the sequences.
 |||
 |||	    ColPtr                       Pointer to a juggler collection in which to
 |||	                                 store the converted MIDI sequences.
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
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CreateObject(), MFLoadSequence(), MFUnloadCollection()
 |||
**/
int32 MFLoadCollection ( MIDIFileParser *mfpptr, char *filename, Collection *ColPtr)
{
	int32 Result;

	Result = OpenFlexStreamFile(&mfpptr->mfp_FlexStream, filename);
	if (Result < 0) return(Result);
	PRT(("%s opened\n", filename));

	Result = MFReadCollection( mfpptr, ColPtr );

	CloseFlexStreamFile(&mfpptr->mfp_FlexStream);
	return(Result);
}


/******************************************************************
** Define and parse a Collection for a MIDI File
** Assume the file is a Format1 or 0
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/mfdefinecollection
 |||	MFDefineCollection - Creates a juggler collection from a MIDI file image
 |||	                     in RAM.
 |||
 |||	  Synopsis
 |||
 |||	    Err MFDefineCollection( MIDIFileParser *mfpptr, char
 |||	    *Image, int32 NumBytes, Collection *ColPtr)
 |||
 |||	  Description
 |||
 |||	    This procedure creates a juggler collection from a MIDI file image
 |||	    imported into RAM as part of a data streaming process.  The MIDI file in
 |||	    the image may be a format 0 or a format 1 MIDI file.  To use this
 |||	    procedure, you must first create a juggler collection using CreateObject()
 |||	    .  The procedure then creates an appropriate number of juggler sequences
 |||	    to contain the sequences found in the MIDI file image.
 |||
 |||	    The procedure translates the MIDI messages in each file sequence into MIDI
 |||	    events in an appropriate juggler sequences within the collection.  To
 |||	    work, the procedure must have a MIDIFileParser data structure to keep
 |||	    track of the MIDI sequence's original settings.
 |||
 |||	    Note that this procedure treats format 0 MIDI files as one-track format 1
 |||	    MIDI files.
 |||
 |||	  Arguments
 |||
 |||	    mfpptr                       Pointer to a MIDIFileParser data structure.
 |||
 |||	    Image                        Pointer to a MIDI file image in RAM (a
 |||	                                 string of bytes).
 |||
 |||	    NumBytes                     The size of the MIDI file image in bytes.
 |||
 |||	    ColPtr                       Pointer to a juggler collection in which to
 |||	                                 store the converted MIDI sequences.
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
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    CreateObject(), MFLoadCollection(), MFLoadSequence(), MFUnloadCollection()
 |||
**/
int32 MFDefineCollection ( MIDIFileParser *mfpptr, char *Image, int32 NumBytes, Collection *ColPtr)
{
	int32 Result;

	Result = OpenFlexStreamImage(&mfpptr->mfp_FlexStream, Image, NumBytes);
	if (Result < 0) return(Result);

	Result = MFReadCollection( mfpptr, ColPtr );

	CloseFlexStreamImage(&mfpptr->mfp_FlexStream);
	return(Result);
}

/******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/mfunloadcollection
 |||	MFUnloadCollection - Unloads a MIDI collection.
 |||
 |||	  Synopsis
 |||
 |||	    Err MFUnloadCollection( Collection *ColPtr )
 |||
 |||	  Description
 |||
 |||	    This procedure unloads a juggler collection created by importing a MIDI
 |||	    format 1 file.  Unloading destroys the collection and any sequences
 |||	    defined as part of it.
 |||
 |||	  Arguments
 |||
 |||	    ColPtr                       Pointer to a juggler collection data
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
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    MFLoadCollection(), MFLoadSequence()
 |||
**/
int32 MFUnloadCollection ( Collection *ColPtr )
{
	Sequence *SeqPtr;

	while( GetNthFromObject( ColPtr, 0, &SeqPtr ) == 0)
	{
		RemoveNthFromObject( ColPtr, 0 );      /* SC_LR%00003 */
		FreeObject( SeqPtr );                  /* SC_OF%00002 */
		DestroyObject( (COBObject *) SeqPtr ); /* SC_OD%00001 */
	}
	return 0;
}

/******************************************************************
** Callback function for MIDI File Parser
** Called whenever a Track is encountered.
******************************************************************/
static int32 PrepareScoreSequence( Sequence *SeqPtr, char *EventDataPtr, int32 MaxEvents )
{
	TagArg Tags[7];
	int32 i;

	i=0;
	Tags[i].ta_Tag = JGLR_TAG_INTERPRETER_FUNCTION;
	Tags[i++].ta_Arg = (void *) InterpretMIDIEvent;
	Tags[i].ta_Tag = JGLR_TAG_EVENTS;
	Tags[i++].ta_Arg = (void *) EventDataPtr;
	Tags[i].ta_Tag = JGLR_TAG_EVENT_SIZE;
	Tags[i++].ta_Arg = (void *) sizeof(MIDIEvent);
	Tags[i].ta_Tag = JGLR_TAG_MANY;
	Tags[i++].ta_Arg = (void *) 0;
	Tags[i].ta_Tag = JGLR_TAG_MAX;
	Tags[i++].ta_Arg = (void *) MaxEvents;
	Tags[i].ta_Tag =  TAG_END;
	return SetObjectInfo(SeqPtr, Tags);

}

/******************************************************************
** Callback function for MIDI File Parser
** Adds a single event to the sequence
******************************************************************/
static int32 LoadScoreEvent( MIDIFileParser *mfpptr, unsigned char *Data, int32 NumBytes)
{
	int32 i;
	Sequence *SeqPtr;
	MIDIEvent *MEvPtr;

#ifdef DEBUG
	PRT(("T=%8d, ", mfpptr->mfp_Time));
	PRT(("Data: "));
	for (i=0; i<NumBytes; i++) PRT(("0x%x ", Data[i]));
	PRT(("\n"));
#endif

/* Put data in sequence. */
	SeqPtr = (Sequence *) mfpptr->mfp_UserData;
	i = SeqPtr->jglr_Many++;
	MEvPtr = (MIDIEvent *) SeqPtr->seq_Events;
	MEvPtr[i].mev_Time = mfpptr->mfp_Time >> mfpptr->mfp_Shift;
	bcopy((void *) Data, (void *) &MEvPtr[i].mev_Command, (int) NumBytes);

	return 0;
}


/******************************************************************
** Interpreter for Juggler, callback function
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/interpretmidievent
 |||	InterpretMIDIEvent - Interprets MIDI events within a MIDI object.
 |||
 |||	  Synopsis
 |||
 |||	    Err InterpretMIDIEvent( Sequence *SeqPtr, MIDIEvent
 |||	    *MEvCur,ScoreContext *scon )
 |||
 |||	  Description
 |||
 |||	    This procedure is the interpreter procedure for MIDI sequence objects
 |||	    created using MFLoadSequence() and MFLoadCollection(). It's an
 |||	    internal procedure that is called by the juggler and, in turn, calls
 |||	    InterpretMIDIMessage().
 |||
 |||	  Arguments
 |||
 |||	    SeqPtr                       Pointer to a sequence data structure.
 |||
 |||	    MEvCur                       Pointer to the current MIDI event within the
 |||	                                 sequence.
 |||
 |||	    scon                         Pointer to a ScoreContext data structure
 |||	                                 used for playback.
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
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    InterpretMIDIMessage()
 |||
**/
int32 InterpretMIDIEvent( Sequence *SeqPtr, MIDIEvent *MEvCur, ScoreContext *scon)
{
	int32  Result;
	int32  IfMute;

DBUG(("InterpretMIDIEvent( 0x%x, 0x%x\n", SeqPtr, *(uint32 *) &MEvCur->mev_Command));

	IfMute = SeqPtr->jglr_Flags & JGLR_FLAG_MUTE;
	Result = InterpretMIDIMessage ( scon, (char *) &MEvCur->mev_Command, IfMute);
	if (Result < 0) return Result;
	return 0;
}
