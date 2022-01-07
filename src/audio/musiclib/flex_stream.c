/* $Id: flex_stream.c,v 1.10 1994/08/16 22:56:45 peabody Exp $ */
/****************************************************************
**
** Flexible Stream Routines
** You can read from these as a FileStream or a memory buffer.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
**
** 931008 PLB Changed return code for bad file to ML_ERR_BAD_FILE_NAME
** 940727 WJB Made file names const.
** 940816 WJB Replaced seek to end/beginning length algorithm with
**            reading stream->st_FileLength.
** 940816 WJB Fixed includes, stripped trailing spaces.
**
*****************************************************************/

#include <filestreamfunctions.h>
#include <flex_stream.h>
#include <mem.h>                /* malloc() */
#include <musicerror.h>         /* ML_ERR_ */
#include <stdio.h>              /* printf() */

#define PRT(x)  { printf x; }
#define ERR(x)  PRT(x)
#define DBUG(x)	/* PRT(x) */
#define TRACEFS(x)	/* PRT(x) */

/******************************************************/
int32 OpenFlexStreamFile( FlexStream *flxs, const char *filename )
{
/* Returns ML_ERR_BAD_FILE_NAME if no file found. */
DBUG(("OpenFlexStreamFile( flxs=0x%x, %s )\n",flxs,filename));
	flxs->flxs_FileStream =  OpenDiskStream( (char *)filename, 0);      /* !!! shouldn't need this cast */

DBUG(("OpenFlexStream: fileid = 0x%x\n", flxs->flxs_FileStream));
	if ((int32) flxs->flxs_FileStream == 0) return ML_ERR_BAD_FILE_NAME; /* 931008 */
	flxs->flxs_Image =  NULL;
	return ((int32) 0);
}

/******************************************************/
int32 CloseFlexStreamFile( FlexStream *flxs )
{
DBUG(("CloseFlexStream - %d\n", flxs->flxs_FileStream));

	if (flxs->flxs_FileStream != 0)
	{
		CloseDiskStream(flxs->flxs_FileStream);
		flxs->flxs_FileStream = 0;
	}
	return(0);
}

/******************************************************/
int32 OpenFlexStreamImage( FlexStream *flxs, char *Image, int32 NumBytes )
{
DBUG(("OpenFlexStreamImage( 0x%x, 0x%x, %d )\n",flxs,Image, NumBytes));
	flxs->flxs_FileStream =  0;
	flxs->flxs_Image =  Image;
	flxs->flxs_Cursor =  0;
	flxs->flxs_Size =  NumBytes;

	return( 0);
}

/******************************************************/
int32 CloseFlexStreamImage( FlexStream *flxs )
{
DBUG(("CloseFlexStreamImage( 0x%x, 0x%x, %d )\n",flxs ));
	flxs->flxs_FileStream =  0;
	flxs->flxs_Image =  NULL;
	flxs->flxs_Cursor =  0;
	flxs->flxs_Size =  0;
	return(0);
}


/******************************************************/
int32 ReadFlexStream( FlexStream *flxs, char *Addr, int32 NumBytes )
{
	int32 num;
	int32 Result;


	if(flxs->flxs_FileStream)
	{
TRACEFS(("ReadFlexStream: Read( 0x%x, 0x%x, %d )\n", flxs->flxs_FileStream, Addr, NumBytes));
#if 0
if(NumBytes == 0) ERR(( "ReadFlexStream: NumBytes == 0!\n" ));
#endif
		Result = ReadDiskStream( flxs->flxs_FileStream, Addr, NumBytes );
		if( Result < 0)
		{
			ERR(("ReadFlexStream: error 0x%x\n", Result));
		}
		return Result;
	}
	else if(flxs->flxs_Image)
	{
		if( (flxs->flxs_Size - flxs->flxs_Cursor) > NumBytes)
		{
			num = NumBytes;
		}
		else
		{
			num = flxs->flxs_Size - flxs->flxs_Cursor;
		}

		if( num > 0 )
		{
			memcpy( Addr, flxs->flxs_Image + flxs->flxs_Cursor, num );
			flxs->flxs_Cursor += num;
		}
		return num;
	}
	else
	{
		return ML_ERR_NOT_OPEN;
	}
}

/******************************************************/
int32 TellFlexStream( const FlexStream *flxs )
{
	return flxs->flxs_Cursor;
}

/******************************************************/
char *TellFlexStreamAddress( const FlexStream *flxs )
{
	char *p;

	p = (flxs->flxs_Image) ? (flxs->flxs_Image + flxs->flxs_Cursor) : NULL;

	return p;
}

/******************************************************/
int32 SeekFlexStream( FlexStream *flxs, int32 Offset, enum SeekOrigin Mode )
{
	int32 NewPos;

	if(flxs->flxs_FileStream)
	{
DBUG(("SeekFlexStream: Seek( 0x%x, %d, %d )\n", flxs->flxs_FileStream, Offset, Mode));
		return SeekDiskStream( flxs->flxs_FileStream, Offset, Mode );
	}
	else if(flxs->flxs_Image)
	{
		switch(Mode)
		{
			case SEEK_SET:
				NewPos = Offset;
				break;
			case SEEK_CUR:
				NewPos = flxs->flxs_Cursor + Offset;
				break;
			case SEEK_END:
				NewPos = flxs->flxs_Size + Offset;
				break;
			default:
				return ML_ERR_BAD_SEEK;
				break;
		}

		if( (NewPos < 0) || (NewPos > flxs->flxs_Size))
		{
			ERR(("iffSeekStream: seek out of image = %d\n", NewPos));
			return ML_ERR_BAD_SEEK;
		}
		else
		{
			flxs->flxs_Cursor = NewPos;
		}
		return flxs->flxs_Cursor;
	}
	else
	{
		return ML_ERR_NOT_OPEN;
	}
}


/******************************************************/
char *LoadFileImage( const char *Name, int32 *NumBytesPtr )
{
	Stream *str;
	char *Image;
	int32 NumBytes;
	int32 Result;

	str = OpenDiskStream( (char *)Name, 0);     /* !!! shouldn't need this cast */
	if (str == NULL)
	{
		ERR(("LoadFlexStreamImage: Could not open file = %s\n", Name));
		return NULL;
	}

    /* 940816: Replaced seek to end/beginning length algorithm with
               reading stream->st_FileLength. */
  #if 0
	NumBytes = SeekDiskStream( str, 0, SEEK_END );
	SeekDiskStream( str, 0, SEEK_SET );
  #endif
	NumBytes = str->st_FileLength;

	Image = (char *) malloc(NumBytes);
	if( Image == NULL )
	{
		ERR(("LoadFlexStreamImage: Insufficient memory.\n"));
		return NULL;
	}

	Result = ReadDiskStream( str, Image, NumBytes );
	if( Result != NumBytes)
	{
		ERR(("LoadFlexStreamImage: Could not read file.\n"));
		free(Image);
		return NULL;
	}

	CloseDiskStream( str );

	*NumBytesPtr = NumBytes;
	return Image;
}
