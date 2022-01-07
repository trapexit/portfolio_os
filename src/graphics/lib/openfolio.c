/* $Id: openfolio.c,v 1.3 1994/09/21 00:10:35 vertex Exp $ */

#include "types.h"
#include "folio.h"
#include "stdio.h"
#include "graphics.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *graphicslib_version;
static void *graplib_version = &graphicslib_version;
#endif


/*****************************************************************************/


Item GrafFolioNum;
GrafFolio *GrafBase;

#define DEBUGGRAF(x) /* printf x */


int32
OpenGraphicsFolio (void)
{
  DEBUGGRAF (("Opening Graphics Folio\n"));
  GrafFolioNum =
    FindAndOpenFolio("Graphics");
  if (GrafFolioNum<0) {
    return GrafFolioNum;
  }

  GrafBase = (GrafFolio *)LookupItem (GrafFolioNum);
  DEBUGGRAF (("GrafBase located at %lx\n", GrafBase));

  return 0;
}
