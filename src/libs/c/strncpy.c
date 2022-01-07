/* $Id: strncpy.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
char *
strncpy(char *a, const char *b, int n)
	    /* as strcpy, but at most n chars */
	    /* NB may not be nul-terminated   */
{   char *p = a;
    while (n-- > 0)
	if ((*p++ = *b++) == 0)
	{   char c = 0;
	    while (n-- > 0) *p++ = c;	/* ANSI says pad out with nul's */
	    return a;
	}
    return a;
}
