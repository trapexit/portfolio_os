/* $Id: getcompressorworkbuffersize.c,v 1.1 1994/08/17 16:38:41 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"


/****************************************************************************/


int32 GetCompressorWorkBufferSize(const TagArg *tags)
{
Err result;

    CallFolioRet(CompressionBase, GETCOMPRESSORWORKBUFFERSIZE, (tags), result, (int32));

    return result;
}
