/* $Id: strcmp.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
int
strcmp(const char *a,const char *b)
{
    for (;;)
    {	char c1 = *a++, c2 = *b++;
	int d = c1 - c2;
	if (d != 0) return d;
	if (c1 == 0) return 0;	   /* no need to check c2 */
    }
}
