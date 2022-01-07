/* $Id: sprintf.c,v 1.4 1994/04/22 21:32:11 vertex Exp $ */

#include "types.h"
#include "stdarg.h"
#include "stdio.h"


/*****************************************************************************/


typedef struct SprintfData
{
    char *sd_Dest;
} SprintfData;


/*****************************************************************************/


static int32 sprintfputc(char c, SprintfData *sd)
{
    *sd->sd_Dest++ = c;
    return c;
}


/*****************************************************************************/


extern int __vfprintf(SprintfData *sd, const char *fmt, va_list args, void *, int32 (*putc)(char,SprintfData *));


int32 sprintf(char *buff, const char *fmt, ...)
{
SprintfData sd;
va_list     args;
int32       len;

    sd.sd_Dest = buff;

    va_start(args, fmt);

    len = __vfprintf(&sd, fmt, args, NULL, sprintfputc);
    sprintfputc(0,&sd);

    va_end(a);

    return(len);
}
