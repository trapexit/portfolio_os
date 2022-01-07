/* $Id: getcurrentfont.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


Font*
GetCurrentFont( void )
{
  Font* rval;
  CALLFOLIORET (GrafBase, _GETCURRENTFONT_, ( ), rval, (Font*));
  return rval;
}
