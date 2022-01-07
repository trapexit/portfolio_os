/* $Id: addscreengroup.c,v 1.2 1994/09/24 03:33:02 ewhac Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"
#include <varargs_glue.h>


/*****************************************************************************/


Err
AddScreenGroup( Item screenGroup, TagArg *targs )
{
  int32 rval;
  CALLFOLIORET (GrafBase, _ADDSCREENGROUP_, (screenGroup, targs), rval, (int32));
  return rval;
}

VAGLUE_FUNC (Err,
	AddScreenGroupVA (Item screenGroup, VAGLUE_VA_TAGS),
	AddScreenGroup   (screenGroup, VAGLUE_TAG_POINTER))
