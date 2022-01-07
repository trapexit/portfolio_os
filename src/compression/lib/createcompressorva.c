/* $Id: createcompressorva.c,v 1.1 1994/09/16 00:15:26 vertex Exp $ */

#include "types.h"
#include "compression_lib.h"
#include "varargs_glue.h"


/****************************************************************************/


FOLIOGLUE_VA_FUNC(CreateCompressorVA,CompressionBase,CREATECOMPRESSOR,
                  (Compressor **comp, CompFunc cf, VAGLUE_VA_TAGS),
                  (comp,cf,VAGLUE_TAG_POINTER), Err)
