/* $Id: getvblattrs.c,v 1.3 1994/09/24 03:33:02 ewhac Exp $ */

#include <types.h>
#include <folio.h>
#include <graphics.h>
#include <varargs_glue.h>


/***************************************************************************/


Err
GetVBLAttrs (struct TagArg *args)
{
	Err	rval;

	CALLFOLIORET (GrafBase, _GETVBLATTRS_, (args), rval, (Err));
	return (rval);
}


VAGLUE_FUNC (Err,
	GetVBLAttrsVA (VAGLUE_VA_TAGS),
	GetVBLAttrs   (VAGLUE_TAG_POINTER))
