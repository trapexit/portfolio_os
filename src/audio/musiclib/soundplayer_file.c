/******************************************************************************
**
**  $Id: soundplayer_file.c,v 1.24 1994/10/14 23:47:45 peabody Exp $
**
**  Advanced Sound Player - sound file class.
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
**  940818 WJB  Replaced marker index system with marker name system.
**              Added support for INST chunk in AIFF parser.
**  940819 WJB  Added some debug code.
**  940826 WJB  Added minimum buffer size trap based on block size.
**  940901 WJB  Added trap for readlen less than blocksize.
**  940913 WJB  Improved AIFF reader.
**  940915 WJB  Improved AIFF reader some more.
**  940915 WJB  Now expecting DoIO() to check io_Error.
**  940922 WJB  Removed redundant non-NULL tests before calling FreeMem().
**  940922 WJB  Added error handler for non-DMA-aligned markers and lengths.
**  940927 WJB  Changed allocation functions from returning pointer to returning error code.
**  941007 WJB  Added caveats re sample frame size.
**  941010 WJB  Moved cursor advancement code from ReadSoundMethods to ReadSoundData().
**  941014 WJB  Replaced all int16's with 8-bit arrays (packed_uint16, etc).
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**
******************************************************************************/

#include <audio.h>
#include <filesystem.h>
#include <filefunctions.h>
#include <iff_fs_tools.h>       /* for AIFF internals */
#include <limits.h>             /* UCHAR_MAX */
#include <mem.h>                /* AllocMem() */

#include "soundplayer_internal.h"


/* -------------------- Debug */

#define DEBUG_AIFF      0       /* AIFF parser */


/* -------------------- Version control */

#define VERS_NewDoIO    1       /* Set: DoIO() returns value io_Error (V24 feature)
                                   Clear: caller of DoIO() must look at io_Error explicitly. */


/* -------------------- Class definition */

    /* file class */
typedef struct SPSoundFile {
    SPSound spsf;                       /* Base class */
    Item    spsf_File;                  /* File to read from */
    Item    spsf_IOReq;                 /* IOReq to use w/ spso_File */
    uint32  spsf_DataOffset;            /* Byte offset in file where sound data begins. */
    uint32  spsf_BlockSize;             /* Block size of file */
} SPSoundFile;

static void DeleteSoundFile (SPSoundFile *);
static Err ReadSoundFile (SPSoundFile *, uint32 cursorbyte, uint32 rembytes, char *bufaddr, uint32 bufsize, bool optimized, char **useaddrp, uint32 *uselenp, uint32 *nextpartiallenp);
static const SPSoundClassDefinition SoundFileClass = {
    sizeof (SPSoundFile),
    (SPDeleteSoundMethod)DeleteSoundFile,
    (SPReadSoundMethod)ReadSoundFile,
};


/* -------------------- pstring stuff (for AIFF) */

#define PSTR_MAX_LENGTH UCHAR_MAX

    /* functions */
#define pstrlen(s)  ( (size_t) *(const uchar *)(s) )
#define pstraddr(s) ( (char *) ((s) + 1) )
#define pstrsize(s) ( (pstrlen(s)+1+1) & ~1 )
char *ptocstr (char *dest, const char *src, size_t destsize);


/* -------------------- AIFF definitions */

    /* AIFF chunk names */
#define ID_AIFF MAKE_ID('A','I','F','F')
#define ID_AIFC MAKE_ID('A','I','F','C')
#define ID_APPL MAKE_ID('A','P','P','L')
#define ID_COMM MAKE_ID('C','O','M','M')
#define ID_FVER MAKE_ID('F','V','E','R')
#define ID_INST MAKE_ID('I','N','S','T')
#define ID_MARK MAKE_ID('M','A','R','K')
#define ID_SSND MAKE_ID('S','S','N','D')

    /* packed long stuff */
    /* @@@ this could be extended to handle extracting 16-bit values from bytes if necessary */
typedef uint8 packed_uint16[2];
typedef uint8 packed_int16[2];
typedef uint8 packed_uint32[4];
typedef uint8 packed_int32[4];
#define UNPACK_UINT16(data) ((uint32)(data)[0] << 8 | (data)[1])
#define UNPACK_INT16(data)  ((int32)(int8)(data)[0] << 8 | (data)[1])
#define UNPACK_UINT32(data) ((((uint32)(data)[0] << 8 | (data)[1]) << 8 | (data)[2]) << 8 | (data)[3])
#define UNPACK_INT32(data)  ((((int32)(int8)(data)[0] << 8 | (data)[1]) << 8 | (data)[2]) << 8 | (data)[3])

    /* AIFF structures (some are packed: uses arrays of words instead of longs where they aren't aligned) */

    /* packed COMM chunk (has non-long-aligned longs) */
typedef struct AIFFPackedCommon {
    packed_uint16   commx_NumChannels;      /* # of audio channels */
    packed_uint32   commx_NumSampleFrames;  /* # of samples per channel */
    packed_uint16   commx_SampleSize;       /* # of bits per sample */
    uint8           commx_SampleRate[10];   /* sample frames per second (ieee 80-bit format) */
    packed_uint32   commx_CompressionType;  /* Compression type ID */
    /* compression name follows */
} AIFFPackedCommon;


    /* packed loop 'structure' contained in an INST chunk */
typedef packed_uint16 AIFFPackedLoop[3];

enum {      /* AIFF loop play modes for alop_PlayMode values */
    AIFFLOOPT_NONE,
    AIFFLOOPT_FORWARD,
    AIFFLOOPT_BACKWARD_FORWARD
};

    /* packed INST chunk (has non-long-aligned structures) */
typedef struct AIFFPackedInstrument {
    uint8           instx_BaseNote;
    int8            instx_Detune;
    uint8           instx_LowNote;
    uint8           instx_HighNote;
    uint8           instx_LowVelocity;
    uint8           instx_HighVelocity;
    packed_int16    instx_Gain;
    AIFFPackedLoop  instx_SustainLoop;
    AIFFPackedLoop  instx_ReleaseLoop;
} AIFFPackedInstrument;


    /* packed marker structure */
typedef struct AIFFPackedMarker {
    packed_uint16   markx_ID;               /* unique 16-bit ID of marker */
    packed_uint32   markx_Position;         /* frame number of marker */
    char            markx_Name[1];          /* Name of marker (pstring) */
} AIFFPackedMarker;

#define AIFF_PACKED_MARKER_MIN_SIZE offsetof (AIFFPackedMarker, markx_Name[1])
#define AIFFPackedMarkerSize(markx) (offsetof (AIFFPackedMarker, markx_Name) + pstrsize ((markx)->markx_Name))


    /* header of SSND chunk, followed by sound data */
typedef struct AIFFSoundDataHeader {
    uint32  ssnd_Offset;
    uint32  ssnd_BlockSize;
} AIFFSoundDataHeader;


/* -------------------- ScanAIFF() stuff */

    /* unpacked loop structure from INST */
typedef struct AIFFLoop {
    uint32  alop_PlayMode;          /* AIFFLOOPT_ below */
    uint32  alop_BeginMarker;       /* marker IDs of beginning/end of loop, when PlayMode is a value other than AIFFLOOPT_NONE */
    uint32  alop_EndMarker;
} AIFFLoop;

typedef struct AIFFInfo
{
        /* FORM stuff */
    uint32 ainf_FormID;             /* FORM ID for sound (useful for determining whether or not it should consider compression */

        /* sound parameters (from COMM) */
    uint32 ainf_NumFrames;          /* # of frames in sound (uncompressed) */
    uint32 ainf_Width;              /* # of bytes per sample (uncompressed) */
    uint32 ainf_Channels;           /* # of channels per frame */
    uint32 ainf_CompressionType;    /* ID of compression type - guaranteed to be 0x0 for FORM AIFF */
    uint32 ainf_CompressionRatio;   /* compression ratio (>= 1) */

        /* loop info (from INST) */
    AIFFLoop ainf_SustainLoop;
    AIFFLoop ainf_ReleaseLoop;

        /* Markers (from MARK) */
    uint32 ainf_NumMarkers;         /* # of markers in ainf_MarkerBuf */
    void  *ainf_MarkerBuf;          /* packed marker array from MARK chunk */
    uint32 ainf_MarkerBufSize;      /* size of packed marker array */

        /* Sound data location in file (from SSND) */
    uint32 ainf_DataOffset;         /* Byte offset in file where data actually starts */
    uint32 ainf_NumBytes;           /* Number of bytes of sound data actually in the file (compressed) */
} AIFFInfo;

static Err ScanAIFF (AIFFInfo **resultAIFFInfo, const char *filename);
static void FreeAIFFInfo (AIFFInfo *);


/* -------------------- Interesting macros */

    /*
        Determines the maximum number of blocks that the requested number
        of bytes would span. (e.g. 2 bytes could span 2 blocks, but 1 byte could only span 1)
    */
#define MAXSPANBLOCKS(readsize,blocksize) UCEIL ((readsize) + (blocksize)-1, (blocksize))


/* -------------------- Local functions */

    /* file support */
static int32 GetFileBlockSize (Item fileioreq);
static Err ReadFile (Item fileioreq, uint32 blocknum, void *readaddr, uint32 readlen);


/* -------------------- Create/Delete */

static const char *GetSpecialMarkerName (const AIFFInfo *ainfo, uint32 id);

 /**
 |||	AUTODOC PUBLIC mpg/musiclib/soundplayer/spaddsoundfile
 |||	spAddSoundFile - Create an SPSound for an AIFF sound file.
 |||
 |||	  Synopsis
 |||
 |||	    Err spAddSoundFile (SPSound **resultSound, SPPlayer *player,
 |||	                        const char *fileName)
 |||
 |||	  Description
 |||
 |||	    Creates an SPSound for the specified AIFF sound file and adds it to the
 |||	    specified player. SPSounds created this way cause the player to spool
 |||	    the sound data directly off of disc instead of buffering the whole sound
 |||	    in memory. This is useful for playing back really long sounds.
 |||
 |||	    This function opens the specified AIFF file and scans collects its
 |||	    properties (e.g. number of channels, size of frame, number of frames,
 |||	    markers, etc). The sound is checked for sample frame formatting compatibility
 |||	    with the other SPSounds in the SPPlayer and for buffer size compatibility.
 |||	    A mismatch causes an error to be returned.
 |||
 |||	    Once that is done, all of the markers from the AIFF file are translated into
 |||	    SPMarkers. Additionally the following special markers are created:
 |||
 |||	        SP_MARKER_NAME_BEGIN         - set to the beginning of the sound data
 |||
 |||	        SP_MARKER_NAME_END           - set to the end of the sound data
 |||
 |||	        SP_MARKER_NAME_SUSTAIN_BEGIN - set to the beginning of the sustain loop
 |||	                                       if the sound file has a sustain loop.
 |||
 |||	        SP_MARKER_NAME_SUSTAIN_END   - set to the end of the sustain loop
 |||	                                       if the sound file has a sustain loop.
 |||
 |||	        SP_MARKER_NAME_RELEASE_BEGIN - set to the beginning of the release loop
 |||	                                       if the sound file has a release loop.
 |||
 |||	        SP_MARKER_NAME_RELEASE_END   - set to the end of the release loop
 |||	                                       if the sound file has a release loop.
 |||
 |||	    The file is left open for the entire life of this type of SPSound for
 |||	    later reading by the player.
 |||
 |||	    The length of the sound file and all of its markers must be DMA-aligned
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
 |||	    fileName                    Name of an AIFF file to read.
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
 |||	    SoundDesigner II has several classes of markers that it supports in sound:
 |||	    loop, numeric, and text. When it saves to an AIFF file, it silently throws away
 |||	    all but the first 2 loops that may be in the edited sound. Numeric markers are
 |||	    written to an AIFF file with a leading "# " which SDII apparently uses to
 |||	    recognize numeric markers when reading an AIFF file. It unfortunately ignores the
 |||	    rest of the marker name in that case, making the actual numbers somewhat variable.
 |||	    Text markers, thankfully, have user editable names that are saved verbatim in an
 |||	    AIFF file. For this reason, we recommend using only text markers (and possibly loops 1
 |||	    and 2) when preparing AIFF files for use with the sound player in SoundDesigner II.
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
 |||	    spRemoveSound(), spAddSample()
 |||
 **/
Err spAddSoundFile (SPSound **resultSound, SPPlayer *player, const char *filename)
{
    AIFFInfo *ainfo = NULL;
    SPSoundFile *sound = NULL;
    Err errcode;

  #if DEBUG_Create
    printf ("spAddSoundFile() '%s'\n", filename);
  #endif

        /* initialize result (must be done first) */
    if (!resultSound) return ML_ERR_BADPTR;
    *resultSound = NULL;

        /* scan file */
    if ( (errcode = ScanAIFF (&ainfo, filename)) < 0 ) goto clean;

        /* alloc/init SPSound */
    if ((errcode = spAllocSound ((SPSound **)&sound, player, &SoundFileClass, ainfo->ainf_NumFrames, ainfo->ainf_Width, ainfo->ainf_Channels, ainfo->ainf_CompressionRatio)) < 0) goto clean;
    sound->spsf_DataOffset = ainfo->ainf_DataOffset;

        /* add markers from file */
    {
        const AIFFPackedMarker *markx = (AIFFPackedMarker *)ainfo->ainf_MarkerBuf;    /* always winds up being 16-bit aligned because of pstring word alignment rules */
        uint32 rembytes = ainfo->ainf_MarkerBufSize;
        uint32 nmarkers = ainfo->ainf_NumMarkers;
        uint32 markersize;

        while (nmarkers--) {

          #if DEBUG_AIFF
                /* note: this can overindex memory here (before rembytes check below),
                   but this is debug code so it shouldn't matter */
            printf ("MARK: markx=$%08lx rembytes=%lu id=%lu pos=%lu namelen=%lu size=%lu($%lx)\n",
                markx, rembytes,
                UNPACK_UINT16 (markx->markx_ID),
                UNPACK_UINT32 (markx->markx_Position),
                pstrlen (markx->markx_Name),
                AIFFPackedMarkerSize(markx), AIFFPackedMarkerSize(markx));
          #endif

                /* validate that there's enough data left for an AIFFPackedMarker */
            if (rembytes < AIFF_PACKED_MARKER_MIN_SIZE) {
                errcode = ML_ERR_BAD_FORMAT;
                goto clean;
            }

                /* get marker size and verify there's enough bytes left in */
            markersize = AIFFPackedMarkerSize(markx);
            if (rembytes < markersize) {
                errcode = ML_ERR_BAD_FORMAT;
                goto clean;
            }

            {
                const uint32 pos = UNPACK_UINT32 (markx->markx_Position);
                char namebuf [PSTR_MAX_LENGTH + 1];
                const char *name;

                    /* look up name */
                if ((name = GetSpecialMarkerName (ainfo, UNPACK_UINT16(markx->markx_ID))) == NULL)
                    name = ptocstr (namebuf, markx->markx_Name, sizeof namebuf);

                    /* create marker */
                if ((errcode = spAddMarker ((SPSound *)sound, pos, name)) < 0) goto clean;
            }

            markx = (AIFFPackedMarker *)(((char *)markx) + markersize);
            rembytes -= markersize;
        }
    }

        /* open sound file and create i/o request */
    {
        int32 result;

        if ( (result = OpenDiskFile ((char *)filename)) < 0 ) {     /* @@@ shouldn't need this cast */
            errcode = result;
            goto clean;
        }
        sound->spsf_File = (Item)result;

        if ( (result = CreateIOReq ((char *)filename, 0, sound->spsf_File, 0)) < 0 ) {  /* @@@ shouldn't need this cast */
            errcode = result;
            goto clean;
        }
        sound->spsf_IOReq = (Item)result;

        if ( (result = GetFileBlockSize (sound->spsf_IOReq)) < 0 ) {
            errcode = result;
            goto clean;
        }
        sound->spsf_BlockSize = (uint32)result;
    }

        /* test player's buffer size against minimum buffer size to support
           file's blocksize (depends on SampleInfo and blocksize) */
    {
        const uint32 reqdbufsize =
            spMinBufferSize (&sound->spsf.spso_SampleFrameInfo)-1 +     /* nearly enough space to hold a sample frame or min required for spooler */
            SP_DMA_ALIGNMENT-1 +                                        /* enough space to offset reading */
                                                                        /* enough space to read blocks necessary to read a frame */
            MAXSPANBLOCKS (sound->spsf.spso_SampleFrameInfo.spfi_AlignmentBytes, sound->spsf_BlockSize) * sound->spsf_BlockSize;

      #if DEBUG_Create
        printf ("  minbufsize=%lu\n", reqdbufsize);
      #endif

        if (player->sp_BufferSize < reqdbufsize) {
            errcode = ML_ERR_BUFFER_TOO_SMALL;
            goto clean;
        }
    }

  #if DEBUG_Create
    printf ("  file=%ld ioreq=%ld dataoffset=%lu($%lx) blocksize=%lu($%lx)\n",
        sound->spsf_File, sound->spsf_IOReq, sound->spsf_DataOffset, sound->spsf_DataOffset, sound->spsf_BlockSize, sound->spsf_BlockSize);
  #endif

        /* success: add to player's sounds list and set result */
    spAddSound ((SPSound *)sound);
    *resultSound = (SPSound *)sound;

clean:
        /* normal clean up */
    FreeAIFFInfo (ainfo);

        /* additional clean up for failure */
    if (errcode < 0) {
        spFreeSound ((SPSound *)sound);
    }

    return errcode;
}

/*
    Returns special name for marker ID if there is one. Otherwise returns NULL.
*/
static const char *GetSpecialMarkerName (const AIFFInfo *ainfo, uint32 id)
{
    if (ainfo->ainf_SustainLoop.alop_PlayMode) {
        if (ainfo->ainf_SustainLoop.alop_BeginMarker == id) return SP_MARKER_NAME_SUSTAIN_BEGIN;
        if (ainfo->ainf_SustainLoop.alop_EndMarker == id)   return SP_MARKER_NAME_SUSTAIN_END;
    }
    if (ainfo->ainf_ReleaseLoop.alop_PlayMode) {
        if (ainfo->ainf_ReleaseLoop.alop_BeginMarker == id) return SP_MARKER_NAME_RELEASE_BEGIN;
        if (ainfo->ainf_ReleaseLoop.alop_EndMarker == id)   return SP_MARKER_NAME_RELEASE_END;
    }
    return NULL;
}


/*
    SoundFileClass delete method.
    Closes file.
    sound is never NULL.
*/
static void DeleteSoundFile (SPSoundFile *sound)
{
  #if DEBUG_Create
    printf ("DeleteSoundFile() fileitem=%ld\n", sound->spsf_File);
  #endif

        /* close file */
    DeleteIOReq (sound->spsf_IOReq);
    CloseDiskFile (sound->spsf_File);
}


/* -------------------- Read */

/*
    SoundFileClass read method
*/
static Err ReadSoundFile (SPSoundFile *sound, uint32 cursorbyte, uint32 rembytes, char *bufaddr, uint32 bufsize, bool optimized, char **useaddrp, uint32 *uselenp, uint32 *nextpartiallenp)
{
    SPPlayer * const player = sound->spsf.spso_Player;
    uint32 filepos, fileblocknum, fileblockoffset;
    uint32 readoffset, readlen;
    uint32 validlen;
    uint32 useoffset, uselen;
    uint32 nextpartiallen;
    Err errcode;

        /* find data in file */
    filepos         = sound->spsf_DataOffset + cursorbyte;
    fileblocknum    = filepos / sound->spsf_BlockSize;
    fileblockoffset = filepos % sound->spsf_BlockSize;

        /*
            Trap misalignment

            Only one of PartialFrameLen and fileblockoffset can be non-zero
            because PartialFrameLen is the amount left over from the previous
            block. If adding it to the current cursor doesn't advance us to
            an even block, there's an error.
        */
    if (player->sp_PartialFrameLen && fileblockoffset) {
        errcode = ML_ERR_CORRUPT_DATA;
        goto clean;
    }

        /*
            If partiallen == 0: first byte we care about in block needs to be DMA aligned.
            Find a useoffset that is DMA aligned, then figure out readoffset

            If partiallen > 0: partial gets copied to beginning of buffer and block
            should be appended to it (block offset must be 0)
        */
    useoffset = (fileblockoffset + SP_DMA_ALIGNMENT - 1) & SP_DMA_MASK;
    readoffset = useoffset - fileblockoffset + player->sp_PartialFrameLen;

        /* compute part of buffer to read into (bound by read offset and # of blocks we need and can fit) */
    readlen = MIN ( UCEIL (fileblockoffset + rembytes, sound->spsf_BlockSize),  /* # of blocks we need */
                    UFLOOR (bufsize - readoffset, sound->spsf_BlockSize)        /* # of blocks that can fit into buffer */
                  ) * sound->spsf_BlockSize;

  #if DEBUG_FillSpooler
    printf ("    file: pos=%lu block=%lu offset=%lu\n", filepos, fileblocknum, fileblockoffset);
    printf ("    read: offset=%lu len=%lu\n", readoffset, readlen);
  #endif

        /* check for readlen being less than a block (could only happen if bufsize
           was too small, so shouldn't actually need to succeed for this case) */
    if (readlen < sound->spsf_BlockSize) {
        errcode = ML_ERR_BUFFER_TOO_SMALL;
        goto clean;
    }

        /* compute valid part of buffer, next partial frame, and amount to return to caller (uselen) */
    validlen       = player->sp_PartialFrameLen + MIN (readlen - fileblockoffset, rembytes);
    nextpartiallen = validlen % player->sp_SampleFrameInfo.spfi_AlignmentBytes;
    uselen         = validlen - nextpartiallen;

  #if DEBUG_FillSpooler
    printf ("    result: useoffset=%lu validlen=%lu uselen=%lu partiallen=%lu\n", useoffset, validlen, uselen, nextpartiallen);
  #endif

        /* read into buffer */
        /* @@@ this memcpy() could be centralized */
    memcpy (bufaddr, player->sp_PartialFrameAddr, player->sp_PartialFrameLen);
    if ( (errcode = ReadFile (sound->spsf_IOReq, fileblocknum, bufaddr + readoffset, readlen)) < 0 ) goto clean;

        /* set resulting useaddr, uselen, nextpartiallen */
    *useaddrp        = bufaddr + useoffset;
    *uselenp         = uselen;
    *nextpartiallenp = nextpartiallen;

    return 0;

clean:
    return errcode;
}


/* -------------------- File System and I/O support stuff */

    /* I/O support */
#if VERS_NewDoIO
    /* real DoIO() returns io_Error */
  #define IntDoIO DoIO
#else
    /* caller of DoIO() must also check io_Error */
  static Err IntDoIO (Item ioreq, const IOInfo *);
#endif

/*
    This function returns an error instead of returning a block size of 0.
*/
static int32 GetFileBlockSize (Item fileioreq)
{
    IOInfo ioinfo;
    FileStatus stat;
    Err errcode;

    memset (&ioinfo, 0, sizeof ioinfo);
    ioinfo.ioi_Command         = CMD_STATUS;
    ioinfo.ioi_Recv.iob_Buffer = &stat;
    ioinfo.ioi_Recv.iob_Len    = sizeof stat;

    return (errcode = IntDoIO (fileioreq, &ioinfo)) < 0 ? errcode
         : stat.fs.ds_DeviceBlockSize ? stat.fs.ds_DeviceBlockSize
         : ML_ERR_BADITEM;      /* @@@ could have a more suitable error code, but this should be fine */
}

static Err ReadFile (Item fileioreq, uint32 blocknum, void *readaddr, uint32 readlen)
{
    IOInfo ioinfo;

    memset (&ioinfo, 0, sizeof ioinfo);
    ioinfo.ioi_Command         = CMD_READ;
    ioinfo.ioi_Offset          = blocknum;
    ioinfo.ioi_Recv.iob_Buffer = readaddr;
    ioinfo.ioi_Recv.iob_Len    = readlen;

    return IntDoIO (fileioreq, &ioinfo);
}

#if !VERS_NewDoIO
static Err IntDoIO (Item ioreq, const IOInfo *ioinfo)
{
    Err errcode;
    const IOReq *ioreqp;

    return (errcode = DoIO (ioreq, (IOInfo *)ioinfo)) < 0 ? errcode
         : (ioreqp = (IOReq *)LookupItem (ioreq)) != NULL ? ioreqp->io_Error
         : ML_ERR_BADITEM;
}
#endif  /* !VERS_NewDoIO */


/* -------------------- ScanAIFF() */

static Err ParseAIFFForm (iff_control *, uint32 FormType, int32 FormSize);
static Err HandleAIFFChunk (iff_control *, uint32 ChunkType, uint32 ChunkSize);
static uint32 GetCompressionRatio (uint32 compressiontype);
static void UnpackAIFFLoop (AIFFLoop *, const AIFFPackedLoop *);

/*
    Parse an AIFF file and fill out an AIFFInfo structure.
*/
static Err ScanAIFF (AIFFInfo **resultAIFFInfo, const char *filename)
{
    AIFFInfo *ainfo = NULL;
    Err errcode;

        /* initialize result (must be done first) */
    if (!resultAIFFInfo) return ML_ERR_BADPTR;
    *resultAIFFInfo = NULL;

        /* allocate AIFFInfo */
    if ( (ainfo = (AIFFInfo *)AllocMem (sizeof *ainfo, MEMTYPE_ANY | MEMTYPE_FILL)) == NULL ) {
        errcode = ML_ERR_NOMEM;
        goto clean;
    }

        /* parse file */
    {
        iff_control iffcb;

        memset (&iffcb, 0, sizeof iffcb);
        iffcb.iffc_FormHandler = ParseAIFFForm;
        iffcb.iffc_UserContext = ainfo;

        if ( (errcode = iffParseFile (&iffcb, filename)) < 0 ) goto clean;
    }

  #if DEBUG_AIFF
    printf ("ScanAIFF() results:\n");
    printf ("  COMM: frames=%lu width=%lu chan=%lu comp='%.4s' X%lu\n",
        ainfo->ainf_NumFrames, ainfo->ainf_Width, ainfo->ainf_Channels, &ainfo->ainf_CompressionType, ainfo->ainf_CompressionRatio);
    printf ("  INST: sustain: %lu %lu %lu  release: %lu %lu %lu\n",
        ainfo->ainf_SustainLoop.alop_PlayMode,
        ainfo->ainf_SustainLoop.alop_BeginMarker,
        ainfo->ainf_SustainLoop.alop_EndMarker,
        ainfo->ainf_ReleaseLoop.alop_PlayMode,
        ainfo->ainf_ReleaseLoop.alop_BeginMarker,
        ainfo->ainf_ReleaseLoop.alop_EndMarker);
    printf ("  MARK: num=%lu buf=$%08lx,%lu\n", ainfo->ainf_NumMarkers, ainfo->ainf_MarkerBuf, ainfo->ainf_MarkerBufSize);
    printf ("  SSND: dataoffset=%lu numbytes=%lu\n", ainfo->ainf_DataOffset, ainfo->ainf_NumBytes);
  #endif

        /* sanity check sound parameters */
    if ( !ainfo->ainf_NumFrames ||
         !ainfo->ainf_Width ||
         !ainfo->ainf_Channels ||
         ainfo->ainf_NumBytes * ainfo->ainf_CompressionRatio / (ainfo->ainf_Width * ainfo->ainf_Channels) < ainfo->ainf_NumFrames ) {

        errcode = ML_ERR_BAD_FORMAT;
        goto clean;
    }

    /* @@@ could sanity check loop marker ordering here, too, but that's not really critical for this player */

        /* success: set result */
    *resultAIFFInfo = ainfo;
    return 0;

clean:
    FreeAIFFInfo (ainfo);
    return errcode;
}

/*
    Free memory allocated by ScanAIFF()
*/
static void FreeAIFFInfo (AIFFInfo *ainfo)
{
    if (ainfo) {
        FreeMem (ainfo->ainf_MarkerBuf, ainfo->ainf_MarkerBufSize);
        FreeMem (ainfo, sizeof *ainfo);
    }
}

/*
    Parse FORM AIFF
*/
static Err ParseAIFFForm (iff_control *iffc, uint32 FormType, int32 FormSize)
{
    AIFFInfo * const ainfo = (AIFFInfo *)iffc->iffc_UserContext;
    Err (* const OldHandler)() = iffc->iffc_ChunkHandler;
    Err errcode = 0;

    iffc->iffc_ChunkHandler = HandleAIFFChunk;

  #if DEBUG_AIFF
    printf ("ParseAIFFForm: FORM='%.4s'\n", &FormType);
  #endif

    switch (FormType) {
        case ID_AIFF:
        case ID_AIFC:
            ainfo->ainf_FormID = FormType;
            break;

        default:
            errcode = ML_ERR_INVALID_FILE_TYPE;
            goto clean;
    }

    errcode = iffScanChunks (iffc, FormSize);

clean:
    iffc->iffc_ChunkHandler = OldHandler;
    return errcode;
}

/*
    Handle AIFF chunks
    This function expects FreeAIFFInfo() to free anything that it allocated
*/
static Err HandleAIFFChunk (iff_control *iffc, uint32 ChunkType, uint32 ChunkSize)
{
    AIFFInfo * const ainfo = (AIFFInfo *)iffc->iffc_UserContext;
    Err errcode;

  #if DEBUG_AIFF
    printf ("HandleAIFFChunk() '%.4s' %ld\n", &ChunkType, ChunkSize);
  #endif

    switch (ChunkType) {
        case ID_COMM:
            {
                AIFFPackedCommon comm;

                    /* read packed COMM chunk */
                memset (&comm, 0, sizeof comm);
                if ((errcode = iffReadChunkData (iffc, &comm, MIN (sizeof comm, ChunkSize))) < 0) goto clean;

                ainfo->ainf_Channels         = UNPACK_UINT16 (comm.commx_NumChannels);
                ainfo->ainf_NumFrames        = UNPACK_UINT32 (comm.commx_NumSampleFrames);
                ainfo->ainf_Width            = UCEIL (UNPACK_UINT16 (comm.commx_SampleSize), 8);
                if (ainfo->ainf_FormID == ID_AIFC)
                    ainfo->ainf_CompressionType = UNPACK_UINT32 (comm.commx_CompressionType);
                ainfo->ainf_CompressionRatio = GetCompressionRatio (ainfo->ainf_CompressionType);

              #if DEBUG_AIFF
                printf ("COMM: frames=%lu width=%lu chan=%lu comp='%.4s' X%lu\n",
                    ainfo->ainf_NumFrames, ainfo->ainf_Width, ainfo->ainf_Channels, &ainfo->ainf_CompressionType, ainfo->ainf_CompressionRatio);
              #endif
            }
            break;

        case ID_INST:
            {
                AIFFPackedInstrument inst;

                    /* read packed INST chunk */
                memset (&inst, 0, sizeof inst);
                if ((errcode = iffReadChunkData (iffc, &inst, MIN (sizeof inst, ChunkSize))) < 0) goto clean;

                    /* get loop parameters */
                UnpackAIFFLoop (&ainfo->ainf_SustainLoop, &inst.instx_SustainLoop);
                UnpackAIFFLoop (&ainfo->ainf_ReleaseLoop, &inst.instx_ReleaseLoop);

              #if DEBUG_AIFF
                printf ("INST: sustain: %lu %lu %lu  release: %lu %lu %lu\n",
                    ainfo->ainf_SustainLoop.alop_PlayMode,
                    ainfo->ainf_SustainLoop.alop_BeginMarker,
                    ainfo->ainf_SustainLoop.alop_EndMarker,
                    ainfo->ainf_ReleaseLoop.alop_PlayMode,
                    ainfo->ainf_ReleaseLoop.alop_BeginMarker,
                    ainfo->ainf_ReleaseLoop.alop_EndMarker);
              #endif
            }
            break;

        case ID_MARK:
            {
                packed_uint16 x_nummarkers;

                    /* complain about multiple MARK chunks */
                if (ainfo->ainf_NumMarkers) {
                    errcode = ML_ERR_BAD_FORMAT;
                    goto clean;
                }

                    /* read MARK header */
                if (ChunkSize < sizeof x_nummarkers) {
                    errcode = ML_ERR_BAD_FORMAT;
                    goto clean;
                }
                if ((errcode = iffReadChunkData (iffc, &x_nummarkers, sizeof x_nummarkers)) < 0) goto clean;

                    /* allocate space for and read packed marker table */
                if ((ainfo->ainf_NumMarkers = UNPACK_UINT16(x_nummarkers)) != 0) {
                    ainfo->ainf_MarkerBufSize = ChunkSize - sizeof x_nummarkers;

                        /* trap 0-size marker table - mangled file */
                    if (!ainfo->ainf_MarkerBufSize) {
                        errcode = ML_ERR_BAD_FORMAT;
                        goto clean;
                    }

                        /* do alloc and read */
                    if ((ainfo->ainf_MarkerBuf = AllocMem (ainfo->ainf_MarkerBufSize, MEMTYPE_ANY)) == NULL) {
                        errcode = ML_ERR_NOMEM;
                        goto clean;
                    }
                    if ((errcode = iffReadChunkData (iffc, ainfo->ainf_MarkerBuf, ainfo->ainf_MarkerBufSize)) < 0) goto clean;
                }

              #if DEBUG_AIFF
                printf ("MARK: num=%lu buf=$%08lx,%lu\n", ainfo->ainf_NumMarkers, ainfo->ainf_MarkerBuf, ainfo->ainf_MarkerBufSize);
              #endif
            }
            break;

        case ID_SSND:
            {
                AIFFSoundDataHeader ssndhead;

                    /* read SSND header */
                if (ChunkSize < sizeof ssndhead) {
                    errcode = ML_ERR_BAD_FORMAT;
                    goto clean;
                }
                if ((errcode = iffReadChunkData (iffc, &ssndhead, sizeof ssndhead)) < 0) goto clean;

                    /*
                        Skip to beginning of audio data.
                        Store file seek position (conveniently returned from skip()) and length of sound data.
                    */
                ainfo->ainf_DataOffset = iffSkipChunkData (iffc, ssndhead.ssnd_Offset);
                ainfo->ainf_NumBytes   = ChunkSize - (sizeof ssndhead + ssndhead.ssnd_Offset);

              #if DEBUG_AIFF
                printf ("SSND: dataoffset=%lu numbytes=%lu\n", ainfo->ainf_DataOffset, ainfo->ainf_NumBytes);
              #endif
            }
            break;
    }

    return 0;

clean:
    return errcode;
}

/*
    Attempt to identify compression ratio of a compression type
*/
static uint32 GetCompressionRatio (uint32 compressiontype)
{
    switch (compressiontype)
    {
        case 0:         return 1;
        case ID_SDX2:   return 2;
        case ID_SDX3:   return 3;
        case ID_ADP4:   return 4;
    }

    {
        const char compressionguess = (char)compressiontype;    /* get last character */

        return compressionguess >= '1' && compressionguess <= '9' ? (uint32)compressionguess - '0' : 2;
    }
}

/*
    Unpack AIFFLoop (AIFFPackedLoop is really an array of 3 16-bit words)
*/
static void UnpackAIFFLoop (AIFFLoop *alop, const AIFFPackedLoop *alopx)
{
    alop->alop_PlayMode    = UNPACK_UINT16((*alopx)[0]);
    alop->alop_BeginMarker = UNPACK_UINT16((*alopx)[1]);
    alop->alop_EndMarker   = UNPACK_UINT16((*alopx)[2]);
}


/* -------------------- pstring support (for AIFF) */

/*
    Convert a pstring to a C string
*/
char *ptocstr (char *dest, const char *src, size_t destsize)
{
    const size_t len = MIN(destsize-1,pstrlen(src));

    memcpy (dest, pstraddr(src), len);  /* copy bytes to dest string */
    dest[len] = 0;                      /* add null termination */

    return dest;
}
