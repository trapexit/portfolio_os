/* $Id: perror.c,v 1.4 1994/09/08 17:34:22 vertex Exp $ */

#include "types.h"
#include "stdio.h"

void perror(char *s)
{
    printf("System perror: ");
    if (s) printf("%s\n",s);
}

