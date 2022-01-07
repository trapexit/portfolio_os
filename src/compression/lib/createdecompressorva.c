/* $Id: createdecompressorva.c,v 1.1 1994/09/16 00:15:26 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"
#include "varargs_glue.h"


/****************************************************************************/


FOLIOGLUE_VA_FUNC(CreateDecompressorVA,CompressionBase,CREATEDECOMPRESSOR,
                  (Decompressor **decomp, CompFunc cf, VAGLUE_VA_TAGS),
                  (decomp,cf,VAGLUE_TAG_POINTER), Err)
