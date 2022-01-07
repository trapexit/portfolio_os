#pragma force_top_level
#pragma include_only_once

/******************************************************************************
**
**  $Id: fmvdriver.h,v 1.2 1995/01/09 23:05:06 george Exp $
**
**  FMV audio and video device interface and definitions
**
******************************************************************************/

/* file: FMVDriver.h */
/* FMV driver definitions, declarations */
/* 4/22/93 George Mitsuoka */
/* The 3DO Company Copyright © 1993 */

#ifndef FMVDRIVER_HEADER
#define FMVDRIVER_HEADER

#include "types.h"
#include "item.h"
#include "device.h"
#include "driver.h"
#include "io.h"
#include "operror.h"

#define FMV_DRIVER_NAME			"FMVDriver"
#define FMV_AUDIO_DEVICE_NAME	"FMVAudioDevice"
#define FMV_VIDEO_DEVICE_NAME	"FMVVideoDevice"

#define CMD_CONTROL 			3

//=========================================================================
// Definition of IOReq and IOInfo IOReqOptions structure for FMV writes and 
// reads.

//=========================================================================

typedef struct FMVIOReqOptions
{
	uint32		FMVOpt_Flags;
	uint32		FMVOpt_PTS;
	uint32		FMVOpt_PTS_Offset;
	uint32		Reserved1;
	uint32		Reserved2;
	uint32		Reserved3;
} FMVIOReqOptions, *FMVIOReqOptionsPtr;

//=========================================================================
// FMVValidPTS and FMVPTSHIGHBIT flags
//
// When System or Audio packet streams are inuse the PTS for the
// decompressed data can be retreived.  Contained in the first long of the
// "io_Extension" field is 32 bits of the first PTS contained within the
// data.  The second 32 bits contains the offset within the request that
// most closely correspond to the PTS.
//
// If a valid PTS was found the high bit in the "io_Flags" field is set.
// If this bit is cleared you  should not use the values in the extensions
// field.
//=========================================================================

#define FMVValidPTS				0x80000000
#define FMVPTSHIGHBIT			0x40000000

#define FMV_PTS_INDEX			0
#define FMV_PTS_OFFSET			1

#define FMVGetPTSValue(ior) ((ior)->io_Extension[FMV_PTS_INDEX])
#define FMVGetPTSOffset(ior) ((ior)->io_Extension[FMV_PTS_OFFSET])

#define FMVSetPTSOffset(tsPtr, theOffset) \
	(((FMVIOReqOptions*)(tsPtr))->FMVOpt_PTS_Offset = (theOffset))
#define FMVSetPTSValue(tsPtr, thePTS) \
	(((FMVIOReqOptions*)(tsPtr))->FMVOpt_PTS = (thePTS))

//=========================================================================
// FMV_END_OF_STREAM_FLAG & FMV_DISCONTINUITY_FLAG flag 
//
// On a zero length write, the client can set the FMV_END_OF_STREAM_FLAG flag 
// in the ((FMVIOReqOptions*)ioInfo->ioi_CmdOptions)->FMVOpt_Flags to  
// indicate that no more writes for this stream will be forthcoming, 
// and therefore to finish any reads in progress (the audio drvier fills 
// the remaining reads with zeros).
//
// [TBD] On a read, we may want the driver to set the FMV_END_OF_STREAM_FLAG 
// flag in ioReq->io_Flags after the 
// ((FMVIOReqOptions*)ioInfo->ioi_CmdOptions)->FMVOpt_Flagsflag has been set 
// on a write and there is no more data to read.
//
// The FMV_DISCONTINUITY_FLAG is used similarly to tell the driver that
// a discontinuity in the compressed data is occuring, i.e., don't throw away
// any data, but here comes a discontinuity. [TBD] We may want the driver to 
// then inform the user when a read completes that corresponds to the 
// discontinuity.
//
// The FMV_FLUSH_FLAG is used similarly to tell the driver that a flush of 
// the compressed data is occuring, i.e., throw away anything you can to make 
// the transition as soon as possible. [TBD] We may want the driver to then 
// inform the user when a read completes that corresponds to the flush.
//
// FMVFlags macro takes ioInfo->ioi_CmdOptions as an argument and does the
// appropriate casting and indirection to read the flags.
//=========================================================================

#define FMV_END_OF_STREAM_FLAG		0x20000000
#define FMV_DISCONTINUITY_FLAG		0x10000000
#define FMV_FLUSH_FLAG				0x08000000

#define FMVFlags(opt) ( (opt) != 0 ? ((FMVIOReqOptions*)(opt))->FMVOpt_Flags : 0 )


Item CreateFMVDriver( void );

//=========================================================================
// Video Decompressor Capabililties flags
//=========================================================================

enum
{
	kVideoCODECOutDoesPlayThrough	= (1L<<0),
	kVideoCODECOutDoesDither		= (1L<<1),
	kVideoCODECOutDoesMPEGI			= (1L<<2),
	kVideoCODECOutDoesMPEGII		= (1L<<3),
	kVideoCODECOutDoesDMA			= (1L<<4),
	kVideoCODECOutDoesVariableSize	= (1L<<5),
	kVideoCODECOutDoesSquarePixel	= (1L<<6),
	kVideoCODECOutDoesNTSC			= (1L<<7),
	kVideoCODECOutDoesPAL			= (1L<<8),
	kVideoCODECOutDoesKeyFrame		= (1L<<9),
	kVideoCODECOutDoes1BPP			= (1L<<10),
	kVideoCODECOutDoes2BPP			= (1L<<11),
	kVideoCODECOutDoes4BPP			= (1L<<12),
	kVideoCODECOutDoes8BPP			= (1L<<13),
	kVideoCODECOutDoes16BPP			= (1L<<14),
	kVideoCODECOutDoes24BPP			= (1L<<15),
	kVideoCODECOutDoes32BPP			= (1L<<16)
};

#define	kMAX_CODEC_ARGS		20		/* The total possible arguments returned */

//=========================================================================
// CODEC device status structure is returned when a CMD_STATUS is made.
// The CODEC device arguments follow the standard device arguments.
//=========================================================================

typedef struct CODECDeviceStatus
{
	DeviceStatus	codec_ds;						/* Standard device status tag args */
	TagArg			codec_TagArg[kMAX_CODEC_ARGS];
} CODECDeviceStatus, *CODECDeviceStatusPtr;

//=========================================================================
// Definition of the Video CODEC device tag arguments.
//=========================================================================

enum
{
	VID_CODEC_TAG_CAPABILITIES = TAG_ITEM_LAST+1,	/* Return capabilties flags */
	VID_CODEC_TAG_HSIZE,							/* Horizontal output image size */
	VID_CODEC_TAG_VSIZE,							/* Vertical output image size */
	VID_CODEC_TAG_DEPTH,							/* Output image depth */
	VID_CODEC_TAG_DITHER,							/* The dither method */
	VID_CODEC_TAG_STANDARD,							/* Resampling method */
	VID_CODEC_TAG_PLAYTHROUGH,						/* Play through SlipStream */
	VID_CODEC_TAG_PLAY,								/* Decode all pictures */
	VID_CODEC_TAG_SKIPFRAMES,						/* Skip n pictures */
	VID_CODEC_TAG_KEYFRAMES,						/* Decode only I pictures */
	VID_CODEC_TAG_SCR,								/* System clock reference for playthrough */
	VID_CODEC_TAG_PAUSE								/* Stops decoder on specified frame type */
};

#define	kCODEC_SQUARE_RESAMPLE	0x00
#define	kCODEC_NTSC_RESAMPLE	0x01
#define	kCODEC_PAL_RESAMPLE		0x02

#define kCODEC_RANDOM_DITHER	0x00
#define	kCODEC_MATRIX_DITHER	0x01

#define kCODEC_PAUSE_ON_I_FRAME		0x01			/* Stop decode on I frame only */
#define kCODEC_PAUSE_ON_IP_FRAME	0x02			/* Stop decode on reference frame only */
#define kCODEC_PAUSE_ON_IPB_FRAME	0x03			/* Stop decode on any frame */

//=========================================================================
// Audio Decompressor Capabililties flags
//=========================================================================

enum
{
	kAudioCODECOutDoesPlayThrough			= (1L<<0),
	kAudioCODECOutDoesStereo				= (1L<<1),
	kAudioCODECOutDoesMono					= (1L<<2),
	kAudioCODECOutDoesAttenuation			= (1L<<3),
	kAudioCODECOutDoesDMA					= (1L<<4),
	kAudioCODECOutDoesVariableSampleRate	= (1L<<5),
	kAudioCODECOutDoesMPEGSystemStream		= (1L<<6),
	kAudioCODECOutDoesMPEGAudioPacket		= (1L<<7),
	kAudioCODECOutDoesMPEGAudioStream		= (1L<<8),
	kAudioCODECOutDoesSampleRateConvert		= (1L<<9),
	kAudioCODECOutDoesMultiChannel			= (1L<<10)
};

//=========================================================================
// Definition of the Audio CODEC device tag arguments.
//=========================================================================

enum
{
	AUD_CODEC_TAG_CAPABILITIES = TAG_ITEM_LAST+1,	/* Return capabilties flags */
	AUD_CODEC_TAG_ATTENUATION,						/* Adjust right and left channel attenuation */
	AUD_CODEC_TAG_SAMPLERATE,						/* Sample rate */
	AUD_CODEC_TAG_DATA_FORMAT,						/* Input data format */
	AUD_CODEC_TAG_PLAYTHROUGH,						/* Play directly through to audio output */
	AUD_CODEC_TAG_PLAY,								/* Start decoding dat */
	AUD_CODEC_TAG_FLUSH								/* ?? */
};

//=========================================================================
// The following defines are used to set the format of the sound data
// being passed to the sound driver.  There are three formats currently
// supported, MPEG audio packets, MPEG system streams, and MPEG audio
// stream.
//
// Following are the command values to set the data type for the sound
// device.  These defines are placed in the "ioi_Command" field.
//
//	FMV_AUDIO_STREAM			Audio driver accepts MPEG Audio Streams
//	FMV_AUDIO_PACKET			Audio driver accepts MPEG Audio Packets
//	FMV_AUDIO_SYSTEM_STREAM		Audio driver accepts MPEG System Streams
//	FMV_AUDIO_BYPASS			Audio hardware accepts raw data.
//
//=========================================================================

#define	FMV_AUDIO_STREAM			0x00L
#define	FMV_AUDIO_PACKET			0x01L
#define	FMV_SYSTEM_STREAM			0x02L
#define	FMV_AUDIO_BYPASS			0x03L

#define FMV_AUDIO_SAMPLERATE_44		44100L
#define FMV_AUDIO_SAMPLERATE_48		48000L
#define FMV_AUDIO_SAMPLERATE_32		32000L

#endif
