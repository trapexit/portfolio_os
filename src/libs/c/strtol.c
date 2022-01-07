/* $Id: strtol.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
#include "types.h"
#include "strings.h"
#include "ctype.h"

#include "scanf.h"

extern ulong _strtoul(char *,char **,int);

long int strtol(const char *nsptr, char **endptr, int base)
{
/* The specification in the ANSI information bulletin upsets me here:	 */
/* strtol is of type long int, and 'if the correct value would cause	 */
/* overflow LONG_MAX or LONG_MIN is returned'. Thus for hex input the	 */
/* string 0x80000000 will be considered to have overflowed, and so will  */
/* be returned as LONG_MAX.						 */
/* These days one should use strtoul for unsigned values, so some of	 */
/* my worries go away.							 */

/* This code is NOT shared with the %i conversion in scanf for several	 */
/* reasons: (a) here I deal with overflow in a silly way as noted above, */
/* (b) in scanf I have to deal with field width limitations, which does  */
/* not fit in neatly here (c) this functions scans an array of char,	 */
/* while scanf reads from a stream - fudging these together seems too	 */
/* much work, (d) here I have the option of specifying the radix, while  */
/* in scanf there seems to be no provision for that. Ah well!		 */

    const unsigned char *nptr = (const unsigned char *)nsptr;  /* see scanf */
    int flag = 0, c;
    while ((c = *nptr++)!=0 && isspace(c));
    switch (c)
    {
case '-': flag |= NUMNEG;
	  /* drop through */
case '+': break;
default:  nptr--;
	  break;
    }
    {	char *endp;
	unsigned long ud = _strtoul((char *)nptr, &endp, base);
	if (endptr) *endptr = endp==(char *)nptr ? (char *)nsptr : endp;
/* The following lines depend on the fact that unsigned->int casts and	 */
/* unary '-' cannot cause arithmetic traps.  Recode to avoid this?	 */
#ifdef ERRNO
	if (flag & NUMNEG)
	    return (-(long)ud <= 0) ? -(long)ud : (errno = ERANGE, LONG_MIN);
	else
	    return (+(long)ud >= 0) ? +(long)ud : (errno = ERANGE, LONG_MAX);
#else
	if (flag & NUMNEG) ud = -ud;
	    return (long)ud;
#endif
    }
}
