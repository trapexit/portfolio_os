/* $Id: createbitmapva.c,v 1.1 1994/09/24 03:33:02 ewhac Exp $ */

#include <types.h>
#include <folio.h>
#include <graphics.h>
#include <varargs_glue.h>


/***************************************************************************/

VAGLUE_FUNC (Item,
	CreateBitmapVA (VAGLUE_VA_TAGS),
	CreateBitmap   (VAGLUE_TAG_POINTER))
