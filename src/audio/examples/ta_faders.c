
/******************************************************************************
**
**  $Id: ta_faders.c,v 1.25 1994/11/22 23:16:59 peabody Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC tpg/shell/dspfaders
|||	dspfaders - demo a DSP instrument
|||
|||	  Format
|||
|||	    dspfaders \<instrument name> [\<sample name>]
|||
|||	  Description
|||
|||	    This program is a diagnostic and experimentation tool to work with DSP
|||	    instruments. It loads the named DSP instrument, connects up a noise source
|||	    (to instruments with a signal input), connects up the named sample (for
|||	    sample player instruments), and connects the outputs to directout.dsp.
|||	    The instrument is then played and a set of faders is presented on the 3DO
|||	    display that can be used to control the knobs of the instrument being
|||	    tested.
|||
|||	    Control pad buttons:
|||
|||	        A                       Toggle between Start and Release of instrument.
|||
|||	        B                       Start instrument when pressed, Release instrument
|||	                                when released (like a key on a MIDI keyboard).
|||
|||	        X                       Quit
|||
|||	        Up or Down              Select a fader by moving up or down.
|||
|||	        Left or Right           Adjust current fader (coarse).
|||
|||	        LShift + Left or Right  Adjust current fader (fine).
|||
|||	    Also displays benchmarking information about the instrument being tested:
|||
|||	        CT      Current ticks used.
|||
|||	        MT      Max ticks used.
|||
|||	    This program is somewhat similar to patchdemo in nature, but is far handier
|||	    than patchdemo for testing out single instruments.
|||
|||	  Arguments
|||
|||	    \<instrument name>           Name of a DSP instrument to demo
|||	                                (e.g. svfilter.dsp).
|||
|||	    \<sample name>               Sample to connect to sample playing
|||	                                instruments (e.g. sampler.dsp). If not
|||	                                specified, no sample is connected to
|||	                                the instrument. There is also no
|||	                                provision to insure that the sample
|||	                                format and the sample player match.
|||
|||	  Implementation
|||
|||	    Released as an example in V20.
|||
|||	    Implemented as a command in V24 (source code no longer available).
|||
|||	  Location
|||
|||	    $c/dspfaders
|||
|||	  See Also
|||
|||	    patchdemo
|||
**/

#include "types.h"

#include "audio.h"
#include "debug.h"
#include "event.h"
#include "filefunctions.h"
#include "folio.h"
#include "graphics.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"

#include "audiodemo.h"

#define DEMOMODE

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%08lx\n", name, val)); \
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

#define MIXER_PRIORITY  (40)
#define LOW_PRIORITY  (50)
#define TEST_PRIORITY  (100)
#define HIGH_PRIORITY  (150)

#define MAX_KNOBS MAX_FADERS
Item TestKnob[MAX_KNOBS];
char *Name[MAX_KNOBS];

Item gTestIns = 0;

/********************************************************************/
/*********** Benchmark DSP Tools ************************************/
/********************************************************************/
typedef struct BenchmarkPatch
{
	Item   bmp_BenchBefore;
	Item   bmp_BenchAfter;
	Item   bmp_Maximum;
	Item   bmp_Subtract;
	Item   bmp_CurTicksProbe;
	Item   bmp_MaxTicksProbe;
} BenchmarkPatch;

/********************************************************************/
void DeleteBenchmarkPatch( BenchmarkPatch *bmp )
{
	if( bmp == NULL ) return;

	UnloadInstrument( bmp->bmp_BenchBefore );
	UnloadInstrument( bmp->bmp_BenchAfter );
	UnloadInstrument( bmp->bmp_Maximum );
	UnloadInstrument( bmp->bmp_Subtract );
	DeleteProbe( bmp->bmp_CurTicksProbe );
	DeleteProbe( bmp->bmp_MaxTicksProbe );

	FreeMem( bmp, sizeof( BenchmarkPatch ) );
}

/********************************************************************/
BenchmarkPatch *CreateBenchmarkPatch( int32 HighPri, int32 LowPri )
{
	BenchmarkPatch *bmp;
	int32 Result;

	bmp = (BenchmarkPatch *) AllocMem( sizeof( BenchmarkPatch ), MEMTYPE_FILL );
	if( bmp == NULL ) return NULL;

/* Create instruments */
	bmp->bmp_BenchBefore = LoadInstrument( "benchmark.dsp", 0, HighPri );
	CHECKRESULT( bmp->bmp_BenchBefore, "LoadInstrument benchmark" );

	bmp->bmp_BenchAfter = LoadInstrument( "benchmark.dsp", 0, LowPri );
	CHECKRESULT( bmp->bmp_BenchAfter, "LoadInstrument benchmark" );

	bmp->bmp_Subtract = LoadInstrument( "subtract.dsp", 0, LowPri-2 );
	CHECKRESULT( bmp->bmp_Subtract, "LoadInstrument subtract" );

	bmp->bmp_Maximum = LoadInstrument( "maximum.dsp", 0, LowPri-3 );
	CHECKRESULT( bmp->bmp_Maximum, "LoadInstrument maximum" );

/* Connect them into a patch. */
	Result = ConnectInstruments( bmp->bmp_BenchBefore, "Output",
	                              bmp->bmp_Subtract, "InputB" );
	CHECKRESULT( Result, "ConnectInstruments" );

	Result = ConnectInstruments( bmp->bmp_BenchAfter, "Output",
	                              bmp->bmp_Subtract, "InputA" );
	CHECKRESULT( Result, "ConnectInstruments" );

	Result = ConnectInstruments( bmp->bmp_Subtract, "Output",
	                              bmp->bmp_Maximum, "InputA" );
	CHECKRESULT( Result, "ConnectInstruments" );
/* Loop output of Maximum back to input for historical maximum. */
	Result = ConnectInstruments( bmp->bmp_Maximum, "Output",
	                              bmp->bmp_Maximum, "InputB" );
	CHECKRESULT( Result, "ConnectInstruments" );

/* Attach Probes to outputs. */
	bmp->bmp_CurTicksProbe = CreateProbe( bmp->bmp_Subtract, "Output", NULL );
	CHECKRESULT( Result, "CreateProbe" );
	bmp->bmp_MaxTicksProbe = CreateProbe( bmp->bmp_Maximum, "Output", NULL );
	CHECKRESULT( Result, "CreateProbe" );

/* Start everything. */
	Result = StartInstrument( bmp->bmp_BenchBefore, NULL );
	CHECKRESULT( Result, "StartInstrument" );
	Result = StartInstrument( bmp->bmp_BenchAfter, NULL );
	CHECKRESULT( Result, "StartInstrument" );
	Result = StartInstrument( bmp->bmp_Subtract, NULL );
	CHECKRESULT( Result, "StartInstrument" );
	Result = StartInstrument( bmp->bmp_Maximum, NULL );
	CHECKRESULT( Result, "StartInstrument" );

	return bmp;

cleanup:
	DeleteBenchmarkPatch( bmp );
	return NULL;
}

/********************************************************************/
int32 ReadBenchmarkPatch( BenchmarkPatch *bmp, int32 *CurTicks, int32 *MaxTicks )
{
	int32 Result;

	Result = ReadProbe( bmp->bmp_CurTicksProbe, CurTicks );
	CHECKRESULT( Result, "ReadProbe" );
	Result = ReadProbe( bmp->bmp_MaxTicksProbe, MaxTicks );
	CHECKRESULT( Result, "ReadProbe" );
cleanup:
	return Result;
}

/********************************************************************/
int32 RunFaders ( char *InsName )
{
	int32 doit = TRUE;
	int32 Result;
	uint32 joy;
	int32 IfNoteOn = TRUE;
	int32 EnableA = TRUE;
	int32 IfControlBOn = FALSE;
	int32 OverheadTicks;
	int32 CurrentTicks;
	int32 MaximumTicks;
	BenchmarkPatch *bmp = NULL;

	Result = InitJoypad();
	CHECKRESULT( Result, InitJoypad );

	bmp = CreateBenchmarkPatch( HIGH_PRIORITY, LOW_PRIORITY );
	if( bmp == NULL )
	{
		ERR(("Could not make BenchjmarkPatch!\n"));
		goto cleanup;
	}

	SleepAudioTicks( 20 );  /* Get background measurement. */

	Result = ReadBenchmarkPatch( bmp, &CurrentTicks, &OverheadTicks );
	CHECKRESULT(Result,"ReadBenchmarkPatch");

/* Start the instrument we want to test */
	Result = StartInstrument( gTestIns, NULL );

	while (doit)
	{
		Result = ReadJoypad( &joy );
		CHECKRESULT(Result,"ReadJoypad");
		if (joy & ControlX)
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

		Result = ReadBenchmarkPatch( bmp, &CurrentTicks, &MaximumTicks );
		CHECKRESULT(Result,"ReadBenchmarkPatch");

		MoveTo( &GCon[0], 245, TOP_VISIBLE_EDGE + 5 );
		DrawNumber( CurrentTicks - OverheadTicks );

		MoveTo( &GCon[0], 220, TOP_VISIBLE_EDGE + 20 );
		DrawText8( &GCon[0], CURBITMAPITEM, "MT:" );

		MoveTo( &GCon[0], 245, TOP_VISIBLE_EDGE + 20 );
		DrawNumber( MaximumTicks - OverheadTicks );

		Result = SwitchScreens();
		CHECKRESULT(Result,"SwitchScreens");
	}

cleanup:
	if( bmp )  DeleteBenchmarkPatch( bmp );
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
int32 InitFaders( int32 NumFaders, FaderEventFunctionP EventFunc )
/* This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 * Returns non-FALSE if all is well, FALSE if error
 */
{
	int32 Result, i;
	Fader *fdr;

	InitFaderBlock ( &MyFaderBlock, &MyFaders[0], NumFaders, EventFunc );

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
		printf( "DisplayScreen() failed, error=%ld\n", Result );
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

int32 CustomFaderFunc( int32 kn, int32 FaderValue, FaderBlock *fdbl )
{
	DBUG(("%8ld => %s\n", FaderValue, Name[kn]));
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
PRT(("fdr->fdr_VMin = %ld\n", fdr->fdr_VMin));
	fdr->fdr_Value = (int32) Tags[1].ta_Arg;
	fdr->fdr_VMax  = (int32) Tags[2].ta_Arg;
	fdr->fdr_Increment  = ((fdr->fdr_VMax - fdr->fdr_VMin) + 99) / 100;
	if (fdr->fdr_Increment < 1 ) fdr->fdr_Increment = 1;
	TweakRawKnob( Knob, fdr->fdr_Value );

cleanup:
	return Result;
}

int main(int argc, char *argv[])
{
	Item NoiseIns = 0;
	Item OutputIns = 0;
	char *InsName;
	int32 i;
	int32 NumKnobs;
	int32 Result;
	int32 useNoise = FALSE;
	Item Att, SampleItem;

	NumKnobs = 0;
	SampleItem = 0;
	Att = 0;

	Result = InitGraphics( 1 );
	CHECKRESULT(Result, "InitGraphics");

	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

/* Load instrument definition/template */
	if (argc < 2)
	{
		PRT(("You forgot to specify an instrument!\n"));
		PRT(("Usage: %s <filename.dsp>\n", argv[0]));
		goto cleanup;
	}
	InsName = argv[1];
	gTestIns = LoadInstrument( InsName, 0, TEST_PRIORITY);
	CHECKRESULT(gTestIns,"LoadInstrument");


	if( argc > 2)
	{
		SampleItem = LoadSample( argv[2] );
		CHECKRESULT(SampleItem,"LoadSample");
PRT(("Try to attach sample!"));
		Att = AttachSample( gTestIns, SampleItem, NULL );
		CHECKRESULT(Att,"AttachSample");
	}

/* Load a directout instrument to send the sound to the DAC. */
	OutputIns = LoadInstrument("directout.dsp",  0, MIXER_PRIORITY);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

	NoiseIns = LoadInstrument("noise.dsp", 0, 100 );
	CHECKRESULT(NoiseIns,"LoadInstrument noise");

	PRT(("Connect Instruments, Noise -> Input\n"));
	Result = ConnectInstruments (NoiseIns, "Output", gTestIns, "Input");
	if (Result < 0)
	{
		PRT(("%s has no Input\n", argv[1]));
	}
	else
	{
		useNoise = TRUE;
	}


/* Connect to Mixer */
	PRT(("Connect Instruments, test -> Mixer\n"));

/*
** Connect Sampler Instrument to DirectOut. Works for mono or stereo.
** Don't check result in this test so that we can test instruments with
** no output.  In a real app, check the Result.
*/
	Result = ConnectInstruments (gTestIns, "Output", OutputIns, "InputLeft");
	if( Result >= 0 )
	{
		ConnectInstruments (gTestIns, "Output", OutputIns, "InputRight");
	}
	else
	{
		Result = ConnectInstruments (gTestIns, "LeftOutput", OutputIns, "InputLeft");
		Result = ConnectInstruments (gTestIns, "RightOutput", OutputIns, "InputRight");
	}

	NumKnobs = GetNumKnobs(gTestIns);
	PRT(("%ld knobs.\n", NumKnobs));
	if( NumKnobs > MAX_FADERS )
	{
		PRT(("Too many knobs for this program to handle = %ld\n", NumKnobs));
		NumKnobs = MAX_FADERS;
	}

	Result = InitFaders( NumKnobs , CustomFaderFunc);
	if ( Result < 0 )
	{
		printf( "InitFaders() failed, error=%ld\n", Result );
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
#ifdef DEMOMODE
			if (strcmp(Name[i], "Amplitude") == 0)
			{
				TweakRawKnob( TestKnob[i], 0 );
				MyFaders[i].fdr_Value = 0;
			}
#endif
		}
	}

/* If the testIns has an input then start the noise */
	if(useNoise)
	  Result = StartInstrument(NoiseIns, NULL);

	DrawFaderScreen();

	RunFaders( InsName );

	StopInstrument(gTestIns, NULL);
	StopInstrument(NoiseIns, NULL);

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	for (i=0; i<NumKnobs; i++)
	{
		ReleaseKnob(TestKnob[i]);
	}
	UnloadInstrument( gTestIns );
	UnloadInstrument( NoiseIns );
	UnloadInstrument( OutputIns );
	if(SampleItem) UnloadSample(SampleItem);

	CloseAudioFolio();
	TermDemo();
	PRT(("Finished %s\n", argv[0] ));

	return( (int)Result );
}
