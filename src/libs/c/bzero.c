/* $Id: bzero.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
void
bzero(s,n)
unsigned char *s;
int n;
{
	while (n--)	*s++ = 0;
}
