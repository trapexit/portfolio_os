/*	$Id: t_rsa.c,v 1.14 1994/09/20 20:12:03 markn Exp $
**
**	code for RSA and MD5 checking
**
**	3DO Confidential -- Contains 3DO Trade Secrets -- internal use only
*/

#include "types.h"
#include "global.h"
#include "bsafe2.h"
#include "dipir.h"

/*#define DEBUG2*/

/* If the standard C library comes with a memmove() that correctly
     handles overlapping buffers, MEMMOVE_PRESENT should be defined as
     1, else 0.
   The following defines MEMMOVE_PRESENT as 1 if it has not already been
     defined as 0 with C compiler flags.
 */


void T_memset (p, c, count)
POINTER p;
int c;
unsigned int count;
{
#ifdef DEBUG2
  PUTS("T_memset(");PUTHEX(p);PUTS(",");PUTHEX(c);PUTS(",");PUTHEX(count);PUTS(")");
#endif
  while (count--)	*p++ = c;
}

void T_memmove (d, s, count)
POINTER d, s;
unsigned int count;
{
  unsigned int i;

#ifdef DEBUG2
  PUTS("T_memmove");
#endif
  if ((char *)d == (char *)s)
    return;
  else if ((char *)d > (char *)s) {
    for (i = count; i > 0; i--)
      ((char *)d)[i-1] = ((char *)s)[i-1];
  }
  else {
    for (i = 0; i < count; i++)
      ((char *)d)[i] = ((char *)s)[i];
  }
}

void T_memcpy (d, s, count)
POINTER d, s;
unsigned int count;
{
#ifdef DEBUG2
  PUTS("T_memcpy");
#endif
  if (count != 0)
    T_memmove (d, s, count);
}

/*
 *	"oops!"  (ask Dale for details)
 */
int
B_memcmp(const char *a, const char *b, int n)
{
    while (n > 0)
    {
	if( (*a++) != (*b++) )
		return 1;
	n--;
    }
    return 0;
}

