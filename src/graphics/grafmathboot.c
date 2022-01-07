

#include "types.h"
#include "ctype.h"
#include "stdlib.h"
#include "debug.h"

#include "intgraf.h"
#include "mathfolio.h"


#ifdef	DEVELOPER
extern void	print_vinfo(void);
#endif



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

#ifdef	DEVELOPER
  print_vinfo();
#endif

  if ( (i=InstallMathFolio (argc, argv)) < 0) {
    return i;
  }
  if ( (i=InstallGraphicsFolio (argc, argv)) < 0) {
    return i;
  }

  WaitSignal (0);
}

