/* $Id: strcat.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
char *strcat(char *a, const char *b)	/* concatenate b on the end of a */
{   char *p = a;
    while (*p != 0) p++;
    while ((*p++ = *b++) != 0);
    return a;
}
