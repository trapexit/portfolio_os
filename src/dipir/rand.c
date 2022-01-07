/* $Id: rand.c,v 1.7 1995/01/26 22:11:20 markn Exp $ */

/*
 * Use simple and small version of random number generator for dipir.
 * This is the ANSI-recommended version.
 */

static unsigned long int next = 1;

unsigned int urand()
{
	next = next * 1103515245 + 12345;
	return (unsigned int) next;
}

void srand(unsigned int seed)
{
	next = seed;
}
