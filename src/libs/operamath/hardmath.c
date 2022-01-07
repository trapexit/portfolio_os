/***************************************************************\
*								*
* File: hardmath.c						*
*								*
* Opera math routines						*
*								*
* Functions that access the matrix hardware                     *
*                                                               *
* By:  Stephen H. Landrum					*
*								*
* Last update:  17-Jun-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company                     *
* All rights reserved						*
* This program is proprietary and confidential			*
*								*
\***************************************************************/


#include "mathfolio.h"


void
sCross3_F16 (vec3f16 dest, vec3f16 v1, vec3f16 v2)
{
  frac16 i, j, k;

  i = MulSF16(v1[1],v2[2]) - MulSF16(v1[2],v2[1]);
  j = MulSF16(v1[2],v2[0]) - MulSF16(v1[0],v2[2]);
  k = MulSF16(v1[0],v2[1]) - MulSF16(v1[1],v2[0]);

  dest[0] = i;
  dest[1] = j;
  dest[2] = k;
}


frac16
sDot3_F16 (vec3f16 v1, vec3f16 v2)
{
  return MulSF16(v1[0],v2[0])+MulSF16(v1[1],v2[1])+MulSF16(v1[2],v2[2]);
}


frac16
sDot4_F16 (vec4f16 v1, vec4f16 v2)
{
  return MulSF16(v1[0],v2[0])+MulSF16(v1[1],v2[1])+MulSF16(v1[2],v2[2])+MulSF16(v1[3],v2[3]);
}


void
MulManyF16 (frac16 *dest, frac16 *src1, frac16 *src2, int32 count)
/* Multiply an array of 16.16 fractions by another array of fractions */
{
  while (count-- > 0) {
    *dest++ = MulSF16 (*src1++, *src2++);
  }
}


void
MulScalarF16 (frac16 *dest, frac16 *src, frac16 scalar, int32 count)
/* Multiply a 16.16 scalar by an array of 16.16 fractions */
{
  while (count-- > 0) {
    *dest++ = MulSF16 (*src++, scalar);
  }
}


void
sMulVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat)
/* Multiply a 3x3 matrix of 16.16 values by a vector and return the vector result */
{
  frac16 i, j, k;

  i = MulSF16(vec[0],mat[0][0]) + MulSF16(vec[1],mat[1][0]) + MulSF16(vec[2],mat[2][0]);
  j = MulSF16(vec[0],mat[0][1]) + MulSF16(vec[1],mat[1][1]) + MulSF16(vec[2],mat[2][1]);
  k = MulSF16(vec[0],mat[0][2]) + MulSF16(vec[1],mat[1][2]) + MulSF16(vec[2],mat[2][2]);

  dest[0] = i;
  dest[1] = j;
  dest[2] = k;
}


void
sMulVec3Mat33DivZ_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, frac16 n)
/* Multiply a 3x3 matrix of 16.16 values by a vector and return the vector result */
{
  frac16 i, j, k;
  frac32 l64, m64, n64, o64;

  i = MulSF16(vec[0],mat[0][0]) + MulSF16(vec[1],mat[1][0]) + MulSF16(vec[2],mat[2][0]);
  j = MulSF16(vec[0],mat[0][1]) + MulSF16(vec[1],mat[1][1]) + MulSF16(vec[2],mat[2][1]);
  k = MulSF16(vec[0],mat[0][2]) + MulSF16(vec[1],mat[1][2]) + MulSF16(vec[2],mat[2][2]);

/*  dest[0] = i; */
/*  dest[1] = j; */
  dest[2] = k;

  ConvertSF16_F32 (&m64, k);
  l64.hi = n;
  l64.lo = 0;
  DivS64 (&n64, &o64, &l64, &m64);
  MulSF16_F32 (&o64, i, n64.lo);
  dest[0] = ConvertF32_F16 (&o64);
  MulSF16_F32 (&o64, j, n64.lo);
  dest[1] = ConvertF32_F16 (&o64);
}


#ifdef _SUPPORT_RED
void
MulVec3Mat33DivZ_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, frac16 n)
/* Multiply a 3x3 matrix of 16.16 values by a vector and return the vector result */
{
  frac32 l64, m64, n64, o64;

  MulVec3Mat33_F16 (dest, vec, mat);

  ConvertSF16_F32 (&m64, dest[2]);
  l64.hi = n;
  l64.lo = 0;
  DivS64 (&n64, &o64, &l64, &m64);
  MulSF16_F32 (&o64, dest[0], n64.lo);
  dest[0] = ConvertF32_F16 (&o64);
  MulSF16_F32 (&o64, dest[0], n64.lo);
  dest[1] = ConvertF32_F16 (&o64);
}
#endif /* _SUPPORT_RED */


void
sMulManyVec3Mat33_F16 (vec3f16 *dest, vec3f16 *src, mat33f16 mat, int32 count)
/* Multiply many vectors by a matrix */
{
  while (count-- > 0) {
    sMulVec3Mat33_F16 (*dest++, *src++, mat);
  }
}


void
sMulManyVec3Mat33DivZ_F16 (mmv3m33d *s)
/* Multiply many vectors by a matrix */
{
  vec3f16 *dest, *src;
  uint32 count;

  dest = s->dest;
  src = s->src;
  count = s->count;
  while (count-- > 0) {
    sMulVec3Mat33DivZ_F16 (*dest++, *src++, *s->mat, s->n);
  }
}


#ifdef _SUPPORT_RED
void
MulManyVec3Mat33DivZ_F16 (mmv3m33d *s)
/* Multiply many vectors by a matrix */
{
  vec3f16 *dest, *src;
  uint32 count;

  dest = s->dest;
  src = s->src;
  count = s->count;
  while (count-- > 0) {
    MulVec3Mat33DivZ_F16 (*dest++, *src++, *s->mat, s->n);
  }
}
#endif /* _SUPPORT_RED */


#if 0
void
gMulManyVec3Mat33DivZ_F16 (mmv3m33d *s)
/* Multiply many vectors by a matrix */
{
  vec3f16 *dest, *src;
  uint32 count;

  dest = s->dest;
  src = s->src;
  count = s->count;
  while (count-- > 0) {
    gMulVec3Mat33DivZ_F16 (*dest++, *src++, *s->mat, s->n);
  }
}
#endif


void
sMulObjectVec3Mat33_F16 (void **objectlist, ObjOffset1 *offsetstruct, int32 count)
/* Multiply many vectors within object structures by a matrix within that object
 * structure, and repeat over a number of objects
 */
{
  while (count-- > 0) {
   sMulManyVec3Mat33_F16 (*(vec3f16 **)((int32)(*objectlist)+offsetstruct->oo1_DestArrayPtrOffset),
			   *(vec3f16 **)((int32)(*objectlist)+offsetstruct->oo1_SrcArrayPtrOffset),
			   *(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo1_MatOffset),
			   *(int32 *)((int32)(*objectlist)+offsetstruct->oo1_CountOffset));
    objectlist++;
  }
}


#ifdef _SUPPORT_RED
void
MulObjectVec3Mat33_F16 (void **objectlist, ObjOffset1 *offsetstruct, int32 count)
/* Multiply many vectors within object structures by a matrix within that object
 * structure, and repeat over a number of objects
 */
{
  while (count-- > 0) {
    MulManyVec3Mat33_F16 (*(vec3f16 **)((int32)(*objectlist)+offsetstruct->oo1_DestArrayPtrOffset),
			  *(vec3f16 **)((int32)(*objectlist)+offsetstruct->oo1_SrcArrayPtrOffset),
			  *(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo1_MatOffset),
			  *(int32 *)((int32)(*objectlist)+offsetstruct->oo1_CountOffset));
    objectlist++;
  }
}
#endif /* _SUPPORT_RED */


void
gMulObjectVec3Mat33_F16 (void **objectlist, ObjOffset1 *offsetstruct, int32 count)
/* Multiply many vectors within object structures by a matrix within that object
 * structure, and repeat over a number of objects
 */
{
  while (count-- > 0) {
   gMulManyVec3Mat33_F16 (*(vec3f16 **)((int32)(*objectlist)+offsetstruct->oo1_DestArrayPtrOffset),
			  *(vec3f16 **)((int32)(*objectlist)+offsetstruct->oo1_SrcArrayPtrOffset),
			  *(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo1_MatOffset),
			  *(int32 *)((int32)(*objectlist)+offsetstruct->oo1_CountOffset));
#if 0
   Superkprintf ("%lx %lx\n", 
		 *(vec3f16 **)((int32)(*objectlist)+offsetstruct->oo1_DestArrayPtrOffset),
		 *(vec3f16 **)((int32)(*objectlist)+offsetstruct->oo1_SrcArrayPtrOffset));
   Superkprintf ("%lx %lx\n",
		 *(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo1_MatOffset),
		 *(int32 *)((int32)(*objectlist)+offsetstruct->oo1_CountOffset));
#endif
   objectlist++;
  }
}


void
sMulMat33Mat33_F16 (mat33f16 dest, mat33f16 src1, mat33f16 src2)
/* Multiply two 3x3 matrices of 16.16 values and return the result */
{
  if ((dest==src1) || (dest==src2)) {
    mat33f16 tmp;
    sMulVec3Mat33_F16 (tmp[0], src1[0], src2);
    sMulVec3Mat33_F16 (tmp[1], src1[1], src2);
    sMulVec3Mat33_F16 (tmp[2], src1[2], src2);
    memcpy (dest, tmp, sizeof(tmp));
  } else {
    sMulVec3Mat33_F16 (dest[0], src1[0], src2);
    sMulVec3Mat33_F16 (dest[1], src1[1], src2);
    sMulVec3Mat33_F16 (dest[2], src1[2], src2);
  }
}


void
sMulObjectMat33_F16 (void **objectlist, ObjOffset2 *offsetstruct, mat33f16 mat, int32 count)
/* Multiply a matrix within an object structure by an external matrix, and repeat
 * over a number of objects
 */
{
  while (count-- > 0) {
    sMulMat33Mat33_F16 (*(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo2_DestMatOffset),
			*(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo2_SrcMatOffset),
			mat);
    objectlist++;
  }
}


#ifdef _SUPPORT_RED
void
MulObjectMat33_F16 (void **objectlist, ObjOffset2 *offsetstruct, mat33f16 mat, int32 count)
/* Multiply a matrix within an object structure by an external matrix, and repeat
 * over a number of objects
 */
{
  while (count-- > 0) {
    MulMat33Mat33_F16 (*(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo2_DestMatOffset),
			*(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo2_SrcMatOffset),
			mat);
    objectlist++;
  }
}
#endif /* _SUPPORT_RED */


void
gMulObjectMat33_F16 (void **objectlist, ObjOffset2 *offsetstruct, mat33f16 mat, int32 count)
/* Multiply a matrix within an object structure by an external matrix, and repeat
 * over a number of objects
 */
{
  while (count-- > 0) {
    gMulMat33Mat33_F16 (*(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo2_DestMatOffset),
			*(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo2_SrcMatOffset),
			mat);
    objectlist++;
  }
}


void
sMulVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat)
/* Multiply a 4x4 matrix of 16.16 values by a vector and return the vector result */
{
  frac16 i, j, k, l;

  i = MulSF16(vec[0],mat[0][0]) + MulSF16(vec[1],mat[1][0]) + MulSF16(vec[2],mat[2][0])
    + MulSF16(vec[3],mat[3][0]);
  j = MulSF16(vec[0],mat[0][1]) + MulSF16(vec[1],mat[1][1]) + MulSF16(vec[2],mat[2][1])
    + MulSF16(vec[3],mat[3][1]);
  k = MulSF16(vec[0],mat[0][2]) + MulSF16(vec[1],mat[1][2]) + MulSF16(vec[2],mat[2][2])
    + MulSF16(vec[3],mat[3][2]);
  l = MulSF16(vec[0],mat[0][3]) + MulSF16(vec[1],mat[1][3]) + MulSF16(vec[2],mat[2][3])
    + MulSF16(vec[3],mat[3][3]);

  dest[0] = i;
  dest[1] = j;
  dest[2] = k;
  dest[3] = l;
}


void
sMulManyVec4Mat44_F16 (vec4f16 *dest, vec4f16 *src, mat44f16 mat, int32 count)
/* Multiply many vectors by a matrix */
{
  while (count-- > 0) {
    sMulVec4Mat44_F16 (*dest++, *src++, mat);
  }
}


#ifdef _SUPPORT_RED
void
MulObjectVec4Mat44_F16 (void **objectlist, ObjOffset1 *offsetstruct, int32 count)
/* Multiply many vectors within object structures by a matrix within that object
 * structure, and repeat over a number of objects
 */
{
  while (count-- > 0) {
    MulManyVec4Mat44_F16 (*(vec4f16 **)((int32)(*objectlist)+offsetstruct->oo1_DestArrayPtrOffset),
			  *(vec4f16 **)((int32)(*objectlist)+offsetstruct->oo1_SrcArrayPtrOffset),
			  *(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo1_MatOffset),
			  *(int32 *)((int32)(*objectlist)+offsetstruct->oo1_CountOffset));
    objectlist++;
  }
}
#endif /* _SUPPORT_RED */


void
gMulObjectVec4Mat44_F16 (void **objectlist, ObjOffset1 *offsetstruct, int32 count)
/* Multiply many vectors within object structures by a matrix within that object
 * structure, and repeat over a number of objects
 */
{
  while (count-- > 0) {
   gMulManyVec4Mat44_F16 (*(vec4f16 **)((int32)(*objectlist)+offsetstruct->oo1_DestArrayPtrOffset),
			   *(vec4f16 **)((int32)(*objectlist)+offsetstruct->oo1_SrcArrayPtrOffset),
			   *(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo1_MatOffset),
			   *(int32 *)((int32)(*objectlist)+offsetstruct->oo1_CountOffset));
    objectlist++;
  }
}


void
sMulObjectVec4Mat44_F16 (void **objectlist, ObjOffset1 *offsetstruct, int32 count)
/* Multiply many vectors within object structures by a matrix within that object */
/* structure, and repeat over a number of objects */
{
  while (count-- > 0) {
   sMulManyVec4Mat44_F16 (*(vec4f16 **)((int32)(*objectlist)+offsetstruct->oo1_DestArrayPtrOffset),
			   *(vec4f16 **)((int32)(*objectlist)+offsetstruct->oo1_SrcArrayPtrOffset),
			   *(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo1_MatOffset),
			   *(int32 *)((int32)(*objectlist)+offsetstruct->oo1_CountOffset));
    objectlist++;
  }
}


void
sMulMat44Mat44_F16 (mat44f16 dest, mat44f16 src1, mat44f16 src2)
/* Multiply two 4x4 matrices of 16.16 values and return the result */
{
  if ((dest==src1) || (dest==src2)) {
    mat44f16 tmp;
    sMulVec4Mat44_F16 (tmp[0], src1[0], src2);
    sMulVec4Mat44_F16 (tmp[1], src1[1], src2);
    sMulVec4Mat44_F16 (tmp[2], src1[2], src2);
    sMulVec4Mat44_F16 (tmp[3], src1[3], src2);
    memcpy (dest, tmp, sizeof(tmp));
  } else {
    sMulVec4Mat44_F16 (dest[0], src1[0], src2);
    sMulVec4Mat44_F16 (dest[1], src1[1], src2);
    sMulVec4Mat44_F16 (dest[2], src1[2], src2);
    sMulVec4Mat44_F16 (dest[3], src1[3], src2);
  }
}


#ifdef _SUPPORT_RED
void
MulObjectMat44_F16 (void **objectlist, ObjOffset2 *offsetstruct, mat44f16 mat, int32 count)
/* Multiply a matrix within an object structure by an external matrix, and repeat
 * over a number of objects
 */
{
  while (count-- > 0) {
    MulMat44Mat44_F16 (*(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo2_DestMatOffset),
		       *(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo2_SrcMatOffset),
		       mat);
    objectlist++;
  }
}
#endif /* _SUPPORT_RED */


void
gMulObjectMat44_F16 (void **objectlist, ObjOffset2 *offsetstruct, mat44f16 mat, int32 count)
/* Multiply a matrix within an object structure by an external matrix, and repeat
 * over a number of objects
 */
{
  while (count-- > 0) {
    gMulMat44Mat44_F16 (*(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo2_DestMatOffset),
			*(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo2_SrcMatOffset),
			mat);
    objectlist++;
  }
}


void
sMulObjectMat44_F16 (void **objectlist, ObjOffset2 *offsetstruct, mat44f16 mat, int32 count)
/* Multiply a matrix within an object structure by an external matrix, and repeat
 * over a number of objects
 */
{
  while (count-- > 0) {
    sMulMat44Mat44_F16 (*(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo2_DestMatOffset),
		       *(mat44f16 *)((int32)(*objectlist)+offsetstruct->oo2_SrcMatOffset),
		       mat);
    objectlist++;
  }
}


#ifdef _RED_SUPPORT
frac16
AbsVec3_F16 (vec3f16 vec)
{
  return SqrtF16(Dot3_F16(vec,vec));
}
#endif /* _RED_SUPPORT */


frac16
gAbsVec3_F16 (vec3f16 vec)
{
  return SqrtF16(gDot3_F16(vec,vec));
}


frac16
sAbsVec3_F16 (vec3f16 vec)
{
  return SqrtF16(sDot3_F16(vec,vec));
}


#ifdef _RED_SUPPORT
frac16
AbsVec4_F16 (vec4f16 vec)
{
  return SqrtF16(Dot4_F16(vec,vec));
}
#endif /* _RED_SUPPORT */


frac16
gAbsVec4_F16 (vec4f16 vec)
{
  return SqrtF16(gDot4_F16(vec,vec));
}


frac16
sAbsVec4_F16 (vec4f16 vec)
{
  return SqrtF16(sDot4_F16(vec,vec));
}



