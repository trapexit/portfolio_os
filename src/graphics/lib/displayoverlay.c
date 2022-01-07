/* $Id: displayoverlay.c,v 1.1 1994/05/31 22:28:53 ewhac Exp $ */

#include <types.h>
#include <folio.h>
#include <graphics.h>


/*****************************************************************************/


Item
DisplayOverlay (Item bitmap, int32 topedge)
{
  int32 rval;
  CALLFOLIORET (GrafBase, _DISPLAYOVERLAY_, (bitmap, topedge), rval, (Item));
  return rval;
}
