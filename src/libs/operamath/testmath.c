/***************************************************************\
*								*
* Simple task to test math routines				*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  5-Aug-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company                     *
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

ulong i32, j32, k32;
uint64 i64, j64, k64, l64;
ufrac16 if16, jf16, kf16;
ufrac32 if32, jf32, kf32, lf32;
frac16 ifs16, jfs16, kfs16;

mat33f16 mat33if16, mat33jf16;
vec3f16 vec3if16, vec3jf16;
mat44f16 mat44if16;
vec4f16 vec4if16, vec4jf16;

extern void aMulVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat);


/*
 * test routines
 */

void
testConvert32_F16 (int32 x)
{
  if16 = Convert32_F16(x);
  DBUG (("Convert32_F16(0x%lx) = 0x%lx.%04lx\n", x, (uint32)if16>>16, if16&0xffff));
}


void
testConvertF16_32 (frac16 x)
{
  i32 = ConvertF16_32(x);
  DBUG (("ConvertF16_32(0x%lx.%04lx) = 0x%lx\n", (uint32)x>>16, x&0xffff, i32));
}


void
testConvert32_F32 (int32 x)
{
  Convert32_F32(&if32, x);
  DBUG (("Convert32_F32(0x%lx) = 0x%lx %08lx\n", x, if32.hi, if32.lo));
}


void
testConvertUF16_F32 (ufrac16 x)
{
  ConvertUF16_F32(&if32, x);
  DBUG (("ConvertUF16_F32(0x%lx) = 0x%lx.%08lx\n", x, if32.hi, if32.lo));
}


void
testConvertSF16_F32 (frac16 x)
{
  ConvertSF16_F32(&if32, x);
  DBUG (("ConvertSF16_F32(0x%lx) = 0x%lx.%08lx\n", x, if32.hi, if32.lo));
}


void
testConvertF32_F16 (ulong xhi, ulong xlo)
{
  if32.hi = xhi;  if32.lo = xlo;
  if16 = ConvertF32_F16(&if32);
  DBUG (("ConvertF32_F16(0x%lx.%08lx) = ", xhi, xlo));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testConvertU32_64 (uint32 x)
{
  ConvertU32_64(&i64, x);
  DBUG (("ConvertU32_64(0x%lx) = 0x%lx %08lx\n", x, i64.hi, i64.lo));
}


void
testConvertS32_64 (int32 x)
{
  ConvertS32_64(&i64, x);
  DBUG (("ConvertS32_64(0x%lx) = 0x%lx %08lx\n", x, i64.hi, i64.lo));
}


extern ufrac16 aRecipUF16 (ufrac16 x);

void
testRecipUF16 (ufrac16 x)
{
  if16 = aRecipUF16(x);
  DBUG (("RecipUF16(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testRecipSF16 (frac16 x)
{
  if16 = RecipSF16(x);
  DBUG (("RecipSF16(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testSquareUF16 (ufrac16 x)
{
  if16 = SquareUF16(x);
  DBUG (("SquareUF16(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testSquareSF16 (frac16 x)
{
  if16 = SquareSF16(x);
  DBUG (("SquareSF16(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testSqrt32 (uint32 x)
{
  i32 = Sqrt32(x);
  DBUG (("Sqrt32(0x%lx) = 0x%lx\n", x, i32));
}


void
testSqrt64_32 (uint32 hi, uint32 lo)
{
  uint64 x;
  x.hi = hi; x.lo = lo;
  i32 = Sqrt64_32(&x);
  DBUG (("Sqrt64_32(0x%08lx %08lx) = 0x%lx\n", x.hi, x.lo, i32));
}


void
testSqrtF16 (ufrac16 x)
{
  if16 = SqrtF16(x);
  DBUG (("SqrtF16(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testDivUF16 (ufrac16 x, ufrac16 y)
{
  if16 = DivRemUF16 (&jf16, x, y);
  DBUG (("DivRemUF16(0x%lx.%04lx,", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx) = ", (uint32)y>>16, y&0xffff));
  DBUG (("0x%lx.%04lx ", (uint32)if16>>16, if16&0xffff));
  DBUG (("(0x.%08lx)\n", jf16));
}


void
testDivSF16 (frac16 x, frac16 y)
{
  if16 = DivRemSF16 (&jfs16, x, y);
  DBUG (("DivRemSF16(0x%lx.%04lx,", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx) = ", (uint32)y>>16, y&0xffff));
  DBUG (("0x%lx.%04lx ", (uint32)if16>>16, if16&0xffff));
  DBUG (("(0x.%08lx)\n", jfs16));
}


void
testMulU32_64 (uint32 x, uint32 y)
{
  MulU32_64 (&i64, x, y);
  DBUG (("MulU64(0x%lx,0x%lx) = ", x, y));
  DBUG (("0x%lx %08lx\n", i64.hi, i64.lo));
}


void
testAdd64 (ulong xhi, ulong xlo, ulong yhi, ulong ylo)
{
  i64.hi = xhi;  i64.lo = xlo;  j64.hi = yhi;  j64.lo = ylo;
  Add64 (&k64, &i64, &j64);
  DBUG (("Add64(0x%lx %08lx, ", xhi, xlo));
  DBUG (("0x%lx %08lx) = ", yhi, ylo));
  DBUG (("0x%lx %08lx\n", k64.hi, k64.lo));
}


void
testSub64 (ulong xhi, ulong xlo, ulong yhi, ulong ylo)
{
  i64.hi = xhi;  i64.lo = xlo;  j64.hi = yhi;  j64.lo = ylo;
  Sub64 (&k64, &i64, &j64);
  DBUG (("Sub64(0x%lx %08lx, ", xhi, xlo));
  DBUG (("0x%lx %08lx) = ", yhi, ylo));
  DBUG (("0x%lx %08lx\n", k64.hi, k64.lo));
}


void
testCompareU64 (ulong xhi, ulong xlo, ulong yhi, ulong ylo)
{
  i64.hi = xhi;  i64.lo = xlo;  j64.hi = yhi;  j64.lo = ylo;
  i32 = CompareU64 (&i64, &j64);
  DBUG (("CompareU64(0x%lx %08lx, ", xhi, xlo));
  DBUG (("0x%lx %08lx) = %lx\n", yhi, ylo, i32));
}


void
testCompareS64 (ulong xhi, ulong xlo, ulong yhi, ulong ylo)
{
  i64.hi = xhi;  i64.lo = xlo;  j64.hi = yhi;  j64.lo = ylo;
  i32 = CompareS64 (&i64, &j64);
  DBUG (("CompareS64(0x%lx %08lx, ", xhi, xlo));
  DBUG (("0x%lx %08lx) = %lx\n", yhi, ylo, i32));
}


void
testMulS32_64 (uint32 x, uint32 y)
{
  MulS32_64 (&i64, x, y);
  DBUG (("MulS64(0x%lx,0x%lx) = ", x, y));
  DBUG (("0x%lx %08lx\n", i64.hi, i64.lo));
}


void
testDivU64 (ulong xhi, ulong xlo, ulong yhi, ulong ylo)
{
  i64.hi = xhi; i64.lo = xlo; j64.hi = yhi; j64.lo = ylo;
  DivU64 (&k64, &l64, &i64, &j64);
  DBUG (("DivU64(0x%lx %08lx, ", i64.hi, i64.lo));
  DBUG (("0x%lx %08lx) = ", j64.hi, j64.lo));
  DBUG (("0x%lx %08lx ", k64.hi, k64.lo));
  DBUG (("(0x%lx %08lx)\n", l64.hi, l64.lo));
}


void
testDivS64 (ulong xhi, ulong xlo, ulong yhi, ulong ylo)
{
  i64.hi = xhi; i64.lo = xlo; j64.hi = yhi; j64.lo = ylo;
  DivS64 (&k64, &l64, &i64, &j64);
  DBUG (("DivS64(0x%lx %08lx, ", i64.hi, i64.lo));
  DBUG (("0x%lx %08lx) = ", j64.hi, j64.lo));
  DBUG (("0x%lx %08lx ", k64.hi, k64.lo));
  DBUG (("(0x%lx %08lx)\n", l64.hi, l64.lo));
}


void
testMul64 (ulong xhi, ulong xlo, ulong yhi, ulong ylo)
{
  i64.hi = xhi;  i64.lo = xlo;  j64.hi = yhi;  j64.lo = ylo;
  Mul64 (&k64, &i64, &j64);
  DBUG (("Mul64(0x%lx %08lx, ", xhi, xlo));
  DBUG (("0x%lx %08lx) = ", yhi, ylo));
  DBUG (("0x%lx %08lx\n", k64.hi, k64.lo));
}


void
testSquare64 (ulong xhi, ulong xlo)
{
  i64.hi = xhi;  i64.lo = xlo;
  Square64 (&k64, &i64);
  DBUG (("Square64(0x%lx %08lx) = ", xhi, xlo));
  DBUG (("0x%lx %08lx\n", k64.hi, k64.lo));
}


void
testSqrtF32_F16 (ulong xhi, ulong xlo)
{
  if32.hi = xhi;  if32.lo = xlo;
  if16 = SqrtF32_F16(&if32);
  DBUG (("SqrtF32_F16(0x%lx.%08lx) = ", xhi, xlo));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testDivRemU32 (int32 x, int32 y)
{
  i32 = DivRemU32(&j32,x,y);
  DBUG (("DivRemU32(%lx,%lx) = %lx", x, y, i32));
  DBUG (("(%lx)\n", j32));
}


void
testDivRemS32 (int32 x, int32 y)
{
  i32 = DivRemS32((int32 *)&j32,x,y);
  DBUG (("DivRemS32(%lx,%lx) = %lx", x, y, i32));
  DBUG (("(%lx)\n", j32));
}


void
testMulUF16 (ufrac16 x, ufrac16 y)
{
  if16 = MulUF16 (x, y);
  DBUG (("MulUF16(0x%lx.%04lx,", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx) = ", (uint32)y>>16, y&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testMulSF16 (frac16 x, frac16 y)
{
  if16 = MulSF16 (x, y);
  DBUG (("MulSF16(0x%lx.%04lx,", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx) = ", (uint32)y>>16, y&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testMulSF30 (frac16 x, frac16 y)
{
  if16 = MulSF30 (x, y);
  DBUG (("MulSF30(0x%lx.%08lx (0x%08lx),", (uint32)x>>30, x<<2, x));
  DBUG (("0x%lx.%08lx (0x%08lx)) = ", (uint32)y>>30, y<<2, y));
  DBUG (("0x%lx.%08lx (0x%08lx)\n", (uint32)if16>>30, if16<<2, if16));
}


void
testSinF16 (frac16 x)
{
  if16 = SinF16(x);
  DBUG (("SinF16(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testCosF16 (frac16 x)
{
  if16 = CosF16(x);
  DBUG (("CosF16(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
}


void
testAtan2F16 (frac16 x, frac16 y)
{
  if16 = Atan2F16(x,y);
  DBUG (("Atan2F16(0x%lx.%04lx, 0x%lx.%04lx) = 0x%lx.%04lx\n", (uint32)x>>16, x&0xffff,
	 (uint32)y>>16, y&0xffff, (uint32)if16>>16, if16&0xffff));
}


void
testSinF32 (frac16 x)
{
  SinF32 (&if32, x);
  DBUG (("SinF32(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%08lx\n", if32.hi, if32.lo));
}


void
testCosF32 (frac16 x)
{
  CosF32 (&if32, x);
  DBUG (("CosF32(0x%lx.%04lx) = ", (uint32)x>>16, x&0xffff));
  DBUG (("0x%lx.%08lx\n", if32.hi, if32.lo));
}



int
main (int argc, char **argv)
{
  DBUG (("Opera math routines test program\n"));

  if (OpenMathFolio() < 0) {
    DBUG (("Error opening math folio\n"));
    return (1);
  }

  testConvert32_F16(1);
  testConvert32_F16(0x1234);
  testConvert32_F16(0x87654321);

  testConvertF16_32(0x10000);
  testConvertF16_32(0x561234);
  testConvertF16_32(0x87654321);

  testConvert32_F32(0x1);
  testConvert32_F32(0x12345678);
  testConvert32_F32(0xffffffff);

  testConvertUF16_F32(0x1);
  testConvertUF16_F32(0x10000);
  testConvertUF16_F32(0x12345678);
  testConvertUF16_F32(0x87654321);

  testConvertSF16_F32(0x1);
  testConvertSF16_F32(0x10000);
  testConvertSF16_F32(0x12345678);
  testConvertSF16_F32(0x87654321);

  testConvertF32_F16(0x0, 0x1);
  testConvertF32_F16(0x0, 0x12345678);
  testConvertF32_F16(0x12345678, 0x9abcdef0);

  testConvertU32_64(0x0);
  testConvertU32_64(0x1);
  testConvertU32_64(0x87654321);
  testConvertU32_64(0xffffffff);

  testConvertS32_64(0x0);
  testConvertS32_64(0x1);
  testConvertS32_64(0x87654321);
  testConvertS32_64(0xffffffff);

  testSqrt32(0x1);
  testSqrt32(0xffffffff);
  testSqrt32(12345678);
  testSqrt32(0x12345678);

  testSqrt64_32(0x0, 0x1);
  testSqrt64_32(0x1, 0x0);
  testSqrt64_32(0xffffffff, 0xffffffff);
  testSqrt64_32(0x12345678, 0x9abcdef0);

  testSqrtF16(1);
  testSqrtF16(0x10000);
  testSqrtF16(0xffffffff);

  testDivUF16(0x10000, 0x10000);
  testDivUF16(0x10000000, 0x10000);
  testDivUF16(0xffffffff, 0x10000);
  testDivUF16(0xffffffff, 0x20000);
  testDivUF16(0x10000, 0x1);
  testDivUF16(0x10000, 0x2);
  testDivUF16(0x10000, 0x1234);

  testDivSF16(0x10000, 0x10000);
  testDivSF16(0x10000000, 0x10000);
  testDivSF16(0xffffffff, 0x10000);
  testDivSF16(0xffffffff, 0x20000);
  testDivSF16(0x1, 0xfffe0000);
  testDivSF16(0x1, 0x20000);
  testDivSF16(0x10000, 0x1);
  testDivSF16(0x10000, 0x2);
  testDivSF16(0x10000, 0x1234);

  testRecipUF16(0x1);
  testRecipUF16(0x10000);
  testRecipUF16(0x20000);
  testRecipUF16(0x12345678);
  testRecipUF16(0x87654321);

  testRecipSF16(0x1);
  testRecipSF16(0x10000);
  testRecipSF16(0x20000);
  testRecipSF16(0x12345678);
  testRecipSF16(0x87654321);

  testMulU32_64 (0x1, 0x1);
  testMulU32_64 (0xffff, 0xffff);
  testMulU32_64 (0xffff, 0x12345678);
  testMulU32_64 (0xffffffff, 0xffffffff);

  testMulS32_64 (0x1, 0x1);
  testMulS32_64 (0xffff, 0xffff);
  testMulS32_64 (0xffff, 0x12345678);
  testMulS32_64 (0xffffffff, 0xffffffff);

  testSquareUF16 (0x1);
  testSquareUF16 (0x8000);
  testSquareUF16 (0x123400);
  testSquareUF16 (0xfffe0000);

  testSquareSF16 (0x1);
  testSquareSF16 (0x8000);
  testSquareSF16 (0x123400);
  testSquareSF16 (0xfffe0000);

  testAdd64 (0x0, 0x1, 0x0, 0x1);
  testAdd64 (0x0, 0x1, 0x0, 0xffffffff);

  testSub64 (0, 1, 0, 1);
  testSub64 (0, 1, 0, 2);
  testSub64 (0x12345678, 0x9abcdef0, 0x12345678, 0x9abcdef0);
  testSub64 (0x12345678, 0x9abcdef0, 0, 0);

  testCompareU64 (0, 1, 0, 1);
  testCompareU64 (0, 1, 0, 2);
  testCompareU64 (0x12345678, 0x9abcdef0, 0x12345678, 0x9abcdef0);
  testCompareU64 (0x12345678, 0x9abcdef0, 0x80000000, 0);

  testCompareS64 (0, 1, 0, 1);
  testCompareS64 (0, 1, 0, 2);
  testCompareS64 (0x12345678, 0x9abcdef0, 0x12345678, 0x9abcdef0);
  testCompareS64 (0x12345678, 0x9abcdef0, 0x80000000, 0);

  testDivU64 (0, 1, 0, 1);
  testDivU64 (1, 0, 0, 1);
  testDivU64 (0, 1, 1, 0);
  testDivU64 (1, 0, 0, 0x12345678);
  testDivU64 (0xffffffff, 0xffffffff, 0, 0x12345678);

  testDivS64 (0, 1, 0, 1);
  testDivS64 (0xffffffff, 0xffffffff, 0, 1);
  testDivS64 (0, 1, 0xffffffff, 0xffffffff);
  testDivS64 (1, 0, 0, 1);
  testDivS64 (0, 1, 1, 0);
  testDivS64 (1, 0, 0, 0x12345678);
  testDivS64 (0xffffffff, 0xffffffff, 0, 0x12345678);
  testDivS64 (0x7fffffff, 0xffffffff, 0, 0x12345678);
  testDivS64 (0xffffffff, 0xffffffff, 0xffffffff, 0xedcba988);
  testDivS64 (0x7fffffff, 0xffffffff, 0xffffffff, 0xedcba988);

  testMul64 (0, 1, 0, 1);
  testMul64 (0, 1, 0x12345678, 0x9abcdef0);
  testMul64 (0, 0xffffffff, 0, 0xffffffff);
  testMul64 (0xffffffff, 0xfffffffe, 0x12345678, 0x9abcdef0);

  testSquare64 (0, 1);
  testSquare64 (0, 0xffffffff);
  testSquare64 (0xffffffff, 0xfffffffe);
  testSquare64 (0, 0x44444444);

  testSqrtF32_F16 (0, 1);
  testSqrtF32_F16 (0xffffffff, 0xffffffff);
  testSqrtF32_F16 (0xfffffffe, 0x00000001);
  testSqrtF32_F16 (0xfffffffe, 0x00000000);
  testSqrtF32_F16 (0x12345678, 0x9abcdef0);

  testDivRemU32 (0, 1);
  testDivRemU32 (1, 1);
  testDivRemU32 (1, 2);
  testDivRemU32 (0xffffffff, 1);
  testDivRemU32 (0x12345678, 0x4444);

  testDivRemS32 (0, 1);
  testDivRemS32 (1, 1);
  testDivRemS32 (1, 2);
  testDivRemS32 (0xffffffff, 1);
  testDivRemS32 (0x12345678, 0x4444);

  testMulUF16 (0x10000, 0x10000);
  testMulUF16 (0x10000, 0x1);
  testMulUF16 (0xffffffff, 0x10000);
  testMulUF16 (0x1, 0x12345678);
  testMulUF16 (0xffffffff, 0xffffffff);
  testMulUF16 (0xffffffff, 0x8000);
  testMulUF16 (0x20000, 0x12345678);

  testMulSF16 (0x10000, 0x10000);
  testMulSF16 (0x10000, 0x1);
  testMulSF16 (0xffffffff, 0x10000);
  testMulSF16 (0x1, 0x12345678);
  testMulSF16 (0xffffffff, 0xffffffff);
  testMulSF16 (0xffff0000, 0x8000);
  testMulSF16 (0x20000, 0x12345678);
  testMulSF16 (0x20000, 0xb504);
  testMulSF16 (0xb504, 0x20000);

  testMulSF30 (1<<30, 1<<30);
  testMulSF30 (1<<30, 0x1);
  testMulSF30 (0xffffffff, 1<<30);
  testMulSF30 (0x1, 0x12345678);
  testMulSF30 (0xffffffff, 0xffffffff);
  testMulSF30 (-1<<30, 1<<29);
  testMulSF30 (-2<<30, 0x12345678);
  testMulSF30 (-2<<30, 0xb504);
  testMulSF30 (0xb504, -2<<30);

  testSinF16(0);
  testSinF16(1);
  testSinF16(0x80);
  testSinF16(0x100);
  testSinF16(0x10000);
  testSinF16 (0x200000);
  testSinF16 (0x400000);
  testSinF16 (0x800000);
  testSinF16 (0xa00000);
  testSinF16 (0xc00000);
  testSinF16 (0xe00000);
  testSinF16 (0x1000000);
  testSinF16 (0xffff0000);

  testCosF16(0);
  testCosF16(1);
  testCosF16(0x80);
  testCosF16(0x100);
  testCosF16(0x10000);
  testCosF16 (0x200000);
  testCosF16 (0x400000);
  testCosF16 (0x800000);
  testCosF16 (0xa00000);
  testCosF16 (0xc00000);
  testCosF16 (0xe00000);
  testCosF16 (0x1000000);
  testCosF16 (0xffff0000);

  testAtan2F16 (0, 0);
  testAtan2F16 (0x10000, 0x0001);
  testAtan2F16 (0x10000, 0x0002);
  testAtan2F16 (0x10000, 0x0004);
  testAtan2F16 (0x10000, 0x0008);
  testAtan2F16 (0x10000, 0x0010);
  testAtan2F16 (0x10000, 0x0020);
  testAtan2F16 (0x10000, 0x0040);
  testAtan2F16 (0x10000, 0x0080);
  testAtan2F16 (0x10000, 0x0100);
  testAtan2F16 (0, 0x10000);
  testAtan2F16 (0x10000, 0);
  testAtan2F16 (0, -0x10000);
  testAtan2F16 (-0x10000, 0);
  testAtan2F16 (0x10000, 0x10000);
  testAtan2F16 (-0x10000, 0x10000);
  testAtan2F16 (0x10000, -0x10000);
  testAtan2F16 (-0x10000, -0x10000);
  testAtan2F16 (0x20000, 0x10000);
  testAtan2F16 (-0x20000, 0x10000);
  testAtan2F16 (0x20000, -0x10000);
  testAtan2F16 (-0x20000, -0x10000);
  testAtan2F16 (0x10000, 0x20000);
  testAtan2F16 (-0x10000, 0x20000);
  testAtan2F16 (0x10000, -0x20000);
  testAtan2F16 (-0x10000, -0x20000);

  testSinF32 (0);
  testSinF32 (1);
  testSinF32 (0x80);
  testSinF32 (0x100);
  testSinF32 (0x10000);
  testSinF32 (0x200000);
  testSinF32 (0x400000);
  testSinF32 (0x800000);
  testSinF32 (0xa00000);
  testSinF32 (0xc00000);
  testSinF32 (0xe00000);
  testSinF32 (0x1000000);
  testSinF32 (0xffff0000);

  testCosF32 (0);
  testCosF32 (1);
  testCosF32 (0x80);
  testCosF32 (0x100);
  testCosF32 (0x10000);
  testCosF32 (0x200000);
  testCosF32 (0x400000);
  testCosF32 (0x800000);
  testCosF32 (0xa00000);
  testCosF32 (0xc00000);
  testCosF32 (0xe00000);
  testCosF32 (0x1000000);
  testCosF32 (0xffff0000);

  mat33if16[0][0] = 0x10000;  mat33if16[1][0] = 0x00000;  mat33if16[2][0] = 0x00000;
  mat33if16[0][1] = 0x00000;  mat33if16[1][1] = 0x0b504;  mat33if16[2][1] = -0x0b504;
  mat33if16[0][2] = 0x00000;  mat33if16[1][2] = 0x0b504;  mat33if16[2][2] = 0x0b504;
  vec3if16[0] = 0x10000;  vec3if16[1] = 0x20000;  vec3if16[2] = 0x30000;
  vec3jf16[0] = 0x10000;  vec3jf16[1] = 0x20000;  vec3jf16[2] = 0x30000;
  MulVec3Mat33_F16 (vec3jf16, vec3if16, mat33if16);
  DBUG (("MulVec3Mat33:  vec[0] = 0x%lx.%04lx, ", (uint32)vec3jf16[0]>>16, vec3jf16[0]&0xffff));
  DBUG (("vec[1] = 0x%lx.%04lx, ", (uint32)vec3jf16[1]>>16, vec3jf16[1]&0xffff));
  DBUG (("vec[2] = 0x%lx.%04lx\n", (uint32)vec3jf16[2]>>16, vec3jf16[2]&0xffff));

  vec3jf16[0] = 0x10000;  vec3jf16[1] = 0x20000;  vec3jf16[2] = 0x30000;
  aMulVec3Mat33_F16 (vec3jf16, vec3if16, mat33if16);
  DBUG (("MulVec3Mat33:  vec[0] = 0x%lx.%04lx, ", (uint32)vec3jf16[0]>>16, vec3jf16[0]&0xffff));
  DBUG (("vec[1] = 0x%lx.%04lx, ", (uint32)vec3jf16[1]>>16, vec3jf16[1]&0xffff));
  DBUG (("vec[2] = 0x%lx.%04lx\n", (uint32)vec3jf16[2]>>16, vec3jf16[2]&0xffff));

  vec3if16[0] = 0x10000;  vec3if16[1] = 0x20000;  vec3if16[2] = 0x30000;
  if16 = Dot3_F16 (vec3if16, vec3if16);
  DBUG (("dot3 = 0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
  vec4if16[0] = 0x10000;  vec4if16[1] = 0x20000;  vec4if16[2] = 0x30000;  vec4if16[3] = 0x40000;
  if16 = Dot4_F16 (vec4if16, vec4if16);
  DBUG (("dot4 = 0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
  if16 = AbsVec3_F16 (vec3if16);
  DBUG (("absvec3 = 0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));
  if16 = AbsVec4_F16 (vec4if16);
  DBUG (("absvec4 = 0x%lx.%04lx\n", (uint32)if16>>16, if16&0xffff));

  vec3jf16[0] = 0x10000;  vec3jf16[1] = 0x00000;  vec3jf16[2] = 0x00000;
  Cross3_F16 (vec3jf16, vec3if16, vec3jf16);
  DBUG (("Cross3:  vec[0] = 0x%lx.%04lx, ", (uint32)vec3jf16[0]>>16, vec3jf16[0]&0xffff));
  DBUG (("vec[1] = 0x%lx.%04lx, ", (uint32)vec3jf16[1]>>16, vec3jf16[1]&0xffff));
  DBUG (("vec[2] = 0x%lx.%04lx\n", (uint32)vec3jf16[2]>>16, vec3jf16[2]&0xffff));

  mat44if16[0][0] = 0x10000;  mat44if16[1][0] = 0x00000;  mat44if16[2][0] = 0x00000;
  mat44if16[3][0] = 0x50000;
  mat44if16[0][1] = 0x00000;  mat44if16[1][1] = 0x0b504;  mat44if16[2][1] = -0x0b504;
  mat44if16[3][1] = 0x60000;
  mat44if16[0][2] = 0x00000;  mat44if16[1][2] = 0x0b504;  mat44if16[2][2] = 0x0b504;
  mat44if16[3][2] = 0x70000;
  mat44if16[0][3] = 0x00000;  mat44if16[1][3] = 0x00000;  mat44if16[2][3] = 0x00000;
  mat44if16[3][3] = 0x10000;
  vec4if16[0] = 0x10000;  vec4if16[1] = 0x20000;  vec4if16[2] = 0x30000;  vec4if16[3] = 0x10000;
  MulVec4Mat44_F16 (vec4jf16, vec4if16, mat44if16);
  DBUG (("MulVec4Mat44:  vec[0] = 0x%lx.%04lx, ", (uint32)vec4jf16[0]>>16, vec4jf16[0]&0xffff));
  DBUG (("vec[1] = 0x%lx.%04lx, ", (uint32)vec4jf16[1]>>16, vec4jf16[1]&0xffff));
  DBUG (("vec[2] = 0x%lx.%04lx, ", (uint32)vec4jf16[2]>>16, vec4jf16[2]&0xffff));
  DBUG (("vec[3] = 0x%lx.%04lx\n", (uint32)vec4jf16[3]>>16, vec4jf16[3]&0xffff));

  mat33if16[0][0] = 0x10000;  mat33if16[1][0] = 0x20000;  mat33if16[2][0] = 0x30000;
  mat33if16[0][1] = 0x40000;  mat33if16[1][1] = 0x50000;  mat33if16[2][1] = 0x60000;
  mat33if16[0][2] = 0x70000;  mat33if16[1][2] = 0x80000;  mat33if16[2][2] = 0x90000;
  Transpose33_F16 (mat33jf16, mat33if16);
  DBUG (("Transpose33_F16:\n"));
  DBUG (("0x%lx.%04lx ", mat33jf16[0][0]>>16, mat33jf16[0][0]&0xffff));
  DBUG (("0x%lx.%04lx ", mat33jf16[1][0]>>16, mat33jf16[1][0]&0xffff));
  DBUG (("0x%lx.%04lx\n", mat33jf16[2][0]>>16, mat33jf16[2][0]&0xffff));
  DBUG (("0x%lx.%04lx ", mat33jf16[0][1]>>16, mat33jf16[0][1]&0xffff));
  DBUG (("0x%lx.%04lx ", mat33jf16[1][1]>>16, mat33jf16[1][1]&0xffff));
  DBUG (("0x%lx.%04lx\n", mat33jf16[2][1]>>16, mat33jf16[2][1]&0xffff));
  DBUG (("0x%lx.%04lx ", mat33jf16[0][2]>>16, mat33jf16[0][2]&0xffff));
  DBUG (("0x%lx.%04lx ", mat33jf16[1][2]>>16, mat33jf16[1][2]&0xffff));
  DBUG (("0x%lx.%04lx\n", mat33jf16[2][2]>>16, mat33jf16[2][2]&0xffff));
}


