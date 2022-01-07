/* $Id: readclutcolor.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


RGB888
ReadCLUTColor (ulong index)
{
	RGB888 c;
	CALLFOLIORET (GrafBase, _READVDLCOLOR_, (index), c, (RGB888));
	return c;
}
