/* $Id: testgravel.c,v 1.4 1994/02/18 01:54:55 limes Exp $ */
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
int main(int argc, char *argv[])
{
	GravelControl Gravel, *grvl;
	int32 Result;
	int32 Amplitude, Freq1, Freq2, ModDepth;
	int32 i;
	
	PRT(("%s\n", argv[0]));
	
	grvl = &Gravel;
	
/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	CHECKRESULT(Result,"OpenAudioFolio");

/* Trace top level calls of Audio Folio */
	
	Freq1 = (argc > 1) ? atoi(argv[1]) : 0x200;
	Freq2 = (argc > 2) ? atoi(argv[2]) : 0x200;
	ModDepth = (argc > 3) ? atoi(argv[3]) : 0x200;
	Amplitude = (argc > 4) ? atoi(argv[4]) : 0x7FFF;
	PRT(("Freq1 = %d, Freq2 = %d, ModDepth = %d, Amp = %d\n", Freq1, Freq2, ModDepth, Amplitude));
	
	Result = InitGravel( grvl );
	CHECKRESULT(Result,"InitGravel");
	
	Result = StartGravel( grvl, Amplitude );
	CHECKRESULT(Result,"StartGravel");
	
	SleepAudioTicks( 300 );
	for(i=0; i<10; i++)
	{
		SetGravelAmplitude( grvl, Amplitude/(i+1) );
		SleepAudioTicks( 100 );
	}
	
	Result = StopGravel( grvl );
	CHECKRESULT(Result,"StopGravel");
	
	Result = TermGravel( grvl );
	CHECKRESULT(Result,"TermGravel");
	
	
cleanup:

	CloseAudioFolio();
	return((int) Result);
}

