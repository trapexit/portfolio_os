/* $Id: deletedecompressor.c,v 1.1 1994/08/17 16:38:41 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"


/****************************************************************************/


Err DeleteDecompressor(Decompressor *decomp)
{
Err result;

    CallFolioRet(CompressionBase, DELETEDECOMPRESSOR, (decomp), result, (Err));

    return result;
}
