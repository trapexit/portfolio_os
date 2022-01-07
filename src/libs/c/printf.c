/* $Id: printf.c,v 1.6 1994/02/09 01:22:45 limes Exp $ */
/* printf.c: ANSI draft (X3J11 Oct 86) part of section 4.9 code */
/* Copyright (C) Codemist Ltd., 1988                            */
/* Copyright (C) Advanced Risc Machines Ltd., 1991              */

/* version 0.05b */

/* printf and its friends return the number of characters planted. In    */
/* the case of sprintf this does not include the terminating '\0'.       */
/* Consider using ftell instead of charcount in printf (see scanf).      */

#include "types.h"
#include "varargs.h"
#include "stdio.h"

typedef int (*fp_print)(int ch, double *d, char buff[], int flags,
                        char **lvprefix, int *lvprecision, int *lvbefore_dot,
                        int *lvafter_dot);

extern int _no_fp_display(int ch, double *d, char buff[], int flags,
                         char **lvprefix, int *lvprecision, int *lvbefore_dot,
                         int *lvafter_dot);

extern int _fp_display(int ch, double *d, char buff[], int flags,
                       char **lvprefix, int *lvprecision, int *lvbefore_dot,
                       int *lvafter_dot);

int __vfprintf(FILE *p, const char *fmt, va_list args, fp_print fp_display_fn, int32 (*putc)(char,FILE *));

int32 printf(const char *fmt, ...)
{
    va_list a;
    int n;
    va_start(a, fmt);
    n = __vfprintf(stdout, fmt, a, _no_fp_display, putc);
    va_end(a);
    return n;
}
