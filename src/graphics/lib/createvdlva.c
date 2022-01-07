/* $Id: createvdlva.c,v 1.1 1994/09/24 03:33:02 ewhac Exp $ */

#include <types.h>
#include <folio.h>
#include <graphics.h>
#include <varargs_glue.h>


/***************************************************************************/

VAGLUE_FUNC (Item,
	CreateVDLVA (VAGLUE_VA_TAGS),
	CreateVDL   (VAGLUE_TAG_POINTER))
