/* $Id: createscreengroup.c,v 1.2 1994/09/24 03:33:02 ewhac Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"
#include <varargs_glue.h>


/*****************************************************************************/


Item
CreateScreenGroup( Item *screenItemArray, TagArg *targs )
{
  Item r;
  CALLFOLIORET (GrafBase, _CREATESCREENGROUP_, (screenItemArray, targs), r, (Item));
  return r;
}


VAGLUE_FUNC (Item,
	CreateScreenGroupVA (Item *screenItemArray, VAGLUE_VA_TAGS),
	CreateScreenGroup   (screenItemArray, VAGLUE_TAG_POINTER))
