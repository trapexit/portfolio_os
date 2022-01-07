/* $Id: score.c,v 1.79 1994/10/25 00:14:54 phil Exp $ */
/****************************************************************
**
** Score Playing Routines
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 930504 PLB Check error return in TermScoreContext.
** 930506 PLB Add ScoreContext parameter to InterpretMIDIEvent
** 930517 PLB Add Dynamic Voice Allocation
** 930526 PLB Fixed some list handling problems.
** 930527 PLB Improved Dynamic Voice Allocation
** 930603 PLB Set NumVoices=0 in FreeChannelInstruments
** 930809 PLB Blank lines now allowed in PIMap
** 930811 PLB Fixed bad Note tracker list handling for templates in multiple channels.
** 930830 PLB UpdateChannelVolumes correctly in InitScoreMixer
** 931110 PLB Moved SelectSamplePlayer() out to own file.
** 931111 PLB LoadPIMap now recognises .aifc and .aif as samples, now case insensitive
** 931111 PLB Missing EOL at end of PIMap file no longer causes a skipped line
** 931111 PLB Voice allocator now first tries to steal matching note
** 931208 PLB Fix memory leak in MFTrackToSequence
** 931210 PLB Fixed uninitialised Result in PurgeScoreInstruments. Thanks Eric.
** 940105 PLB Handle tabs in PIMap text files.
** 940404 PLB Fix PitchBend, centered at 0x2000, applies to new notes.
** 940405 PLB Revert PrintError changes to allow 1p2 compile.
** 940412 WJB Moved MIDI File loader stuff to score_mfloader.c.
** 940413 WJB Moved PIMap loader stuff to score_pimaploader.c.
** 940426 PLB Added comma to InputNames to fix mixer channels 8-11.
** 940606 WJB Changed voice allocation to permit multiple NoteTrackers per MIDI note.
** 940608 WJB Replaced start time comparison with CompareAudioTimes.
** 940727 WJB Added autodocs.
** 940811 WJB Added local conditional definition of CompareAudioTimes() to permit compiling under 1.3.
** 940812 WJB Changed voice stealing logic to permit stealing a
**            stopped voice from a higher priority instrument.
** 940812 WJB Cleaned up includes.
** 940815 WJB Changed voice stealing logic to steal higher priority voices
**            that are AF_ABANDONED instead of AF_STOPPED.
** 940819 WJB Added some missing autodoc records.
**            Privatized some functions that shouldn't have been public.
** 940819 WJB Updated autodocs.
** 940822 WJB Fixed problem in voice stealing where voices with MaxActivity+1
**            could get stolen.
** 940823 WJB Removed autodocs for Init|TermScoreContext and made them static.
** 940902 WJB Added StopScoreNote().
**            Updated autodocs.
** 940902 WJB Fixed release velocity for 0 velocity note on.
**            Made SamplerTags[] and auto local to NoteOnIns().
**            Tweaked debug code.
**            Tweaked autodocs.
** 940913 WJB Tweaked autodocs.
** 940921 PLB Added -r option to PIMap
**            Use new CreateInstrument() call.
** 941018 WJB Added TermScoreMixer() autodocs.
** 941024 PLB Changed AF_TAG_CALCRATE_SHIFT to AF_TAG_CALCRATE_DIVIDE
****************************************************************/


/****************************************************************
Description of Voice Allocation Scheme

Need fixed versus dynamic allocation option.

InitScoreDynamics
	Allocate linked list of note trackers

Note On
	Tries to adopt instrument from self.

	If at MaxVoices, then scavenges from self, must succeed.
		else tries to allocate new instrument.

	Then tries to adopt from other channels.
	Then tries to scavenges from self.
	Then tries to scavenge from other channels.
	Calls external PurgeHook to allow App to free resources.

Note Off
	Release - with FATLADYSINGS and AUTOABANDON

Program Change
	Stop and Free all notes.

Issues
	Note Trackers do not know when instrument abandoned.
	When do they free NoteTracker?
		When they get a new note, they scan and remove instrument from
		channel they stole from.
		Or should folio signal completion.
****************************************************************/

#include <audio.h>          /* CompareAudioTimes(), various folio calls */
#include <handy_tools.h>    /* EZMemAlloc/Free() */
#include <list.h>
#include <mem.h>
#include <musicerror.h>     /* ML_ERR_ */
#include <score.h>
#include <stdio.h>          /* printf() */
#include <types.h>

#include "music_internal.h"     /* package id */


/* #define DEBUG */

DEFINE_MUSICLIB_PACKAGE_ID(score)

#ifndef CompareAudioTimes   /* @@@ added to permit compiling under 1.3 */
  #define CompareAudioTimes(t1,t2)         ( (int32) ( (t1) - (t2) ) )
#endif

static int32 gMessagesDisabled = FALSE;

#define	PRT(x)	{ if (!gMessagesDisabled) printf x; }
#define	ERR(x)           PRT(x)
#define	DBUG(x)       /* PRT(x) */
#define	DBUGALLOC(x)  /* PRT(x) */
#define	DBUGLOAD(x)   /* PRT(x) */
#define	DBUGNOTE(x)   /* PRT(x) */

/* Real Time Trace.  Needed cuz PRT disturbs timing. */
/* #define REAL_TIME_TRACE */
#ifdef REAL_TIME_TRACE
static int32 TraceIndex = 0;
typedef struct TraceRecord
{
	int32 trec_Time;
	Item  trec_Inst;
	int8  trec_Note;
	int8  trec_Channel;
	int8  trec_Reserved1;
	int8  trec_Reserved2;
	char  *trec_AllocationType;
} TraceRecord;
/* AllocationTypes */
static char *AtAdoptSelf = "Adopt Self";
static char *AtAdoptOther = "Adopt Other";
static char *AtStealSelf = "Steal Self";
static char *AtStealOther = "Steal Other";
static char *AtAllocate = "Allocate";
static char *AtSameNote = "Same Note";
static char *AtPurgeHook = "PurgeHook";
static char *AtPurgeIns = "PurgeInstrument";

#define MAX_TRACES (2000)
TraceRecord TRecs[MAX_TRACES];
Err DumpTraceRecord( void )
{
	TraceRecord *trec;

	int32 i;

	for( i=0; i<TraceIndex; i++)
	{
		trec = &TRecs[i];
		PRT(("%4d: T=%8d, Ch=%2d, N=%3d, Ins=0x%8x, AT=%s\n",
			i, trec->trec_Time, trec->trec_Channel, trec->trec_Note,
			 trec->trec_Inst, trec->trec_AllocationType));
	}
	return 0;
}
#endif

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		if (!gMessagesDisabled) ERR(("Failure in %s, result = 0x%x\n",name,Result)); \
		goto cleanup; \
	}


#define MIDI_BEND_SHIFT (13)
#define MIDI_BEND_CENTER (1<<MIDI_BEND_SHIFT)
#define MIDI_DEFAULT_VELOCITY 64

/*********************************************************************/
static char *ChannelNames[] =
{
	"Input0",
	"Input1",
	"Input2",
	"Input3",
	"Input4",
	"Input5",
	"Input6",
	"Input7",
/* This next comma was missing so the name was wrong.
** This affected folks using mixer12x2.dsp 940526 */
	"Input8",
	"Input9",
	"Input10",
	"Input11"
};

static char *LeftNames[] =
{
	"LeftGain0",
	"LeftGain1",
	"LeftGain2",
	"LeftGain3",
	"LeftGain4",
	"LeftGain5",
	"LeftGain6",
	"LeftGain7",
	"LeftGain8",
	"LeftGain9",
	"LeftGain10",
	"LeftGain11"
};

static char *RightNames[] =
{
	"RightGain0",
	"RightGain1",
	"RightGain2",
	"RightGain3",
	"RightGain4",
	"RightGain5",
	"RightGain6",
	"RightGain7",
	"RightGain8",
	"RightGain9",
	"RightGain10",
	"RightGain11"
};

static int32 InitScoreContext (  ScoreContext *scon );
static int32 TermScoreContext (  ScoreContext *scon );
static Item CreateChannelInstrument( ScoreContext *scon, int32 Channel, NoteTracker *nttr );
static Item DeleteChannelInstrument( ScoreContext *scon, NoteTracker *nttr );
static int32 UpdateChannelVolume( ScoreContext *scon, int32 Channel );
static int32 SetScoreNoteGain( ScoreContext *scon, int32 Channel, NoteTracker *nttr );
static NoteTracker *FindScoreInstrument( ScoreContext *scon, Item Instrument );
static int32 NoteStopIns ( Item Instrument, int32 Note );


/*****************************************************************/
/****** Library Routines *****************************************/
/*****************************************************************/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/convertpitchbend
 |||	ConvertPitchBend - Converts a MIDI pitch bend value into frequency
 |||	                   multiplier.
 |||
 |||	  Synopsis
 |||
 |||	    Err ConvertPitchBend( int32 Bend, int32 SemitoneRange,
 |||	                          frac16 *BendFractionPtr )
 |||
 |||	  Description
 |||
 |||	    This procedure accepts a MIDI pitch bend value from 0x0 to 0x3FFF.  It
 |||	    also accepts a pitch bend range in semitones (half steps).  The range
 |||	    value measures the distance from normal pitch to the farthest bent pitch.
 |||	    For example, a pitch bend range of 12 semitones means that an instrument
 |||	    can be bent up in pitch by 12 semitones, and down in pitch by 12 semitones
 |||	    for a total range of 24 semitones (two octaves).
 |||
 |||	    The pitch bend value operates within the pitch bend range: 0x0 means bend
 |||	    pitch all the way to the bottom of the range; 0x2000 means don't bend
 |||	    pitch; and 0x3FFF means bend pitch all the way to the top of the range.
 |||
 |||	    ConvertPitchBend() uses the pitch bend range and the pitch bend value to
 |||	    calculate an internal pitch bend valuea value used to multiply the output
 |||	    of an instrument, bending the instrument's pitch up or down.  The
 |||	    internal pitch bend value is written into the variable "BendFractionPtr."
 |||	    This bend value is the same as the bend value used for the audio folio
 |||	    call BendInstrumentPitch().
 |||
 |||	    Note that DSP sampled-sound instruments won't bend any more than 12
 |||	    semitones, so don't set the pitch bend range to a value greater than
 |||	    12.
 |||
 |||	  Arguments
 |||
 |||	    Bend                         A MIDI pitch bend value from 0x0 to 0x3FFF.
 |||
 |||	    SemitoneRange                A pitch bend range value from 1 to 12,
 |||	                                 measured in semitones away from normal
 |||	                                 pitch.
 |||
 |||	    BendFractionPtr              A pointer to a frac16 variable in which to
 |||	                                 store the returned frequency multiplier.
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
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    ChangeScorePitchBend(), GetScoreBendRange(), SetScoreBendRange()
 |||
**/
Err ConvertPitchBend( int32 Bend, int32 SemitoneRange, frac16 *BendFractionPtr )
{
	int32 Semis, SemiParts, BigBend, Cents, Result;

/*	BigBend = Bend * SemitoneRange; 940404 Corrected for center offset. */
	BigBend = (Bend - MIDI_BEND_CENTER) * SemitoneRange;
	Semis = BigBend >> MIDI_BEND_SHIFT;
	SemiParts = (BigBend - (Semis << MIDI_BEND_SHIFT));
	Cents = (SemiParts * 100) >> MIDI_BEND_SHIFT;
DBUG(("Bend = 0x%x, Semis = %d, SemiParts = %d\n", Bend, Semis, SemiParts));
	Result = Convert12TET_F16( Semis, Cents, BendFractionPtr );
	if( Result < 0 ) return Result;
	return 0;
}

/* Set range of pitch bend so users don't have to poke data structure. */
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/setscorebendrange
 |||	SetScoreBendRange - Sets the current pitch bend range value for a score
 |||	                    context.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetScoreBendRange( ScoreContext *scon,
 |||	                           int32 BendRange )
 |||
 |||	  Description
 |||
 |||	    The procedure sets a score's current pitch bend range value.  This
 |||	    value is an integer, measured in semitones (half steps), that sets the
 |||	    range above or below normal that instruments can be bent.  A pitch range
 |||	    of two, for example, means that instruments can bend up two semitones (a
 |||	    whole step up) and down two semitones (a whole step down).
 |||
 |||	    Note that DSP sampled-sound instruments won't bend any more than 12
 |||	    semitones, so don't set BendRange to a value greater than 12.
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to a ScoreContext data structure.
 |||
 |||	    BendRange                    A pitch bend range value from 1 to 12.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if all went well; otherwise, an error code (less
 |||	    then 0) indicating that an error occurred.
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
 |||	    ChangeScorePitchBend(), ConvertPitchBend(), GetScoreRange()
 |||
**/
Err SetScoreBendRange( ScoreContext *scon, int32 BendRange )
{
	scon->scon_BendRange = BendRange;
	return 0;
}
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/getscorebendrange
 |||	GetScoreBendRange - Gets the current pitch bend range value for a score
 |||	                    context.
 |||
 |||	  Synopsis
 |||
 |||	    int32 GetScoreBendRange( ScoreContext *scon )
 |||
 |||	  Description
 |||
 |||	    This procedure retrieves the pitch bend range value currently set for the
 |||	    score.  This value is an integer, measured in semitones (half steps), that
 |||	    sets the range above or below normal that instruments can be bent.  A
 |||	    pitch range of two, for example, means that instruments can bend up two
 |||	    semitones (a whole step up) and down two semitones (a whole step down).
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to a ScoreContext data structure.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns the current pitch bend range value of the specified
 |||	    score if successful, or an error code (a negative value) if an error
 |||	    occurs.
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
 |||	    ChangeScorePitchBend(), ConvertPitchBend(), SetScoreBendRange()
 |||
**/
int32 GetScoreBendRange( ScoreContext *scon  )
{
	return scon->scon_BendRange;
}


 /**
 |||	AUTODOC PUBLIC mpg/musiclib/score/disablescoremessages
 |||	DisableScoreMessages - Enable or disable printed messages during score playback.
 |||
 |||	  Synopsis
 |||
 |||	    int32 DisableScoreMessages( int32 Flag )
 |||
 |||	  Description
 |||
 |||	    This function turns on or off the informational and error message
 |||	    printing during score playback. Messages default to being enabled.
 |||	    Also, there are a few error messages that cannot be disabled.
 |||
 |||	  Arguments
 |||
 |||	    Flag                         Non-zero to disable messages, 0 to enable messages.
 |||	                                 Messages default to being enabled.
 |||
 |||	  Return Value
 |||
 |||	    Previous enable/disable flag.
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
 **/
int32 DisableScoreMessages( int32 Flag )
{
	int32 ret;
	ret = gMessagesDisabled;
	gMessagesDisabled = Flag;
	return ret;
}

/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/setpimapentry
 |||	SetPIMapEntry - Specifies the instrument to use when a MIDI program change
 |||	                occurs.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetPIMapEntry( ScoreContext *scon, int32 ProgramNum,
 |||	                       Item InsTemplate, int32 MaxVoices, int32 Priority )
 |||
 |||	  Description
 |||
 |||	    This procedure specifies the instrument type to use when a MIDI program
 |||	    change occurs.
 |||
 |||	    This procedure directly sets a PIMap entry, assigning an instrument
 |||	    template, a maximum voice value, and an instrument priority to a MIDI
 |||	    program number.  When a MIDI program change occurs, a new instrument
 |||	    specified by the instrument template is used for notes played after the
 |||	    program change.
 |||
 |||	    The variable "ProgramNum" ranges from 0 to 127 (unlike some
 |||	    synthesizers, which range program numbers from 1 to 128).  It must not
 |||	    exceed the number of programs allocated in CreateScoreContext().
 |||
 |||	    Note that it's easier to create a full PIMap using LoadPIMap().
 |||
 |||	  Arguments
 |||
 |||	    scon                         A pointer to the ScoreContext data structure
 |||	                                 whose PIMap is to be changed.
 |||
 |||	    ProgramNum                   Value indicating a program number, ranging
 |||	                                 from 0127; not to exceed the maximum number
 |||	                                 allocated in CreateScoreContext().
 |||
 |||	    InsTemplate                  The item number of an instrument template.
 |||
 |||	    MaxVoices                    The maximum number of voices that can play
 |||	                                 simultaneously for this program.
 |||
 |||	    Priority                     An instrument priority value from 0 to 200.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if all went well; or an error code (less then 0)
 |||	    indicating that an error occurred.
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
 |||	    LoadPIMap(), UnloadPIMap()
 |||
**/
int32 SetPIMapEntry( ScoreContext *scon, int32 ProgramNum, Item InsTemplate, int32 MaxVoices, int32 Priority )
{
	PIMapEntry *pimp;

	if ((ProgramNum < 0) || (ProgramNum > scon->scon_PIMapSize))
	{
		ERR(("SetPIMapEntry: Program Number out of range = %d\n", ProgramNum));
		return ML_ERR_OUT_OF_RANGE;
	}
	pimp = &scon->scon_PIMap[ProgramNum];
	pimp->pimp_InsTemplate = InsTemplate;
	pimp->pimp_Priority = (uint8)Priority;
	pimp->pimp_MaxVoices = (uint8)MaxVoices;
	return 0;
}

/******************************************************************
** Init Dynamic Voice Allocation
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/initscoredynamics
 |||	InitScoreDynamics - Sets up dynamic voice allocation.
 |||
 |||	  Synopsis
 |||
 |||	    Err InitScoreDynamics( ScoreContext *scon,
 |||	                           int32 MaxScoreVoices )
 |||
 |||	  Description
 |||
 |||	    This procedure creates an appropriate number of note trackers to handle
 |||	    the maximum number of voices specified.  It's called internally by
 |||	    InitScoreMixer().  You should use this call instead of InitScoreMixer() if
 |||	    your task is setting up MIDI score playback using non-DSP instruments.
 |||	    InitScoreDynamics() sets up voice allocation without creating a mixer
 |||	    instrument.
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to a ScoreContext data structure.
 |||
 |||	    MaxNumVoices                 A value indicating the maximum number of
 |||	                                 voices for the score.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
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
 |||	    InitScoreMixer(), TermScoreMixer()
 |||
**/
int32 InitScoreDynamics (  ScoreContext *scon, int32 MaxScoreVoices )
{
	int32 i;
	NoteTracker  *nttr;

/* Allocate and link Note Trackers */
	scon->scon_NoteTrackers = (NoteTracker  *) EZMemAlloc(MaxScoreVoices * sizeof(NoteTracker), 0);
	if (scon->scon_NoteTrackers == NULL)
	{
		ERR(("InitScoreContext failed to allocate NoteTrackers\n"));
		return ML_ERR_NOMEM;
	}

/* Init each NoteTracker. */
	for (i=0; i<MaxScoreVoices; i++)
	{
		nttr = &scon->scon_NoteTrackers[i];
		nttr->nttr_Note = 0;
		nttr->nttr_Instrument = 0;
		nttr->nttr_MixerChannel = (uint8)i;
		nttr->nttr_Channel = -1;
DBUGALLOC(("NoteTracker 0x%x added to Free 0x%x.\n", nttr, &scon->scon_FreeNoteTrackers));
		AddTail( &scon->scon_FreeNoteTrackers, (Node *) nttr );
		nttr->nttr_Flags = NTTR_FLAG_FREE;
	}
	scon->scon_MaxVoices = (uint8)MaxScoreVoices;
	return 0;
}

/******************************************************************
** Allocate a Score Context
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/createscorecontext
 |||	CreateScoreContext - Allocates a score context.
 |||
 |||	  Synopsis
 |||
 |||	    ScoreContext *CreateScoreContext( int32 MaxNumPrograms )
 |||
 |||	  Description
 |||
 |||	    This procedure allocates and initializes a score context with room enough
 |||	    for nMIDI programs.  It also creates a default PIMap for the score
 |||	    context.  The score context is necessary to import a MIDI score from a
 |||	    disc file and then play back that file.
 |||
 |||	  Arguments
 |||
 |||	    MaxNumPrograms               The maximum number of MIDI programs (1128)
 |||	                                 for which memory will be allocated within
 |||	                                 the score context.
 |||
 |||	  Return Value
 |||
 |||	    Returns a pointer to a new ScoreContext on success; NULL on failure.
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
 |||	    InitScoreDynamics(), InitScoreMixer(),
 |||	    ChangeScoreControl() TermScoreMixer(),
 |||	    ChangeScoreProgram(), DeleteScoreContext()
 |||
**/
ScoreContext *CreateScoreContext( int32 MaxNumPrograms )
{
	ScoreContext *scon;
	PIMapEntry *pimp;
	int32 Result;

    PULL_MUSICLIB_PACKAGE_ID(score);

	scon = (ScoreContext *) UserMemAlloc( sizeof(ScoreContext), MEMTYPE_FILL );
	if(scon == NULL) return NULL;

	pimp = (PIMapEntry *) UserMemAlloc( sizeof(PIMapEntry) * MaxNumPrograms, MEMTYPE_FILL );
	if(pimp == NULL)
	{
		goto freescon;
	}
	scon->scon_PIMapSize = (uint8)MaxNumPrograms;
	scon->scon_PIMap = pimp;

	Result = InitScoreContext( scon );
	if(Result < 0)
	{
		goto freepimp;
	}

	return scon;

freepimp:
	UserMemFree( pimp, sizeof(PIMapEntry) * MaxNumPrograms );
freescon:
	UserMemFree( scon, sizeof(ScoreContext) );
	return NULL;
}

/******************************************************************
** Delete a Score Context
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/deletescorecontext
 |||	DeleteScoreContext - Deletes a score context.
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteScoreContext( ScoreContext *scon )
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the ScoreContext data structure along with all of
 |||	    its attendant data structures, including the NoteTracker data structures
 |||	    used for dynamic voice allocation.
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to the ScoreContext data structure
 |||	                                 to be deleted.
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
 |||	    InitScoreMixer(), CreateScoreContext()
 |||
**/
int32 DeleteScoreContext( ScoreContext *scon )
{

	TermScoreContext( scon );

	if (scon->scon_PIMap )
	{
		UserMemFree( scon->scon_PIMap,
			sizeof(PIMapEntry) * (uint32)scon->scon_PIMapSize);
		scon->scon_PIMap = 0;
	}

	UserMemFree( scon, sizeof(ScoreContext) );

	return 0;
}

/******************************************************************
** Set to reasonable defaults.
******************************************************************/
static int32 InitScoreContext (  ScoreContext *scon )
{
	int32 i, Result = 0;
	ScoreChannel *schn;

	scon->scon_MaxVolume = (MAXDSPAMPLITUDE / MAXSCOREVOICES);
	scon->scon_MaxVoices = 0;
	scon->scon_MixerIns = 0;
	scon->scon_Flags = SCON_FLAG_USE_INSPRI | SCON_FLAG_VERBOSE | SCON_FLAG_NO_TEMPLATE_OK;
	scon->scon_NoteTrackers = NULL;
	scon->scon_BendRange = 2;

	for (i=0; i<NUMMIDICHANNELS; i++)
	{
		schn = &scon->scon_Channels[i];

		InitList( &(schn->schn_NoteList), "NoteLists" );
		schn->schn_DefaultProgram = (uint8) i;
		schn->schn_CurrentProgram = -2;  /* Meaning no instrument, not even default. */
		schn->schn_NumVoices = 0;
		schn->schn_Priority = 100;
		schn->schn_Volume = 127 << 7;
		schn->schn_Pan = 64 << 7;
		schn->schn_PitchBend = MIDI_BEND_CENTER;
		UpdateChannelVolume( scon, i );
	}
	for (i=0; i<scon->scon_PIMapSize; i++)
	{
		scon->scon_PIMap[i].pimp_InsTemplate = 0;
		scon->scon_PIMap[i].pimp_MaxVoices = 1;
		scon->scon_PIMap[i].pimp_Priority = 100;
	}

	InitList( &scon->scon_FreeNoteTrackers, "FreeNotes" );

	return Result;
}

/******************************************************************
** Allocate a note tracker from the FreeList.
******************************************************************/
static NoteTracker *AllocNoteTracker(  ScoreContext *scon )
{
	NoteTracker *nttr;

	nttr = (NoteTracker *)FirstNode( &scon->scon_FreeNoteTrackers );

	if (IsNode(&scon->scon_FreeNoteTrackers, nttr))
	{
		nttr->nttr_Flags &= ~NTTR_FLAG_FREE;
DBUGALLOC(("RemNode 0x%x in AllocNoteTracker{", nttr));
		RemNode( (Node *)nttr );
DBUGALLOC(("}\n"));
	}
	else
	{
		nttr = NULL;  /* Empty. */
	}

	return nttr;
}

/******************************************************************
** Unlink a tracker from a channel, mark one less instrument.
******************************************************************/
static int32 UnhookNoteTracker( ScoreContext *scon, NoteTracker *nttr )
{
	if (nttr->nttr_Flags & NTTR_FLAG_INUSE )
	{
DBUGALLOC(("RemNode 0x%x in UnhookNoteTracker\n", nttr));
		RemNode( (Node *) nttr );
		nttr->nttr_Flags &= ~NTTR_FLAG_INUSE;
		scon->scon_Channels[nttr->nttr_Channel].schn_NumVoices -= 1;
	}
	return 0;
}

/******************************************************************
** Free a notes Instrument and return it to the FreeList.
******************************************************************/
static int32 FreeNoteTracker(  ScoreContext *scon, NoteTracker *nttr )
{
	int32 Result = 0;

	if ( nttr->nttr_Instrument > 0)
	{
		Result = DeleteChannelInstrument( scon, nttr );
		if (Result < 0)
		{
			if ((Result & 0xFF) != ER_BadItem)  /* Expected error. */
			{
				ERR(("FreeNoteTracker: delete channel instrument returned 0x%x\n", Result));
				return Result;
			}
		}
	}

	UnhookNoteTracker( scon, nttr );

	if((nttr->nttr_Flags & NTTR_FLAG_FREE) == 0)
	{
DBUGALLOC(("FreeNoteTracker: NoteTracker 0x%x added to Free 0x%x.\n", nttr, &scon->scon_FreeNoteTrackers));
		AddTail( &scon->scon_FreeNoteTrackers, (Node *) nttr );
		nttr->nttr_Flags |= NTTR_FLAG_FREE;
		nttr->nttr_Channel = -1; /* clear channel so update works 940404 */
	}
	else
	{
		ERR(("FreeNoteTracker: nttr 0x%x already free!\n", nttr));
	}

	return Result;
}


/******************************************************************
** Traverse list of notes for channel and free them.
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/freechannelinstruments
 |||	FreeChannelInstruments - Frees all of a MIDI channel's instruments.
 |||
 |||	  Synopsis
 |||
 |||	    Err FreeChannelInstruments( ScoreContext *scon,
 |||	                                int32 Channel )
 |||
 |||	  Description
 |||
 |||	    This procedure frees all instruments currently allocated to a MIDI channel
 |||	    of a score context, an action that abruptly stops any notes the
 |||	    instruments may be playing.
 |||
 |||	    This call is useful because allocated instruments may accumulate in a
 |||	    channel as dynamic voice allocation creates instruments to handle
 |||	    simultaneous note playback within the channel.  As notes stop, the
 |||	    instruments used to play the notes are abandoned, but not freed, remaining
 |||	    in existence so they can be used to immediately play incoming notes.
 |||	    These abandoned instruments use up system resources.
 |||
 |||	    If a task knows that it will not play notes in a MIDI channel for a period
 |||	    of time, it can use FreeChannelInstruments() to free all allocated
 |||	    instruments dedicatedto a channel.  This frees all of the system resources
 |||	    used to support those instruments, making them available for other audio
 |||	    jobs.  When notes are played in a channel after all of its voices are
 |||	    freed, dynamic voice allocation immediately allocates new instruments to
 |||	    play those notes.
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to a ScoreContext data structure.
 |||
 |||	    Channel                      The number of the MIDI channel for which to
 |||	                                 free instruments.
 |||
 |||	  Return Value
 |||
 |||	    This macro returns 0 if successful or an error code (a negative value) if
 |||	    an error occurs.
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
 |||	    ReleaseScoreNote(), StartScoreNote()
 |||
**/
int32 FreeChannelInstruments( ScoreContext *scon, int32 Channel )
{
	int32 Result = 0;
	NoteTracker  *nttr, *nextnttr;
	ScoreChannel *schn;
	int32 VoiceCount, NumVoices;

	schn = &scon->scon_Channels[Channel];
	NumVoices = schn->schn_NumVoices;
	VoiceCount = 0;

/* Scan list of Notes, freeing all instruments. */
	nttr = (NoteTracker *)FirstNode( &schn->schn_NoteList );

	while (ISNODE(&schn->schn_NoteList, nttr))
	{
		nextnttr = (NoteTracker *)NextNode((Node *)nttr);
		FreeNoteTracker( scon, nttr );
		nttr = nextnttr;
		VoiceCount++;
	}
	if(VoiceCount != NumVoices)
	{
		ERR(("FreeChannelInstruments: schn_NumVoices = %d, but found %d\n",
			 NumVoices, VoiceCount));
		ERR(("FreeChannelInstruments: Channel = %d\n", Channel));
	}
	schn->schn_NumVoices = 0;  /* 930603 */

	return Result;
}

/*****************************************************************/
static int32 TermScoreContext (  ScoreContext *scon )
{
	int32 i;
	int32 Result = 0;


	for (i=0; i<NUMMIDICHANNELS; i++)
	{
		Result = FreeChannelInstruments( scon, i );
		CHECKRESULT(Result, "FreeChannelInstruments" );
	}

	if(scon->scon_NoteTrackers)
	{
		EZMemFree(scon->scon_NoteTrackers);
		scon->scon_NoteTrackers = NULL;
	}

cleanup:
	return TermScoreMixer(scon);
}

/******************************************************************
** Find the note tracker that uses the instrument by searching the
** array of notes trackers.
******************************************************************/
NoteTracker *FindScoreInstrument( ScoreContext *scon, Item Instrument )
{
	NoteTracker *nttr;
	int32 i;

	nttr = scon->scon_NoteTrackers;
	for( i=0; i<scon->scon_MaxVoices; i++)
	{
		if(nttr->nttr_Instrument == Instrument)
		{
			return nttr;
		}
		nttr++;
	}
	return NULL;
}

/******************************************************************/
static Item CreateChannelInstrument( ScoreContext *scon, int32 Channel, NoteTracker *nttr )
{
	Item Instrument;
	TagArg Tags[4];
	int32 Result;
	Item Template;
	ScoreChannel *schn;
	PIMapEntry *pimp;

	schn = &scon->scon_Channels[Channel];
	pimp = &scon->scon_PIMap[schn->schn_CurrentProgram];
	Template = pimp->pimp_InsTemplate;

	Tags[0].ta_Tag = AF_TAG_PRIORITY;
	Tags[0].ta_Arg = (void *) schn->schn_Priority;
	Tags[1].ta_Tag = AF_TAG_SET_FLAGS;
	Tags[1].ta_Arg = (void *) AF_INSF_AUTOABANDON;
/* Set calculation rate divisor if non-zero value specified. */
	if( pimp->pimp_RateDivide != 0 )
	{
		Tags[2].ta_Tag = AF_TAG_CALCRATE_DIVIDE;
		Tags[2].ta_Arg = (void *) pimp->pimp_RateDivide;
		Tags[3].ta_Tag = TAG_END;
	}
	else
	{
		Tags[2].ta_Tag = TAG_END;
	}

	Instrument  = CreateInstrument( Template, Tags );
DBUGALLOC(("CreateInstrument returns 0x%x\n", Instrument ));
	if (Instrument <= 0) return Instrument;
	nttr->nttr_Instrument = Instrument;

/* Connect Instrument to Mixer if available. !!!!! */
	if (scon->scon_MixerIns)
	{
		Result = ConnectInstruments (Instrument, "Output", scon->scon_MixerIns,
					ChannelNames[nttr->nttr_MixerChannel]);
		if( Result < 0 )
		{
			ERR(("ConnectInstruments failed! 0x%x\n", Result ));
			return Result;
		}
	}
	return Instrument;
}

/******************************************************************/
static Item DeleteChannelInstrument( ScoreContext *scon, NoteTracker *nttr )
{
	Item Instrument;
	int32 Result;

	Instrument = nttr->nttr_Instrument;
	if(Instrument)
	{
/* Disconnect Instrument from Mixer.*/
		if (scon->scon_MixerIns)
		{
			DisconnectInstruments (Instrument, "Output", scon->scon_MixerIns,
					ChannelNames[nttr->nttr_MixerChannel]);
		}

		Result = FreeInstrument( Instrument );
		nttr->nttr_Instrument = 0;
		if (Result < 0)
		{
			DBUG(("DeleteChannelInstrument: FreeInstrument returns 0x%x\n", Result));
			return Result;
		}
	}

DBUGALLOC(("Instrument 0x%x freeed.\n", Instrument));
	return 0;
}
/******************************************************************
** Find the lowest priority instrument.
******************************************************************/
typedef struct VoiceStealer
{
	int32  vcst_Priority;
	int32  vcst_Activity;
} VoiceStealer;

static Err SelectNoteTracker( ScoreContext *scon, VoiceStealer *vcst, NoteTracker **NttrPtrPtr)
{
	int32 i, Result;
	Item Instrument;
	NoteTracker *nttr;
/* Order of parameters: Activity > Time > Priority */
/* Note: Lowest/Earliest values are invalid while BestNTTR==NULL */
	int32 LowestPriority=0, Priority, MaxPriority;
	int32 LowestActivity=0, Activity, MaxActivity;
	int32 EarliestTime=0, StartTime;
	NoteTracker *BestNTTR;
	TagArg Tags[4];

DBUGALLOC(("SelectNoteTracker: pri=%ld act=%ld\n", vcst->vcst_Priority, vcst->vcst_Activity));

	Tags[0].ta_Tag = AF_TAG_PRIORITY;
	Tags[1].ta_Tag = AF_TAG_STATUS;
	Tags[2].ta_Tag = AF_TAG_START_TIME;
	Tags[3].ta_Tag = TAG_END;

	MaxPriority = vcst->vcst_Priority;
	MaxActivity = vcst->vcst_Activity;
	BestNTTR = NULL;
	*NttrPtrPtr = NULL;
	Result = 0;   /* 931210 */

	nttr = scon->scon_NoteTrackers;
	for( i=0; i<scon->scon_MaxVoices; i++)
	{
		if(nttr->nttr_Instrument != 0)
		{
			Instrument = nttr->nttr_Instrument;
			Result = GetAudioItemInfo( Instrument, Tags );
			if( Result < 0 )
			{
				ERR(("SelectNoteTracker: GetAudioItemInfo returned 0x%x\n", Result));
				return Result;
			}
			Priority = (int32) Tags[0].ta_Arg;
			Activity = (int32) Tags[1].ta_Arg;
			StartTime = (int32) Tags[2].ta_Arg;
DBUGALLOC(("Select: Inst = 0x%x, Pri = %d, Act = %d, StartTime = %d\n", Instrument, Priority, Activity, StartTime ));

                /* 940812: new criteria to choose a voice to steal */
            if (
                (Activity <= MaxActivity) &&                                    /* only consider voices that are <= maxact */
                ((Priority <= MaxPriority) || (Activity <= AF_ABANDONED)) &&    /* and <= maxpri or abandoned */
                (
                    !BestNTTR ||                                                /* none matched so far? */
                    (Activity < LowestActivity) ||                              /* lower activity? */
                    (Activity == LowestActivity) &&
                    (
                        (Priority < LowestPriority) ||                          /* lower priority? */
                        (Priority == LowestPriority) &&
                            (CompareAudioTimes (StartTime,EarliestTime) < 0)    /* earlier start time? */
                    )
                )
            )

		  /* 940812: replaced w/ above
			if( ((Activity < LowestActivity) && (Priority <= MaxPriority)) ||
				((Priority < LowestPriority) && (Activity <= LowestActivity)) ||
				((CompareAudioTimes (StartTime,EarliestTime) < 0) && (Priority <= LowestPriority) &&
					(Activity <= LowestActivity)))
		  */

			{

DBUGALLOC(("Select: Best so far = 0x%x\n",  Instrument ));
				BestNTTR = nttr;
				LowestPriority = Priority;
				LowestActivity = Activity;
				EarliestTime = StartTime;
			}
		}
		nttr++;
	}

	if(BestNTTR)
	{
		vcst->vcst_Priority = LowestPriority;
		vcst->vcst_Activity = LowestActivity;
		*NttrPtrPtr = BestNTTR;
	}
	return Result;
}

/******************************************************************
** Allocate a note tracker with an instrument.
******************************************************************/
static int32 AllocScoreNote( ScoreContext *scon, int32 Channel, int32 Note, NoteTracker **NttrPtrPtr )
{

	Item Instrument = 0;
	Item Template;
	ScoreChannel *schn;
	int32 Result;
	NoteTracker *nttr, *FoundNTTR;
	int32 MaxVoices;
	VoiceStealer VCST;
	frac16 BendFrac; /* 940404 */

	*NttrPtrPtr = NULL;

#ifdef REAL_TIME_TRACE
	if(TraceIndex >= MAX_TRACES) TraceIndex = 0;
	TRecs[TraceIndex].trec_Time = GetAudioTime();
	TRecs[TraceIndex].trec_Note = Note;
	TRecs[TraceIndex].trec_Channel = Channel;
	TRecs[TraceIndex].trec_Inst = 0;
#endif

	schn = &scon->scon_Channels[Channel];

/*
	Order of attempts to get a voice:

	Grab same note if already assigned. (AA1)
	Tries to adopt instrument from self. (AA2)
	Then if < MaxVoices, tries to allocate new instrument. (AA3)
	Then tries to scavenge from self. (AA4)

	Finds optimal note to steal from Score Context.
	Compares that to external note if PurgeHook set.

	Then tries to adopt from other channels. (AA5)
	Then tries to adopt from external. (AA6)
	Then tries to scavenge from other channels. (AA7)
	Then tries to scavenge from external. (AA8)
*/

/* Use default program if no program change received. */
	if ( schn->schn_CurrentProgram < -1)
	{
		Result = ChangeScoreProgram(scon, Channel,
			schn->schn_DefaultProgram);
		if (Result < 0) return Result;
	}

	if ( schn->schn_CurrentProgram < 0)
	{
		return 0;
	}

	Template = scon->scon_PIMap[schn->schn_CurrentProgram].pimp_InsTemplate;
	MaxVoices = scon->scon_PIMap[schn->schn_CurrentProgram].pimp_MaxVoices;
	nttr = NULL;

DBUGALLOC(("-------\nAllocScoreNote: Channel = %d, Template = 0x%x\n", Channel, Template ));

#if 0       /* removed 940603 */
/**************************************************************************
** Look to see if there is already a Note Tracker assigned to that note.
** If so, use it. (AA1) */
	nttr = FindScoreNote( schn, Note );
	if (nttr)
	{
#ifdef REAL_TIME_TRACE
	TRecs[TraceIndex].trec_AllocationType = AtSameNote;
#endif
		UnhookNoteTracker( scon, nttr );
DBUGALLOC(("Note %d already assigned so use it. Instrument = 0x%x\n", Note, nttr->nttr_Instrument));
		goto done;
	}
#endif	    /* removed 940603 */

/* Try to adopt one from current template.  (AA2) *******************************/
	Instrument = AdoptInstrument( Template );
	if ( Instrument > 0 )
	{
DBUGALLOC(("Instrument 0x%x adopted from self.\n", Instrument));
		nttr = FindScoreInstrument( scon, Instrument );
		if (nttr)
		{
#ifdef REAL_TIME_TRACE
	TRecs[TraceIndex].trec_AllocationType = AtAdoptSelf;
#endif
			UnhookNoteTracker( scon, nttr );
			goto done;
		}
		else
		{
			ERR(("AllocScoreNote: Could not find adopted instrument!\n"));
			goto done;
		}
	}

/* Create a new Instrument (AA3) ***********************************************/
DBUGALLOC(("schn->schn_NumVoices = %d\n", schn->schn_NumVoices));
	if( schn->schn_NumVoices < MaxVoices)
	{
		nttr = AllocNoteTracker( scon );
		if (nttr)
		{
			Instrument = CreateChannelInstrument( scon, Channel, nttr );
			if ( Instrument > 0 )
			{
#ifdef REAL_TIME_TRACE
	TRecs[TraceIndex].trec_AllocationType = AtAllocate;
#endif
				goto done;
			}
			else
			{
DBUGALLOC(("Could not allocate new instrument.\n", Instrument));
				FreeNoteTracker( scon, nttr );
				nttr = NULL;
			}
		}
	}
/* Steal from own template first if maxed out. (AA4) *******************************************/
	else
	{
		Instrument = ScavengeInstrument( Template, schn->schn_Priority, AF_STARTED, 0);
		if ( Instrument > 0 )
		{
DBUGALLOC(("Instrument 0x%x scavenged from own template.\n", Instrument));
			nttr = FindScoreInstrument( scon, Instrument );
			if (nttr)
			{
#ifdef REAL_TIME_TRACE
	TRecs[TraceIndex].trec_AllocationType = AtStealSelf;
#endif

				UnhookNoteTracker( scon, nttr );
				goto done;
			}
			else
			{
				ERR(("AllocScoreNote: Could not find scavenged instrument!\n"));
				goto cleanup;
			}
		}
		else
		{
			ERR(("AllocScoreNote: Scavenge from self failed!\n"));
			goto cleanup;
		}
	}

/* Select best note from score context ***********************************************/

	VCST.vcst_Activity = AF_STARTED;
	VCST.vcst_Priority = schn->schn_Priority;
	Result = SelectNoteTracker( scon, &VCST, &FoundNTTR );
	if( Result ) goto cleanup;

	if(FoundNTTR != NULL)
	{
		if((scon->scon_PurgeHook != NULL) && (VCST.vcst_Activity > AF_STOPPED))
		{
			nttr = AllocNoteTracker( scon );
			if (nttr)
			{
				if( (*scon->scon_PurgeHook)( schn->schn_Priority, AF_STOPPED ) > 0)
				{
					Instrument = CreateChannelInstrument( scon, Channel, nttr );
					if ( Instrument > 0 )
					{
#ifdef REAL_TIME_TRACE
	TRecs[TraceIndex].trec_AllocationType = AtPurgeHook;
#endif
						goto done; /* =========> */
					}
				}
				FreeNoteTracker( scon, nttr );
				nttr = NULL;
			}
		}

/* Use result from SelectNoteTracker() */
		nttr = FoundNTTR;
DBUGALLOC(("Deleted best=0x%x from ScoreContext.\n", nttr->nttr_Instrument));
		Result = DeleteChannelInstrument( scon, nttr);
		CHECKRESULT(Result, "DeleteChannelInstrument Scavenged");
		UnhookNoteTracker( scon, nttr );

		Instrument = CreateChannelInstrument( scon, Channel, nttr );
		if (Instrument > 0)
		{
#ifdef REAL_TIME_TRACE
	TRecs[TraceIndex].trec_AllocationType = AtStealOther;
#endif
				goto done; /* =========> */
		}
DBUGALLOC(("Create after steal failed.\n"));
		FreeNoteTracker( scon, nttr );
		nttr = NULL;
	}



/* Create a new Instrument after purging other instruments using callback. *********/
	if( (schn->schn_NumVoices < MaxVoices) && (scon->scon_PurgeHook != NULL))
	{
		nttr = AllocNoteTracker( scon );
		if (nttr)
		{
			if( (*scon->scon_PurgeHook)( schn->schn_Priority, AF_RELEASED ) > 0)
			{
				Instrument = CreateChannelInstrument( scon, Channel, nttr );
				if ( Instrument > 0 )
				{
#ifdef REAL_TIME_TRACE
	TRecs[TraceIndex].trec_AllocationType = AtPurgeHook;
#endif
					goto done; /* =========> */
				}
			}
			FreeNoteTracker( scon, nttr );
			nttr = NULL;
		}
	}

done:
	if (nttr)
	{
DBUGALLOC(("NoteTracker 0x%x with ins=0x%x added to 0x%x.\n", nttr,
		nttr->nttr_Instrument, &schn->schn_NoteList));

		nttr->nttr_Note = (int8) Note;
#ifdef REAL_TIME_TRACE
		TRecs[TraceIndex++].trec_Inst = nttr->nttr_Instrument;
#endif


DBUGALLOC(("nttr->nttr_Channel = %d, nttr->nttr_MixerChannel = %d.\n",
		nttr->nttr_Channel, nttr->nttr_MixerChannel ));

/* Have we moved to a new channel? */
		if(nttr->nttr_Channel != Channel)
		{
			nttr->nttr_Channel = (uint8)Channel;
/* Update mixer pan and level. */
			SetScoreNoteGain( scon, Channel, nttr );
/* Update pitch bend! 940404 */
			if( schn->schn_PitchBend != MIDI_BEND_CENTER );
			ConvertPitchBend( schn->schn_PitchBend, scon->scon_BendRange, &BendFrac );
			BendInstrumentPitch( nttr->nttr_Instrument, BendFrac );
		}

		if( nttr->nttr_Flags & NTTR_FLAG_INUSE)
		{
			ERR(("AllocScoreNote: NoteTracker in use!\n"));
		}
		else
		{
			AddTail( &schn->schn_NoteList, (Node *) nttr );
			nttr->nttr_Flags |= NTTR_FLAG_INUSE;
			schn->schn_NumVoices += 1;
		}

		if(schn->schn_NumVoices > MaxVoices)
		{
			ERR(("AllocScoreNote: maximum voice count exceeded, %d > %d\n",
				schn->schn_NumVoices,MaxVoices));
		}

		*NttrPtrPtr = nttr;
		return 0;
	}

cleanup:

	if(scon->scon_Flags & SCON_FLAG_VERBOSE)
	{
		ERR(("AllocScoreNote: Could not allocate or steal any notes.\n"));
	}

	return ML_ERR_NO_NOTES;
}

/******************************************************************
** Called by an application to purge unused instruments
** from the ScoreContext.  This call, coupled with the scon_PurgeHook
** allows an application to share DSP resources with the ScoreContext.
******************************************************************/

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/score/purgescoreinstrument
 |||	PurgeScoreInstrument - Purges an unused instrument from a ScoreContext.
 |||
 |||	  Synopsis
 |||
 |||	    Err PurgeScoreInstrument( ScoreContext *scon, uint8 Priority, int32 MaxLevel )
 |||
 |||	  Description
 |||
 |||	    This procedure purges an unused instrument from a ScoreContext. This
 |||	    call, coupled with the scon_PurgeHook allows an application to share
 |||	    DSP resources with the ScoreContext.
 |||
 |||	  Arguments
 |||
 |||	    scon                         A pointer to a ScoreContext data structure
 |||	                                 controlling playback.
 |||
 |||	    Priority                     The maximum instrument priority to purge
 |||	                                 for instruments that are still playing
 |||	                                 (in the range of 0 to 200). Instruments of
 |||	                                 higher priority that this may be purged
 |||	                                 if they have stopped playing.
 |||
 |||	    MaxLevel                     The maximum activity to purge (i.e.
 |||	                                 AF_ABANDONED, AF_STOPPED, AF_RELEASED,
 |||	                                 AF_STARTED).
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns a positive value if an instrument was actually
 |||	    purged, zero if no instrument matching the specifications could be
 |||	    purged, or a negative error code on failure.
 |||
 |||	  Examples
 |||
 |||	    This code fragment can be used to free the DSP resources used by all
 |||	    instruments that have finished playing:
 |||
 |||	    {
 |||	        int32 result;
 |||
 |||	            * loop until function returns no voice purged or error *
 |||	        while ( (result = PurgeScoreInstrument (scon, SCORE_MAX_PRIORITY,
 |||	                                                AF_STOPPED)) > 0 ) ;
 |||
 |||	            * catch error *
 |||	        if (result < 0) ...
 |||	    }
 |||
 |||	  Caveats
 |||
 |||	    This function deletes items that are created by StartScoreNote().
 |||	    Frequent use of this function and StartScoreNote() can consume the
 |||	    item table. If you simply want to stop a score note in it's tracks
 |||	    for later use with the same channel, use StopScoreNote() instead. It
 |||	    merely stops instruments and doesn't delete them.
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
 |||	    StopScoreNote()
 |||
 **/
Err PurgeScoreInstrument( ScoreContext *scon, uint8 Priority, int32 MaxLevel )
{
	int32 Result;
	NoteTracker *nttr;
	VoiceStealer VCST;

	VCST.vcst_Activity = MaxLevel;
	VCST.vcst_Priority = Priority;
	Result = SelectNoteTracker( scon, &VCST, &nttr );
	if( Result ) return Result;

	if(nttr)
	{
#ifdef REAL_TIME_TRACE
	if(TraceIndex >= MAX_TRACES) TraceIndex = 0;
	TRecs[TraceIndex].trec_Time = GetAudioTime();
	TRecs[TraceIndex].trec_Note = 0;
	TRecs[TraceIndex].trec_Channel = 0;
	TRecs[TraceIndex].trec_Inst = nttr->nttr_Instrument;
	TRecs[TraceIndex++].trec_AllocationType = AtPurgeIns;
#endif
		FreeNoteTracker( scon, nttr );
		nttr = NULL;
		Result = 1;
	}

	return Result;
}


#if 0       /* 940902: removed */
/******************************************************************
** Find the note tracker that matches the note by searching the
** linked list of notes currently active and ON.
******************************************************************/
static NoteTracker *FindScoreNote( ScoreChannel *schn, int32 Note )
{
	NoteTracker *nttr;

	nttr = (NoteTracker *)FirstNode( &schn->schn_NoteList );

DBUGNOTE(("FindScoreNote: Note = %d\n", Note ));

	while (ISNODE(&schn->schn_NoteList, nttr))
	{
DBUGNOTE(("FindScoreNote: check Note=%d, Ins=0x%x, Flags=0x%x\n",
	nttr->nttr_Note, nttr->nttr_Instrument, nttr->nttr_Flags));
		if (nttr->nttr_Note == Note
			&& ((nttr->nttr_Flags & NTTR_FLAG_NOTEON) != 0))        /* restored note on check 940603 */
		{
DBUGNOTE(("FindScoreNote: returns 0x%x\n", nttr ));
			return nttr;
		}
		nttr = (NoteTracker *)NextNode((Node *)nttr);
	}
	return NULL;
}
#endif

/******************************************************************
** Note Off
******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/score/releasescorenote
 |||	ReleaseScoreNote - Releases a MIDI note, uses voice allocation.
 |||
 |||	  Synopsis
 |||
 |||	    Err ReleaseScoreNote( ScoreContext *scon, int32 Channel,
 |||	                          int32 Note, int32 Velocity )
 |||
 |||	  Description
 |||
 |||	    This procedure releases a note for the score context and uses dynamic
 |||	    voice allocation to do so.  It is equivalent to a MIDI NoteOff message.
 |||
 |||	  Arguments
 |||
 |||	    scon                         A pointer to a ScoreContext data structure
 |||	                                 controlling playback.
 |||
 |||	    Channel                      The number of the MIDI channel in which to
 |||	                                 play the note.
 |||
 |||	    Note                         The MIDI pitch value of the note (0..127).
 |||
 |||	    Velocity                     The MIDI release velocity value of the note (0..127).
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Caveats
 |||
 |||	    At present, release velocity is ignored.
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
 |||	    FreeChannelInstruments(), NoteOnIns(), NoteOffIns(), StartScoreNote(),
 |||	    StopScoreNote()
 |||
 **/
int32 ReleaseScoreNote( ScoreContext *scon, int32 Channel, int32 Note, int32 Velocity )
{
    int32 Result = 0;
    NoteTracker *nttr;

        /* 940902: replaced completely */
        /* Release all "on" voices for this note on this channel */
    SCANLIST (&scon->scon_Channels[Channel].schn_NoteList, nttr, NoteTracker) {

        if (nttr->nttr_Note == Note && (nttr->nttr_Flags & NTTR_FLAG_NOTEON)) {
            nttr->nttr_Flags &= ~NTTR_FLAG_NOTEON;
            Result = NoteOffIns ( nttr->nttr_Instrument, Note, Velocity );
            if (Result < 0) {
                ERR(("InterpretMIDIMessage: NoteOffIns failed, Chan = %d, Note = %d\n", Channel, Note));
                return Result;
            }
        }

    }

    return Result;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/score/stopscorenote
 |||	StopScoreNote - Stop a MIDI note w/o releasing it, uses voice allocation.
 |||
 |||	  Synopsis
 |||
 |||	    Err StopScoreNote( ScoreContext *scon, int32 Channel, int32 Note )
 |||
 |||	  Description
 |||
 |||	    This procedure instantly stops a note for the score context and uses dynamic
 |||	    voice allocation to do so.  It differs from ReleaseScoreNote() in that
 |||	    it doesn't allow the note to go through any release phase that it might have.
 |||
 |||	  Arguments
 |||
 |||	    scon                         A pointer to a ScoreContext data structure
 |||	                                 controlling playback.
 |||
 |||	    Channel                      The number of the MIDI channel in which to
 |||	                                 play the note.
 |||
 |||	    Note                         The MIDI pitch value of the note (0..127).
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V24.
 |||
 |||	  Associated Files
 |||
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    FreeChannelInstruments(), NoteOnIns(), NoteOffIns(), StartScoreNote(),
 |||	    ReleaseScoreNote(), PurgeScoreInstrument()
 |||
 **/
int32 StopScoreNote( ScoreContext *scon, int32 Channel, int32 Note )
{
    int32 Result = 0;
    NoteTracker *nttr;

        /* stop all voices for this note on this channel ("on" or otherwise) */
    SCANLIST (&scon->scon_Channels[Channel].schn_NoteList, nttr, NoteTracker) {

        if (nttr->nttr_Note == Note) {
            nttr->nttr_Flags &= ~NTTR_FLAG_NOTEON;
            Result = NoteStopIns ( nttr->nttr_Instrument, Note );
            if (Result < 0) {
                ERR(("InterpretMIDIMessage: NoteStopIns failed, Chan = %d, Note = %d\n", Channel, Note));
                return Result;
            }
        }

    }

    return Result;
}

/******************************************************************
** Update Gain for Note for Volume and Pan
******************************************************************/
static int32 SetScoreNoteGain( ScoreContext *scon, int32 Channel, NoteTracker *nttr )
{
	int32 MixerChannel;
	int32 Result;
	ScoreChannel *schn;

	MixerChannel = nttr->nttr_MixerChannel;
	schn = &scon->scon_Channels[Channel];

	Result = TweakKnob ( scon->scon_LeftGains[MixerChannel], schn->schn_LeftVolume );
	if(Result < 0) return Result;
	Result = TweakKnob ( scon->scon_RightGains[MixerChannel], schn->schn_RightVolume );

DBUGALLOC(("SetScoreNoteGain: mc=%d, lv=%d, rv=%d\n", MixerChannel, schn->schn_LeftVolume, schn->schn_RightVolume));

	return Result;
}

/******************************************************************
** Note On for channel in score.
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/startscorenote
 |||	StartScoreNote - Starts a MIDI note, uses voice allocation.
 |||
 |||	  Synopsis
 |||
 |||	    Err StartScoreNote( ScoreContext *scon, int32 Channel,
 |||	                        int32 Note, int32 Velocity )
 |||
 |||	  Description
 |||
 |||	    This procedure turns on a note for the score context and uses dynamic
 |||	    voice allocation to do so.  It is equivalent to a MIDI Note On message.
 |||
 |||	  Arguments
 |||
 |||	    scon                         A pointer to a ScoreContext data structure
 |||	                                 controlling playback.
 |||
 |||	    Channel                      The number of the MIDI channel in which to
 |||	                                 play the note.
 |||
 |||	    Note                         The MIDI pitch value of the note (0..127).
 |||
 |||	    Velocity                     The MIDI attack velocity value of the note (0..127).
 |||	                                 If attack velocity is is 0, the specified note is
 |||	                                 released (identical behavior to the MIDI Note On event).
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
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
 |||	    FreeChannelInstruments(), NoteOnIns(), NoteOffIns(), ReleaseScoreNote(),
 |||	    StopScoreNote()
 |||
**/
int32 StartScoreNote( ScoreContext *scon, int32 Channel, int32 Note, int32 Velocity )
{
	int32 Result;
	NoteTracker *nttr;

	if(Velocity == 0)
	{
		Result = ReleaseScoreNote( scon, Channel, Note, MIDI_DEFAULT_VELOCITY);     /* 940902: now uses default velocity for 0 velocity note on */
	}
	else
	{
		Result = AllocScoreNote( scon, Channel, Note, &nttr);
		if (Result < 0)
		{
			/* Not unexpected. */
/*			ERR(("StartScoreNote: AllocScoreNote returned 0x%x\n", Result)); */
			return 0;
		}

		if(nttr)
		{
DBUG(("nttr->nttr_Instrument = 0x%x\n", nttr->nttr_Instrument));
			Result = NoteOnIns ( nttr->nttr_Instrument, Note, Velocity );
			nttr->nttr_Flags |= NTTR_FLAG_NOTEON;
		}
	}
	return Result;
}

/******************************************************************
** Translate any MIDI Event to Audio Folio calls based on Context
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/interpretmidimessage
 |||	InterpretMIDIMessage - Executes a MIDI message.
 |||
 |||	  Synopsis
 |||
 |||	    Err InterpretMIDIMessage( ScoreContext *ScoreCon,
 |||	                              char *MIDIMsg, int32 IfMute )
 |||
 |||	  Description
 |||
 |||	    This procedure executes a MIDI message by calling another music library
 |||	    procedure appropriate to that message, and passing the message's data
 |||	    to that procedure.  The called procedure uses audio folio procedures for
 |||	    playback based on the settings of a ScoreContext data structure.
 |||
 |||	    InterpretMIDIMessage() is called by InterpretMIDIEvent(), which extracts
 |||	    MIDI messages from juggler sequences containing MIDI events.  Although
 |||	    this procedure is used most often as an internal call of the music
 |||	    library, tasks may call it directly to execute a supplied MIDI message.
 |||	    The message should be stored in the first byte (first character) of the
 |||	    MIDIMsg string, followed by data bytes (if present) in subsequent bytes of
 |||	    the string.
 |||
 |||	    Whenever the IfMute argument is set to TRUE, this procedure does not
 |||	    process Note On messages with velocity values greater than zero.  In other
 |||	    words, it doesn't start new notes, but it does release existing notes
 |||	    and process all other recognized MIDI messages.  When IfMute is set to
 |||	    FALSE, this procedure processes all recognizable messages, including Note
 |||	    On messages.
 |||
 |||	    Note that InterpretMIDIEvent() reads a juggler sequence's mute flag
 |||	    and, if true, passes that true setting on to InterpretMIDIMessage() so
 |||	    that the sequence stops playing notes.  It likewise passes a false setting
 |||	    on to InterpretMIDIMessage() so that the sequence can play notes.
 |||
 |||	  Arguments
 |||
 |||	    ScoreCon                     Pointer to a ScoreContext data structure.
 |||
 |||	    MIDIMsg                      Pointer to a character string containing the
 |||	                                 MIDI message.
 |||
 |||	    IfMute                       A flag that turns muting on or off.  TRUE is
 |||	                                 on, FALSE is off.
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
 |||	  Caveats
 |||
 |||	    This procedure only handles Note On, Note Off, Change Program, Change
 |||	    Control (values 7 and 10), and Pitchbend MIDI messages.  It does not
 |||	     support running status.
 |||
 |||	  See Also
 |||
 |||	    InterpretMIDIEvent()
 |||
**/
int32 InterpretMIDIMessage( ScoreContext *scon, char *MIDIMsg, int32 IfMute)
{
	int32 Command, Channel, Result=0;

/* Interpret event. */
	Command = (int32) MIDIMsg[0] & 0xF0;
	Channel = (int32) MIDIMsg[0] & 0x0F;
DBUG(("Com = 0x%x, Chan = 0x%x\n", Command, Channel));

	switch(Command)
	{

		case 0x80:
			Result = ReleaseScoreNote( scon, Channel, MIDIMsg[1], MIDIMsg[2]);
			break;

		case 0x90:
			if((!IfMute) || (MIDIMsg[2] == 0)) /* Mute if Vel=0, 931028 */
			{
				Result = StartScoreNote( scon, Channel, MIDIMsg[1], MIDIMsg[2]);
			}
			break;

		case 0xB0:
			Result = ChangeScoreControl(scon, Channel, MIDIMsg[1], MIDIMsg[2]);
			break;

		case 0xC0:
			Result = ChangeScoreProgram(scon, Channel, MIDIMsg[1]);
			break;

		case 0xE0:
			Result = ChangeScorePitchBend(scon, Channel,
				    (((int32)MIDIMsg[2]<<(int32)7) |
				    (int32)MIDIMsg[1]) );
			break;
	}
	if( Result < 0 )
	{
		ERR(("InterpretMIDIMessage: Result = 0x%x, Command = 0x%x\n", Result, Command));
	}

	return Result;
}

/******************************************************************
** Allocate a new Instrument based on the PIMap
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/changescoreprogram
 |||	ChangeScoreProgram - Changes the MIDI program for a channel.
 |||
 |||	  Synopsis
 |||
 |||	    Err ChangeScoreProgram( ScoreContext *ScoreCon,
 |||	                            int32 Channel, int32 ProgramNum )
 |||
 |||	  Description
 |||
 |||	    This procedure changes the current MIDI program used to play notes in a
 |||	    MIDI channel.  It's the equivalent of a MIDI program change message.
 |||
 |||	  Arguments
 |||
 |||	    ScoreCon                     Pointer to a ScoreContext data structure
 |||	                                 controlling playback.
 |||
 |||	    Channel                      The number of the MIDI channel for which to
 |||	                                 change the program.
 |||
 |||	    ProgramNum                   The number of the new MIDI program to use
 |||	                                 for the channel.
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
 |||	    ChangeScoreControl(), ChangeScorePitchBend(), InterpretMIDIMessage(),
 |||	    StartScoreNote(), StopScoreNote()
 |||
**/
int32 ChangeScoreProgram( ScoreContext *scon, int32 Channel, int32 ProgramNum)
{
	int32 Result = 0;

DBUG(("ChangeScoreProgram( 0x%x, 0x%x, 0x%x )\n", scon,
		Channel, ProgramNum));

	if ( ProgramNum >= scon->scon_PIMapSize )
	{
		ERR(("ChangeScoreProgram: ProgramNum too large = %d\n", ProgramNum));
		return ML_ERR_OUT_OF_RANGE;
	}

	if ( scon->scon_PIMap[ProgramNum].pimp_InsTemplate > 0 )
	{
/* Free Instruments allocated for old program num. */
		if ( scon->scon_Channels[ Channel ].schn_CurrentProgram != (int8) ProgramNum )
		{
			FreeChannelInstruments( scon, Channel );
			scon->scon_Channels[ Channel ].schn_CurrentProgram = (int8) ProgramNum;
			if( scon->scon_Flags & SCON_FLAG_USE_INSPRI )
			{
				scon->scon_Channels[ Channel ].schn_Priority =
						scon->scon_PIMap[ProgramNum].pimp_Priority;
			}
		}
	}
	else
	{
		scon->scon_Channels[ Channel ].schn_CurrentProgram = (int8) -1;
		if(scon->scon_Flags & SCON_FLAG_VERBOSE)
		{
			ERR(("ChangeScoreProgram: No Template for Program %d+1=%d\n", ProgramNum,ProgramNum+1));
		}
		if((scon->scon_Flags & SCON_FLAG_NO_TEMPLATE_OK) == 0)
		{
			Result = ML_ERR_NO_TEMPLATE;
		}
	}

	return Result;
}

/******************************************************************
** Calculate new channel volumes based on Volume and Pan
** Channel ranges from 0-15
******************************************************************/
static int32 UpdateChannelVolume( ScoreContext *scon, int32 Channel )
{
	ScoreChannel *schn;
	int32 TotalVolume;
	NoteTracker *nttr;

	schn = &scon->scon_Channels[Channel];
/* Use 14 bit control values. */
	TotalVolume = (scon->scon_MaxVolume * schn->schn_Volume) >> 14;
	schn->schn_LeftVolume = (TotalVolume * ((1<<14) - schn->schn_Pan)) >> 14;
	schn->schn_RightVolume = (TotalVolume * schn->schn_Pan) >> 14;

DBUG(("UpdateChannelVolume: ch=%d, tvol=%d, lv=%d, rv=%d\n",
	Channel, TotalVolume, schn->schn_LeftVolume, schn->schn_RightVolume));

/* Update all notes on current channel. */
	nttr = (NoteTracker *)FirstNode( &schn->schn_NoteList );
	while (ISNODE(&schn->schn_NoteList, nttr))
	{
		SetScoreNoteGain( scon, Channel, nttr );
		nttr = (NoteTracker *)NextNode((Node *)nttr);
	}
	return 0;
}

/******************************************************************
** Process a Control Change Message
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/changescorecontrol
 |||	ChangeScoreControl - Changes a MIDI control value for a channel.
 |||
 |||	  Synopsis
 |||
 |||	    Err ChangeScoreControl( ScoreContext *ScoreCon, int32
 |||	                            Channel, int32 Index, int32 Value )
 |||
 |||	  Description
 |||
 |||	    This procedure changes a specified MIDI control value for notes played on
 |||	    the specified channel.  It's the equivalent of a MIDI control message,
 |||	    currently limited to control values of 7 (channel volume control) and 10
 |||	    (channel pan control).
 |||
 |||	    The procedure determines the control to change through the value passed in
 |||	    the index argument.  It then assigns the value passed in the value
 |||	    argument to the control.  For channel volume, the value can range from 0
 |||	    to 127, with 0 as silence and 127 as maximum volume.  For channel panning,
 |||	    the value can range from 0 to 127, with 0 playing all the notes in the
 |||	    left of a stereo output and 127 playing all the notes in the right of a
 |||	    stereo output.
 |||
 |||	  Arguments
 |||
 |||	    ScoreCon                     Pointer to a ScoreContext data structure
 |||	                                 controlling playback.
 |||
 |||	    Channel                      The number of the MIDI channel for which to
 |||	                                 change the control.
 |||
 |||	    Index                        The number of the control to be changed
 |||	                                 (currently only 7 and 10 are recognized).
 |||
 |||	    Value                        A value from 0 to 127 used as the new
 |||	                                 control setting.
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
 |||	    ChangeProgramControl(), ChangeScorePitchBend(), InterpretMIDIMessage(),
 |||	    StartScoreNote(), StopScoreNote()
 |||
**/
int32 ChangeScoreControl( ScoreContext *scon, int32 Channel, int32 Index, int32 Value)
{
	int32 Result = 0;

DBUG(("ChangeScoreControl( 0x%x, %d, %d, %d)\n", scon,
		Channel, Index, Value));

	switch(Index)
	{
		case 7:   /* MIDI Channel Volume. */
			scon->scon_Channels[Channel].schn_Volume = Value << 7;
			UpdateChannelVolume( scon, Channel );
			break;

		case 10:   /* MIDI Channel Pan. */
			scon->scon_Channels[Channel].schn_Pan = Value << 7;
			UpdateChannelVolume( scon, Channel );
			break;
	}
	return Result;
}

/******************************************************************
** Process a Control Pitch Bend Message
******************************************************************/
/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/changescorepitchbend
 |||	ChangeScorePitchBend - Changes a channel's pitch bend value.
 |||
 |||	  Synopsis
 |||
 |||	    Err ChangeScorePitchBend( ScoreContext *scon,
 |||	                              int32 Channel, int32 Bend )
 |||
 |||	  Description
 |||
 |||	    This procedure changes the current MIDI pitch bend value used to alter the
 |||	    pitch values of all instruments in the channel.  The procedure is the
 |||	    equivalent of a MIDI Pitch Bend message.
 |||
 |||	    The Bend argument ranges from 0x0 to 0x3FFF.  A setting of 0x2000 means
 |||	    that instrument pitch values aren't bent up or down.  A setting of 0x0
 |||	    means that instrument pitches are bent as far down as possible.  A setting
 |||	    of 0x3FFF means that instrument pitches are bent as far up as possible.
 |||
 |||	    The pitch bend value supplied by ChangeScorePitchBend() works within a
 |||	    pitch bend range, which is contained in the score context.  If the range
 |||	    is 8 semitones, for example, the score voices can be bent up a maximum of
 |||	    8 semitones or down a minimum of 8 semitones.  You can set the pitch bend
 |||	    range using SetScoreBendRange() and read the current pitch bend range
 |||	    usingGetScoreBendRange().
 |||
 |||	    Note that ChangeScorePitchBend() only affects notes whose pitches are
 |||	    specified by MIDI note values (0127).  It doesn't affect the pitch of
 |||	    notes specified by frequency.
 |||
 |||	  Arguments
 |||
 |||	    ScoreCon                     Pointer to a ScoreContext data structure
 |||	                                 controlling playback.
 |||
 |||	    Channel                      The number of the MIDI channel for which to
 |||	                                 change the pitch bend value.
 |||
 |||	    Bend                         The new pitch bend setting (0x00x3FFF).
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V21.
 |||
 |||	  Associated Files
 |||
 |||	    score.h, music.lib
 |||
 |||	  Caveats
 |||
 |||	    Pitch can't be bent beyond the range of the instrument.
 |||
 |||	  See Also
 |||
 |||	    ChangeScoreControl(), ChangeScoreProgram(), ConvertPitchBend(),
 |||	    GetScoreBendRange(), InterpretMIDIMessage(), StartScoreNote(),
 |||	    SetScoreBendRange()
 |||
**/
Err ChangeScorePitchBend( ScoreContext *scon, int32 Channel, int32 Bend )
{
	ScoreChannel *schn;
	NoteTracker *nttr;
	frac16 BendFrac;
	int32 Result = 0;

DBUG(("ChangeScorePitchBend( 0x%x, %d, 0x%x)\n", scon,
		Channel, Bend));
	schn = &scon->scon_Channels[Channel];
	schn->schn_PitchBend = Bend;

	Result = ConvertPitchBend( Bend, scon->scon_BendRange, &BendFrac );

/* Update all notes on current channel. */
	nttr = (NoteTracker *)FirstNode( &schn->schn_NoteList );
	while (ISNODE(&schn->schn_NoteList, nttr))
	{
DBUG(("Bend 0x%x to 0x%x\n", nttr->nttr_Instrument, BendFrac));

		BendInstrumentPitch( nttr->nttr_Instrument, BendFrac );
		nttr = (NoteTracker *)NextNode((Node *)nttr);
	}
	return Result;
}

/*********************************************************************
** Play a note based on MIDI pitch.
*********************************************************************/

/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/noteonins
 |||	NoteOnIns - Turns on a note for an instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Err NoteOnIns( Item Instrument, int32 Note,
 |||	                   int32 Velocity )
 |||
 |||	  Description
 |||
 |||	    This procedure turns on a note for an instrument and specifies the pitch
 |||	    and velocity for the instrument to play the note.  The procedure does not
 |||	    use voice allocation for the note.  It's called by StartScoreNote()
 |||	    after StartScoreNote() has worked out voice allocation.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of the instrument.
 |||
 |||	    Note                         The MIDI pitch value of the note.
 |||
 |||	    Velocity                     The MIDI attack velocity value of the note.
 |||	                                 If attack velocity is 0, the specified note is
 |||	                                 released (identical behavior to the MIDI Note On
 |||	                                 event).
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
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
 |||	    NoteOffIns(), ReleaseScoreNote(), StartScoreNote()
 |||
**/
int32 NoteOnIns ( Item Instrument, int32 Note, int32 Velocity )
{
	int32 Result;

DBUG(("NoteOnIns (0x%x, 0x%x, 0x%x)\n", Instrument,  Note,  Velocity));

	if (Velocity)
	{
            /* Define Tags for StartInstrument */
        TagArg SamplerTags[] =
        {
            { AF_TAG_VELOCITY },
            { AF_TAG_PITCH },
            TAG_END
        };
		SamplerTags[0].ta_Arg = (void *) Velocity; /* %Q !!!!!!!CLIPS!!!!!!!! */
		SamplerTags[1].ta_Arg = (void *) Note;
DBUGNOTE(("NoteONIns: 0x%x = %d,%d\n", Instrument, Note, Velocity ));
		Result = StartInstrument( Instrument, SamplerTags );
	}
	else
	{
		Result = NoteOffIns( Instrument, Note, MIDI_DEFAULT_VELOCITY);      /* 940902: now uses default velocity for 0 velocity note on */
	}
DBUG(("NoteOnIns returns 0x%x\n",Result));
	if( Result < 0 )
	{
		ERR(("NoteOnIns: Result = 0x%x, Inst = 0x%x\n", Result, Instrument ));
	}

	return Result;
}

/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/noteoffins
 |||	NoteOffIns - Turns off a note played by an instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Err NoteOffIns( Item Instrument, int32 Note,
 |||	                    int32 Velocity )
 |||
 |||	  Description
 |||
 |||	    This procedure releases a note being played by an instrument and specifies
 |||	    the pitch and velocity for the instrument to release the note.  The
 |||	    procedure does not use voice allocation for the note.  It's called by
 |||	    ReleaseScoreNote() after ReleaseScoreNote() has worked out voice
 |||	    allocation.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of the instrument.
 |||
 |||	    Note                         The MIDI pitch value of the note.
 |||
 |||	    Velocity                     The MIDI release velocity value of the note.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Caveats
 |||
 |||	    At present, release velocity is ignored.
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
 |||	    NoteOnIns(), ReleaseScoreNote(), StartScoreNote()
 |||
**/
int32 NoteOffIns ( Item Instrument, int32 Note, int32 Velocity )
{
DBUGNOTE(("NoteOFFIns: 0x%x = %d,%d\n", Instrument, Note, Velocity ));
	return ReleaseInstrument( Instrument, NULL );
}

/*
    Force a note to stop.

    @@@ could publish and/or could become the kernel for an All Notes Off message.
*/
static int32 NoteStopIns ( Item Instrument, int32 Note )
{
DBUGNOTE(("NoteStopIns: 0x%x = %d\n", Instrument, Note ));
	return StopInstrument( Instrument, NULL );
}

/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/initscoremixer
 |||	InitScoreMixer - Creates and initializes a mixer instrument for MIDI score
 |||	                 playback.
 |||
 |||	  Synopsis
 |||
 |||	    Err InitScoreMixer( ScoreContext *scon, char *MixerName,
 |||	                        int32 MaxNumVoices, int32 Amplitude )
 |||
 |||	  Description
 |||
 |||	    This procedure creates a mixer instrument from the specified instrument
 |||	    template and writes the item number of that mixer into the score context
 |||	    so that notes played with that score context are fed through the mixer.
 |||	    The procedure uses the maximum number of voices specified to create an
 |||	    appropriate number of note trackers to handle MIDI note playback.  The
 |||	    procedure also uses the amplitude value as the maximum possible amplitude
 |||	    allowed for each voice in the score.
 |||
 |||	    Note that a task should first allocate system amplitude to itself.  It can
 |||	    then divide that amplitude up by the number of voices to arrive at a
 |||	    completely safe amplitude value (one that won't drive the DSP to
 |||	    distortion) for this call.  Because it's unlikely that all voices will
 |||	    play simultaneously at full amplitude, a task can typically raise
 |||	    amplitude levels above the level considered to be completely safe.
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to a ScoreContext data structure.
 |||
 |||	    MixerName                    Pointer to a character string containing the
 |||	                                 name of a mixer instrument template.
 |||
 |||	    MaxNumVoices                 Value indicating the maximum number of
 |||	                                 voices used for the score.
 |||
 |||	    Amplitude                    Value indicating the maximum volume for each
 |||	                                 voice in the score.
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
 |||	    ChangeScoreProgram(), InitScoreDynamics(),
 |||	    TermScoreMixer(), ChangeScoreContext()
 |||
**/
int32 InitScoreMixer( ScoreContext *scon, char *MixerName, int32 MaxNumVoices, int32 Amplitude )
{
	int32 Result, i;

	if (scon->scon_MaxVoices <= 0)
	{
		Result = InitScoreDynamics( scon, MaxNumVoices );
		CHECKRESULT(Result,"InitScoreMixer: InitScoreDynamics");
	}

	scon->scon_MixerIns = LoadInstrument( MixerName, 0, 180);
	CHECKRESULT(scon->scon_MixerIns,"InitScoreMixer: LoadInstrument");

/* Grab a knob for each voice. */
	for (i=0; i<MaxNumVoices; i++)
	{
		scon->scon_LeftGains[i] = GrabKnob( scon->scon_MixerIns, LeftNames[i] );
		CHECKRESULT(scon->scon_LeftGains[i],"GrabKnob");
		scon->scon_RightGains[i] = GrabKnob( scon->scon_MixerIns, RightNames[i] );
		CHECKRESULT(scon->scon_RightGains[i],"GrabKnob");
	}

/* Do for each MIDI channel. */
	scon->scon_MaxVolume = Amplitude;
	for (i=0; i<NUMMIDICHANNELS; i++)
	{
		UpdateChannelVolume( scon, i );
	}

/* Mixer must be started */
	Result = StartInstrument( scon->scon_MixerIns, NULL );

cleanup:
	return Result;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/score/termscoremixer
 |||	TermScoreMixer - Dispose of mixer created by InitTermMixer().
 |||
 |||	  Synopsis
 |||
 |||	    Err TermScoreMixer( ScoreContext *scon )
 |||
 |||	  Description
 |||
 |||	    This function unloads the mixer loaded by InitScoreMixer(). This
 |||	    function is automatically called by DeleteScorePlayer().
 |||
 |||	    Calling this function multiple times has no harmful effect.
 |||
 |||	  Arguments
 |||
 |||	    scon                         Pointer to a ScoreContext data structure.
 |||	                                 The mixer need not have been successfully
 |||	                                 initialized.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns a non-negative value if successful or an
 |||	    error code (a negative value) if an error occurs.
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
 |||	    InitScoreMixer(), DeleteScoreContext()
 |||
 **/
int32 TermScoreMixer( ScoreContext *scon )
{
	int32 Result=0, i;

	if (scon->scon_MixerIns)
	{
		for(i=0; i<MAXSCOREVOICES; i++)
		{
			ReleaseKnob( scon->scon_LeftGains[i]);
			ReleaseKnob( scon->scon_RightGains[i]);
		}
		Result = UnloadInstrument( scon->scon_MixerIns );
		scon->scon_MixerIns = 0;
	}

	return Result;
}
