/* $Id: compress.c,v 1.3 1994/09/15 23:43:04 vertex Exp $ */

#include "types.h"
#include "string.h"
#include "mem.h"
#include "lzss.h"
#include "tags.h"


/*****************************************************************************/


/* This is the compression module which implements an LZ77-style (LZSS)
 * compression algorithm. As implemented here it uses a 12 bit index into the
 * sliding window, and a 4 bit length, which is adjusted to reflect phrase
 * lengths of between 3 and 18 bytes.
 *
 * WARNING: Check with the legal department before making any changes to
 *          the algorithms used herein in order to ensure that the code
 *          doesn't infrige on the multiple gratuitous patents on
 *          compression code.
 */


/*****************************************************************************/


typedef void (* CompFuncClone)(void *userData, uint32 word);

typedef struct BitStream
{
    CompFuncClone  bs_OutputWord;
    void          *bs_UserData;
    uint32         bs_BitsLeft;
    uint32         bs_BitBuffer;
} BitStream;


/*****************************************************************************/


typedef struct Compressor
{
    unsigned char ch_Window[WINDOW_SIZE];
    CompNode      ch_Tree[WINDOW_SIZE+1];
    int32         ch_LookAhead;
    uint32        ch_MatchLen;
    uint32        ch_MatchPos;
    uint32        ch_CurrentPos;
    uint32        ch_ReplaceCnt;
    BitStream     ch_BitStream;
    bool          ch_SecondPass;
    bool          ch_AllocatedStructure;
    void         *ch_Cookie;
} Compressor;


/*****************************************************************************/


typedef void *Decompressor;
#define __COMPRESSION_PRIVATE
#include "compression.h"


/*****************************************************************************/


static void InitBitStream(BitStream *bs, CompFuncClone cf, void *userData)
{
    bs->bs_OutputWord = cf;
    bs->bs_UserData   = userData;
    bs->bs_BitsLeft   = 32;
    bs->bs_BitBuffer  = 0;
}


/*****************************************************************************/


static void CleanupBitStream(BitStream *bs)
{
    if (bs->bs_BitsLeft != 32)
        (* bs->bs_OutputWord)(bs->bs_UserData, bs->bs_BitBuffer);
}


/*****************************************************************************/


/* This routine outputs a single header bit, followed by numBits of code */
static void WriteBits(BitStream *bs, uint32 headBit, uint32 code, uint32 numBits)
{
    bs->bs_BitsLeft--;
    bs->bs_BitBuffer |= (headBit << bs->bs_BitsLeft);

    if (numBits >= bs->bs_BitsLeft)
    {
        numBits         -= bs->bs_BitsLeft;
        (* bs->bs_OutputWord)(bs->bs_UserData,(code >> numBits) | bs->bs_BitBuffer);
        bs->bs_BitsLeft  = 32 - numBits;

        if (!numBits)
            bs->bs_BitBuffer = 0;
        else
            bs->bs_BitBuffer = (code << bs->bs_BitsLeft);
    }
    else
    {
        bs->bs_BitsLeft  -= numBits;
        bs->bs_BitBuffer |= (code << bs->bs_BitsLeft);
    }
}


/*****************************************************************************/


/* This routine performs a classic binary tree deletion.
 * If the node to be deleted has a null link in either direction, we
 * just pull the non-null link up one to replace the existing link.
 * If both links exist, we instead delete the next link in order, which
 * is guaranteed to have a null link, then replace the node to be deleted
 * with the next link.
 */
static void DeleteString(CompNode *tree, uint32 node)
{
uint32 parent;
uint32 newNode;
uint32 next;

    parent = tree[node].cn_Parent;
    if (parent != UNUSED)
    {
        if (tree[node].cn_LeftChild == UNUSED)
        {
            newNode                 = tree[node].cn_RightChild;
            tree[newNode].cn_Parent = parent;
        }
        else if (tree[node].cn_RightChild == UNUSED)
        {
            newNode                 = tree[node].cn_LeftChild;
            tree[newNode].cn_Parent = parent;
        }
        else
        {
            newNode = tree[node].cn_LeftChild;
            next = tree[newNode].cn_RightChild;
            if (next != UNUSED)
            {
                do
                {
                    newNode = next;
                    next = tree[newNode].cn_RightChild;
                }
                while (next != UNUSED);

                tree[tree[newNode].cn_Parent].cn_RightChild = UNUSED;
                tree[newNode].cn_Parent                     = tree[node].cn_Parent;
                tree[newNode].cn_LeftChild                  = tree[node].cn_LeftChild;
                tree[newNode].cn_RightChild                 = tree[node].cn_RightChild;
                tree[tree[newNode].cn_LeftChild].cn_Parent  = newNode;
                tree[tree[newNode].cn_RightChild].cn_Parent = newNode;
            }
            else
            {
                tree[newNode].cn_Parent                     = parent;
                tree[newNode].cn_RightChild                 = tree[node].cn_RightChild;
                tree[tree[newNode].cn_RightChild].cn_Parent = newNode;
            }
        }

        if (tree[parent].cn_LeftChild == node)
            tree[parent].cn_LeftChild = newNode;
        else
            tree[parent].cn_RightChild = newNode;

        tree[node].cn_Parent = UNUSED;
    }
}


/*****************************************************************************/


Err CreateCompressor(Compressor **comp, CompFunc cf, const TagArg *tags)
{
bool    allocated;
void   *buffer;
void   *userData;
TagArg *tag;

    if (!comp)
        return COMP_ERR_BADPTR;

    *comp = NULL;

    if (!cf)
        return COMP_ERR_BADPTR;

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
        buffer = AllocMem(sizeof(Compressor),MEMTYPE_ANY);
        if (!buffer)
            return COMP_ERR_NOMEM;

        allocated = TRUE;
    }

    (*comp)                        = (Compressor *)buffer;
    (*comp)->ch_LookAhead          = 1;
    (*comp)->ch_CurrentPos         = 1;
    (*comp)->ch_MatchPos           = 0;
    (*comp)->ch_MatchLen           = 0;
    (*comp)->ch_ReplaceCnt         = 0;
    (*comp)->ch_SecondPass         = FALSE;
    (*comp)->ch_Cookie             = *comp;
    (*comp)->ch_AllocatedStructure = allocated;

    InitBitStream(&(*comp)->ch_BitStream,cf,userData);

    /* To make the tree usable, everything must be set to UNUSED, and a
     * single phrase has to be added to the tree so it has a root node.
     */
    memset((*comp)->ch_Tree, UNUSED, sizeof((*comp)->ch_Tree));
    (*comp)->ch_Tree[TREE_ROOT].cn_RightChild = 1;
    (*comp)->ch_Tree[1].cn_Parent             = TREE_ROOT;

    return (0);
}


/*****************************************************************************/


static void FlushCompressor(Compressor *comp)
{
int32          lookAhead;
uint32         currentPos;
uint32         replaceCnt;
uint32         matchLen;
uint32         matchPos;
CompNode      *tree;
unsigned char *window;
BitStream     *bs;
uint32         temp;

    tree         = comp->ch_Tree;
    window       = comp->ch_Window;
    bs           = &comp->ch_BitStream;
    lookAhead    = comp->ch_LookAhead;
    currentPos   = comp->ch_CurrentPos;
    matchLen     = comp->ch_MatchLen;
    matchPos     = comp->ch_MatchPos;
    replaceCnt   = comp->ch_ReplaceCnt;

    if (comp->ch_SecondPass)
        goto newData;

    while (lookAhead >= 0)
    {
        if (matchLen > lookAhead)
            matchLen = lookAhead;

        if (matchLen <= BREAK_EVEN)
        {
            WriteBits(bs, 1, (uint32) window[currentPos], 8);
            replaceCnt = 1;
        }
        else
        {
            temp = (matchPos << LENGTH_BIT_COUNT) | (matchLen - (BREAK_EVEN + 1));
            WriteBits(bs, 0, temp, INDEX_BIT_COUNT + LENGTH_BIT_COUNT);
            replaceCnt = matchLen;
        }

        while (replaceCnt--)
        {
            DeleteString(tree, MOD_WINDOW(currentPos + LOOK_AHEAD_SIZE));
            lookAhead--;
newData:    currentPos = MOD_WINDOW(currentPos + 1);

            if (lookAhead)
                matchLen = AddString(tree, window, currentPos, &matchPos);
        }
    }
}


/*****************************************************************************/


Err DeleteCompressor(Compressor *comp)
{
    if (!comp || (comp->ch_Cookie != comp))
        return (COMP_ERR_BADPTR);

    comp->ch_Cookie = NULL;

    FlushCompressor(comp);
    WriteBits(&comp->ch_BitStream, 0, END_OF_STREAM, INDEX_BIT_COUNT);
    CleanupBitStream(&comp->ch_BitStream);

    if (comp->ch_AllocatedStructure)
        FreeMem(comp,sizeof(Compressor));

    return (0);
}


/*****************************************************************************/


/* This is the compression routine. It has to first load up the look
 * ahead buffer, then go into the main compression loop. The main loop
 * decides whether to output a single character or an index/length
 * token that defines a phrase. Once the character or phrase has been
 * sent out, another loop has to run. The second loop reads in new
 * characters, deletes the strings that are overwritten by the new
 * character, then adds the strings that are created by the new
 * character.
 */

Err FeedCompressor(Compressor *comp, void *data, uint32 numDataWords)
{
int32          lookAhead;
uint32         currentPos;
uint32         replaceCnt;
uint32         matchLen;
uint32         matchPos;
uint8         *src;
CompNode      *tree;
unsigned char *window;
BitStream     *bs;
uint32         numDataBytes;
uint32         temp;

    if (!comp || (comp->ch_Cookie != comp))
        return (COMP_ERR_BADPTR);

    tree         = comp->ch_Tree;
    window       = comp->ch_Window;
    bs           = &comp->ch_BitStream;
    lookAhead    = comp->ch_LookAhead;
    currentPos   = comp->ch_CurrentPos;
    matchLen     = comp->ch_MatchLen;
    matchPos     = comp->ch_MatchPos;
    replaceCnt   = comp->ch_ReplaceCnt;
    numDataBytes = numDataWords * sizeof(uint32);
    src          = (uint8 *)data;

    if (!numDataBytes)
        return (0);

    if (comp->ch_SecondPass)
        goto newData;

    while (lookAhead <= LOOK_AHEAD_SIZE)
    {
        if (!numDataBytes)
        {
            comp->ch_LookAhead = lookAhead;
            return (0);
        }

        window[lookAhead++] = *src++;
        numDataBytes--;
    }

    lookAhead--;
    while (TRUE)
    {
        if (matchLen > lookAhead)
            matchLen = lookAhead;

        if (matchLen <= BREAK_EVEN)
        {
            WriteBits(bs, 1, (uint32) window[currentPos], 8);
            replaceCnt = 1;
        }
        else
        {
            temp = (matchPos << LENGTH_BIT_COUNT) | (matchLen - (BREAK_EVEN + 1));
            WriteBits(bs, 0, temp, INDEX_BIT_COUNT + LENGTH_BIT_COUNT);
            replaceCnt = matchLen;
        }

        while (replaceCnt--)
        {
            DeleteString(tree, MOD_WINDOW(currentPos + LOOK_AHEAD_SIZE));

            if (!numDataBytes)
            {
                /* We ran out of data. Save all the state, and exit. If
                 * we are called with more data, we'll jump right back in
                 * this loop, and continue processing
                 */

                comp->ch_LookAhead  = lookAhead;
                comp->ch_CurrentPos = currentPos;
                comp->ch_MatchLen   = matchLen;
                comp->ch_MatchPos   = matchPos;
                comp->ch_ReplaceCnt = replaceCnt;
                comp->ch_SecondPass = TRUE;
                return (0);
            }
newData:
            window[MOD_WINDOW(currentPos + LOOK_AHEAD_SIZE)] = *src++;
            numDataBytes--;

            currentPos = MOD_WINDOW(currentPos + 1);

            if (lookAhead)
                matchLen = AddString(tree, window, currentPos, &matchPos);
        }
    }
}


/*****************************************************************************/


int32 GetCompressorWorkBufferSize(const TagArg *tags)
{
    if (tags)
        return COMP_ERR_BADTAG;

    return sizeof(Compressor);
}
