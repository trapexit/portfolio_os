/* file: FMVROM.c */
/* Hollywood Driver ROM access code */
/* 4/7/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

/* this code provides access to the FMV subsystem ROM.
   this is the only code which should need to change to match the Opera
   ROM access mechanism (it definitely must change) */

#include "types.h"

#include "FMV.h"
#include "FMVErrors.h"
#include "FMVROM.h"
#include "Woody.h"

#ifdef THINK_C
#include <stdio.h>
#include <stdlib.h>
#endif

uint32 FMVROMBase = 0L;

#ifdef FMVINROM

/* this code really reads the Woody ROM */

uint32 FMVROMAccessInit()
{
	uint32 temp;
	
	DEBUGP(("FMVROMAccessInit:\n"));
	
	/* setup ROM base */
	FMVROMBase = FMVROMBASEADDRESS;
	
	return( kNoErr );
}

uint32 FMVROMGetWord( uint32 index )
{
	uint32 i, theWord;
	
	/* get the longword at index (duh) */
	
	index <<= 2;				/* convert longword index to byte address */
	index += FMVROMBase;		/* add ROM base offset */
	
	theWord = 0L;
	for( i = 0; i < 4; i++ )	/* assemble four byte reads into one longword */
	{
		theWord <<= 8;
		theWord |= FMVReadROM( index ) & 0xffL;
		index++;
	}
	return( theWord );
}

void FMVROMAccessEnd()
{
}

#else

/* this code models the ROM in RAM */

static uint32 *theFMVROM = (uint32 *) 0;

uint32 FMVROMAccessInit()
{
	FILE *ROMFile;
	uint32 i;
	
	/* allocate space for the ROM image */
	if( !(theFMVROM = (uint32 *) malloc( FMVROMSIZE * sizeof( uint32 ) )) )
	{
		/* outta here... */
		return( kErrAllocateROMMemoryFailed );
	}

	/* read the ROM image from (yeah, yeah, bad, bad) FMVROMFILE */
	if( (ROMFile = fopen(FMVROMFILE, "r")) == (FILE *) NULL )
	{
		return( kErrOpenROMFileFailed );
	}
#ifdef NEVER	
	/* THINK C doesn't want to read the whole thing at once, so read it a little at a time */
	for( i = 0; i < FMVROMSIZE; i++ )
		if( fread( &theFMVROM[ i ], sizeof( uint32 ), 1, ROMFile ) != 1 )
		{
			fclose( ROMFile );
			return( kErrReadROMFileFailed );
		}
#endif
	if( fread( theFMVROM, sizeof( uint32 ), FMVROMSIZE, ROMFile ) != FMVROMSIZE )
	{
		fclose( ROMFile );
		return( kErrReadROMFileFailed );
	}
	
	fclose( ROMFile );
	
	return( kNoErr );
}

uint32 FMVROMGetWord( uint32 index )
{
	/* get the longword at index (duh) */
	return( theFMVROM[ index ] );
}

void FMVROMAccessEnd()
{
	/* free the ROM image space */
	if( theFMVROM )
		free( theFMVROM );
}

#endif

uint32 FMVROMFindResourceIndex( FMVROMChunkType resourceType )
{
	uint32 ROMDirectoryIndex, resourceCount, resource, resourceTagIndex;
	uint32 resourceTag, foundResource = 0;
	
	/* find the resourceType resource in the FMV ROM */
	
	/* locate ROM directory */
	ROMDirectoryIndex = FMVROMGetWord( kFMVROMDirectoryOffset );
	
	/* find number of resources */
	resourceCount = FMVROMGetWord( ROMDirectoryIndex + kFMVChunkCount );
	
	/* loop through resource tags */
	resourceTagIndex = ROMDirectoryIndex + kFMVChunkTagsStart;
	
	for( resource = 0; resource < resourceCount; resource++ )
	{
		resourceTag = FMVROMGetWord( resourceTagIndex );
		
		if( foundResource = (ChunkType( resourceTag ) == resourceType) )
			break;
		
		resourceTagIndex++;
	}
	if( !foundResource )
		return( 0 );
	
	return( ChunkOffset( resourceTag ) );
}
