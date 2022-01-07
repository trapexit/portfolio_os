/* $Id: closefolio.c,v 1.4 1994/10/31 23:43:06 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "operror.h"
#include "international_lib.h"


/****************************************************************************/


int32 intlCloseFolio(void)
{
    if (!InternationalBase)
        return BADPTR;

    return CloseItem(InternationalBase->fn.n_Item);
}
