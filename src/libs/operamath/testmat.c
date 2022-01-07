/***************************************************************\
*								*
* Simple task to test math routines				*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  14-Jun-93					*
*								*
* Copyright (c) 1992, 1993 The 3DO Company                      *
* All rights reserved						*
* This program is proprietary and confidential			*
*								*
\***************************************************************/

#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) /* { printf x ; } */

#include "types.h"
#include "stdlib.h"

#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
/* #include "rom.h" */

#include "strings.h"
#include "stdio.h"

#ifndef GFILE
#define GFILE "graphics.h"
#endif
#include GFILE
#include "operamath.h"

struct ccbin {
  int32 w, h;
  int32 x01, y01;
  int32 x03, y03;
  int32 x0123, y0123;
  int64 n;
};

struct ccbout {
  int32 hdx, hdy;
  int32 vdx, vdy;
  int32 ddx, ddy;
};


void testmat3 (int32 it);
void testmatdiv (int32 it);
void testmat4 (int32 it);
void testdiv (int32 it);
void testccb (int32 it);


int
main (int argc, char **argv)
{
  int32 i;
  char opt;

  DBUG (("Opera math matrix hardware test program\n"));

  if (OpenMathFolio() < 0) {
    DBUG (("Error opening math folio\n"));
    return (1);
  }

  opt = '4';
  if (argc>1) {
    opt = *argv[1];
  }
  for (i=0; ; i++) {
    switch (opt) {
    case 'd':
      testdiv (i);
      break;
    case '3':
      testmat3 (i);
      break;
    case '4':
      testmat4 (i);
      break;
    case 'm':
      testmatdiv (i);
      break;
    case 'c':
      testccb (i);
      break;
    default:
      DBUG (("Whoa, dude!  Unrecognized option (%s)\n", argv[1]));
      exit (1);
    }
    if (i%10000==0) {
      DBUG (("."));
    }
  }
}


void
testdiv (int32 it)
{
  int32 i;
/*  int32 j; */
  int64 l, m, n, o;

  do {
    i = urand();
  } while (i>0x55555556 || i<-0x55555556);
  l.hi = urand();
  l.lo = urand();

/*  i &= 0xffff; */
/*  l.hi = 0x10000; */
/*  l.lo = 0; */

/*  j = qdiv (&l, i); */
  m.lo = i<<16;
  m.hi = i>>16;
  DivS64 (&n, &o, &l, &m);
/*  j = getnz(); */

#if 0
  if (j != n.lo) {
    DBUG (("\nError (iteration %ld)\n", it));
    DBUG (("n = %08lx %08lx\n", l.hi, l.lo));
    DBUG (("d = %08lx\n", i));
    DBUG (("hardware: %08lx\n", j));
    DBUG (("software: %08lx\n", n.lo));
/*    DBUG (("getnz: %08lx\n", getnz())); */
  }
#endif
}


void
testccb (int32 it)
{
  struct ccbin s;
  struct ccbout t, u;
  int32 wi, hi, whi;
  int64 m, n, o;

  do {
    s.w = urand()&0x7fffffff;
  } while (s.w>0x55555556 || s.w<-0x55555556);
  do {
    s.h = urand()&0x7fffffff;
  } while (s.h>0x55555556 || s.h<-0x55555556);
  s.x01 = urand();
  s.y01 = urand();
  s.x03 = urand();
  s.y03 = urand();
  s.x0123 = urand();
  s.y0123 = urand();
  s.n.hi = 0x10000;
  s.n.lo = 0x0;
/*  doccb (&s, &t); */

  m.lo = s.w<<16;
  m.hi = s.w>>16;
  DivS64 (&n, &o, &s.n, &m);
  wi = n.lo;
  m.lo = s.h<<16;
  m.hi = s.h>>16;
  DivS64 (&n, &o, &s.n, &m);
  hi = n.lo;
  MulSF16_F32 (&m, wi, hi);
  whi = ConvertF32_F16 (&m);
  MulSF16_F32 (&m, wi, s.x01);
  u.hdx = ConvertF32_F16 (&m);
  MulSF16_F32 (&m, wi, s.y01);
  u.hdy = ConvertF32_F16 (&m);
  MulSF16_F32 (&m, hi, s.x03);
  u.vdx = ConvertF32_F16 (&m);
  MulSF16_F32 (&m, hi, s.y03);
  u.vdy = ConvertF32_F16 (&m);
  MulSF16_F32 (&m, whi, s.x0123);
  u.ddx = ConvertF32_F16 (&m);
  MulSF16_F32 (&m, whi, s.y0123);
  u.ddy = ConvertF32_F16 (&m);

  if (t.hdx!=u.hdx || t.hdy!=u.hdy ||
      t.vdx!=u.vdx || t.vdy!=u.vdy || t.ddx!=u.ddx || t.ddy!=u.ddy) {
    DBUG (("\nError (iteration %ld)\n", it));
    DBUG (("--- hardware:\n"));
    DBUG (("hdx, hdy = %08lx, %08lx\n", t.hdx, t.hdy));
    DBUG (("vdx, vdy = %08lx, %08lx\n", t.vdx, t.vdy));
    DBUG (("ddx, ddy = %08lx, %08lx\n", t.ddx, t.ddy));
    DBUG (("--- software:\n"));
    DBUG (("hdx, hdy = %08lx, %08lx\n", u.hdx, u.hdy));
    DBUG (("vdx, vdy = %08lx, %08lx\n", u.vdx, u.vdy));
    DBUG (("ddx, ddy = %08lx, %08lx\n", u.ddx, u.ddy));
    DBUG (("--- inputs:\n"));
    DBUG (("w, h = %08lx, %08lx\n", s.w, s.h));
    DBUG (("x01, y01 = %08lx, %08lx\n", s.x01, s.y01));
    DBUG (("x03, y03 = %08lx, %08lx\n", s.x03, s.y03));
    DBUG (("x0123, y0123 = %08lx, %08lx\n", s.x0123, s.y0123));
  }
}


void
tv4 (vec4f16 d, vec4f16 v, mat44f16 m)
{
  frac32 f1, f2, f3, f4;

  MulSF16_F32 (&f1, v[0], m[0][0]);
  MulSF16_F32 (&f2, v[1], m[1][0]);
  MulSF16_F32 (&f3, v[2], m[2][0]);
  MulSF16_F32 (&f4, v[3], m[3][0]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  AddF32 (&f1, &f1, &f4);
  d[0] = ConvertF32_F16 (&f1);

  MulSF16_F32 (&f1, v[0], m[0][1]);
  MulSF16_F32 (&f2, v[1], m[1][1]);
  MulSF16_F32 (&f3, v[2], m[2][1]);
  MulSF16_F32 (&f4, v[3], m[3][1]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  AddF32 (&f1, &f1, &f4);
  d[1] = ConvertF32_F16 (&f1);

  MulSF16_F32 (&f1, v[0], m[0][2]);
  MulSF16_F32 (&f2, v[1], m[1][2]);
  MulSF16_F32 (&f3, v[2], m[2][2]);
  MulSF16_F32 (&f4, v[3], m[3][2]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  AddF32 (&f1, &f1, &f4);
  d[2] = ConvertF32_F16 (&f1);

  MulSF16_F32 (&f1, v[0], m[0][3]);
  MulSF16_F32 (&f2, v[1], m[1][3]);
  MulSF16_F32 (&f3, v[2], m[2][3]);
  MulSF16_F32 (&f4, v[3], m[3][3]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  AddF32 (&f1, &f1, &f4);
  d[3] = ConvertF32_F16 (&f1);
}


void
tv3 (vec3f16 d, vec3f16 v, mat33f16 m)
{
  frac32 f1, f2, f3;

  MulSF16_F32 (&f1, v[0], m[0][0]);
  MulSF16_F32 (&f2, v[1], m[1][0]);
  MulSF16_F32 (&f3, v[2], m[2][0]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  d[0] = ConvertF32_F16 (&f1);

  MulSF16_F32 (&f1, v[0], m[0][1]);
  MulSF16_F32 (&f2, v[1], m[1][1]);
  MulSF16_F32 (&f3, v[2], m[2][1]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  d[1] = ConvertF32_F16 (&f1);

  MulSF16_F32 (&f1, v[0], m[0][2]);
  MulSF16_F32 (&f2, v[1], m[1][2]);
  MulSF16_F32 (&f3, v[2], m[2][2]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  d[2] = ConvertF32_F16 (&f1);
}


void
tvd3 (vec3f16 d, vec3f16 v, mat33f16 m, int64 *l)
{
  frac32 f1, f2, f3;
  int64 p, n, o;

  MulSF16_F32 (&f1, v[0], m[0][0]);
  MulSF16_F32 (&f2, v[1], m[1][0]);
  MulSF16_F32 (&f3, v[2], m[2][0]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  d[0] = ConvertF32_F16 (&f1);

  MulSF16_F32 (&f1, v[0], m[0][1]);
  MulSF16_F32 (&f2, v[1], m[1][1]);
  MulSF16_F32 (&f3, v[2], m[2][1]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  d[1] = ConvertF32_F16 (&f1);

  MulSF16_F32 (&f1, v[0], m[0][2]);
  MulSF16_F32 (&f2, v[1], m[1][2]);
  MulSF16_F32 (&f3, v[2], m[2][2]);
  AddF32 (&f1, &f1, &f2);
  AddF32 (&f1, &f1, &f3);
  d[2] = ConvertF32_F16 (&f1);

  ConvertSF16_F32 (&p, d[2]);
  DivS64 (&n, &o, l, &p);
  MulSF16_F32 (&f1, d[0], n.lo);
  d[0] = ConvertF32_F16 (&f1);
  MulSF16_F32 (&f1, d[1], n.lo);
  d[1] = ConvertF32_F16 (&f1);
}


void
tmul4 (mat44f16 d, mat44f16 s1, mat44f16 s2)
{
  tv4 (d[0], s1[0], s2);
  tv4 (d[1], s1[1], s2);
  tv4 (d[2], s1[2], s2);
  tv4 (d[3], s1[3], s2);
}


void
tmul3 (mat33f16 d, mat33f16 s1, mat33f16 s2)
{
  tv3 (d[0], s1[0], s2);
  tv3 (d[1], s1[1], s2);
  tv3 (d[2], s1[2], s2);
}


void
tmuld3 (mat33f16 d, mat33f16 s1, mat33f16 s2, int64 *l)
{
  tvd3 (d[0], s1[0], s2, l);
  tvd3 (d[1], s1[1], s2, l);
  tvd3 (d[2], s1[2], s2, l);
}


void
testmat4 (int32 it)
{
  int32 i, j, error;
  mat44f16 m1, m2, m3, m4;

  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) {
      m1[i][j] = (rand()*2)^rand();
      m2[i][j] = (rand()*2)^rand();
    }
  }

  MulMat44Mat44_F16 (m3, m1, m2);
  tmul4 (m4, m1, m2);

  error = 0;
  for (i=0; i<4; i++) {
    for (j=0; j<4; j++) {
      if (m3[i][j] != m4[i][j]) error++;
    }
  }

  if (error) {
    DBUG (("\nError: (iteration %ld)\n", it));
    DBUG (("--- hardware result\n"));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m3[0][0], m3[0][1], m3[0][2], m3[0][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m3[1][0], m3[1][1], m3[1][2], m3[1][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m3[2][0], m3[2][1], m3[2][2], m3[2][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m3[3][0], m3[3][1], m3[3][2], m3[3][3]));
    DBUG (("--- software result\n"));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m4[0][0], m4[0][1], m4[0][2], m4[0][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m4[1][0], m4[1][1], m4[1][2], m4[1][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m4[2][0], m4[2][1], m4[2][2], m4[2][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m4[3][0], m4[3][1], m4[3][2], m4[3][3]));
    DBUG (("--- matrix 1 input\n"));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m1[0][0], m1[0][1], m1[0][2], m1[0][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m1[1][0], m1[1][1], m1[1][2], m1[1][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m1[2][0], m1[2][1], m1[2][2], m1[2][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m1[3][0], m1[3][1], m1[3][2], m1[3][3]));
    DBUG (("--- matrix 2 input\n"));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m2[0][0], m2[0][1], m2[0][2], m2[0][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m2[1][0], m2[1][1], m2[1][2], m2[1][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m2[2][0], m2[2][1], m2[2][2], m2[2][3]));
    DBUG (("%08lx, %08lx, %08lx, %08lx\n", m2[3][0], m2[3][1], m2[3][2], m2[3][3]));
  }
}


void
testmat3 (int32 it)
{
  int32 i, j, error;
  mat33f16 m1, m2, m3, m4;

  for (i=0; i<3; i++) {
    for (j=0; j<3; j++) {
      m1[i][j] = (rand()*2)^rand();
      m2[i][j] = (rand()*2)^rand();
    }
  }

  MulMat33Mat33_F16 (m3, m1, m2);
  tmul3 (m4, m1, m2);

  error = 0;
  for (i=0; i<3; i++) {
    for (j=0; j<3; j++) {
      if (m3[i][j] != m4[i][j]) error++;
    }
  }

  if (error) {
    DBUG (("\nError: (iteration %ld)\n", it));
    DBUG (("--- hardware result\n"));
    DBUG (("%08lx, %08lx, %08lx\n", m3[0][0], m3[0][1], m3[0][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m3[1][0], m3[1][1], m3[1][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m3[2][0], m3[2][1], m3[2][2]));
    DBUG (("--- software result\n"));
    DBUG (("%08lx, %08lx, %08lx\n", m4[0][0], m4[0][1], m4[0][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m4[1][0], m4[1][1], m4[1][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m4[2][0], m4[2][1], m4[2][2]));
    DBUG (("--- matrix 1 input\n"));
    DBUG (("%08lx, %08lx, %08lx\n", m1[0][0], m1[0][1], m1[0][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m1[1][0], m1[1][1], m1[1][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m1[2][0], m1[2][1], m1[2][2]));
    DBUG (("--- matrix 2 input\n"));
    DBUG (("%08lx, %08lx, %08lx\n", m2[0][0], m2[0][1], m2[0][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m2[1][0], m2[1][1], m2[1][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m2[2][0], m2[2][1], m2[2][2]));
  }
}


void
testmatdiv (int32 it)
{
  int32 i, j, error;
  mat33f16 m1, m2, m3, m4;
  int64 l;

#if 1
  l.hi = urand()&0x7fffffff;
#else
  l.hi = 0x10000;
#endif
  l.lo = 0;

  for (i=0; i<3; i++) {
    for (j=0; j<3; j++) {
      m1[i][j] = (rand()*2)^rand();
      m2[i][j] = (rand()*2)^rand();
    }
  }

#if 0
  m1[0][0] = 0xca8898ec;
  m1[0][1] = 0x7ee7e9e7;
  m1[0][2] = 0xa5fdf4d2;
#if 0
  m1[1][0] = 0xa1120e76;
  m1[1][1] = 0x12373a1e;
  m1[1][2] = 0x0351d346;
  m1[2][0] = 0x35ce2b5e;
  m1[2][1] = 0x9acf6f2a;
  m1[2][2] = 0x4b619e1d;
#endif
  m1[1][0] = 0xca8898ec;
  m1[1][1] = 0x7ee7e9e7;
  m1[1][2] = 0xa5fdf4d2;
  m1[2][0] = 0xca8898ec;
  m1[2][1] = 0x7ee7e9e7;
  m1[2][2] = 0xa5fdf4d2;

  m2[0][0] = 0x3b696e64;
  m2[0][1] = 0x6f08defa;
  m2[0][2] = 0xb333fc07;
  m2[1][0] = 0x6757c55b;
  m2[1][1] = 0xf0a2ae33;
  m2[1][2] = 0x5417ae6f;
  m2[2][0] = 0xeac021a1;
  m2[2][1] = 0xe8e56278;
  m2[2][2] = 0xfcc0c1bb;
#endif

  MulVec3Mat33DivZ_F16 (m3[0], m1[0], m2, l.hi);
  MulVec3Mat33DivZ_F16 (m3[1], m1[1], m2, l.hi);
  MulVec3Mat33DivZ_F16 (m3[2], m1[2], m2, l.hi);
  tmuld3 (m4, m1, m2, &l);

  error = 0;
  for (i=0; i<3; i++) {
#if 0
    if (m3[i][2]<0) continue;	/* for now don't check divide by neg. z */
#endif
    if (m3[i][2] != m4[i][2]) error++;
    if ((m3[i][0]-m4[i][0]>1) || (m3[i][0]-m4[i][0]<-1)) error++;
    if ((m3[i][1]-m4[i][1]>1) || (m3[i][1]-m4[i][1]<-1)) error++;
  }

  if (error) {
    DBUG (("\nError (iteration %ld)\n", it));
    DBUG (("--- hardware result\n"));
    DBUG (("%08lx, %08lx, %08lx\n", m3[0][0], m3[0][1], m3[0][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m3[1][0], m3[1][1], m3[1][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m3[2][0], m3[2][1], m3[2][2]));
    DBUG (("--- software result\n"));
    DBUG (("%08lx, %08lx, %08lx\n", m4[0][0], m4[0][1], m4[0][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m4[1][0], m4[1][1], m4[1][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m4[2][0], m4[2][1], m4[2][2]));
    DBUG (("--- matrix 1 input\n"));
    DBUG (("%08lx, %08lx, %08lx\n", m1[0][0], m1[0][1], m1[0][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m1[1][0], m1[1][1], m1[1][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m1[2][0], m1[2][1], m1[2][2]));
    DBUG (("--- matrix 2 input\n"));
    DBUG (("%08lx, %08lx, %08lx\n", m2[0][0], m2[0][1], m2[0][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m2[1][0], m2[1][1], m2[1][2]));
    DBUG (("%08lx, %08lx, %08lx\n", m2[2][0], m2[2][1], m2[2][2]));
    DBUG (("--- n = %08lx %08lx\n", l.hi, l.lo));
  }
}


