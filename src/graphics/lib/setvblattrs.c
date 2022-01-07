/* $Id: setvblattrs.c,v 1.3 1994/09/24 03:33:02 ewhac Exp $ */

#include <types.h>
#include <folio.h>
#include <graphics.h>
#include <varargs_glue.h>


/***************************************************************************/


Err
SetVBLAttrs (struct TagArg *args)
{
	Err	rval;

	CALLFOLIORET (GrafBase, _SETVBLATTRS_, (args), rval, (Err));
	return (rval);
}


VAGLUE_FUNC (Err,
	SetVBLAttrsVA (VAGLUE_VA_TAGS),
	SetVBLAttrs   (VAGLUE_TAG_POINTER))
