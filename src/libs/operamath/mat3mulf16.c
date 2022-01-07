/***************************************************************\
*								*
* File: mat3mulf16.c						*
*								*
* Opera math routines						*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  22-Mar-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company, Inc.               *
* All rights reserved						*
* This program is proprietary and confidential			*
*								*
\***************************************************************/


#include "mathfolio.h"


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
sMulManyVec3Mat33_F16 (vec3f16 *dest, vec3f16 *src, mat33f16 mat, int32 count)
/* Multiply many vectors by a matrix */
{
  while (count-- > 0) {
    sMulVec3Mat33_F16 (*dest++, *src++, mat);
  }
}


void
MulObjectVec3Mat33_F16 (void **objectlist, ObjOffset1 *offsetstruct, int32 count)
/* Multiply many vectors within object structures by a matrix within that object */
/* structure, and repeat over a number of objects */
{
  while (count-- > 0) {
    sMulManyVec3Mat33_F16 ((vec3f16 *)((int32)(*objectlist)+offsetstruct->oo1_DestArrayPtrOffset),
			   (vec3f16 *)((int32)(*objectlist)+offsetstruct->oo1_SrcArrayPtrOffset),
			   *(mat33f16 *)((int32)(*objectlist)+offsetstruct->oo1_MatOffset),
			   *(int32 *)((int32)(*objectlist)+offsetstruct->oo1_CountOffset));
    objectlist++;
  }
}


