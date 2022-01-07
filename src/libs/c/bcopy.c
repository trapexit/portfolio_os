/* $Id: bcopy.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
void
bcopy(s,d,n)
unsigned char *s,*d;
int n;
{
	while (n--)	*d++ = *s++;
}
