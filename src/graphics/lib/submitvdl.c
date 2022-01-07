/* $Id: submitvdl.c,v 1.1 1994/05/25 00:11:58 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "graphics.h"


/*****************************************************************************/


Item
SubmitVDL( VDLEntry *VDLDataPtr, int32 length, int32 type )
{
  Item rval;
  CALLFOLIORET (GrafBase, _SUBMITVDL_, ( VDLDataPtr, length, type ), rval, (Item));
  return rval;
}
