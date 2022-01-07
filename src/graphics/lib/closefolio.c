/* $Id: closefolio.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "item.h"
#include "graphics.h"


/*****************************************************************************/


Err
CloseGraphicsFolio (void)
{
  return CloseItem (GrafFolioNum);
}
