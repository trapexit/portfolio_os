/* $Id: leftright.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
/***************************************************************
**
** Test Left/Right channel and phase using impulse train.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
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
#define	DBUG(x)	PRT(x)

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
        { TAG_END, 0 }
    };
	

int main( int32 argc, char *argv[])
{
	Item SawTmp;
	Item SawIns;
	Item FreqKnob;
	int32 Result;
	Item OutputIns;
	int32 Duration;

	PRT(("%s <ticks>\n", argv[0]));

	Duration = (argc > 1) ? atoi( argv[1] ) : 5000;
	
/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}
	
/* Load "directout" for connecting to DAC. */
	OutputIns = LoadInstrument("directout.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	
/* Load description of instrument */
	SawTmp = LoadInsTemplate( "impulse.dsp", 0);
	CHECKRESULT(SawTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	SawIns = AllocInstrument(SawTmp, 0);
	CHECKRESULT(SawIns,"AllocInstrument");
	
/* Attach the Frequency knob. */
	FreqKnob = GrabKnob( SawIns, "Frequency" );
	CHECKRESULT(FreqKnob,"GrabKnob");

/* Play a note using StartInstrument */
	SawTags[0].ta_Arg = (void *) 0x7FFF; /* Amplitude */
	SawTags[1].ta_Arg = (void *) 0x0100; /* Freq */
	Result = StartInstrument( SawIns, &SawTags[0] );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Connect output of sawtooth to left. */
	PRT(("Left\n"));
	Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	SleepAudioTicks( 500 );
	Result = DisconnectInstruments (SawIns, "Output", OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	
/* Raise the pitch for the right side */
	Result = TweakRawKnob(FreqKnob, 0x0200);
	CHECKRESULT(Result,"TweakRawKnob");
	SleepAudioTicks( 60 );
	
	PRT(("Right\n"));
	Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");
	SleepAudioTicks( 500 );
	Result = DisconnectInstruments (SawIns, "Output", OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");


/* Raise the pitch for both */
	Result = TweakRawKnob(FreqKnob, 0x0400);
	CHECKRESULT(Result,"TweakRawKnob");
	SleepAudioTicks( 60 );
	
	PRT(("Both\n"));
	Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (SawIns, "Output", OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");

	SleepAudioTicks( Duration );
	
	PRT(("%s all done.\n", argv[0]));
	StopInstrument(SawIns, NULL);
	StopInstrument(OutputIns, NULL);

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	ReleaseKnob( FreqKnob);
	FreeInstrument( SawIns );
	UnloadInsTemplate( SawTmp );
	UnloadInstrument( OutputIns );
	CloseAudioFolio();
	return((int) Result);
}



