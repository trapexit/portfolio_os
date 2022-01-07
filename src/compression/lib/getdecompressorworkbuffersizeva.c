/* $Id: getdecompressorworkbuffersizeva.c,v 1.1 1994/09/16 00:15:26 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"
#include "varargs_glue.h"


/****************************************************************************/


FOLIOGLUE_VA_FUNC(GetDecompressorWorkBufferSizeVA,CompressionBase,GETDECOMPRESSORWORKBUFFERSIZE,
                  (VAGLUE_VA_TAGS),
                  (VAGLUE_TAG_POINTER), int32)
