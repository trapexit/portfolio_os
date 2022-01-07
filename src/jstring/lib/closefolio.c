/* $Id: closefolio.c,v 1.2 1994/10/31 23:47:09 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "operror.h"
#include "jstring_lib.h"


/****************************************************************************/


int32 CloseJStringFolio(void)
{
    if (!JStringBase)
        return BADPTR;

    return CloseItem(JStringBase->fn.n_Item);
}
