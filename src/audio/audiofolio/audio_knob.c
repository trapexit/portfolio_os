/* $Id: audio_knob.c,v 1.29 1994/12/01 05:41:46 phil Exp $ */
/***************************************************************
**
** Audio Knobs
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************
** 930816 PLB Removed call to CHECKAUDIOOPEN
** 930828 PLB Return proper error code for Bad Item in internalCreateAudioKnob,
**            TweakKnob and TweakRawKnob.
** 930830 PLB Started to implement AF_TAG_CURRENT for knobs.
** 931222 PLB Fully implemented AF_TAG_CURRENT for knobs.
**            Optimised by calling DSPPPutKnob() directly.
** 941024 PLB Removed GetKnob() which was dead code.
***************************************************************/

#include "audio_internal.h"

/* Macros for debugging. */
#define DBUG(x)   /* PRT(x) */

/******************************************************************/
/***** USER MODE **************************************************/
/******************************************************************/

 /**
 |||	AUTODOC PUBLIC spg/items/knob
 |||	Knob - An item for adjusting an Instrument's parameters.
 |||
 |||	  Description
 |||
 |||	    A Knob is an Item for adjusting an Instrument's parameters.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_KNOB_NODE
 |||
 |||	  Create
 |||
 |||	    CreateItem()
 |||
 |||	    GrabKnob()
 |||
 |||	  Delete
 |||
 |||	    DeleteItem()
 |||
 |||	    ReleaseKnob()
 |||
 |||	  Query
 |||
 |||	    GetAudioItemInfo()
 |||
 |||	  Modify
 |||
 |||	    None
 |||
 |||	  Use
 |||
 |||	    TweakKnob()
 |||
 |||	    TweakRawKnob()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_CURRENT               (int32) Query. Returns the current raw value
 |||	                                 of knob.
 |||
 |||	    AF_TAG_DEFAULT               (int32) Query. Returns the default raw value
 |||	                                 of knob.
 |||
 |||	    AF_TAG_INSTRUMENT            (Item) Create. Specifies from which
 |||	                                 instrument to grab knob.
 |||
 |||	    AF_TAG_MAX                   (int32) Query. Returns maximum raw value of
 |||	                                 knob.
 |||
 |||	    AF_TAG_MIN                   (int32) Query. Returns minimum raw value of
 |||	                                 knob.
 |||
 |||	    AF_TAG_NAME                  (const char *) Create, Query. Knob name.
 |||	                                 On creation, specifies name of knob to grab.
 |||	                                 On query, returns a pointer to knob's name.
 |||
 |||	  See Also
 |||
 |||	    Instrument, Probe
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/grabknob
 |||	GrabKnob - Gain direct access to one of an Instrument's knobs.
 |||
 |||	  Synopsis
 |||
 |||	    Item GrabKnob (Item Instrument, char *Name)
 |||
 |||	  Description
 |||
 |||	    This procedure creates a Knob item that provides a fast connection between
 |||	    a task and one of an Instrument's parameters. You can then call
 |||	    TweakKnob() or TweakRawKnob() to rapidly modify that parameter.
 |||
 |||	    See the Instrument Template pages for complete listings of each Instrument
 |||	    Templates knobs.
 |||
 |||	    Call ReleaseKnob() to relinquish access to this knob. All Knob Items
 |||	    grabbed for an Instrument are deleted when the Instrument is deleted.
 |||	    This can save you a bunch of calls to ReleaseKnob().
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of the instrument.
 |||
 |||	    Name                         The name of the knob to grab. The knob
 |||	                                 name is matched case-insensitively.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the Knob (a positive value) if
 |||	    successful or an error code (a negative value) if an error occurs.
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
 |||	    ReleaseKnob(), TweakKnob(), TweakRawKnob(), GetKnobName(), GetNumKnobs(),
 |||	    Knob
 |||
 **/
Item GrabKnob ( Item Instrument, char *Name )
{
	TagArg Tags[3];
	
	Tags[0].ta_Tag = AF_TAG_NAME;
	Tags[0].ta_Arg = (void *) Name;
	
	Tags[1].ta_Tag = AF_TAG_INSTRUMENT;
	Tags[1].ta_Arg = (void *) Instrument;
	
	Tags[2].ta_Tag = TAG_END;
	
    return CreateItem( MKNODEID(AUDIONODE,AUDIO_KNOB_NODE), Tags );
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/releaseknob
 |||	ReleaseKnob - Releases a knob grabbed with GrabKnob().
 |||
 |||	  Synopsis
 |||
 |||	    Err ReleaseKnob (Item KnobItem)
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the Knob item created by GrabKnob(), freeing its
 |||	    resource and disconnecting the task that created the Knob item from the
 |||	    instrument containing the Knob.
 |||
 |||	  Arguments
 |||
 |||	    KnobItem                     Item number of knob.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in audio.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    GrabKnob()
 |||
 **/
 
/******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getknobname
 |||	GetKnobName - Queries name of one of an Instrument's knob.
 |||
 |||	  Synopsis
 |||
 |||	    char *GetKnobName (Item Instrument, int32 KnobNumber)
 |||
 |||	  Description
 |||
 |||	    This procedure returns the name of one of the Knobs belonging to an
 |||	    Instrument. The Knob is selected by specifying an index between
 |||	    0 and NKnobs-1, where NKnobs is the # of Knobs the instrument has
 |||	    (see GetNumKnobs()). The name of the Knob can be used to grab that Knob.
 |||	    This function is useful if you don't already know the names of the Knobs
 |||	    belonging to an instrument (see the example ta_faders.c or patchdemo.c
 |||	    for example usage).
 |||
 |||	    If you already have a Knob Item, you can find out its name with
 |||	    GetAudioItemInfo().
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   Item number of the instrument.
 |||
 |||	    KnobNumber                   Index number of a knob. Must be between
 |||	                                 0 and one less than the result of
 |||	                                 GetNumKnobs() for this instrument.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a pointer to the name of the specified knob if
 |||	    successful or NULL if an error occurs. Note that this is not a memory
 |||	    allocation, but a pointer to system memory. You can read it, but you can't
 |||	    write it, free it, or change it in any way. This pointer becomes invalid
 |||	    after the Instrument's Template has been deleted.
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
 |||	    GetNumKnobs(), GrabKnob(), Knob
 |||
 **/
char *GetKnobName(Item Instrument, int32 KnobNumber)
{
/* Returns NULL if either input invalid */
	int32 i;
	DSPPKnob *dknb, *base;
	AudioInstrument *ains;
	DSPPInstrument *dins;
	
	ains = (AudioInstrument *)CheckItem(Instrument, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return NULL;
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	
	dknb = dins->dins_Template->dtmp_Knobs;
	base = dknb;
	for (i=0; i<KnobNumber; i++)
	{
		dknb = DSPPNextKnob(base, dknb);
		if (dknb == NULL) return NULL;
	}
	return dknb->dknb_Name;
}

/******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/getnumknobs
 |||	GetNumKnobs - Finds how many knobs an Instrument has.
 |||
 |||	  Synopsis
 |||
 |||	    int32 GetNumKnobs (Item Instrument)
 |||
 |||	  Description
 |||
 |||	    This procedure indicates how many knobs an instrument has.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of the instrument.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the number of knobs if successful (a non-negative
 |||	    value: 0 means no knobs) or an error code (a negative value) if an error
 |||	    occurs.
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
 |||	    GetKnobName(), GrabKnob(), Knob
 |||
 **/
int32 GetNumKnobs ( Item Instrument )
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	
	ains = (AudioInstrument *)CheckItem(Instrument, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	return (dins->dins_Template->dtmp_NumKnobs);
}

/******************************************************************/
int32 internalGetKnobInfo ( AudioKnob *aknob, TagArg *args )
{
	int32 result=0;
	DSPPKnob *dknb;
	uint32 tagc, *tagp;
  	
TRACEE(TRACE_INT,TRACE_SAMPLE,("internalGetKnobInfo( 0x%x, 0x%x )\n", aknob, args ));

	dknb = (DSPPKnob *)aknob->aknob_DeviceKnob;
	
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
TRACEB(TRACE_INT,TRACE_SAMPLE,("internalGetKnobInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{
			case AF_TAG_MIN:
				*tagp++ = dknb->dknb_Min;
				break;
			case AF_TAG_MAX:
				*tagp++ = dknb->dknb_Max;
				break;
			case AF_TAG_DEFAULT:
				*tagp++ = dknb->dknb_Default;
				break;
			case AF_TAG_CURRENT:
				*tagp++ = aknob->aknob_CurrentValue;
 				break;
			case AF_TAG_NAME:
				*tagp++ = (int32) &dknb->dknb_Name;
				break;
			default:
				*tagp++ = AF_ERR_BADTAG;
				return AF_ERR_BADTAG;
				break;
			}
		}
	}
	
	return result;
}

/******************************************************************/
/******** Folio Creation of Knobs *********************************/
/******************************************************************/

Item internalCreateAudioKnob (AudioKnob *aknob, TagArg *args)
{
	AudioInstrument *ains = NULL;
  	char *Name = NULL;
  	int32 result;
	uint32 tagc, *tagp;
	int32 Result;
	DSPPKnob *dknb;
  	
TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioKnob(0x%x, 0x%lx)\n", aknob, args));

    Result = TagProcessor( aknob, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioKnob: TagProcessor failed.\n"));
    	return Result;
    }

	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			switch (tagc)
			{
			case AF_TAG_INSTRUMENT:
				ains = (AudioInstrument *)CheckItem(*tagp++, AUDIONODE, AUDIO_INSTRUMENT_NODE);
				break;
				
			case AF_TAG_NAME:
				Name = (char *) *tagp++; /* Validate length. */
				Result = afi_IsRamAddr( Name, 1);
				if(Result < 0) return Result;
				break;
				
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Unrecognized tag in internalCreateAudioKnob - 0x%lx: 0x%lx\n",
					tagc, *tagp++));
					return AF_ERR_BADTAG;
				}
			}
		}
	}

	if (Name == NULL) return AF_ERR_BADNAME;
	if (ains == NULL) return AF_ERR_BADITEM; /* 930828 */
	
	result = DSPPAttachKnob ( aknob,
		(DSPPInstrument *)ains->ains_DeviceInstrument, Name);
	if (result)
	{
	 return result;
	}
	
	dknb = (DSPPKnob *)aknob->aknob_DeviceKnob;
	aknob->aknob_CurrentValue = dknb->dknb_Default;
	
	AddTail( &ains->ains_KnobList, (Node *) aknob );
	
	return (aknob->aknob_Item.n_Item);
}
/******************************************************************/
int32 internalDeleteAudioKnob (AudioKnob *aknob)
{

TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioKnob(0x%lx)\n", aknob));

/* Remove from Instrument's List */
	ParanoidRemNode( (Node *) aknob );
	
	return (0);
}

/******************************************************************/
/***** SUPERVISOR MODE ********************************************/
/******************************************************************/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/tweakknob
 |||	TweakKnob - Changes the value of a knob.
 |||
 |||	  Synopsis
 |||
 |||	    Err TweakKnob (Item KnobItem, int32 Value)
 |||
 |||	  Description
 |||
 |||	    This procedure sets the value of a knob that is attached to an instrument.
 |||	    The value is clipped to the allowable range. This has been optimized to
 |||	    allow rapid modulation of sound parameters.
 |||
 |||	    This function differs from TweakRawKnob() in that the value passed to this
 |||	    funciton goes through knob-specific conversion. For example, typically
 |||	    frequency knobs are controlled with a frac16 Hz value with this function,
 |||	    and with a phase increment value with TweakRawKnob(). For many other knobs,
 |||	    the conversion is trivial: TweakRawKnob() and TweakKnob() yield the same
 |||	    results.
 |||
 |||	    The current raw value of the Knob can be read by GetAudioItemInfo() using
 |||	    the AF_TAG_CURRENT_VALUE tag. There is no inverse conversion function,
 |||	    however, to convert back to the units supplied to TweakKnob().
 |||
 |||	    If this knob is connected to the output of another instrument via
 |||	    ConnectInstruments(), the tweaked value is ignored until that connection
 |||	    is broken.
 |||
 |||	  Arguments
 |||
 |||	    KnobItem                     Item number of the knob to be tweaked.
 |||
 |||	    Value                        The new value for that knob.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    TweakRawKnob(), GrabKnob(), Knob
 |||
 **/
int32 swiTweakKnob ( Item KnobItem, int32 Value )
{
	AudioKnob *aknob;
	DSPPInstrument *dins;
	DSPPKnob *dknb;
	
TRACEE(TRACE_TOP,TRACE_KNOB,("swiTweakKnob(0x%x, 0x%lx)\n", KnobItem, Value));

/*	CHECKAUDIOOPEN; */
	
	aknob = (AudioKnob *)CheckItem(KnobItem, AUDIONODE, AUDIO_KNOB_NODE);
	if (aknob == NULL) return AF_ERR_BADITEM; /* 930828 */
	
	dins = (DSPPInstrument *) aknob->aknob_DeviceInstrument;
	dknb = (DSPPKnob *) aknob->aknob_DeviceKnob;
	
	return DSPPPutKnob(dins, dknb, Value, &aknob->aknob_CurrentValue, TRUE);
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/tweakrawknob
 |||	TweakRawKnob - Changes the value of a knob using a raw value.
 |||
 |||	  Synopsis
 |||
 |||	    Err TweakRawKnob (Item KnobItem, int32 Value)
 |||
 |||	  Description
 |||
 |||	    This procedure sets the value of a Knob that is attached to an Instrument.
 |||	    This procedure bypasses any knob-specific (e.g. frequency-to-raw-value)
 |||	    conversion that is done in TweakKnob(), and instead provides the value
 |||	    directly used by the instrument. This saves a simple operation in
 |||	    execution, which is useful if a task uses a stream of tweaking commands.
 |||
 |||	    The current raw value of the Knob can be read by GetAudioItemInfo() using
 |||	    the AF_TAG_CURRENT_VALUE tag.
 |||
 |||	    If this knob is connected to the output of another instrument via
 |||	    ConnectInstruments(), the tweaked value is ignored until that connection
 |||	    is broken.
 |||
 |||	  Arguments
 |||
 |||	    KnobItem                     Item number of the knob to be tweaked.
 |||
 |||	    Value                        The new raw value for that knob.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    TweakKnob(), GrabKnob(), Knob
 |||
 **/
int32 swiTweakRawKnob ( Item KnobItem, int32 Value )
{
	AudioKnob *aknob;
	DSPPInstrument *dins;
	DSPPKnob *dknb;
	
TRACEE(TRACE_TOP,TRACE_KNOB,("swiTweakRawKnob(0x%x, 0x%lx)\n", KnobItem, Value));

/*	CHECKAUDIOOPEN; */
	
	aknob = (AudioKnob *)CheckItem(KnobItem, AUDIONODE, AUDIO_KNOB_NODE);
	if (aknob == NULL) return AF_ERR_BADITEM; /* 930828 */
	
	dins = (DSPPInstrument *) aknob->aknob_DeviceInstrument;
	dknb = (DSPPKnob *) aknob->aknob_DeviceKnob;
	
	return DSPPPutKnob(dins, dknb, Value, &aknob->aknob_CurrentValue, FALSE);
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/bendinstrumentpitch
 |||	BendInstrumentPitch - Bends an instrument's output pitch up or down by
 |||	                      a specified amount.
 |||
 |||	  Synopsis
 |||
 |||	    Err BendInstrumentPitch (Item Instrument, frac16 BendFrac)
 |||
 |||	  Description
 |||
 |||	    This procedure sets a new pitch bend value for an Instrument, a value that
 |||	    is stored as part of the Instrument. This function sets the resulting pitch
 |||	    to the product of the Instrument's frequency and BendFrac.
 |||
 |||	    This setting remains in effect until changed by another call to
 |||	    BendInstrumentPitch(). To restore the original pitch of an Instrument, pass
 |||	    the result of Convert32_F16(1) as BendFrac.
 |||
 |||	    Use Convert12TET_F16() to compute a BendFrac value from an interval of
 |||	    semitones and cents.
 |||
 |||	    This procedure won't bend pitch above the upper frequency limit of the
 |||	    instrument.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of an instrument
 |||
 |||	    BendFrac                     16.16 factor to multiply Instrument's normal
 |||	                                 frequency by. This is a signed value; negative
 |||	                                 values result in a negative frequency value,
 |||	                                 which is not supported by all instruments.
 |||	                                 Specify the result of Convert32_F16(1) as BendFrac
 |||	                                 to restore the Instrument's original pitch.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    Convert12TET_F16()
 |||
 **/
Err swiBendInstrumentPitch( Item InstrumentItem, frac16 BendFrac )
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	int32 Result;
	int32 Freq;
	AudioKnob *aknob;
	
TRACEE(TRACE_TOP,TRACE_KNOB,("swiBendInstrumentPitch(0x%x, 0x%lx)\n", InstrumentItem, BendFrac));
	Result = 0;
	
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	
	dins = (DSPPInstrument *) ains->ains_DeviceInstrument;
	
	aknob = &dins->dins_FreqKnob;
	if(aknob->aknob_DeviceInstrument != 0)
	{
		ains->ains_Bend = BendFrac;
		Freq = MulSF16( ains->ains_OriginalRate, ains->ains_Bend );
		
		Result = DSPPPutKnob(dins, dins->dins_FreqKnob.aknob_DeviceKnob,
			Freq, &aknob->aknob_CurrentValue, FALSE);
	}		
	return Result;
	
}

