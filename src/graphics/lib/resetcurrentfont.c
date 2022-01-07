/* $Id: resetcurrentfont.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


int32
ResetCurrentFont( void )
{
	int32 i;

	CALLFOLIORET (GrafBase, _RESETFONT_, (), i, (int32));
	return i;
}
