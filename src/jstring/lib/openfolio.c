/* $Id: openfolio.c,v 1.3 1994/09/21 00:02:07 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "jstring_lib.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *jstringlib_version;
static void *jstrlib_version = &jstringlib_version;
#endif


/****************************************************************************/


Folio *JStringBase;


/****************************************************************************/


int32 OpenJStringFolio(void)
{
Item it;

    it = FindAndOpenFolio(JSTR_FOLIONAME);
    if (it >= 0)
        JStringBase = (Folio *)LookupItem(it);

    return (it);
}
