/* $Id: createitemva.c,v 1.1 1994/09/16 00:52:01 vertex Exp $ */

#include "types.h"
#include "varargs_glue.h"
#include "item.h"


/*****************************************************************************/


VAGLUE_FUNC (Item, CreateItemVA (int32 ctype, VAGLUE_VA_TAGS), CreateItem (ctype, VAGLUE_TAG_POINTER))
