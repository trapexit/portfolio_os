/* $Id: ta_polyphony.c,v 1.22 1994/02/18 01:54:55 limes Exp $ */
/***************************************************************
**
** Play Polyphonic Notes using Sampled Waveform - ta_polyphony.c
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************
** 930315 PLB Conforms to new API
*/

#include "types.h"
#include "filefunctions.h"
#include "debug.h"
#include "operror.h"
#include "stdio.h"

/* Include this when using the Audio Folio */
#include "audio.h"

#define NUMSECONDS (10000000 )

// #define DEMOMODE
#ifdef DEMOMODE
#include "audiodemo.h"
#endif

/* Don't go above 12 cuz mixer is 12x2 */
#define MAXVOICES 12
#define NUMVOICES 8
#define NUMER 6
#define DENOM 5
#define STARTFREQ 0x2000
#define NUMSECS 1
#define SAMPPRIORITY 50
#define MIXPRIORITY 100

#define INSNAME "varmono16.dsp"
// #define INSNAME "sampler.dsp"
// #define INSNAME "samplerenv.dsp"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define DBUG(x) PRT(x)

int32 PlayFreqNote ( Item Instrument, int32 Freq, int32 Duration );

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}
	
	
/* Define Tags for StartInstrument */

Item MixerTmp = -1;
Item MixerIns = -1;
Item LeftGains[MAXVOICES];
Item RightGains[MAXVOICES];
int32 SetupMixer( void );

static char StringPad[32];    /* For building names. */
	
int main(int argc, char *argv[])
{
	Item SamplerTmp;
	Item SamplerIns[MAXVOICES];
	Item SampleItem = 0;
	int32 i, Result = -1;
	int32 Freq;
	int32 Rate;
	uint32 Joy;
	uint32 StartTime, EndTime;
	int32 NumVoices;
	
	
	TagArg SamplerTags[] =
	{
		{ AF_TAG_AMPLITUDE, (int32 *) 0x7000},
		{ AF_TAG_RATE, 0},
        { 0, 0 }
    };
    

	PRT(("ta_polyphony <numvoices>\n"));
	PRT(("   use %s\n", INSNAME));
	
/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	NumVoices =  (argc > 1) ? atoi(argv[1]) : NUMVOICES;
	if(NumVoices > MAXVOICES)
	{
		ERR(("Too many voices! Set to %d\n", MAXVOICES));
		NumVoices = MAXVOICES;
	}
	
/*	TraceAudio(TRACE_ITEM); */
	
	if (SetupMixer()) return -1;
	
	PRT(("Please wait a few seconds for %d samples and instruments to load.\n", NumVoices));
	
/* Load Sampler instrument definition/template */
	SamplerTmp = LoadInsTemplate(INSNAME, 0);
	CHECKRESULT(SamplerTmp,"LoadInsTemplate");
		
/* Make Sampler instruments based on template. */
	for (i=0; i<NumVoices; i++)
	{
		SamplerIns[i] = AllocInstrument(SamplerTmp, SAMPPRIORITY);
		CHECKRESULT(SamplerIns[i],"AllocInstrument");
PRT(("Create #%d\n", i+1));
	}
	
/* Load digital audio Sample from disk. */
	SampleItem = LoadSample("sinewave.aiff"); /* default */
	CHECKRESULT(SampleItem,"LoadSample");
	
/* Look at sample information. */
/*	DebugSample(SampleItem); */

/* Attach the sample to the instrument. */
	for (i=0; i<NumVoices; i++)
	{
		Result = AttachSample(SamplerIns[i], SampleItem, 0);
		CHECKRESULT(Result,"AttachSample");
		sprintf(StringPad, "Input%d", i);
PRT(("Connect %d to %s\n", i, StringPad));
		Result = ConnectInstruments (SamplerIns[i], "Output", MixerIns,
			StringPad);
		CHECKRESULT(Result,"ConnectInstruments");
	}

	Rate = ConvertF16_32( GetAudioRate() );
	
	Freq = STARTFREQ;
	
	for (i=0; i<NumVoices; i++)
	{
		PRT(("Play voice %d at $%x\n", i, Freq));
		SamplerTags[1].ta_Arg = (void *) Freq; /* Freq */
		Freq = (Freq * NUMER) / DENOM;
		Result = StartInstrument( SamplerIns[i], SamplerTags );
		PRT(("Ticks = %d\n", DSPGetTicks()));
		Result = SleepAudioTicks( NUMSECS*Rate );
	}
	
#ifdef DEMOMODE
	
	PRT(("Press joypad to stop sound.\n"));
	InitJoypad();
	do
	{
		SleepAudioTicks( 20 );
		ReadJoypad( &Joy );
	} while ( Joy == 0);
	
#else
		SleepAudioTicks(NUMSECONDS*Rate);
#endif

	
	for (i=0; i<NumVoices; i++)
	{
		PRT(("Stop voice %d\n", i));
		StopInstrument( SamplerIns[i], NULL );
		PRT(("Ticks = %d\n", DSPGetTicks()));
		SleepAudioTicks(Rate>>2);
	}
	
cleanup:
/* We only need to free the templates and the instruments. */
	UnloadInsTemplate( SamplerTmp );
	UnloadSample( SampleItem );
	UnloadInsTemplate( MixerTmp );
	
/*	TraceAudio(0); */
	CloseAudioFolio();
	PRT(("Test Complete.\n"));
	return((int) Result);
}

/*********************************************************************/
int32 SetupMixer( )
{
	int32 Result=0, VoiceID, i;
	
	MixerTmp = LoadInsTemplate("mixer12x2.dsp", 0);
	CHECKRESULT(MixerTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	MixerIns = AllocInstrument(MixerTmp, MIXPRIORITY);
	CHECKRESULT(MixerIns,"AllocInstrument");
	
/* Attach the Left and Right gain knobs. */
	for (i=0; i<MAXVOICES; i++)
	{
		sprintf(StringPad, "LeftGain%d", i);
		LeftGains[i] = GrabKnob( MixerIns, StringPad );
		CHECKRESULT(LeftGains[i],"GrabKnob");
		TweakKnob ( LeftGains[i], 0 );
	
		sprintf(StringPad, "RightGain%d", i);
		RightGains[i] = GrabKnob( MixerIns, StringPad );
		CHECKRESULT(RightGains[i],"GrabKnob");
		TweakKnob ( RightGains[i], 0 );
	}

#define TOTALLOUD (MAXDSPAMPLITUDE)
/* Attach the Left and Right gain knobs. */
	for (i=0; i<MAXVOICES; i++)
	{
		TweakKnob ( LeftGains[i], TOTALLOUD/MAXVOICES );
		TweakKnob ( RightGains[i], TOTALLOUD/MAXVOICES );
	}
	
/* Mixer must be started */
	VoiceID = StartInstrument( MixerIns, NULL );
	return Result;
	
cleanup:
	FreeInstrument( MixerIns );
	UnloadInsTemplate( MixerTmp );
	return Result;
}


