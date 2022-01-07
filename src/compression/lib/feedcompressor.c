/* $Id: feedcompressor.c,v 1.1 1994/08/17 16:38:41 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"


/****************************************************************************/


Err FeedCompressor(Compressor *comp, void *data, uint32 numDataWords)
{
Err result;

    CallFolioRet(CompressionBase, FEEDCOMPRESSOR, (comp,data,numDataWords), result, (Err));

    return result;
}
