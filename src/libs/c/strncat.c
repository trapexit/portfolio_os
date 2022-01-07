/* $Id: strncat.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
char *
strncat(char *a, const char *b, int n)
      /* as strcat, but at most n chars */
{   char *p = a;
    while (*p != 0) p++;
    while (n-- > 0)
	if ((*p++ = *b++) == 0) return a;
    *p = 0;
    return a;
}
