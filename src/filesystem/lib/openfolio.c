/* $Id: openfolio.c,v 1.2 1994/09/21 00:13:44 vertex Exp $ */

#include "filefolio_lib.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *filesystemlib_version;
static void *filelib_version = &filesystemlib_version;
#endif


/*****************************************************************************/


Item FileFolioNum;
FileFolio *FileFolioBase;


void
OpenFileFolio(void)
{
    FileFolioNum = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),"File"), 0);
    FileFolioBase = (FileFolio *)LookupItem(FileFolioNum);
}
