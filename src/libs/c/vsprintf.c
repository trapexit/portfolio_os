/* $Id: vsprintf.c,v 1.4 1994/04/22 21:32:11 vertex Exp $ */

#include "types.h"
#include "stdarg.h"


/*****************************************************************************/


typedef struct SprintfData
{
    char *sd_Dest;
} SprintfData;


/*****************************************************************************/


static int32 vsprintfputc(char c, SprintfData *sd)
{
    *sd->sd_Dest++ = c;
    return c;
}


/*****************************************************************************/


extern int __vfprintf(SprintfData *sd, const char *fmt, va_list args, void *, int32 (*putc)(char,SprintfData *));


int32 vsprintf(char *buff, const char *fmt, va_list args)
{
SprintfData sd;
int32       len;

    sd.sd_Dest = buff;

    len = __vfprintf(&sd, fmt, args, NULL, vsprintfputc);
    vsprintfputc(0,&sd);

    return(len);
}
