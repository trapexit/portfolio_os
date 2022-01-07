/* $Id: deletecompressor.c,v 1.1 1994/08/17 16:38:41 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"


/****************************************************************************/


Err DeleteCompressor(Compressor *comp)
{
Err result;

    CallFolioRet(CompressionBase, DELETECOMPRESSOR, (comp), result, (Err));

    return result;
}
