/* $Id: strchr.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
char *strchr(const char *s, char ch)
					/* find first instance of ch in s */
{   char c1 = ch;
    for (;;)
    {	char c = *s++;
	if (c == c1) return (char *)s-1;
	if (c == 0) return 0;
    }
}
