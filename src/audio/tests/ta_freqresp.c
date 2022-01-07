/* $Id: ta_freqresp.c,v 1.3 1994/02/18 01:54:55 limes Exp $ */

/*******************************************************************
**
** Frequency Response measurement.
**
** By: Phil Burk
********************************************************************/


#include "types.h"

#include "kernel.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "debug.h"
#include "operamath.h"
#include "filefunctions.h"
#include "graphics.h"
#include "audio.h"
#include "stdio.h"
#include "event.h"

#include "audiodemo.h"

#define DEMOMODE

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}
	
#define CHECKPTR(val,name) \
	if (val == 0) \
	{ \
		Result = -1; \
		ERR(("Failure in %s\n", name)); \
		goto cleanup; \
	}

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define MAX_FADERS (12)
Fader MyFaders[MAX_FADERS];
FaderBlock MyFaderBlock;

Item MixerIns = -1;
Item LeftGainKnob = -1;
Item RightGainKnob = -1;
int32 SetupMixer( void );

Item gTestIns;
Item gSamplerIns;
Item gFilterFollowerIns;
Item gFilterMonitorIns;
Item gSamplerFollowerIns;
Item gSamplerMonitorIns;

#define MAX_KNOBS MAX_FADERS
Item TestKnob[MAX_KNOBS];
char *Name[MAX_KNOBS];
	
#define NUM_SAMPLES    (8)
#define BYTESPERSAMPLE (2)
#define SMALLEST_SIZE  (4)
#define DELAYTIME     (20)
Item SineWaves[8];


/********************************************************************/
Item GenerateSineSample( int32 NumFrames )
{
	int32 i;
	Item SampleItem;
	int16 *Data;
	TagArg Tags[8];
	int32 NumBytes;
	int32 Result;
	frac16 Angle, Sin;
	int32 Val;
	
	NumBytes = NumFrames * BYTESPERSAMPLE;
	Tags[0].ta_Tag = AF_TAG_SUSTAINBEGIN;
	Tags[0].ta_Arg = (void *) 0;
	Tags[1].ta_Tag = AF_TAG_SUSTAINEND;
	Tags[1].ta_Arg = (void *) NumFrames;
	Tags[2].ta_Tag = TAG_END;
	SampleItem = MakeSample( NumBytes, Tags );
	
	Tags[0].ta_Tag = AF_TAG_ADDRESS;
	Tags[0].ta_Arg = NULL;
	Tags[1].ta_Tag = TAG_END;
	Result = GetAudioItemInfo(SampleItem, Tags);
	CHECKRESULT(Result,"GetAudioItemInfo");
	Data = Tags[0].ta_Arg;
	
	for (i=0; i<NumFrames; i++)
	{
		Angle = MulSF16( FULLCIRCLE, DivUF16( i, NumFrames ) );
		Sin = SinF16( Angle );
	/*	DBUG(("%d Sin[0x%x] = 0x%x\n", i, Angle, Sin )); */
		Val = Sin >> 1;
		if( Val > 0x7FFF ) Val = 0x7FFF;
		if( Val < -0x8000 ) Val = -0x8000;
		
		*Data++ = (int16) (Val & 0xFFFF);
	}
	
cleanup:
	return SampleItem;
}

	
/********************************************************************/
int32 GenerateSineSamples( int32 Smallest, Item *Samps, int32 NumSamps )
{
	int32 i;
	Item SampleItem;
	int32 Frames;
	short *Data;
	TagArg Tags[3];
	int32 Result = 0;
		
	for( i=0; i<NumSamps; i++)
	{
		SampleItem = GenerateSineSample( Smallest << i );
		CHECKRESULT(SampleItem,"GenerateSineSample");
		*Samps++ = SampleItem;
	}
cleanup:
	return Result;
}

/********************************************************************/
int32 SweepSpectrum( int32 FreqStart, int32 FreqEnd, int32 Numer, int32 Denom )
{
	int32 LastSampleIndex;
	int32 SampleIndex;
	int32 Result, i,j,Size;
	frac16 Freq16;
	uint32 Rate;
	Item Att;
	TagArg Tags[3];
	int32 FilterMonitorEO;
	int32 SamplerMonitorEO;
	Item MyCue;
	int32 FilterLevel, SamplerLevel;
	
	Att = 0;
	LastSampleIndex = -1;
	
	Tags[0].ta_Tag = AF_TAG_RATE;
	Tags[1].ta_Tag = AF_TAG_AMPLITUDE;
	Tags[1].ta_Arg = (void *) 0x4000;
	Tags[2].ta_Tag = TAG_END;
	
	Result = DSPGetRsrcAlloc(gFilterMonitorIns, DRSC_EO_MEM,
		"Monitor", &FilterMonitorEO);
	FilterMonitorEO -= 0x300;
	CHECKRESULT(Result,"DSPGetRsrcAlloc");
	Result = DSPGetRsrcAlloc(gSamplerMonitorIns, DRSC_EO_MEM,
		"Monitor", &SamplerMonitorEO);
	SamplerMonitorEO -= 0x300;
	CHECKRESULT(Result,"DSPGetRsrcAlloc");
	
	MyCue = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	CHECKRESULT(MyCue, "CreateItem Cue");
	
	Freq16 = Convert32_F16( FreqStart );
	do
	{
/* Determine best sample. */
/*		Rate = 0x8000 * (Freq/SR)  * Size */
		SampleIndex = 0;
		for( j=0; j<NUM_SAMPLES; j++ )
		{
			Size = SMALLEST_SIZE << j;
			Rate = MulSF16( 0x8000,
				DivUF16( ConvertF16_32(Freq16)*Size , 44100 ));
			if( Rate > 0x4000 )
			{
				SampleIndex = j;
				break;
			}
		}
		
/* Switch samples if necessary. */
		if( SampleIndex != LastSampleIndex )
		{
			LastSampleIndex = SampleIndex;
			StopInstrument( gSamplerIns, NULL );
			CHECKRESULT(Result,"StopInstrument");
			if( Att) DetachSample( Att );
			Att = AttachSample( gSamplerIns, SineWaves[SampleIndex], 0 );
			CHECKRESULT(Att,"AttachSample");
		}
		
		Tags[0].ta_Arg = (void *) Rate;
		Result = StartInstrument( gSamplerIns, Tags );
		CHECKRESULT(Result,"StartInstrument");
		
/* Measure response, low pass filter data. */
		SleepUntilTime( MyCue, GetAudioTime() + DELAYTIME );
		FilterLevel = DSPReadEO( FilterMonitorEO );
		SamplerLevel = DSPReadEO( SamplerMonitorEO );
		SleepUntilTime( MyCue, GetAudioTime() + 2 );
		FilterLevel = (FilterLevel + DSPReadEO( FilterMonitorEO ))/2;
		SamplerLevel = (SamplerLevel + DSPReadEO( SamplerMonitorEO ))/2;
		SleepUntilTime( MyCue, GetAudioTime() + 2 );
		FilterLevel = (FilterLevel + DSPReadEO( FilterMonitorEO ))/2;
		SamplerLevel = (SamplerLevel + DSPReadEO( SamplerMonitorEO ))/2;
		
/* Report result. */
DBUG(("SI = %2d, Rate = 0x%4x, Size = %4d, ", SampleIndex, Rate, Size));
		PRT(("Freq = %5d, InputLevel = %5d, OutputLevel = %5d, \n",
			Freq16>>16, SamplerLevel, FilterLevel ));
		
/* Calculate next frequency. */
		Freq16 = MulSF16( Freq16, DivUF16( Numer, Denom ));
		
	} while( (Freq16 >> 16) < FreqEnd );
	
cleanup:
	DeleteItem( MyCue );
	if( Att) DetachSample( Att );
	return Result;
}

/********************************************************************/
int32 RunFaders ( char *InsName )
{
	int32 doit;
	int32 Result;
	int32 i;
	uint32 joy;
	int32 IfNoteOn;
	int32 EnableA;
	int32 IfControlBOn;
	int32 OverheadTicks;
	int32 CurrentTicks;
	int32 MaximumTicks;

	doit = TRUE;
	IfNoteOn = TRUE;
	EnableA = TRUE;
	IfControlBOn = FALSE;
	
	Result = InitJoypad();
	CHECKRESULT( Result, InitJoypad );
	
	/* figure out how many ticks are already being used */
	OverheadTicks = 0;
	for(i=0; i<20; i++)
	{
		SleepAudioTicks(1);
		OverheadTicks += DSPGetTicks();
	}

	OverheadTicks /= 20;
	
	
	MaximumTicks = 0;
	while (doit)
	{
		Result = ReadJoypad( &joy );
		CHECKRESULT(Result,"ReadJoypad");
		if (joy & ControlStart)
		{
			doit = FALSE;
		}
		DriveFaders ( &MyFaderBlock, joy );

/* Toggle with ControlA */
		if (joy & ControlA)
		{
			if(EnableA)
			{
			
				if(IfNoteOn)
				{
					Result = ReleaseInstrument( gTestIns, NULL );
					IfNoteOn = FALSE;
				}
				else
				{
				
					Result = StartInstrument( gTestIns, NULL );
					IfNoteOn = TRUE;
				}
				CHECKRESULT(Result, "Start/ReleaseInstrument");
				EnableA = FALSE;
			}
		}
		else
		{
			EnableA = TRUE;
		}
		
/* Play with ControlB */
		if (joy & ControlB)
		{
			if(!IfControlBOn)
			{
				Result = StartInstrument( gTestIns, NULL );
				IfControlBOn = TRUE;
				IfNoteOn = TRUE;
			}
		}
		else
		{
			if(IfControlBOn)
			{
				Result = ReleaseInstrument( gTestIns, NULL );
				IfControlBOn = FALSE;
				IfNoteOn = FALSE;
			}
		}

		CHECKRESULT(Result, "Start/ReleaseInstrument");
		
		MoveTo( &GCon[0], 20, TOP_VISIBLE_EDGE + 5 );
		DrawText8( &GCon[0], CURBITMAPITEM, "3DO Instrument Tester" );
		MoveTo( &GCon[0], 20, TOP_VISIBLE_EDGE + 20 );
		DrawText8( &GCon[0], CURBITMAPITEM, InsName );
		MoveTo( &GCon[0], 220, TOP_VISIBLE_EDGE + 5 );

		DrawText8( &GCon[0], CURBITMAPITEM, "CT:" );

		CurrentTicks = DSPGetTicks() - OverheadTicks;

		MoveTo( &GCon[0], 245, TOP_VISIBLE_EDGE + 5 );
		DrawNumber( CurrentTicks );

		MoveTo( &GCon[0], 220, TOP_VISIBLE_EDGE + 20 );
		DrawText8( &GCon[0], CURBITMAPITEM, "MT:" );

		if(CurrentTicks > MaximumTicks)
			MaximumTicks = CurrentTicks;

		MoveTo( &GCon[0], 245, TOP_VISIBLE_EDGE + 20 );
		DrawNumber(MaximumTicks);
		
		Result = SwitchScreens();
		CHECKRESULT(Result,"SwitchScreens");
	}
cleanup:
	return Result;
}

/********************************************************************/
int32 DrawFaderScreen( void )
{
	int32 i;
	
	ClearScreen();
	for (i=0; i<MyFaderBlock.fdbl_NumFaders; i++)
	{
		DrawFader ( &MyFaders[i] );
	}
	return 0;
}

/********************************************************************/
int32 InitFaders( int32 NumFaders, int32 (*CustomFunc)() )
/* This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 * Returns non-FALSE if all is well, FALSE if error
 */
{
	int32 Result, i;
	Fader *fdr;
	
	InitFaderBlock ( &MyFaderBlock, &MyFaders[0], NumFaders, CustomFunc );

/* Change YMIN in faders.h %Q */
	for (i=0; i<NumFaders; i++)
	{
		fdr = &MyFaders[i];
		fdr->fdr_YMin = FADER_YMIN + ( i * FADER_SPACING ) + 15;
		fdr->fdr_YMax = fdr->fdr_YMin + FADER_HEIGHT;
	}
	
	DrawFaderScreen();
	Result = SwitchScreens();
	CHECKRESULT(Result,"SwitchScreens");

	DrawFaderScreen();
	Result = SwitchScreens();
	CHECKRESULT(Result,"SwitchScreens");
	
	Result = DisplayScreen( ScreenItems[0], 0 );
	if ( Result < 0 )
	{
		printf( "DisplayScreen() failed, error=%d\n", Result );
		goto cleanup;
	}

cleanup:
	return Result;
}

/********************************************************************/
int32 TermDemo( void )
{
	return TermGraphics();
}

/********************************************************************/

int32 CustomFaderFunc( int32 kn, int32 FaderValue )
{
	DBUG(("%8d => %s\n", FaderValue, Name[kn]));
	return TweakRawKnob(TestKnob[kn], FaderValue);
}

/********************************************************************/
int32 SetupKnobFader( Item Knob, Fader *fdr )
{
	int32 Result;
	
	TagArg Tags[4];

/* Get attributes of knob. */
	Tags[0].ta_Tag = AF_TAG_MIN;
	Tags[1].ta_Tag = AF_TAG_DEFAULT;
	Tags[2].ta_Tag = AF_TAG_MAX;
	Tags[3].ta_Tag = TAG_END;
	
	Result = GetAudioItemInfo ( Knob, Tags);
	CHECKRESULT(Result, "GetKnobInfo");
/* Now Pull Values from TagList */

	fdr->fdr_VMin  = (int32) Tags[0].ta_Arg;
PRT(("fdr->fdr_VMin = %d\n", fdr->fdr_VMin));
	fdr->fdr_Value = (int32) Tags[1].ta_Arg;
	fdr->fdr_VMax  = (int32) Tags[2].ta_Arg;
	fdr->fdr_Increment  = ((fdr->fdr_VMax - fdr->fdr_VMin) + 99) / 100;
	TweakRawKnob( Knob, fdr->fdr_Value );
	
cleanup:
	return Result;
}

int main(int argc, char *argv[])
{
	char *InsName;
	int32 i;
	int32 NumKnobs;
	int32 Result;
	int32 Entry;

	NumKnobs = 0;

/* FindMathFolio to get MathBase */
#ifdef cardinal2b
	Result = FindMathFolio();
#else
	Result = OpenMathFolio();
#endif
	if (Result < 0)
	{
		PrintError(0,"open math folio",0,Result);
		ERR(("Did you run operamath?\n"));
		goto cleanup;
	}
	
	PRT(("%s, Delay = %d\n", argv[0], DELAYTIME));
	
	Result = InitGraphics( 1 );
	CHECKRESULT(Result, "InitGraphics");

	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	Result = GenerateSineSamples( SMALLEST_SIZE, &SineWaves[0], NUM_SAMPLES );
	CHECKRESULT(Result,"GenerateSineSamples");
	
	MixerIns = LoadInstrument("directout.dsp", 0, 0);
	CHECKRESULT(MixerIns,"LoadInstrument");
	
/* Mixer must be started */
	Result = StartInstrument( MixerIns, NULL );
	
/* Load instrument definition/template */
	if (argc < 2)
	{
		PRT(("You forgot to specify an instrument!\n"));
		PRT(("Usage:   ta_faders <filename.dsp>\n"));
		goto cleanup;
	}
	InsName = argv[1];
	gTestIns = LoadInstrument( InsName, 0, 140);
	CHECKRESULT(gTestIns,"LoadInstrument");
	gSamplerIns = LoadInstrument("sampler.dsp", 0, 150);
	CHECKRESULT(gSamplerIns,"LoadInstrument samp");
	gFilterFollowerIns = LoadInstrument("envfollower.dsp", 0, 130);
	CHECKRESULT(gFilterFollowerIns,"LoadInstrument follower")
	gFilterMonitorIns = LoadInstrument("monitor.dsp", 0, 120);
	CHECKRESULT(gFilterMonitorIns,"LoadInstrument monitor");
	gSamplerFollowerIns = LoadInstrument("envfollower.dsp", 0, 130);
	CHECKRESULT(gSamplerFollowerIns,"LoadInstrument follower")
	gSamplerMonitorIns = LoadInstrument("monitor.dsp", 0, 120);
	CHECKRESULT(gSamplerMonitorIns,"LoadInstrument monitor");
	
	
	Result = ConnectInstruments (gSamplerIns, "Output", gTestIns, "Input");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (gSamplerIns, "Output", MixerIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (gTestIns, "Output", MixerIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (gTestIns, "Output", gFilterFollowerIns, "Input");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (gFilterFollowerIns, "Output", gFilterMonitorIns, "Input");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (gSamplerIns, "Output", gSamplerFollowerIns, "Input");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (gSamplerFollowerIns, "Output", gSamplerMonitorIns, "Input");
	CHECKRESULT(Result,"ConnectInstruments");
	
	NumKnobs = GetNumKnobs(gTestIns);
	PRT(("%d knobs.\n", NumKnobs));
	if( NumKnobs > MAX_FADERS )
	{
		PRT(("Too many knobs for this program to handle = %d\n", NumKnobs));
		NumKnobs = MAX_FADERS;
	}
	
	Result = InitFaders( NumKnobs , CustomFaderFunc);
	if ( Result < 0 )
	{
		printf( "InitFaders() failed, error=%d\n", Result );
		goto cleanup;
	}
	
/* Attach all available knobs */
	for (i=0; i<NumKnobs; i++)
	{
		Name[i] = GetKnobName( gTestIns, i);
		if (Name[i] != NULL)
		{
/* Attach knob so we can tweak what's there. */
			TestKnob[i] = GrabKnob (gTestIns, Name[i] );
			CHECKRESULT(TestKnob[i],"GrabKnob");
			SetupKnobFader( TestKnob[i], &MyFaders[i] );
			MyFaders[i].fdr_Name = Name[i];
#if 0
			if (strcmp(Name[i], "Amplitude") == 0)
			{
				TweakRawKnob( TestKnob[i], 0 );
				MyFaders[i].fdr_Value = 0;
			}
#endif
		}
	}
	
	Result = StartInstrument( gFilterFollowerIns, NULL );
	Result = StartInstrument( gFilterMonitorIns, NULL );
	Result = StartInstrument( gSamplerFollowerIns, NULL );
	Result = StartInstrument( gSamplerMonitorIns, NULL );
	Result = StartInstrument( gTestIns, NULL );
	
	DrawFaderScreen();
	RunFaders( InsName );
	SweepSpectrum( 100, 20000, 110, 100 );
	
	StopInstrument(gSamplerIns, NULL);
	StopInstrument(gFilterFollowerIns, NULL);
	StopInstrument(gFilterMonitorIns, NULL);
	StopInstrument(gTestIns, NULL);

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	for (i=0; i<NumKnobs; i++)
	{
		ReleaseKnob(TestKnob[i]);
	}
	UnloadInstrument( gTestIns );
	ReleaseKnob( LeftGainKnob );
	ReleaseKnob( RightGainKnob );
	UnloadInstrument( MixerIns );
	CloseAudioFolio();
	TermDemo();
	PRT(("Finished %s\n", argv[0] ));
	
	return( (int)Result );
}
