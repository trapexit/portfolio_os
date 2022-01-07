/* $Id: modifyvdl.c,v 1.2 1994/09/24 03:33:02 ewhac Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"
#include <varargs_glue.h>


/*****************************************************************************/


Err
ModifyVDL (Item vdlItem, TagArg* vdlTags)
{
  Err rval;
  CALLFOLIORET (GrafBase, _MODIFYVDL_, (vdlItem, vdlTags), rval, (Err));
  return rval;
}

VAGLUE_FUNC (Err,
	ModifyVDLVA (Item vdlItem, VAGLUE_VA_TAGS),
	ModifyVDL   (vdlItem, VAGLUE_TAG_POINTER))
