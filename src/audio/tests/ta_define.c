/* $Id: ta_define.c,v 1.4 1994/07/28 19:14:49 peabody Exp $ */
/***************************************************************
**
** Test DefineInsTemplate.
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
#include "driver.h"
#include "folio.h"
#include "stdio.h"
#include "mem.h"

#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"

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

#if 0   /* 940728: now using the version in music.lib */
char *LoadFileImage( char *Name, int32 *NumBytesPtr )
{
	Stream *str;
	char *Image;
	int32 NumBytes;
	int32 Result;


	str = OpenDiskStream( Name, 0);
	if (str == NULL)
	{
		ERR(("Could not open file = %s\n", Name));
		return NULL;
	}

	NumBytes = SeekDiskStream( str, 0, SEEK_END );
	SeekDiskStream( str, 0, SEEK_SET );
	Image = (char *) malloc(NumBytes);
	if( Image == NULL )
	{
		ERR(("Insufficient memory.\n"));
		return NULL;
	}

	Result = ReadDiskStream( str, Image, NumBytes );
	if( Result != NumBytes)
	{
		ERR(("Could not read file.\n"));
		free(Image);
		return NULL;
	}

	CloseDiskStream( str );

	*NumBytesPtr = NumBytes;
	return Image;
}
#endif

int main(int argc, char *argv[])
{
	Item SawTmp = -1;
	Item SawIns = -1;
	int32 Result = -1;
	char *Image;
	int32 NumBytes;

/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	if(argc < 2)
	{
		PRT(("%s{filename.dsp}\n"));
		return -1;
	}

	TraceAudio(0);
/* Load template file into memory to simulate stream loading. */
	Image = LoadFileImage( argv[1], &NumBytes );
	if( Image == NULL ) return -1;

	SawTmp = DefineInsTemplate( Image, NumBytes, 0, argv[1]);
	CHECKRESULT(SawTmp,"LoadInsTemplate");
	free( Image );

/* Make an instrument based on template. */
	SawIns = AllocInstrument(SawTmp, 0);
	CHECKRESULT(SawIns,"AllocInstrument");

	PRT(("Success!\n"));

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	FreeInstrument( SawIns );
	UnloadInsTemplate( SawTmp );
	TraceAudio(0);

	CloseAudioFolio();
	return((int) Result);
}
