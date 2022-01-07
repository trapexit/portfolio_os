/* $Id: gravel.c,v 1.3 1994/02/18 01:54:55 limes Exp $ */
/***************************************************************
**
** Gravel for Race Game
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

#include "gravel.h"
 
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}
	
/***************************************************************/
int32 InitGravel( GravelControl *grvl )
{
	
	int32 Result;
	
/* Load "directout" for connecting to DAC. */
	grvl->grvl_OutputIns = LoadInstrument("directout.dsp",  0,  100);
	CHECKRESULT(grvl->grvl_OutputIns,"LoadInstrument");
	
/* Load RedNoise instrument */
	grvl->grvl_Noise1 = LoadInstrument("rednoise.dsp",  0,  100);
	CHECKRESULT(grvl->grvl_Noise1,"LoadInstrument");
	grvl->grvl_Noise2 = LoadInstrument("rednoise.dsp",  0,  100);
	CHECKRESULT(grvl->grvl_Noise2,"LoadInstrument");
	grvl->grvl_TimesPlus = LoadInstrument("timesplus.dsp",  0,  100);
	CHECKRESULT(grvl->grvl_TimesPlus,"LoadInstrument");
	
	grvl->grvl_DepthKnob = GrabKnob ( grvl->grvl_TimesPlus, "InputB");
	CHECKRESULT(grvl->grvl_DepthKnob,"GrabKnob");
	grvl->grvl_CenterKnob = GrabKnob ( grvl->grvl_TimesPlus, "InputC");
	CHECKRESULT(grvl->grvl_CenterKnob,"GrabKnob");
	grvl->grvl_AmpKnob = GrabKnob ( grvl->grvl_Noise2, "Amplitude");
	CHECKRESULT(grvl->grvl_CenterKnob,"GrabKnob");
	
	Result = ConnectInstruments (grvl->grvl_Noise1, "Output", grvl->grvl_TimesPlus, "InputA");
	CHECKRESULT(Result,"ConnectInstruments");
	
	Result = ConnectInstruments (grvl->grvl_TimesPlus, "Output", grvl->grvl_Noise2, "Frequency");
	CHECKRESULT(Result,"ConnectInstruments");
	
	Result = ConnectInstruments (grvl->grvl_Noise2, "Output", grvl->grvl_OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (grvl->grvl_Noise2, "Output", grvl->grvl_OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");

cleanup:
	return Result;
}

/***************************************************************/
int32 StartGravel( GravelControl *grvl, int32 Amplitude )
{
	TagArg Tags[4];
	
	Tags[0].ta_Tag = AF_TAG_RATE;
	Tags[0].ta_Arg = (void *) FREQ1;
	
	Tags[1].ta_Tag = AF_TAG_AMPLITUDE;
	Tags[1].ta_Arg = (void *) 0x7FFF;
	
	Tags[2].ta_Tag = TAG_END;
	
	StartInstrument(grvl->grvl_Noise1, Tags);
	
	TweakKnob( grvl->grvl_CenterKnob, FREQ2 );
	TweakKnob( grvl->grvl_DepthKnob, MODDEPTH );
	
	Tags[1].ta_Arg = (void *) Amplitude;
	StartInstrument(grvl->grvl_Noise2, Tags);
	StartInstrument(grvl->grvl_TimesPlus, Tags);
	StartInstrument( grvl->grvl_OutputIns, NULL );
	return 0;
}

/***************************************************************/
int32 SetGravelAmplitude( GravelControl *grvl, int32 Amplitude )
{
	return TweakKnob( grvl->grvl_AmpKnob, Amplitude );
}

/***************************************************************/
int32 StopGravel( GravelControl *grvl )
{
	StopInstrument(grvl->grvl_Noise1,NULL);
	StopInstrument(grvl->grvl_Noise2, NULL);
	StopInstrument(grvl->grvl_TimesPlus, NULL);
	StopInstrument( grvl->grvl_OutputIns, NULL );
	return 0;
}

/***************************************************************/
int32 TermGravel( GravelControl *grvl )
{
	int32 Result;
	StopGravel( grvl );
	UnloadInstrument( grvl->grvl_Noise1 );
	UnloadInstrument( grvl->grvl_Noise2 );
	UnloadInstrument( grvl->grvl_TimesPlus );
	UnloadInstrument( grvl->grvl_OutputIns );
	
	return Result;
}
