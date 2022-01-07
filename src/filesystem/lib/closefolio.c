/* $Id: closefolio.c,v 1.1 1994/07/07 18:57:28 vertex Exp $ */

#include "filefolio_lib.h"


/****************************************************************************/


void CloseFileFolio(void)
{
    CloseItem(FileFolioNum);
}
