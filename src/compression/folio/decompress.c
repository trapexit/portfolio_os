/* $Id: decompress.c,v 1.4 1994/11/03 18:57:35 vertex Exp $ */

#include "types.h"
#include "mem.h"
#include "lzss.h"
#include "tags.h"


/*****************************************************************************/


/* WARNING: Check with the legal department before making any changes to
 *          the algorithms used herein in order to ensure that the code
 *          doesn't infringe on the multiple gratuitous patents on
 *          compression code.
 */


/*****************************************************************************/


typedef struct BitStream
{
    uint32 *bs_Data;
    uint32  bs_NumDataWords;
    uint32  bs_BitsLeft;
    uint32  bs_BitBuffer;
    bool    bs_Error;
} BitStream;


/*****************************************************************************/


typedef void (* CompFuncClone)(void *userData, uint32 word);

typedef struct Decompressor
{
    CompFuncClone  dh_OutputWord;
    void          *dh_UserData;
    uint32         dh_WordBuffer;
    uint32         dh_BytesLeft;
    uint32         dh_Pos;
    unsigned char  dh_Window[WINDOW_SIZE];
    BitStream      dh_BitStream;
    bool           dh_AllocatedStructure;
    void          *dh_Cookie;
} Decompressor;


/*****************************************************************************/


typedef void *Compressor;
#define __COMPRESSION_PRIVATE
#include "compression.h"


/*****************************************************************************/


static void InitBitStream(BitStream *bs)
{
    bs->bs_BitsLeft  = 0;
    bs->bs_BitBuffer = 0;
    bs->bs_Error     = FALSE;
}


/*****************************************************************************/


static void FeedBitStream(BitStream *bs, const void *data, uint32 numDataWords)
{
    bs->bs_Data         = (uint32 *)data;
    bs->bs_NumDataWords = numDataWords;
}


/*****************************************************************************/


static uint32 ReadBits(BitStream *bs, uint32 numBits)
{
uint32 result;

    result = 0;

    if (numBits > bs->bs_BitsLeft)
    {
        if (bs->bs_BitsLeft)
        {
            result = (bs->bs_BitBuffer << (numBits - bs->bs_BitsLeft)) & ((1 << numBits) - 1);
            numBits -= bs->bs_BitsLeft;
        }

        if (!bs->bs_NumDataWords)
        {
            bs->bs_Error = TRUE;
            return (0);
        }

        bs->bs_NumDataWords--;
        bs->bs_BitBuffer = *bs->bs_Data++;
        bs->bs_BitsLeft  = 32;
    }

    bs->bs_BitsLeft -= numBits;
    result |= (bs->bs_BitBuffer >> bs->bs_BitsLeft) & ((1 << numBits) - 1);

    return (result);
}


/*****************************************************************************/


/* All this decompression routine has to do is read in flag bits, decide
 * whether to read in a character or an index/length pair, and take the
 * appropriate action.
 */

static Err internalFeedDecompressor(Decompressor *decomp, void *data,
                                    uint32 numDataWords)
{
uint32         i;
uint32         pos;
uint32         c;
uint32         matchLen;
uint32         matchPos;
BitStream     *bs;
uint32         wordBuffer;
uint32         bytesLeft;
CompFunc       cf;
unsigned char *window;
void          *userData;

    cf         = decomp->dh_OutputWord;
    wordBuffer = decomp->dh_WordBuffer;
    bytesLeft  = decomp->dh_BytesLeft;
    window     = decomp->dh_Window;
    userData   = decomp->dh_UserData;
    bs         = &decomp->dh_BitStream;
    pos        = decomp->dh_Pos;

    FeedBitStream(bs, data, numDataWords);

    while (bs->bs_NumDataWords)
    {
        if (ReadBits(bs,1))
        {
            c = ReadBits(bs, 8);

            if (bytesLeft == 0)
            {
                (* cf)(userData, wordBuffer);
                wordBuffer = c;
                bytesLeft  = 3;
            }
            else
            {
                wordBuffer = (wordBuffer << 8) | c;
                bytesLeft--;
            }

            window[pos] = (unsigned char) c;
            pos = MOD_WINDOW(pos + 1);
        }
        else
        {
            matchPos = ReadBits(bs, INDEX_BIT_COUNT);
            if (matchPos == END_OF_STREAM)
                break;

            matchLen = ReadBits(bs, LENGTH_BIT_COUNT) + BREAK_EVEN;

            for (i = matchPos; i <= matchLen + matchPos; i++)
            {
                c = window[MOD_WINDOW(i)];

                if (bytesLeft == 0)
                {
                    (* cf)(userData, wordBuffer);
                    wordBuffer = c;
                    bytesLeft  = 3;
                }
                else
                {
                    wordBuffer = (wordBuffer << 8) | c;
                    bytesLeft--;
                }

                window[pos] = (unsigned char) c;
                pos = MOD_WINDOW(pos + 1);
            }
        }
    }

    decomp->dh_BytesLeft  = bytesLeft;
    decomp->dh_WordBuffer = wordBuffer;
    decomp->dh_Pos        = pos;

    return (0);
}


/*****************************************************************************/


Err CreateDecompressor(Decompressor **decomp, CompFunc cf, const TagArg *tags)
{
bool    allocated;
void   *buffer;
void   *userData;
TagArg *tag;

    if (!decomp)
        return COMP_ERR_BADPTR;

    *decomp = NULL;

    if (!cf)
        return (COMP_ERR_BADPTR);

    buffer   = NULL;
    userData = NULL;

    while ((tag = NextTagArg(&tags)) != NULL)
    {
        switch (tag->ta_Tag)
        {
            case COMP_TAG_WORKBUFFER: buffer = tag->ta_Arg;
                                      break;

            case COMP_TAG_USERDATA  : userData = tag->ta_Arg;
                                      break;

            default                 : return COMP_ERR_BADTAG;
        }
    }

    allocated = FALSE;
    if (!buffer)
    {
        buffer = AllocMem(sizeof(Decompressor),MEMTYPE_FILL|MEMTYPE_ANY);
        if (!buffer)
            return COMP_ERR_NOMEM;

        allocated = TRUE;
    }

    (*decomp)                        = (Decompressor *)buffer;
    (*decomp)->dh_OutputWord         = cf;
    (*decomp)->dh_UserData           = userData;
    (*decomp)->dh_WordBuffer         = 0;
    (*decomp)->dh_BytesLeft          = 4;
    (*decomp)->dh_Pos                = 1;
    (*decomp)->dh_Cookie             = *decomp;
    (*decomp)->dh_AllocatedStructure = allocated;
    InitBitStream(&(*decomp)->dh_BitStream);

    return (0);
}


/*****************************************************************************/


Err DeleteDecompressor(Decompressor *decomp)
{
Err result;

    if (!decomp || (decomp->dh_Cookie != decomp))
        return (COMP_ERR_BADPTR);

    decomp->dh_Cookie = NULL;

    result = 0;

    if (decomp->dh_BytesLeft == 0)
        (* decomp->dh_OutputWord)(decomp->dh_UserData, decomp->dh_WordBuffer);

    if (decomp->dh_BitStream.bs_NumDataWords)
        result = COMP_ERR_DATAREMAINS;

    if (decomp->dh_BitStream.bs_Error)
        result = COMP_ERR_DATAMISSING;

    if (decomp->dh_AllocatedStructure)
        FreeMem(decomp,sizeof(Decompressor));

    return (result);
}


/*****************************************************************************/


Err FeedDecompressor(Decompressor *decomp, void *data, uint32 numDataWords)
{
    if (!decomp || (decomp->dh_Cookie != decomp))
        return (COMP_ERR_BADPTR);

    return (internalFeedDecompressor(decomp, data, numDataWords));
}


/*****************************************************************************/


int32 GetDecompressorWorkBufferSize(const TagArg *tags)
{
    if (tags)
        return COMP_ERR_BADTAG;

    return sizeof(Decompressor);
}
