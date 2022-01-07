/* $Id: audio_envelopes.c,v 1.34 1994/12/23 20:04:51 phil Exp $ */
/****************************************************************
**
** Audio Internals to support Envelopes
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************
** 930804 PLB make KnobName bigger to avoid going past end.
** 930807 PLB added AF_TAG_RELEASEJUMP
** 930830 PLB Check for number of points in envelope.
** 931117 PLB Send signal for MonitorAttachment() for envelopes.
** 940505 PLB Delay at least 1 audio timer tick between envelope segments.
**            This was added after fixing the audio timer event handler on 940413.
**            The timer fix had a side effect of causing all events for a given time
**            to be processed, even if they were added to the list as a result
**            of processing another event at that time.  Before the fix, the event
**            would be delayed until the next tick.  The result was that envelopes
**            with very short attacks (1 msec) would not have a chance to rise.
**            This affected the laser sound for ShockWave.  It now works as before. 
** 940602 WJB Moved default error code set to just after afi_IsRamAddr() to avoid
**            being clobbered by same.
** 940616 PLB COmmented out DebugEnvelope
** 940707 PLB Use default name "Env" if NULL name passed.
** 940811 PLB Used %.4s to print ChunkTypes instead of scratch array kludge.
** 941224 PLB Take DSP instrument execution rate into account for envelope timing.
****************************************************************/

/****************
Envelope design:

X- Need signal driven event buffer.
Insertion sort events into pending list.

Internal Event Structure
	Node minimal
	Time to Execute
	Execution Vector
	User Data....
	
Envelope Attachment contains single node used for pending events.
	Allows simple removal from pending list if deleted.
	
Envelope Service:
	Do normal audio timer stuff then,
	Check Env Time
	Update EnvPhinc before ReqTarget because we should be near end of
	segment anyway and it will just clip if much bigger.
	Signal Envelope Servicer

May need to update in interrupt cuz signal latency could really mess up timing.

Attachment points to "class" method dispatcher for Start/Release/Stop/Update

Need to update EI via interrupt, calculate next and queue for interrupt service.
Envelope calculation:
	Delta = NewTarget-OldTarget
	EnvPhinc = (Delta * 32768) / (dT(sec) * SamplePerSecond) rounded up one
	Calculate actual NewTarget based on EnvPhinc
	Calc time to update
	
How to handle level segments when we don't quite get there on last seg,
	minimum update time.

Nasty Issues:
    How do we handle Attachment Names when envelopes use two knobs?
         SVFENV.target    user passes SVFENV
         SVFENV.phinc
         
*******************/

#include "audio_internal.h"
#include "filefunctions.h"

/* Macros for debugging. */
#define DBUG(x)    /* PRT(x) */
#define NODBUG(x)  /* */

#define CHECKMEM(msg) /* { PRT(msg); ReportMemoryUsage(); } */

/*****************************************************************/
/***** Envelope Loader********************************************/
/*****************************************************************/
/*****************************************************************/
void DefaultEnvelope ( AudioEnvelope *aenv)
{
/* Setup Envelope field defaults. */
	aenv->aenv_Points = NULL;
	aenv->aenv_NumPoints = 0;
	aenv->aenv_SustainBegin = -1;
	aenv->aenv_SustainEnd = -1;
	aenv->aenv_SustainTime = 0;
	aenv->aenv_ReleaseJump = -1;
	aenv->aenv_ReleaseBegin = -1;
	aenv->aenv_ReleaseEnd = -1;
	aenv->aenv_ReleaseTime = 0;
	aenv->aenv_MicrosPerUnit = 1000;
	aenv->aenv_Flags = 0;
}

/*****************************************************************/
#if 0
/* Only used for debugging. */
int32 DebugEnvelope( Item EnvelopeItem )
{
/* User level dump of Envelope contents. */
	AudioEnvelope *aenv;
	
	aenv = (AudioEnvelope *)CheckItem(EnvelopeItem,  AUDIONODE, AUDIO_ENVELOPE_NODE);
	if (aenv == NULL) return AF_ERR_BADITEM;
	return internalDumpEnvelope (aenv);
}

/*****************************************************************/
#define REPORT_ENVELOPE(name,member) PRT(("Envelope.%s = 0x%x\n", name, aenv->member));
int32 internalDumpEnvelope ( AudioEnvelope *aenv )
{
	PRT(("--------------------------\nEnvelope Structure Dump\n"));
    REPORT_ENVELOPE("aenv_Points", aenv_Points);
    REPORT_ENVELOPE("aenv_NumPoints", aenv_NumPoints);
    REPORT_ENVELOPE("aenv_SustainBegin", aenv_SustainBegin);
    REPORT_ENVELOPE("aenv_SustainEnd", aenv_SustainEnd);
    REPORT_ENVELOPE("aenv_SustainTime", aenv_SustainTime);
    REPORT_ENVELOPE("aenv_ReleaseJump", aenv_ReleaseJump);
    REPORT_ENVELOPE("aenv_ReleaseBegin", aenv_ReleaseBegin);
    REPORT_ENVELOPE("aenv_ReleaseEnd", aenv_ReleaseEnd);
    REPORT_ENVELOPE("aenv_ReleaseTime", aenv_ReleaseTime);
    REPORT_ENVELOPE("aenv_MicrosPerUnit", aenv_MicrosPerUnit);
    REPORT_ENVELOPE("aenv_Flags", aenv_Flags);
	return 0;
}
#endif

/*****************************************************************/
/* Load Envelope file and fill fields */

typedef struct
{
	AudioEnvelope *lsc_Envelope;
} LoadEnvelopeContext;

#if 0
/*****************************************************************/
int32 LoadENVLEnvelope (AudioEnvelope *aenv, char *filename)
{
	iff_control iffcb;
	LoadEnvelopeContext LSContext;
	int32 Result;
	
TRACEE(TRACE_INT, TRACE_ENVELOPE, ("LoadENVLEnvelope( 0x%x, %s )\n",
	aenv, filename));
	
/* Setup Parser. */
	iffcb.iffc_ChunkHandler = 0;
	iffcb.iffc_FormHandler = ParseENVLForm;
	LSContext.lsc_Envelope = aenv;
	iffcb.iffc_UserContext = (void *) &LSContext;
	iffcb.iffc_LastChanceDir = AudioBase->af_StartupDirectory;

	Result = (int32) iffParseFile(&iffcb, filename);
TRACEB(TRACE_TOP, TRACE_TOP, ("Done\n"));		
	if (Result < 0)
	{
		ERR(("LoadENVLEnvelope: Could not parse file = %s\n", filename));
	}
	return Result;
}
#endif

/******************************************************************/
/* Make Envelope Item, return in ATSMParserContext context. ********/
/******************************************************************/
int32 HandleATNVForm ( iff_control *iffc, uint32 FormType , uint32 FormSize )
{
	int32 Result = 0;
	TagArg Tags[14];
	LoadEnvelopeContext LEContext;
	TemplateParserContext *tmplpc;
	AudioEnvelope AEnvelope;
	int32 ti;
	int32 (*OldHandler)();
	void *OldContext;
	
	tmplpc = (TemplateParserContext *) iffc->iffc_UserContext;

DBUG(("HandleATNVForm: %.4s, %d\n",  &FormType, FormSize));
DBUG(("HandleATNVForm: iffc->iffc_ChunkHandler = 0x%x\n", iffc->iffc_ChunkHandler));

/* Setup Parser. */
	OldHandler = iffc->iffc_ChunkHandler;
	iffc->iffc_ChunkHandler = HandleENVLChunk;
	
	LEContext.lsc_Envelope = &AEnvelope;
	OldContext = iffc->iffc_UserContext;
	iffc->iffc_UserContext = &LEContext;

	DefaultEnvelope ( &AEnvelope );
	 
	Result = ParseENVLForm( iffc, FormType, FormSize );

	ti = 0;
	Tags[ti].ta_Tag = AF_TAG_ADDRESS;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_Points;
	Tags[ti].ta_Tag = AF_TAG_FRAMES;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_NumPoints;
	Tags[ti].ta_Tag = AF_TAG_SUSTAINBEGIN;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_SustainBegin;
	Tags[ti].ta_Tag = AF_TAG_SUSTAINEND;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_SustainEnd;
	Tags[ti].ta_Tag = AF_TAG_SUSTAINTIME;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_SustainTime;
	Tags[ti].ta_Tag = AF_TAG_RELEASEJUMP;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_ReleaseJump;
	Tags[ti].ta_Tag = AF_TAG_RELEASEBEGIN;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_ReleaseBegin;
	Tags[ti].ta_Tag = AF_TAG_RELEASEEND;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_ReleaseEnd;
	Tags[ti].ta_Tag = AF_TAG_RELEASETIME;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_ReleaseTime;
	Tags[ti].ta_Tag = AF_TAG_MICROSPERUNIT;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_MicrosPerUnit;
	Tags[ti].ta_Tag = AF_TAG_SET_FLAGS;
	Tags[ti++].ta_Arg = (void *) AEnvelope.aenv_Flags;
	Tags[ti].ta_Tag =  TAG_END;
/* Don't overflow!*/
    tmplpc->tmplpc_SlaveItem = CreateItem( MKNODEID(AUDIONODE,AUDIO_ENVELOPE_NODE), Tags );

/* restore iff parser */
	iffc->iffc_ChunkHandler = OldHandler;
	iffc->iffc_UserContext = OldContext;
	
	return Result;
}

/******************************************************************/
int32 HandleATNVChunk ( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize )
{
	int32 Result = 0;
	char *name;
	TemplateParserContext *tmplpc;
	
	tmplpc = (TemplateParserContext *) iffc->iffc_UserContext;
	
DBUG(("HandleATNVChunk: %.4s, %d, level = %d\n",  &ChunkType, ChunkSize, iffc->iffc_Level));

	switch(ChunkType)
	{
		case ID_HOOK:
			name = (char *) &tmplpc->tmplpc_HookName;
			if (ChunkSize < AF_MAX_NAME_SIZE)
			{
				Result = iffReadChunkData(iffc, name, ChunkSize);
				CHECKRSLT(("Error reading CHUNK data = %d\n", Result));
				name[ChunkSize] = '\0';
				DBUG(("Hook = %s\n", name));
			}
			else
			{
				ERR(("HandleATNVChunk: HOOK name too large = %d\n", ChunkSize));
				Result = AF_ERR_BADOFX;
			}
			break;
	}
	
error:
	return Result;
}


/*****************************************************************/
int32 ParseENVLForm( iff_control *iffc, int32 FormType, int32 FormSize )
{
	int32 Result;
	int32 (*OldHandler)();
	LoadEnvelopeContext *lsc;

	OldHandler = iffc->iffc_ChunkHandler;
	lsc = (LoadEnvelopeContext *) iffc->iffc_UserContext;
	
	iffc->iffc_ChunkHandler = HandleENVLChunk;
	
	Result = iffScanChunks( iffc, FormSize );
	iffc->iffc_ChunkHandler = OldHandler;
	CHECKRSLT(("HandleENVLForm: Error scanning Envelope = 0x%x\n", Result));
	
error:
	return Result;
}


/*****************************************************************/
int32 HandleENVLChunk( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize )
{
	int32 Result=0;
	AudioEnvelope *aenv;
	LoadEnvelopeContext *lsc;
	void *tmp;
	ENVH_Chunk *envh;
	
	lsc = (LoadEnvelopeContext *) iffc->iffc_UserContext;
	
	aenv = lsc->lsc_Envelope;
	
TRACEB(TRACE_INT, TRACE_ENVELOPE, ("HandleENVLChunk: Chunk: %.4s, %d\n", &ChunkType, ChunkSize));

	tmp = NULL;
	if (ChunkSize > 0)
	{
		tmp = EZMemAlloc(ChunkSize, MEMTYPE_ANY);
		if (tmp == NULL)
		{
			ERR(("Could not allocate Envelope memory.\n"));
			Result = AF_ERR_NOMEM;
			goto error;
		}
		Result = iffReadChunkData(iffc, tmp, ChunkSize);
		if (Result < 0)
		{
			ERR(("Error reading CHUNK data = 0x%x\n", Result));
			goto error;
		}
	}
	else
	{
		return 0;
	}
	
	switch(ChunkType)
	{
		case ID_PNTS:
			aenv->aenv_Points = (DataTimePair *) tmp;
DBUG(("Allocated Envelope points at 0x%x\n", aenv->aenv_Points ));
			aenv->aenv_Flags |= AF_ENVF_FOLIO_OWNS;
			break;
			
/* Read values from COMM chunk */
		case ID_ENVH:
			envh = (ENVH_Chunk *) tmp;
			aenv->aenv_NumPoints = envh->envh_NumPoints;
			aenv->aenv_SustainBegin = envh->envh_SustainBegin;
			aenv->aenv_SustainEnd = envh->envh_SustainEnd;
			aenv->aenv_SustainTime = envh->envh_SustainTime;
			aenv->aenv_ReleaseJump = envh->envh_ReleaseJump;
			aenv->aenv_ReleaseBegin = envh->envh_ReleaseBegin;
			aenv->aenv_ReleaseEnd = envh->envh_ReleaseEnd;
			aenv->aenv_ReleaseTime = envh->envh_ReleaseTime;
			aenv->aenv_MicrosPerUnit = envh->envh_MicrosPerDelta;
			aenv->aenv_Flags |= envh->envh_Flags;
			EZMemFree( tmp );
			break;
			
		default:
			if(tmp) EZMemFree( tmp );
			ERR(("Unrecognized chunk = 0x%lx\n", ChunkType ));
			
	} 
			
TRACEB(TRACE_INT, TRACE_ENVELOPE, ("HandleENVLChunk: %d bytes remaining.\n",iffc->iffc_length));

error:
	return Result;

}

/*****************************************************************/
/***** USER Level Folio Calls ************************************/
/*****************************************************************/

 /**
 |||	AUTODOC PUBLIC spg/items/envelope
 |||	Envelope - Audio envelope.
 |||
 |||	  Description
 |||
 |||	    An envelope is a time-variant control signal which can be used to control
 |||	    parameters of sounds that are to change over time (e.g. amplitude,
 |||	    frequency, filter characteristics, modulation amount, etc.).
 |||
 |||	    Envelope Items use a function defined by a set of points in time-level space
 |||	    described by an array of DataTimePairs (defined in audio.h). The function
 |||	    is a continuous set of line segments drawn between the points in the
 |||	    DataTimePair array.
 |||
 |||	    Envelopes are used in conjunction with Instruments that accept the Envelope
 |||	    Item's data and output the control signal (e.g. envelope.dsp).
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_ENVELOPE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateEnvelope()
 |||
 |||	    CreateItem()
 |||
 |||	  Delete
 |||
 |||	    DeleteEnvelope()
 |||
 |||	    DeleteItem()
 |||
 |||	  Query
 |||
 |||	    None
 |||
 |||	  Modify
 |||
 |||	    SetAudioItemInfo()
 |||
 |||	  Use
 |||
 |||	    AttachEnvelope()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_ADDRESS               (const DataTimePair *) Create, Modify.
 |||	                                 Pointer to array of DataTimePairs used to
 |||	                                 define the envelope. All of the points in this
 |||	                                 array must be sorted in increasing order by
 |||	                                 time. The length of the array is specified
 |||	                                 with AF_TAG_FRAMES.
 |||
 |||	    AF_TAG_CLEAR_FLAGS           (uint32) Create, Modify. Set of AF_ENVF_
 |||	                                 flags to clear. Clears every flag for which
 |||	                                 a 1 is set in ta_Arg.
 |||
 |||	    AF_TAG_FRAMES                (int32) Create, Modify. Number of
 |||	                                 DataTimePairs in array pointed to by
 |||	                                 AF_TAG_ADDRESS.
 |||
 |||	    AF_TAG_MICROSPERUNIT         (int32) Create, Modify. Number of
 |||	                                 microseconds for each time unit specified in
 |||	                                 DataTimePairs and time related envelope
 |||	                                 tags. Defaults to 1000 on creation, which
 |||	                                 sets each time unit equal to one
 |||	                                 millisecond.
 |||
 |||	    AF_TAG_RELEASEBEGIN          (int32) Create, Modify. Index in
 |||	                                 DataTimePair array for beginning of release
 |||	                                 loop. -1 indicates no loop, which is the
 |||	                                 default on creation. If not -1, must <= the
 |||	                                 value set by AF_TAG_RELEASEEND.
 |||
 |||	    AF_TAG_RELEASEEND            (int32) Create, Modify. Index in
 |||	                                 DataTimePair array for end of release loop.
 |||	                                 -1 indicates no loop, which is the default
 |||	                                 on creation. If not -1, must >= the value set
 |||	                                 by AF_TAG_RELEASEBEGIN.
 |||
 |||	    AF_TAG_RELEASEJUMP           (int32) Create, Modify. Index in
 |||	                                 DataTimePair array to jump to on release.
 |||	                                 When set, release causes escape from normal
 |||	                                 envelope processing to the specified index
 |||	                                 without disturbing the current output
 |||	                                 envelope value. From there, the envelope
 |||	                                 proceeds to the next DataTimePair from the
 |||	                                 current value. -1 to disable, which is the
 |||	                                 default on creation.
 |||
 |||	    AF_TAG_RELEASETIME           (int32) Create, Modify. The time in units
 |||	                                 used when looping from the end of the
 |||	                                 release loop back to the beginning. Defaults
 |||	                                 to 0 on creation.
 |||
 |||	    AF_TAG_SET_FLAGS             (uint32) Create, Modify. Set of AF_ENVF_
 |||	                                 flags to set. Sets every flag for which a 1
 |||	                                 is set in ta_Arg.
 |||
 |||	    AF_TAG_SUSTAINBEGIN          (int32) Create, Modify. Index in
 |||	                                 DataTimePair array for beginning of sustain
 |||	                                 loop. -1 indicates no loop, which is the
 |||	                                 default on creation. If not -1, <= the value
 |||	                                 set by AF_TAG_SUSTAINEND.
 |||
 |||	    AF_TAG_SUSTAINEND            (int32) Create, Modify. Index in
 |||	                                 DataTimePair array for end of sustain loop.
 |||	                                 -1 indicates no loop, which is the default
 |||	                                 on creation. If not -1, >= the value set
 |||	                                 by AF_TAG_SUSTAINBEGIN.
 |||
 |||	    AF_TAG_SUSTAINTIME           (int32) Create, Modify. The time in units
 |||	                                 used when looping from the end of the
 |||	                                 sustain loop back to the beginning. Defaults
 |||	                                 to 0 on creation.
 |||
 |||	  Flags
 |||
 |||	    AF_ENVF_FATLADYSINGS         The state of this flag indicates the default
 |||	                                 setting for the AF_ATTF_FATLADYSINGS flag
 |||	                                 whenever this envelope is attached to an
 |||	                                 instrument.
 |||
 |||	  See Also
 |||
 |||	    Attachment, Instrument, envelope.dsp
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/createenvelope
 |||	CreateEnvelope - Creates an envelope for a sample or cue.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateEnvelope (DataTimePair *Points, int32 Numpoints,
 |||	                         int32 SustainBegin, int32 SustainEnd)
 |||
 |||	  Description
 |||
 |||	    Creates an envelope for a sample or a cue. Envelopes are described by an
 |||	    array of pairs of time and data. It can be used to specify a loop for
 |||	    sustained sound.
 |||
 |||	    Data typically ranges from -32,768 to 32,767 or 0 to 65,535, and must be
 |||	    sign-extended to 32 bits. Loops are specified as point indices. For
 |||	    more information, read the description of envelopes in the Music
 |||	    Programmer's Guide.
 |||
 |||	    When you are finished with the envelope, you should call DeleteEnvelope()
 |||	    to delete it and deallocate its resources.
 |||
 |||	  Arguments
 |||
 |||	    Points                       An array of DataTimePair values giving delta
 |||	                                 time in milliseconds accompanied by data
 |||	                                 values. This becomes the value of the
 |||	                                 envelope item's AF_TAG_ADDRESS tag
 |||	                                 argument described in SetAudioItemInfo().
 |||	                                 The Points array is not copied: it must
 |||	                                 remain valid for the life of the Envelope
 |||	                                 Item or until it is replaced with another
 |||	                                 array of Points by a call to
 |||	                                 SetAudioItemInfo().
 |||
 |||	    NumPoints                    The number of points in the array. This
 |||	                                 becomes the value of the envelope item's
 |||	                                 AF_TAG_FRAMES tag argument.
 |||
 |||	    SustainBegin                 The beginning index of the loop. This
 |||	                                 becomes the value of the envelope item's
 |||	                                 AF_TAG_SUSTAINBEGIN tag argument. Set to
 |||	                                 -1 for no sustain loop.
 |||
 |||	    SustainEnd                   The ending index of the loop. This becomes
 |||	                                 the value of the envelope item's
 |||	                                 AF_TAG_SUSTAINEND tag argument. Set to
 |||	                                 -1 for no sustain loop. Otherwise must be
 |||	                                 >= SustainBegin.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the envelope (a positive value) or
 |||	    an error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DeleteEnvelope(), AttachEnvelope()
 |||
 **/
Item  CreateEnvelope( DataTimePair *Points, int32 NumPoints,
		int32 SustainBegin, int32 SustainEnd )
{
	Item Result;
	TagArg Tags[5];

TRACEE(TRACE_INT,TRACE_SAMPLE,("CreateEnvelope( 0x%x, %d )\n", Points, NumPoints));

	Tags[0].ta_Tag = AF_TAG_ADDRESS;
	Tags[0].ta_Arg = (void *) Points;
	Tags[1].ta_Tag = AF_TAG_FRAMES;
	Tags[1].ta_Arg = (void *) NumPoints;
	Tags[2].ta_Tag = AF_TAG_SUSTAINBEGIN;
	Tags[2].ta_Arg = (void *) SustainBegin;
	Tags[3].ta_Tag = AF_TAG_SUSTAINEND;
	Tags[3].ta_Arg = (void *) SustainEnd;
	Tags[4].ta_Tag =  0;
    Result = CreateItem( MKNODEID(AUDIONODE,AUDIO_ENVELOPE_NODE), Tags );
TRACER(TRACE_INT, TRACE_ENVELOPE, ("CreateEnvelope returns 0x%08x\n", Result));
	return Result;
}

/*****************************************************************/
int32 internalSetEnvelopeInfo (AudioEnvelope *aenv, TagArg *args)
{
  	int32 Result;  	
	uint32 tagc, *tagp;
	DataTimePair *Points;
	int32 NumPoints;
	int32 SustainBegin, SustainEnd, ReleaseBegin, ReleaseEnd, ReleaseJump;
	int32 Temp;
	
	Points = aenv->aenv_Points;
	NumPoints = aenv->aenv_NumPoints;
	SustainBegin = aenv->aenv_SustainBegin;
	SustainEnd = aenv->aenv_SustainEnd;
	ReleaseBegin = aenv->aenv_ReleaseBegin;
	ReleaseEnd = aenv->aenv_ReleaseEnd;
	ReleaseJump = aenv->aenv_ReleaseJump;
	
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("internalSetEnvelopeInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{
			case AF_TAG_ADDRESS:
				Points = (DataTimePair *) *tagp++;
				break;
			case AF_TAG_FRAMES:
				Temp =  *tagp++;
				if(Temp < 1)
				{
					ERR(("Too few points in envelope. %d\n", Temp));
					Result = AF_ERR_BADTAGVAL;
					goto DONE;
				}
				NumPoints = Temp;
				break;
			case AF_TAG_SUSTAINBEGIN:
				SustainBegin = *tagp++;
				break;
			case AF_TAG_SUSTAINEND:
				SustainEnd = *tagp++;
				break;
			case AF_TAG_SUSTAINTIME:
				aenv->aenv_SustainTime = *tagp++;
				break;
			case AF_TAG_RELEASEJUMP:
				ReleaseJump = *tagp++;
				break;
			case AF_TAG_RELEASEBEGIN:
				ReleaseBegin = *tagp++;
				break;
			case AF_TAG_RELEASEEND:
				ReleaseEnd = *tagp++;
				break;
			case AF_TAG_RELEASETIME:
				aenv->aenv_ReleaseTime = *tagp++;
				break;
			case AF_TAG_MICROSPERUNIT:
				aenv->aenv_MicrosPerUnit = *tagp++;
				break;
			case AF_TAG_SET_FLAGS:
				Temp =  *tagp++;
				if(Temp & ~AF_ENVF_LEGALFLAGS)
				{
					ERR(("Illegal envelope flags. 0x%x\n", Temp));
					Result = AF_ERR_BADTAGVAL;
					goto DONE;
				}
				aenv->aenv_Flags |= Temp;
				break;
			case AF_TAG_CLEAR_FLAGS:
				Temp =  *tagp++;
				if(Temp & ~AF_ENVF_LEGALFLAGS)
				{
					ERR(("Illegal envelope flags. 0x%x\n", Temp));
					Result = AF_ERR_BADTAGVAL;
					goto DONE;
				}
				aenv->aenv_Flags &= ~(Temp);
				break;
				
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Uunrecognized tag in internalSetEnvelopeInfo - 0x%x: 0x%x\n",
						tagc, *tagp));	
					Result = AF_ERR_BADTAG;
					goto DONE;
				}
			}
		}
	}
	
/*
** Validate Envelope data.
*/
	Result = afi_IsRamAddr( (char *) Points, NumPoints * sizeof(DataTimePair) );
	if(Result < 0)
	{
		ERR(("Envelope data address not in RAM"));
		goto DONE;
	}
	
	Result = AF_ERR_BADTAGVAL;  /* set default error (moved after afi_IsRamAddr() 940602) */
	
	aenv->aenv_Points = Points;
	aenv->aenv_NumPoints = NumPoints;
	
	if(SustainBegin != -1)
	{
		if((SustainBegin < 0) || (SustainBegin >= NumPoints))
		{
			ERR(("SustainBegin out of range. = %d\n", SustainBegin ));
			goto DONE;
		}
		if((SustainEnd < SustainBegin) || (SustainEnd > NumPoints))
		{
			ERR(("SustainEnd out of range. = %d\n", SustainEnd ));
			goto DONE;
		}
	}
	
	aenv->aenv_SustainBegin = SustainBegin;
	aenv->aenv_SustainEnd = SustainEnd;
	
	if(ReleaseBegin != -1)
	{
		if((ReleaseBegin < 0) || (ReleaseBegin >= NumPoints))
		{
			ERR(("ReleaseBegin out of range. = %d\n", ReleaseBegin ));
			goto DONE;
		}
		if((ReleaseEnd < aenv->aenv_SustainBegin) || (ReleaseEnd > NumPoints))
		{
			ERR(("ReleaseEnd out of range. = %d\n", ReleaseEnd ));
			goto DONE;
		}
	}
	
	aenv->aenv_ReleaseBegin = ReleaseBegin;
	aenv->aenv_ReleaseEnd = ReleaseEnd;
	
	if(ReleaseJump >= NumPoints)
	{
			ERR(("ReleaseJump out of range. = %d\n", ReleaseJump ));
			goto DONE;
	}
	
	aenv->aenv_ReleaseJump = ReleaseJump;

/* Set Frames per time Unit to allow fast envelope scaling.
** (Frames/Unit) = (Frames/Sec) * (MicroSecs/Unit) / (Micros/Sec) */
#define MICROS_PER_SECOND (1000000)
	Temp = DSPPData.dspp_SampleRate * aenv->aenv_MicrosPerUnit;
	aenv->aenv_FramesPerUnit = DivUF16( Temp, MICROS_PER_SECOND );
DBUG(("aenv->aenv_FramesPerUnit = 0x%X\n", aenv->aenv_FramesPerUnit));

/* Everything passed. */
	Result = 0;
	
DONE:
	return Result;
}

#define SuffixIncr    ".incr"
#define SuffixRequest ".request"
#define SuffixTarget  ".target"
#define SuffixCurrent ".current"

/*****************************************************************/
int32 DSPPAttachEnvelope( DSPPInstrument *dins, AudioEnvelope *aenv, AudioAttachment *aatt)
{
	int32 Result = 0;
	DSPPResource *drsc = NULL;
	char *EnvName, *KnobName = NULL;
	uint32 len;
	AudioEnvExtension *aeva;
	
TRACEE(TRACE_INT,TRACE_ENVELOPE,("DSPPAttachEnvelope ( dins=0x%lx, aenv=0x%lx)\n",
	dins, aenv));
	EnvName = aatt->aatt_HookName;
	
	if (EnvName)
	{
		TRACEE(TRACE_INT,TRACE_ENVELOPE,(" %s)\n", EnvName));
	}
	else
	{
/*		return AF_ERR_BADNAME;   940707 */
		EnvName = "Env";   /* 940707 Use default name "Env" if NULL name passed. */
	}

	aeva = (AudioEnvExtension *) aatt->aatt_Extension;
	
/* Look for resources with NAME.suffix */
	len = strlen(EnvName) + strlen(SuffixRequest);
	KnobName = (char *) EZMemAlloc( len+4, 0 );
	if( KnobName == NULL) return AF_ERR_NOMEM;
	
/* Get xxx.incr */
	strcpy( KnobName, EnvName );
	strcat( KnobName, SuffixIncr );
	drsc = DSPPFindResource( dins, DRSC_EI_MEM, KnobName );
	if (drsc == NULL)
	{
		Result = AF_ERR_BADNAME;
		goto DONE;
	}
	aeva->aeva_IncrEI = drsc->drsc_Allocated;
	
/* Get xxx.request */
	strcpy( KnobName, EnvName );
	strcat( KnobName, SuffixRequest );
	drsc = DSPPFindResource( dins, DRSC_EI_MEM, KnobName );
	if (drsc == NULL)
	{
		Result = AF_ERR_BADNAME;
		goto DONE;
	}
	aeva->aeva_RequestEI = drsc->drsc_Allocated;
	
/* Get xxx.target */
	strcpy( KnobName, EnvName );
	strcat( KnobName, SuffixTarget );
	drsc = DSPPFindResource( dins, DRSC_I_MEM, KnobName );
	if (drsc == NULL)
	{
		Result = AF_ERR_BADNAME;
		goto DONE;
	}
	aeva->aeva_TargetEI = drsc->drsc_Allocated;
	
/* Get xxx.current */
	strcpy( KnobName, EnvName );
	strcat( KnobName, SuffixCurrent );
	drsc = DSPPFindResource( dins, DRSC_I_MEM, KnobName );
	if (drsc == NULL)
	{
		Result = AF_ERR_BADNAME;
		goto DONE;
	}
	aeva->aeva_CurrentEI = drsc->drsc_Allocated;
	
TRACEB(TRACE_INT, TRACE_NOTE, ("DSPPAttachEnvelope: IncrEI =  0x%x, TargetEI = 0x%x\n",
	aeva->aeva_IncrEI, aeva->aeva_TargetEI));
	
	AddTail(&dins->dins_EnvelopeAttachments, (Node *)aatt);
	
/* Copy FATLADYSINGS bit from envelope. */
	if(aenv->aenv_Flags & AF_ENVF_FATLADYSINGS)
	{
		aatt->aatt_Flags |= AF_ATTF_FATLADYSINGS;
	}
	
DONE:
	if(KnobName) EZMemFree(KnobName);
	return Result;
}

/*****************************************************************/
void SetEnvAttTimeScale( AudioAttachment *aatt, ufrac16 TimeScale )
{
	AudioEnvelope *aenv;
	AudioEnvExtension *aeva;
	
	aenv = aatt->aatt_Structure;
	aeva = aatt->aatt_Extension;
	aeva->aeva_FramesPerUnit = MulUF16( aenv->aenv_FramesPerUnit, TimeScale );
	DBUG(("aeva->aeva_FramesPerUnit = 0x%x\n", aeva->aeva_FramesPerUnit));
}

#define ENV_PHASERANGE (32768)
#define ENV_MAXDELTA (400)
/******************************************************************
** On entry to this routine:
**    aeva->aeva_CurIndex = index of current point
******************************************************************/
int32 NextEnvSegment( AudioEnvExtension *aeva )
{
	AudioEnvelope *aenv;
	AudioAttachment *aatt;
	int32 PhaseIncrement;
	int32 Frames, Index;
	int32 Result = 0, PostIt;
	int32 ActualFrames, FramesLeft, DelayTime;
	int32 InterpTarget, Target;
	
	aatt = aeva->aeva_Parent;
	aenv = (AudioEnvelope *) aatt->aatt_Structure;
	Index = aeva->aeva_CurIndex;
	
	CHECKMEM(("Begin NextEnvSegment"));
	
	if(Index < aenv->aenv_NumPoints)
	{
/* Do we continue with a long segment or start a new one? */
		if( aeva->aeva_FramesLeft > 0 )
		{
			Frames = aeva->aeva_FramesLeft;
TRACEB(TRACE_INT,TRACE_ENVELOPE,("------\nNextEnvSegment: FramesLeft = %d )\n", Frames));
		}
		else
		{
			Frames = MulUF16(aeva->aeva_DeltaTime, aeva->aeva_FramesPerUnit);
TRACEB(TRACE_INT,TRACE_ENVELOPE,("==========\nNextEnvSegment: Delta = %d, Frames = %d )\n",
						aeva->aeva_DeltaTime, Frames));
		}
		Target = aenv->aenv_Points[Index].dtpr_Data;
TRACEB(TRACE_INT,TRACE_ENVELOPE,("NextEnvSegment: data[%d] = %d )\n", Index, Target));
		
/* Calculate actual number of frames we will take to reach 1.0 */

		if(Frames > ENV_PHASERANGE)
		{
			PhaseIncrement = 1;
		}
		else
		{
			if(Frames == 0) Frames = 1;
			PhaseIncrement = (ENV_PHASERANGE + (Frames - 1)) / Frames;
		}

/* We need to take into account the execution rate of the instrument. 941224 */
		{
			AudioInstrument *ains;
			DSPPInstrument *dins;

			ains = (AudioInstrument *) LookupItem(aatt->aatt_HostItem);
			if( ains == NULL ) return AF_ERR_BADITEM;
			dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
		
			PhaseIncrement = PhaseIncrement << dins->dins_RateShift;
			ActualFrames = (ENV_PHASERANGE / PhaseIncrement) << dins->dins_RateShift;
		}

DBUG(("\nNextEnvSegment: PhaseIncrement = %d, Frames = %d\n", PhaseIncrement, Frames ));
DBUG(("NextEnvSegment: ActualFrames = %d )\n", ActualFrames));

/* Do we break the remaining segment into pieces? */
		FramesLeft = Frames - ActualFrames;
		if( FramesLeft > ENV_MAXDELTA )
		{
/* Do segment in stages. */
			aeva->aeva_FramesLeft = FramesLeft;
/* Interpolate intermediate value. */
			InterpTarget = (((Target - aeva->aeva_PrevTarget) * ActualFrames) / Frames) +
					aeva->aeva_PrevTarget;
			Target = InterpTarget;
TRACEB(TRACE_INT,TRACE_ENVELOPE,("NextEnvSegment: InterpTarget = %d )\n", InterpTarget));
		}
		else
		{
/* Do it in one shot. */
			aeva->aeva_FramesLeft = 0;
			aeva->aeva_CurIndex++;
			aeva->aeva_DeltaTime = aenv->aenv_Points[aeva->aeva_CurIndex].dtpr_Time -
					aenv->aenv_Points[aeva->aeva_CurIndex-1].dtpr_Time;
		}
		aeva->aeva_PrevTarget = Target;
TRACEB(TRACE_INT,TRACE_ENVELOPE,("NextEnvSegment: IncrEI = %d, RequestEI = %d)\n", PhaseIncrement, Target ));
		dsphWriteEIMem( aeva->aeva_IncrEI, PhaseIncrement );
		dsphWriteEIMem( aeva->aeva_RequestEI, Target );
		
/* Decide whether to loop and whether to post or not. */
		PostIt = TRUE;
		if(aatt->aatt_ActivityLevel == AF_STARTED)
		{
			if(aenv->aenv_SustainBegin >= 0)
			{
				if(aeva->aeva_CurIndex > aenv->aenv_SustainEnd)
				{
					aeva->aeva_CurIndex = aenv->aenv_SustainBegin;  /* Loop back. */
					aeva->aeva_DeltaTime = aenv->aenv_SustainTime;
					if(aenv->aenv_SustainBegin == aenv->aenv_SustainEnd)
					{
						PostIt = FALSE;  /* Hold at single Sustain point. */
					}
				}
			}
		}
		if(aenv->aenv_ReleaseBegin >= 0)
		{
			if(aeva->aeva_CurIndex > aenv->aenv_ReleaseEnd)
			{
				aeva->aeva_CurIndex = aenv->aenv_ReleaseBegin;  /* Loop back. */
				aeva->aeva_DeltaTime = aenv->aenv_ReleaseTime;
			}
		}

		if(PostIt)
		{
TRACEB(TRACE_INT,TRACE_ENVELOPE,("NextEnvSegment: post at %d + %d\n", AudioBase->af_Time, (ActualFrames/AudioBase->af_Duration) ));
			DelayTime = ActualFrames / AudioBase->af_Duration;
			if( DelayTime < 1 ) DelayTime = 1;
			Result = PostAudioEvent( (AudioEvent *) aeva, AudioBase->af_Time + DelayTime);
		}
	}
	else
	{
/* Stop instrument if envelope finished and FATLADYSINGS bit set. */
		DBUG(("Env stopped, flags = 0x%x\n", aatt->aatt_Flags ));
		if( aatt->aatt_Flags & AF_ATTF_FATLADYSINGS )
		{
			DBUG(("Env stopped, FLS => StopIns\n" ));
			Result = swiStopInstrument( aatt->aatt_HostItem, NULL );
			if (Result < 0)
			{
				ERR(("NextEnvSegment: error stopping 0x%x\n", aatt->aatt_HostItem));
			}
		}

/* 931117 Signal task that called MonitorAttachment. */
		SignalMonitoringCue( aatt );
	
	}
	
	return Result;
}

/*****************************************************************/
int32 DSPPStartEnvAttachment( AudioAttachment *aatt )
{
	AudioEnvExtension *aeva;
	AudioEnvelope *aenv;
	int32 Result;
	
TRACEE(TRACE_INT,TRACE_ENVELOPE,("DSPPStartEnvAttachment( attt=0x%lx )\n", aatt));
	aeva = (AudioEnvExtension *) aatt->aatt_Extension;
	aenv = (AudioEnvelope *) aatt->aatt_Structure;
	
TRACEB(TRACE_INT,TRACE_ENVELOPE,("DSPPStartEnvAttachment: aeva = 0x%lx )\n", aeva));
/* set initial envelope value in DSP by . */
	dsphWriteIMem( aeva->aeva_CurrentEI, aenv->aenv_Points[0].dtpr_Data );
	dsphWriteIMem( aeva->aeva_TargetEI, aenv->aenv_Points[0].dtpr_Data-1 );
	aeva->aeva_CurIndex = 0;
	aeva->aeva_PrevTarget = 0;
	aeva->aeva_FramesLeft = 0;
	aeva->aeva_DeltaTime = 0;
	aeva->aeva_Event.aevt_Perform = NextEnvSegment;   /* Set callback function. */
	aatt->aatt_ActivityLevel = AF_STARTED;

	Result = NextEnvSegment( aeva );
TRACER(TRACE_INT,TRACE_ENVELOPE,("DSPPStartEnvAttachment returns 0x%lx )\n", Result));
	return Result;
}

/*****************************************************************
** Upon release:

** If stuck and no release => advance to next

** If looping and no release => noop, AF_RELEASED will break loop.
** If no sustain and no release => continue

** If stuck and yes release => advance to beginning of release
** If looping and yes release => unpost, advance to beginning of release
** If no sustain and yes release => unpost, advance to beginning of release
** How finish?
*/
int32 DSPPReleaseEnvAttachment( AudioAttachment *aatt )
{
	AudioEnvExtension *aeva;
	AudioEnvelope *aenv;
	int32 Result = 0;
	int32 IfRepost;
	
TRACEE(TRACE_INT,TRACE_ENVELOPE,("DSPPReleaseEnvAttachment ( attt=0x%lx )\n", aatt));
	
	aatt->aatt_ActivityLevel = AF_RELEASED;
	
	aeva = (AudioEnvExtension *) aatt->aatt_Extension;
	aenv = (AudioEnvelope *) aatt->aatt_Structure;
	IfRepost = FALSE;
	
/* Jump to release if a release jump is specified. */
	if(aenv->aenv_ReleaseJump > 0)
	{
DBUG(("DSPPReleaseEnvAttachment: Cur = %d, RJ = %d\n", aeva->aeva_CurIndex, aenv->aenv_ReleaseJump));

		if(aeva->aeva_CurIndex <= aenv->aenv_ReleaseJump)
		{
			aeva->aeva_CurIndex = aenv->aenv_ReleaseJump+1;
			IfRepost = TRUE;
		}
	}
	else if (aeva->aeva_Event.aevt_InList == NULL)
	{
/* Envelope is held at single Sustain point. */
		aeva->aeva_CurIndex++;
		IfRepost = TRUE;
	}
	
	if( IfRepost )
	{
		UnpostAudioEvent( (AudioEvent *) aeva );
		aeva->aeva_FramesLeft = 0;
		aeva->aeva_DeltaTime = aenv->aenv_Points[aeva->aeva_CurIndex].dtpr_Time -
				aenv->aenv_Points[aeva->aeva_CurIndex-1].dtpr_Time;
		Result = NextEnvSegment( aeva );
	}
	
	return Result;

}
/*****************************************************************/
int32 DSPPStopEnvAttachment( AudioAttachment *aatt )
{
	int32 Result;
	
TRACEE(TRACE_INT,TRACE_ENVELOPE,("DSPPStopEnvAttachment ( attt=0x%lx )\n", aatt));
	
	aatt->aatt_ActivityLevel = AF_STOPPED;
	Result = UnpostAudioEvent( (AudioEvent *) aatt->aatt_Extension );
	
	return Result;

}
/***************************************j*************************/
/***** Create Envelope Item for Folio ******************************/
/*****************************************************************/
Item internalCreateAudioEnvelope (AudioEnvelope *aenv, TagArg *args)
{
  	int32 Result;

TRACEE(TRACE_INT,TRACE_ITEM|TRACE_ENVELOPE,("internalCreateAudioEnvelope( 0x%x, 0x%x)\n", aenv, args));

    Result = TagProcessor( aenv, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioEnvelope: TagProcessor failed.\n"));
    	return Result;
    }

	DefaultEnvelope( aenv );

/* Parse remaining tags to allow overwriting file info. */
	Result = internalSetEnvelopeInfo(aenv, args);
	CHECKRSLT(("Bad Tag value in internalCreateAudioEnvelope\n"));

/* Init List of attachments. */
	InitList(&aenv->aenv_AttachmentRefs, "EnvelopeRefs");
	
/* Everything OK so pass back item. */
	Result = aenv->aenv_Item.n_Item;

error:
	return (Result);
}

/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/deleteenvelope
 |||	DeleteEnvelope - Deletes an envelope created by CreateEnvelope().
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteEnvelope (Item Envelope)
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the specified envelope, freeing its resources. It
 |||	    also deletes any attachments to the envelope. It does not delete the
 |||	    array of DataTimePairs used to define the envelope, which must be freed
 |||	    directly by the task that created it.
 |||
 |||	  Arguments
 |||
 |||	    Envelope                     Item number of the envelope to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    CreateEnvelope(), DetachEnvelope()
 |||
 **/
int32 DeleteEnvelope ( Item EnvelopeItem  )
{
	AudioEnvelope *aenv;
	
DBUG(("Deleting Envelope Item 0x%x\n", EnvelopeItem ));

	aenv = (AudioEnvelope *)CheckItem(EnvelopeItem, AUDIONODE, AUDIO_ENVELOPE_NODE);
	if (aenv == NULL) return AF_ERR_BADITEM;

	if(aenv->aenv_Points)
	{
		if(aenv->aenv_Flags & AF_ENVF_FOLIO_OWNS)
		{
DBUG(("Deleting Envelope points at 0x%x\n", aenv->aenv_Points ));
			EZMemFree( aenv->aenv_Points );
		}
	}
	
	return DeleteItem ( EnvelopeItem );
}

/**************************************************************/

int32 internalDeleteAudioEnvelope (AudioEnvelope *aenv)
{
	int32 Result;
	
TRACEE(TRACE_INT,TRACE_ITEM|TRACE_ENVELOPE,("internalDeleteAudioEnvelope( 0x%x )\n", aenv));
	Result = afi_DeleteReferencedItems( &aenv->aenv_AttachmentRefs );
	if( Result < 0) return Result;
	return 0;
}

/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/attachenvelope
 |||	AttachEnvelope - Attaches an envelope to an instrument and creates an
 |||	                 attachment item.
 |||
 |||	  Synopsis
 |||
 |||	    Item AttachEnvelope (Item Instrument, Item Envelope, char *EnvName)
 |||
 |||	  Description
 |||
 |||	    This procedure attaches the specified envelope to the specified instrument
 |||	    and creates an attachment item. The attachment point (the hook) must be
 |||	    specified by name, as instruments may have multiple places where envelopes
 |||	    can be attached.
 |||
 |||	    An instrument may, for example, have an amplitude envelope and a filter
 |||	    envelope, which may be called AmpEnv and FilterEnv respectively. The simple
 |||	    envelope.dsp instrument has a hook name of Env. You'll find hook names in
 |||	    the Instrument Templates chapter of this book. Note that if EnvName is set
 |||	    to NULL, the audio folio will look for "Env" as a default.
 |||
 |||	    You may use the same envelope for several instruments. The envelope data
 |||	    may be edited at any time, but be aware that it is read by a high priority
 |||	    task in the audio folio. Thus you should not leave it in a potentially
 |||	    "goofy" state if it is actively used.
 |||
 |||	    When you finish, you should call DetachEnvelope() to delete the attachment
 |||	    and free its resources.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of the instrument to which to
 |||	                                 attach the Envelope.
 |||
 |||	    Envelope                     The item number of the envelope to be attached.
 |||
 |||	    EnvName                      The name of the attachment point (hook).
 |||	                                 Can be NULL, in which case an Envelope Hook
 |||	                                 named "Env" is used.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the Attachment created (a positive
 |||	    value) or an error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  Caveat
 |||
 |||	    Prior to V24, EnvName == NULL was not supported.
 |||
 |||	  See Also
 |||
 |||	    DetachEnvelope(), CreateEnvelope(), envelope.dsp
 |||
 **/
Item AttachEnvelope( Item Instrument, Item Envelope, char *FIFOName )
{
	Item Result;
	TagArg Tags[4];

TRACEE(TRACE_INT,TRACE_ENVELOPE,("AttachEnvelope( 0x%x, 0x%x, %s )\n", Instrument,
		Envelope, FIFOName));

	Tags[0].ta_Tag = AF_TAG_INSTRUMENT;
	Tags[0].ta_Arg = (void *) Instrument;
	Tags[1].ta_Tag = AF_TAG_ENVELOPE;
	Tags[1].ta_Arg = (void *) Envelope;
	if (FIFOName == NULL)
	{
		Tags[2].ta_Tag =  TAG_END;
	}
	else
	{
		Tags[2].ta_Tag = AF_TAG_HOOKNAME;
		Tags[2].ta_Arg = (void *) FIFOName;
		Tags[3].ta_Tag =  TAG_END;
	}
	
    Result = CreateItem( MKNODEID(AUDIONODE,AUDIO_ATTACHMENT_NODE), Tags );
TRACER(TRACE_INT, TRACE_ENVELOPE, ("AttachEnvelope returns 0x%08x\n", Result));
	return Result;
}

/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/detachenvelope
 |||	DetachEnvelope - Detaches an envelope from an instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Err DetachEnvelope (Item Attachment)
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the specified Attachment between an Envelope and an
 |||	    Instrument created by AttachEnvelope(). The Envelope and Instrument are not
 |||	    deleted by this function.
 |||
 |||	  Arguments
 |||
 |||	    Attachment                   Item number of the Attachment to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DeleteEnvelope(), AttachEnvelope()
 |||
 **/
int32 DetachEnvelope( Item Attachment )
{
	return DeleteItem( Attachment );
}
