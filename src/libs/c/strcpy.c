/* $Id: strcpy.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */

char *
strcpy(char *a, const char *b)
{
	char *p = a;
	while ((*p++ = *b++) != 0);
	return a;
}
