/* $Id: math64.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */
/* file: math64.c */
#include "types.h"

int
DTst(x)
ulong *x;
{
	/* return -1 if negative */
	/* return 0 if zero */
	/* return 1 if positive */
	ulong t = *x;
	if (t & 0x80000000)	return -1;
	if (t | x[1])	return 1;
	return 0;
}


void
DMul(x,y,r)
ulong x,y;
ulong *r;
{
	/* unsigned 32x32->64 bit multiply */
	/* This routine is misnamed since it */
	/* does not take 2 64 bit values as input */
	ulong xa = x>>16;
	ulong xb = x & 0xffff;
	ulong ya = y>>16;
	ulong yb = y & 0xffff;
	ulong ra,rb,t1,t2,t;
	
	rb = yb*xb;
	ra = ya*xa;

	t1 = xa*yb;
	t2 = xb*ya;

	ra += (t1>>16) + (t2>>16);
	t1 &= 0xffff;
	t2 &= 0xffff;

	t = t1+t2 + (rb>>16);

	ra += t>>16;

	rb = (rb&0xffff) | (t<<16);
	*r++ = ra;
	*r = rb;
}

void
DNot(x)
ulong *x;
{
	*x = ~(*x);
	x++;
	*x = ~(*x);
}

void
DAdd(x,y)
ulong *x,*y;
{
	ulong t,t1,t2;
	t1 = x[1];
	t2 = y[1];
	t = t1+t2;
	x[1] = t;
	if ( (t < t1) | (t < t2) ) t = 1;
	else t = 0;
	t += x[0] + y[0];
	x[0] = t;
}

void
DNeg(x)
ulong *x;
{
	ulong one[2];
	DNot(x);
	one[0] = 0;
	one[1] = 1;
	DAdd(x,one);
}

void
DSub(x,y)
ulong *x,*y;
{
	DNeg(y);
	DAdd(x,y);
}

int
DCmp(x,y)
ulong *x,*y;
{
	ulong a[2];

	a[0] = x[0];
	a[1] = x[1];
	DSub(a,y);
	return DTst(a);
}

