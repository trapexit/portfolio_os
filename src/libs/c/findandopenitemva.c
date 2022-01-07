/* $Id: findandopenitemva.c,v 1.1 1994/09/16 19:57:36 vertex Exp $ */

#include "types.h"
#include "varargs_glue.h"
#include "item.h"


/*****************************************************************************/


VAGLUE_FUNC (Item, FindAndOpenItemVA (int32 ctype, VAGLUE_VA_TAGS), FindAndOpenItem (ctype, VAGLUE_TAG_POINTER))
