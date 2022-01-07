/******************************************************************************
**
**  $Id: soundplayer.c,v 1.73 1994/10/17 23:10:35 peabody Exp $
**
**  Advanced Sound Player.
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
**  940815 WJB  Privatized data structures.
**              Added spGetPlayerSignals().
**  940818 WJB  Replaced marker index system with marker name system.
**  940818 WJB  Made SP_MARKERNAME_ constants refer to const char arrays instead of string literals.
**  940824 WJB  Added first attempt at short spooler buffer handling.
**  940826 WJB  Added default minimum buffer size trap in spAllocSound().
**  940826 WJB  Completed short spooler buffer handling code.
**  940901 WJB  Removed temp define for memmove().
**  940902 WJB  Changed spCreatePlayer() syntax.
**  940902 WJB  Tweaked spDeletePlayer() to better deal w/ partial success of spCreatePlayer().
**  940907 WJB  Hooked up ssplUnrequestBuffer().
**  940913 WJB  Removed VAGLUE_ stuff and redundant list functions.
**  940915 WJB  Replaced local code with call to FindNamedNode().
**  940916 WJB  Added custom buffer support.
**  940919 WJB  Privatized some autodocs headers.
**  940920 WJB  Added autodocs.
**  940920 WJB  Removed redundant player arg from spStartReading().
**  940922 WJB  Removed redundant non-NULL tests before calling FreeMem().
**  940922 WJB  Added error handler for non-DMA-aligned markers and lengths.
**  940922 WJB  Added protection to prevent spRemoveMarker() from deleting begin and end markers.
**  940927 WJB  Changed allocation functions from returning pointer to returning error code.
**  940927 WJB  Added a bit more debug.
**  940928 WJB  Renamed spGetPlayerSignals() to spGetPlayerSignalMask().
**  940928 WJB  Removed const from function args that are conceptually non-const.
**  940928 WJB  Renamed typedefs to use uppercase SP.
**  940929 WJB  Added spGetPlayerFromSound() and spGetSoundFromMarker().
**  941003 WJB  Replaced SPAction system with a private structure.
**  941003 WJB  Updated docs.
**  941003 WJB  Added spGetMarkerPosition() and spGetMarkerName().
**  941003 WJB  Replaced spGetPlayerState() w/ spGetPlayerStatus().
**  941005 WJB  Added usage of ssplGetSpoolerStatus() to spGetPlayerStatus().
**  941005 WJB  Updated autodocs.
**  941007 WJB  Added caveats re sample frame size.
**  941010 WJB  Moved cursor advancement code from ReadSoundMethods to ReadSoundData().
**  941014 WJB  Added trap in spCreatePlayer() to fail on numbufs < 2.
**  941017 WJB  Added cross-SPPlayer traps to prevent branching from one SPPlayer's objects to another.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <audio.h>
#include <list.h>
#include <mem.h>
#include <soundspooler.h>
#include <string.h>
#include <varargs_glue.h>       /* VAGLUE_ stuff */
/* #include <stddef.h> */       /* offsetof() */ /* @@@ stddef.h doesn't exist, offsetof() currently found in types.h */

#include "music_internal.h"     /* package ids */
#include "soundplayer_internal.h"


/* -------------------- List support */

    /* process nodes in list (including removing them)
       notes: only evaluates NextNode() when IsNode() is true */
#define PROCESSLIST(l,n,s,t)                                \
    for ( n = (t *)FirstNode(l);                            \
          IsNode(l,n) && ((s = (t *)NextNode(n)), TRUE);    \
          n = s )


/* -------------------- Package ID */

DEFINE_MUSICLIB_PACKAGE_ID(soundplayer)


/* -------------------- Options */

#define OPTION_CoerceBadSampleAlignment 0   /* truncate misaligned lengths and marker positions */


/* -------------------- Special marker names */

const char sp_markerNameBegin[]        = "<Begin>";
const char sp_markerNameEnd[]          = "<End>";
const char sp_markerNameSustainBegin[] = "<SustainBegin>";
const char sp_markerNameSustainEnd[]   = "<SustainEnd>";
const char sp_markerNameReleaseBegin[] = "<ReleaseBegin>";
const char sp_markerNameReleaseEnd[]   = "<ReleaseEnd>";


/* -------------------- Local functions */

    /* Add/Remove markers */
static Err spInternalAddMarker (SPSound *, uint32 position, const char *markername, bool permanent);
#define spAddPermanentMarker(sound,position,markername) spInternalAddMarker ((sound), (position), (markername), TRUE)
static Err spInternalRemoveMarker (SPMarker *);

    /* macros to identify a marker type (@@@ these can do multiple evaluations) */
#define IsBeginningMarker(marker) (!(marker)->spmk_Position)
#define IsEndMarker(marker)       ((marker)->spmk_Position == (marker)->spmk_Sound->spso_NumFrames)

    /* marker action */
static void SetMarkerBranch (SPMarker *frommarker, SPMarker *tomarker);
#define ClearMarkerBranch(marker) SetMarkerBranch ((marker), NULL)
#define SetDefaultMarkerBranch(marker) SetMarkerBranch ((marker), (marker))

    /* FillSpooler() */
static Err FillSpooler (SPPlayer *);

    /* marker action processing */
static Err PerformMarkerAction (SPPlayer *, SPMarker *marker);

    /* SPAction support functions */
static void InitAction (SPAction *, SPPlayer *);
static void ClearAction (SPAction *);
static void SetBranchAction (SPAction *, SPMarker *tomarker);
#define SetStopAction(action) SetBranchAction (action, NULL)

    /* SetReaderStatus() */
static void SetReaderStatus (SPPlayer *, const SPMarker *);

    /* marker lookup */
static SPMarker *FindMarkerAfter (const SPSound *, uint32 position);
static Err ValidateFromMarker (const SPMarker *frommarker);
static Err ValidateToMarker (const SPMarker *tomarker);
#define GetMarkerFromBranchRefNode(refnode) (SPMarker *)((char *)(refnode) - offsetof(SPMarker,spmk_BranchRefNode))

    /* SampleFrameInfo stuff */
static void SetSampleFrameInfo (SPSampleFrameInfo *, uint32 width, uint32 channels, uint32 compressionratio);
static int CompareSampleFrameInfo (const SPSampleFrameInfo *, const SPSampleFrameInfo *);
static Err CheckSampleFrameAlignment (const SPSampleFrameInfo *, uint32 framenum);
#define AlignFrame(frameinfo,framenum)  ((framenum) - (framenum) % (frameinfo)->spfi_AlignmentFrames)

    /* local node management */
static Node *AllocNamedNode (size_t structsize, const char *name);
static void FreeNamedNode (Node *);


/* -------------------- Player create, delete, set attributes */

    /* Player size includes a table of buffer pointers allocated after end of structure */
#define spGetPlayerSize(numbufs) (sizeof (SPPlayer) + (numbufs) * sizeof (void *))

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spcreateplayer
 |||	spCreatePlayer - Create an SPPlayer.
 |||
 |||	  Synopsis
 |||
 |||	    Err spCreatePlayer (SPPlayer **resultPlayer,
 |||	                        Item samplerInstrument,
 |||	                        uint32 numBuffers, uint32 bufferSize,
 |||	                        void * const customBuffers[])
 |||
 |||	  Description
 |||
 |||	    Creates an SPPlayer (the top-level player context). The player needs:
 |||	        . a sample player DSP instrument to play the sound.
 |||	        . a set of identically-sized audio buffers used to spool sound off
 |||	          of disc.
 |||
 |||	    The client must supply the sample player instrument. Buffers can be
 |||	    allocated automatically or supplied by the client.
 |||
 |||	    At least 2 buffers are required in order to permit smooth sound
 |||	    playback. Since each buffer will have a signal allocated for it, there
 |||	    can be no more than 23 buffers (the number of free signals for a task
 |||	    that has no signals allocated). 4 or 5 is probably a comfortable
 |||	    number of buffers.
 |||
 |||	    Buffer size must be at least 512 bytes for in-memory sounds or 2 *
 |||	    blocksize + 514 for sounds off disc. Typically much larger buffers are
 |||	    required to smoothly playback sounds directly off disk (e.g. 16K or
 |||	    so for 44100 frame/sec playback of a 16-bit monophonic sound).
 |||
 |||	    For disc-based sounds, the buffer size, truncated to a multiple of
 |||	    block size, represents the maximum amount of data that can be read
 |||	    from the disc in a single I/O request. Given the nature of data being
 |||	    read from a CD, the larger the buffer, the more efficiently the player
 |||	    will read data off of disc. In order to guarantee smooth sound
 |||	    playback, there must be enough data spooled at all times to cover the
 |||	    read of 1 buffer. Obviously, this requires that the data can be read
 |||	    faster than it can be played.
 |||
 |||	    The total size of all of the buffers represents the maximum latency
 |||	    between reading and the actual sound output (the responsiveness to
 |||	    branches and decisions). The maximum latency can be computed as
 |||	    follows:
 |||
 |||	        max latency (sec) = numBuffers * bufferSize / frame size / sample rate
 |||
 |||	    For example:
 |||
 |||	        numBuffers = 4
 |||	        bufferSize = 16384 bytes
 |||	        frame size = 2 (16-bit monophonic)
 |||	        sample rate = 44100 frames / sec
 |||
 |||	        max latency = 4 * 16384 / 2 / 44100 = 0.743 sec
 |||
 |||	    With the above buffer configuration and playback rate, the results of
 |||	    branches and decision functions will be heard no later than about 3/4
 |||	    second afterwards.
 |||
 |||	    The client is left with the problem of striking a good balance between
 |||	    efficient I/O usage and responsiveness for any specific application.
 |||
 |||	    Use spDeletePlayer() to dispose of this SPPlayer when you are done
 |||	    with it.
 |||
 |||	  Arguments
 |||
 |||	    resultPlayer                Pointer to buffer to write resulting
 |||	                                SPPlayer pointer. Must be supplied or
 |||	                                or else this function returns ML_ERR_BADPTR.
 |||
 |||	    samplerInstrument           Item number of sample player instrument
 |||	                                to be used to play back sounds with this
 |||	                                SPPlayer. Note that all sounds added
 |||	                                to the player must be of a format that
 |||	                                this sampler instrument can play. The
 |||	                                player has no way to verify correctness of
 |||	                                instrument, however.
 |||
 |||	    numBuffers                  Number of buffers to use. Valid range is
 |||	                                2..23 (limited by available signals).
 |||
 |||	    bufferSize                  Size of each buffer in bytes. For in-memory
 |||	                                sounds, must be at least 512 bytes. For
 |||	                                sounds to spool directly off disc, must be
 |||	                                at least 2*blocksize+514. In practice larger
 |||	                                buffers for disc-based sounds are probably
 |||	                                required in order to provide smooth sound
 |||	                                playback.
 |||
 |||	    customBuffers               Table of pointers to client-supplied
 |||	                                buffers. If NULL is passed in for this
 |||	                                argument, the player will allocate memory
 |||	                                for the specified buffers.
 |||
 |||	                                Use this if you want to allocate
 |||	                                your own buffers instead of letting the
 |||	                                player do it. The table must contain
 |||	                                numBuffers pointers to unique buffers that
 |||	                                must be bufferSize in length. Each buffer
 |||	                                must be of MEMTYPE_AUDIO and have a
 |||	                                DMA-aligned starting address.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Outputs
 |||
 |||	    A pointer to an allocated SPPlayer is written to the buffer
 |||	    pointed to by resultPlayer on success. NULL is written to this
 |||	    buffer on failure.
 |||
 |||	  Caveats
 |||
 |||	    The sound player allocates one signal from your task for each buffer.
 |||	    Keep this in mind when deciding how many buffers to use.
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
 |||	    spDeletePlayer(), spAddSoundFile(), spAddSample(), spStartReading(),
 |||	    spStartPlaying(), spStop()
 |||
 **/
Err spCreatePlayer (SPPlayer **resultPlayer, Item samplerins, uint32 numbufs, uint32 bufsize, void * const custombuffers[])
{
    SPPlayer *player = NULL;
    Err errcode;

    PULL_MUSICLIB_PACKAGE_ID(soundplayer);

  #if DEBUG_Create
    printf ("spCreatePlayer() samplerins=%ld numbufs=%ld bufsize=%ld\n", samplerins, numbufs, bufsize);
    if (custombuffers) {
        int i;

        printf ("  custombuffers:\n");
        for (i=0; i<numbufs; i++)
            printf ("    %d: $%08lx\n", i, custombuffers[i]);
    }
  #endif

        /* initialize result (must be done first) */
    if (!resultPlayer) return ML_ERR_BADPTR;
    *resultPlayer = NULL;

        /* validate args */
        /* @@@ spooler can be created w/ a NULL instrument and later set by ssplAttachInstrument().
               I need this test here until there's a facility to do this to the player's spooler */
    if (!samplerins) {
        errcode = ML_ERR_BAD_ARG;
        goto clean;
    }
    if (numbufs < 2) {
        errcode = ML_ERR_BAD_ARG;
        goto clean;
    }
    if (bufsize < SP_SPOOLER_MIN_LENGTH) {
        errcode = ML_ERR_BUFFER_TOO_SMALL;
        goto clean;
    }

        /* Allocate Player and BufferArray (an array of pointers only, at end of player) */
    if ((player = (SPPlayer *)AllocMem (spGetPlayerSize(numbufs), MEMTYPE_ANY | MEMTYPE_FILL)) == NULL) {
        errcode = ML_ERR_NOMEM;
        goto clean;
    }
    InitList (&player->sp_Sounds, "Sounds");
    player->sp_BufferArray = (void **)(player + 1);
    player->sp_NumBuffers  = (uint8)numbufs;
    player->sp_BufferSize  = bufsize;

        /* create spooler */
    if ((player->sp_Spooler = ssplCreateSoundSpooler (numbufs, samplerins)) == NULL) {
        errcode = ML_ERR_NOMEM;     /* @@@ not entirely right, but sspl doesn't have a real error code return facility */
        goto clean;
    }

        /* init sp_BufferArray */
    if (custombuffers) {            /* copy client's buffer array to sp_BufferArray */
        int i;

            /* validate custom buffer pointers */
        for (i=0; i<player->sp_NumBuffers; i++) {
            const void * const addr = custombuffers[i];

                /* trap invalid pointer */
            if (!addr) {
                errcode = ML_ERR_BAD_SAMPLE_ADDRESS;
                goto clean;
            }

                /* trap bad alignment */
                /* @@@ this could become a convenience function */
            if ((uint32)addr & ~SP_DMA_MASK) {
                errcode = ML_ERR_BAD_SAMPLE_ALIGNMENT;
                goto clean;
            }
        }

            /* copy client-supplied buffer pointers to our buffer table */
        memcpy (player->sp_BufferArray, custombuffers, numbufs * sizeof (void *));
    }
    else {                          /* allocate buffers */
        int i;

        player->sp_BufferFlags |= SP_BUFFERF_ALLOCATED;

        for (i=0; i<player->sp_NumBuffers; i++) {
            if ((player->sp_BufferArray[i] = AllocMem (player->sp_BufferSize, MEMTYPE_AUDIO)) == NULL) {
                errcode = ML_ERR_NOMEM;
                goto clean;
            }
        }
    }

        /* success: set result */
    *resultPlayer = player;
    return 0;

clean:
    spDeletePlayer (player);
    return errcode;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spdeleteplayer
 |||	spDeletePlayer - Delete an SPPlayer.
 |||
 |||	  Synopsis
 |||
 |||	    Err spDeletePlayer (SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Deletes an SPPlayer created by spCreatePlayer(). Automatically stops
 |||	    the SPPlayer and calls spRemoveSound() for all SPSounds belonging to the
 |||	    SPPlayer. If the spCreatePlayer() allocated buffers, those buffers are
 |||	    automatically freed. Custom allocated buffers passed in to spCreatePlayer()
 |||	    are not deleted.
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to an SPPlayer to delete. Can be
 |||	                                NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
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
 |||	    spCreatePlayer(), spRemoveSound(), spRemoveMarker()
 |||
 **/
Err spDeletePlayer (SPPlayer *player)
{
  #if DEBUG_Create
    printf ("spDeletePlayer ($%08lx)\n", player);
  #endif

    if (player) {
            /*
                Stop player, delete spooler.
                Do this first since we're about to delete memory that the spooler might point to.
            */
        if (player->sp_Spooler) {
            spStop (player);
            ssplDeleteSoundSpooler (player->sp_Spooler);
        }

            /* remove sounds */
        {
            SPSound *sound, *next;

            PROCESSLIST (&player->sp_Sounds,sound,next,SPSound) {
                spRemoveSound (sound);
            }
        }

            /* free buffers if we allocated them */
        if (player->sp_BufferFlags & SP_BUFFERF_ALLOCATED) {
            int i;

            for (i=0; i<player->sp_NumBuffers; i++) {
                FreeMem (player->sp_BufferArray[i], player->sp_BufferSize);
            }
        }

            /* free player */
        FreeMem (player, spGetPlayerSize ((uint32)player->sp_NumBuffers));
    }

    return 0;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spsetdefaultdecisionfunction
 |||	spSetDefaultDecisionFunction - Install a global decision function to be
 |||	                               called for every marker.
 |||
 |||	  Synopsis
 |||
 |||	    Err spSetDefaultDecisionFunction (SPPlayer *player,
 |||	                                      SPDecisionFunction decisionFunc,
 |||	                                      void *decisionData)
 |||
 |||	  Description
 |||
 |||	    Install a global decision function that is to be called when the player
 |||	    reaches each marker. This can be used as a way to do some common
 |||	    operation at multiple locations during playback (e.g. processing a
 |||	    script).
 |||
 |||	    Clear the global decision function with
 |||	    spClearDefaultDecisionFunction().
 |||
 |||	  Arguments
 |||
 |||	    player                      SPPlayer to which to install default
 |||	                                decision function.
 |||
 |||	    decisionFunc                An SPDecisionFunction to install, or NULL
 |||	                                to clear.
 |||
 |||	    decisionData                A pointer to client-supplied data to be
 |||	                                passed to decisionFunc when called. Can be
 |||	                                NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Notes
 |||
 |||	    The player normally optimizes reading data from disc by
 |||	    ignoring markers that have neither a branch destination nor a marker
 |||	    decision function. When a default decision function is installed, the
 |||	    player must interrupt reading data from disc in preperation for a
 |||	    potential branch at EVERY marker, even if the default decision
 |||	    function does nothing. This can severely impact
 |||	    performance depending on the placement of markers.
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
 |||	    spCleateDefaultDecisionFunction(), spSetMarkerDecisionFunction(),
 |||	    SPDecisionFunction
 |||
 **/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spcleardefaultdecisionfunction
 |||	spClearDefaultDecisionFunction - Clears global decision function.
 |||
 |||	  Synopsis
 |||
 |||	    Err spClearDefaultDecisionFunction (SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Removes a global decision function installed by
 |||	    spSetDefaultDecisionFunction().
 |||
 |||	  Arguments
 |||
 |||	    player                      SPPlayer from which to remove default
 |||	                                decision function.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in soundplayer.h V24.
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spSetDefaultDecisionFunction()
 |||
 **/
Err spSetDefaultDecisionFunction (SPPlayer *player, SPDecisionFunction decisionfunc, void *decisiondata)
{
    player->sp_DefaultDecisionFunction = decisionfunc;
    player->sp_DefaultDecisionData     = decisiondata;

    return 0;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spgetplayersignalmask
 |||	spGetPlayerSignalMask - Get set of signals player will send to client.
 |||
 |||	  Synopsis
 |||
 |||	    int32 spGetPlayerSignalMask (const SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Returns the set of signals that the player will send to the client.
 |||	    The client must wait on this signal set between calls to spService().
 |||	    This signal set is constant for the life of the SPPlayer. This
 |||	    function cannot fail.
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to SPPlayer.
 |||
 |||	  Return Value
 |||
 |||	    Set of signals suitable for passing to WaitSignal().
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
 |||	    spCreatePlayer(), spService()
 |||
 **/
int32 spGetPlayerSignalMask (const SPPlayer *player)
{
    return player->sp_Spooler->sspl_SignalMask;
}


/* -------------------- Sound Add, Remove */

/*
    SPSound derived class allocation support
*/
Err spAllocSound (SPSound **resultSound, SPPlayer *player, const SPSoundClassDefinition *classdef, uint32 numframes, uint32 width, uint32 channels, uint32 compressionratio)
{
    SPSound *sound = NULL;
    Err errcode;

  #if DEBUG_Create
    printf ("spAllocSound() structsize=%lu numframes=%lu width=%lu channels=%lu compression=%lu\n", classdef->spsc_StructSize, numframes, width, channels, compressionratio);
  #endif

        /* initialize result (must be done first) */
    if (!resultSound) return ML_ERR_BADPTR;
    *resultSound = NULL;

        /* allocate/init SPSound */
    if ((sound = (SPSound *)AllocMem (classdef->spsc_StructSize, MEMTYPE_ANY | MEMTYPE_FILL)) == NULL) {
        errcode = ML_ERR_NOMEM;
        goto clean;
    }
    sound->spso_Player          = player;
    sound->spso_ClassDefinition = classdef;
    sound->spso_NumFrames       = numframes;
    SetSampleFrameInfo (&sound->spso_SampleFrameInfo, width, channels, compressionratio);
    InitList (&sound->spso_Markers, "Markers");

        /* make sure sound is compatible with other sounds in player */
        /* (depends on spso_SampleFrameInfo) */
    if (!IsEmptyList (&player->sp_Sounds) && CompareSampleFrameInfo (&player->sp_SampleFrameInfo, &sound->spso_SampleFrameInfo)) {
        errcode = ML_ERR_INCOMPATIBLE_SOUND;
        goto clean;
    }

        /* make sure player's buffers are big enough to hold minimum amount of data from sound */
        /* (depends on spso_SampleFrameInfo) */
    if (player->sp_BufferSize < spMinBufferSize (&sound->spso_SampleFrameInfo)) {
        errcode = ML_ERR_BUFFER_TOO_SMALL;
        goto clean;
    }

    /* note: not trapping bogus CompressionRatio or FrameSize here because such things will probably
       cause obviously wrong results and never actually happen in the field */

        /* trap misaligned end */
    if ((errcode = CheckSampleFrameAlignment (&sound->spso_SampleFrameInfo, sound->spso_NumFrames)) < 0) {
      #if DEBUG_SampleAlignment
        printf ("spAllocSound(): length of %lu frames is not DMA aligned.\n", sound->spso_NumFrames);
      #endif

      #if OPTION_CoerceBadSampleAlignment
        {
            const uint32 alignedlen = AlignFrame (&sound->spso_SampleFrameInfo, sound->spso_NumFrames);

          #if DEBUG_SampleAlignment
            printf ("spAllocSound(): Warning: length truncated to %lu.\n", alignedlen);
          #endif
            sound->spso_NumFrames = alignedlen;
        }
      #else /* OPTION_CoerceBadSampleAlignment */
        goto clean;
      #endif
    }

        /* add beginning and end default markers */
    if ((errcode = spAddPermanentMarker (sound, 0,                     SP_MARKER_NAME_BEGIN)) < 0) goto clean;
    if ((errcode = spAddPermanentMarker (sound, sound->spso_NumFrames, SP_MARKER_NAME_END)) < 0) goto clean;

        /* success: set result */
    *resultSound = sound;
    return 0;

clean:
    spFreeSound (sound);
    return errcode;
}

/*
    On successful creation of a derived sound class object, add it to player's sound list
*/
void spAddSound (SPSound *sound)
{
    SPPlayer * const player = sound->spso_Player;

        /* if this is the first sound, set the player's SampleFrameInfo */
    if (IsEmptyList (&player->sp_Sounds)) {
        player->sp_SampleFrameInfo = sound->spso_SampleFrameInfo;
    }

        /* add to player's sound list */
    AddTail (&player->sp_Sounds, (Node *)sound);
}


 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spremovesound
 |||	spRemoveSound - Manually remove an SPSound from an SPPlayer.
 |||
 |||	  Synopsis
 |||
 |||	    Err spRemoveSound (SPSound *sound)
 |||
 |||	  Description
 |||
 |||	    Removes and frees the specified sound from the SPPlayer that it
 |||	    belongs to. All markers belonging to this sound including any
 |||	    the client may have added, are automatically freed by this function.
 |||
 |||	    All sounds belonging to an SPPlayer are
 |||	    automatically disposed of when that SPPlayer is disposed of with
 |||	    spDeletePlayer(). Use this function if you want to remove a sound
 |||	    manually.
 |||
 |||	    For sound file class SPSounds, the file opened by spAddSoundFile()
 |||	    is closed. The sample item passed into spAddSample() is left behind
 |||	    after this spRemoveSound() for the client to clean up.
 |||
 |||	    An SPSound cannot be removed from its owning SPPlayer while it is being
 |||	    read from while the player is running. spIsSoundInUse() can be used to
 |||	    determine if the sound is being read. If this function is called for an
 |||	    SPSound for which spIsSoundInUse() returns TRUE, the SPPlayer is
 |||	    stopped by calling spStop().
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to an SPSound to remove. Can be
 |||	                                NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Caveats
 |||
 |||	    This function only stops
 |||	    the SPPlayer when while the SPPlayer is still reading from the SPSound
 |||	    being removed, which is fine for AIFF Sound File class SPSounds.
 |||	    For Sample Item class SPSounds, the player passes pointers
 |||	    to the Sample Item's sound data in memory the SoundSpooler rather than
 |||	    copying the sound data to the SPPlayer's buffers. That means that the
 |||	    sound data must remaing valid until the SoundSpooler has completely
 |||	    played the sound data, which happens some amount of time after the SPPlayer
 |||	    is done 'reading' that data.
 |||
 |||	    If you remove a Sample Item class SPSound during that window of vulnerability
 |||	    and then delete the data for the Sample Item (e.g. UnloadSample()),
 |||	    the SoundSpooler is then pointing to freed memory, whose contents
 |||	    may be clobbered by another task at any time. Unfortunately
 |||	    you may not hear bad results if you do this inadvertently because
 |||	    there's no guarantee that freed memory _will_ get trashed (unless you
 |||	    are running a deamon to munge newly freed memory).
 |||
 |||	    So, avoid removing Sample Item class SPSounds while the SPPlayer is playing.
 |||	    Or at least go to measures to insure that the Sample Item class SPSound you
 |||	    are removing is not in the spooler (e.g. it has never been played, or at least
 |||	    not since the last spStop()).
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
 |||	    spAddSample(), spAddSoundFile(), spRemoveMarker(), spDeletePlayer(),
 |||	    spIsSoundInUse()
 |||
 **/
Err spRemoveSound (SPSound *sound)
{
  #if DEBUG_Create
    printf ("spRemoveSound ($%08lx)\n", sound);
  #endif

    if (sound) {
            /* stop player if sound is in use */
        if (spIsSoundInUse (sound)) spStop (sound->spso_Player);

            /* remove and free sound */
        RemNode ((Node *)sound);
        spFreeSound (sound);
    }

    return 0;
}

/*
    Free an SPSound (and it's derived class)
    Assumes it's already removed from player's Sounds list and that it is not in use.
    Tolerates NULL sound pointer.
*/
void spFreeSound (SPSound *sound)
{
    if (sound) {
      #if DEBUG_Create
        printf ("spFreeSound() $%08lx len=%lu\n", sound, sound->spso_NumFrames);
      #endif

            /* call class-specific delete method */
        if (sound->spso_ClassDefinition->spsc_Delete) sound->spso_ClassDefinition->spsc_Delete (sound);

            /* remove markers */
        {
            SPMarker *marker, *next;

            PROCESSLIST (&sound->spso_Markers,marker,next,SPMarker) {
                spInternalRemoveMarker (marker);
            }
        }

            /* free memory */
        FreeMem (sound, sound->spso_ClassDefinition->spsc_StructSize);
    }
}


 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spgetplayerfromsound
 |||	spGetPlayerFromSound - Get SPPlayer that owns an SPSound.
 |||
 |||	  Synopsis
 |||
 |||	    SPPlayer *spGetPlayerFromSound (const SPSound *sound)
 |||
 |||	  Description
 |||
 |||	    Returns a pointer to the SPPlayer to which the specified SPSound
 |||	    was added.
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to SPSound to interrogate.
 |||
 |||	  Return Value
 |||
 |||	    Pointer to SPPlayer that owns this SPSound.
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
 |||	    spAddSoundFile(), spAddSample()
 |||
 **/
SPPlayer *spGetPlayerFromSound (const SPSound *sound)
{
    return sound->spso_Player;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spissoundinuse
 |||	spIsSoundInUse - Determines if SPPlayer is currently reading from a
 |||	                 particular SPSound.
 |||
 |||	  Synopsis
 |||
 |||	    bool spIsSoundInUse (const SPSound *sound)
 |||
 |||	  Description
 |||
 |||	    Determines if SPPlayer is currently reading from a particular SPSound.
 |||	    An SPSound cannot be removed from its owning SPPlayer while it is being
 |||	    read from while the player is running. This function is necessary for
 |||	    a client to know when it is safe to remove an SPSound.
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to SPSound to test.
 |||
 |||	  Return Value
 |||
 |||	    TRUE if sound is currently being read by player; FALSE otherwise.
 |||
 |||	  Caveats
 |||
 |||	    Returns FALSE too early for Sample Item class SPSounds. Since the
 |||	    spooler is given a pointer to the Sample Item's memory directly,
 |||	    rather than copying it into the spooler buffers, it mustn't return
 |||	    FALSE until a Sample Item SPSound has actually finished playing, not
 |||	    finished being read.
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
 |||	    spRemoveSound(), spStop()
 |||
 **/
bool spIsSoundInUse (const SPSound *sound)
{
    /*
        !!! This needs to detect if in-memory sound is
            currently being pointed to by a SoundBufferNode.
            (see Caveats above)
    */

    return (bool) (sound->spso_Player->sp_CurrentSound == sound);
}


/* -------------------- Marker Add, Remove, Set Attributes */

static void FreeMarker (SPMarker *);
static bool CmpMarkers (const SPMarker *newmarker, const SPMarker *cmpmarker);

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spaddmarker
 |||	spAddMarker - Add a new SPMarker to an SPSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err spAddMarker (SPSound *sound, uint32 position,
 |||	                     const char *markerName)
 |||
 |||	  Description
 |||
 |||	    Adds a new SPMarker to the specified SPSound.
 |||
 |||	    All markers added to an SPSound are automatically freed when the
 |||	    SPSounds is freed by spRemoveSound(). A marker may be manually
 |||	    freed by calling spRemoveMarker().
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to an SPSound to which to add an
 |||	                                SPMarker.
 |||
 |||	    position                    Position (in frames) within the sound data
 |||	                                for the new marker. The range is
 |||	                                0..nframes, where nframes is the length of
 |||	                                the sound in frames. 0 places the new
 |||	                                marker before the first frame in the
 |||	                                sound; nframes places the marker after the
 |||	                                last frame in the sound. This position must
 |||	                                be DMA-aligned, or else this function
 |||	                                returns ML_ERR_BAD_SAMPLE_ALIGNMENT.
 |||
 |||	    markerName                  Name for the new marker. This name must be
 |||	                                unique for the sound. Names are compared
 |||	                                case-insensitively. The name passed in to
 |||	                                this function is copied, so the caller need not
 |||	                                keep the string buffer pointed to by markerName
 |||	                                intact after calling this function.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
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
 |||	    spRemoveMarker(), spBranchAtMarker(), spStopAtMarker(),
 |||	    spContinueAtMarker(), spSetMarkerDecisionFunction(),
 |||	    spClearMarkerDecisionFunction()
 |||
 **/
Err spAddMarker (SPSound *sound, uint32 position, const char *markername)
{
    return spInternalAddMarker (sound, position, markername, FALSE);
}

/*
    Internal add marker.
    If permanent is set, creates a marker w/ SP_MARKERF_PERMANENT set.
    Otherwise, SP_MARKERF_PERMANENT is not set.
*/
static Err spInternalAddMarker (SPSound *sound, uint32 position, const char *markername, bool permanent)
{
    SPMarker *marker = NULL;
    Err errcode;

  #if DEBUG_Create
    printf ("spAddMarker($%08lx,%ld,'%s',%ld)\n", sound, position, markername, permanent);
  #endif

        /* validate arguments */
        /* make sure position is within the length of the sound */
    if (position > sound->spso_NumFrames) {
        errcode = ML_ERR_OUT_OF_RANGE;
        goto clean;
    }

        /* trap misaligned marker */
    if ((errcode = CheckSampleFrameAlignment (&sound->spso_SampleFrameInfo, position)) < 0) {
      #if DEBUG_SampleAlignment
        printf ("spAddMarker(): marker at frame %lu is not DMA aligned.\n", position);
      #endif

      #if OPTION_CoerceBadSampleAlignment
        {
            const uint32 alignedpos = AlignFrame (&sound->spso_SampleFrameInfo, position);

          #if DEBUG_SampleAlignment
            printf ("spAddMarker(): Warning: marker moved to %lu.\n", alignedpos);
          #endif
            position = alignedpos;
        }
      #else /* OPTION_CoerceBadSampleAlignment */
        goto clean;
      #endif
    }

        /* trap duplicate name */
    if (spFindMarkerName (sound, markername)) {
        errcode = ML_ERR_DUPLICATE_NAME;
        goto clean;
    }

        /* allocate SPMarker */
    if ((marker = (SPMarker *)AllocNamedNode (sizeof *marker, markername)) == NULL) {
        errcode = ML_ERR_NOMEM;
        goto clean;
    }
    marker->spmk_Sound    = sound;
    marker->spmk_Position = position;
    InitList (&marker->spmk_BranchRefList, "BranchRefs");
    if (permanent) marker->spmk_MarkerFlags |= SP_MARKERF_PERMANENT;

        /* set up default branch (continue) */
    SetDefaultMarkerBranch (marker);

        /* on success, insert into player's marker list (sort by position) */
    UniversalInsertNode (&sound->spso_Markers, (Node *)marker, (bool (*)(Node *, Node *))CmpMarkers);

        /* return nonnegative value on success */
    return 0;

clean:
    FreeMarker (marker);
    return errcode;
}

/* sorts markers by position in increasing order. */
static bool CmpMarkers (const SPMarker *newmarker, const SPMarker *cmpmarker)
{
    return (bool)(newmarker->spmk_Position < cmpmarker->spmk_Position);
}


 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spremovemarker
 |||	spRemoveMarker - Manually remove an SPMarker from an SPSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err spRemoveMarker (SPMarker *marker)
 |||
 |||	  Description
 |||
 |||	    Removes and frees the specified marker from the sound that it belongs
 |||	    to. All markers belonging to an SPSound are automatically disposed of
 |||	    when that sound is disposed of with spRemoveSound() or when the player
 |||	    that owns that sound is deleted with spDeletePlayer(). Use this
 |||	    function if you want to remove a marker manually.
 |||
 |||	    This function cannot be used to remove a permanent marker
 |||	    (SP_MARKER_NAME_BEGIN or SP_MARKER_NAME_END) from a sound.
 |||
 |||	  Arguments
 |||
 |||	    marker                      Pointer to an SPMarker to remove. Can be
 |||	                                NULL. If this is one of the permanent
 |||	                                markers (SP_MARKER_NAME_BEGIN or
 |||	                                SP_MARKER_NAME_END) this function does
 |||	                                nothing and returns ML_ERR_INVALID_MARKER.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
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
 |||	    spFindMarkerName(), spAddMarker(), spRemoveSound(), spDeletePlayer()
 |||
 **/
Err spRemoveMarker (SPMarker *marker)
{
  #if DEBUG_Create
    printf ("spRemoveMarker ($%08lx)\n", marker);
  #endif

        /* trap NULL */
    if (!marker) return 0;

        /* trap permanent markers */
    if (marker->spmk_MarkerFlags & SP_MARKERF_PERMANENT) return ML_ERR_INVALID_MARKER;

    return spInternalRemoveMarker (marker);
}

/*
    Internal remove marker function.
    Removes any marker, including SP_MARKERF_PERMANENT markers.
*/
static Err spInternalRemoveMarker (SPMarker *marker)
{
    if (marker) {
        RemNode ((Node *)marker);
        FreeMarker (marker);
    }

    return 0;
}

/*
    Free marker
    Assumes it has already removed from Sound's marker list
*/
static void FreeMarker (SPMarker *marker)
{
    if (marker) {
            /* decouple marker from other markers */
            /* @@@ could move to spRemoveMarker(), except to keep parity of Set/ClearMarkerBranch() */

            /* first, decouple from marker that this marker branches to by clearing the marker. */
        ClearMarkerBranch (marker);

            /* next, restore default branch to all markers that branch to this one
               (must be done after above ClearMarkerBranch or else we'll get an infinite loop) */
        {
            MinNode *refnode, *succ;

            PROCESSLIST (&marker->spmk_BranchRefList,refnode,succ,MinNode) {
                SetDefaultMarkerBranch (GetMarkerFromBranchRefNode (refnode));
            }
        }

            /* free marker */
        FreeNamedNode ((Node *)marker);
    }
}


 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spsetmarkerdecisionfunction
 |||	spSetMarkerDecisionFunction - Install a marker decision function.
 |||
 |||	  Synopsis
 |||
 |||	    Err spSetMarkerDecisionFunction (SPSound *sound,
 |||	                                     const char *markerName,
 |||	                                     SPDecisionFunction decisionFunc,
 |||	                                     void *decisionData)
 |||
 |||	  Description
 |||
 |||	    Install a decision function that is to be called when the player
 |||	    reaches the specified marker. Each marker can have a different
 |||	    decision function.
 |||
 |||	    Clear the marker's decision function with
 |||	    spCleateMarkerDecisionFunction().
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to SPSound containing marker to
 |||	                                which to install decision function.
 |||
 |||	    markerName                  Name of marker in sound to which to install
 |||	                                decision function.
 |||
 |||	    decisionFunc                An SPDecisionFunction to install, or NULL
 |||	                                to clear.
 |||
 |||	    decisionData                A pointer to client-supplied data to be
 |||	                                passed to decisionFunc when called. Can be
 |||	                                NULL.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V24.
 |||
 |||	  Notes
 |||
 |||	    While a marker decision function is installed, reading from disc is
 |||	    interrupted every time this marker is encountered. This can have an
 |||	    impact on performance.
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spCleateMarkerDecisionFunction(), spSetDefaultDecisionFunction(),
 |||	    SPDecisionFunction
 |||
 **/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spclearmarkerdecisionfunction
 |||	spClearMarkerDecisionFunction - Clears a marker decision function.
 |||
 |||	  Synopsis
 |||
 |||	    Err spClearMarkerDecisionFunction (SPSound *sound,
 |||	                                       const char *markerName)
 |||
 |||	  Description
 |||
 |||	    Removes a marker decision function installed by
 |||	    spSetMarkerDecisionFunction().
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to SPSound containing marker from
 |||	                                which to remove decision function.
 |||
 |||	    markerName                  Name of marker in sound from which to remove
 |||	                                decision function.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in soundplayer.h V24.
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spSetMarkerDecisionFunction()
 |||
 **/
Err spSetMarkerDecisionFunction (SPSound *sound, const char *markername, SPDecisionFunction decisionfunc, void *decisiondata)
{
    SPMarker * const marker = spFindMarkerName (sound, markername); /* can be NULL */
    Err errcode;

        /* validate args */
    if ((errcode = ValidateFromMarker (marker)) < 0) goto clean;

        /* set marker decision function */
    marker->spmk_DecisionFunction = decisionfunc;
    marker->spmk_DecisionData     = decisiondata;

    return 0;

clean:
    return errcode;
}


 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spgetsoundfrommarker
 |||	spGetSoundFromMarker - Get SPSound that owns an SPMarker.
 |||
 |||	  Synopsis
 |||
 |||	    SPSound *spGetSoundFromMarker (const SPMarker *marker)
 |||
 |||	  Description
 |||
 |||	    Returns a pointer to the SPSound to which the specified SPMarker
 |||	    belongs.
 |||
 |||	  Arguments
 |||
 |||	    marker                      Pointer to SPMarker to interrogate.
 |||
 |||	  Return Value
 |||
 |||	    Pointer to SPSound that owns this SPMarker.
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
 |||	    spAddMarker(), spFindMarkerName(), spGetMarkerPosition(), spGetMarkerName()
 |||
 **/
SPSound *spGetSoundFromMarker (const SPMarker *marker)
{
    return marker->spmk_Sound;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spgetmarkerposition
 |||	spGetMarkerPosition - Get position of an SPMarker.
 |||
 |||	  Synopsis
 |||
 |||	    uint32 spGetMarkerPosition (const SPMarker *marker)
 |||
 |||	  Description
 |||
 |||	    Returns frame position of the SPMarker.
 |||
 |||	  Arguments
 |||
 |||	    marker                      Pointer to SPMarker to interrogate.
 |||
 |||	  Return Value
 |||
 |||	    Frame position of the SPMarker.
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
 |||	    spAddMarker(), spFindMarkerName(), spGetMarkerName()
 |||
 **/
uint32 spGetMarkerPosition (const SPMarker *marker)
{
    return marker->spmk_Position;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spgetmarkername
 |||	spGetMarkerName - Get name of an SPMarker.
 |||
 |||	  Synopsis
 |||
 |||	    const char *spGetMarkerName (const SPMarker *marker)
 |||
 |||	  Description
 |||
 |||	    Returns a pointer to the name of the SPMarker.
 |||
 |||	  Arguments
 |||
 |||	    marker                      Pointer to SPMarker to interrogate.
 |||
 |||	  Return Value
 |||
 |||	    Pointer to the SPMarker name string.
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
 |||	    spAddMarker(), spFindMarkerName(), spGetMarkerPosition()
 |||
 **/
const char *spGetMarkerName (const SPMarker *marker)
{
    return marker->spmk.n_Name;
}


/* -------------------- Marker Actions */

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spcontinueatmarker
 |||	spContinueAtMarker - Clear static branch at a marker.
 |||
 |||	  Synopsis
 |||
 |||	    Err spContinueAtMarker (SPSound *sound, const char *markerName)
 |||
 |||	  Description
 |||
 |||	    Restores the marker to its default action of playing through (for a
 |||	    marker in the middle of a sound), or stopping (for a marker at the end
 |||	    of a sound), barring the results from a marker or default decision
 |||	    function. This resets the effect of spBranchAtMarker() or
 |||	    spStopAtMarker().
 |||
 |||	    Markers can have one of 3 static actions: continue, branch, or stop.
 |||	    This action can be changed at any time, including while the player is
 |||	    playing.
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to SPSound containing marker to
 |||	                                clear branch from.
 |||
 |||	    markerName                  Name of marker in sound to clear branch from. All
 |||	                                markers except markers at the beginning of
 |||	                                sounds are considered valid for this
 |||	                                function.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
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
 |||	    spBranchAtMarker(), spStopAtMarker(), spSetMarkerDecisionFunction(),
 |||	    spSetDefaultDecisionFunction()
 |||
 **/
Err spContinueAtMarker (SPSound *sound, const char *markername)
{
    SPMarker * const marker = spFindMarkerName (sound, markername); /* can be NULL */
    Err errcode;

  #if DEBUG_ActionControl
    printf ("spContinueAtMarker ($%08lx, '%s')\n", sound, markername);
  #endif

        /* validate args */
    if ((errcode = ValidateFromMarker (marker)) < 0) goto clean;

        /* set up branch to self (continue)  */
    SetDefaultMarkerBranch (marker);

    return 0;

clean:
    return errcode;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/splinksounds
 |||	spLinkSounds - Branch at end of one sound to beginning of another.
 |||
 |||	  Synopsis
 |||
 |||	    Err spLinkSounds (SPSound *fromSound, SPSound *toSound)
 |||
 |||	  Description
 |||
 |||	    This is a convenient macro for linking multiple sounds together. It
 |||	    simply does this:
 |||
 |||	        spBranchAtMarker (fromSound, SP_MARKER_NAME_END,
 |||	                          toSound, SP_MARKER_NAME_BEGIN)
 |||
 |||	  Arguments
 |||
 |||	    fromSound                   Pointer to SPSound to link from.
 |||
 |||	    toSound                     Pointer to SPSound to link to the end
 |||	                                of fromsound.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in soundplayer.h V24.
 |||
 |||	  Notes
 |||
 |||	    Both SPSounds must belong to the same SPPlayer.
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spBranchAtMarker(), spLoopSound()
 |||
 **/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/sploopsound
 |||	spLoopSound - Branch at end of sound back to the beginning.
 |||
 |||	  Synopsis
 |||
 |||	    Err spLoopSound (SPSound *sound)
 |||
 |||	  Description
 |||
 |||	    This is a convenient macro for looping a single sound. It simply does
 |||	    this:
 |||
 |||	        spBranchAtMarker (sound, SP_MARKER_NAME_END,
 |||	                          sound, SP_MARKER_NAME_BEGIN)
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to SPSound to loop.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in soundplayer.h V24.
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spBranchAtMarker(), spLinkSounds()
 |||
 **/
 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spbranchatmarker
 |||	spBranchAtMarker - Set up a static branch at a marker.
 |||
 |||	  Synopsis
 |||
 |||	    Err spBranchAtMarker (SPSound *fromSound, const char *fromMarkerName,
 |||	                          SPSound *toSound, const char *toMarkerName)
 |||
 |||	  Description
 |||
 |||	    Sets up a static branch from one marker to another. Barring the result
 |||	    of a marker or default decision function, when the playback encounters
 |||	    the 'from' marker, the playback will skip to the 'to' marker seamlessly.
 |||
 |||	    Markers can have one of 3 static actions: continue, branch, or stop.
 |||	    This action can be changed at any time, including while the player is
 |||	    playing.
 |||
 |||	  Arguments
 |||
 |||	    fromSound                   Pointer to SPSound containing marker to
 |||	                                branch from.
 |||
 |||	    fromMarkerName              Name of marker in fromSound to branch from. All markers
 |||	                                except markers at the beginning of sounds
 |||	                                are considered valid "from" markers.
 |||
 |||	    toSound                     Pointer to SPSound containing marker to
 |||	                                branch to.
 |||
 |||	    toMarkerName                Name of marker to in toSound branch to. All markers
 |||	                                except markers at the end of sounds are
 |||	                                considered valid "to" markers.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Notes
 |||
 |||	    Both SPSounds must belong to the same SPPlayer.
 |||
 |||	    Good sounding results require good placement of markers in the sounds.
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
 |||	    spStopAtMarker(), spContinueAtMarker(), spSetMarkerDecisionFunction(),
 |||	    spSetDefaultDecisionFunction()
 |||
 **/
Err spBranchAtMarker (SPSound *fromsound, const char *frommarkername, SPSound *tosound, const char *tomarkername)
{
    SPMarker * const frommarker = spFindMarkerName (fromsound, frommarkername);     /* can be NULL */
    SPMarker * const tomarker   = spFindMarkerName (tosound, tomarkername);         /* can be NULL */
    Err errcode;

  #if DEBUG_ActionControl
    printf ("spBranchAtMarker ($%08lx, '%s', $%08lx, '%s')\n", fromsound, frommarkername, tosound, tomarkername);
  #endif

        /* validate args */
    if (fromsound->spso_Player != tosound->spso_Player) {       /* sounds must belong to the same player */
        errcode = ML_ERR_BAD_ARG;
        goto clean;
    }
    if ((errcode = ValidateFromMarker (frommarker)) < 0) goto clean;
    if ((errcode = ValidateToMarker (tomarker)) < 0) goto clean;

        /* set new branch */
    SetMarkerBranch (frommarker, tomarker);

    return 0;

clean:
    return errcode;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spstopatmarker
 |||	spStopAtMarker - Stop when playback reaches marker.
 |||
 |||	  Synopsis
 |||
 |||	    Err spStopAtMarker (SPSound *sound, const char *markerName)
 |||
 |||	  Description
 |||
 |||	    Barring results of a marker or default decision function, causes
 |||	    playback to stop when the player reaches this marker.
 |||
 |||	    Markers can have one of 3 static actions: continue, branch, or stop.
 |||	    This action can be changed at any time, including while the player is
 |||	    playing.
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to SPSound containing marker to
 |||	                                stop at.
 |||
 |||	    markerName                  Name of marker in sound to stop at. All markers
 |||	                                except markers at the beginning of sounds
 |||	                                are considered valid places to stop.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
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
 |||	    spBranchAtMarker(), spContinueAtMarker(),
 |||	    spSetMarkerDecisionFunction(), spSetDefaultDecisionFunction()
 |||
 **/
Err spStopAtMarker (SPSound *sound, const char *markername)
{
    SPMarker * const marker = spFindMarkerName (sound, markername); /* can be NULL */
    Err errcode;

  #if DEBUG_ActionControl
    printf ("spStopAtMarker ($%08lx, '%s')\n", sound, markername);
  #endif

        /* validate args */
    if ((errcode = ValidateFromMarker (marker)) < 0) goto clean;

        /* remove marker branch (stop) */
    ClearMarkerBranch (marker);

    return 0;

clean:
    return errcode;
}

/*
    Changes frommarker's BranchToMarker and does necessary decoupling and
    recoupling of BranchRefNodes and BranchRefLists.

    frommarker can have a non-NULL BranchToMarker
    tomarker can be any marker including frommarker or NULL
        . NULL - stop
        . frommarker - continue
        . other marker - branch to marker
*/
static void SetMarkerBranch (SPMarker *frommarker, SPMarker *tomarker)
{
  #if DEBUG_ActionControl
    printf ("SetMarkerBranch ($%08lx,$%08lx)\n", frommarker, tomarker);
  #endif

        /* Decouple from old previous BranchToMarker's BranchRefList */
    if (frommarker->spmk_BranchToMarker) RemNode ((Node *)&frommarker->spmk_BranchRefNode);

        /* set new BranchToMarker */
    frommarker->spmk_BranchToMarker = tomarker;

        /* Add to new BranchToMarker's BranchRefList */
    if (tomarker) AddTail (&tomarker->spmk_BranchRefList, (Node *)&frommarker->spmk_BranchRefNode);
}


/* -------------------- Player control */

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spstartreading
 |||	spStartReading - Start SPPlayer reading from an SPSound.
 |||
 |||	  Synopsis
 |||
 |||	    Err spStartReading (SPSound *startSound,
 |||	                        const char *startMarkerName)
 |||
 |||	  Description
 |||
 |||	    This function begins the process of spooling data for an SPPlayer,
 |||	    processing marker actions along the way. It completely fills the
 |||	    spooler buffers belonging to the SPPlayer in preparation for playback,
 |||	    which can take an unpredictable amount of time. Therefore, the actual
 |||	    function to begin playback, spStartPlaying(), is a separate call so
 |||	    that starting the actual sound playback may be synchronized to some
 |||	    user event.
 |||
 |||	    Normally this function is called while the player's SP_STATUS_F_READING
 |||	    status flag is cleared (see spGetPlayerStatus()), in which case it merely
 |||	    begins reading at the specified location. The SP_STATUS_F_READING flag is
 |||	    then set. If the entire sound data to be played fits completely into the
 |||	    buffers, this flag is cleared again before this function returns.
 |||
 |||	    This function can also be called while the SP_STATUS_F_READING flag is set in
 |||	    order to force the playback to a different location without waiting for
 |||	    a marker branch or decision function. This abnormal method of
 |||	    relocating sound playback will almost certainly produce unpleasant
 |||	    sound output, but there may be times when it is necessary. Note that
 |||	    this merely causes reading to begin at a new location. It does not
 |||	    does not flush the spooler. Anything buffered already will continue
 |||	    play.
 |||
 |||	  Arguments
 |||
 |||	    startSound                  Pointer to SPSound to start reading from.
 |||
 |||	    startMarkerName             Name of marker in startSound to start reading from.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V24.
 |||
 |||	  Examples
 |||
 |||	    // error checking omitted for brevity
 |||
 |||	    {
 |||	        const int32 playersigs = spGetPlayerSignalMask (player);
 |||
 |||	            // read from beginning of one of the sounds
 |||	        spStartReading (player, sound1, SP_MARKER_NAME_BEGIN);
 |||
 |||	        // could wait for some event to trigger playback here
 |||
 |||	            // begin playback
 |||	        spStartPlayingVA (player,
 |||	                          AF_TAG_AMPLITUDE, 0x7fff,
 |||	                          TAG_END);
 |||
 |||	            // service player until it's done
 |||	        while (spGetPlayerStatus(player) & SP_STATUS_F_BUFFER_ACTIVE) {
 |||	            const int32 sigs = WaitSignal (playersigs);
 |||
 |||	            spService (player, playersigs);
 |||	        }
 |||	    }
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spStartPlaying(), spStop(), spService(), spGetPlayerStatus()
 |||
 **/
Err spStartReading (SPSound *startsound, const char *startmarkername)
{
    SPPlayer * const player      = startsound->spso_Player;
    SPMarker * const startmarker = spFindMarkerName (startsound, startmarkername);  /* can be NULL */
    Err errcode;

  #if DEBUG_PlayerControl
    printf ("spStartReading() sound=$%08lx marker='%s'\n", startsound, startmarkername);
  #endif

    /*
        @@@ Note: not testing SP_READERSTATUSF_READING because this only saves
            the client from himself and doing so prevents being able to jam
            the stream to a new location in an emergency (e.g. service function fails).
    */

        /* validate args */
    if ((errcode = ValidateToMarker (startmarker)) < 0) goto clean;

        /* set reader to startmarker */
    SetReaderStatus (player, startmarker);

        /* fill spooler */
    return FillSpooler (player);

clean:
    return errcode;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spstartplaying
 |||	spStartPlaying - Begin emitting sound for an SPPlayer.
 |||
 |||	  Synopsis
 |||
 |||	    Err spStartPlaying (SPPlayer *player, const TagArg *samplerTags)
 |||
 |||	    Err spStartPlayingVA (SPPlayer *player, uint32 samplerTag1, ...)
 |||
 |||	  Description
 |||
 |||	    Begins the actual output of spooled sound. This function begins
 |||	    playback immediately making it suitable for being called in
 |||	    sync with some user activity if necessary. spStartReading() prefills
 |||	    the spooler buffers. This function should therefore be called some time
 |||	    after calling spStartReading().
 |||
 |||	    Calling this function starts the process of signals arriving which
 |||	    need to be serviced with spService(). This function causes the
 |||	    SP_STATUS_F_PLAYING flag to be set.
 |||
 |||	    Call spStop() to manually stop or wait for playback to finish by
 |||	    checking spGetPlayerStatus() after calling spService().
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to SPPlayer to start playing.
 |||
 |||	  Tags
 |||
 |||	    AF_TAG_AMPLITUDE            (uint32) Amplitude to set SPPlayer's sample
 |||	                                player instrument to. Defaults to whatever
 |||	                                the last setting was. Valid range is 0 to
 |||	                                0x7fff.
 |||
 |||	    AF_TAG_RATE                 (uint32) Sample playback rate value for
 |||	                                variable rate sample player instruments
 |||	                                expressed as a 1.15 fixed point value.
 |||	                                0x8000 is the normal rate (e.g. 44100
 |||	                                sample frames / sec). Valid range is
 |||	                                0x0001 (really, really, slow) to 0xffff
 |||	                                (nearly twice the normal rate). Defaults
 |||	                                to whatever the last setting was
 |||	                                (variable rate sample player instruments
 |||	                                typically default to 0x8000). Ignored for
 |||	                                fixed rate sample players.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Notes
 |||
 |||	    The only bad thing that will happen if you call this function before calling
 |||	    spStartReading() is that you won't have any control over the
 |||	    precise time that playback begins. In that case, playback would start
 |||	    as soon as spStartReading() has read a buffer's worth of data. The player also
 |||	    might starved briefly if the 2nd buffer weren't ready before the first one
 |||	    finished.
 |||
 |||	    Multiple calls to spStartPlaying() without an intervening stop of some
 |||	    kind (spStop() or marker causing a stop) cause stuttered sound output.
 |||
 |||	    A call to spStartPlaying() will supercede a previous call to spPause().
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
 |||	    spStartReading(), spStop(), spService(), spGetPlayerStatus()
 |||
 **/
VAGLUE_FUNC (Err, spStartPlayingVA (SPPlayer *player, VAGLUE_VA_TAGS), spStartPlaying (player, VAGLUE_TAG_POINTER))
Err spStartPlaying (SPPlayer *player, const TagArg *samplertags)
{
  #if DEBUG_PlayerControl
    printf ("spStartPlaying()\n");
  #endif

    return ssplStartSpoolerTags (player->sp_Spooler, samplertags);
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spstop
 |||	spStop - Stops an SPPlayer
 |||
 |||	  Synopsis
 |||
 |||	    Err spStop (SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Stops reading and sound playback.
 |||
 |||	    Clears all SP_STATUS_F_ flags. Can be called regardless of
 |||	    SPPlayer's current state. Multiple calls have no effect.
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to SPPlayer to stop.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
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
 |||	    spStartReading(), spStartPlaying(), spGetPlayerStatus()
 |||
 **/
Err spStop (SPPlayer *player)
{
  #if DEBUG_PlayerControl
    printf ("spStop()\n");
  #endif

        /* stop reading */
    SetReaderStatus (player, NULL);

        /* abort spooler */
    ssplAbort (player->sp_Spooler, NULL);

    return 0;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/sppause
 |||	spPause - Pause an SPPlayer.
 |||
 |||	  Synopsis
 |||
 |||	    Err spPause (SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Pauses playback of an SPPlayer. Use this to stop playback dead in its
 |||	    tracks and be able to pick up at the same spot. This differs from
 |||	    spStop() in that playback cannot be restarted at the position at which
 |||	    spStop() occurred. Resume playback with spResume().
 |||
 |||	    Sets SP_STATUS_F_PAUSED. Calling spPause() multiple times has no effect,
 |||	    the calls do not nest. Calling spPause() while the SPPlayer's
 |||	    SP_STATUS_F_PLAYING flag is not set has no effect. The paused state is
 |||	    superceded by a call to spStop() or spStartPlaying().
 |||
 |||	    There is presently no way to know if an SPPlayer is paused.
 |||
 |||	    Data can still be spooled while paused if there were any completed
 |||	    buffers prior to pausing.
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to SPPlayer to pause.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
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
 |||	    spResume(), spStartPlaying(), spStop()
 |||
 **/
Err spPause (SPPlayer *player)
{
  #if DEBUG_PlayerControl
    printf ("spPause()\n");
  #endif

    return ssplPause (player->sp_Spooler);
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spresume
 |||	spResume - Resume playback of an SPPlayer after being paused.
 |||
 |||	  Synopsis
 |||
 |||	    Err spResume (SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Resumes playback of an SPPlayer after being paused.
 |||
 |||	    Clears the SP_STATUS_F_PAUSED flag. Calling spResume() multiple
 |||	    times has no effect, the calls do not nest. Calling spResume()
 |||	    while the SPPlayer is the SP_STATUS_F_PLAYING flag is not set
 |||	    has no effect. The paused state is superceded by a call to
 |||	    spStop() or spStartPlaying().
 |||
 |||	    There is presently no way to know if an SPPlayer is paused.
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to SPPlayer to resume.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
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
 |||	    spPause(), spStartPlaying(), spStop()
 |||
 **/
Err spResume (SPPlayer *player)
{
  #if DEBUG_PlayerControl
    printf ("spResume()\n");
  #endif

    return ssplResume (player->sp_Spooler);
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spservice
 |||	spService - Service SPPlayer
 |||
 |||	  Synopsis
 |||
 |||	    int32 spService (SPPlayer *player, int32 signals)
 |||
 |||	  Description
 |||
 |||	    This function
 |||	        . processes signals received by the client's sent by the SPPlayer,
 |||	          and
 |||	        . reads and spools more sound data, processing marker actions
 |||	          along the way.
 |||
 |||	    This function synchronizes the SPPlayer's SoundSpooler to the state of
 |||	    the signals read from the last WaitSignal(). You must call it after
 |||	    waiting for SPPlayer's signals before calling any other sound player
 |||	    function for this SPPlayer (with the exception of spDeletePlayer(),
 |||	    which can tolerate the spooler being out of sync with the task's
 |||	    signals).
 |||
 |||	    This function can set or clear the SP_STATUS_F_BUFFER_ACTIVE flag, depending
 |||	    on buffer usage. It clears SP_STATUS_F_READING when there's no more sound data
 |||	    to read. This function may be called under any player state including
 |||	    when SP_STATUS_F_PLAYING is not set without any ill effects.
 |||
 |||	    This function must be called exactly once for each WaitSignal() on
 |||	    an SPPlayer's signals. It will almost certainly have dangerous results
 |||	    if fed incorrect signals (signals other than from the most recent
 |||	    WaitSignal(), or signals that it has already processed).
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to SPPlayer to service.
 |||
 |||	    signals                     Signals last received to process. Ignores
 |||	                                any signals that do not belong to this
 |||	                                SPPlayer. Does nothing if none of the
 |||	                                SPPlayer's signals are set.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative player status flags (SP_STATUS_F_) on success;
 |||	    negative error code on failure.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in music.lib V24.
 |||
 |||	  Examples
 |||
 |||	    // error checking omitted for brevity
 |||
 |||	    {
 |||	        const int32 playersigs = spGetPlayerSignalMask (player);
 |||
 |||	            // service player until it's done
 |||	        while (spGetPlayerStatus(player) & SP_STATUS_F_BUFFER_ACTIVE) {
 |||	            const int32 sigs = WaitSignal (playersigs |
 |||	                                           othersignals1 |
 |||	                                           othersignals2);
 |||
 |||	                // service player before servicing other
 |||	                // things that might affect the player
 |||	            spService (player, playersigs);
 |||
 |||	                // service other things
 |||	            if (sigs & othersignals1) {
 |||	            }
 |||	            if (sigs & othersignals2) {
 |||	            }
 |||	        }
 |||	    }
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spGetPlayerSignalMask(), spGetPlayerStatus(), spStartReading(),
 |||	    spStartPlaying()
 |||
 **/
int32 spService (SPPlayer *player, int32 signals)
{
    Err errcode;

    /* note: tolerates all player states - no need to validate */

        /* process spooler signals */
    if ((errcode = ssplProcessSignals (player->sp_Spooler, signals, NULL)) < 0) goto clean;

        /* fill spooler */
    if ((errcode = FillSpooler (player)) < 0) goto clean;

        /* return current player state on success */
    return spGetPlayerStatus (player);

clean:
    return errcode;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spgetplayerstatus
 |||	spGetPlayerStatus - Get current status of an SPPlayer.
 |||
 |||	  Synopsis
 |||
 |||	    int32 spGetPlayerStatus (const SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Returns a set of flags indicating the current state of an SPPlayer.
 |||	    The most useful thing about this function is that it can be used
 |||	    to find out when an SPPlayer has finished playing.
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to SPPlayer to interrogate.
 |||
 |||	  Return Value
 |||
 |||	    Any combination of the following SP_STATUS_F_ flags (always a
 |||	    non-negative value):
 |||
 |||	    SP_STATUS_F_BUFFER_ACTIVE   This flag indicates that there's something
 |||	                                in the sound buffers waiting to
 |||	                                be played. It is set or cleared by
 |||	                                spStartReading(), spService(),
 |||	                                and cleared by spStop().
 |||
 |||	                                When this flag has been cleared
 |||	                                after playback has started,
 |||	                                all of the sound data has been
 |||	                                played: it's safe to stop.
 |||
 |||	    SP_STATUS_F_READING         This flag indicates that there's more data to
 |||	                                read. It is set by spStartReading() and
 |||	                                cleared when there is no more to read (by
 |||	                                spStartReading() or spService()),
 |||	                                or when spStop() is called.
 |||
 |||	    SP_STATUS_F_PLAYING         This flag indicates that playback is
 |||	                                underway. It is set by
 |||	                                spStartPlaying() and cleared by
 |||	                                spStop().
 |||
 |||	    SP_STATUS_F_PAUSED          This flag indicates that player has been
 |||	                                paused. It is set by spPause() and cleared
 |||	                                by spResume(), spStop(),spStartPlaying().
 |||	                                This flag is really only meaningful when
 |||	                                SP_STATUS_F_PLAYING is set.
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
 |||	    spStartReading(), spStartPlaying(), spStop()
 |||
 **/
/* this must be a signed value because spService() returns it on success */
int32 spGetPlayerStatus (const SPPlayer *player)
{
    int32 status = 0;

        /* convert spooler status to player status */
    {
        const int32 sspl_status = ssplGetSpoolerStatus (player->sp_Spooler);

        if (sspl_status & SSPL_STATUS_F_ACTIVE)     status |= SP_STATUS_F_BUFFER_ACTIVE;
        if (sspl_status & SSPL_STATUS_F_STARTED)    status |= SP_STATUS_F_PLAYING;
        if (sspl_status & SSPL_STATUS_F_PAUSED)     status |= SP_STATUS_F_PAUSED;
    }

        /* pick up remaining player status */
    if (player->sp_ReaderStatus & SP_READERSTATUSF_READING) status |= SP_STATUS_F_READING;

    return status;
}


/* -------------------- FillSpooler() */

static Err FillSpoolerBuffer (SPPlayer *, SoundBufferNode *);
static Err ReadSoundData (SPPlayer *, char *bufaddr, uint32 bufsize, bool optimized, char **useaddrp, uint32 *uselenp);
static SPMarker *FindNextActiveMarker (const SPPlayer *);

/*
    Fills the spooler by reading from the current sound, following marker actions, etc.

    If this fails, the reader is still in a stable state, although some sound data may
    have been lost. It does cause spStartReading() and spService() to return an error code.
    To recover, the client can call spStartReading() at a new location or may even try calling
    spService(), but that may just return the same error, depending on the cause.
*/
static Err FillSpooler (SPPlayer *player)
{
    SoundBufferNode *sbn;
    Err errcode;

        /* fill as many buffers as are available while we're still reading */
    while ( (player->sp_ReaderStatus & SP_READERSTATUSF_READING) &&
            (sbn = ssplRequestBuffer (player->sp_Spooler)) != NULL )
    {

            /* fill buffer (need to return unused SBN to spooler if this fails) */
        if ( (errcode = FillSpoolerBuffer (player, sbn)) < 0 ) {
            ssplUnrequestBuffer (player->sp_Spooler, sbn);
            goto clean;
        }

            /* submit SBN (spooler automatically reclaims the SBN if this fails) */
        if ( (errcode = ssplSendBuffer (player->sp_Spooler, sbn)) < 0 ) goto clean;
    }

    return 0;

clean:
    return errcode;
}

/*
    Fill an spooler buffer, and set up SBN to point to it
*/
static Err FillSpoolerBuffer (SPPlayer *player, SoundBufferNode *sbn)
{
    const int32 bufindex = ssplGetSequenceNum (player->sp_Spooler, sbn);    /* buffer associated with sbn (based on sequence number) */
    char * const bufaddr = (char *)player->sp_BufferArray[bufindex];
    char *spooladdr = NULL;     /* address, length of buffer to pass to spooler */
    uint32 spoollen = 0;
    Err errcode;

  #if DEBUG_FillSpooler
    printf ("fill #%ld: $%08lx,%lu\n", bufindex, bufaddr, player->sp_BufferSize);
  #endif

        /* read sound (optimized: point directly to in-memory sample) */
    if ((errcode = ReadSoundData (player, bufaddr, player->sp_BufferSize, TRUE, &spooladdr, &spoollen)) < 0) goto clean;

        /*
            Is this too little to send to the spooler?
            And is there more that we can send to the spooler?
        */
    if ( spoollen < SP_SPOOLER_MIN_LENGTH &&
         (player->sp_ReaderStatus & SP_READERSTATUSF_READING) )
    {
        char *useaddr1, *useaddr2 = NULL;
        uint32 uselen1, uselen2 = 0;

            /* Pack first part into the beginning of the buffer to maximize available buffer space. */
        useaddr1 = (char *)memmove (bufaddr, spooladdr, spoollen);
        uselen1  = spoollen;

      #if DEBUG_FillSpooler || DEBUG_FillSpoolerShort
        printf ("  short: $%08lx,%lu\n", useaddr1, uselen1);
      #endif

            /* complete spooler buffer (non-optimized) */
            /* (@@@ could take advantage of useaddr1 == bufaddr) */
        if ( (errcode = ReadSoundData (player,
                useaddr1 + uselen1, (bufaddr + player->sp_BufferSize) - (useaddr1 + uselen1),
                FALSE, &useaddr2, &uselen2)) < 0 ) goto clean;

        /*
            Make buffer contiguous:
            Repack buffer move first part to be adjacent to 2nd part
            (which is presumably larger or we wouldn't have gotten here)

            note: there might be a partial frame at the end of the 2nd part,
                  but that isn't affected by this.
        */
        spooladdr = (char *)memmove (useaddr2 - uselen1, useaddr1, uselen1);
        spoollen = uselen1 + uselen2;
    }

        /* set up SBN */
  #if DEBUG_FillSpooler || DEBUG_FillSpoolerShort
    printf ("spool #%ld: $%08lx,%lu\n", bufindex, spooladdr, spoollen);
  #endif
    return ssplSetBufferAddressLength (player->sp_Spooler, sbn, spooladdr, spoollen);

clean:
    return errcode;
}

/*
    Read sound data into buffer limited either by location of next marker
    or size of buffer.  If reading was limited by a marker, perform it's
    action at end.
*/
static Err ReadSoundData (SPPlayer *player, char *bufaddr, uint32 bufsize, bool optimized, char **useaddrp, uint32 *uselenp)
{
    SPSound * const sound = player->sp_CurrentSound;
    SPMarker *nextmarker;
    Err errcode;

        /*
            look ahead for next significant marker
            (trap failure - which pretty much means mangled data)
        */
    if ((nextmarker = FindNextActiveMarker (player)) == NULL) {
        errcode = ML_ERR_CORRUPT_DATA;
        goto clean;
    }

  #if DEBUG_FillSpooler
    printf ( "  read: sound=$%08lx cursor=%ld nextmarker=%ld buf=$%08lx,%lu partial=$%08lx,%lu optimized=%d\n",
        player->sp_CurrentSound, player->sp_Cursor,
        nextmarker->spmk_Position,
        bufaddr, bufsize,
        player->sp_PartialFrameAddr, player->sp_PartialFrameLen,
        optimized );
  #endif

        /* read sound data, advance cursor, return where valid data in buffer is */
    {
            /*
                common file/mem parameters
                Note: all offsets and lengths are in bytes
            */
            /* cursorbyte is next position in sound file to actually read (past end of partialframe) */
        const uint32 cursorbyte = spCvtFrameToByte (&player->sp_SampleFrameInfo, player->sp_Cursor) + player->sp_PartialFrameLen;
            /* rembytes is remaining # of bytes to read from sound file (not including partialframe) */
        const uint32 rembytes = spCvtFrameToByte (&player->sp_SampleFrameInfo, nextmarker->spmk_Position) - cursorbyte;
        uint32 nextpartiallen = 0;

      #if DEBUG_FillSpooler
        printf ("  read: cursorbyte=%lu rembytes=%lu\n", cursorbyte, rembytes);
      #endif

      #if DEBUG_MungFillSpooler
            /* clobber available area for read with junk */
        memset (bufaddr, 0x40, bufsize);
      #endif

            /*
                Call SPSound's read method

                Method must fill in *useaddrp, *uselenp, *nextpartiallenp.
                *useaddrp must be DMA aligned.
                *uselenp must be a multiple of SPSampleFrameInfo.sfi_AlignmentBytes.
                *nextpartiallenp must be in the range of 0..SPSampleFrameInfo.sfi_AlignmentBytes-1.
            */
        if ((errcode = sound->spso_ClassDefinition->spsc_Read (sound, cursorbyte, rembytes, bufaddr, bufsize, optimized, useaddrp, uselenp, &nextpartiallen)) < 0) goto clean;

            /* update Cursor and PartialFrame */
        player->sp_Cursor += spCvtByteToFrame (&player->sp_SampleFrameInfo, *uselenp);
        spSetPartialFrame (player, *useaddrp + *uselenp, nextpartiallen);

      #if DEBUG_FillSpooler
        printf ("  use: $%08lx,%lu  partial: $%08lx,%lu\n", *useaddrp, *uselenp, player->sp_PartialFrameAddr, player->sp_PartialFrameLen);
      #endif

      #if DEBUG_MungFillSpooler
            /* clobber area outside of valid area with junk (only if *useaddr is actually in the buffer) */
        if (*useaddrp >= bufaddr && *useaddrp < bufaddr + bufsize) {
            memset (bufaddr, 0x7f, *useaddrp-bufaddr);
            memset (*useaddrp+*uselenp+player->sp_PartialFrameLen, 0x7f, (bufaddr+bufsize) - (*useaddrp+*uselenp+player->sp_PartialFrameLen));
        }
      #endif
    }

        /* perform marker action if we're at a marker */
    if (player->sp_Cursor == nextmarker->spmk_Position) {
        if ((errcode = PerformMarkerAction (player, nextmarker)) < 0) goto clean;
    }

    return 0;

clean:
    return errcode;
}

/* locate next marker that we need to do something with */
static SPMarker *FindNextActiveMarker (const SPPlayer *player)
{
    SPMarker *marker;

        /* find first marker after current position (fail if there isn't one) */
    if ((marker = FindMarkerAfter (player->sp_CurrentSound, player->sp_Cursor)) == NULL) return NULL;

        /* if there's a default decision function, always return first marker after current position */
    if (player->sp_DefaultDecisionFunction) return marker;

        /* otherwise, find first non-trivial marker */
    for (; IsNode (&player->sp_CurrentSound->spso_Markers, marker); marker = (SPMarker *)NextNode ((Node *)marker)) {
        if ( marker->spmk_DecisionFunction ||           /* if there's a marker decision function */
             marker->spmk_BranchToMarker != marker ||   /* if the marker is set to stop or branch to something other than itself */
             IsEndMarker (marker)                       /* or if the marker is at end of sound */
           ) return marker;
    }

    return NULL;
}


/* -------------------- Marker action processing */

static Err GetMarkerAction (SPAction *, SPPlayer *, SPMarker *);
static int32 CallDecisionFunction (SPAction *, SPPlayer *, SPDecisionFunction, void *decisiondata, SPMarker *);

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spdecisionfunction
 |||	SPDecisionFunction - Typedef for decision callback functions
 |||
 |||	  Synopsis
 |||
 |||	    typedef Err (*SPDecisionFunction) (SPAction *resultAction,
 |||	                                       void *decisionData,
 |||	                                       SPSound *sound,
 |||	                                       const char *markerName)
 |||
 |||	  Description
 |||
 |||	    Client-supplied callback functions of this form may be installed in
 |||	    each marker and/or as a default for an SPPlayer.
 |||
 |||	    A marker's action is processed when the sound data for that marker's
 |||	    position is read by the player when any of the following is true:
 |||	        . there is a default decision function for the marker's SPPlayer.
 |||	        . the marker has a marker decision function
 |||	        . the marker specifies a non-trivial action (branch or stop)
 |||	        . the marker is at the end of a sound
 |||
 |||	    Normally, disc I/O is performed based on the size of each buffer. When
 |||	    one of the above conditions is true for a marker, the player will
 |||	    split up disc I/O at that marker in order to prepare for a possible
 |||	    branch. This impacts disc I/O efficiency. It is important to avoid
 |||	    having so many markers with non-trivial actions as to cripple the
 |||	    players ability to produce smooth sounding output. Since default
 |||	    decision functions affect every marker, it is necessary to insure
 |||	    that the marker spacing is such that smooth playback can still be
 |||	    produced. This is unfortunately one of those things that requires a
 |||	    bit of trial and error.
 |||
 |||	    The following steps are used in determining what action the SPPlayer
 |||	    will take when processing a marker's action:
 |||
 |||	        . If the marker has a decision function, call it. If the marker's
 |||	          decision function sets its resultAction (e.g. by a call to
 |||	          spSetBranchAction() or spSetStopAction()), then take that action.
 |||
 |||	        . Otherwise, if the there is a default decision function, call it.
 |||	          If the default decision function sets its resultAction (e.g. by a call to
 |||	          spSetBranchAction() or spSetStopAction()), then take that action.
 |||
 |||	        . Otherwise, if the marker specifies some static action (branch or
 |||	          stop), take that action.
 |||
 |||	        . Otherwise, if the marker is at the end of a sound, stop reading.
 |||
 |||	        . Otherwise continue reading the sound after the marker.
 |||
 |||	    A decision function MAY do almost anything to the SPPlayer that owns
 |||	    this marker including adding new sounds or markers, changing the
 |||	    static action for this or any other marker, changing the default or
 |||	    marker decision functions for this or any other marker, deleting this
 |||	    or any other marker or sound (with the below caveats in mind).
 |||
 |||	    A decision functoin MUST NOT do any of the following:
 |||	        . call any function that changes the player state (e.g.
 |||	          spDeletePlayer(), spStop(), spStartReading(), spStartPlaying(),
 |||	          spService(), etc) for the current SPMarker's SPPlayer.
 |||
 |||	        . delete the current SPMarker's SPSound since this has the side effect
 |||	          of calling spStop().
 |||
 |||	        . delete the current marker without
 |||	          setting up resultAction to stop or branch to another marker.
 |||
 |||	        . take a long time to execute.
 |||
 |||	  Arguments
 |||
 |||	    resultAction                SPAction to optionally be filled out
 |||	                                by decision function. If it isn't filled
 |||	                                out, the sound player ignores the call
 |||	                                to the decision function and continues
 |||	                                as if the decision function hadn't been
 |||	                                installed.
 |||
 |||	    decisionData                Pointer to client data passed to
 |||	                                spSetMarkerDecisionFunction() or
 |||	                                spSetDefaultDecisionFunction().
 |||
 |||	    sound                       Pointer to SPSound containing marker for which the player
 |||	                                is requesting a playback decision.
 |||
 |||	    markerName                  Name of marker for which the player
 |||	                                is requesting a playback decision.
 |||
 |||	  Return Value
 |||
 |||	    Client should return a non-negative value for success, or a negative error code on failure.
 |||	    An error code returned to the sound player causes the sound player to stop reading
 |||	    and return that error code back to the caller.
 |||
 |||	  Outputs
 |||
 |||	    Decision function can set the SPAction by calling one of the following:
 |||
 |||	        spSetBranchAction() - specifies a marker to branch to as the result of
 |||	        this decision function.
 |||
 |||	        spSetStopAction() - specifies that reader should stop as the result of
 |||	        this decision function.
 |||
 |||	    If the decision does note set the SPAction, the sound player ignores the
 |||	    the decision function (acts as if decision function hadn't been installed).
 |||
 |||	  Examples
 |||
 |||	    Err mydecision (SPAction *resultAction, int32 *remaining,
 |||	                    SPSound *sound, const char *markerName)
 |||	    {
 |||	            // stop when remaining count reaches zero
 |||	        if ((*remaining)-- <= 0) {
 |||	            return spSetStopAction (resultAction);
 |||	        }
 |||
 |||	            // do nothing (take default action) otherwise
 |||	        return 0;
 |||	    }
 |||
 |||	  Implementation
 |||
 |||	    Typedef for callback function defined in soundplayer.h V24.
 |||
 |||	  Associated Files
 |||
 |||	    soundplayer.h, music.lib
 |||
 |||	  See Also
 |||
 |||	    spSetMarkerDecisionFunction(), spSetDefaultDecisionFunction(),
 |||	    spSetBranchAction(), spSetStopAction()
 |||
 **/

/*
    Do marker action using the following path:
        Specific Decision Function => NeverMind, Branch, or Stop
        Global Decision Function => NeverMind, Branch, or Stop
        Static Action => Branch or Stop
        Continue if in Middle, Stop if at End.

    Decision function rules WRT marker management:
    (more rules in SPDecisionFunction autodoc)
        . decision function can change marker set for current sound.  for that reason
          the marker must be looked up prior to calling each decision function
        . decision function can delete current marker only if it returns
          an action that causes a branch to another marker or stop.
          Return SP_ACTION_IGNORE or pointer to self will cause the player to crash.

    @@@ could beef this function up to catch removing the current marker and then
        returning default or self-pointer
*/
static Err PerformMarkerAction (SPPlayer *player, SPMarker *marker)
{
    SPAction action;
    Err errcode;

        /* Compute an SPAction from this SPMarker (calls decision functions or uses action from marker) */
    if ((errcode = GetMarkerAction (&action, player, marker)) < 0) goto clean;

        /* process SPAction */
        /* shouldn't have to cope with SP_ACTION_IGNORE because GetMarkerAction()
           can't return it as a valid action */
    switch (action.spa_ActionCode) {
        case SP_ACTION_BRANCH:      /* branch to marker returned as action (handles branch and all stop cases) */
            {
                SPMarker * const tomarker = action.spa_BranchToMarker;

                    /* Last line of defense to see if to marker belongs to same player.
                       If not, something is seriously messed up. */
                if (tomarker && tomarker->spmk_Sound->spso_Player != player) {
                    errcode = ML_ERR_CORRUPT_DATA;
                    goto clean;
                }

                    /* follow branch */
                SetReaderStatus (player, tomarker);
            }
            break;

        default:                    /* any other code means we got mangled data */
            errcode = ML_ERR_CORRUPT_DATA;
            goto clean;
    }

    return 0;

clean:
        /* we might have no where to go if this fails. set reader status to stop just to be safe. */
    SetReaderStatus (player, NULL);
    return errcode;
}

/*
    Figure out what action take at this marker.
    Always returns an action code other than SP_ACTION_IGNORE.
*/
static Err GetMarkerAction (SPAction *action, SPPlayer *player, SPMarker *marker)
{
    int32 result;

        /* call marker's decision function */
    if ((result = CallDecisionFunction (action, player, marker->spmk_DecisionFunction,      marker->spmk_DecisionData,      marker)) != 0) return result;

        /* call default decision function */
    if ((result = CallDecisionFunction (action, player, player->sp_DefaultDecisionFunction, player->sp_DefaultDecisionData, marker)) != 0) return result;

        /* use marker's action */
    SetBranchAction (action, marker->spmk_BranchToMarker);

    return 0;
}

/*
    Call decision function, if there is one
    Returns:
        >0: there was a function and it returned something other than SP_ACTION_IGNORE.
        0: returned SP_ACTION_IGNORE or no function
        <0: error
*/
static int32 CallDecisionFunction (SPAction *action, SPPlayer *player, SPDecisionFunction decisionfunc, void *decisiondata, SPMarker *marker)
{
        /* init action */
    InitAction (action, player);

        /* call decision function if there is one, return error if function returns an error */
    if (decisionfunc) {
        Err errcode;

        if ((errcode = decisionfunc (action, decisiondata, marker->spmk_Sound, marker->spmk.n_Name)) < 0) return errcode;
    }

        /* return non-zero if action is something other than ignore */
    return action->spa_ActionCode != SP_ACTION_IGNORE;
}


/* -------------------- SPAction functions */

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spsetbranchaction
 |||	spSetBranchAction - Set up an SPAction to branch to the specified marker.
 |||
 |||	  Synopsis
 |||
 |||	    Err spSetBranchAction (SPAction *action, SPSound *toSound,
 |||	                           const char *toMarkerName)
 |||
 |||	  Description
 |||
 |||	    Sets up the SPAction to cause a branch to the specified marker as the
 |||	    resulting action of a decision function.
 |||
 |||	  Arguments
 |||
 |||	    action                      Pointer to an SPAction to fill out. This must
 |||	                                be the pointer passed to your decision function.
 |||
 |||	    toSound                     Pointer to SPSound containing marker to
 |||	                                branch to. This SPSound must belong to the same
 |||	                                SPPlayer as the SPMarker for which the decision function
 |||	                                was called.
 |||
 |||	    toMarkerName                Name of marker to in toSound branch to. All markers
 |||	                                except markers at the end of sounds are
 |||	                                considered valid "to" markers.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Outputs
 |||
 |||	    On success, fills out SPAction to cause a branch to the specified
 |||	    marker. On failure, nothing is written to the SPAction.
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
 |||	    SPDecisionFunction, spSetStopAction(), spBranchAtMarker()
 |||
 **/
Err spSetBranchAction (SPAction *action, SPSound *toSound, const char *toMarkerName)
{
    SPMarker * const tomarker = spFindMarkerName (toSound, toMarkerName);       /* can be NULL */
    Err errcode;

  #if DEBUG_ActionControl
    printf ("spSetBranchAction ($%08lx, '%s')\n", toSound, toMarkerName);
  #endif

        /* validate args */
    if (action->spa_Player != toSound->spso_Player) {
        errcode = ML_ERR_BAD_ARG;
        goto clean;
    }
    if ((errcode = ValidateToMarker (tomarker)) < 0) goto clean;

        /* set new branch */
    SetBranchAction (action, tomarker);

    return 0;

clean:
    return errcode;
}

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spsetstopaction
 |||	spSetStopAction - Set up an SPAction to stop reading.
 |||
 |||	  Synopsis
 |||
 |||	    Err spSetStopAction (SPAction *resultAction)
 |||
 |||	  Description
 |||
 |||	    Sets up the SPAction to stop reading as the resulting action
 |||	    of a decision function.
 |||
 |||	  Arguments
 |||
 |||	    action                      Pointer to an SPAction to fill out. This must
 |||	                                be the pointer passed to your decision function.
 |||
 |||	  Return Value
 |||
 |||	    Non-negative value on success; negative error code on failure.
 |||
 |||	  Outputs
 |||
 |||	    On success, fills out SPAction to cause a branch to the specified
 |||	    marker. On failure, nothing is written to the SPAction.
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
 |||	    SPDecisionFunction, spSetBranchAction(), spStopAtMarker()
 |||
 **/
Err spSetStopAction (SPAction *action)
{
    SetStopAction (action);
    return 0;
}


/*
    Initialize SPAction
*/
static void InitAction (SPAction *action, SPPlayer *player)
{
    memset (action, 0, sizeof *action);
    action->spa_Player = player;
    ClearAction (action);
}

/*
    Clear action
*/
static void ClearAction (SPAction *action)
{
    action->spa_ActionCode     = SP_ACTION_IGNORE;
}

/*
    Set branch action.
    tomarker == NULL to stop.
*/
static void SetBranchAction (SPAction *action, SPMarker *tomarker)
{
    action->spa_ActionCode     = SP_ACTION_BRANCH;
    action->spa_BranchToMarker = tomarker;
}


/* -------------------- SetReaderStatus() */

/*
    Set cursor to tomarker and update SP_READERSTATUSF_READING to reflect the change
    Marker can be one of the following:
        . a "to" marker (caller must validate as a legitimate SPMarker belonging to this SPPlayer)
          - turns on READING flag on and sets CurrentSound and Cursor to marker's position
        . NULL or an End marker - turns off READING flag and clears CurrentSound and Cursor.

    Note: The stop reading operation is performed by calling
          SetReaderStatus() w/ a NULL tomarker.
*/
static void SetReaderStatus (SPPlayer *player, const SPMarker *tomarker)
{
  #if DEBUG_PlayerControl
    printf ("SetReaderStatus($%08lx,$%08lx)\n", player, tomarker);
    printf ("  before: status=$%02x sound=$%08lx cursor=%ld\n", player->sp_ReaderStatus, player->sp_CurrentSound, player->sp_Cursor);
  #endif

        /* handle no marker or end marker as stop */
    if (tomarker && !IsEndMarker (tomarker)) {
        player->sp_ReaderStatus |= SP_READERSTATUSF_READING;
        player->sp_CurrentSound  = tomarker->spmk_Sound;
        player->sp_Cursor        = tomarker->spmk_Position;
    }
        /* handle other markers as start */
    else {
        player->sp_ReaderStatus &= ~SP_READERSTATUSF_READING;
        player->sp_CurrentSound  = NULL;
        player->sp_Cursor        = 0;
    }

        /* flush PartialFrame */
    spClearPartialFrame (player);

  #if DEBUG_PlayerControl
    printf ("   after: status=$%02x sound=$%08lx cursor=%ld\n", player->sp_ReaderStatus, player->sp_CurrentSound, player->sp_Cursor);
  #endif
}


/* -------------------- Marker lookup */

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spfindmarkername
 |||	spFindMarkerName - Return pointer an SPMarker by looking up its name.
 |||
 |||	  Synopsis
 |||
 |||	    SPMarker *spFindMarkerName (const SPSound *sound, const char *markerName)
 |||
 |||	  Description
 |||
 |||	    Locates the specified marker name in the specified sound.
 |||
 |||	  Arguments
 |||
 |||	    sound                       Pointer to SPSound to search for marker.
 |||
 |||	    markerName                  Name of marker in sound to find.
 |||
 |||	  Return Value
 |||
 |||	    Pointer to matched SPMarker on success; NULL on failure.
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
 |||	    spAddMarker(), spRemoveMarker(), SPDecisionFunction
 |||
 **/
SPMarker *spFindMarkerName (const SPSound *sound, const char *name)
{
    return (SPMarker *)FindNamedNode (&sound->spso_Markers, name);
}

/*
    Finds the next marker after the specified position.
    Returns NULL if no markers are found beyond the current position.
    Never returns a beginning marker
*/
static SPMarker *FindMarkerAfter (const SPSound *sound, uint32 position)
{
    SPMarker *marker;

    SCANLIST (&sound->spso_Markers, marker, SPMarker) {
        if (marker->spmk_Position > position) return marker;
    }

    return NULL;
}


/*
    Verifies that marker returned from spLookupMarker() can have its action or
    decision function set.
    Returns 0 if it can, or an error code if it can't.
*/
static Err ValidateFromMarker (const SPMarker *frommarker)
{
    return !frommarker ? ML_ERR_NAME_NOT_FOUND                      /* found marker? */
         : IsBeginningMarker(frommarker) ? ML_ERR_INVALID_MARKER    /* make sure frommarker isn't a beginning marker, cuz we can't take an action on a beginning marker */
         : 0;
}

/*
    Verifies that marker returned from spLookupMarker() can be branched to.
    Returns 0 if it can be, or an error code if it can't.

    Not checking for !IsEndMarker() because an end marker _is_ a legal ToMarker,
    it just causes the reader to stop.  Less code to permit it and trap it during
    marker action support than to trap it here.
*/
static Err ValidateToMarker (const SPMarker *tomarker)
{
    return !tomarker ? ML_ERR_NAME_NOT_FOUND    /* found marker? */
         : 0;
}


/* -------------------- SPSampleFrameInfo stuff */

/*
    Set a SampleFrameInfo's contents
*/
static void SetSampleFrameInfo (SPSampleFrameInfo *frameinfo, uint32 width, uint32 channels, uint32 compressionratio)
{
        /* set parameters */
    frameinfo->spfi_Width            = (uint8)width;
    frameinfo->spfi_Channels         = (uint8)channels;
    frameinfo->spfi_CompressionRatio = (uint8)compressionratio;

        /* these are dependent on the others */
        /*
            @@@ This makes the assumption about sample frame size relationship to dma granularity:

                If frame size < dma granularity, then dma granularity must be a multiple of frame size.
                If frame size > dma granularity, then frame size must be a multiple of dma granularity.

                To fix this, compute a least common multiple between SP_DMA_ALIGNMENT and frame
                size for AlignmentBytes (in bits if it helps) and then compute AlignmentBytes and
                AlignmentFrames from this.

                See Caveats in spCreatePlayer(), spAddSoundFile(), and spAddSample().
        */
    frameinfo->spfi_AlignmentFrames  = MAX (spCvtByteToFrame (frameinfo, SP_DMA_ALIGNMENT), 1);
    frameinfo->spfi_AlignmentBytes   = spCvtFrameToByte (frameinfo, frameinfo->spfi_AlignmentFrames);
}

/*
    Returns zero if identical, non-zero otherwise
*/
static int CompareSampleFrameInfo (const SPSampleFrameInfo *frameinfo1, const SPSampleFrameInfo *frameinfo2)
{
    return frameinfo1->spfi_Width            != frameinfo2->spfi_Width ||
           frameinfo1->spfi_Channels         != frameinfo2->spfi_Channels ||
           frameinfo1->spfi_CompressionRatio != frameinfo2->spfi_CompressionRatio;
}

/*
    Check frame alignment of pos
*/
static Err CheckSampleFrameAlignment (const SPSampleFrameInfo *frameinfo, uint32 framenum)
{
    return framenum % frameinfo->spfi_AlignmentFrames
        ? ML_ERR_BAD_SAMPLE_ALIGNMENT
        : 0;
}


/* -------------------- Internal list goodies */

/*
    Allocate a Node and space for it's name.
    The only reason this function can fail is for lack of memory.
*/
static Node *AllocNamedNode (size_t structsize, const char *name)
{
    const size_t allocsize = structsize + strlen(name) + 1;
    Node *node = NULL;

        /* alloc space for structsize + name */
    if ((node = (Node *)AllocMem (allocsize, MEMTYPE_ANY | MEMTYPE_FILL)) == NULL) goto clean;

        /* init node fields */
    strcpy (node->n_Name = (char *)node + structsize, name);
    node->n_Size = allocsize;

    return node;

clean:
    FreeNamedNode (node);
    return NULL;
}

/*
    Free a Node allocated with AllocNamedNode()
    node can be NULL.
*/
static void FreeNamedNode (Node *node)
{
        /* must test non-NULL here since it looks at node->n_Size */
    if (node) FreeMem (node, node->n_Size);
}


/* -------------------- Debug stuff */

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spdumpplayer
 |||	spDumpPlayer - Print debug information for sound player
 |||
 |||	  Synopsis
 |||
 |||	    void spDumpPlayer (const SPPlayer *player)
 |||
 |||	  Description
 |||
 |||	    Prints out a bunch of debug information for an SPPlayer including:
 |||	        . list of the SPPlayer's SPSounds
 |||	        . list of each SPSound's SPMarkers
 |||	        . cross reference of static branches between markers
 |||
 |||	  Arguments
 |||
 |||	    player                      Pointer to SPPlayer to print.
 |||
 |||	  Return Value
 |||
 |||	    None
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
 |||	    spCreatePlayer()
 |||
 **/
void spDumpPlayer (const SPPlayer *player)
{
    const SPSound *sound;
    const SPMarker *marker;
    const MinNode *refnode;

    printf ("SPPlayer $%08lx width=%lu channels=%lu compression=%lu alignment=%lu frames %lu bytes. xref:\n", player, player->sp_SampleFrameInfo.spfi_Width, player->sp_SampleFrameInfo.spfi_Channels, player->sp_SampleFrameInfo.spfi_CompressionRatio, player->sp_SampleFrameInfo.spfi_AlignmentFrames, player->sp_SampleFrameInfo.spfi_AlignmentBytes);

    SCANLIST (&player->sp_Sounds, sound, SPSound) {
        printf ("  SPSound $%08lx: numframes=%ld numbytes=%ld class=$%08lx\n", sound, sound->spso_NumFrames, spCvtFrameToByte(&sound->spso_SampleFrameInfo,sound->spso_NumFrames), sound->spso_ClassDefinition);

      #if 0
            /* done this way to avoid debug code being a method, since that would cause the debug code to
               always be linked in */
        if (sound->spso_ClassDefinition == &sp_soundFileClass) {
            printf ("  fileitem=%ld offset=%ld blocksize=%ld\n",
                ((SPSoundFile *)sound)->spsf_File, ((SPSoundFile *)sound)->spsf_DataOffset, ((SPSoundFile *)sound)->spsf_BlockSize );
        }
        else if (sound->spso_ClassDefinition == &sp_soundSampleClass) {
            printf ("  address=$%08lx\n", ((SPSoundSample *)sound)->spss_DataAddress);
        }
      #endif

        SCANLIST (&sound->spso_Markers, marker, SPMarker) {
            printf ("    SPMarker $%08lx: pos=%lu tomarker=$%08lx decision=$%08lx,$%08lx name='%s'\n", marker, marker->spmk_Position, marker->spmk_BranchToMarker, marker->spmk_DecisionFunction, marker->spmk_DecisionData, marker->spmk.n_Name);

            SCANLIST (&marker->spmk_BranchRefList, refnode, MinNode) {
                const SPMarker *refmarker = GetMarkerFromBranchRefNode (refnode);
                printf ("      ref: marker=$%08lx sound=$%08lx\n", refmarker, refmarker->spmk_Sound);
            }
        }
    }
}
