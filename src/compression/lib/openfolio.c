/* $Id: openfolio.c,v 1.4 1994/09/21 00:02:59 vertex Exp $ */

#include "types.h"
#include "nodes.h"
#include "folio.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "compression_lib.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *compressionlib_version;
static void *complib_version = &compressionlib_version;
#endif


/****************************************************************************/


Folio *CompressionBase;


/****************************************************************************/


Err OpenCompressionFolio(void)
{
Item it;

    it = FindAndOpenFolio(COMP_FOLIONAME);
    if (it >= 0)
        CompressionBase = (Folio *)LookupItem(it);

    return (int)it;
}
