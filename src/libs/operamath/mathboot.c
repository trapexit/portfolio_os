/* $Id: mathboot.c,v 1.4 1994/08/25 22:15:38 vertex Exp $ */

#define DEMAND_LOADING

#include "mathfolio.h"
#include "debug.h"


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
Item result;

#ifdef DEMAND_LOADING
  if (argc != DEMANDLOAD_MAIN_CREATE)
      return 0;
#endif

  print_vinfo();

  result = InstallMathFolio (argc, argv);

#ifndef DEMAND_LOADING
  if (result < 0)
      return (int) result;

  WaitSignal (0);

#else
  return (int) result;

#endif
}
