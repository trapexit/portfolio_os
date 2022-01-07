/***************************************************************\
*								*
* File:  sinf32.c						*
*								*
* Opera math routines						*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  15-Mar-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company                     *
* All rights reserved						*
* This program is proprietary and confidential			*
*								*
\***************************************************************/


#include "mathfolio.h"
extern ulong _sintable[];

/* #define CIRCLEUNITS 65536 */
/* #define CIRCLEBITS 16 */
#define CIRCLEUNITS 16384
#define CIRCLEBITS 14

#define CIRCLESHIFT (24-CIRCLEBITS)
#define CIRCLESCALE (1<<CIRCLESHIFT)
#define CIRCLEMASK (CIRCLESCALE-1)


void
SinF32 (frac32 *dest, frac16 angle)
/* Calculate the 32.32 sine of a 16.16 fraction (assuming there are 256.0 units in a circle) */
/* Alternatively, a 32 bit integer could be passed in assuming 16,777,216 units in a circle */
{
  long i, j, k, l, m;

  j = 0;
  i = angle & 0xffffff;
  if (i>0x800000) {
    j = 1;
    i = i-0x800000;
  }
  if (i>0x400000) {
    i = 0x800000-i;
  }
  k = i&CIRCLEMASK;
  i = i>>CIRCLESHIFT;
  l = (_sintable[i]&0xffff)*(CIRCLESCALE-k) + (_sintable[i+1]&0xffff)*k;
  m = (_sintable[i]>>16)*(CIRCLESCALE-k) + (_sintable[i+1]>>16)*k;
  dest->lo = (l>>(CIRCLESHIFT-1))+(m<<(17-CIRCLESHIFT));
  dest->hi = ((dest->lo>>(17-CIRCLESHIFT)) < m);
  if (j) {
    dest->hi = -dest->hi - (dest->lo!=0);
    dest->lo = -dest->lo;
  }
}



