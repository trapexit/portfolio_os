/******************************************************************************
**
**  $Id: soundplayer_internal.h,v 1.30 1994/10/17 23:14:28 peabody Exp $
**
**  Advanced Sound Player - internal include file.
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
**  940818 WJB  Updated temp error codes.
**  940824 WJB  Tweaked debug.
**  940826 WJB  Added minimum spooler length stuff.
**  940902 WJB  Removed sp_SignalMask from SPPlayer.
**  940915 WJB  Removed ML_ERR_INVALID_FILE_FORMAT.
**  940915 WJB  Published new ML_ERR_ codes.
**  940916 WJB  Added a triple bang re SP_DMA_ALIGNMENT.
**  940922 WJB  Replaced DEBUG_DMAAlignment with DEBUG_SampleAlignment.
**  940922 WJB  Added protection to prevent spRemoveMarker() from deleting begin and end markers.
**  940927 WJB  Changed allocation functions from returning pointer to returning error code.
**  941003 WJB  Replaced SPAction system with a private structure.
**  941007 WJB  Processed some triple-bangs.
**  941007 WJB  Converted some SPSampleFrameInfo fields to uint8.
**  941007 WJB  Converted sp_NumBuffers to uint8.
**  941007 WJB  Converted SPSound to use a MinNode instead of a Node.
**  941010 WJB  Moved cursor advancement code from ReadSoundMethods to ReadSoundData().
**  941017 WJB  Added SPPlayer pointer in SPAction.
**  941017 WJB  Added #error for EXTERNAL_RELEASE.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#ifdef EXTERNAL_RELEASE
  #error This is an internal include file.
#endif

#ifndef __SOUNDPLAYER_INTERNAL_H
#define __SOUNDPLAYER_INTERNAL_H

#pragma force_top_level
#pragma include_only_once

#include <list.h>               /* List and Node */
#include <musicerror.h>         /* ML_ERR_ */
#include <soundplayer.h>
#include <soundspooler.h>       /* SoundSpooler structure */
#include <stdio.h>              /* printf() */
#include <types.h>


/* -------------------- Debugging control */

#define DEBUG_Create            0   /* Create/Delete functions */
#define DEBUG_SampleAlignment   0   /* print alignment error message */
#define DEBUG_ActionControl     0
#define DEBUG_PlayerControl     0   /* start/stop/status */
#define DEBUG_FillSpooler       0   /* note: using this causes a huge performance hit */
#define DEBUG_FillSpoolerShort  0   /* short form of above */
#define DEBUG_MungFillSpooler   0   /* trash entire buffer before reading, trash scratch areas after reading */


/* -------------------- General Macros (@@@ could publish these some day) */

#define MAX(a,b)    ((a)>(b)?(a):(b))
#define MIN(a,b)    ((a)<(b)?(a):(b))

#define UFLOOR(n,d) ((n) / (d))                 /* unsigned int floor() */
#define UCEIL(n,d)  UFLOOR ((n) + (d)-1, (d))   /* unsigned int ceil() */
#define UROUND(n,d) UFLOOR ((n) + (d)/2, (d))   /* unsigned int round() */


/* -------------------- audio rules */

    /* DMA */
#define SP_DMA_ALIGNMENT    4                       /* DMA alignment in bytes (must be a power of 2) (!!! this is magic and is likely to vanish in favor of a GetAudioFolioInfo() tag) */

#define SP_DMA_MASK         ~(SP_DMA_ALIGNMENT-1)   /* mask to use to force DMA alignment on a pointer or length */

    /* Spooler */
#define SP_SPOOLER_MIN_LENGTH 512                   /* Recommended minimum amount to send to spooler to guarantee
                                                       smooth playback to subsequent buffers */


/* -------------------- Structures (private) */

    /* SPSampleFrameInfo - Information about sample frame */
typedef struct SPSampleFrameInfo {
        /* basic sound parameters */
    uint8   spfi_Width;                 /* bytes per uncompressed sample */
    uint8   spfi_Channels;              /* channels per frame */
    uint8   spfi_CompressionRatio;      /* compression ratio */
    uint8   spfi_pad0;

        /* derived stuff */
    uint32  spfi_AlignmentFrames;       /* Number of sample frames that fit into a DMA frame (>=1).
                                           Composite of DMA and sample frame alignment rules expressed in frames.
                                           Cursor and all marker positions are multiples of this. */
    uint32  spfi_AlignmentBytes;        /* Number of bytes for composite DMA and sample frame alignment */
} SPSampleFrameInfo;

    /* compression < framesize:  get first byte in frame
       compression >= framesize: byte containing frame */
#define spCvtFrameToByte(frameInfo,frameNum) ((frameNum) * ((uint32)(frameInfo)->spfi_Width * (frameInfo)->spfi_Channels) / (uint32)(frameInfo)->spfi_CompressionRatio)
    /* compression >= framesize: get first frame in byte
       compression < framesize:  frame containing byte */
#define spCvtByteToFrame(frameInfo,byteNum)  ((byteNum) * (uint32)(frameInfo)->spfi_CompressionRatio / ((uint32)(frameInfo)->spfi_Width * (frameInfo)->spfi_Channels))

    /* SPPlayer - main control structure */
struct SPPlayer {
        /* reader */
    List    sp_Sounds;                  /* List of sounds */

                                        /* Frame description for all sounds in sp_Sounds list (valid only when sp_Sounds isn't empty) */
    SPSampleFrameInfo sp_SampleFrameInfo;

    SPSound *sp_CurrentSound;           /* current SPSound being read (valid only when SP_READERSTATUSF_READING is set) */
    uint32  sp_Cursor;                  /* current frame position (next frame to spool) in sp_CurrentSound (valid only when SP_READERSTATUSF_READING is set) */

    uint8   sp_ReaderStatus;            /* current status of reader (SP_READERSTATUSF_ flags) */
    uint8   sp_pad1[3];

                                        /* Default decision function for all markers (or NULL if none) */
    SPDecisionFunction sp_DefaultDecisionFunction;
    void   *sp_DefaultDecisionData;     /* Data passed to default decision function */

        /* spooler stuff */
    SoundSpooler *sp_Spooler;           /* SoundSpooler */
    void  **sp_BufferArray;             /* pointer to buffer array */
    uint32  sp_BufferSize;              /* size of each buffer in bytes */
    uint8   sp_NumBuffers;              /* number of buffers */
    uint8   sp_BufferFlags;             /* SP_BUFFERF_ */
    uint8   sp_pad2[2];
    void   *sp_PartialFrameAddr;        /* Pointer into last spooled buffer of beginning of frame that spanned last block read.
                                           This is used to defer partial sample/dma frames until the next buffer is spooled.
                                           Only valid when sp_PartialFrameLen is non-0.
                                           Gets cleared whenever a branch is followed or reader status changes. */
    uint32  sp_PartialFrameLen;         /* # of bytes pointed to by sp_PartialFrameAddr (0..spfi_AlignmentBytes-1) */
};

    /* sp_ReaderStatus flags */
#define SP_READERSTATUSF_READING    0x01    /* 1: reading.  0: not reading */

    /* sp_BufferFlags */
#define SP_BUFFERF_ALLOCATED        0x01    /* 1: allocated by spCreatePlayer(), spDeletePlayer must free.
                                               0: client-supplied, don't free. */


    /* SPSound - Sound belonging to Player */
struct SPSound {
        /* linkage */
    MinNode spso;                       /* node in sp_Sounds list. */
    SPPlayer *spso_Player;              /* link back to player */

        /* sound data */
                                        /* description of sample frame for sound */
    SPSampleFrameInfo spso_SampleFrameInfo;
    uint32  spso_NumFrames;             /* # of frames in sound */
    List    spso_Markers;               /* list of SPMarkers sorted by position (always includes markers for beginning and end of sound) */

        /* class definition */
    const struct SPSoundClassDefinition *spso_ClassDefinition;
};


    /* SPMarker - Marker in a Sound */
struct SPMarker {
        /* linkage to Sound */
    Node spmk;                          /* node in spso_Markers list. n_Name is name, n_Size is sizeof (SPMarker) + name */
    SPSound *spmk_Sound;                /* link back to Sound */

        /* Marker info */
    uint32  spmk_Position;              /* marker's position in sound data in frames
                                           (falls between a pair of sample frames,
                                           0: before first frame, nframes: after last frame.)
                                           Note: must be DMA aligned */

                                        /* Function to call when reading reaches this marker or NULL */
    SPDecisionFunction spmk_DecisionFunction;
    void   *spmk_DecisionData;

    SPMarker *spmk_BranchToMarker;      /* Pointer to another SPMarker to branch to or:
                                            . NULL - stop reader.
                                            . Pointer to self - continue reading from here if not at end
                                              of sound, or stop if at end of sound.
                                           See spmk_BranchRefNode below.
                                        */
    uint8   spmk_MarkerFlags;           /* SP_MARKERF_ */
    uint8   spmk_pad1[3];

        /* marker cross referencing */
    List spmk_BranchRefList;            /* List of spmk_BranchRefNode's for Markers that branch to this one. */
    MinNode spmk_BranchRefNode;         /* This node is added to spmk_BranchToMarker->spmk_BranchRefList
                                           when spmk_BranchToMarker is non-NULL (including self-referential case).
                                           Not in any list when spmk_BranchToMarker is NULL.
                                        */
};

    /* spmk_MarkerFlags */
#define SP_MARKERF_PERMANENT    0x01    /* 1: Permanent markers associated with the sound
                                              (e.g. beginning and end).
                                              Client can't modify or delete these.
                                           0: Markers created by client w/ spAddMarker().
                                              Client can delete these.
                                        */


/* -------------------- Sound Class Definition */

    /* method prototypes */
typedef Err (*SPReadSoundMethod) (SPSound *, uint32 cursorByte, uint32 remBytes, char *bufAddr, uint32 bufSize, bool optimized, char **useAddrP, uint32 *useLenP, uint32 *nextPartialLenP);
typedef void (*SPDeleteSoundMethod) (SPSound *);

    /* class definition structure */
typedef struct SPSoundClassDefinition {
    uint32  spsc_StructSize;            /* size of data structure (saves an arg to AllocSound()) */
    SPDeleteSoundMethod spsc_Delete;    /* delete method (optional) */
    SPReadSoundMethod   spsc_Read;      /* read method (required) */
} SPSoundClassDefinition;


/* -------------------- Actions (internal definitions) */

/*
    Decision functions fill these out by calling a set of functions that
    act on them (e.g. spSetBranchAction(), spSetStopAction()).
*/

struct SPAction {
        /* linkage */
    SPPlayer *spa_Player;               /* Player that owns this SPAction */

        /* action variables */
    uint32  spa_ActionCode;             /* SP_ACTION_ code below */
    SPMarker *spa_BranchToMarker;       /* Data for SP_ACTION_BRANCH: Pointer to SPMarker
                                           to branch to, or NULL or pointer to end marker
                                           to stop reader. */
};

enum {                                  /* spa_ActionCode values */
    SP_ACTION_IGNORE,                   /* Act as if decision function was never called. spa_BranchToMarker is ignored. */
    SP_ACTION_BRANCH                    /* Branch to spa_BranchToMarker (or stop if
                                           spa_BranchToMarker is NULL or end marker) */
};


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* Alloc/Free/Add sound */
Err spAllocSound (SPSound **resultSound, SPPlayer *, const SPSoundClassDefinition *, uint32 numFrames, uint32 width, uint32 channels, uint32 compressionRatio);
void spAddSound (SPSound *);
void spFreeSound (SPSound *);

    /* compute minimum legal buffer size based on spooler limitations and SampleFrameInfo */
#define spMinBufferSize(frameInfo)          MAX (SP_SPOOLER_MIN_LENGTH, (frameInfo)->spfi_AlignmentBytes)

    /* reader status */
#define spSetPartialFrame(player,addr,len)  do { (player)->sp_PartialFrameAddr = (addr); (player)->sp_PartialFrameLen = (len); } while(0)
#define spClearPartialFrame(player)         spSetPartialFrame ((player),NULL,0)

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __SOUNDPLAYER_INTERNAL_H */
