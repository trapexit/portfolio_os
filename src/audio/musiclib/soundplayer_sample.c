/******************************************************************************
**
**  $Id: soundplayer_sample.c,v 1.17 1994/10/11 00:57:36 peabody Exp $
**
**  Advanced Sound Player - sample item class.
**
**  By: Bill Barton
**
**  Copyright (c) 1994, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  940809 WJB  Added to music.lib.
**  940819 WJB  Added some debug code.
**  940901 WJB  Added magic marker creation for sample item loop points.
**  940922 WJB  Added error handler for non-DMA-aligned markers and lengths.
**  940927 WJB  Changed allocation functions from returning pointer to returning error code.
**  941005 WJB  Updated autodocs.
**  941007 WJB  Added caveats re sample frame size.
**  941007 WJB  Added PartialLen non-zero trap in ReadSoundSample().
**  941010 WJB  Moved cursor advancement code from ReadSoundMethods to ReadSoundData().
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <audio.h>

#include "soundplayer_internal.h"


/* -------------------- Class definition */

    /* preloaded sample class */
typedef struct SPSoundSample {
    SPSound spss;                       /* Base class */
    char   *spss_DataAddress;           /* Address of sound data (char * to facilitate address/offset computation) */
} SPSoundSample;

static Err ReadSoundSample (SPSoundSample *, uint32 cursorbyte, uint32 rembytes, char *bufaddr, uint32 bufsize, bool optimized, char **useaddrp, uint32 *uselenp, uint32 *nextpartiallenp);
static const SPSoundClassDefinition SoundSampleClass = {
    sizeof (SPSoundSample),
    NULL,
    (SPReadSoundMethod)ReadSoundSample,
};


/* -------------------- Create/Delete */

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spaddsample
 |||	spAddSample - Create an SPSound for a Sample Item.
 |||
 |||	  Synopsis
 |||
 |||	    Err spAddSample (SPSound **resultSound, SPPlayer *player, Item sample)
 |||
 |||	  Description
 |||
 |||	    Creates an SPSound for the specified Sample Item and adds it to the
 |||	    specified player. This is useful for playing back sounds that are
 |||	    already in memory.
 |||
 |||	    This function queries the sample item for its properties
 |||	    (e.g. number of channels, size of frame, number of frames,
 |||	    loop points, etc). The sound is checked for sample frame formatting compatibility
 |||	    with the other SPSounds in the SPPlayer and for buffer size compatibility.
 |||	    A mismatch causes an error to be returned.
 |||
 |||	    Once that is done, the following special SPMarkers are created for the new SPSound:
 |||
 |||	        SP_MARKER_NAME_BEGIN         - set to the beginning of the sample
 |||
 |||	        SP_MARKER_NAME_END           - set to the end of the sample
 |||
 |||	        SP_MARKER_NAME_SUSTAIN_BEGIN - set to the beginning of the sustain loop
 |||	                                       if the sample has a sustain loop.
 |||
 |||	        SP_MARKER_NAME_SUSTAIN_END   - set to the end of the sustain loop
 |||	                                       if the sample has a sustain loop.
 |||
 |||	        SP_MARKER_NAME_RELEASE_BEGIN - set to the beginning of the release loop
 |||	                                       if the sample has a release loop.
 |||
 |||	        SP_MARKER_NAME_RELEASE_END   - set to the end of the release loop
 |||	                                       if the sample has a release loop.
 |||
 |||	    Since a Sample Item has no provision for storing any markers other than
 |||	    the loop points, an SPSound created from a sample item returned by LoadSample()
 |||	    will not have any of the markers from the AIFF file other than the ones
 |||	    listed above.
 |||
 |||	    The length of the sample and all of its loop points must be DMA-aligned
 |||	    or else this function will return ML_ERR_BAD_SAMPLE_ALIGNMENT.
 |||
 |||	    All SPSounds added to an SPPlayer are automatically disposed of when
 |||	    the SPPlayer is deleted with spDeletePlayer() (by calling spRemoveSound()).
 |||	    You can manually dispose of an SPSound with spRemoveSound().
 |||
 |||	  Arguments
 |||
 |||	    resultSound                 Pointer to buffer to write resulting
 |||	                                SPSound pointer. Must be supplied or
 |||	                                or else this function returns ML_ERR_BADPTR.
 |||
 |||	    player                      Pointer to an SPPlayer.
 |||
 |||	    sample                      Item number of a sample to add.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Outputs
 |||
 |||	    A pointer to an allocated SPSound is written to the buffer
 |||	    pointed to by resultSound on success. NULL is written to this
 |||	    buffer on failure.
 |||
 |||	  Notes
 |||
 |||	    Since all SPSounds belonging to an SPPlayer are played by the same
 |||	    sample player instrument, they must all have the same frame sample frame
 |||	    characteristics (width, number of channels, compression type, and compression ratio).
 |||
 |||	    SPSound to SPSound cross verification is done: an error is returned
 |||	    if they don't match. However, there is no way to verify the correctness of sample frame
 |||	    characteristics for the instrument supplied to spCreatePlayer().
 |||
 |||	  Caveats
 |||
 |||	    There is no restriction on adding Sample Item class SPSounds while the SPPlayer
 |||	    is playing, but removing them while playing can be dangerous. See the Caveats
 |||	    section of spRemoveSound() for more details on this.
 |||
 |||	    Note that this only applies to Sample Item class SPSounds (the kind made with this function).
 |||	    There is no restriction on adding _or_ removing AIFF Sound File class
 |||	    SPSounds while playing.
 |||
 |||	    The sound player will not work correctly unless the sample frame
 |||	    size for the sample follows these rules:
 |||
 |||	    If frame size (in bytes) < 4, then it must divide into 4 evenly.
 |||
 |||	    If frame size (in bytes) > 4, then it must must be a multiple of 4.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V24.
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spRemoveSound(), spAddSoundFile()
 |||
 **/
Err spAddSample (SPSound **resultSound, SPPlayer *player, Item sample)
{
    SPSoundSample *sound = NULL;
    uint32 numframes, width, channels, compressionratio;
    int32 sustainbegin, sustainend, releasebegin, releaseend;
    char *dataaddress;
    Err errcode;

  #if DEBUG_Create
    printf ("spAddSample() item=%ld\n", sample);
  #endif

        /* initialize result (must be done first) */
    if (!resultSound) return ML_ERR_BADPTR;
    *resultSound = NULL;

        /* interrogate sample */
    {
        TagArg tags[] = {
            { AF_TAG_FRAMES },
            { AF_TAG_CHANNELS },
            { AF_TAG_WIDTH },
            { AF_TAG_COMPRESSIONRATIO },
            { AF_TAG_ADDRESS },
            { AF_TAG_SUSTAINBEGIN },
            { AF_TAG_SUSTAINEND },
            { AF_TAG_RELEASEBEGIN },
            { AF_TAG_RELEASEEND },
            TAG_END
        };

        if ((errcode = GetAudioItemInfo (sample, tags)) < 0) goto clean;

        numframes        = (uint32)tags[0].ta_Arg;
        channels         = (uint32)tags[1].ta_Arg;
        width            = (uint32)tags[2].ta_Arg;
        compressionratio = (uint32)tags[3].ta_Arg;
        dataaddress      = (char *)tags[4].ta_Arg;
        sustainbegin     = (int32) tags[5].ta_Arg;
        sustainend       = (int32) tags[6].ta_Arg;
        releasebegin     = (int32) tags[7].ta_Arg;
        releaseend       = (int32) tags[8].ta_Arg;
    }

        /* alloc/init SPSound */
    if ((errcode = spAllocSound ((SPSound **)&sound, player, &SoundSampleClass, numframes, width, channels, compressionratio)) < 0) goto clean;
    sound->spss_DataAddress = dataaddress;

        /* add loop markers from sample */
    if (sustainbegin >= 0 && (errcode = spAddMarker ((SPSound *)sound, sustainbegin, SP_MARKER_NAME_SUSTAIN_BEGIN)) < 0) goto clean;
    if (sustainend   >= 0 && (errcode = spAddMarker ((SPSound *)sound, sustainend,   SP_MARKER_NAME_SUSTAIN_END))   < 0) goto clean;
    if (releasebegin >= 0 && (errcode = spAddMarker ((SPSound *)sound, releasebegin, SP_MARKER_NAME_RELEASE_BEGIN)) < 0) goto clean;
    if (releaseend   >= 0 && (errcode = spAddMarker ((SPSound *)sound, releaseend,   SP_MARKER_NAME_RELEASE_END))   < 0) goto clean;

  #if DEBUG_Create
    printf ("  address=$%08lx\n", sound->spss_DataAddress);
  #endif

        /* success: add to player's sounds list and set result */
    spAddSound ((SPSound *)sound);
    *resultSound = (SPSound *)sound;

    return 0;

clean:
    spFreeSound ((SPSound *)sound);
    return errcode;
}


/* -------------------- Read */

/*
    SoundSampleClass read method
*/
static Err ReadSoundSample (SPSoundSample *sound, uint32 cursorbyte, uint32 rembytes, char *bufaddr, uint32 bufsize, bool optimized, char **useaddrp, uint32 *uselenp, uint32 *nextpartiallenp)
{
    SPPlayer * const player = sound->spss.spso_Player;
    char * const sampleaddr = sound->spss_DataAddress + cursorbyte;
    char *useaddr;
    uint32 uselen;

        /* Trap non-zero PartialFrameLen - should never get here with that being non-zero. */
    if (player->sp_PartialFrameLen) return ML_ERR_CORRUPT_DATA;

        /* determine how much of sample to use */
    uselen = MIN (rembytes, bufsize);
    uselen -= uselen % player->sp_SampleFrameInfo.spfi_AlignmentBytes;      /* DMA/frame align length */

        /* read data into buffer */
    if (optimized) {        /* optimized: point to sample directly */
        useaddr = sampleaddr;
    }
    else {                  /* non-optimized: copy from sample to buffer */
        useaddr = (char *)memcpy (bufaddr, sampleaddr, uselen);
    }

        /* set resulting useaddr, uselen, and nextpartiallen */
    *useaddrp        = useaddr;
    *uselenp         = uselen;
    *nextpartiallenp = 0;

    return 0;
}

