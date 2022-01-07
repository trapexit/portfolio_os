/* $Id: rand.c,v 1.5 1994/06/23 19:57:12 limes Exp $ */
/* stdlib.c: ANSI draft (X3J11 Oct 86) library, section 4.10 */
/* Copyright (C) Codemist Ltd, 1988			     */
/* Copyright (C) Advanced Risc Machines Ltd., 1991	     */
/* version 0.02a */

/*
 * $Log: rand.c,v $
 * Revision 1.5  1994/06/23  19:57:12  limes
 * tune urand for ARM smoke
 *
 * Revision 1.4  1994/06/23  19:49:20  limes
 * - simplify global static names, they don't leave this file
 * - change addressing of state array from 1-based to 0-based
 * - change usage of "k" and "j" to predecrement
 * - simplify j,k decrement/wrap code
 *
 */

/* The random-number generator that the world is expected to use */

static unsigned state[] =
/* The values here are just those that would be put in this horrid
   array by a call to __srand(1). DO NOT CHANGE __srand() without
   making a corresponding change to these initial values.
*/
{               0x66d78e85, 0xd5d38c09, 0x0a09d8f5, 0xbf1f87fb,
    0xcb8df767, 0xbdf70769, 0x503d1234, 0x7f4f84c8, 0x61de02a3,
    0xa7408dae, 0x7a24bde8, 0x5115a2ea, 0xbbe62e57, 0xf6d57fff,
    0x632a837a, 0x13861d77, 0xe19f2e7c, 0x695f5705, 0x87936b2e,
    0x50a19a6e, 0x728b0e94, 0xc5cc55ae, 0xb10a8ab1, 0x856f72d7,
    0xd0225c17, 0x51c4fda3, 0x89ed9861, 0xf1db829f, 0xbcfbc59d,
    0x83eec189, 0x6359b159, 0xcc505c30, 0x9cbc5ac9, 0x2fe230f9,
    0x39f65e42, 0x75157bd2, 0x40c158fb, 0x27eb9a3e, 0xc582a2d9,
    0x0569d6c2, 0xed8e30b3, 0x1083ddd2, 0x1f1da441, 0x5660e215,
    0x04f32fc5, 0xe18eef99, 0x4a593208, 0x5b7bed4c, 0x8102fc40,
    0x515341d9, 0xacff3dfa, 0x6d096cb5, 0x2bb3cc1d, 0x253d15ff
};

#define	STATE_SIZE     (sizeof state / sizeof state[0])

static int j = 23, k = 0;

unsigned int urand()
{
/* See Knuth vol 2 section 3.2.2 for a discussion of this random
   number generator.
   */
    int lj, lk;
    lj = j, lk = k;
    if (--lj < 0) lj = STATE_SIZE-1;
    if (--lk < 0) lk = STATE_SIZE-1;
    j = lj, k = lk;
    return state[lk] += state[lj];	/* 32 bit result */
}

int rand()
{
    return (urand() & 0x7fffffff); 	/* result is a 31-bit value */
}

void srand(unsigned int seed)
{
/* This only allows you to put 32 bits of seed into the random sequence,
   but it is very improbable that you have any good source of randomness
   that good to start with anyway! A linear congruential generator
   started from the seed is used to expand from 32 to 32*STATE_SIZE bits.
*/
    int i;
    j = 23;
    k = 0;
    for (i = 0; i<STATE_SIZE; i++)
    {
/* This is not even a good way of setting the initial values.  For instance */
/* a better scheme would have r<n+1> = 7^4*r<n> mod (3^31-1).  Still I will */
/* leave this for now.							    */
	seed = 69069*seed + 1725307361;  /* computed modulo 2^32 */
	state[i] = seed + (seed >> 16);
    }
}

static unsigned long int next = 1;

int _ANSI_rand()     /* This is the ANSI suggested portable code */
{
    next = next * 1103515245 + 12345;
    return (unsigned int) (next >> 16) % 32768;
}

void _ANSI_srand(unsigned int seed)
{
    next = seed;
}
