/* $Id: finditemva.c,v 1.1 1994/09/16 16:54:18 vertex Exp $ */

#include "types.h"
#include "varargs_glue.h"
#include "item.h"


/*****************************************************************************/


VAGLUE_FUNC (Item, FindItemVA (int32 ctype, VAGLUE_VA_TAGS), FindItem (ctype, VAGLUE_TAG_POINTER))
