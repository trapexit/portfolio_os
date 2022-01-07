/* $Id: grafboot.c,v 1.4 1994/08/25 22:17:29 vertex Exp $ */

#define DEMAND_LOADING

#include "types.h"
#include "ctype.h"
#include "stdlib.h"
#include "debug.h"

#include "intgraf.h"


extern void	print_vinfo(void);



/* Quick hack to get printf to be small */
int32 printf(const char *fmt, ...);
int stdout;
void putc (int a)
{
  kprintf ("%c", a);
}


int
main (int argc, char *argv[])
{
  int i;

#ifdef DEMAND_LOADING
  if (argc != DEMANDLOAD_MAIN_CREATE)
      return 0;
#endif

  print_vinfo();

  i=InstallGraphicsFolio (argc, argv);

#ifndef DEMAND_LOADING
  if (i < 0)
      return i;

  WaitSignal (0);

#else
  return i;

#endif
}
