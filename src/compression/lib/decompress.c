/* $Id: decompress.c,v 1.2 1995/02/14 00:57:54 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"


/*****************************************************************************/


typedef struct Context
{
    uint32 *ctx_Dest;
    uint32 *ctx_Max;
    bool    ctx_Overflow;
} Context;


/*****************************************************************************/


static void PutWord(Context *ctx, uint32 word)
{
    if (ctx->ctx_Dest >= ctx->ctx_Max)
        ctx->ctx_Overflow = TRUE;
    else
        *ctx->ctx_Dest++ = word;
}


/*****************************************************************************/


Err SimpleDecompress(void *source, uint32 sourceWords,
                     void *result, uint32 resultWords)
{
Decompressor *decomp;
TagArg        tags[2];
Context       ctx;
Err           err;

    ctx.ctx_Dest     = (uint32 *)result;
    ctx.ctx_Max      = (uint32 *)((uint32)result + resultWords * sizeof(uint32));
    ctx.ctx_Overflow = FALSE;

    tags[0].ta_Tag = COMP_TAG_USERDATA;
    tags[0].ta_Arg = (void *)&ctx;
    tags[1].ta_Tag = TAG_END;

    err = CreateDecompressor(&decomp,(CompFunc)PutWord,tags);
    if (err >= 0)
    {
        FeedDecompressor(decomp,source,sourceWords);
        err = DeleteDecompressor(decomp);

        if (err == 0)
        {
            if (ctx.ctx_Overflow)
                err = COMP_ERR_OVERFLOW;
            else
                err = ((uint32)ctx.ctx_Dest - (uint32)result) / sizeof(uint32);
        }
    }

    return err;
}
