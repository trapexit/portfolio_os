/* $Id: closefolio.c,v 1.2 1994/10/31 23:48:35 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "operror.h"
#include "compression_lib.h"


/****************************************************************************/


Err CloseCompressionFolio(void)
{
    if (!CompressionBase)
        return BADPTR;

    return CloseItem(CompressionBase->fn.n_Item);
}
