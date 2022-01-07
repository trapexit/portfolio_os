/* $Id: selectsampleplayer.c,v 1.9 1994/10/25 01:16:36 phil Exp $ */
/****************************************************************
**
** Select the appropriate sample playing instrument for a sample.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 931110 PLB Moved SelectSamplePlayer() out to own file.
** 940727 WJB Added autodocs.
** 941024 PLB Added dcsqxdvarmono.dsp and adpcmvarmono.dsp
****************************************************************/

#include "types.h"
#include "ctype.h"
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
#include "strings.h"
#include "operror.h"
#include "audio.h"
#include "music.h"

#include "music_internal.h"     /* version stuff */

#define	PRT(x)	{ printf x; }
#define	ERR(x)         PRT(x)
#define	DBUG(x)        /* PRT(x) */

/**
 |||	AUTODOC PUBLIC mpg/musiclib/score/selectsampleplayer
 |||	SelectSamplePlayer - Gets the name of the instrument to play the sample.
 |||
 |||	  Synopsis
 |||
 |||	    char *SelectSamplePlayer( Item Sample, int32 IfVariable )
 |||
 |||	  Description
 |||
 |||	    This procedure gets and returns the name of the appropriate instrument
 |||	    that will play the given sample.  If there is no appropriate instrument to
 |||	    play the sample, it returns NULL.  This procedure looks at the sample
 |||	    rate, the number of channels (mono or stereo), width (8 or 16), and
 |||	    compression type to determine the appropriate instrument for the sample.
 |||	    It also lets you specify whether you need to vary the pitch or play only
 |||	    at the original pitch.
 |||
 |||	  Arguments
 |||
 |||	    Sample                       The item number of the sample to be played.
 |||
 |||	    IfVariable                   A value indicating whether or not the pitch
 |||	                                 is to vary.  If TRUE, the pitch will vary.
 |||	                                 If FALSE, the pitch won't vary.
 |||
 |||	  Return Value
 |||
 |||	    This procedure returns the name of the instrument that will play the given
 |||	    sample, or NULL if there is no appropriate instrument to play the sample.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V20.
 |||
 |||	  Associated Files
 |||
 |||	    score.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    LoadSoundFile()
 |||
**/
char *SelectSamplePlayer( Item Sample , int32 IfVariable)
{
	char *InsName = NULL;
	int32 Channels;
	uint32 CompressionType;
	int32 Bits;
	frac16 SampleRate;
	int32 Result;
	TagArg Tags[5];
	int32 HalfSampleRate;
	int32 iSampleRate;

	PULL_MUSICLIB_VERSION;

/* Get information from sample. */
	Tags[0].ta_Tag = AF_TAG_CHANNELS;
	Tags[1].ta_Tag = AF_TAG_COMPRESSIONTYPE;
	Tags[2].ta_Tag = AF_TAG_NUMBITS;
	Tags[3].ta_Tag = AF_TAG_SAMPLE_RATE;
	Tags[4].ta_Tag = TAG_END;
	Result = GetAudioItemInfo( Sample, Tags );
	if( Result < 0) return NULL;

	Channels = (int32) Tags[0].ta_Arg;
	CompressionType = (uint32) Tags[1].ta_Arg;
DBUG(("CompressionType = 0x%x\n", CompressionType));
	Bits = (int32) Tags[2].ta_Arg;
	SampleRate = (frac16) Tags[3].ta_Arg;
	iSampleRate = ConvertF16_32(SampleRate);
DBUG(("SampleRate = %d\n", iSampleRate));
	HalfSampleRate = (((iSampleRate+1)>>1) == 11025);

	if (IfVariable)
	{
		if (Channels == 1)
		{
			if (CompressionType == 0)
			{
				if (Bits == 16)
				{
					InsName = "sampler.dsp";
				/*	InsName = "varmono16.dsp"; */
				}
				else
				{
					InsName = "varmono8.dsp";
				}
			}
			else if (CompressionType == ID_SDX2)
			{
				InsName = "dcsqxdvarmono.dsp";
			}
			else if (CompressionType == ID_ADP4)
			{
				InsName = "adpcmvarmono.dsp";
			}
		}
	}
	else if (Channels == 1)
	{
		if (CompressionType == ID_SDX2)
		{
			if (HalfSampleRate)
			{
				InsName = "dcsqxdhalfmono.dsp";
			}
			else
			{
				InsName = "dcsqxdmono.dsp";
			}
		}
		else if (CompressionType == ID_ADP4)
		{
			if (HalfSampleRate)
			{
				InsName = "adpcmhalfmono.dsp";
			}
			else
			{
				InsName = "adpcmmono.dsp";
			}
		}
		else
		{
			if(Bits == 8)
			{
				if (HalfSampleRate)
				{
					InsName = "halfmono8.dsp";
				}
				else
				{
					InsName = "fixedmono8.dsp";
				}
			}
			else
			{
				if (HalfSampleRate)
				{
					InsName = "halfmonosample.dsp";
				}
				else
				{
					InsName = "fixedmonosample.dsp";
				}
			}
		}
	}
	if (Channels == 2)
	{
		if (CompressionType == ID_SDX2)
		{
			if (HalfSampleRate)
			{
				InsName = "dcsqxdhalfstereo.dsp";
			}
			else
			{
				InsName = "dcsqxdstereo.dsp";
			}
		}
		else if (CompressionType == ID_ADP4)
		{
			if (HalfSampleRate)
			{
				InsName = "adpcmhalfstereo.dsp";
			}
			else
			{
				InsName = "adpcmstereo.dsp";
			}
		}
		else
		{
			if(Bits == 8)
			{
				if (HalfSampleRate)
				{
					InsName = "halfstereo8.dsp";
				}
				else
				{
					InsName = "fixedstereo8.dsp";
				}
			}
			else
			{
				if (HalfSampleRate)
				{
					InsName = "halfstereosample.dsp";
				}
				else
				{
					InsName = "fixedstereosample.dsp";
				}
			}
		}
	}
	return InsName;
}
