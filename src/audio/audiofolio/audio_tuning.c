/* $Id: audio_tuning.c,v 1.18 1994/12/01 05:42:29 phil Exp $ */
/****************************************************************
**
** Audio Folio support for Tunings
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 930828 PLB Fixed item check in TuneInstrument
** 930830 PLB Check for negative counts in tuning
** 931129 PLB Added Convert12TET_F16
** 940406 PLB Support TuneInsTemplate()
** 940912 PLB Use Read16 to read uint16 values for NuPup
****************************************************************/

/****************************************************************
How Tuning Works

Tuning tables are specified in 16.16 frequencies. The base of the table is 
associated with a specific pitch (note index). When an instrument is played 
at a specific pitch, it is converted to frequency via table lookup. If the 
pitch is outside the table, it will extrapolate the frequency based on 
octave replication. For tunings that do not replicate by octave, the entire 
range must be specified. The number of notes per octave can also be 
specified to balance memory usage and speed of lookup. 

The frequency is then passed to the frequency knob which converts the freq 
to device specific parameters. For samples, the ratio of the desired 
frequency to the frequency of the sample is passed to the DSP. For speed 
reasons, the inverse of the sample frequency is stored with the sample so 
that we can do a multiply instead of a divide. For synthetic waveforms, we 
often simply divide the 16.16 frequency by the sample rate to get a 16.16 
phase increment. This can also be sped up by multiplying by a scaled sample 
period. 

****************************************************************/

#include "audio_internal.h"
#include "filefunctions.h"

/* Macros for debugging. */
#define DBUG(x)    /* PRT(x) */

#define SEMITONES_PER_OCTAVE  (12)

/*****************************************************************/
/****** Statically allocated default tuning table. ***************/
/*****************************************************************/
 AudioTuning DefaultTuning;
 
 ufrac16 DefaultTuningTable[] =
 {
 	0x001B80000,  /*  0 A440 */
    0x001D229EC,  /*  1 */
    0x001EDE220,  /*  2 */
    0x0020B404A,  /*  3 */
    0x0022A5D82,  /*  4 */
    0x0024B545C,  /*  5 */
    0x0026E4104,  /*  6 */
    0x00293414F,  /*  7 */
    0x002BA74DB,  /*  8 */
    0x002E3FD25,  /*  9 */
    0x0030FFDAA,  /* 10 */
    0x0033E9C01   /* 11 */
 };
 
 /* Tables for Pitch bend conversion. */
/* -50 0 12 50 * 1 16 shift dump.ntet.hex \ Forth code to gen table */

static uint16 Semitone50ths[] =
{
    0xF1A2,  /* -50 */
    0xF1E9,  /* -49 */
    0xF231,  /* -48 */
    0xF279,  /* -47 */
    0xF2C0,  /* -46 */
    0xF308,  /* -45 */
    0xF350,  /* -44 */
    0xF398,  /* -43 */
    0xF3E0,  /* -42 */
    0xF428,  /* -41 */
    0xF470,  /* -40 */
    0xF4B9,  /* -39 */
    0xF501,  /* -38 */
    0xF54A,  /* -37 */
    0xF592,  /* -36 */
    0xF5DB,  /* -35 */
    0xF624,  /* -34 */
    0xF66D,  /* -33 */
    0xF6B6,  /* -32 */
    0xF6FF,  /* -31 */
    0xF748,  /* -30 */
    0xF791,  /* -29 */
    0xF7DA,  /* -28 */
    0xF823,  /* -27 */
    0xF86D,  /* -26 */
    0xF8B6,  /* -25 */
    0xF900,  /* -24 */
    0xF94A,  /* -23 */
    0xF993,  /* -22 */
    0xF9DD,  /* -21 */
    0xFA27,  /* -20 */
    0xFA71,  /* -19 */
    0xFABB,  /* -18 */
    0xFB05,  /* -17 */
    0xFB50,  /* -16 */
    0xFB9A,  /* -15 */
    0xFBE5,  /* -14 */
    0xFC2F,  /* -13 */
    0xFC7A,  /* -12 */
    0xFCC4,  /* -11 */
    0xFD0F,  /* -10 */
    0xFD5A,  /* -9 */
    0xFDA5,  /* -8 */
    0xFDF0,  /* -7 */
    0xFE3B,  /* -6 */
    0xFE87,  /* -5 */
    0xFED2,  /* -4 */
    0xFF1D,  /* -3 */
    0xFF69,  /* -2 */
    0xFFB4  /* -1 */
};


/* -12 12 12 1 16 shift dump.ntet.hex */
static uint16 SemitoneFractions[] =
{
    0x8000,  /* -12 */
    0x879C,  /* -11 */
    0x8FAD,  /* -10 */
    0x9838,  /* -9 */
    0xA145,  /* -8 */
    0xAADC,  /* -7 */
    0xB505,  /* -6 */
    0xBFC9,  /* -5 */
    0xCB30,  /* -4 */
    0xD745,  /* -3 */
    0xE412,  /* -2 */
    0xF1A2  /* -1 */
};


/*****************************************************************/
/***** USER Level Folio Calls ************************************/
/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC spg/items/tuning
 |||	Tuning - An item that contains information for tuning an instrument.
 |||
 |||	  Description
 |||
 |||	    A tuning item contains information for converting MIDI pitch numbers to
 |||	    frequency for an instrument or template. The information includes an array
 |||	    of frequencies, and 32-bit integer values indicating the number of
 |||	    pitches, notes in an octave, and the lowest pitch for the tuning
 |||	    frequency. See the CreateTuning() call in the \xd2 Audio Folio Calls\xd3
 |||	    chapter in this manual for more information.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_TUNING_NODE
 |||
 |||	  Create
 |||
 |||	    CreateItem()
 |||
 |||	    CreateTuning()
 |||
 |||	  Delete
 |||
 |||	    DeleteItem()
 |||
 |||	    DeleteTuning()
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
 |||	    TuneInsTemplate()
 |||
 |||	    TuneInstrument()
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_ADDRESS               (const ufrac16 *) Create, Modify. Array of
 |||	                                 frequencies in 16.16 Hz to be used as a
 |||	                                 lookup table.
 |||
 |||	    AF_TAG_BASENOTE              (uint32) Create, Modify. MIDI note number
 |||	                                 that should be given the first frequency in
 |||	                                 the frequency array in the range of 0..127.
 |||
 |||	    AF_TAG_FRAMES                (uint32) Create, Modify. Number of
 |||	                                 frequencies in array pointed to by
 |||	                                 AF_TAG_ADDRESS. This value must be >=
 |||	                                 NotesPerOctave.
 |||
 |||	    AF_TAG_NOTESPEROCTAVE        (uint32) Create, Modify. Number of notes per
 |||	                                 octave. This is used to determine where in
 |||	                                 the frequency array to look for notes that
 |||	                                 fall outside the range of
 |||	                                 BaseNote..BaseNote+Frames-1. This value must
 |||	                                 be <= Frames.
 **/

 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/convert12tet_f16
 |||	Convert12TET_F16 - Converts a pitch bend value in semitones and cents into
 |||	                   an f16 bend value.
 |||
 |||	  Synopsis
 |||
 |||	    Err Convert12TET_F16( int32 Semitones, int32 Cents,
 |||	    frac16 *FractionPtr )
 |||
 |||	  Description
 |||
 |||	    This procedure, whose name reads (convert 12-tone equal-tempered to
 |||	    f16,)  converts a pitch bend value expressed in semitones and cents to
 |||	    an f16 bend value.  The bend value is used with BendInstrumentPitch() to
 |||	    bend the instrument's pitch up or down: The value multiplies the
 |||	    frequency of the instrument's output to create a resultant pitch bent
 |||	    up or down by the specified amount.
 |||
 |||	    Note that the semitones and cents values need not both be positive or both
 |||	    be negative.  One value can be positive while the other value is negative.
 |||	     For example, -5 semitones and +30 cents bends pitch down by 4 semitones
 |||	    and 70 cents.
 |||
 |||	    Convert12TET_F16() stores the resultant bend value in FractionPtr.
 |||
 |||	  Arguments
 |||
 |||	    Semitones                    The integer number of semitones to bend up
 |||	                                 or down (can be negative or positive).
 |||
 |||	    Cents                        The integer number of cents (from -100 to
 |||	                                 +100) to bend up or down.
 |||
 |||	    FractionPtr                  Pointer to a frac16 variable where the bend
 |||	                                 value will be stored.
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
 |||	    BendInstrumentPitch()
 |||
 **/
Err Convert12TET_F16( int32 Semitones, int32 Cents, frac16 *FractionPtr )
{
	frac16 SemiFrac, PartFrac, Fraction;
	int32 SemiParts;
	int32 Shifter;
	int32 NewCents;
	
	NewCents = Cents;
	
	if( NewCents > 0 )
	{
		NewCents -= 100;
		Semitones += 1;
	}
	
	if(( NewCents > 0 ) || ( NewCents < -100 ))
	{
		ERR(("Convert12TET_F16: Cents = %d\n", Cents));
		return AF_ERR_OUTOFRANGE;
	}
	
	if(( Semitones < -120 ) || ( Semitones > 120 ))
	{
		ERR(("Convert12TET_F16: Semitones = %d\n", Semitones));
		return AF_ERR_OUTOFRANGE;
	}

	Shifter = 0;
	while( Semitones < -12 )
	{
		Semitones += SEMITONES_PER_OCTAVE;
		Shifter -= 1;
	}
	while( Semitones >= 0 )
	{
		Semitones -= SEMITONES_PER_OCTAVE;
		Shifter += 1;
	}

DBUG(("NewCents = %d\n", NewCents));
DBUG(("Semitones = %d\n", Semitones));
DBUG(("Shifter = %d\n", Shifter));
	if( NewCents)
	{
		SemiParts = NewCents >> 1;
		PartFrac = (frac16) Read16(&Semitone50ths[SemiParts + 50]);
		SemiFrac = (frac16) Read16(&SemitoneFractions[Semitones + 12]);
		Fraction = MulSF16( PartFrac, SemiFrac );
DBUG(("SemiParts = %d\n", SemiParts));
DBUG(("PartFrac = 0x%x\n", PartFrac));
DBUG(("SemiFrac = 0x%x\n", SemiFrac));
	}
	else
	{
		Fraction = (frac16) Read16(&SemitoneFractions[Semitones+12]);
	}

/* Apply shift. */
	if( Shifter < 0 )
	{
		Fraction = Fraction >> (-Shifter);
	}
	else if( Shifter > 0 )
	{
		Fraction = Fraction << Shifter;
	}
	
	*FractionPtr = Fraction;
	
DBUG(("Convert12TET_F16: Fraction = 0x%x\n", Fraction));

	return 0;
}

/* @@@ this gets a variable, but otherwise doesn't do anything */
frac16 GetMasterTuning( void )
{
	return AudioBase->af_MasterTuning;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/createtuning
 |||	CreateTuning - Creates a tuning item.
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateTuning( ufrac16 *Frequencies, int32 NumNotes,
 |||	    int32 NotesPerOctave, int32 BaseNote )
 |||
 |||	  Description
 |||
 |||	    This procedure creates a tuning item that can be used to tune an
 |||	    instrument.
 |||
 |||	    NotesPerOctave is set to 12 for a standard western tuning system.  For
 |||	    every octave's number of notes up or down, the frequency will be
 |||	    doubled or halved.  You can thus specify 12 notes of a 12-tone scale, then
 |||	    extrapolate up or down for the other octaves.  You should specify the
 |||	    entire range of pitches if the tuning does not repeat itself every octave.
 |||	     For more information, see the Music Programmer's Guide.
 |||
 |||	    When you are finished with the tuning item, you should call DeleteTuning()
 |||	    to deallocate the resources.
 |||
 |||	  Arguments
 |||
 |||	    Frequencies                  A pointer to an array of 16.16 frequencies,
 |||	                                 which lists a tuning pattern.
 |||
 |||	    NumNotes                     The number of frequencies in the array.
 |||
 |||	    NotesPerOctave               The number of notes in an octave (12 in a
 |||	                                 standard tuning scale).
 |||
 |||	    BaseNote                     The MIDI pitch (note) value of the first
 |||	                                 frequency in the array.  If your array
 |||	                                 starts with the frequency of middle C, its
 |||	                                 MIDI pitch value would equal 60.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number for the tuning or an error code (a
 |||	    negative value) if an error occurs.
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
 |||	    DeleteTuning(), TuneInsTemplate(), TuneInstrument()
 |||
 **/
Item  CreateTuning( ufrac16 *Frequencies, int32 NumIntervals, int32 NotesPerOctave, int32 BaseNote )
{
	Item Result;
	TagArg Tags[5];

TRACEE(TRACE_INT,TRACE_TUNING,("CreateTuning( 0x%x, 0x%x, %s )\n", 
Frequencies,
		NumIntervals, NotesPerOctave,  BaseNote ));

	Tags[0].ta_Tag = AF_TAG_ADDRESS;
	Tags[0].ta_Arg = (void *) Frequencies;
	Tags[1].ta_Tag = AF_TAG_FRAMES;
	Tags[1].ta_Arg = (void *) NumIntervals;
	Tags[2].ta_Tag = AF_TAG_NOTESPEROCTAVE;
	Tags[2].ta_Arg = (void *) NotesPerOctave;
	Tags[3].ta_Tag = AF_TAG_BASENOTE;
	Tags[3].ta_Arg = (void *) BaseNote;
	Tags[4].ta_Tag =  TAG_END;
	
    Result = CreateItem( MKNODEID(AUDIONODE,AUDIO_TUNING_NODE), Tags );
TRACER(TRACE_INT, TRACE_TUNING, ("CreateTuning returns 0x%08x\n", Result));
	return Result;
}


/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/deletetuning
 |||	DeleteTuning - Deletes a tuning.
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteTuning( Item Tuning )
 |||
 |||	  Description
 |||
 |||	    This procedure deletes the specified tuning and frees any resources
 |||	    dedicated to it.  Note that if you delete a tuning that's in use for
 |||	    an existing instrument, that instrument returns to its default tuning.
 |||
 |||	  Arguments
 |||
 |||	    Tuning                       The item number of the tuning to delete.
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
 |||	    CreateTuning(), TuneInsTemplate(), TuneInstrument()
 |||
 **/
int32 DeleteTuning( Item Tuning )
{
	return DeleteItem( Tuning );
}

/*****************************************************************/
/***** Create Tuning Item for Folio ******************************/
/*****************************************************************/
Item internalCreateAudioTuning (AudioTuning *atun, TagArg *args)
{
 	int32 Result;
	  
	Result = TagProcessor( atun, args, afi_DummyProcessor, 0);
    if(Result < 0)
    {
    	ERR(("internalCreateAudioTuning: TagProcessor failed.\n"));
    	return Result;
    }


/* Parse remaining tags to allow overwriting file info. */
	Result = internalSetTuningInfo(atun, args);
	CHECKRSLT(("Bad Tag value in internalCreateAudioTuning"));

/* Everything OK so pass back item. */
	Result = atun->atun_Item.n_Item;

error:
	return (Result);
}

/*****************************************************************/
Item internalSetTuningInfo (AudioTuning *atun, TagArg *args)
{
  	int32 Result, res;  	
	uint32 tagc, *tagp;
	ufrac16 *Frequencies;
	int32 NumFrequencies;
	int32 NotesPerOctave;

	Frequencies = atun->atun_Frequencies;
	NumFrequencies = atun->atun_NumNotes;
	NotesPerOctave = atun->atun_NotesPerOctave;
	
	tagp = (uint32 *)args;
	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
DBUG(("internalPutEnvelopeInfo: Tag = %d, Arg = $%x\n", tagc, *tagp));
			switch (tagc)
			{
			case AF_TAG_ADDRESS:
				Frequencies = (ufrac16 *) *tagp++;
				break;
				
			case AF_TAG_FRAMES:
				NumFrequencies = *tagp++;
				if( NumFrequencies < 0 ) /* 930830 */
				{
					ERR(("Negative number of frequencies in tuning!\n"));
					return AF_ERR_BADTAGVAL;
				}
				break;
				
			case AF_TAG_BASENOTE:
				atun->atun_BaseNote = *tagp++;
				break;
				
			case AF_TAG_NOTESPEROCTAVE:
				NotesPerOctave = *tagp++;
				if( NotesPerOctave < 0 )  /* 930830 */
				{
					ERR(("Negative number of notes per octave in tuning!\n"));
					return AF_ERR_BADTAGVAL;
				}
				break;
				
			default:
				if(tagc > TAG_ITEM_LAST)
				{
					ERR (("Unrecognized tag in internalPutTuningInfo - 0x%lx: 0x%lx\n",
					tagc, *tagp++));
					return(AF_ERR_BADTAG);
				}
				tagp++;
			}
		}
	}
	
	Result = AF_ERR_BADTAGVAL;  /* set default */
/*
** Validate Tuning data.
*/
	res = afi_IsRamAddr( (char *) Frequencies, NumFrequencies * sizeof(ufrac16) );
	if(res < 0)
	{
		ERR(("Tuning data address not in RAM"));
		goto DONE;
	}
	
	atun->atun_Frequencies = Frequencies;
	atun->atun_NumNotes = NumFrequencies;
	atun->atun_NotesPerOctave = NotesPerOctave;
	
/* Everything passed. */
	Result = 0;
	
DONE:
	return Result;
}


/**************************************************************/

int32 internalDeleteAudioTuning (AudioTuning *atun)
{	
	return 0;
}


/*****************************************************************/
/* @@@ this sets a variable, but otherwise doesn't do anything */
int32 swiSetMasterTuning( ufrac16 Frequency )
{
	AudioBase->af_MasterTuning = Frequency;
	return 0;
}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/tuneinstemplate
 |||	TuneInsTemplate - Applies the specified tuning to an instrument template.
 |||
 |||	  Synopsis
 |||
 |||	    Err TuneInsTemplate( Item InsTemplate, Item Tuning )
 |||
 |||	  Description
 |||
 |||	    This procedure applies the specified tuning to the specified instrument
 |||	    template.  When an instrument is created using the template, the tuning is
 |||	    applied to the new instrument.  When notes are played on the instrument,
 |||	    they play using the specified tuning system.
 |||
 |||	    The tuning is created using CreateTuning().
 |||
 |||	  Arguments
 |||
 |||	    InsTemplate                  The item number of the template.
 |||
 |||	    Tuning                       The item number of the tuning.
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
 |||	    CreateTuning(), TuneInstrument(), DeleteTuning()
 |||
 **/
int32 swiTuneInsTemplate( Item InsTemplate, Item Tuning )
{
	AudioTuning *atun;
	AudioInsTemplate *aitp;

	aitp = (AudioInsTemplate *) CheckItem( InsTemplate,
			AUDIONODE, AUDIO_TEMPLATE_NODE);
	if( aitp == NULL )
	{
		return AF_ERR_BADITEM;
	}

	aitp->aitp_Tuning = 0;
	if(Tuning)
	{
		atun = (AudioTuning *) CheckItem( Tuning,  /* Fix item check. 930828 */
				AUDIONODE, AUDIO_TUNING_NODE);
		if( atun == NULL )
		{
			return AF_ERR_BADITEM;
		}
		else
		{
			aitp->aitp_Tuning = Tuning;
		}
	}
	return 0;

}

/*****************************************************************/
 /**
 |||	AUTODOC PUBLIC mpg/audiofolio/tuneinstrument
 |||	TuneInstrument - Applies the specified tuning to an instrument.
 |||
 |||	  Synopsis
 |||
 |||	    Err TuneInstrument( Item Instrument, Item Tuning )
 |||
 |||	  Description
 |||
 |||	    This procedure applies the specified tuning (created using CreateTuning())
 |||	    to an instrument so that notes played on the instrument use that tuning.
 |||	    If no tuning is specified, the default 12-tone equal-tempered tuning is
 |||	    used.
 |||
 |||	  Arguments
 |||
 |||	    Instrument                   The item number of the instrument.
 |||
 |||	    Tuning                       The item number of the tuning.
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
 |||	    CreateTuning(), TuneInsTemplate(), DeleteTuning()
 |||
 **/
int32 swiTuneInstrument( Item Instrument, Item Tuning )
{
	AudioTuning *atun;
	AudioInstrument *ains;

	ains = (AudioInstrument *) CheckItem( Instrument,
			AUDIONODE, AUDIO_INSTRUMENT_NODE);
	if( ains == NULL )
	{
		return AF_ERR_BADITEM;
	}

	ains->ains_Tuning = 0;
	if(Tuning)
	{
		atun = (AudioTuning *) CheckItem( Tuning,  /* Fix item check. 930828 */
				AUDIONODE, AUDIO_TUNING_NODE);
		if( atun == NULL )
		{
			return AF_ERR_BADITEM;
		}
		else
		{
			ains->ains_Tuning = Tuning;
		}
	}
	return 0;
}


/*****************************************************************/
/***** Internal Tuning Calculations ******************************/
/*****************************************************************/
int32 InitDefaultTuning( void )
{
	DefaultTuning.atun_Frequencies = DefaultTuningTable;
	DefaultTuning.atun_NotesPerOctave = 12;
	DefaultTuning.atun_NumNotes = 12;
	DefaultTuning.atun_BaseNote = AF_A440_PITCH;
	AudioBase->af_DefaultTuning = &DefaultTuning;
	return 0;
}

int32 TermDefaultTuning( void )
{
/* No memory allocated. */
	AudioBase->af_DefaultTuning = NULL;
	return 0;
}

/*****************************************************************/
AudioTuning *GetInsTuning( AudioInstrument *ains)
{
	AudioTuning *atun;
	
/* Use specific or default tuning. */
	if( ains->ains_Tuning )
	{
		atun = (AudioTuning *) CheckItem( ains->ains_Tuning,
				AUDIONODE, AUDIO_TUNING_NODE);
		if( atun == NULL )
		{
			atun = AudioBase->af_DefaultTuning;
		}
	}
	else
	{
		atun = AudioBase->af_DefaultTuning;
	}
	return atun;
}


/*****************************************************************/
int32 PitchToFrequency( AudioTuning *atun, int32 Pitch, ufrac16 *FrequencyPtr)
{
	int32 Index;
	int32 Shifter;
	
/* Look up scalar and shift by octaves if needed. */
	Index = Pitch - atun->atun_BaseNote;
	Shifter = 0;
	if( Index >= (int32) atun->atun_NumNotes )
	{
		do
		{
			Shifter++;
			Index -= atun->atun_NotesPerOctave;
		}while( Index > (int32) atun->atun_NumNotes);
		*FrequencyPtr = atun->atun_Frequencies[Index] << Shifter;
	}
	else if ( Index < 0 )
	{
		do
		{
			Shifter++;
			Index += atun->atun_NotesPerOctave;
		}while( Index < 0);
		*FrequencyPtr = atun->atun_Frequencies[Index] >> Shifter;
	}
	else
	{
		*FrequencyPtr = atun->atun_Frequencies[Index];
	}
	
DBUG(("Calc: Index = %d, *FrequencyPtr = 0x%x, Shifter = %d\n", Index, *FrequencyPtr, Shifter));

	return 0;
}

