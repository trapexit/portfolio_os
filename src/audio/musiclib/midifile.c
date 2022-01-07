/* $Id: midifile.c,v 1.15 1994/05/13 21:16:20 hayes Exp $ */
/*
** MIDI File Parser
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "types.h"
#include "stdio.h"
#include "debug.h"
#include "nodes.h"
#include "mem.h"
#include "operamath.h"
#include "stdarg.h"
#include "strings.h"

/* Include this when using the Audio Folio */
#include "audio.h"
#include "music.h"

#define	PRT(x)	 { printf x; }
#define	ERR(x)	 PRT(x)
#define	DBUG(x)	 /* PRT(x) */
#define	DBUG2(x) /* PRT(x) */

/************************************************************/

Err ParseMFHeader( MIDIFileParser *mfpptr )
{
	int32 type, size, num, desired;
	int16 pad[4];
	int32 lpad[4];

	desired = CHKID('M','T','h','d');
/* Read identifying 'MThd' */
	num = ReadFlexStream (&mfpptr->mfp_FlexStream, (char *) &type, 4);
	lpad[0]=type;
	lpad[1]=0;
DBUG(("Header type = %s = $%x\n", lpad, type));
	lpad[0]=desired;
	lpad[1]=0;
DBUG(("Expected = %s = $%x\n", lpad, desired));

	if (num != 4) return (ML_ERR_END_OF_FILE);
	if (type != desired) return(ML_ERR_BAD_FORMAT);

/* Size of header "chunk" must be six. */
	num = ReadFlexStream ( &mfpptr->mfp_FlexStream, (char *) &size, 4);
	if (num != 4) return(ML_ERR_END_OF_FILE);
	if (size != 6) return(ML_ERR_BAD_FORMAT);

/*
** Read array into int16 array then into struct for
** machines that align to int32 words.
*/
	num = ReadFlexStream ( &mfpptr->mfp_FlexStream, (char *) &pad[0], 6);
	if (num != 6) return (ML_ERR_END_OF_FILE);
	mfpptr->mfp_Format = pad[0];
	mfpptr->mfp_NumTracks = pad[1];
	mfpptr->mfp_Division = pad[2];
	mfpptr->mfp_Tempo = 0;
	mfpptr->mfp_Shift = 0;

/* Adjust time scaling to reduce tick rate. */
DBUG(("ParseMFHeader: Original Division = %d\n", mfpptr->mfp_Division ));
	while( mfpptr->mfp_Division > 160 )
	{
		mfpptr->mfp_Division = mfpptr->mfp_Division >> 1;
		mfpptr->mfp_Shift++;
	}
DBUG(("ParseMFHeader: Reduced Division = %d\n", mfpptr->mfp_Division ));
DBUG(("ParseMFHeader: Shift = %d\n", mfpptr->mfp_Shift ));

	mfpptr->mfp_Rate = Convert32_F16( mfpptr->mfp_Division * 2 );
	
	return(0);
}

/**************************************************************
** ReadMFTrackHeader returns size of Track
**************************************************************/
int32 ReadMFTrackHeader( MIDIFileParser *mfpptr )
{
	int32 type, size, num;

/* Read identifying 'MTrk' */
	num = ReadFlexStream ( &mfpptr->mfp_FlexStream, (char *) &type, 4);
DBUG(("ParseMFTrack: Type = 0x%x\n", type));
	if (num != 4)
	{
		ERR(("ReadMFTrackHeader: End Of File!\n"));
		return (ML_ERR_END_OF_FILE);
	}
	
	if (type != CHKID('M','T','r','k'))
	{
		ERR(("ReadMFTrackHeader: Not 'Mtrk', got 0x%x\n", type));
		return(-7);
	}
	
	num = ReadFlexStream ( &mfpptr->mfp_FlexStream, (char *) &size, 4 );
	if (num != 4)
	{
		ERR(("ReadMFTrackHeader: End Of File!\n"));
		return(ML_ERR_END_OF_FILE);
	}
	
	return size;
}
	
/************************************************************/

Err ScanMFTrack( MIDIFileParser *mfpptr, int32 Size)
{
	mfpptr->mfp_Time = 0;
	mfpptr->mfp_BytesLeft = Size;   /* Set bytes left in Track. */
	while ( mfpptr->mfp_BytesLeft > 0)
	{
		if (ParseMFEvent( mfpptr ) < 0) return (ML_ERR_END_OF_FILE);
	}
	return 0;
}

/************************************************************/

Err ParseMFTrack( MIDIFileParser *mfpptr )
{
	int32 Size, Result;

	Size = ReadMFTrackHeader( mfpptr );
	if( Size < 0 ) return Size;
	
	if (mfpptr->mfp_HandleTrack != NULL)
	{
		Result = mfpptr->mfp_HandleTrack(mfpptr, Size);
		if( Result < 0)
		{
			return Result;
		}
	}

	return ScanMFTrack( mfpptr, Size );
}

/************************************************************/

int32 GetMFChar( MIDIFileParser *mfpptr )
{
	char c[4];
	
	mfpptr->mfp_BytesLeft--;
	ReadFlexStream( &mfpptr->mfp_FlexStream, &c[0], 1 );
DBUG2(("GetMFChar: c=$%x\n", c[0]));
	return((int32) c[0]);

}

/************************************************************/

Err ParseMFMetaEvent( MIDIFileParser *mfpptr )
{
	int32 metasize;
	int32 Type;
	uint8 *data;
	uint8 pad[4];
	frac16 BeatsPerSecond;
	
	data = NULL;
	
/* Get type */
	Type = (unsigned char) GetMFChar( mfpptr );
DBUG(("MetaEvent Type = $%x", Type));

	metasize = ParseMFVLN(mfpptr);
DBUG((", MetaEvent Size = %x\n", metasize));

	if(metasize == 0)
	{
		data = NULL;
	}
	else
	{
		data = (uint8 *)EZMemAlloc( metasize, 0 );
		if(data == NULL) return ML_ERR_NOMEM;
		ReadFlexStream( &mfpptr->mfp_FlexStream, data, metasize );
		mfpptr->mfp_BytesLeft -= metasize;
	}
	
	switch( Type )
	{
		case 0x51:   /* Tempo */
			if( mfpptr->mfp_Tempo == 0)
			{
				pad[0] = 0;
				pad[1] = data[0];
				pad[2] = data[1];
				pad[3] = data[2];
				mfpptr->mfp_Tempo = *((uint32 *) pad);
/* Calculate Rate in ticks/beat. */
				BeatsPerSecond= DivUF16(1000000, mfpptr->mfp_Tempo);
				mfpptr->mfp_Rate = MulUF16( Convert32_F16(mfpptr->mfp_Division), BeatsPerSecond );
				
PRT(("ParseMFMetaEvent: Tempo = %d, Rate = %d\n", mfpptr->mfp_Tempo,
		ConvertF16_32(mfpptr->mfp_Rate)));
			}
			else
			{
				ERR(("ParseMFMetaEvent: multiple Tempos ignored.\n"));
			}
			break;
			
/*		default:
			DumpMemory( data, metasize ); */
	}
	
	if(data) EZMemFree(data);

	return(0);
}

/************************************************************/

int32 ParseMFVLN( MIDIFileParser *mfpptr )
{
	int32 accum=0, flag=1;
	int32 datum;

	do
	{
		datum = GetMFChar( mfpptr );
DBUG2(("ParseMFVLN: datum=$%x\n", datum));
		if (datum < 0) return(datum);
		flag = ((datum & 0x80) != 0);
		accum = ((datum) & 0x7F) | (accum << 7);
	} while (flag);

DBUG(("ParseMFVLN: accum=%d\n", accum));
	return(accum);
}

/************************************************************/

Err ParseMFEvent( MIDIFileParser *mfpptr )
{
	int32 rtime, command;
	int32 NumToRead, NumData, num;
	uint8 pad[8];
	int32 i;

	rtime = ParseMFVLN(mfpptr);
	if (rtime < 0) return (ML_ERR_BAD_FORMAT);
	mfpptr->mfp_Time += rtime;
	

	command = (uint8) GetMFChar( mfpptr );
	
DBUG(("ParseMFEvent: Time = %d, Com = 0x%x\n", mfpptr->mfp_Time, command));

/* New command */
	if( command == 0xFF )
	{
		ParseMFMetaEvent(mfpptr);
	}
	else
	{
/* Check for command byte versus running status */
		i = 0;
		 if(command == 0xF8) /* 930728 */
		{
/* Real Time Message does not affect Running Status */
			pad[i++] = (uint8) command;
			NumData = MIDIDataBytesPer(command);
			NumToRead = NumData;
		}
/* Is it a command? */
		else if(command & 0x80)
		{
			pad[i++] = (uint8) command;
			NumData = MIDIDataBytesPer(command);
			NumToRead = NumData;
			mfpptr->mfp_NumData = NumData;
			mfpptr->mfp_RunningStatus = command;
		}
/* Data byte so use running status from earlier command. */
		else
		{
			pad[i++] = (uint8) mfpptr->mfp_RunningStatus;
			pad[i++] = (uint8) command;
			NumData = mfpptr->mfp_NumData;
			NumToRead = NumData - 1; /* already got one byte */
		}
		
		if ((NumData > 0) && (NumData < 4))
		{
			num = ReadFlexStream ( &mfpptr->mfp_FlexStream, (char *)&pad[i], NumToRead );	
			if (num < NumToRead) return(-10);
			mfpptr->mfp_BytesLeft -= num;
			if (mfpptr->mfp_HandleEvent != 0)
			{
				mfpptr->mfp_HandleEvent(mfpptr, pad, NumData+1);
			}
		}
	}
	return(0);
}

static char DataBytesPer[] = 
{
	2,  2,  2,  2,  1,  1,  2,  0,   /* 0x80, 0x90, ..., 0xE0, unused */
	1,  1,  2,  1,  0,  0,  0,  0,   /* 0xF0, 0xF1, ..., 0xF7 */
	0,  0,  0,  0,  0,  0,  0,  0    /* 0xF8, 0xF9, ..., 0xFF */
};

int32 MIDIDataBytesPer(int32 command)
{
	int32 indx;
	
	if ((command & 0xF0) == 0xF0)
	{
		indx = ((command >> 4) & 0x0F) + 8;
	}
	else
	{
		indx = (command >> 4) & 0x07;
	}
	return(DataBytesPer[indx]);
}
