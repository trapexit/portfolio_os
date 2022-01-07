/* $Id: createsizeditemva.c,v 1.1 1994/09/16 00:52:01 vertex Exp $ */

#include "types.h"
#include "varargs_glue.h"
#include "item.h"


/*****************************************************************************/


VAGLUE_FUNC (Item, CreateSizedItemVA (int32 ctype, int32 size, VAGLUE_VA_TAGS), CreateSizedItem (ctype, VAGLUE_TAG_POINTER, size))
