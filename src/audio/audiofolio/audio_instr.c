/* $Id: audio_instr.c,v 1.89 1994/12/16 08:41:27 phil Exp $ */
/****************************************************************
**
** Audio Instruments
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/ 

/****************************************************************
** 00001 PLB 11/16/92 Fixed ref to uninitialized dins in Release and Stop
** 00002 PLB 12/01/92 Added LoadInstrument
** 930311 PLB UnloadInstrument deletes the Template
** 930319 PLB Don't StripInsTemplate if error in LoadInsTempExt
** 930415 PLB Track connections between Items
** 930516 PLB Add AbandonInstrument, AdoptInstrument
** 930824 PLB Free NAME data from NAME chunk in 3INS FORM, was leaking.
** 930824 PLB Do not error on unrecognised FORMs
** 930829 PLB Reject if AudioDevice != 0 to allow future use.
** 930830 PLB Fix error checks for Instrument creation.
** 930831 PLB Reject Tags for Stop and Release cuz none currently supported.
** 930904 PLB Check for no template passed to CreateAudioInstrument.
** 940224 PLB Move Attachment list to aitp to prepare for shared dtmp.
** 940304 PLB DOn't SplitFileName of template name cuz it clobbers callers string.
** 940406 PLB Support TuneInsTemplate()
** 940606 WJB Added start time comparison to ScavengeInstrument().  
** 940608 WJB Replaced start time comparison with CompareAudioTimes.
** 940608 PLB Check for no template in internalCreateAudioTemplate
** 940609 PLB Added shared library template support. 
** 940726 WJB Removed iffParseImage() prototype.
** 940811 PLB Used %.4s to print ChunkTypes instead of scratch array kludge. 
** 940812 PLB Allow AF_TAG_LEAVE_IN_PLACE cuz works now for samples. 
** 940812 PLB Added AF_TAG_SPECIAL.  Support Half Calculation Rate.
** 940818 PLB Improved error cleanup in internalCreateAudioIns
** 940818 PLB Add calls to DSPPOpenSplitIns() and DSPPCloseSplitIns() for half rate.
** 940829 DAS/PLB Change MUD name to SysInfo
** 940922 PLB Add ains_ProbeList support.
** 940927 WJB Tweaked autodocs for Create/DeleteInstrument().  
** 940930 WJB Reimplemented LoadInstrument() w/ code that handles failures better.
** 941011 WJB Removed GetNumInstruments() (unimplemented function).  
** 941011 WJB Added some autodocs.  
** 941012 WJB Added autodocs for Adopt/AbandonInstrument().
**            Syncronized swiAbandonInstrument() prototype in audio.h
** 941024 PLB Changed AF_TAG_CALCRATE_SHIFT to AF_TAG_CALCRATE_DIVIDE
** 941031 PLB Trap NULL reference in internalDeleteAudioIns
** 941116 PLB Add hack for Enabling Audio Input.  Remove when SYSINFO in place. %Q
** 941121 PLB StopInstrument() if StartInstrument() called and it is still running.
** 941206 PLB Use SuperSetSysInfo() to enable audio input.
** 941215 PLB Nest enables and disables properly for multiple tasks.
****************************************************************/

#include "audio_internal.h"
#include "sysinfo.h"

/* Enable DSPP template sharing to save memory and reduce disk seeks. */
#define SHARE_DSPP_TEMPLATE


int32 Handle3INSForm ( iff_control *iffc, int32 FormType, int32 FormSize );
int32 Handle3INSChunk ( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize );
int32 StripInsTemplate ( DSPPTemplate *dtmp );

/* Macros for debugging. */
#define DBUG(x)    /* PRT(x) */
#define PRTX(x)    /* PRT(x) */

#if 0
#define REPORTMEM(msg) \
{ \
	PRT(( msg )); \
	PRT(("App===========")); \
	ReportMemoryUsage(); \
}
#else
#define REPORTMEM(msg) /* ReportMemoryUsage */
#endif

/******************************************************************
**  User Mode calls. These cannot change system data!
******************************************************************/
 /**
 |||	AUTODOC PUBLIC spg/items/template
 |||	Template - A description of an audio instrument.
 |||
 |||	  Description
 |||
 |||	    A Template is the description of a DSP audio instrument (including the DSP
 |||	    code, resource requirements, parameter settings, etc.) from which
 |||	    Instrument items are created.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_TEMPLATE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateInsTemplate()
 |||
 |||	    CreateItem()
 |||
 |||	    DefineInsTemplate()
 |||
 |||	    LoadInsTemplate()
 |||
 |||	  Delete
 |||
 |||	    DeleteItem()
 |||
 |||	    UnloadInsTemplate()
 |||
 |||	  Query
 |||
 |||	    None
 |||
 |||	  Modify
 |||
 |||	    None
 |||
 |||	  Use
 |||
 |||	    AdoptInstrument()
 |||
 |||	    AllocInstrument()
 |||
 |||	    AttachEnvelope()
 |||
 |||	    AttachSample()
 |||
 |||	    ScavengeInstrument()
 |||
 |||	    TuneInsTemplate()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_ALLOC_FUNCTION        (void *(*)(uint32 memsize, uint32
 |||	                                 memflags)) Create. Sets custom memory
 |||	                                 allocation function to be called during
 |||	                                 template creation. Defaults to AllocMem().
 |||	                                 If you supply a custom allocation function
 |||	                                 you must also provide a custom free function
 |||	                                 with AF_TAG_FREE_FUNCTION.
 |||
 |||	    AF_TAG_DEVICE                (Item) Create. Audio device Item for
 |||	                                 instrument template. 0 indicates the default audio
 |||	                                 device, the DSP, which is the only valid audio
 |||	                                 device item at the present time.
 |||
 |||	    AF_TAG_FREE_FUNCTION         (void (*)(void *memptr, uint32 memsize))
 |||	                                 Create. Sets custom memory free function to
 |||	                                 be called during template deletion. Defaults
 |||	                                 to FreeMem(). If you supply a custom free
 |||	                                 function you must also provide a custom
 |||	                                 allocation function with
 |||	                                 AF_TAG_ALLOC_FUNCTION.
 |||
 |||	    AF_TAG_IMAGE_ADDRESS         (const char *) Create. Specifies a memory
 |||	                                 location containing a template file image.
 |||	                                 Must use in conjunction with
 |||	                                 AF_TAG_IMAGE_LENGTH. Mutually exclusive
 |||	                                 AF_TAG_NAME.
 |||
 |||	    AF_TAG_IMAGE_LENGTH          (uint32) Create. Specifies number of bytes
 |||	                                 in template file image pointed to by
 |||	                                 AF_TAG_IMAGE_ADDRESS.
 |||
 |||	    AF_TAG_NAME                  (const char *) Create. Name of template file
 |||	                                 to load. Mutually exclusive with
 |||	                                 AF_TAG_IMAGE_ADDRESS.
 |||
 |||	  See Also
 |||
 |||	    Instrument, Attachment
 |||
 **/
 /*
    The following additional Template Tags are supported for internal use only:
    
        AF_TAG_CLONE                 

        AF_TAG_EXTERNAL              

        AF_TAG_TEMPLATE              

        TAG_ITEM_NAME                
 */

 /**
 |||	AUTODOC PUBLIC spg/items/instrument
 |||	Instrument - DSP Instrument Item.
 |||
 |||	  Description
 |||
 |||	    Instrument items are created from Template items. They correspond
 |||	    to actual instances of DSP code running on the DSP. Several Instruments
 |||	    can be connected together to create a patch. Instruments can be played (by
 |||	    starting them), and controlled (by tweaking knobs). An Instrument is
 |||	    monophonic in the sense that it corresponds to a single voice. Multiple
 |||	    voices require creating an Instrument per voice.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_INSTRUMENT_NODE
 |||
 |||	  Create
 |||
 |||	    AllocInstrument()
 |||
 |||	    CreateInstrument() (new for V24)
 |||
 |||	    CreateItem()
 |||
 |||	    LoadInstrument()
 |||
 |||	  Delete
 |||
 |||	    DeleteInstrument() (new for V24)
 |||
 |||	    DeleteItem()
 |||
 |||	    FreeInstrument()
 |||
 |||	    UnloadInstrument()
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
 |||	    AbandonInstrument()
 |||
 |||	    AttachEnvelope()
 |||
 |||	    AttachSample()
 |||
 |||	    BendInstrumentPitch()
 |||
 |||	    ConnectInstruments()
 |||
 |||	    CreateProbe() (new for V24)
 |||
 |||	    DisconnectInstruments()
 |||
 |||	    GetKnobName()
 |||
 |||	    GetNumKnobs()
 |||
 |||	    GrabKnob()
 |||
 |||	    PauseInstrument()
 |||
 |||	    ReleaseInstrument()
 |||
 |||	    ResumeInstrument()
 |||
 |||	    StartInstrument()
 |||
 |||	    StopInstrument()
 |||
 |||	    TuneInstrument()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_AMPLITUDE             (uint32) Start. Value to set instrument's
 |||	                                 Amplitude knob to before starting instrument
 |||	                                 (for instruments that have an Amplitude
 |||	                                 knob). Valid range is 0..0x7fff. Has no
 |||	                                 effect if Amplitude knob is connected to the
 |||	                                 output from another instrument. This tag is
 |||	                                 mutually exclusive with AF_TAG_VELOCITY.
 |||
 |||	    AF_TAG_CALCRATE_DIVIDE       (uint32) (new tag for V24) Create.
 |||	                                 Specifies the the denominator of the fraction
 |||	                                 of the total DSP cycles on which this instrument
 |||	                                 is to run. The only valid
 |||	                                 settings at this time are 1 to run on all DSP
 |||	                                 cycles (i.e. execute at 44,100 cycles/sec),
 |||	                                 and 2 to run on only 1/2 of the DSP
 |||	                                 cycles (i.e. execute at 22,050 cycles/sec).
 |||	                                 Defaults to 1.
 |||
 |||	    AF_TAG_FREQUENCY             (ufrac16) Start. Value to set Frequency knob
 |||	                                 to in 16.16 Hertz (for instruments that have
 |||	                                 a Frequency knob). Has no effect if
 |||	                                 Frequency knob is connected to the output
 |||	                                 from another instrument. This tag is
 |||	                                 mutually exclusive with AF_TAG_PITCH and
 |||	                                 AF_TAG_RATE.
 |||
 |||	    AF_TAG_PITCH                 (uint32) Start. Value to set Frequency knob
 |||	                                 (for instruments that have a Frequency knob)
 |||	                                 expressed as a MIDI note number. The range
 |||	                                 is 0 to 127; 60 is middle C. For multisample
 |||	                                 instruments, picks the sample associated with
 |||	                                 the MIDI pitch number. This tag is mutually
 |||	                                 exclusive with AF_TAG_FREQUENCY and
 |||	                                 AF_TAG_RATE.
 |||
 |||	    AF_TAG_PRIORITY              (uint32) Create, Query. The priority of
 |||	                                 execution in DSP in the range of 0..255,
 |||	                                 where 255 is the highest priority. Defaults
 |||	                                 to 100 on creation.
 |||
 |||	    AF_TAG_RATE                  (uint32) Start. Value to set Frequency knob
 |||	                                 to in instrument-specific frequency units
 |||	                                 (e.g. phase incrment, proportion of original
 |||	                                 sample rate) for instruments that have a
 |||	                                 Frequency knob. Has no effect if Frequency
 |||	                                 knob is connected to the output from another
 |||	                                 instrument. This tag is mutually exclusive
 |||	                                 with AF_TAG_FREQUENCY and AF_TAG_PITCH.
 |||
 |||	    AF_TAG_SET_FLAGS             (uint32) Create. AF_INSF_ flags to set at
 |||	                                 creation time. Defaults to all cleared.
 |||
 |||	    AF_TAG_START_TIME            (AudioTime) Query. Returns the AudioTime
 |||	                                 value of when the instrument was last
 |||	                                 started.
 |||
 |||	    AF_TAG_STATUS                (uint32) Query. Returns the current
 |||	                                 instrument status: AF_STARTED, AF_RELEASED,
 |||	                                 AF_STOPPED, or AF_ABANDONED.
 |||
 |||	    AF_TAG_TEMPLATE              (Item) Create. DSP Template Item used from
 |||	                                 which to create instrument. Note: this tag
 |||	                                 cannot be used with CreateInstrument().
 |||
 |||	    AF_TAG_TIME_SCALE            (ufrac16) Start, Release. Scale times for
 |||	                                 all Envelopes attached to this Instrument.
 |||	                                 Original value is derived from the
 |||	                                 AF_TAG_TIME_SCALE provided when the Envelope
 |||	                                 Attachment was made. This value remains in
 |||	                                 effect until another AF_TAG_TIME_SCALE is
 |||	                                 passed to StartInstrument() or
 |||	                                 ReleaseInstrument().
 |||
 |||	    AF_TAG_VELOCITY              (uint32) Start. MIDI note velocity
 |||	                                 indicating the value to set instrument's
 |||	                                 Amplitude knob to before starting instrument
 |||	                                 (for instruments that have an Amplitude
 |||	                                 knob). Valid range is 0..127. Has no effect
 |||	                                 if Amplitude knob is connected to the output
 |||	                                 from another instrument. This tag is
 |||	                                 mutually exclusive with AF_TAG_AMPLITUDE.
 |||
 |||	  Flags
 |||
 |||	    AF_INSF_AUTOABANDON          When set, causes instrument to automatically
 |||	                                 go to the AF_ABANDONED state when stopped
 |||	                                 (either automatically because of an
 |||	                                 AF_ATTF_FATLADYSINGS flag, or manually
 |||	                                 because of a StopInstrument() call).
 |||	                                 Otherwise, the instrument goes to the
 |||	                                 AF_STOPPED state when stopped.
 |||
 |||	                                 Note that regardless of the state of this
 |||	                                 flag, instruments are created in the
 |||	                                 AF_STOPPED state.
 |||
 |||	  See Also
 |||
 |||	    Template, Attachment, Knob, Probe
 |||
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/loadinstrument
 |||	LoadInstrument - Loads Instrument and Template in one call.
 |||
 |||	  Synopsis
 |||
 |||	    Item LoadInstrument (char *Name, Item AudioDevice, uint8 Priority)
 |||
 |||	  Description
 |||
 |||	    This functions combines the actions of LoadInsTemplate() and
 |||	    CreateInstrument(), and returns the resulting Instrument item.
 |||
 |||	    Call UnloadInstrument() (not DeleteInstrument()) to free an Instrument
 |||	    created by this function. Calling DeleteInstrument() deletes the
 |||	    Instrument, but not the Template, leaving you with an unaccessible Template
 |||	    Item that you can't delete.
 |||
 |||	  Arguments
 |||
 |||	    Name                         Name of the file containing the instrument
 |||	                                 template.
 |||
 |||	    AudioDevice                  Audio device Item for instrument. 0 indicates
 |||	                                 the default audio
 |||	                                 device, the DSP, which is the only valid audio
 |||	                                 device item at the present time.
 |||
 |||	    Priority                     Determines order of execution in DSP. Set
 |||	                                 from 0 to 200. A typical value would be
 |||	                                 100. This also determines the priority over
 |||	                                 other instruments when voices are stolen for
 |||	                                 dynamic voice allocation.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns an Instrument item number (a positive value) if
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
 |||	    UnloadInstrument(), LoadInsTemplate(), CreateInstrument()
 |||
 **/
#if 1
Item LoadInstrument ( /* !!! const */ char *InsName, Item AudioDevice , uint8 Priority)
{
    Item instemplate = 0;
    Item instrument = 0;
    Err errcode = 0;
	
	if ( (errcode = instemplate = LoadInsTemplate (InsName, AudioDevice)) < 0 ) goto clean; 
	if ( (errcode = instrument = AllocInstrumentSpecial (instemplate, Priority, 0)) < 0 ) goto clean;
	
DBUG(("LoadInstrument: returns 0x%x\n", instrument));
	return instrument;
	
clean:	
    UnloadInsTemplate (instemplate);
    return errcode;
}
#else
/* !!! %Q Get rid of old version of LoadInstrument() when satisfied that replacement is safe. */
Item LoadInstrument ( char *InsName, Item AudioDevice , uint8 Priority)
{
	Item Tmp;
	int32 Result;
	
	Tmp = LoadInsTemplate ( InsName, AudioDevice );
	if (Tmp < 0) return Tmp;

	Result = AllocInstrumentSpecial( Tmp, Priority, 0 );
	
DBUG(("LoadInstrument: returns 0x%x\n", Result));
	return Result;
}
#endif

/*******************************************************************
** Create an instrument from a template.
** First create any needed Library instruments.
*******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/createinstrument
 |||	CreateInstrument - Creates an instrument using a template.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateInstrument (Item InsTemplate, const TagArg *tagList)
 |||
 |||	    Item CreateInstrumentVA (Item InsTemplate, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure allocates an Instrument based on an instrument Template,
 |||	    previously loaded using LoadInsTemplate() or similar call, and allocates
 |||	    the DSP resources necessary for the Instrument. See also the convenience
 |||	    function LoadInstrument() combines these two steps.
 |||
 |||	    The new instrument is always created in the AF_STOPPED state, regardless
 |||	    of the setting of AF_INSF_ABANDONED.
 |||
 |||	    Call DeleteInstrument() when you're finished with the Instrument to
 |||	    deallocate its resources.
 |||
 |||	    This function is a superset of AllocInstrument().
 |||
 |||	  Arguments
 |||
 |||	    InsTemplate                  Item number of a loaded instrument template
 |||	                                 used to allocate the instrument.
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_CALCRATE_DIVIDE       (uint32) (new tag for V24) Create.
 |||	                                 Specifies the the denominator of the fraction
 |||	                                 of the total DSP cycles on which this instrument
 |||	                                 is to run. The only valid
 |||	                                 settings at this time are 1 to run on all DSP
 |||	                                 cycles (i.e. execute at 44,100 cycles/sec),
 |||	                                 and 2 to run on only 1/2 of the DSP
 |||	                                 cycles (i.e. execute at 22,050 cycles/sec).
 |||	                                 Defaults to 1.
 |||
 |||	    AF_TAG_PRIORITY              (uint32) Priority of new instrument in range
 |||	                                 of 0..255. Defaults to 100.
 |||
 |||	    AF_TAG_SET_FLAGS             (uint32) Set of AF_INSF_ flags to set in new
 |||	                                 instrument. All flags default to cleared.
 |||	                                 See the Instrument item page a for complete
 |||	                                 description of AF_INSF_ flags.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number for the Instrument (a positive value)
 |||	    or an error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V24.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DeleteInstrument(), AllocInstrument(), LoadInsTemplate(), LoadInstrument()
 |||
 **/
 /**
 |||
 |||	  Private Tags
 |||
 |||	    AF_TAG_SPECIAL               (int32) AF_SPECIAL_ value.
 |||	                                 !!! phil: please add internal documentation for this
 |||
 **/
#if 1
Item CreateInstrument ( Item InsTemplate, const TagArg *Tags )
{
    return CreateItemVA( MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE),
	                     AF_TAG_TEMPLATE, InsTemplate,
	                     TAG_JUMP, Tags );
}
#else
/* %Q Get rid of old version of CreateInstrument */
Item CreateInstrument ( Item InsTemplate, TagArg *Tags )
{
	TagArg MyTags[2];

	MyTags[0].ta_Tag = AF_TAG_TEMPLATE;
	MyTags[0].ta_Arg = (TagData) InsTemplate;

	if( Tags )
	{
		MyTags[1].ta_Tag = TAG_JUMP;
		MyTags[1].ta_Arg = (TagData) Tags;
	}
	else
	{
		MyTags[1].ta_Tag = TAG_END;
	}   				

	return CreateItem( MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE), MyTags );
}
#endif
/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/unloadinstrument
 |||	UnloadInstrument - Unloads an instrument loaded with LoadInstrument().
 |||
 |||	  Synopsis
 |||
 |||	    Err UnloadInstrument (Item Instrument)
 |||
 |||	  Description
 |||
 |||	    This procedure frees the Instrument and unloads the Template loaded by
 |||	    LoadInstrument().
 |||
 |||	    Do not confuse this function with DeleteInstrument(), which deletes an
 |||	    Instrument created by CreateInstrument(). Calling DeleteInstrument() for an
 |||	    instrument created by LoadInstrument() deletes the Instrument, but not the
 |||	    Template, leaving you with an unaccessible Template Item that you can't
 |||	    delete. Calling UnloadInstrument() for and Instrument created by
 |||	    CreateInstrument() deletes Template for that Instrument along with all
 |||	    other Instruments created from that Template.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   Item number of the instrument.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns 0 if successful or an error code (a negative value)
 |||	    if an error occurs.
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
 |||	    LoadInstrument(), DeleteInstrument(), UnloadInsTemplate()
 |||
 **/
int32 UnloadInstrument ( Item InstrumentItem  )
{
	int32 Result;
	AudioInstrument *ains;
	AudioInsTemplate *aitp;

DBUG(("UnloadInstrument( 0x%x )\n", InstrumentItem));

	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	
	aitp = ains->ains_Template;
	
	Result = DeleteItem ( InstrumentItem );
	if (Result < 0) goto done;
	Result = UnloadInsTemplate ( aitp->aitp_Item.n_Item );
done:
DBUG(("UnloadInstrument: returns 0x%x\n", Result));
	return Result;
}

/**************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/unloadinstemplate
 |||	UnloadInsTemplate - Unloads an instrument template.
 |||
 |||	  Synopsis
 |||
 |||	    Err UnloadInsTemplate (Item InsTemplate)
 |||
 |||	  Description
 |||
 |||	    This procedure unloads (or deletes) an instrument template created by
 |||	    CreateInsTemplate(), LoadInsTemplate(), and DefineInsTemplate(). It unloads
 |||	    the Template from memory and frees its resources. Any Instruments created
 |||	    using the Template are deleted as well, with the expected side-effects.
 |||	    Any Attachments made to this Template are also deleted.
 |||
 |||	    Unlike DeleteInstrument(), any Envelopes or Samples attached to this
 |||	    Template are also deleted, along with the expected side-effects. If you
 |||	    need to protect the Envelopes or Attachments, detach them from the Template
 |||	    first.
 |||
 |||	  Arguments
 |||
 |||	    InsTemplate                  Item number of Template to delete.
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
 |||	  Caveats
 |||
 |||	    Ideally there should be a DeleteInsTemplate() function, but there isn't one
 |||	    yet. Sorry for any confusion that might arise from pairing
 |||	    CreateInsTemplate() or DefineInsTemplate() with UnloadInsTemplate().
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    CreateInsTemplate(), LoadInsTemplate(), DefineInsTemplate()
 |||
 **/
int32 UnloadInsTemplate ( Item TemplateItem  )
{
	int32 Result;
	AudioInsTemplate *aitp;
	ItemNode *Slave;
	AudioAttachment *aatt, *nextaatt;
	
TRACEE(TRACE_INT,TRACE_ITEM,("UnloadInsTemplate( 0x%x )\n", TemplateItem));

	aitp = (AudioInsTemplate *)CheckItem(TemplateItem, AUDIONODE, AUDIO_TEMPLATE_NODE);
	if (aitp == NULL) return AF_ERR_BADITEM;
	
/* Delete any items specified in the template. */
	aatt = (AudioAttachment *)FirstNode(&aitp->aitp_Attachments); /* 940424 */
	while (ISNODE(&aitp->aitp_Attachments,aatt))
	{
DBUG(("UnloadInsTemplate: Attachment 0x%x\n", aatt));

		nextaatt = (AudioAttachment *)NextNode((Node *)aatt);
		Slave = (ItemNode *) aatt->aatt_Structure;
		switch(Slave->n_Type)
		{
			case AUDIO_SAMPLE_NODE:
TRACEE(TRACE_INT,TRACE_ITEM,("UnloadInsTemplate: Deleting sample item 0x%x\n", aatt->aatt_SlaveItem));
DBUG(("UnloadInsTemplate: Deleting sample item 0x%x\n", aatt->aatt_SlaveItem));
				UnloadSample( aatt->aatt_SlaveItem );
				break;
				
			case AUDIO_ENVELOPE_NODE:
DBUG(("UnloadInsTemplate: Deleting envelope item 0x%x\n", aatt->aatt_SlaveItem));
				DeleteEnvelope( aatt->aatt_SlaveItem );
				break;
				
			default:
				ERR(("UnloadInsTemplate: Unrecognized Slave Item = 0x%x\n", Slave));
				Result = AF_ERR_BADITEM;
				goto done;
		}
		
		aatt = nextaatt;
	}

	Result = DeleteItem ( TemplateItem );
done:
TRACER(TRACE_INT,TRACE_ITEM,("UnloadInsTemplate returns 0x%x )\n", Result));
	return Result;
}

/*****************************************************************/
/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/createinstemplate
 |||	CreateInsTemplate - Generic template creation function.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateInsTemplate (TagArg *tagList)
 |||
 |||	    Item CreateInsTemplateVA (uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This is the general function to create an instrument Template. It is used
 |||	    internally by all the other template creation functions (e.g.
 |||	    LoadInsTemplate(), DefineInsTemplate(), etc) to actually perform the
 |||	    template creation. Use it if you need to do something that these other
 |||	    functions don't provide.
 |||
 |||	    Call UnloadInsTemplate() to dispose of the Item created by this function.
 |||
 |||	  Arguments
 |||
 |||	    None
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_ALLOC_FUNCTION        (void *(*)(uint32 memsize, uint32
 |||	                                 memflags)) Sets custom memory
 |||	                                 allocation function to be called during
 |||	                                 template creation. Defaults to AllocMem().
 |||	                                 If you supply a custom allocation function
 |||	                                 you must also provide a custom free function
 |||	                                 with AF_TAG_FREE_FUNCTION.
 |||
 |||	    AF_TAG_DEVICE                (Item) Audio device Item for
 |||	                                 instrument template. 0 indicates the default audio
 |||	                                 device, the DSP, which is the only valid audio
 |||	                                 device item at the present time. Conveniently
 |||	                                 defaults to 0, so you should never need to use
 |||	                                 this tag.
 |||
 |||	    AF_TAG_FREE_FUNCTION         (void (*)(void *memptr, uint32 memsize))
 |||	                                 Sets custom memory free function to
 |||	                                 be called during template deletion. Defaults
 |||	                                 to FreeMem(). If you supply a custom free
 |||	                                 function you must also provide a custom
 |||	                                 allocation function with
 |||	                                 AF_TAG_ALLOC_FUNCTION.
 |||
 |||	    AF_TAG_IMAGE_ADDRESS         (const char *) Specifies a memory
 |||	                                 location containing a template file image.
 |||	                                 Must use in conjunction with
 |||	                                 AF_TAG_IMAGE_LENGTH. Mutually exclusive
 |||	                                 AF_TAG_NAME.
 |||
 |||	    AF_TAG_IMAGE_LENGTH          (uint32) Specifies number of bytes
 |||	                                 in template file image pointed to by
 |||	                                 AF_TAG_IMAGE_ADDRESS.
 |||
 |||	    AF_TAG_NAME                  (const char *) Name of template file
 |||	                                 to load. Mutually exclusive with
 |||	                                 AF_TAG_IMAGE_ADDRESS.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns an instrument Template Item number (a positive value)
 |||	    if successful or an error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    Ideally there should be a DeleteInsTemplate() function, but there isn't one
 |||	    yet. Sorry for any confusion that might arise from pairing
 |||	    CreateInsTemplate() with UnloadInsTemplate().
 |||
 |||	  Examples
 |||
 |||	        // These two calls are equivalent
 |||	    LoadInsTemplate ("directout.dsp", 0);
 |||	    CreateInsTemplateVA (
 |||	        AF_TAG_NAME, "directout.dsp",
 |||	        TAG_END);
 |||
 |||	        // These two calls are also equivalent
 |||	    DefineInsTemplate (dspFileImage, dspFileLength, 0, "foo.dsp");
 |||	    CreateInsTemplateVA (
 |||	        AF_TAG_IMAGE_ADDRESS, dspFileImage,
 |||	        AF_TAG_IMAGE_LENGTH,  dspFileLength,
 |||	        TAG_END);
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    UnloadInsTemplate(), LoadInsTemplate(), DefineInsTemplate(), Template,
 |||	    CreateInstrument(), LoadInstrument(), AttachSample(), AttachEnvelope()
 |||
 **/
Item CreateInsTemplate ( TagArg *Tags)
{
	Item Result;
	uint32 tagc, *tagp, temp;
	iff_control iffcb;
	TemplateParserContext TMPLPC;
	TagArg MyTags[3];
	int32 ti;
	ItemNode *n;
	AudioInsTemplate *MasterTemplatePtr;
	DSPPTemplate *dtmp;

	char *InsName = NULL;
	void *(*CustomAllocMem)() = NULL;
	void (*CustomFreeMem)() = NULL;
	int32 IfLeaveInPlace = FALSE;
	char *ImageAddress = NULL;
	int32 ImageLength = 0;
	int32 IfLoadingTemplate = FALSE;
	int32 IfOtherTags = FALSE;
	Item  AudioDevice = 0;
	int32 InsMode = AF_TAG_TEMPLATE;
	
	Result = 0;
TRACEE(TRACE_INT,TRACE_OFX,("CreateInsTemplate( 0x%lx)\n", Tags));

	CHECKAUDIOOPEN;
		
	tagp = (uint32 *) Tags;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			temp = *tagp++;
DBUG(("CreateInsTemplate: Tag = %d, Arg = $%x\n", tagc, temp));
			
			switch (tagc)
			{
			case AF_TAG_NAME:
				InsName = (char *)temp;
DBUG(("CreateInsTemplate: load %s\n", InsName));
				IfLoadingTemplate = TRUE;
				break;
				
			case AF_TAG_IMAGE_ADDRESS:
				ImageAddress = (char *) temp;
				IfLoadingTemplate = TRUE;
				break;
			case AF_TAG_IMAGE_LENGTH:
				ImageLength = temp;
				IfLoadingTemplate = TRUE;
				break;
			
			case AF_TAG_LEAVE_IN_PLACE:
				IfLeaveInPlace = temp;
				IfLoadingTemplate = TRUE;
				break;
				
			case AF_TAG_ALLOC_FUNCTION:
				CustomAllocMem = (void *(*)()) temp;
				IfLoadingTemplate = TRUE;
				break;
				
			case AF_TAG_FREE_FUNCTION:
				CustomFreeMem = (void (*)()) temp;
				IfLoadingTemplate = TRUE;
				break;
				
			case AF_TAG_EXTERNAL:
				InsMode = (int32) AF_TAG_EXTERNAL;
				break;
			case AF_TAG_TEMPLATE:
				InsMode = (int32) AF_TAG_TEMPLATE;
				break;
				
			case AF_TAG_DEVICE:
				AudioDevice = (Item) temp;
				break;
				
			default:
				ERR(("CreateInsTemplate: unexpected tag = 0x%x\n"));
				IfOtherTags = TRUE;
				break;
			}
		}
	}

/* Detect Tag errors. */
	if( IfLoadingTemplate && IfOtherTags )
	{
		ERR(("CreateInsTemplate: mixed loading tags with make from scratch tags.\n"));
		return AF_ERR_BADTAG;
	}
	
	if((CustomAllocMem==NULL) ^ (CustomFreeMem==NULL))
	{
		ERR(("CreateInsTemplate: Both custom functions must be passed!\n"));
		return AF_ERR_OUTOFRANGE;
	}
	
	if(AudioDevice != 0)
	{
		ERR(("LoadInsTemplate: Illegal AudioDevice = 0x%x\n", AudioDevice));
		return AF_ERR_BADITEM; /* 930828 */
	}

/* Load from file or image. */
	if( IfLoadingTemplate )
	{
	
#ifdef SHARE_DSPP_TEMPLATE
		if(InsName)
		{
/* Look to see if instrument has already been loaded. If so, share. 940224 */
			n = (ItemNode *) FindNamedNode(&AudioBase->af_TemplateList, InsName );
			MasterTemplatePtr = (AudioInsTemplate *) n;
			if(n && IsEmptyList( &MasterTemplatePtr->aitp_Attachments ))
			{
				dtmp = (DSPPTemplate *)(MasterTemplatePtr->aitp_DeviceTemplate);
				if(dtmp->dtmp_FunctionID > 0)
    			{
DBUG(("Share: 0x%x = %s, ID = %d\n", n, n->n_Name, dtmp->dtmp_FunctionID));
					ti=0;
/* TAG_ITEM_NAME added for shared templates. */
					MyTags[ti].ta_Tag = TAG_ITEM_NAME;
					MyTags[ti++].ta_Arg = (void *) InsName;  /* Don't SplitFileName 940304 */
					MyTags[ti].ta_Tag = AF_TAG_CLONE;
					MyTags[ti++].ta_Arg = (void *) MasterTemplatePtr->aitp_Item.n_Item;
					MyTags[ti].ta_Tag = TAG_END;
    				Result = CreateItem( MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE), MyTags );
    				goto done;
    			}
			}
		}
#else
	PRT(("DSPPTemplate Sharing disabled\n"));
#endif

/* Setup Parser. */
		memset( (char *) &TMPLPC, 0, sizeof(TemplateParserContext) );
		TMPLPC.tmplpc_InsName = InsName;
		TMPLPC.tmplpc_InsMode = InsMode;
		TMPLPC.tmplpc_CustomAllocMem = CustomAllocMem;
		TMPLPC.tmplpc_CustomFreeMem = CustomFreeMem;
		TMPLPC.tmplpc_IfLeaveInPlace = IfLeaveInPlace;
		memset( (char *) &iffcb, 0, sizeof(iff_control) );
		iffcb.iffc_UserContext = (void *) &TMPLPC;
		iffcb.iffc_FormHandler = Handle3INSForm;
		iffcb.iffc_LastChanceDir = AudioBase->af_StartupDirectory;
		iffcb.iffc_Level = 0;
		iffcb.iffc_ChunkHandler = 0;

	 	if(ImageAddress != NULL)
	 	{
			Result = (int32) iffParseImage(&iffcb, ImageAddress, ImageLength );
		}
		else
		{
	 		Result = (int32) iffParseFile(&iffcb, InsName);
		}
		if (Result < 0)
		{
			ERR(("CreateInsTemplate: Could not parse file.\n"));
			goto error;
		}
		/* Do we have the functional info we need to be BullDog compatible? */
		if( (TMPLPC.tmplpc_Flags & (TMPLPC_FLAG_CRDC_FOUND|TMPLPC_FLAG_DFID_FOUND)) == 0)
		{
			PRT(("Warning: Instrument is obsolete. Missing ARIA Patch or DFID!\n"));
			if( InsName ) ERR(("      %s\n", InsName));
/* Don't allow illegal instrument templates at all!!! 940811 */
			DeleteItem(TMPLPC.tmplpc_TemplateItem);
			Result = AF_ERR_BADOFX;
			goto error;
		}

		Result = TMPLPC.tmplpc_TemplateItem;
	}
	else
	{
		ERR(("CreateInsTemplate: no load tags!\n"));
		Result = AF_ERR_BADTAG;
	}
	
TRACER(TRACE_INT, TRACE_OFX, ("CreateInsTemplate returns 0x%08x\n", Result));
done:
error:
	return Result;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/loadinstemplate
 |||	LoadInsTemplate - Loads template for creating instruments.
 |||
 |||	  Synopsis
 |||
 |||	    Item LoadInsTemplate (char *Name, Item AudioDevice)
 |||
 |||	  Description
 |||
 |||	    This procedure loads an instrument template from the specified file. Note
 |||	    that the procedure doesn't create an instrument from the instrument
 |||	    template. Call CreateInstrument() to create an Instrument from this Template.
 |||
 |||	    When you finish using the Template, call UnloadInsTemplate() to deallocate
 |||	    the template's resources.
 |||
 |||	  Arguments
 |||
 |||	    Name                         Name of the file containing the instrument
 |||	                                 template (e.g. "directout.dsp").
 |||
 |||	    AudioDevice                  Audio device Item for
 |||	                                 instrument template. 0 indicates the default audio
 |||	                                 device, the DSP, which is the only valid audio
 |||	                                 device item at the present time.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a Template item number if successful (a positive
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
 |||	  See Also
 |||
 |||	    UnloadInsTemplate(), CreateInsTemplate(), CreateInstrument(),
 |||	    LoadInstrument(), AttachSample(), AttachEnvelope()
 |||
 **/
Item LoadInsTemplate ( char *InsName, Item AudioDevice )
{
DBUG(("\nLoadInsTemplate: InsName = %s\n", InsName));
	return LoadInsTempExt ( InsName, AudioDevice, AF_TAG_TEMPLATE );
}

/*****************************************************************/
Item LoadInsExternal ( char *InsName, Item AudioDevice)
{
	return LoadInsTempExt ( InsName, AudioDevice, AF_TAG_EXTERNAL );
}

	
/*****************************************************************/
Item LoadInsTempExt ( char *InsName, Item AudioDevice, int32 InsMode )
{
	Item Result;
	TagArg Tags[4];

TRACEE(TRACE_INT,TRACE_OFX,("LoadInsTempExt( %s )\n", InsName));

	Tags[0].ta_Tag = AF_TAG_NAME;
	Tags[0].ta_Arg = (void *) InsName;
	Tags[1].ta_Tag = AF_TAG_DEVICE;
	Tags[1].ta_Arg = (void *) AudioDevice;
	Tags[2].ta_Tag = InsMode;
	Tags[2].ta_Arg = (void *) 0;
	Tags[3].ta_Tag = TAG_END;
	
	Result = CreateInsTemplate( Tags );
	
TRACER(TRACE_INT, TRACE_OFX, ("LoadInsTempExt returns 0x%08x\n", Result));
	return Result;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/defineinstemplate
 |||	DefineInsTemplate - Creates an instrument template from a .dsp file.
 |||
 |||	  Synopsis
 |||
 |||	    Item DefineInsTemplate (uint8 *Definition, int32 NumBytes, Item Device,
 |||	                            char *Name)
 |||
 |||	  Description
 |||
 |||	    This procedure defines an instrument template from a .dsp file image that
 |||	    has been loaded into RAM. The procedure is intended for use with streaming
 |||	    data applications, or any other application where calling LoadInsTemplate(),
 |||	    which reads DSP files off disc, at instrument template creation time is
 |||	    impractical.
 |||
 |||	    When you are finished with the template, you should call
 |||	    UnloadInsTemplate() to deallocate the resources.
 |||
 |||	    See CreateInsTemplate() for a more general solution to creating instrument
 |||	    templates.
 |||
 |||	  Arguments
 |||
 |||	    Definition                   A pointer to the DSP instrument template file
 |||	                                 image in memory. This memory does not need
 |||	                                 to remain valid after the Template has been
 |||	                                 created.
 |||
 |||	    NumBytes                     The length of the file image pointed to by
 |||	                                 Definition.
 |||
 |||	    Device                       The item number of the device on which you
 |||	                                 want to play instruments created with the
 |||	                                 template. 0 indicates the default audio
 |||	                                 device, the DSP, which is the only valid audio
 |||	                                 device item at the present time.
 |||
 |||	    Name                         The name of the instrument template file
 |||	                                 image. Ignored.
 |||
 |||	  Return Value
 |||
 |||	    This returns the Item number of the Template (a positive value) or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    Ideally there should be a DeleteInsTemplate() function, but there isn't one
 |||	    yet. Sorry for any confusion that might arise from pairing
 |||	    DefineInsTemplate() with UnloadInsTemplate().
 |||
 |||	    The name argument is ignored.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    UnloadInsTemplate(), CreateInsTemplate(), LoadInsTemplate(),
 |||	    DefineSampleHere()
 |||
 **/
Item  DefineInsTemplate( uint8 *Image, int32 NumBytes, Item Device, char *Name )
{
	Item Result;
	TagArg Tags[4];

TRACEE(TRACE_INT,TRACE_OFX,("DefineInsTemplate( %s )\n", Name));
	
	Tags[0].ta_Tag = AF_TAG_IMAGE_ADDRESS;
	Tags[0].ta_Arg = (void *) Image;
	Tags[1].ta_Tag = AF_TAG_IMAGE_LENGTH;
	Tags[1].ta_Arg = (void *) NumBytes;
	Tags[2].ta_Tag = AF_TAG_DEVICE;
	Tags[2].ta_Arg = (void *) Device;
	Tags[3].ta_Tag = TAG_END;
	
	Result = CreateInsTemplate( Tags );
	
TRACER(TRACE_INT, TRACE_OFX, ("DefineInsTemplate returns 0x%08x\n", Result));
	return Result;
}

/* Free all associated allocated memory structures. */
int32 StripInsTemplate ( DSPPTemplate *dtmp )
{
	if (dtmp->dtmp_Resources != NULL) EZMemFree(dtmp->dtmp_Resources);
	if (dtmp->dtmp_ResourceNames != NULL) EZMemFree(dtmp->dtmp_ResourceNames);
	if (dtmp->dtmp_Relocations != NULL) EZMemFree(dtmp->dtmp_Relocations);
	if (dtmp->dtmp_Knobs != NULL) EZMemFree(dtmp->dtmp_Knobs);
	if (dtmp->dtmp_Codes != NULL) EZMemFree(dtmp->dtmp_Codes);
	if (dtmp->dtmp_DataInitializer != NULL) EZMemFree(dtmp->dtmp_DataInitializer);
	return 0;
}

/*******************************************************************
**  Handle FORMs encountered when loading dsp instrument files.
*******************************************************************/
int32 Handle3INSForm ( iff_control *iffc, int32 FormType, int32 FormSize )
{
	int32 Result;
	int32 (*OldChunkHandler)();
	int32 (*OldFormHandler)();
	TemplateParserContext *tmplpc;
	DSPPTemplate *dtmp = NULL;
	TagArg Tags[4];
	int32 ti;
	
	Result = 0;
/* save old handlers for proper form nesting. */
	OldChunkHandler = iffc->iffc_ChunkHandler;
	OldFormHandler = iffc->iffc_FormHandler;
	
	tmplpc = (TemplateParserContext *) iffc->iffc_UserContext;
	
	switch(FormType)
	{
		case ID_3INS:
			iffc->iffc_ChunkHandler = Handle3INSChunk;
			Result = iffScanChunks( iffc, FormSize );
			CHECKRSLT(("Handle3INSForm: Error scanning instrument = 0x%x\n", Result));
			break;
						
/* Load sample then attach it to previously created template. */
		case ID_ATSM:
			iffc->iffc_FormHandler = HandleATSMForm;
			iffc->iffc_ChunkHandler = HandleATSMChunk;
			Result = iffScanChunks( iffc, FormSize );
			CHECKRSLT(("Handle3INSForm: Error scanning ATSM = 0x%x\n", Result));
			if (tmplpc->tmplpc_TemplateItem != 0)
			{
				Result = AttachSample( tmplpc->tmplpc_TemplateItem, tmplpc->tmplpc_SlaveItem,
					tmplpc->tmplpc_HookName);
				CHECKRSLT(("Handle3INSForm: Error attaching sample = 0x%x\n", Result));
			}
			else
			{
				ERR(("Handle3INSForm: Sample before DSP Patch!\n"));
				return AF_ERR_BADOFX;
			}
			break;

/* Load envelope then attach it to previously created template. */
		case ID_ATNV:
			iffc->iffc_FormHandler = HandleATNVForm;
			iffc->iffc_ChunkHandler = HandleATNVChunk;
			Result = iffScanChunks( iffc, FormSize );
			CHECKRSLT(("Handle3INSForm: Error scanning ATNV = 0x%x\n", Result));
			if (tmplpc->tmplpc_TemplateItem != 0)
			{
				Result = AttachEnvelope( tmplpc->tmplpc_TemplateItem, tmplpc->tmplpc_SlaveItem,
					tmplpc->tmplpc_HookName);
				CHECKRSLT(("Handle3INSForm: Error attaching sample = 0x%x\n", Result));
			}
			else
			{
				ERR(("Handle3INSForm: Envelope before DSP Patch!\n"));
				return AF_ERR_BADOFX;
			}
			break;

/* Load a DSP specific main instrument template. --------------------- */
		case ID_DSPP:		
			dtmp = (DSPPTemplate *) UserMemAlloc( sizeof(DSPPTemplate), MEMTYPE_FILL );
			if(dtmp == NULL)
			{
				Result = AF_ERR_NOMEM;
				goto error;
			}
/* parse chunks in DSPP FORM using DSPPHandleInsChunk */
			iffc->iffc_ChunkHandler = DSPPHandleInsChunk;
			iffc->iffc_UserContext = (void *) dtmp;
			Result = iffScanChunks( iffc, FormSize );
			CHECKRSLT(("Handle3INSForm: Error scanning DSPP = 0x%x\n", Result));
			
/* TAG_ITEM_NAME added for shared templates. 940224 */
			ti=0;
			if(tmplpc->tmplpc_InsName != NULL)
			{
				Tags[ti].ta_Tag = TAG_ITEM_NAME;
				Tags[ti++].ta_Arg = (void *) tmplpc->tmplpc_InsName;
			}
			Tags[ti].ta_Tag = tmplpc->tmplpc_InsMode;
			Tags[ti++].ta_Arg = (void *) dtmp;
			Tags[ti].ta_Tag = TAG_END;
    		tmplpc->tmplpc_TemplateItem = CreateItem( MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE), Tags );
    		Result = tmplpc->tmplpc_TemplateItem;
/* Did we find a registered function ID. */
    		if(dtmp->dtmp_FunctionID > 0)
    		{
    			tmplpc->tmplpc_Flags |= TMPLPC_FLAG_DFID_FOUND;
    		}
			break;
			
/* Load a DSP specific shared library template. 940609 ------------------- */
		case ID_DSPS:		
			dtmp = (DSPPTemplate *) UserMemAlloc( sizeof(DSPPTemplate), MEMTYPE_FILL );
			if(dtmp == NULL)
			{
				Result = AF_ERR_NOMEM;
				goto error;
			}
/* parse chunks in DSPP FORM using DSPPHandleInsChunk */
			iffc->iffc_ChunkHandler = DSPPHandleInsChunk;
			iffc->iffc_UserContext = (void *) dtmp;
			Result = iffScanChunks( iffc, FormSize );
			CHECKRSLT(("Handle3INSForm: Error scanning DSPP = 0x%x\n", Result));
			
			Tags[0].ta_Tag = TAG_ITEM_NAME;
			Tags[0].ta_Arg = (void *) "SharedLibrary";  /* %Q Is this OK? */
			Tags[1].ta_Tag = tmplpc->tmplpc_InsMode;
			Tags[1].ta_Arg = (void *) dtmp;
			Tags[2].ta_Tag = AF_TAG_USED_BY;
			Tags[2].ta_Arg = (void *) tmplpc->tmplpc_TemplateItem;
			Tags[3].ta_Tag = TAG_END;
    		{
    			Item LibraryTemplateItem;
    			
    			LibraryTemplateItem = CreateItem( MKNODEID(AUDIONODE,AUDIO_TEMPLATE_NODE), Tags );
    			Result = LibraryTemplateItem;
 DBUG(("Library template created = 0x%x\n", LibraryTemplateItem ));
    			if( Result < 0 ) goto error;
/* Give item to audiofolio so it can be shared. */
    			Result = SetItemOwner( LibraryTemplateItem, AudioBase->af_Folio.fn.n_Owner );
    			if( Result < 0 )
    			{
    				ERR(("SetItemOwner failed!\n"));
    				DeleteItem( LibraryTemplateItem );
    				goto error;
    			}
    		}
    		break;

		default:
/*			ERR(("Unrecognized 3INS FORM = 0x%x\n", FormType)); */
/*			Result = AF_ERR_BADOFX; */
/*			goto error; */
			break;
	}
	
error:
/* We're done with the USER mode template so get rid of it.
** Moved to here to prevent memory leak if error causes jump to here. */
	if( dtmp)
	{
		StripInsTemplate ( dtmp );
		UserMemFree( dtmp, sizeof(DSPPTemplate) );
		dtmp = NULL;
	}
		
	iffc->iffc_UserContext = tmplpc;
	iffc->iffc_ChunkHandler = OldChunkHandler;
	iffc->iffc_FormHandler = OldFormHandler;
		
	return Result;
}

/******************************************************************/
int32 Handle3INSChunk ( iff_control *iffc, uint32 ChunkType , uint32 ChunkSize )
{
	int32 Result;
/*	char *name; */
	TemplateParserContext *tmplpc;
	
	tmplpc = (TemplateParserContext *) iffc->iffc_UserContext;
	Result = 0;
	
DBUG(("Handle3INSChunk: %.4s, %d\n",  &ChunkType, ChunkSize));

/* ATSM, DSPP and other FORMs handled by Handle3INSForm */
	switch(ChunkType)
	{
		case ID_NAME:
			/* %Q Not used! */
/*			name = (char *) EZMemAlloc(ChunkSize+8, MEMTYPE_FILL); */
/*			if (name == 0) goto error;		 */
/*			Result = iffReadChunkData(iffc, name, ChunkSize); */
/*			CHECKRSLT(("Error reading CHUNK data = %d\n", Result)); */
/*			name[ChunkSize] = '\0'; */
/*			DBUG(("Name = %s\n", name)); */
/*			EZMemFree( name ); */
			break;

		case ID_CRDC:
			tmplpc->tmplpc_Flags |= TMPLPC_FLAG_CRDC_FOUND;
			break;
			
		default:
DBUG(("Handle3INSChunk: saw %.4s\n", &ChunkType));
			break;
	}
	
	return Result;
}

/*******************************************************************
** Create an instrument from a template.
** Don't check Library instruments.
*******************************************************************/
static Item superAllocInstrumentSpecial ( Item Template, uint8 Priority, int32 Specialness )
{
	TagArg Tags[4];
	
	Tags[0].ta_Tag = AF_TAG_TEMPLATE;
	Tags[0].ta_Arg = (void *) Template;
	Tags[1].ta_Tag = AF_TAG_PRIORITY;
	Tags[1].ta_Arg = (void *) Priority;
	Tags[2].ta_Tag = AF_TAG_SPECIAL;
	Tags[2].ta_Arg = (void *) Specialness;
	Tags[3].ta_Tag = TAG_END;
	
    return SuperCreateItem( MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE), &Tags[0] );
}

/*******************************************************************
** Scan referenced library templates and call callback for each one found.
*******************************************************************/
static Err ScanLibraryTemplates( DSPPTemplate *dtmp, int32 (*ScanHook)(Item LibraryTemplate) )
{
	int32 Result = 0;
	AudioReferenceNode *arnd;
	
/* Get first node and traverse list. */
	arnd = (AudioReferenceNode *) FirstNode(&dtmp->dtmp_LibraryTemplateRefs);
	while (ISNODE( &dtmp->dtmp_LibraryTemplateRefs, (Node *) arnd))
	{
DBUG(("ScanLibraryTemplates: arnd = 0x%x, Call ScanHook for 0x%x\n", arnd, arnd->arnd_RefItem ));
		Result = (*ScanHook)(arnd->arnd_RefItem);
DBUG(("ScanLibraryTemplates: Result = 0x%x\n", Result ));
		if( Result < 0 ) return Result;
		arnd = (AudioReferenceNode *) NextNode( (Node *) arnd );
	}

	return Result;
}

/*******************************************************************
** Callback for scanner for Alloc. 940609
*******************************************************************/
static int32 AllocIfNoneScanHook(Item LibraryTemplate)
{
	AudioInsTemplate *lib_aitp;
	int32 Result = 0;
				
DBUG(("AllocIfNoneScanHook( Template = 0x%x )\n", LibraryTemplate ));

	lib_aitp = (AudioInsTemplate *) CheckItem(LibraryTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
/* #ifdef PARANOID %Q */
	if( lib_aitp == NULL )
	{
		ERR(("AllocIfNoneScanHook: bogus library template.\n"));
		return AF_ERR_BADITEM;
	}
/* #endif */

/* If the list is empty, no instruments. */
	if( IsEmptyList( &lib_aitp->aitp_InstrumentList ) )
	{
DBUG(("AllocIfNoneScanHook: need to allocate library instrument from template = 0x%x\n",
		LibraryTemplate ));
		Result = superAllocInstrumentSpecial( LibraryTemplate, 0, AF_SPECIAL_NOT );
		if( Result < 0 )
		{
			ERR(("CheckLibraryTemplates: could not allocate library instrument.\n"));
			return Result;
		}
	}
	return Result;
}

/*******************************************************************
** Allocate a half rate splitter if needed. 940822
*******************************************************************/
static Err OpenSplitExec( void )
{
	int32 Result = 0;
	
	if( AudioBase->af_SplitExecIns == 0 )
	{
		Result = superAllocInstrumentSpecial( gSplitExecTemplate, 100, AF_SPECIAL_SPLIT );
DBUG(("OpenSplitExec: 0x%x = superAllocInstrumentSpecial\n", Result ));
		if( Result <= 0 )
		{
			ERR(("OpenSplitExec: could not alloc audio split instrument!\n"));
			goto error;
			
		}
		
		AudioBase->af_SplitExecIns = Result;
		AudioBase->af_SplitExecCount = 1;
		
		Result = swiStartInstrument( AudioBase->af_SplitExecIns, NULL );
DBUG(("OpenSplitExec: 0x%x = swiStartInstrument\n", Result ));
		if( Result < 0 )
		{
			ERR(("OpenSplitExec: could not start audio split instrument!\n"));
			goto error;
		}
	}
	else
	{
		AudioBase->af_SplitExecCount++;
	}
DBUG(("OpenSplitExec: AudioBase->af_SplitExecCount = %d\n", AudioBase->af_SplitExecCount ));

error:
	return Result;
}

/*******************************************************************
** Close a half rate splitter if last used. 940822
*******************************************************************/
static Err CloseSplitExec( void )
{
	int32 Result = 0;
	
	if( --AudioBase->af_SplitExecCount <= 0 )
	{

		Result = SuperInternalDeleteItem( AudioBase->af_SplitExecIns );
		if( Result < 0 )
		{
			ERR(("CloseSplitExec: could not delete audio split instrument: 0x%x!\n", Result));
			goto error;
			
		}
		
		AudioBase->af_SplitExecIns = 0;
	}
DBUG(("Close: AudioBase->af_SplitExecCount = %d\n", AudioBase->af_SplitExecCount ));
	
error:
	return Result;
}
	
/*******************************************************************
** Callback for scanner for Free. 940609
*******************************************************************/
static int32 FreeIfDoneScanHook(Item LibraryTemplate)
{
	AudioInsTemplate *aitp;
	AudioInstrument *ains;
	int32 Result = 0;
				
	aitp = (AudioInsTemplate *) CheckItem(LibraryTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
/* #ifdef PARANOID %Q */
	if( aitp == NULL )
	{
		ERR(("FreeIfDoneScanHook: bogus library template.\n"));
		return AF_ERR_BADITEM;
	}
/* #endif */
	
/* Get first and only instrument to see if it has references. */

	ains = (AudioInstrument *)FirstNode(&aitp->aitp_InstrumentList);
	if( ISNODE(&aitp->aitp_InstrumentList,ains) )
	{
		DSPPInstrument *dins;
		
		dins = (DSPPInstrument *) ains->ains_DeviceInstrument;
		if( DSPPSumResourceReferences( dins ) == 0 )
		{
DBUG(("FreeIfDoneScanHook: Sum==0 so delete 0x%x\n", ains->ains_Item.n_Item ));
			Result = SuperInternalDeleteItem( ains->ains_Item.n_Item );
		}
	}
	
	return Result;
}

/*******************************************************************
** Create an instrument from a template.
** First create any needed Library instruments.
*******************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/allocinstrument
 |||	AllocInstrument - Allocates an instrument using a template.
 |||
 |||	  Synopsis
 |||
 |||	    Item AllocInstrument (Item InsTemplate, uint8 Priority)
 |||
 |||	  Description
 |||
 |||	    This procedure allocates an Instrument based on an instrument Template,
 |||	    previously loaded using LoadInsTemplate() or similar call, and allocates
 |||	    the DSP resources necessary for the Instrument. See also the convenience
 |||	    function LoadInstrument() combines these two steps.
 |||
 |||	    Call FreeInstrument() when you're finished with the Instrument to
 |||	    deallocate its resources.
 |||
 |||	    AllocInstrument() and FreeInstrument() are special cases of the more
 |||	    CreateInstrument() and DeleteInstrument(), which offer greater flexibility.
 |||
 |||	  Arguments
 |||
 |||	    InsTemplate                  Item number of a loaded instrument template
 |||	                                 used to allocate the instrument.
 |||
 |||	    Priority                     Determines order of execution in DSP. Set
 |||	                                 from 0 to 200. A typical value would be
 |||	                                 100. This value also determines the
 |||	                                 priority over other instruments when voices
 |||	                                 are stolen for dynamic voice allocation.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the Item number for the Instrument (a positive value)
 |||	    or an error code (a negative value) if an error occurs.
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
 |||	    FreeInstrument(), CreateInstrument(), LoadInsTemplate(), LoadInstrument()
 |||
 **/
Item AllocInstrument ( Item Template, uint8 Priority )
{
	return AllocInstrumentSpecial( Template, Priority, 0 );
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/deleteinstrument
 |||	DeleteInstrument - Frees an instrument and its resources allocated by CreateInstrument().
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteInstrument (Item InstrumentItem)
 |||
 |||	  Description
 |||
 |||	    This procedure frees an instrument allocated by CreateInstrument(), which
 |||	    frees the resources allocated for the instrument. If the instrument is
 |||	    running, it is stopped. All of the Attachments, Knobs, and Probes belonging
 |||	    to this Instrument are also deleted by this function. The Envelopes and
 |||	    Samples at the other end of Attachments are not deleted by this function.
 |||
 |||	    If you delete an instrument Template, all Instruments created using that
 |||	    Template are freed, so you don't need to use DeleteInstrument() on them.
 |||
 |||	    Do not confuse this function with UnloadInstrument(). If you create an
 |||	    Instrument using LoadInstrument(), you shouldn't use DeleteInstrument() to
 |||	    free the instrument because it will leave the instrument's Template loaded
 |||	    and without access to unload the Template. Use UnloadInstrument() instead.
 |||
 |||	    This function is to CreateInstrument() as FreeInstrument() is to
 |||	    AllocInstrument().
 |||
 |||	  Arguments
 |||
 |||	    InstrumentItem               The item number of the instrument to free.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V24.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    CreateInstrument(), FreeInstrument(), UnloadInstrument(), UnloadInsTemplate()
 |||
 **/

Err DeleteInstrument( Item Instrument )
{
	return DeleteItem( Instrument );
}

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/freeinstrument
 |||	FreeInstrument - Frees an instrument and its resources allocated by
 |||	                 AllocInstrument().
 |||
 |||	  Synopsis
 |||
 |||	    Err FreeInstrument (Item InstrumentItem)
 |||
 |||	  Description
 |||
 |||	    This procedure frees an instrument allocated by AllocInstrument(), which
 |||	    frees the resources allocated for the instrument. If the instrument is
 |||	    running, it is stopped. All of the Attachments, Knobs, and Probes belonging
 |||	    to this Instrument are also deleted by this function. The Envelopes and
 |||	    Samples at the other end of Attachments are not deleted by this function.
 |||
 |||	    If you delete an instrument Template, all Instruments created using that
 |||	    Template are freed, so you don't need to use DeleteInstrument() on them.
 |||
 |||	    Do not confuse this function with UnloadInstrument(). If you create an
 |||	    Instrument using LoadInstrument(), you shouldn't use DeleteInstrument() to
 |||	    free the instrument because it will leave the instrument's Template loaded
 |||	    and without access to unload the Template. Use UnloadInstrument() instead.
 |||
 |||	    AllocInstrument() and FreeInstrument() are special cases of the more
 |||	    CreateInstrument() and DeleteInstrument(), which offer greater flexibility.
 |||
 |||	  Arguments
 |||
 |||	    InstrumentItem               The item number of the instrument to free.
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
 |||	    AllocInstrument(), DeleteInstrument(), UnloadInstrument(), UnloadInsTemplate()
 |||
 **/


/*******************************************************************/
Item AllocInstrumentSpecial ( Item Template, uint8 Priority, int32 Specialness )
{
	TagArg Tags[4];
	
	Tags[0].ta_Tag = AF_TAG_TEMPLATE;
	Tags[0].ta_Arg = (void *) Template;
	Tags[1].ta_Tag = AF_TAG_PRIORITY;
	Tags[1].ta_Arg = (void *) Priority;
	Tags[2].ta_Tag = AF_TAG_SPECIAL;
	Tags[2].ta_Arg = (void *) Specialness;
	Tags[3].ta_Tag = TAG_END;
	
    return CreateItem( MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE), &Tags[0] );
}


/*****************************************************************/
int32 internalGetInstrumentInfo (AudioInstrument *ains, TagArg *args)
{
  	int32 Result = 0;  	
	uint32 tagc, *tagp;
  	
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("internalGetInstrumentInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{
			case AF_TAG_PRIORITY:
				*tagp++ = (uint32) ains->ains_Item.n_Priority;
				break;
				
			case AF_TAG_STATUS:
				*tagp++ = (uint32) ains->ains_Status;
				break;
				
			case AF_TAG_START_TIME:
				*tagp++ = (uint32) ains->ains_StartTime;
				break;
			
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Warning - unrecognized tag in internalGetInstrumentInfo - 0x%x: 0x%x\n",
					tagc, *tagp));	
					Result = AF_ERR_BADTAG;
					goto DONE;
				}
				tagp++;
			}
		}
	}
DONE:
	return Result;
}

/******************************************************************/
/***** SUPERVISOR MODE ********************************************/
/******************************************************************/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/startinstrument
 |||	StartInstrument - Begins playing an instrument (Note On).
 |||
 |||	  Synopsis
 |||
 |||	    Err StartInstrument (Item Instrument, TagArg *tagList)
 |||
 |||	    Err StartInstrumentVA (Item Instrument, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure begins execution of an instrument. This typically starts a
 |||	    sound but may have other results, depending on the nature of the
 |||	    instrument. This call links the DSP code into the list of active
 |||	    instruments. If the instrument has Samples or Envelopes attached, they will
 |||	    also be started (unless the Attachments specify otherwise). This is
 |||	    equivalent to a MIDI "Note On" event.
 |||
 |||	    The Amplitude and Frequency knobs, of instruments that have such, can
 |||	    be tweaked by some of the tags listed below before the instrument is
 |||	    started. When none of the tags for a particular know are specified, that
 |||	    knob is left set to its previous value. At most one tag for each knob can
 |||	    be specified. Tags are ignored for Instruments without the corresponding
 |||	    knob. Knobs connected to the output of another Instrument
 |||	    (ConnectInstruments()), cannot be set in this manner. A Knob that has been
 |||	    grabbed, can however be set in this manner.
 |||
 |||	    This function puts the instrument in the AF_STARTED state. If the instrument
 |||	    was previous running, it is first stopped and then restarted. If the instrument
 |||	    has a sustain or release loop, it stays in the AF_STARTED state until the
 |||	    state is explicitly changed (e.g. ReleaseInstrument(), StopInstrument()).
 |||
 |||	    This function supercedes a call to PauseInstrument().
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number for the instrument.
 |||
 |||	  Tags
 |||
 |||	    Amplitude:
 |||
 |||	    AF_TAG_AMPLITUDE             (uint32) Value to set instrument's
 |||	                                 Amplitude knob. Valid range is 0..0x7fff.
 |||
 |||	    AF_TAG_VELOCITY              (uint32) MIDI note velocity
 |||	                                 indicating the value to set instrument's
 |||	                                 Amplitude knob. Valid range is 0..127.
 |||
 |||	    Frequency:
 |||
 |||	    AF_TAG_FREQUENCY             (ufrac16) Value to set Frequency knob
 |||	                                 to in 16.16 Hz.
 |||
 |||	    AF_TAG_PITCH                 (uint32) Value to set Frequency knob
 |||	                                 expressed as a MIDI note number. The range
 |||	                                 is 0 to 127; 60 is middle C. For multisample
 |||	                                 instruments, picks the sample associated with
 |||	                                 the MIDI pitch number.
 |||
 |||	    AF_TAG_RATE                  (uint32) Value to set Frequency knob
 |||	                                 to in instrument-specific frequency units
 |||	                                 (e.g. phase incrment, proportion of original
 |||	                                 sample rate).
 |||
 |||	    Other:
 |||
 |||	    AF_TAG_TIME_SCALE            (ufrac16) Scale times for
 |||	                                 all Envelopes attached to this Instrument.
 |||	                                 Original value is derived from the
 |||	                                 AF_TAG_TIME_SCALE provided when the Envelope
 |||	                                 Attachment was made. This value remains in
 |||	                                 effect until another AF_TAG_TIME_SCALE is
 |||	                                 passed to StartInstrument() or
 |||	                                 ReleaseInstrument().
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
 |||	  Caveats
 |||
 |||	    Ideally this function should also use the MIDI note velocity specified
 |||	    by AF_TAG_VELOCITY to pick samples in a multisample (e.g. a pp piano
 |||	    sample versus a ff piano sound) based on AF_TAG_HIGHVELOCITY and
 |||	    AF_TAG_LOWVELOCITY sample settings. At present multisample selection
 |||	    ignores AF_TAG_HIGHVELOCITY and AF_TAG_LOWVELOCITY sample settings.
 |||	    This features cannot be added such that it would affect existing
 |||	    applications. If it were added, there would be some mechanism to enable it,
 |||	    for example a new tag for either Samples or StartInstrument().
 |||
 |||	  See Also
 |||
 |||	    ReleaseInstrument(), StopInstrument(), PauseInstrument(),
 |||	    ResumeInstrument(), Instrument
 |||
 **/
int32 swiStartInstrument ( Item InstrumentItem, TagArg *args)
{
	int32 Result;
	AudioInstrument *ains;

TRACEE(TRACE_TOP, TRACE_TOP|TRACE_NOTE,
	("swiStartInstrument ( 0x%x, 0x%x )\n", InstrumentItem, args));

DBUG(("swiStartInstrument ( 0x%x, 0x%x )\n", InstrumentItem, args));
	CHECKAUDIOOPEN;

	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	
TRACEB(TRACE_INT, TRACE_NOTE,("swiStartInstrument: ains = 0x%x \n", ains));

/* Stop instrument if already running. */
	if( ains->ains_Status > AF_STOPPED )
	{
		Result = swiStopInstrument( InstrumentItem, NULL );
		if( Result < 0 ) return Result;
	}

/* Return positive if did not start but no error. */
	Result = DSPPStartInstrument(ains, args);
TRACER(TRACE_INT, TRACE_NOTE, ("swiStartInstrument returns 0x%x\n", Result));
	if (Result) return Result;
	
	ains->ains_Status = AF_STARTED;
	ains->ains_StartTime = AudioBase->af_Time;
	
	return 0;
}

/*************************************************************************/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/connectinstruments
 |||	ConnectInstruments - Patches the output of an instrument to the input of
 |||	                     another instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Err ConnectInstruments (Item SrcIns, char *SrcName, Item DstIns,
 |||	                            char *DstName)
 |||
 |||	  Description
 |||
 |||	    This procedure connects an output from one instrument to an input of
 |||	    another instrument. This allows construction of complex "patches"
 |||	    from existing synthesis modules.
 |||
 |||	    An output can be connected to one or more inputs; only one output can be
 |||	    connected to any given input. If you connect an output to a knob, it
 |||	    disconnects that knob from any possible control by TweakKnob() or
 |||	    TweakRawKnob(). Unlike Attachments, this kind of connection does not create
 |||	    an Item.
 |||
 |||	    Call DisconnectInstruments() to break a connection set up by this function.
 |||	    Deleting either instrument will cause the connection to automatically
 |||	    be broken.
 |||
 |||	    See the DSP Template pages for complete listings of each template's inputs,
 |||	    outputs and knobs.
 |||
 |||	  Arguments
 |||
 |||	    SrcIns                       Item number of the source instrument.
 |||
 |||	    SrcName                      Name of the output port of the source
 |||	                                 instrument to connect to.
 |||
 |||	    DstIns                       Item number of the destination instrument.
 |||
 |||	    DstName                      Name of the input port of the destination
 |||	                                 instrument to connect to.
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
 |||	  Notes
 |||
 |||	    Port names are matched case-insensitively.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    DisconnectInstruments()
 |||
 **/
int32 swiConnectInstruments  ( Item SrcIns, char *SrcName, Item DstIns, char *DstName)
{
	AudioInstrument *ains_src, *ains_dst;
	DSPPInstrument *dins_src, *dins_dst;
	int32 Result;
	
DBUG(("ConnectInstruments: 0x%x,%s ", SrcIns, SrcName ));
DBUG(("   =>   0x%x,%s\n", DstIns, DstName ));

	ains_src = (AudioInstrument *)CheckItem(SrcIns, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains_src == NULL)
	{
		ERR(("swiConnectInstruments: bad SrcIns = 0x%x\n", SrcIns));
		return AF_ERR_BADITEM;
	}
	dins_src = (DSPPInstrument *)ains_src->ains_DeviceInstrument;

	ains_dst = (AudioInstrument *)CheckItem(DstIns, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains_dst == NULL)
	{
		ERR(("swiConnectInstruments: bad DstIns = 0x%x\n", DstIns));
		return AF_ERR_BADITEM;
	}

	dins_dst = (DSPPInstrument *)ains_dst->ains_DeviceInstrument;
	
/* Verify name addresses. */
	Result = afi_IsRamAddr( SrcName, 1);
	if(Result < 0) return Result;
	Result = afi_IsRamAddr( DstName, 1);
	if(Result < 0) return Result;

/* Call DSPP */
	return DSPPConnectInstruments (dins_src, SrcName, dins_dst, DstName );
}

/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/disconnectinstruments
 |||	DisconnectInstruments - Disconnects one instrument from another.
 |||
 |||	  Synopsis
 |||
 |||	    Err DisconnectInstruments (Item SrcIns, char *SrcName, Item DstIns,
 |||	                               char *DstName)
 |||
 |||	  Description
 |||
 |||	    This procedure breaks a connection made by ConnectInstruments() between
 |||	    two instruments. If the connection was to a knob of the second
 |||	    instrument, the knob is once again available for tweaking.
 |||
 |||	  Arguments
 |||
 |||	    SrcIns                       Item number of the source instrument.
 |||
 |||	    SrcName                      Name of the output port of the source
 |||	                                 instrument to break connection from.
 |||
 |||	    DstIns                       Item number of the destination instrument.
 |||
 |||	    DstName                      Name of the input port of the destination
 |||	                                 instrument to break connection to.
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
 |||	    ConnectInstruments()
 |||
 **/
int32 swiDisconnectInstruments  ( Item SrcIns, char *SrcName, Item DstIns, char *DstName)
{
	AudioInstrument *ains_src, *ains_dst;
	DSPPInstrument *dins_src, *dins_dst;
	int32 Result;
	
	ains_src = (AudioInstrument *)CheckItem(SrcIns, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains_src == NULL) return AF_ERR_BADITEM;
	dins_src = (DSPPInstrument *)ains_src->ains_DeviceInstrument;

	ains_dst = (AudioInstrument *)CheckItem(DstIns, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains_dst == NULL) return AF_ERR_BADITEM;
	dins_dst = (DSPPInstrument *)ains_dst->ains_DeviceInstrument;
	
/* Verify names. */
	Result = afi_IsRamAddr( SrcName, 1);
	if(Result < 0) return Result;
	Result = afi_IsRamAddr( DstName, 1);
	if(Result < 0) return Result;

/* Call DSPP */
	return DSPPDisconnectInstruments (dins_src, SrcName, dins_dst, DstName );
}

/*************************************************************************/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/releaseinstrument
 |||	ReleaseInstrument - Instruct an instrument to begin to finish (Note Off).
 |||
 |||	  Synopsis
 |||
 |||	    Err ReleaseInstrument (Item Instrument, TagArg *tagList)
 |||
 |||	    Err ReleaseInstrumentVA (Item Instrument, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This tells an Instrument to progress into its "release phase" if it hasn't
 |||	    already done so. This is equivalent to a MIDI Note Off event. Any Samples
 |||	    or Envelopes that are attached are set to their release portion, which may
 |||	    or may not involve a release loop. The sound may continue to be produced
 |||	    indefinitely depending on the release characteristics of the instrument.
 |||
 |||	    This has no audible effect on instruments that don't have some kind of
 |||	    sustain loop (e.g. sawtooth.dsp).
 |||
 |||	    Affects only instruments in the AF_STARTED state: sets them to the
 |||	    AF_RELEASED state. By default if and when an instrument reaches the end of
 |||	    its release phase, it stays in the AF_RELEASED state until explicitly
 |||	    changed (e.g. StartInstrument(), StopInstrument()). If one of this
 |||	    Instrument's running Attachments has the AF_ATTF_FATLADYSINGS flag set, the
 |||	    Instrument is automatically stopped when that Attachment completes. In
 |||	    that case, the instrument goes into the AF_STOPPED state, or, if the
 |||	    Instrument has the AF_INSF_AUTOABANDON flag set, the AF_ABANDONED state.
 |||
 |||	    !!! not sure of the effect this has on a paused instrument
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number for the instrument.
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_TIME_SCALE            (ufrac16) Scale times for
 |||	                                 all Envelopes attached to this Instrument.
 |||	                                 Original value is derived from the
 |||	                                 AF_TAG_TIME_SCALE provided when the Envelope
 |||	                                 Attachment was made. This value remains in
 |||	                                 effect until another AF_TAG_TIME_SCALE is
 |||	                                 passed to StartInstrument() or
 |||	                                 ReleaseInstrument().
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
 |||	    StartInstrument(), StopInstrument(), PauseInstrument(),
 |||	    ResumeInstrument(), Instrument
 |||
 **/
int32 swiReleaseInstrument ( Item InstrumentItem, TagArg *args)
/*
** Allows an event to move toward completion and then free its voice.  Some 
** sounds will need time to die out before stopping completely.
*/
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;  /* 00001 */
	if (ains->ains_Status < AF_RELEASED) return 0;
	
DBUG(("swiReleaseInstrument( 0x%x )\n", InstrumentItem));
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	ains->ains_Status = AF_RELEASED;
	return DSPPReleaseInstrument(dins, args);
}

/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/stopinstrument
 |||	StopInstrument - Abruptly stops an instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Err StopInstrument (Item Instrument, TagArg *tagList)
 |||
 |||	    Err StopInstrumentVA (Item Instrument, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    This procedure, which abruptly stops an instrument, is called when you
 |||	    want to abort the execution of an instrument immediately. This can cause
 |||	    a click because of its suddenness. You should use ReleaseInstrument() to
 |||	    gently release an instrument according to its release characteristics.
 |||
 |||	    Affects only instruments in the AF_STARTED or AF_RELEASED states: sets them
 |||	    to the AF_STOPPED state, or, if the Instrument has the AF_INSF_AUTOABANDON
 |||	    flag set, the AF_ABANDONED state.
 |||
 |||	    This function supercedes a call to PauseInstrument().
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number for the instrument.
 |||
 |||	  Tags
 |||
 |||	    None
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
 |||	    StartInstrument(), ReleaseInstrument(), PauseInstrument(),
 |||	    ResumeInstrument(), Instrument
 |||
 **/
int32 swiStopInstrument ( Item InstrumentItem, TagArg *args)
/*
** Immediately stop an instrument
*/
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	int32 Result;
	
DBUG(("StopInstrument( 0x%x )\n", InstrumentItem));
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if( ains == NULL) return AF_ERR_BADITEM;
	if( args ) return AF_ERR_BADTAG;  /* 930831 */
	if( ains->ains_Status <= AF_STOPPED) return 0;
	
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	ains->ains_Status = AF_STOPPED;
	Result = DSPPStopInstrument(dins, args);
	if (Result < 0) return Result;
	if (ains->ains_Flags & AF_INSF_AUTOABANDON)
	{
		ains->ains_Status = AF_ABANDONED;
	}
	return Result;
}

/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/pauseinstrument
 |||	PauseInstrument - Pauses an instrument's playback.
 |||
 |||	  Synopsis
 |||
 |||	    Err PauseInstrument (Item Instrument)
 |||
 |||	  Description
 |||
 |||	    This procedure pauses an instrument during playback. A paused
 |||	    instrument ceases to play, but retains its position in playback so
 |||	    that it can resume playback at that point. ResumeInstrument() allows
 |||	    a paused instrument to continue its playback.
 |||
 |||	    This procedure is intended primarily for sampled-sound instruments,
 |||	    where a paused instrument retains its playback position within a
 |||	    sampled sound. PauseInstrument() and ResumeInstrument() used with
 |||	    sound-synthesis instruments may not have effects any different from
 |||	    StartInstrument() and StopInstrument().
 |||
 |||	    The paused state is superceded by a call to StartInstrument() or
 |||	    StopInstrument().
 |||
 |||	    !!! unsure if ReleaseInstrument() affects a paused instrument.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number for the instrument.
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
 |||	  Caveats
 |||
 |||	    This procedure will not pause an envelope attached to an instrument. The
 |||	    envelope will continue to play while the instrument is paused.
 |||
 |||	  See Also
 |||
 |||	    ResumeInstrument(), StartInstrument(), StopInstrument(), ReleaseInstrument()
 |||
 **/
int32 swiPauseInstrument ( Item InstrumentItem )
/*
** Pause an instrument
*/
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	int32 Result;
	
PRTX(("PauseInstrument( 0x%x )\n", InstrumentItem ));
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	if (ains->ains_Status <= AF_STOPPED) return 0;
	
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	Result = DSPPPauseInstrument(dins);
	return Result;
}
/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/resumeinstrument
 |||	ResumeInstrument - Resumes playback of a paused instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Err ResumeInstrument (Item Instrument)
 |||
 |||	  Description
 |||
 |||	    This procedure resumes playback of an instrument paused using
 |||	    PauseInstrument(). A resumed instrument continues playback from the point
 |||	    where it was paused. It does not restart from the beginning of a note.
 |||
 |||	    This procedure is intended primarily for sampled-sound instruments, where
 |||	    a paused instrument retains its playback position within a sampled sound.
 |||	    PauseInstrument() and ResumeInstrument() used with sound-synthesis
 |||	    instruments may not have effects any different than StartInstrument() and
 |||	    StopInstrument().
 |||
 |||	    This function has no effect on an instrument that has been stopped
 |||	    or restarted after being paused.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number for the instrument.
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
 |||	    PauseInstrument(), StartInstrument(), StopInstrument(), ReleaseInstrument()
 |||
 **/
int32 swiResumeInstrument ( Item InstrumentItem )
/*
** Pause an instrument
*/
{
	AudioInstrument *ains;
	DSPPInstrument *dins;
	int32 Result;
	
PRTX(("ResumeInstrument( 0x%x )\n", InstrumentItem ));
	ains = (AudioInstrument *)CheckItem(InstrumentItem, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	if (ains->ains_Status <= AF_STOPPED) return 0;
	
	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	Result = DSPPResumeInstrument(dins);
	return Result;
}

/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/abandoninstrument
 |||	AbandonInstrument - Make an instrument available for adoption.
 |||
 |||	  Synopsis
 |||
 |||	    Err AbandonInstrument (Item Instrument)
 |||
 |||	  Description
 |||
 |||	    This function together with AdoptIntrument() form a simple, but
 |||	    efficient, voice allocation system for a single instrument Template.
 |||	    AbandonInstrument() adds an instrument to a pool of unused
 |||	    instruments; AdoptInstrument() allocates instruments from that pool. A
 |||	    Template's pool can grow or shrink dynamically.
 |||
 |||	    Why should you use this system when CreateInstrument() and
 |||	    DeleteInstrument() also do dynamic voice allocation? The answer is
 |||	    that CreateInstrument() and DeleteInstrument() create and delete Items
 |||	    and allocate and free DSP resources. AdoptInstrument() and
 |||	    AbandonInstrument() merely manage a pool already existing Instrument
 |||	    Items belonging to a template. Therefore they don't have the overhead
 |||	    of Item creation and DSP resource management, and don't thrash the Item
 |||	    table.
 |||
 |||	    This function stops the instrument, if it was running, and sets its
 |||	    status to AF_ABANDONED (see GetAudioItemInfo() AF_TAG_STATUS). This
 |||	    instrument is now available to be adopted from its Template by calling
 |||	    AdoptInstrument().
 |||
 |||	    Instruments created with the AF_INSF_AUTOABANDON flag set, are
 |||	    automatically put into the AF_ABANDONED state when stopped.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number for the instrument to
 |||	                                 abandon.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
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
 |||	    AdoptInstrument(), StopInstrument(), ScavengeInstrument(),
 |||	    GetAudioItemInfo(), Instrument
 |||
 **/
Err swiAbandonInstrument( Item Instrument )
{
	AudioInstrument *ains;
	Err Result = 0;
	
	ains = (AudioInstrument *)CheckItem(Instrument, AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if (ains == NULL) return AF_ERR_BADITEM;
	if (ains->ains_Status > AF_STOPPED)
	{
		Result = swiStopInstrument( Instrument, NULL );
	}
	
	ains->ains_Status = AF_ABANDONED;
	return Result;
}


/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/adoptinstrument
 |||	AdoptInstrument - Adopt an abandoned instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Item AdoptInstrument (Item instrumentTemplate)
 |||
 |||	  Description
 |||
 |||	    This function adopts an instrument from this template's abandoned
 |||	    instrument pool (finds an instrument belonging to the the template
 |||	    whose status is AF_ABANDONED). It then sets the instrument state to
 |||	    AF_STOPPED and returns the instrument item number. If the template has
 |||	    no abandoned instruments, this function returns 0.
 |||
 |||	    Note that this function does not create a new Instrument Item; it
 |||	    returns an Instrument Item that was previously passed to
 |||	    AbandonInstrument() (or became abandoned because AF_INSF_AUTOABANDON
 |||	    was set).
 |||
 |||	    This function together with AbandonIntrument() form a simple, but
 |||	    efficient, voice allocation system for a single instrument Template.
 |||
 |||	  Arguments
 |||
 |||	    instrumentTemplate           The item number for the instrument
 |||	                                 template from which to to attempt to
 |||	                                 adopt an instrument.
 |||
 |||	  Return Value
 |||
 |||	    >0                           Instrument Item number if an instrument
 |||	                                 could be adopted.
 |||
 |||	    0                            If no abandoned instruments in template.
 |||
 |||	    <0                           Error code on failure.
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
 |||	    AbandonInstrument(), ScavengeInstrument(), GetAudioItemInfo(),
 |||	    Instrument
 |||
 **/
Item  swiAdoptInstrument( Item InsTemplate )
{	
	AudioInstrument *ains;
	AudioInsTemplate *aitp;

	CHECKAUDIOOPEN;
	
	aitp = (AudioInsTemplate *)CheckItem(InsTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
	if (aitp == NULL) return AF_ERR_BADITEM;
	
/* Scan list of Instruments for next available for adoption. */
	ains = (AudioInstrument *)FirstNode(&aitp->aitp_InstrumentList);
	
	while (ISNODE(&aitp->aitp_InstrumentList,ains))
	{
DBUG(("Checking instrument status 0x%x = %d\n", ains, ains->ains_Status));
		if (ains->ains_Status == AF_ABANDONED)
		{
			ains->ains_Status = AF_STOPPED;
			return (ains->ains_Item.n_Item);
		}
		ains = (AudioInstrument *)NextNode((Node *)ains);
	}

	return 0;
}


/*************************************************************************/
 /* !!! made this function private until its definition gets resolved */
 /**
 |||	AUTODOC PRIVATE mpg/audiofolio/scavengeinstrument
 |||	ScavengeInstrument - Pick best instrument to steal from a template.
 |||
 |||	  Synopsis
 |||
 |||	    Item ScavengeInstrument (Item instrumentTemplate, uint8 maxPriority,
 |||	                             uint32 maxActivity, int32 ifSystemWide)
 |||
 |||	  Description
 |||
 |||	    This function identifies which of the instruments created from this
 |||	    template is the best to steal for a new voice. This function doesn't
 |||	    actually do anything with the voice it picks (i.e. it doesn't stop
 |||	    it, or change its status in any way). It merely returns its Item
 |||	    number.
 |||
 |||	    The voice stealing logic first finds an instrument belonging to the
 |||	    template with the lowest combination of priority and activity level
 |||	    (i.e. AF_TAG_STATUS result: AF_ABANDONED, AF_STOPPED, AF_RELEASED,
 |||	    AF_STARTED etc). (This is admittedly a bit non-deterministic, the
 |||	    voice it picks depends rather heavily on the order in which instruments
 |||	    are added to the template's instrument list). If this still results in
 |||	    more than one instrument, the one with the earliest start time is
 |||	    picked.
 |||
 |||	    Instruments with higher priority than MaxPriority or higher activity
 |||	    level than MaxActivity are not considered valid choices for stealing.
 |||
 |||	    Note that this function does not create a new Instrument Item; it
 |||	    returns an Instrument Item that was previously created from the template.
 |||
 |||	  Arguments
 |||
 |||	    instrumentTemplate           The item number for the instrument
 |||	                                 template from which to to attempt to
 |||	                                 steal an instrument from.
 |||
 |||	    maxPriority                  Maximum instrument priority to consider
 |||	                                 stealing.
 |||
 |||	    maxActivity                  Maximum instrument activity level
 |||	                                 (AF_ABANDONED, AF_STOPPED, AF_RELEASE, or
 |||	                                 AF_STARTED) to consider stealing.
 |||
 |||	    ifSystemWide                 Ignored. !!!
 |||
 |||	  Return Value
 |||
 |||	    >0                           Instrument Item number of best instrument
 |||	                                 to steal.
 |||
 |||	    0                            If no suitable instrument in template.
 |||
 |||	    <0                           Error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    This function currently doesn't disregard instrument priority for
 |||	    instruments whose status is AF_ABANDONED. This causes higher priority
 |||	    abandoned instruments to be passed over in favor of lower priority
 |||	    active instruments. We recommend that you make sure that all
 |||	    instruments belonging to the template passed to this instrument be
 |||	    created at the same priority.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    AbandonInstrument(), AdoptInstrument(), GetAudioItemInfo(),
 |||	    Instrument
 |||
 **/
 
 /**        
    @@@ The following is the voice stealing algorithm used by the score player: 
 
 |||	    The voice stealing logic first finds the instrument belonging to the
 |||	    template with the lowest activity level (i.e. AF_TAG_STATUS result:
 |||	    AF_ABANDONED, AF_STOPPED, AF_RELEASED, AF_STARTED etc). If this
 |||	    results in more than one instrument, the one with the lowest priority
 |||	    below is picked. If this still results in more than one instrument,
 |||	    the one with the earliest start time is picked.
 
 **/

int32 swiScavengeInstrument( Item InsTemplate, uint8 Priority, uint32 MaxActivity, int32 IfSystemWide )
{
	Item Chosen=0;
	AudioInstrument *ains, *nextains, *bestains;
	AudioInsTemplate *aitp;
	uint8  LowestPriority;
	uint32 LowestActivity;
	AudioTime EarliestTime; 
	
DBUG(("swiScavengeInstrument( 0x%x, %d, %d, ...)\n", InsTemplate, Priority, MaxActivity));

	aitp = (AudioInsTemplate *)CheckItem(InsTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
	if (aitp == NULL) return AF_ERR_BADITEM;
	
	bestains = NULL;
	LowestPriority = Priority+1;
	LowestActivity = MaxActivity+1;
	EarliestTime = AudioBase->af_Time;
	
/* Scan list of Instruments for next available for scavenging. */
	ains = (AudioInstrument *)FirstNode(&aitp->aitp_InstrumentList);
	
/* Search for minimal fit. */
	while (ISNODE(&aitp->aitp_InstrumentList,ains))
	{
		nextains = (AudioInstrument *)NextNode((Node *)ains);
DBUG(("Scavenging instrument 0x%x, status %d\n",
		ains->ains_Item.n_Item, ains->ains_Status));
/* 931129 Check for either lower activity or lower priority. */
		if (((ains->ains_Status < LowestActivity) && (ains->ains_Item.n_Priority <= LowestPriority)) ||
			((ains->ains_Item.n_Priority < LowestPriority) && (ains->ains_Status <= LowestActivity)) ||
/* 940606 All other things being equal, check for earliest start time */
			((CompareAudioTimes (ains->ains_StartTime,EarliestTime) < 0) && (ains->ains_Item.n_Priority <= LowestPriority) && 
				(ains->ains_Status <= LowestActivity)))
		
		{
			bestains = ains;
			LowestPriority = ains->ains_Item.n_Priority;
			LowestActivity = ains->ains_Status;
			EarliestTime   = ains->ains_StartTime;
		}
		
		ains = nextains;
	}

	if (bestains)
	{
		Chosen = bestains->ains_Item.n_Item;
	}
	
DBUG(("swiScavengeInstrument returns 0x%x\n", Chosen));
	return Chosen;
}


/*************************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/enableaudioinput
 |||	EnableAudioInput - Enable Anvil audio input.
 |||
 |||	  Synopsis
 |||
 |||	    Err EnableAudioInput (int32 OnOrOff, TagArg *tagList)
 |||
 |||	    Err EnableAudioInputVA (int32 OnOrOff, uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    Enables or disables the use of audio input on systems that have audio
 |||	    input capability (e.g. Anvil systems) for the calling task. The DSP
 |||	    instrument directin.dsp requires that a successful call to enable audio
 |||	    input be made before that instrument can be loaded.
 |||
 |||	    Enable/Disable calls nest.
 |||
 |||	    If you turn on audio input, you do not not need to turn it off as part of
 |||	    your cleanup.
 |||
 |||	  Arguments
 |||
 |||	    OnOrOff                      Non-zero to enable, zero to disable.
 |||	                                 An enable count is maintained for each
 |||	                                 task so that you may nest calls to
 |||	                                 EnableAudioInput (TRUE, ...).
 |||
 |||	  Tags
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    SWI implemented in audio folio V24.
 |||
 |||	  Caveats
 |||
 |||	    EnableAudioInput() only permits a NULL tag list pointer. Therefore,
 |||	    EnableAudioInputVA() always fails with AF_ERR_BADTAG.
 |||
 |||	  Associated Files
 |||
 |||	    audio.h
 |||
 |||	  See Also
 |||
 |||	    directin.dsp
 |||
 **/

#if 0
static void HackAudioInputEnable( int32 OnOrOff)
{
	uint32 RegVal;

/* Get current value of AUDIN as set by ROM. */
	RegVal = (*AUDIN) & 0x3FFFFFFF;
	if (OnOrOff) RegVal |= 0x40000000 ;

/* Set register access enable bit. */
	*AUDIN = 0x80000000;

/* Set enable bit ORed with original bits. */
	*AUDIN = RegVal;
	PRT(("HackAudioInputEnable: Set 0x%x to 0x%x\n", AUDIN, RegVal));
}
#endif

Err swiEnableAudioInput( int32 OnOrOff, TagArg *Tags )
{
	int32 Result = 0, sres;
	AudioFolioTaskData *aftd;
	int32 folioIndex;

DBUG(("swiEnableAudioInput( 0x%x, 0x%x )\n", OnOrOff, Tags ));

/* No tags currently supported. Must only allow NULL. */
	if(Tags) return AF_ERR_BADTAG;
	
/* Support new name for MUD. 940829 */
	Result = SuperQuerySysInfo( SYSINFO_TAG_AUDINPRESENT, NULL , 0 );
DBUG(("SuperQuerySysInfo returns 0x%x\n", Result ));

	if( Result == SYSINFO_AUDIN_PRESENT )
	{
		folioIndex = AudioBase->af_Folio.f_TaskDataIndex;
		aftd = (AudioFolioTaskData *) CURRENTTASK->t_FolioData[folioIndex];
		if( aftd )
		{
			if(OnOrOff)
			{
				if( aftd->aftd_InputEnables == 0 ) /* First enable for task? */
				{
					if( AudioBase->af_InputEnableCount == 0 ) /* First enable for system? */
					{
						sres = SuperSetSysInfo(SYSINFO_TAG_SETAUDINSTATE,
						   (void *) SYSINFO_AUDIN_ENABLE, 0);
						DBUG(("SuperSetSysInfo returns 0x%x\n", sres));
						if( sres != SYSINFO_SUCCESS ) return 0;  /* Not error, just not allowed. */
					}
					AudioBase->af_InputEnableCount++;
				}
			 	aftd->aftd_InputEnables++;
			}	
			else
			{
			 	if( aftd->aftd_InputEnables > 0)
			 	{
			 		if( --(aftd->aftd_InputEnables) == 0 ) /* Last disable for task? */
			 		{
						if( --(AudioBase->af_InputEnableCount) == 0 )  /* Last disable for system? */
						{
							SuperSetSysInfo(SYSINFO_TAG_SETAUDINSTATE,
							   (void *) SYSINFO_AUDIN_DISABLE, 0);
						}
					}
				}	
			}
		}
		else
		{
			Result = -1;  /* This should never happen. */
		}
	}
	return Result;
}

/*************************************************************************/
Item internalCreateAudioIns (AudioInstrument *ains, TagArg *args)
{

	const TagArg *tstate;
	TagArg *t;

	DSPPInstrument *dins;
	DSPPTemplate *dtmp;
	AudioAttachment *aatt;
	AudioInsTemplate *aitp = NULL;
	Item TemplateItem;
	int32 Priority = 100, Flags = 0, Specialness = 0; /* Tagged */
	int32 Result;
	int32 Temp;
	int32 RateShift = 0;

TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioIns(0x%x, 0x%lx)\n", ains, args));

DBUG(("internalCreateAudioIns(0x%lx) is 0x%x\n", ains, ains->ains_Item.n_Item ));

REPORTMEM(("Entered internalCreateAudioIns ----------------\n"));

    Result = TagProcessor( ains, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioIns: TagProcessor failed.\n"));
    	return Result;
    }
    
REPORTMEM(("After TagProcessor\n"));

    TemplateItem = 0;
    
	if (args)       
	{

		for (tstate = args;
			 (t = NextTagArg (&tstate)) != NULL; )
		{
DBUG(("internalCreateAudioIns: Tag = %d, Arg = 0x%x\n", t->ta_Tag, t->ta_Arg));
	        
			switch (t->ta_Tag)
			{
			case AF_TAG_TEMPLATE:
				TemplateItem = 	(Item) t->ta_Arg;
				break;
				
			case AF_TAG_CALCRATE_DIVIDE:
/* Convert divisor to shift */
				if( (int32) t->ta_Arg == 2 )
				{
					RateShift = 1;
				}
				else if ( (int32) t->ta_Arg == 1 )
				{
					RateShift = 0;
				}
				else
				{
					ERR(("AF_TAG_CALCRATE_DIVIDE out of range! %d\n", (int32) t->ta_Arg));
					return AF_ERR_BADTAGVAL;
				}
				break;
				
			case AF_TAG_SPECIAL:
				Specialness = (int32) t->ta_Arg;
				if( (Specialness < 0) || (Specialness > AF_SPECIAL_MAX) )
				{
					ERR(("AF_TAG_SPECIAL out of range! %d\n", Specialness));
					return AF_ERR_BADTAGVAL;
				}
				break;
				
			case AF_TAG_PRIORITY:
				Priority = (int32) t->ta_Arg;
				if((Priority > 255) || (Priority < 0)) /* 930830 */
				{
					ERR(("Instrument priority out of range! %d\n", Priority));
					return AF_ERR_BADTAGVAL;
				}
				break;
				
			case AF_TAG_SET_FLAGS:
				Temp =  (uint32) t->ta_Arg;
				if(Temp & ~AF_INSF_LEGALFLAGS)
				{
					ERR(("Illegal instrument flags. 0x%x\n", Temp));
					return AF_ERR_BADTAGVAL;
				}
				Flags = Temp;
				break;
				
			default:
				if(t->ta_Tag > TAG_ITEM_LAST)
				{
					ERR(("Warning - unrecognized argument to internalCreateAudioIns - 0x%lx: 0x%lx\n",
						t->ta_Tag, t->ta_Arg));
				}
				break;
			}
		}
	}
	
	aitp = (AudioInsTemplate *)CheckItem(TemplateItem, AUDIONODE, AUDIO_TEMPLATE_NODE);
	if (aitp == NULL)
	{
		ERR((" Bad Template Item = $%x\n", TemplateItem));
		return AF_ERR_BADITEM;  /* 930830 */
	}
	

	dtmp = (DSPPTemplate *)aitp->aitp_DeviceTemplate;

/* Scan to see if the library templates have at least one instrument allocated. */
DBUG(("internalCreateAudioIns: scanning 0x%x with AllocIfNoneScanHook\n", TemplateItem ));
	Result = ScanLibraryTemplates( dtmp, AllocIfNoneScanHook ); /* 940609 */
	if( Result < 0 ) goto err_share_failed;
	
REPORTMEM(("After ScanLibraryTemplates\n"));

/* Allocate SplitExec.dsp if needed for RateSHift */
	if( RateShift == 1 )
	{
		Result = OpenSplitExec();
		if (Result) goto err_split_failed;
	}
		
	Result = DSPPAllocInstrument(dtmp, &dins, RateShift);
TRACEB(TRACE_INT,TRACE_ITEM,("internalCreateAudioIns: dins = 0x%lx\n", dins));
	if (Result) goto err_alloc_failed;
	
REPORTMEM(("After DSPPAllocInstrument\n"));

	ains->ains_Template = aitp; /* 930311 */
	ains->ains_DeviceInstrument = (void *) dins;
	ains->ains_Flags = Flags;
	ains->ains_Status = AF_STOPPED;
	ains->ains_Bend = Convert32_F16(1);
	ains->ains_Item.n_Priority = (uint8) Priority;
	dins->dins_Node.n_Priority = (uint8) Priority;
	dins->dins_Specialness = (uint8) Specialness;
	dins->dins_RateShift = (uint8) RateShift;
	
/* Attach to instrument any samples specified in the template. */
	aatt = (AudioAttachment *)FirstNode(&aitp->aitp_Attachments);  /* 940424 */
	while (ISNODE(&aitp->aitp_Attachments,aatt))
	{
		Result = SuperAttachSlave( ains->ains_Item.n_Item,  (ItemNode *) aatt->aatt_Structure,
			aatt->aatt_HookName, aatt->aatt_Flags);
/* Fixed potential memory leak if attachment fails. 940818 */
		if (Result < 0) goto err_attach_failed;
		aatt = (AudioAttachment *)NextNode((Node *)aatt);
	}

REPORTMEM(("After SuperAttachSlave\n"));

/* Set the tuning to the templates tuning. 940406 */
	ains->ains_Tuning = aitp->aitp_Tuning;
	
/* Connect Instrument to Template's List */
TRACEB(TRACE_INT,TRACE_ITEM,("internalCreateAudioIns: aitp = 0x%lx, ains = 0x%lx\n", aitp, ains));
	AddTail( &aitp->aitp_InstrumentList, (Node *) ains );
	
/* Initialize dependant item lists. */
	InitList(&ains->ains_KnobList, "InstrumentsKnobs");
	InitList(&ains->ains_ProbeList, "InstrumentsProbes");

REPORTMEM(("Leaving internalCreateAudioIns\n"));

	return (ains->ains_Item.n_Item);
	
/* ERROR cleanup 940818 */
err_attach_failed:
	DSPPFreeInstrument(dins, TRUE);
	
err_split_failed:
/* Scan library templates to purge unused instruments. */
	ScanLibraryTemplates( (DSPPTemplate *) aitp->aitp_DeviceTemplate,
		FreeIfDoneScanHook );
		
err_alloc_failed:
	if( RateShift == 1 )
	{
		CloseSplitExec();
	}
	
err_share_failed:
	return Result;
}


/******************************************************************/

int32 internalDeleteAudioIns (AudioInstrument *ains)
{
	DSPPInstrument *dins;
	int32 Result;
	int32 IfOwned;
	int32 RateShift;
	AudioInsTemplate	*aitp;
	
TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioIns(0x%lx)\n", ains));
DBUG(("internalDeleteAudioIns(0x%lx) is 0x%x\n", ains, ains->ains_Item.n_Item));

REPORTMEM(("Entering internalDeleteAudioIns ------------- \n"));

	dins = (DSPPInstrument *)ains->ains_DeviceInstrument;
	RateShift = dins->dins_RateShift;
	
/* Delete any knob items attached to the instrument. */
	Result = afi_DeleteLinkedItems( &ains->ains_KnobList );
	if( Result < 0) return Result;
/* Delete any probe items attached to the instrument. */
	Result = afi_DeleteLinkedItems( &ains->ains_ProbeList );
	if( Result < 0) return Result;
	
/* Remove from Template's List */
	aitp = ains->ains_Template;
	aitp = (AudioInsTemplate *) CheckItem(aitp->aitp_Item.n_Item, AUDIONODE, AUDIO_TEMPLATE_NODE);
/* Do not remove from templates list if the list is gone! */
	if( aitp == NULL )
	{
		ERR(("Instrument's Template invalid when deleted!\n"));
	}
	else
	{
		ParanoidRemNode( (Node *) ains );
	}

	IfOwned = OWNEDBYCALLER(ains);
	Result = DSPPFreeInstrument(dins, IfOwned);
	if( Result < 0) return Result;
	
REPORTMEM(("After DSPPFreeInstrument\n"));

/* Scan library templates to purge unused instruments. 940609 */
/* Verify that template is still valid. 941031 */
	if( aitp != NULL )
	{
		Result = ScanLibraryTemplates( (DSPPTemplate *) aitp->aitp_DeviceTemplate,
			FreeIfDoneScanHook );
		if( Result < 0) return Result;
	}

/* For 1/2 rate execution. */
	if( RateShift == 1 )
	{
		Result = CloseSplitExec();
	}
	
REPORTMEM(("Leaving internalDeleteAudioIns\n"));

	return (Result);
}

/******************************************************************/
/********** Templates *********************************************/
/******************************************************************/

Item internalCreateAudioTemplate (AudioInsTemplate *aitp, TagArg *args)
{
	uint32 tagc, *tagp;
	DSPPTemplate *dtmp = NULL, *newdtmp = NULL;
	DSPPInstrument *newins;
 	int32 ExternalMode = FALSE;
	int32 Result;
	Item MasterTemplate = 0;
	Item CallingTemplate = 0;

TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioTemplate( %x, %x )\n",aitp,args));

    Result = TagProcessor( aitp, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioTemplate: TagProcessor failed.\n"));
    	return Result;
    }
DBUG(("item name after TagProcessor = %s\n", aitp->aitp_Item.n_Name ));

	aitp->aitp_OpenCount = 0;
	
	tagp = (uint32 *)args;

	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			switch (tagc)
			{
			case TAG_ITEM_NAME:
				tagp++;
				break;
				
			case AF_TAG_NAME:
				tagp++;
				break;
			
			case AF_TAG_TEMPLATE:
				dtmp = (DSPPTemplate *) *tagp++;
				ExternalMode = FALSE;
				break;
				
			case AF_TAG_EXTERNAL:
				dtmp = (DSPPTemplate *) *tagp++;
				ExternalMode = TRUE;
				break;
				
/* Use template passed in to clone a new item. */
			case AF_TAG_CLONE:
				MasterTemplate = (Item) *tagp++;
				ExternalMode = FALSE;
				break;
				
/* Item passed in is one that uses the current template. 940609 */
			case AF_TAG_USED_BY:
				CallingTemplate = (Item) *tagp++;
				break;
				
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Unrecognized tag in internalCreateAudioTemplate - 0x%lx: 0x%lx\n",
					tagc, *tagp++));
					return(AF_ERR_BADTAG);
				}
				tagp++;
			}
		}
	}

	if (dtmp != NULL)
	{
		Result = DSPPValidateTemplate( dtmp );
		if(Result < 0) return Result;
		newdtmp = DSPPCloneTemplate (aitp, dtmp);
		if (newdtmp == NULL) return AF_ERR_NOMEM;
	}
	else if( MasterTemplate)
	{
		AudioInsTemplate *MasterTemplatePtr;
/* Share the DSPP Template that is being used by the MasterTemplate.  940224 */
		MasterTemplatePtr = (AudioInsTemplate *)CheckItem(MasterTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
		if( MasterTemplatePtr == NULL )
		{
			ERR(("internalCreateAudioTemplate: bogus master template.\n"));
			return AF_ERR_BADITEM;
		}
		dtmp = (DSPPTemplate *)MasterTemplatePtr->aitp_DeviceTemplate;
		aitp->aitp_DeviceTemplate = dtmp;
		dtmp->dtmp_ShareCount++;
DBUG(("SHARE: dtmp = 0x%x, dtmp->dtmp_ShareCount = 0x%x\n", dtmp, dtmp->dtmp_ShareCount));
	}
/* No template, so no item. 940608 */
	else
	{
		ERR(("internalCreateAudioTemplate: No template specified.\n"));
		return AF_ERR_BADTAG;
	}
	
/* Add reference to library template to the calling template. 940609 */
	if( CallingTemplate )
	{
		AudioInsTemplate *CallingTemplatePtr;
		DSPPTemplate *Calling_dtmp;
		
		CallingTemplatePtr = (AudioInsTemplate *)CheckItem(CallingTemplate, AUDIONODE, AUDIO_TEMPLATE_NODE);
		if( CallingTemplatePtr == NULL )
		{
			ERR(("internalCreateAudioTemplate: bogus calling template.\n"));
			return AF_ERR_BADITEM;
		}
		Calling_dtmp = (DSPPTemplate *)CallingTemplatePtr->aitp_DeviceTemplate;
		{
			AudioReferenceNode *arnd;
		
			arnd = (AudioReferenceNode *) EZMemAlloc(sizeof(AudioReferenceNode), 0); /* ALLOC MEM_0001  */
			if (arnd == NULL) return AF_ERR_NOMEM;
			arnd->arnd_RefItem = aitp->aitp_Item.n_Item;
DBUG(("internalCreateAudioTemplate: arnd = 0x%x\n", arnd));
			AddTail( &Calling_dtmp->dtmp_LibraryTemplateRefs, (Node *) arnd );
		}
DBUG(("internalCreateAudioTemplate: reference to 0x%x linked to 0x%x\n",
					aitp->aitp_Item.n_Item, CallingTemplate ));
	}
	
	if (ExternalMode)
	{
		Result = DSPPLoadPatch (newdtmp, &newins, 0);
		if (Result) return Result;
	}
	
	InitList(&aitp->aitp_InstrumentList, "TemplatesInstruments");
	InitList (&aitp->aitp_Attachments, "TemplateAttachments"); /* 940424 */
	
/* Add to List in folio base so we can track and share. */
	AddTail(&AudioBase->af_TemplateList, (Node *) aitp);

TRACER(TRACE_INT,TRACE_ITEM,
	("internalCreateAudioTemplate returns 0x%x\n",aitp->aitp_Item.n_Item));
	return (aitp->aitp_Item.n_Item);
}

/******************************************************************/
Item internalOpenAudioTemplate (AudioInsTemplate *aitp, void *args)
{
TRACEE(TRACE_INT,TRACE_ITEM,("internalOpenAudioTemplate (0x%lx, 0x%lx)\n", aitp, args));
	
	aitp->aitp_OpenCount += 1;  /* Bump reference count  */
	return (aitp->aitp_Item.n_Item);
}

/******************************************************************/

int32 internalCloseAudioTemplate (Item it, AudioInsTemplate *aitp)
{
	Item ret;
	int32 result = 0;
	
/* Decrement count, if 0 then delete it. */
	
	if(--(aitp->aitp_OpenCount) <= 0)
	{
		ret = afi_SuperDeleteItem(it);  /* Get rid of this junk */
		result = (ret != it);
	}
	return result;
}

/******************************************************************
** Delete any library routines that are attached to template,
** and free reference nodes.
******************************************************************/
static int32 afi_DeleteLibraryTemplates( DSPPTemplate *dtmp )
{
	AudioReferenceNode *arnd, *nextarnd;
	int32 Result = 0;
	
	arnd = (AudioReferenceNode *)FirstNode( &dtmp->dtmp_LibraryTemplateRefs );
	while (ISNODE( &dtmp->dtmp_LibraryTemplateRefs, (Node *) arnd))
	{
		nextarnd = (AudioReferenceNode *) NextNode((Node *) arnd);
		Result = afi_SuperDeleteItem( arnd->arnd_RefItem );
		if( Result < 0 ) return Result;
		ParanoidRemNode( (Node *) arnd );
		EZMemFree( arnd );  /* FREE MEM_0001  */
		arnd = nextarnd;
	}

	return Result;
}

/******************************************************************/

int32 internalDeleteAudioTemplate (AudioInsTemplate *aitp)
{
	DSPPTemplate *dtmp;
	int32 Result;
	
TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioTemplate: aitp=0x%x\n",aitp));

/* If we are the last user, really unload it. */
	if(aitp->aitp_OpenCount <= 0)
	{
		dtmp = (DSPPTemplate *) aitp->aitp_DeviceTemplate;
		
/* Delete any attachments. */
		Result = afi_DeleteLinkedItems( &aitp->aitp_Attachments );
	
/* Delete any instruments derived from the template. */
		Result = afi_DeleteLinkedItems( &aitp->aitp_InstrumentList );
	
/* Clean up DSP Template */
DBUG(("DELETE: dtmp = 0x%x, dtmp->dtmp_ShareCount = 0x%x\n", dtmp, dtmp->dtmp_ShareCount));
		if( dtmp->dtmp_ShareCount-- ==  0)  /* 940224 */
		{
DBUG(("STRIP&FREE: dtmp = 0x%x\n", dtmp));
			StripInsTemplate( dtmp );
			afi_DeleteLibraryTemplates( dtmp );  /* 940609 */
			EZMemFree ( dtmp );
		}
		
		ParanoidRemNode( (Node *) aitp ); /* Remove from AudioFolio list. */
	}
	return (0);
}
