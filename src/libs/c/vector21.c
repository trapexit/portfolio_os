/* $Id: vector21.c,v 1.1 1993/12/14 19:03:17 sdas Exp $ */

#include "types.h"
#include "varargs.h"
#include "folio.h"

extern struct KernelBase *KernelBase;

typedef int (*fp_print)(int ch, double *d, char buff[], int flags,
			char **lvprefix, int *lvprecision,
			int *lvbefore_dot, int *lvafter_dot);

int
__vfprintf(struct FILE *p, const char *fmt, va_list args,
           fp_print fp_display_fn, int (*putc)())
{
	int result;
	CALLFOLIORET(KernelBase,KBV_VFPRINTF,(p,fmt,args,fp_display_fn,putc),result,(int));
	return result;
}
