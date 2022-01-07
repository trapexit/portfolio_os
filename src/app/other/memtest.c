/* $Id: memtest.c,v 1.1 1994/05/06 22:28:23 limes Exp $ */

#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <operror.h>

/*
 * memory system test for anvil:
 * 
 */

#define	TriggerScope()							\
{									\
	unsigned	junk;						\
	junk = *(volatile unsigned *)(0x00000100);			\
}

#define	CHECK(iix, iexp)						\
{									\
	unsigned	act;						\
	unsigned	exp;						\
	unsigned	index;						\
	exp = (unsigned)(iexp);						\
	index = (unsigned)(iix);					\
	act = p[index];							\
	if (act != exp) {						\
TriggerScope();								\
printf("index 0x%08X at 0x%08X act=0x%08X exp=0x%08X xor=0x%08X\n",	\
		       index, p+index,					\
		       act, exp, act ^ exp);				\
	}								\
}

unsigned	pats[] = {
	0x00000000, 0xFFFFFFFF, 0x55555555, 0xAAAAAAAA,
	0xDEADBEEF, 0xFEEDC0ED, 0xBABECAFE, 0x3D03D0FF,
};

#define	NPATS	((sizeof pats) / (sizeof pats[0]))

#define	CHUNKSIZE	32

unsigned *
getblock (unsigned *sizep)
{
	unsigned *p;
	unsigned	size;
	size = *sizep;

	while (1) {
		p = (unsigned *)malloc(size * sizeof *p);
		if (p != 0) {
			*sizep = size;
			return p;
		}
		if (size <= CHUNKSIZE) {
			*sizep = 0;
			return 0;
		}
		size >>= 1;
	}
}

void
test(unsigned *p, unsigned size)
{
	unsigned	va;
	unsigned	ix;
	unsigned	npat;
	unsigned	px;
	unsigned	opat;
	
	npat = 0;
	for (ix = 0; ix < size; ++ix)
		p[ix] = npat;

	for (ix = 0; ix < size; ++ix) {
		va = (unsigned)(p + ix);
		CHECK(ix, npat);
		p[ix] = va;
	}

	for (ix = 0; ix < size; ++ix) {
		va = (unsigned)(p + ix);
		CHECK(ix, va);
		p[ix] = npat;
	}

	ix = size;
	while (ix-- > 0) {
		va = (unsigned)(p + ix);
		CHECK(ix, npat);
		p[ix] = va;
	}

	ix = size;
	while (ix-- > 0) {
		va = (unsigned)(p + ix);
		CHECK(ix, va);
		p[ix] = npat;
	}

	for (px = 0; px < 32; ++px) {
		opat = npat;
		npat = 1<<px;
		for (ix = 0; ix < size; ++ix) {
			CHECK(ix, opat);
			p[ix] = npat;
		}
	}

	for (px = 0; px < 32; ++px) {
		opat = npat;
		npat = ~(1<<px);
		for (ix = 0; ix < size; ++ix) {
			CHECK(ix, opat);
			p[ix] = npat;
		}
	}

	for (px = 0; px < NPATS; ++px) {
		opat = npat;
		npat = pats[px];
		for (ix = 0; ix < size; ++ix) {
			CHECK(ix, opat);
			p[ix] = npat;
		}
	}

#ifdef	NRANDS
	for (px = 0; px < NRANDS; ++px) {
		opat = npat;
		npat = SuperReadHardwareRandomNumber();
		for (ix = 0; ix < size; ++ix) {
			CHECK(ix, opat);
			p[ix] = npat;
		}
	}
#endif

	opat = npat;
	for (ix = 0; ix < size; ++ix)
		CHECK(ix, opat);
}

int
main (int ac, char **av)
{
	unsigned	*p;
	unsigned	size;
	unsigned	ofst;

	size = 4<<20;

	p = getblock(&size);
	if ((p == 0) || (size < CHUNKSIZE)) {
		printf("memtest aborted, could not get memory\n");
		return 0;
	}

	while (1) {
		printf("testing p=0x%06X size=0x%06Xw (%dkb) end=0x%06X\n",
		       p, size, size>>8, p+size);
		test(p, size);
		printf("testing chunks ...\n");

		for (ofst = 0; ofst < size; ofst += CHUNKSIZE)
			test (p+ofst, CHUNKSIZE);
	}
}
