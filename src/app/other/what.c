/* $Id: what.c,v 1.1 1994/06/28 20:35:51 limes Exp $ */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>

int
main ()
{
    char                   *lomem = (char *) 0x000200;
    char                   *himem = (char *) 0x100000;
    char                   *p;
    int                     state = 0;
    int                     ch;
    int                     col = 0;
    int                     revs = 0;
    int			    bs;

/*
 * Find the end of memory.
 * We know that memory ends on a 1MB
 * boundrey, and is contiguous, so
 * allocate all memory and keep track
 * of the highest megabyte touched.
 */
    bs = 0x100000;
    do {
	while ((p = (char *)malloc(bs)) != (char *)0) {
	    p += bs;
	    if (p > himem)
		himem = (char *)(((uint32)p) | 0xFFFFF);
	}
    } while ((bs >>= 1) > 8);

    printf ("searching [0x%lx..0x%lx]\n", lomem, himem);
    p = lomem;
    do {
	ch = *p++;
	switch (state)
	{
	case 0:
	    if (ch == '@')
		state++;
	    else
		state = 0;
	    break;
	case 1:
	    if (ch == '(')
		state++;
	    else
		state = 0;
	    break;
	case 2:
	    if (ch == '#')
		state++;
	    else
		state = 0;
	    break;
	case 3:
	    if (ch == ')')
	    {
		state++;
		revs++;
/*		printf ("%6lx:", p-3); */
	    }
	    else
		state = 0;
	    break;
	case 4:
	    if ((ch == '\0') || (ch == '\n') || (ch == '\r'))
	    {
		printf ("\n");
		col = 0;
		state = 0;
		break;
	    }
	    if (col > 72)
	    {
		printf (" ...");
		break;
	    }
	    if ((ch == ' ') && (col == 0))
		break;
	    printf ("%c", ch);
	    ++col;
	    break;
	}
    } while (p <= himem);
    if (col > 0)
    {
	printf ("\n");
    }
    printf ("%d revision strings found and printed\n", revs);
    return 0;
}
