/* $Id: ta_printins.c,v 1.5 1994/02/18 01:54:55 limes Exp $ */
/****************************************************************
**
** Print Information about an Instrument
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "stdarg.h"
#include "operror.h"
#include "strings.h"
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
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}
	
int main(int argc, char *argv[])
{
	Item TestIns = 0;
	int32 i, NumKnobs, Result = -1;
	char *Name;
#define MAX_KNOBS 32
	Item TestKnob[MAX_KNOBS];
	int32 Min[MAX_KNOBS], Max[MAX_KNOBS], Default[MAX_KNOBS], err;
	TagArg Tags[] =
	{
		{AF_TAG_MIN, 0},
		{AF_TAG_DEFAULT, 0},
		{AF_TAG_MAX, 0},
		{TAG_END, 0},
	};

/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

/* Load instrument definition/template */
	if (argc < 2)
	{
		PRT(("Usage:  %s <filename.ofx>\n", argv[0]));
		goto cleanup;
	}
	
	PRT(("\n%s: loading %s\n", argv[0], argv[1] ));
	
	TestIns = LoadInstrument(argv[1], 0, 100);
	CHECKRESULT(TestIns,"LoadInstrument");
	
	NumKnobs = GetNumKnobs(TestIns);
	PRT(("%d knobs.\n", NumKnobs));
	
/* Attach all available knobs */
	PRT(("--Min-----Default-Max--------Name--\n"));
	for (i=0; i<NumKnobs; i++)
	{
		Name = GetKnobName( TestIns, i);
		if (Name != NULL)
		{
/* Attach knob so we can what's there. */
			TestKnob[i] = GrabKnob (TestIns, Name );
			CHECKRESULT(TestKnob[i],"GrabKnob");

/* Get attributes of knob. */
			err = GetAudioItemInfo ( TestKnob[i], Tags);
			CHECKRESULT(err, "GetAudioItemInfo");
/* Now Pull Values from TagList */
			Min[i]     = (int32) Tags[0].ta_Arg;
			Default[i] = (int32) Tags[1].ta_Arg;
			Max[i]     = (int32) Tags[2].ta_Arg;
            PRT(("  0x%04x, 0x%04x, 0x%04x  :  ", Min[i], Default[i], Max[i]));
			PRT(("%s\n", Name));
		}
	}
	
/* Print resource usage. */
	PRT(("Resource Utilization\n"));
	PRT(("  Nstruction memory = %4d\n", DSPGetInsRsrcUsed(TestIns,DRSC_N_MEM) ));
	PRT(("  EI Input   memory = %4d\n", DSPGetInsRsrcUsed(TestIns,DRSC_EI_MEM) ));
	PRT(("  Internal   memory = %4d\n", DSPGetInsRsrcUsed(TestIns,DRSC_I_MEM) ));
	PRT(("  EO Output  memory = %4d\n", DSPGetInsRsrcUsed(TestIns,DRSC_EO_MEM) ));
	PRT(("  RBASE4 reg blocks = %4d\n", DSPGetInsRsrcUsed(TestIns,DRSC_RBASE4) ));
	PRT(("  RBASE8 reg blocks = %4d\n", DSPGetInsRsrcUsed(TestIns,DRSC_RBASE8) ));
	PRT(("  Input FIFOs       = %4d\n", DSPGetInsRsrcUsed(TestIns,DRSC_IN_FIFO) ));
	PRT(("  Output FIFOs      = %4d\n", DSPGetInsRsrcUsed(TestIns,DRSC_OUT_FIFO) ));

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	for (i=0; i<NumKnobs; i++)
	{
		ReleaseKnob(TestKnob[i]);
	}
	UnloadInstrument( TestIns );
	CloseAudioFolio();
	return((int) Result);
}
