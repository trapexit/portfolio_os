
/******************************************************************************
**
**  $Id: loadfile24.h,v 1.3 1994/11/15 17:48:18 vertex Exp $
**
**  Load an image file and return its image control chunk.
**
******************************************************************************/

#include "types.h"


int32 loadfile24 (char *name, void *buffer, uint32 buffersize, uint32 memtype, VdlChunk **rawVDLPtr,
      ImageCC *image, int32 width, int32 height);


