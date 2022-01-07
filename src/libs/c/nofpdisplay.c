/* $Id: nofpdisplay.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
/* printf.c: ANSI draft (X3J11 Oct 86) part of section 4.9 code */
/* Copyright (C) Codemist Ltd., 1988                            */
/* Copyright (C) Advanced Risc Machines Ltd., 1991              */

/* version 0.05b */

/* printf and its friends return the number of characters planted. In    */
/* the case of sprintf this does not include the terminating '\0'.       */
/* Consider using ftell instead of charcount in printf (see scanf).      */

#include "types.h"
#include "stdio.h"
#include "stdarg.h"
#include "ctype.h"
#include "string.h"

int _no_fp_display(int ch, double *d, char buff[], int flags,
                   char **lvprefix, int *lvprecision, int *lvbefore_dot,
                   int *lvafter_dot)
{
    ch = ch;
    d = d;
    buff = buff;
    flags = flags;
    lvprefix = lvprefix;
    lvprecision = lvprecision;
    lvbefore_dot = lvbefore_dot;
    lvafter_dot = lvafter_dot;
    return 0;
}
