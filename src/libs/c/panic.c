/* $Id: panic.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */

extern void printf(char *, ...);

void Panic(s)
char *s;
{
	printf("Panic: %s\n",s);
	while (1);
}
