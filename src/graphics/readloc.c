/* $Id: readloc.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */


#include "types.h"
#include "ctype.h"
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"


int
main (int argc, char **argv)
{
  uint32 a, b;

  if (argc>1) {
  } else {
    printf ("\nUsage\n\n\t%s <address>\n", argv[0]);
    exit (1);
  }
  a = strtoul (argv[1], 0, 0);
  b = *((uint32 *)a);
  printf ("Location 0x%lx contains 0x%lx\n", a, b);
  exit (0);
}
