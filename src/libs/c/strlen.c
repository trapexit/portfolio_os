/* $Id: strlen.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
int strlen(a)
const char *a;
{
	const char *x = a + 1;
	while (*a++);
	return a-x;
}
