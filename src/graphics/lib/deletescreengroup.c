/* $Id: deletescreengroup.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


Err
DeleteScreenGroup (Item sgi)
{
  Err r;
  CALLFOLIORET (GrafBase, _DELETESCREENGROUP_, (sgi), r, (Err));
  return r;
}
