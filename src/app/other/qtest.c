/* $Id: qtest.c,v 1.3 1994/05/06 22:58:28 limes Exp $ */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>

typedef	vuint32		       *pvuint32;

int
main(int ac, char **av)
{
	uint32		a;
	pvuint32	p;
	uint32		v;
	uint32		w;

	/* need to find a longword at 0x00xx0010 */
	a = (unsigned)malloc(65<<10);
	a |= 0xFFFF;
	a += 0x0011;
	p = (pvuint32) a;
	*p = 0;
	printf("watching memory cell at 0x%08X\n", p);
	while (1) {

		do {
			/* SPIN */
		} while ((v = *p) == 0);
		w = *p;

		*p = 0;

		printf("mem at 0x%08X changed to 0x%08X (2nd read 0x%08X)\n",
		       p, v, w);
	}
}
