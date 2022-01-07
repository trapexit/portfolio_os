/* $Id: gtest.c,v 1.2 1994/05/13 21:48:27 slandrum Exp $ */


#include "types.h"
#include "ctype.h"
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "graphics.h"


#define CHK_ERR(x,y) {if ((x)<0) {printf("Error - %s\n",y);PrintfSysErr(x);}}

int
main (int argc, char **argv)
{
  Err e;
  static uint32 i, j;
  TagArg qtags[] = {
    QUERYGRAF_TAG_FIELDFREQ, (void*)&i,
    QUERYGRAF_TAG_FIELDTIME, (void*)&j,
    QUERYGRAF_TAG_END, (void*) 0,
  };

  if ((e=OpenGraphicsFolio()) < 0) {
    printf ("Error opening graphics folio\n");
    PrintfSysErr (e);
    exit ((int)e);
  }

  i = 0; j = 0;
  
  e = QueryGraphics ( QUERYGRAF_TAG_FIELDFREQ, &i);
  CHK_ERR (e,"QueryGraphics");
  printf ("Video field frequency = %d\n", i);
  e = QueryGraphics ( QUERYGRAF_TAG_FIELDTIME, &j);
  CHK_ERR (e,"QueryGraphics");
  printf ("Video field time = %d usec.\n", j);

  i = 0; j = 0;

  e = QueryGraphicsList (qtags);
  CHK_ERR (e,"QueryGraphicsList");
  printf ("Video field frequency = %d\n", i);
  printf ("Video field time = %d usec.\n", j);

  exit (0);
}
