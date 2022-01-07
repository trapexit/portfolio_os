/* $Id: createcompressor.c,v 1.1 1994/08/17 16:38:41 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"


/****************************************************************************/


Err CreateCompressor(Compressor **comp, CompFunc cf, const TagArg *tags)
{
Err result;

    CallFolioRet(CompressionBase, CREATECOMPRESSOR, (comp,cf,tags), result, (Err));

    return result;
}
