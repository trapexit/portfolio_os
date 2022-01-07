/* $Id: chval.c,v 1.2 1994/02/09 01:22:45 limes Exp $ */
#include "types.h"
#include "ctype.h"

int _chval(int ch, int radix)
{
    int val;
    /* Memo: beware that ANSI currently say a-z act as 10-35 for strtol() */
    /* etc.  The test below is isalpha() etc.  This means that this test  */
    /* may not work in a non-C locale where isalpha('{') may be true	  */
    /* (e.g. Swedish ASCII).						  */
    if ('A' == 193)  /* ebcdic */
	val = (isdigit(ch) ? (ch) - '0' :
	       isalpha(ch) ? (ch |= 0x40,	  /* quick ebcdic toupper */
		   ch <= 'I' ? ch - 'A' + 10 :
		   ch <= 'R' ? ch - 'J' + 19 :
			       ch - 'S' + 28) :
	       -1);
    else
	val = (isdigit(ch) ? (ch) - '0' :
	       islower(ch) ? (ch) - 'a' + 10 :
	       isupper(ch) ? (ch) - 'A' + 10 :
	       -1);
    return (val < radix ? val : -1);
}
