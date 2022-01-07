/* $Id: audio_lib.c,v 1.54 1994/11/17 04:39:11 phil Exp $ */
/****************************************************************
**
** Stub routines to interface to the Audio folio
** This file is linked with the users application.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

/****************************************************************
** 921201 PLB Added LoadInstrument
** 921208 PLB New TRACE, Add DSP routines
** 921209 PLB Name is now "audio"
** 930812 PLB make OpenAudioFolio compatible with future OpenItem scheme
** 930812 PLB Change GetAudioTime to a folio call.
** 931024 PLB Don't clear AudioFolioItem in CloseAudioFolio
** 931129 PLB Added Convert12TET_F16
** 940407 PLB Moved static TagLists into routines to prevent collision.
** 940427 PLB Fixed static TagLists in DSPReadEO.
** 940506 PLB Added GetAudioFolioInfo()
** 940713 WJB Added varargs glue functions and macros to build them.
** 940802 WJB Revised OpenAudioFolio() for demand loading.
** 940805 WJB Fixed wording of error message in OpenAudioFolio().
** 940913 WJB Revised to use varargs_glue.h.
** 940913 WJB Fixed includes.
** 940913 WJB Fixed typo: SetAutioItemInfoVA to SetAudioItemInfoVA.
** 940914 WJB Made varargs user folio glue functions call folio directly
**            rather that go thru another layer of glue.
** 940920 WJB Added version string.
** 941006 WJB Added SuperSetAudioFolioInfoVA().
** 941010 WJB Replaced arm assembly glue for SuperSetAudioFolioInfo() w/ C glue.
** 941011 WJB Removed FindAudioDevice(), ControlAudioDevice(), GetNumInstruments(),
**            GetMasterTuning(), and DSPGetMaxTicks().
** 941018 WJB Added SendAttachmentVA().
** 941116 WJB Removed SendAttachmentVA().
****************************************************************/

/* get rid of argument warnings on printf style routines */
#pragma -v0

#include <audio.h>
#include <folio.h>              /* FindAndOpenFolio() */
#include <item.h>               /* LookupItem() */
#include <varargs_glue.h>       /* VA function glue */

#define USERONLY
#include <audio_internal.h>

#define DBUG(x) /* PRT(x) */

#define TRACEDBG(x) /* PRT(x) */


/* -------------------- Version stuff */

#ifdef DEVELOPMENT
/*
    Pull in version string for the link library so that applications
    linked with this lib have this version string embedded in their
    executable.

    @@@ This works today (940920) but could get optimized out by a smarter
        compiler.
*/
extern const char audiolib_version[];
static const char *versionref = audiolib_version;
#endif


/* -------------------- Global variables */

Item AudioFolioItem;
AudioFolio *AudioBase;

/**********************************************************************/

int32 OpenAudioFolio (void)
{
    Item folio;

	if ((folio = FindAndOpenFolio (AUDIOFOLIONAME)) < 0)
	{
    	PrintError (NULL, "open audio folio", NULL, folio);
		return folio;
	}

/*
** Set AudioFolioItem and AudioBase for values global to the application.
*/
    AudioFolioItem = folio;
	AudioBase      = (AudioFolio *)LookupItem (folio);
	DBUG (("AudioBase located at %lx\n", AudioBase));

	return 0;
}

/**********************************************************************/

int32 CloseAudioFolio (void)
{
/* Don't clear AudioFolioItem cuz each thread needs to call this. 931024 */
	return CloseItem(AudioFolioItem);
}

/************************************************************
* The following routines call into the Folio through the jump
* table in User mode.
************************************************************/

/* Call WITH return. */
Item LoadInstrument ( char *InsName, Item AudioDevice, uint8 Priority)
{
    Item it;
	TRACEDBG(("LoadInstrument( %s, 0x%x, %d, ", InsName, AudioDevice, Priority ));
    CALLFOLIORET (AudioBase, LOADINSTRUMENT,
    	(InsName, AudioDevice, Priority), it, (Item));
	TRACEDBG(("LoadInstrument returns $%x\n", it));
    return it;
}
int32 UnloadInstrument ( Item Instrument  )
{
    int32 ret;
    CALLFOLIORET (AudioBase, UNLOADINSTRUMENT, (Instrument), ret, (int32));
    return ret;
}

Item LoadInsTemplate ( char *InsName, Item AudioDevice)
{
    Item it;
	TRACEDBG(("LoadInsTemplate( %s, --- )\n", InsName));
    CALLFOLIORET (AudioBase, LOADINSTEMPLATE, (InsName, AudioDevice), it, (Item));
    return it;
}

int32 UnloadInsTemplate ( Item TemplateItem  )
{
    int32 ret;
    CALLFOLIORET (AudioBase, UNLOADINSTEMPLATE, (TemplateItem), ret, (int32));
    return ret;
}

Item  DefineInsTemplate( uint8 *Definition, int32 NumBytes, Item AudioDevice, char *Name )
{
    Item it;
	TRACEDBG(("DefineInsTemplate( 0x%x, %d, 0x%x, --- )\n", Definition, NumBytes, AudioDevice ));
    CALLFOLIORET (AudioBase, DEFINEINSTEMPLATE, (Definition, NumBytes, AudioDevice, Name), it, (Item));
    return it;
}


/******************************************************************************/
Item LoadSample ( char *Name)
{
    Item it;
	TRACEDBG(("LoadSample( %s )\n", Name));
    CALLFOLIORET (AudioBase, LOADSAMPLE, (Name), it, (Item));
    return it;
}

Item LoadSampleHere ( char *Name, void *(*CustomAllocMem)(), void (*CustomFreeMem)())
{
    Item it;
	TRACEDBG(("LoadSampleHere( %s, 0x%x, 0x%x)\n", Name, CustomAllocMem, CustomFreeMem));
    CALLFOLIORET (AudioBase, LOADSAMPLEHERE, (Name, CustomAllocMem, CustomFreeMem), it, (Item));
    return it;
}

Item  DefineSampleHere( uint8 *Definition, int32 NumBytes,  void *(*CustomAllocMem)(), void (*CustomFreeMem)() )
{
    Item it;
	TRACEDBG(("DefineSampleHere( 0x%x, %d, 0x%x, 0x%x )\n", Definition, NumBytes, CustomAllocMem, CustomFreeMem ));
    CALLFOLIORET (AudioBase, DEFINESAMPLEHERE, (Definition, NumBytes, CustomAllocMem, CustomFreeMem), it, (Item));
    return it;
}

Item MakeSample ( uint32 NumBytes ,  TagArg *TagList )
{
    Item it;
	TRACEDBG(("MakeSample( %d, 0x%lx )\n", NumBytes, TagList ));
    CALLFOLIORET (AudioBase, MAKESAMPLE, (NumBytes, TagList), it, (Item));
    return it;
}

Item CreateSample ( TagArg *TagList )
{
    Item it;
	TRACEDBG(("CreateSample( %d, 0x%lx )\n", TagList ));
    CALLFOLIORET (AudioBase, CREATESAMPLE, (TagList), it, (Item));
    return it;
}
FOLIOGLUE_VA_FUNC (CreateSampleVA, AudioBase, CREATESAMPLE, (VAGLUE_VA_TAGS), (VAGLUE_TAG_POINTER), Item)

Item CreateInsTemplate ( TagArg *TagList )
{
    Item it;
	TRACEDBG(("CreateInsTemplate( 0x%lx )\n", TagList ));
    CALLFOLIORET (AudioBase, CREATEINSTEMPLATE, (TagList), it, (Item));
    return it;
}
FOLIOGLUE_VA_FUNC (CreateInsTemplateVA, AudioBase, CREATEINSTEMPLATE, (VAGLUE_VA_TAGS), (VAGLUE_TAG_POINTER), Item)

Item CreateInstrument ( Item InsTemplate, const TagArg *TagList )
{
    Item it;
    TRACEDBG(("CreateInstrument( 0x%lx, 0x%lx )\n", InsTemplate, TagList ));
    CALLFOLIORET (AudioBase, CREATEINSTRUMENT, (InsTemplate, TagList), it, (Item));
    return it;
}
FOLIOGLUE_VA_FUNC (CreateInstrumentVA, AudioBase, CREATEINSTRUMENT,
    (Item InsTemplate, VAGLUE_VA_TAGS),
    (InsTemplate, VAGLUE_TAG_POINTER), Item)

FOLIOGLUE_FUNC (DeleteInstrument, AudioBase, DELETEINSTRUMENT, (Item Instrument), (Instrument), Err)

Item CreateProbe ( Item Instrument, const char *OutputName, const TagArg *TagList )
{
    Item it;
    TRACEDBG(("CreateProbe( 0x%lx, 0x%lx )\n", Instrument, TagList ));
    CALLFOLIORET (AudioBase, CREATEPROBE, (Instrument, OutputName, TagList), it, (Item));
    return it;
}
FOLIOGLUE_VA_FUNC (CreateProbeVA, AudioBase, CREATEPROBE,
    (Item Instrument, const char *OutputName, VAGLUE_VA_TAGS),
    (Instrument, OutputName, VAGLUE_TAG_POINTER), Item)

FOLIOGLUE_FUNC (DeleteProbe, AudioBase, DELETEPROBE, (Item Probe), (Probe), Err)

Item CreateDelayLine ( int32 NumBytes , int32 NumChannels, int32 IfLoop)
{
    Item it;
    CALLFOLIORET (AudioBase, CREATEDELAYLINE, (NumBytes , NumChannels, IfLoop), it, (Item));
    return it;
}

int32 DeleteDelayLine( Item DelayLIne )
{
    int32 ret;
    CALLFOLIORET (AudioBase, DELETEDELAYLINE, (DelayLIne), ret, (int32));
    return ret;
}

int32 UnloadSample (Item SampleItem)
{
    int32 ret;
    CALLFOLIORET (AudioBase, UNLOADSAMPLE, (SampleItem), ret, (int32));
    return ret;
}

Item ScanSample ( char *Name, int32 DataSize)
{
    Item it;
	TRACEDBG(("ScanSample( %s, 0x%x )\n", Name, DataSize));
    CALLFOLIORET (AudioBase, SCANSAMPLE, (Name, DataSize), it, (Item));
    return it;
}

int32 DebugSample (Item SampleItem)
{
    int32 ret;
    CALLFOLIORET (AudioBase, DEBUGSAMPLE, (SampleItem), ret, (int32));
    return ret;
}

Item  AttachSample( Item Instrument, Item Sample, char *FIFOName )
{
    Item it;
	TRACEDBG(("AttachSample( 0x%x, 0x%x, %s )\n", Instrument,  Sample, FIFOName));
    CALLFOLIORET (AudioBase, ATTACHSAMPLE, (Instrument,  Sample, FIFOName), it, (Item));
    return it;
}

/* Envelopes */
Item  AttachEnvelope( Item Instrument, Item Envelope, char *EnvName )
{
    Item it;
	TRACEDBG(("AttachEnvelope( 0x%x, 0x%x, %s )\n", Instrument,  Envelope, EnvName));
    CALLFOLIORET (AudioBase, ATTACHENVELOPE, (Instrument,  Envelope, EnvName), it, (Item));
    return it;
}

int32 DetachEnvelope( Item Attachment )
{
    int32 ret;
    CALLFOLIORET (AudioBase, DETACHENVELOPE, (Attachment), ret, (int32));
    return ret;
}
int32 DetachSample( Item Attachment )
{
    int32 ret;
    CALLFOLIORET (AudioBase, DETACHSAMPLE, (Attachment), ret, (int32));
    return ret;
}

Item  CreateEnvelope( DataTimePair *Points, int32 Numpoints,
		int32 SustainBegin, int32 SustainEnd )
{
    Item it;
	TRACEDBG(("CreateEnvelope( 0x%x, %d, %d, %d )\n", Points, Numpoints, SustainBegin, SustainEnd));
    CALLFOLIORET (AudioBase, CREATEENVELOPE, (Points, Numpoints, SustainBegin, SustainEnd), it, (Item));
    return it;
}

int32  DeleteEnvelope( Item Envelope )
{
    int32 ret;
    CALLFOLIORET (AudioBase, DELETEENVELOPE, (Envelope), ret, (int32));
    return ret;
}

Item AllocInstrument ( Item Template, uint8 Priority )
{
    Item it;
	TRACEDBG(("AllocInstrument( 0x%x, %d, %d )\n", Template, Priority));
    CALLFOLIORET (AudioBase, ALLOCINSTRUMENT,
    	(Template, Priority), it, (Item));
    return it;
}

Item GrabKnob ( Item Instrument, char *Name )
{
    Item it;
	TRACEDBG(("GrabKnob( 0x%x, %s)\n", Instrument, Name));
    CALLFOLIORET (AudioBase, GRABKNOB,
    	(Instrument, Name), it, (Item));
    return it;
}
char *GetKnobName(Item Instrument, int32 KnobNumber)
{
    char *name;
	TRACEDBG(("GetKnobName( 0x%x, %d)\n", Instrument, KnobNumber));
    CALLFOLIORET (AudioBase, GETKNOBNAME,
    	(Instrument, KnobNumber), name, (char *));
    return name;
}

int32 GetNumKnobs ( Item Instrument )
{
    int32 ret;
	TRACEDBG(("GetNumKnobs( 0x%x )\n", Instrument));
    CALLFOLIORET (AudioBase, GETNUMKNOBS,
    	(Instrument), ret, (int32));
	TRACEDBG(("GetNumKnobs returns %d\n", ret));
    return ret;
}

/**************************************************************/
/******* Audio Timer ******************************************/
/**************************************************************/

int32 SleepUntilTime( Item ATI, AudioTime Time)
{
    int32 ret;
	TRACEDBG(("SleepUntilTime( %d )\n", Time));
    CALLFOLIORET (AudioBase, SLEEPUNTILTIME,
    	(ATI, Time), ret, (int32));
    return ret;
}

int32 SleepAudioTicks( int32 Ticks )
{
    int32 ret;
	TRACEDBG(("SleepAudioTicks( %d )\n", Ticks));
    CALLFOLIORET (AudioBase, SLEEPAUDIOTICKS,
    	(Ticks), ret, (int32));
    return ret;
}

frac16 GetAudioRate( void )
{
    int32 ret;
    CALLFOLIORET (AudioBase, GETAUDIORATE,
    	( ), ret, (frac16));
    return ret;
}

uint32 GetAudioDuration( void )
{
    int32 ret;
    CALLFOLIORET (AudioBase, GETAUDIODURATION,
    	( ), ret, (uint32));
    return ret;
}

Item OwnAudioClock( void )
{
    int32 ret;
    CALLFOLIORET (AudioBase, OWNAUDIOCLOCK,
    	( ), ret, (Item));
    return ret;
}

int32 DisownAudioClock( Item Owner )
{
    int32 ret;
    CALLFOLIORET (AudioBase, DISOWNAUDIOCLOCK,
    	(Owner), ret, (int32));
    return ret;
}

AudioTime GetAudioTime ( void )
{
    int32 ret;
    CALLFOLIORET (AudioBase, GETAUDIOTIME, (), ret, (uint32));
    return ret;
}

int32 GetCueSignal( Item Cue )
{
    int32 ret;
    CALLFOLIORET (AudioBase, GETCUESIGNAL,
    	(Cue), ret, (int32));
    return ret;
}

Item  CreateTuning( ufrac16 *Frequencies, int32 NumNotes, int32 NotesPerOctave, int32 BaseNote )
{
    Item it;
	TRACEDBG(("CreateTuning( 0x%x, %d, %d, %d )\n", Frequencies, NumNotes, NotesPerOctave, BaseNote ));
    CALLFOLIORET (AudioBase, CREATETUNING, (Frequencies, NumNotes, NotesPerOctave, BaseNote), it, (Item));
    return it;
}

int32 DeleteTuning( Item Tuning )
{
    int32 ret;
    CALLFOLIORET (AudioBase, DELETETUNING, (Tuning), ret, (int32));
    return ret;
}

Err GetAudioItemInfo( Item AnyItem, TagArg *tp )
{
    Err ret;
    CALLFOLIORET (AudioBase, GETAUDIOITEMINFO, (AnyItem, tp), ret, (Err));
    return ret;
}

int32 Convert12TET_F16( int32 Semitones, int32 Cents, frac16 *FractionPtr )
{
    int32 ret;
    CALLFOLIORET (AudioBase, CONVERT12TET_F16, ( Semitones, Cents, FractionPtr ), ret, (int32));
    return ret;
}

Err GetAudioFolioInfo( TagArg *tp )
{
    Err ret;
    CALLFOLIORET (AudioBase, GETAUDIOFOLIOINFO, (tp), ret, (Err));
    return ret;
}

/***********************************************************/

/*
** Rewrite these routines to disable them or make them more M2 compatible.
** Since these are linked, we won't break any released Apps.
*/
#if 0
int32 DSPReadEO(int32 EO_Offset)
{
	int32 Result;
	TagArg ReadEOTags[2];

    /* @@@ could use local TestHackVA()? */
	ReadEOTags[0].ta_Tag = HACK_TAG_READ_EO;
	ReadEOTags[0].ta_Arg = (void *) EO_Offset;
/*	ReadEOTags[1].ta_Arg = TAG_END;  940427 WRONG! Should be like next line. */
	ReadEOTags[1].ta_Tag = TAG_END;
	Result = TestHack(&ReadEOTags[0]);
DBUG(("EO(%d) = 0x%x\n", EO_Offset, Result));
	return Result;
}

int32 DSPGetTicks ( void )
{
/* Read it from EO location 0 */
	int32 ticks;
	ticks = DSPReadEO(EO_BENCHMARK);
	return ticks;
}

#if 0   /* this one was never actually implemented */
int32 DSPGetMaxTicks ( void )
{
/* Read it from EO location 1 */
	int32 ticks;
	ticks = DSPReadEO(EO_MAXTICKS);
	return ticks;
}
#endif

void *DSPWhereDMA ( uint32 DMAChannel )
{
	TagArg DMATags[2];
	DMATags[0].ta_Tag = HACK_TAG_READ_DMA;
	DMATags[0].ta_Arg = (void *) DMAChannel;
	DMATags[1].ta_Tag = TAG_END;

    /* @@@ could use local TestHackVA()? */
	return (void *) TestHack( &DMATags[0] );
}

#else  /* replacement code */

int32 DSPReadEO(int32 EO_Offset)
{
	PRT(("DSPReadEO(%d) no longer supported! Use ReadProbe()\n", EO_Offset));
	return AF_ERR_UNIMPLEMENTED;
}

int32 DSPGetTicks ( void )
{
	return GetAudioCyclesUsed();
}

void *DSPWhereDMA ( uint32 DMAChannel )
{
	PRT(("DSPWhereDMA(%d) no longer supported! Use WhereAttachment()\n", DMAChannel));
	return (void *) AF_ERR_UNIMPLEMENTED;
}

#endif


int32 DSPGetRsrcAlloc (Item Instrument, int32 RsrcType, char *Name, int32 *Alloc)
{
    int32 ret;
    CALLFOLIORET (AudioBase, DSPGETRSRCALLOC,
    	(Instrument, RsrcType, Name, Alloc), ret, (int32));
    return ret;
}
int32 DSPGetTotalRsrcUsed( int32 RsrcType )
{
    int32 ret;
    CALLFOLIORET (AudioBase, DSPGETTOTALRSRCUSED,
    	(RsrcType), ret, (int32));
    return ret;
}

int32 DSPGetInsRsrcUsed(Item Instrument, int32 RsrcType)
{
    int32 ret;
    CALLFOLIORET (AudioBase, DSPGETINSRSRCUSED,
    	(Instrument, RsrcType), ret, (int32));
    return ret;
}

/* Implemented entirely in the Library */

int32 FreeInstrument ( Item Instrument  )
{
	int32 Result;
	TRACEDBG(("FreeInstrument( 0x%x )\n", Instrument));
	Result = DeleteItem ( Instrument );
	TRACEDBG(("FreeInstrument returns 0x%x\n", Result));
	return Result;
}

Err ReleaseKnob ( Item KnobItem  )
{
	TRACEDBG(("ReleaseKnob( 0x%x )\n", KnobItem));
	return DeleteItem ( KnobItem );
}


/* -------------------- Varargs functions for SWIs */

    /* SWIs */
VAGLUE_FUNC (Err, StartInstrumentVA   (Item Instrument, VAGLUE_VA_TAGS), StartInstrument   (Instrument, VAGLUE_TAG_POINTER))
VAGLUE_FUNC (Err, ReleaseInstrumentVA (Item Instrument, VAGLUE_VA_TAGS), ReleaseInstrument (Instrument, VAGLUE_TAG_POINTER))
VAGLUE_FUNC (Err, StopInstrumentVA    (Item Instrument, VAGLUE_VA_TAGS), StopInstrument    (Instrument, VAGLUE_TAG_POINTER))

VAGLUE_FUNC (Err, StartAttachmentVA   (Item Attachment, VAGLUE_VA_TAGS), StartAttachment   (Attachment, VAGLUE_TAG_POINTER))
VAGLUE_FUNC (Err, ReleaseAttachmentVA (Item Attachment, VAGLUE_VA_TAGS), ReleaseAttachment (Attachment, VAGLUE_TAG_POINTER))
VAGLUE_FUNC (Err, StopAttachmentVA    (Item Attachment, VAGLUE_VA_TAGS), StopAttachment    (Attachment, VAGLUE_TAG_POINTER))

VAGLUE_FUNC (Err, SetAudioItemInfoVA (Item AnyItem, VAGLUE_VA_TAGS), SetAudioItemInfo (AnyItem, VAGLUE_TAG_POINTER))

VAGLUE_FUNC (Err, EnableAudioInputVA (int32 OnOrOff, VAGLUE_VA_TAGS), EnableAudioInput (OnOrOff, VAGLUE_TAG_POINTER))
VAGLUE_FUNC (Err, SetAudioFolioInfoVA (VAGLUE_VA_TAGS), SetAudioFolioInfo (VAGLUE_TAG_POINTER))


/* -------------------- Supervisor entry points */

    /* SuperSetAudioFolioInfo() - supervisor entry point for SetAudioFolioInfo() */
FOLIOGLUE_SUPER_FUNC    (SuperSetAudioFolioInfo,   AudioBase, AF_SWI_SETAUDIOFOLIOINFO,
    (const TagArg *tags),
    (tags),
    Err)
FOLIOGLUE_SUPER_VA_FUNC (SuperSetAudioFolioInfoVA, AudioBase, AF_SWI_SETAUDIOFOLIOINFO,
    (VAGLUE_VA_TAGS),
    (VAGLUE_TAG_POINTER),
    Err)

