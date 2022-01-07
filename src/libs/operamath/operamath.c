/***************************************************************\
*								*
* Operamath folio                                               *
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  15-Sep-94					*
*								*
* Copyright (c) 1992, 1993, 1994, The 3DO Company               *
* All rights reserved						*
* This program is proprietary and confidential			*
*								*
\***************************************************************/


#define DBUG(x)	{ printf x ; }
#define DBUGMATH(x) /* { printf x ; } */

/***************************************************************\
* Include files                                                 *
\***************************************************************/

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

#include "strings.h"
#include "stdio.h"
#include "operror.h"

#include "operamath.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *operamathlib_version;
static void *operlib_version = &operamathlib_version;
#endif


/***************************************************************\
* Defines                                                       *
\***************************************************************/



#define RECIPSF16 -8
#define RECIPUF16 -7
#define DIVREMSF16 -6
#define DIVSF16 -5
#define DIVREMUF16 -4
#define DIVUF16 -3
#define MULSF16 -2
#define MULUF16 -1



/***************************************************************\
* Global variables and structures                               *
\***************************************************************/

MathFolio *_MathBase=0;


/***************************************************************\
* Math routines                                                 *
\***************************************************************/


Err
OpenMathFolio (void)
{
  Item f;
  DBUGMATH (("Opening Operamath Folio\n"));
  if ((f=FindAndOpenFolio("Operamath")) < 0) {
    return f;
  }

  DBUGMATH (("MathFolio Item %lx\n", f));
  _MathBase = (MathFolio *)LookupItem (f);
  return 0;
}


Err
CloseMathFolio (void)
{
  if (_MathBase) return CloseItem (_MathBase->mf.fn.n_Item);
  return BADPTR;
}



ufrac16 DivUF16 (ufrac16 d1, ufrac16 d2)
/* Divide an unsigned 16.16 fraction into another an return an unsigned */
/* 16.16 result.  The remainder from DivUF16 will be returned in r1, which */
/* is unavailable to C code, so DivRemUF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum return in both values. */
/* Note:  The remainder is NOT in 16.16 format, but is in 0.32 format. */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are uint32 or ufrac30, as long as both inputs are the same type. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, DIVUF16, (d1,d2), r, (ufrac16));
  return r;
}

ufrac16 DivRemUF16 (ufrac16 *rem, ufrac16 d1, ufrac16 d2)
/* Divide an unsigned 16.16 fraction into another an return an unsigned */
/* 16.16 result.  The remainder from DivUF16 will be returned in r1, which */
/* is unavailable to C code, so DivRemUF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum return in both values. */
/* Note:  The remainder is NOT in 16.16 format, but is in 0.32 format. */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are uint32 or ufrac30, as long as both inputs are the same type. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, DIVREMUF16, (rem,d1,d2), r, (ufrac16));
  return r;
}


frac16 DivSF16 (frac16 d1, frac16 d2)
/* Divide a signed 16.16 fraction into another an return a signed 16.16 */
/* result.  The remainder from DivSF16 will be returned in r1, which is */
/* unavailable to C code, so DivRemSF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum positive return in both values. */
/* Note: the remainder is NOT in 16.16 format, but is in 0.32 format */
/* Note: the MSB of the remainder is a sign bit, and must be extended if */
/* remainder is to be used in subsequent calculations */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are int32 or frac30, as long as both inputs are the same type. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, DIVSF16, (d1,d2), r, (frac16));
  return r;
}

frac16 DivRemSF16 (frac16 *rem, frac16 d1, frac16 d2)
/* Divide a signed 16.16 fraction into another an return a signed 16.16 */
/* result.  The remainder from DivSF16 will be returned in r1, which is */
/* unavailable to C code, so DivRemSF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum positive return in both values. */
/* Note: the remainder is NOT in 16.16 format, but is in 0.32 format */
/* Note: the MSB of the remainder is a sign bit, and must be extended if */
/* remainder is to be used in subsequent calculations */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are int32 or frac30, as long as both inputs are the same type. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, DIVREMSF16, (rem,d1,d2), r, (frac16));
  return r;
}


ufrac16 RecipUF16 (ufrac16 d)
/* Take the reciprocal of an unsigned 16.16 number and return the 16.16 */
/* result.  The remainder will be returned in r1. */
/* Overflow is signaled by all bits set in the return values. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, RECIPUF16, (d), r, (ufrac16));
  return r;
}


frac16 RecipSF16 (frac16 d)
/* Take the reciprocal of a signed 16.16 number and return the 16.16 */
/* result.  The remainder will be returned in r1. */
/* Overflow is signaled by all bits set in the return values. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, RECIPSF16, (d), r, (frac16));
  return r;
}


frac16 MulSF16 (frac16 m1, frac16 m2)
/* Multiply two signed 16.16 integers together, and get a 16.16 result. */
/* Overflows are not detected.  Lower bits are truncated. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, MULSF16, (m1,m2), r, (frac16));
  return r;
}


ufrac16 MulUF16 (ufrac16 m1, ufrac16 m2)
/* Multiply two unsigned 16.16 integers together, and get a 16.16 result. */
/* Overflows are not detected.  Lower bits are truncated. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, MULUF16, (m1,m2), r, (ufrac16));
  return r;
}




#if 0
void
Neg64 (int64 *dest, int64 *src)
/* return the two's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, NEG64, (dest,src));
}

void NegF32 (frac32 *dest, frac32 *src)
/* return the two's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, NEG64, (dest,src));
}

void NegF60 (frac60 *dest, frac60 *src)
/* return the two's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, NEG64, (dest,src));
}

void Not64 (int64 *dest, int64 *src)
/* return the one's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, NOT64, (dest,src));
}

void NotF32 (frac32 *dest, frac32 *src)
/* return the one's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, NOT64, (dest,src));
}

void NotF60 (frac60 *dest, frac60 *src)
/* return the one's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, NOT64, (dest,src));
}


ufrac16 SqrtF16 (ufrac16 x)
/* Return the positive square root of an unsigned 16.16 number */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, SQRTF16, (x), r, (ufrac16));
  return r;
}


uint32 Sqrt32 (uint32 x)
/* Return the positive square root of an unsigned 32 bit number */
{
  uint32 r;
  CALLFOLIORET (_MathBase, SQRT32, (x), r, (uint32));
  return r;
}


ufrac16 DivUF16 (ufrac16 d1, ufrac16 d2)
/* Divide an unsigned 16.16 fraction into another an return an unsigned */
/* 16.16 result.  The remainder from DivUF16 will be returned in r1, which */
/* is unavailable to C code, so DivRemUF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum return in both values. */
/* Note:  The remainder is NOT in 16.16 format, but is in 0.32 format. */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are uint32 or ufrac30, as long as both inputs are the same type. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, DIVUF16, (d1,d2), r, (ufrac16));
  return r;
}

ufrac16 DivRemUF16 (ufrac16 *rem, ufrac16 d1, ufrac16 d2)
/* Divide an unsigned 16.16 fraction into another an return an unsigned */
/* 16.16 result.  The remainder from DivUF16 will be returned in r1, which */
/* is unavailable to C code, so DivRemUF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum return in both values. */
/* Note:  The remainder is NOT in 16.16 format, but is in 0.32 format. */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are uint32 or ufrac30, as long as both inputs are the same type. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, DIVREMUF16, (rem,d1,d2), r, (ufrac16));
  return r;
}


frac16 DivSF16 (frac16 d1, frac16 d2)
/* Divide a signed 16.16 fraction into another an return a signed 16.16 */
/* result.  The remainder from DivSF16 will be returned in r1, which is */
/* unavailable to C code, so DivRemSF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum positive return in both values. */
/* Note: the remainder is NOT in 16.16 format, but is in 0.32 format */
/* Note: the MSB of the remainder is a sign bit, and must be extended if */
/* remainder is to be used in subsequent calculations */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are int32 or frac30, as long as both inputs are the same type. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, DIVSF16, (d1,d2), r, (frac16));
  return r;
}

frac16 DivRemSF16 (frac16 *rem, frac16 d1, frac16 d2)
/* Divide a signed 16.16 fraction into another an return a signed 16.16 */
/* result.  The remainder from DivSF16 will be returned in r1, which is */
/* unavailable to C code, so DivRemSF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum positive return in both values. */
/* Note: the remainder is NOT in 16.16 format, but is in 0.32 format */
/* Note: the MSB of the remainder is a sign bit, and must be extended if */
/* remainder is to be used in subsequent calculations */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are int32 or frac30, as long as both inputs are the same type. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, DIVREMSF16, (rem,d1,d2), r, (frac16));
  return r;
}


ufrac16 RecipUF16 (ufrac16 d)
/* Take the reciprocal of an unsigned 16.16 number and return the 16.16 */
/* result.  The remainder will be returned in r1. */
/* Overflow is signaled by all bits set in the return values. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, RECIPUF16, (d), r, (ufrac16));
  return r;
}


frac16 RecipSF16 (frac16 d)
/* Take the reciprocal of a signed 16.16 number and return the 16.16 */
/* result.  The remainder will be returned in r1. */
/* Overflow is signaled by all bits set in the return values. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, RECIPSF16, (d), r, (frac16));
  return r;
}


frac16 MulSF16 (frac16 m1, frac16 m2)
/* Multiply two signed 16.16 integers together, and get a 16.16 result. */
/* Overflows are not detected.  Lower bits are truncated. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, MULSF16, (m1,m2), r, (frac16));
  return r;
}


ufrac16 MulUF16 (ufrac16 m1, ufrac16 m2)
/* Multiply two unsigned 16.16 integers together, and get a 16.16 result. */
/* Overflows are not detected.  Lower bits are truncated. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, MULUF16, (m1,m2), r, (ufrac16));
  return r;
}


frac30 MulSF30 (frac30 m1, frac30 m2)
/* Multiply two 2.30 fractions together and get a 2.30 result. */
/* Overflows are not detected.  Lower bits are truncated. */
{
  frac30 r;
  CALLFOLIORET (_MathBase, MULSF30, (m1,m2), r, (frac30));
  return r;
}


ufrac16 SquareSF16 (frac16 m)
/* Return the square of a signed 16.16 integer. */
/* Overflows are not detected.  Lower bits are truncated. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, SQUARESF16, (m), r, (frac16));
  return r;
}


ufrac16 SquareUF16 (ufrac16 m)
/* Return the square of an unsigned 16.16 integer. */
/* Overflows are not detected.  Lower bits are truncated. */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, SQUAREUF16, (m), r, (ufrac16));
  return r;
}


void MulS32_64 (int64 *prod, int32 m1, int32 m2)
/* Multiply two signed 32 bit integers together, and get a 64 bit result. */
/* Alternately multiply two signed 16.16 numbers and get a 32.32 result. */
/* Alternately multiply two signed 2.30 numbers and get a 4.60 result. */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, MULS32_64, (prod,m1,m2));
}

void MulSF16_F32 (frac32 *prod, frac16 m1, frac16 m2)
/* Multiply two signed 32 bit integers together, and get a 64 bit result. */
/* Alternately multiply two signed 16.16 numbers and get a 32.32 result. */
/* Alternately multiply two signed 2.30 numbers and get a 4.60 result. */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, MULS32_64, (prod,m1,m2));
}

void MulSF30_F60 (frac60 *prod, frac30 m1, frac30 m2)
/* Multiply two signed 32 bit integers together, and get a 64 bit result. */
/* Alternately multiply two signed 16.16 numbers and get a 32.32 result. */
/* Alternately multiply two signed 2.30 numbers and get a 4.60 result. */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, MULS32_64, (prod,m1,m2));
}


void MulU32_64 (uint64 *prod, uint32 m1, uint32 m2)
/* Multiply two unsigned 32 bit integers together, and get a 64 bit result. */
/* Alternately multiply two unsigned 16.16 numbers and get a 32.32 result. */
/* Alternately multiply two unsigned 2.30 numbers and get a 4.60 result. */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, MULU32_64, (prod,m1,m2));
}

void MulUF16_F32 (ufrac32 *prod, ufrac16 m1, ufrac16 m2)
/* Multiply two unsigned 32 bit integers together, and get a 64 bit result. */
/* Alternately multiply two unsigned 16.16 numbers and get a 32.32 result. */
/* Alternately multiply two unsigned 2.30 numbers and get a 4.60 result. */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, MULU32_64, (prod,m1,m2));
}

void MulUF30_F60 (ufrac60 *prod, ufrac30 m1, ufrac30 m2)
/* Multiply two unsigned 32 bit integers together, and get a 64 bit result. */
/* Alternately multiply two unsigned 16.16 numbers and get a 32.32 result. */
/* Alternately multiply two unsigned 2.30 numbers and get a 4.60 result. */
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, MULU32_64, (prod,m1,m2));
}


void Add64 (int64 *r, int64 *a1, int64 *a2)
/* Add two 64 bit integers together and return the 64 bit result */
/* Alternately add two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, ADD64, (r,a1,a2));
}

void AddF32 (frac32 *r, frac32 *a1, frac32 *a2)
/* Add two 64 bit integers together and return the 64 bit result */
/* Alternately add two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, ADD64, (r,a1,a2));
}

void AddF60 (frac60 *r, frac60 *a1, frac60 *a2)
/* Add two 64 bit integers together and return the 64 bit result */
/* Alternately add two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, ADD64, (r,a1,a2));
}


void Sub64 (int64 *r, int64 *s1, int64 *s2)
/* Subtract two 64 bit integers and return the 64 bit result */
/* Alternately subtract two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, SUB64, (r,s1,s2));
}

void SubF32 (frac32 *r, frac32 *s1, frac32 *s2)
/* Subtract two 64 bit integers and return the 64 bit result */
/* Alternately subtract two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, SUB64, (r,s1,s2));
}

void SubF60 (frac60 *r, frac60 *s1, frac60 *s2)
/* Subtract two 64 bit integers and return the 64 bit result */
/* Alternately subtract two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  CALLFOLIO (_MathBase, SUB64, (r,s1,s2));
}


int32 CompareU64 (uint64 *s1, uint64 *s2)
/* Subtract two unsigned 64 bit integers and return 1 if s1>s2, 0 if s1==s2, */
/* and -1 if s1<s2. */
/* Alternately compare two unsigned 32.32 fractions or two 4.60 fractions */
/* (all names call the same routine) */
/* Subtract two 64 bit integers and return the 64 bit result */
/* Alternately subtract two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  int32 r;
  CALLFOLIORET (_MathBase, COMPAREU64, (s1,s2), r, (int32));
  return r;
}

int32 CompareUF32 (ufrac32 *s1, ufrac32 *s2)
/* Subtract two unsigned 64 bit integers and return 1 if s1>s2, 0 if s1==s2, */
/* and -1 if s1<s2. */
/* Alternately compare two unsigned 32.32 fractions or two 4.60 fractions */
/* (all names call the same routine) */
/* Subtract two 64 bit integers and return the 64 bit result */
/* Alternately subtract two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  int32 r;
  CALLFOLIORET (_MathBase, COMPAREU64, (s1,s2), r, (int32));
  return r;
}

int32 CompareUF60 (ufrac60 *s1, ufrac60 *s2)
/* Subtract two unsigned 64 bit integers and return 1 if s1>s2, 0 if s1==s2, */
/* and -1 if s1<s2. */
/* Alternately compare two unsigned 32.32 fractions or two 4.60 fractions */
/* (all names call the same routine) */
/* Subtract two 64 bit integers and return the 64 bit result */
/* Alternately subtract two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */
{
  int32 r;
  CALLFOLIORET (_MathBase, COMPAREU64, (s1,s2), r, (int32));
  return r;
}


int32 CompareS64 (int64 *s1, int64 *s2)
/* Subtract two signed 64 bit integers and return the high word of the result */
/* (or 1 if the high word is zero, and the low word is non-zero). */
/* Alternately compare two 32.32 fractions or two 4.60 fractions */
/* The result of the comparison will be positive if s1>s2, zero if s1==s2, */
/* and negative if s1<s2. */
/* (all names call the same routine) */
{
  int32 r;
  CALLFOLIORET (_MathBase, COMPARES64, (s1,s2), r, (int32));
  return r;
}

int32 CompareSF32 (frac32 *s1, frac32 *s2)
/* Subtract two signed 64 bit integers and return the high word of the result */
/* (or 1 if the high word is zero, and the low word is non-zero). */
/* Alternately compare two 32.32 fractions or two 4.60 fractions */
/* The result of the comparison will be positive if s1>s2, zero if s1==s2, */
/* and negative if s1<s2. */
/* (all names call the same routine) */
{
  int32 r;
  CALLFOLIORET (_MathBase, COMPARES64, (s1,s2), r, (int32));
  return r;
}

int32 CompareSF60 (frac60 *s1, frac60 *s2)
/* Subtract two signed 64 bit integers and return the high word of the result */
/* (or 1 if the high word is zero, and the low word is non-zero). */
/* Alternately compare two 32.32 fractions or two 4.60 fractions */
/* The result of the comparison will be positive if s1>s2, zero if s1==s2, */
/* and negative if s1<s2. */
/* (all names call the same routine) */
{
  int32 r;
  CALLFOLIORET (_MathBase, COMPARES64, (s1,s2), r, (int32));
  return r;
}


uint64 *DivU64 (uint64 *q, uint64 *r, uint64 *d1, uint64 *d2)
/* Divide one unsigned 64 bit integer into another, and return the quotient */
/* and remainder.  On return, r0 will contain a pointer to the quotient. */
{
  uint64 *ret;
  CALLFOLIORET (_MathBase, DIVU64, (q,r,d1,d2), ret, (uint64 *));
  return ret;
}


int64 *DivS64 (int64 *q, int64 *r, int64 *d1, int64 *d2)
/* Divide one signed 64 bit integer into another, and return the quotient */
/* and remainder.  On return, r0 will contain a pointer to the quotient. */
{
  int64 *ret;
  CALLFOLIORET (_MathBase, DIVS64, (q,r,d1,d2), ret, (int64 *));
  return ret;
}


void Mul64 (int64 *p, int64 *m1, int64 *m2)
/* Multiply one 64 bit integer by another and return the result.  Overflow */
/* is not detected. */
{
  CALLFOLIO (_MathBase, MUL64, (p,m1,m2));
}

void Square64 (uint64 *p, int64 *m)
/* Return the square of a 64 bit integer.  Overflow is not detected. */
{
  CALLFOLIO (_MathBase, SQUARE64, (p,m));
}


uint32 Sqrt64_32 (uint64 *x)
/* Return the 32 bit square root of an unsigned 64 bit integer. */
/* Alternatively, return the square root of 32.32 fraction as a 16.16 fraction. */
/* Alternatively, return the square root of 4.60 fraction as a 2.30 fraction. */
/* (all names call the same routine) */
{
  uint32 r;
  CALLFOLIORET (_MathBase, SQRT64_32, (x), r, (uint32));
  return r;
}

ufrac16 SqrtF32_F16 (ufrac32 *x)
/* Return the 32 bit square root of an unsigned 64 bit integer. */
/* Alternatively, return the square root of 32.32 fraction as a 16.16 fraction. */
/* Alternatively, return the square root of 4.60 fraction as a 2.30 fraction. */
/* (all names call the same routine) */
{
  ufrac16 r;
  CALLFOLIORET (_MathBase, SQRT64_32, (x), r, (ufrac16));
  return r;
}

ufrac30 SqrtF60_F30 (ufrac32 *x)
/* Return the 32 bit square root of an unsigned 64 bit integer. */
/* Alternatively, return the square root of 32.32 fraction as a 16.16 fraction. */
/* Alternatively, return the square root of 4.60 fraction as a 2.30 fraction. */
/* (all names call the same routine) */
{
  ufrac30 r;
  CALLFOLIORET (_MathBase, SQRT64_32, (x), r, (ufrac30));
  return r;
}


/* uint32 __rt_udiv (uint32 d1, uint32 d2); */
/* Divide a 32 bit integer d2 by 32 bit integer d1 and return a 32 bit quotient. */
/* The remainder is also returned in r1.  This routine takes arguments and returns */
/* values in the same way that the compiler expects the internally supplied */
/* library routines to.   Use DivRemU32() to get the remainder for C code. */

/* int32 __rt_sdiv (int32 d1, int32 d2); */
/* Divide a 32 bit integer d2 by 32 bit integer d1 and return a 32 bit quotient. */
/* The remainder is also returned in r1.  This routine takes arguments and returns */
/* values in the same way that the compiler expects the internally supplied */
/* library routines to.  Use DivRemS32() to get the remainder for C code. */

uint32 DivRemU32 (uint32 *rem, uint32 d1, uint32 d2)
/* Calculate the quotient and remainder when dividing unsigned 32 bit integers d1/d2 */
/* This routine calls __rt_udiv, but rearranges the arguments and return values */
/* to return the remainder to C code. */
{
  uint32 r;
  CALLFOLIORET (_MathBase, DIVREMU32, (rem,d1,d2), r, (uint32));
  return r;
}


int32 DivRemS32 (int32 *rem, int32 d1, int32 d2)
/* Calculate the quotient and remainder when dividing signed 32 bit integers d1/d2 */
/* This routine calls __rt_sdiv, but rearranges the arguments and return values */
/* to return the remainder to C code. */
{
  int32 r;
  CALLFOLIORET (_MathBase, DIVREMS32, (rem,d1,d2), r, (int32));
  return r;
}


frac16 SinF16 (frac16 x)
/* Return the 16.16 sine of an angle (assume 256.0 units of angle available) */
/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */
{
  frac16 r;
  CALLFOLIORET (_MathBase, SINF16, (x), r, (frac16));
  return r;
}


frac16 CosF16 (frac16 x)
/* Return the 16.16 cosine of an angle (assume 256.0 units of angle available) */
/* Rptionally, the angle can be an integer dividing the circle into 16,777,216 units */
{
  frac16 r;
  CALLFOLIORET (_MathBase, COSF16, (x), r, (frac16));
  return r;
}


frac16 Atan2F16 (frac16 x, frac16 y)
/* Return the arctangent of the ratio y/x */
/* The result assume 256.0 units in the circle (or 16,777,216 units if used as an integer) */
/* Note:  The correct 16.16 result will be returned if the arguments are int32, frac30 or */
/* frac14, assuming the both arguments are the same type. */
{
  frac16 r;
  CALLFOLIORET (_MathBase, ATAN2F16, (x,y), r, (frac16));
  return r;
}


frac30 SinF30 (frac16 x)
/* Return the 2.30 sine of an angle (assume 256.0 units of angle available) */
/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */
{
  frac30 r;
  CALLFOLIORET (_MathBase, SINF30, (x), r, (frac30));
  return r;
}


frac30 CosF30 (frac16 x)
/* Return the 2.30 cosine of an angle (assume 256.0 units of angle available) */
/* Rptionally, the angle can be an integer dividing the circle into 16,777,216 units */
{
  frac30 r;
  CALLFOLIORET (_MathBase, COSF30, (x), r, (frac30));
  return r;
}


void SinF32 (frac32 *dest, frac16 x)
/* Return the 32.32 sine of an angle (assume 256.0 units of angle available) */
/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */
{
  CALLFOLIO (_MathBase, SINF32, (dest,x));
}
#endif


#if 0
void __swi(MULVEC3MAT33_F16) MulVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat);
/* multiply a 3x3 matrix of 16.16 values by a vector of 16.16 values, return the result */

void __swi(MULVEC4MAT44_F16) MulVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat);
/* multiply a 4x4 matrix of 16.16 values by a vector of 16.16 values, return the result */

void __swi(MULMAT33MAT33_F16) MulMat33Mat33_F16 (mat33f16 dest, mat33f16 src1, mat33f16 src2);
/* Multiply two 3x3 matrices of 16.16 values and return the result */

void __swi(MULMAT44MAT44_F16) MulMat44Mat44_F16 (mat44f16 dest, mat44f16 src1, mat44f16 src2);
/* Multiply two 4x4 matrices of 16.16 values and return the result */

frac16 __swi(DOT3_F16) Dot3_F16 (vec3f16 v1, vec3f16 v2);
/* Return the dot product of two vectors of 16.16 values */

frac16 __swi(DOT4_F16) Dot4_F16 (vec4f16 v1, vec4f16 v2);
/* Return the dot product of two vectors of 16.16 values */

void __swi(CROSS3_F16) Cross3_F16 (vec3f16 dest, vec3f16 v1, vec3f16 v2);
/* Return the cross product of two vectors of 16.16 values */

#endif


#if 0
void Transpose33_F16 (mat33f16 dest, mat33f16 src)
/* Return the transpose of a 3x3 matrix of 16.16 values */
{
  CALLFOLIO (_MathBase, TRANSPOSE33_F16, (dest,src));
}


void Transpose44_F16 (mat44f16 dest, mat44f16 src)
/* Return the transpose of a 4x4 matrix of 16.16 values */
{
  CALLFOLIO (_MathBase, TRANSPOSE44_F16, (dest,src));
}
#endif


#if 0
void __swi(MULMANYVEC3MAT33_F16) MulManyVec3Mat33_F16
       (vec3f16 *dest, vec3f16 *src, mat33f16 mat, int32 count);
/* Multiply many vectors by a matrix */

void __swi(MULMANYVEC4MAT44_F16) MulManyVec4Mat44_F16
       (vec4f16 *dest, vec4f16 *src, mat44f16 mat, int32 count);
/* Multiply many vectors by a matrix */

void __swi(MULOBJECTVEC3MAT33_F16) MulObjectVec3Mat33_F16
       (void *objectlist[], ObjOffset1 *offsetstruct, int32 count);
/* Multiply many vectors within object structures by a matrix within that object */
/* structure, and repeat over a number of objects */

void __swi(MULOBJECTVEC4MAT44_F16) MulObjectVec4Mat44_F16
       (void *objectlist[], ObjOffset1 *offsetstruct, int32 count);
/* Multiply many vectors within object structures by a matrix within that object */
/* structure, and repeat over a number of objects */

void __swi(MULOBJECTMAT33_F16) MulObjectMat33_F16
       (void *objectlist[], ObjOffset2 *offsetstruct, mat33f16 mat, int32 count);
/* Multiply a matrix within an object structure by an external matrix, and repeat */
/* over a number of objects */

void __swi(MULOBJECTMAT44_F16) MulObjectMat44_F16
       (void *objectlist[], ObjOffset2 *offsetstruct, mat44f16 mat, int32 count);
/* Multiply a matrix within an object structure by an external matrix, and repeat */
/* over a number of objects */

void __swi(MULMANYF16) MulManyF16 (frac16 *dest, frac16 *src1, frac16 *src2, int32 count);
/* Multiply an array of 16.16 fractions by another array of fractions */

void __swi(MULSCALARF16) MulScalarF16 (frac16 *dest, frac16 *src, frac16 scalar, int32 count);
/* Multiply a 16.16 scalar by an array of 16.16 fractions */

#endif
