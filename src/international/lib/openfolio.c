/* $Id: openfolio.c,v 1.9 1994/09/21 00:03:42 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "folio.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "international_lib.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *internationallib_version;
static void *intllib_version = &internationallib_version;
#endif


/****************************************************************************/


Folio *InternationalBase;


/****************************************************************************/


int32 intlOpenFolio(void)
{
Item it;

    it = FindAndOpenFolio(INTL_FOLIONAME);
    if (it >= 0)
        InternationalBase = (Folio *)LookupItem(it);

    return (it);
}
