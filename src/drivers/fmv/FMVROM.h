/* file: FMVROM.h */
/* Hollywood Driver ROM format definitions */
/* 4/7/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef FMVROM_HEADER
#define FMVROM_HEADER

#include "types.h"

/* expansion ROM header */
/* absolute longword indices into ROM */

#define FMVROMBASEADDRESS	0x00180000L	/* base address of FMV portion of ROM */
//#define FMVROMBASEADDRESS	0x0L		/* base address of FMV portion of ROM */

typedef
	enum
	{
		kFMVROMId = 0,
		kFMVROMLength,
		kFMVROMCRC,
		kFMVROMDriverEntry,
		kFMVROMRevision,
		kFMVROMDirectoryOffset,
		kFMVROMDriverSize,
		kFMVROMDriverCode
	}
	FMVROMHeaderField;

/* directory header */
/* relative longword offsets from beginning of directory in ROM */

typedef
	enum
	{
		kFMVId = 0,
		kFMVRevision,
		kFMVReserved,
		kFMVComment,
		kFMVChunkCount = 23,
		kFMVChunkTagsStart
	}
	FMVROMDirectoryHeaderField;

/* the chunkTags beginning at kFMVChunkTagsStart consists of chunkTags in 16:16 format.
   the high 16 bits contain the chunk type (defined below) and the low 16 bits
   contain an absolute ROM offset (in longwords) to the chunk */

#define ChunkType( chunkTag )	(((uint32) chunkTag >> 16) & 0xffffl)
#define ChunkOffset( chunkTag )	((uint32) chunkTag & 0xffffl)
#define ChunkTag( type, offset)	(((uint32) type << 16) | ((uint32) offset & 0xffffl))

typedef
	enum
	{
		kFMV450BootCodeType = 1,
		kFMV450MPEG16CodeType,
		kFMV450MPEG24CodeType,
		kFMV450MPEGVideoDataType,
		kFMV450MPEGAudioDataType
	}
	FMVROMChunkType;
	
/* CL450 µcode header */
/* relative longword offsets from beginning of µcode in ROM */

typedef
	enum
	{
		kFMV450IMEMAddress = 0,		/* IMEM starting address */
		kFMV450StartingAddress,		/* µcode entry point */
		kFMV450SegmentCount,		/* number of segments in this µcode resource */
		kFMV450Segment0Address,		/* beginning of µcode segments */
		kFMV450Segment0Size,		/* size of 1st segment code */
		kFMV450Segment0Code			/* beginning of 1st segment code */
	}
	FMV450CodeHeaderField;
	
/* the µcode segments in the above contains sequential µcode segments. each segment
   begins with a uint32 load address (longword address), a segment size (in
   longwords), and (segment size) longwords of µcode */

/* MPEG data header */
/* relative longword offsets from beginning of MPEG data in ROM */

typedef
	enum
	{
		kFMVMPEGDataSize = 0,		/* number of longwords to follow */
		kFMVMPEGDataStart			/* beginning of actual MPEG data */
	}
	FMVMPEGDataHeaderField;
	
/* misc. ROM #defines */

#define FMVROMID		0xaa55aa55L	/* FMV Expansion ROM Identifier */
#define FMVROMSIZE		0x8000L		/* 32768 longwords, 128k bytes */
#define FMVROMREVISION	0x00010000L	/* in 16:16 format */
#define FMVSTARTOFFSET	0x4L		/* CL450 start instruction offset */

#ifndef	FMVID
#define FMVID			0x484c5744L	/* 'HLWD' */
#endif

#define FMVCOMMENT		"Copyright © The 3DO Company 1993"

#define FMVROMFILE		"FMVROM.image"

/* ROM access prototypes */

extern uint32 FMVROMAccessInit( void );
extern uint32 FMVROMGetWord( uint32 index );
extern void FMVROMAccessEnd( void );
extern uint32 FMVROMFindResourceIndex( FMVROMChunkType resourceType );

#endif
