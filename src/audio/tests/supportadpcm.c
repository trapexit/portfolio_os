/***************************************************************
**
** Load the ADPCM decoder instrument.
**
** The application should do this if it is going to use ADPCM.
** We are working on a more automatic method.
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

int main(int argc, char *argv[])
{
	Item ADPCMDecoder;
	int32 Result = -1;


/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}


/* Load external instruments in User mode. */
	ADPCMDecoder = LoadInstrument("decodeadpcm.dsp", 0, 0);
	if (ADPCMDecoder < 0)
	{
		ERR(("LoadInsExternal error = 0x%x\n", ADPCMDecoder));
		Result =  ADPCMDecoder;
		goto error;
	}
	else
	{
		PRT(("ADPCM DVI Decoder loaded for use by other instruments = 0x%x.\n",
			ADPCMDecoder ));
	}
	
	WaitSignal(1);
error:
	CloseAudioFolio();
	return((int) Result);
}
