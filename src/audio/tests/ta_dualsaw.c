/* $Id: ta_dualsaw.c,v 1.9 1994/02/09 01:22:45 limes Exp $ */
/***************************************************************
**
** Play Dual Sawtooth waveforms
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
**	History:
**	6/14/93		rdg		overhauled to conform to dragon
**						 references to changedir, /remote, /dsp, and /aiff removed
**	930315		PLB 	Conforms to new API
**	11/16/92	PLB 	Modified for explicit mixer connect.
***************************************************************/

#include "types.h"
#include "filefunctions.h"
#include "debug.h"
#include "operror.h"
#include "stdio.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define DBUG(x) /* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}


/* Define Tags for StartInstrument */
TagArg SawTags[] =
	{
		{ AF_TAG_AMPLITUDE, 0},
		{ AF_TAG_RATE, 0},
        { 0, 0 }
    };
	
/* Variables local to this file. */
static 	Item 	MixerTmp;
static 	Item 	MixerIns;
static 	Item 	LeftGain0;
static 	Item 	RightGain0;
static 	Item 	LeftGain1;
static 	Item 	RightGain1;

/* Local function prototypes. */
static	int32 	SetupMixer( void );

int main()
{
/* Declare local variables */
	Item 	SawTmp;
	Item 	SawIns0;
	Item 	FreqKnob0;
	Item 	AmplitudeKnob0;
	Item 	SawIns1;
	Item 	FreqKnob1;
	Item 	AmplitudeKnob1;
	int32 	Result;
	int32 	Rate;

/* Initalize local variables */
	SawTmp 			= -1;
	SawIns0 		= -1;
	FreqKnob0		= -1;
	AmplitudeKnob0	= -1;
	SawIns1			= -1;
	FreqKnob1		= -1;
	AmplitudeKnob1	= -1;
	Result			= -1;


	PRT(("\nta_dualsaw.arm\n"));

/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}
	
	if (SetupMixer()) return -1;
	
/* Load description of Sawtooth instrument */
	SawTmp = LoadInsTemplate("sawtooth.dsp", 0);
	CHECKRESULT(SawTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	SawIns0 = AllocInstrument(SawTmp, 0);
	CHECKRESULT(SawIns0,"AllocInstrument");
	SawIns1 = AllocInstrument(SawTmp, 0);
	CHECKRESULT(SawIns1,"AllocInstrument");
	
/* Attach the Frequency knob. */
	FreqKnob0 = GrabKnob( SawIns0, "Frequency" );
	CHECKRESULT(FreqKnob0,"GrabKnob");
	FreqKnob1 = GrabKnob( SawIns1, "Frequency" );
	CHECKRESULT(FreqKnob1,"GrabKnob");

/* Connect Sawtooth to Mixer */
	Result = ConnectInstruments (SawIns0, "Output", MixerIns, "Input0");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (SawIns1, "Output", MixerIns, "Input1");
	CHECKRESULT(Result,"ConnectInstruments");
		
/* Play a note using StartInstrument */
	SawTags[0].ta_Arg = (int32 *) 0x5678; /* Amplitude */
	SawTags[1].ta_Arg = (int32 *) 0x0200; /* Rate */
	Result = StartInstrument( SawIns0, &SawTags[0] );
	Result = StartInstrument( SawIns1, &SawTags[0] );

	Rate = ConvertF16_32(GetAudioRate());
	PRT(("Rate = %d/sec\n", Rate));
	SleepAudioTicks( 2*Rate );
	
/* Raise the pitch to the fifth and the octave */
	TweakRawKnob(FreqKnob0, 0x0200);
/* Pause one second between each. */
	SleepAudioTicks( Rate );
	TweakRawKnob(FreqKnob1, 0x0300);
	SleepAudioTicks( Rate );
	TweakRawKnob(FreqKnob0, 0x0400);
	SleepAudioTicks( Rate );
	TweakRawKnob(FreqKnob1, 0x0500);
	SleepAudioTicks( Rate );

	PRT(("ta_dualsaw: all done.\n"));
	StopInstrument(SawIns0, NULL);
	StopInstrument(SawIns1, NULL);

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	ReleaseKnob( AmplitudeKnob1);
	ReleaseKnob( FreqKnob1);
	FreeInstrument( SawIns1 );
	ReleaseKnob( AmplitudeKnob0);
	ReleaseKnob( FreqKnob0);
	FreeInstrument( SawIns0 );
	UnloadInsTemplate( SawTmp );
	CloseAudioFolio();
	return((int) Result);
}

/*********************************************************************/
static	int32 SetupMixer( )
{
/* Declare local variables */
	int32 Result;
	
/* Initalize local variables */
	Result = 0;

/* Initalize global variables */
	MixerTmp = -1;
	MixerIns = -1;
	LeftGain0 = -1;
	RightGain0 = -1;
	LeftGain1 = -1;
	RightGain1 = -1;


/* Load the instrument template for the mixer. */
	MixerTmp = LoadInsTemplate("mixer4x2.dsp", 0);
	CHECKRESULT(MixerTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	MixerIns = AllocInstrument(MixerTmp, 0);
	CHECKRESULT(MixerIns,"AllocInstrument");
	
/* Set Mixer Levels */
	LeftGain0 = GrabKnob( MixerIns, "LeftGain0" );
	CHECKRESULT(LeftGain0,"GrabKnob");
	TweakKnob ( LeftGain0, 0x2000 );
	LeftGain1 = GrabKnob( MixerIns, "LeftGain1" );
	CHECKRESULT(LeftGain1,"GrabKnob");
	TweakKnob ( LeftGain1, 0x2000 );
	
	RightGain0 = GrabKnob( MixerIns, "RightGain0" );
	CHECKRESULT(RightGain0,"GrabKnob");
	TweakKnob ( RightGain0, 0x2000 );
	RightGain1 = GrabKnob( MixerIns, "RightGain1" );
	CHECKRESULT(RightGain1,"GrabKnob");
	TweakKnob ( RightGain1, 0x2000 );
	
/* Mixer must be started */
	Result = StartInstrument( MixerIns, NULL );
	return Result;
	
cleanup:
	ReleaseKnob( LeftGain0);
	ReleaseKnob( RightGain0);
	ReleaseKnob( LeftGain1);
	ReleaseKnob( RightGain1);
	FreeInstrument( MixerIns );
	UnloadInsTemplate( MixerTmp );
	return Result;
}


