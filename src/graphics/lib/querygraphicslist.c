/* $Id: querygraphicslist.c,v 1.2 1994/09/24 03:33:02 ewhac Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"
#include <varargs_glue.h>


/*****************************************************************************/


Err
QueryGraphicsList ( TagArg *ta )
{
  Err r;
  CALLFOLIORET (GrafBase, _QUERYGRAPHICSLIST_, ( ta ), r, (Err));
  return r;
}

VAGLUE_FUNC (Err,
	QueryGraphicsListVA (VAGLUE_VA_TAGS),
	QueryGraphicsList   (VAGLUE_TAG_POINTER))
