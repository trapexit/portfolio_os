/***************************************************************\
*								*
* Header file for 3D simulation routines			*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  10-Feb-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company, Inc.               *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/

#pragma force_top_level
#pragma include_only_once

#include "types.h"
#include "io.h"
#include "graphics.h"
#include "operamath.h"


typedef vec3f16 Point3d;


extern long tx, ty, tz;
extern long wincx, wincy;
extern frac16 zoom;

extern long sintable[1024*5/4];

#define costable (&(sintable[256]))


typedef struct Face3d {
  long corners[4];
  CCB *ccb;
  ulong flags;
} Face3d;

typedef struct Object3d {
  long numcorners;
  long numfaces;
  Point3d location;
  mat33f16 orientation;
  Point3d (*corners)[];
  Face3d (*faces)[];
  Point3d (*xcorners)[];
  Point3d (*rcorners)[];
  Point3d boxmin;
  Point3d boxmax;
  ulong flags;
  char *name;
} Object3d;



void matrixmul (mat33f16 dest, mat33f16 m1, mat33f16 m2);
void transposemul (mat33f16 dest, mat33f16 m1, mat33f16 m2);
void matrixvectormul (vec3f16 dest, mat33f16 m, vec3f16 v);
void transposevectormul (vec3f16 dest, mat33f16 m, vec3f16 v);
frac16 dotprod (vec3f16 v1, vec3f16 v2);
void transpose (mat33f16 dest, mat33f16 m);
void makerotmatrix (mat33f16 dest, long xrot, long yrot, long zrot);
void rotatematrix (mat33f16 dest, mat33f16 m, long xrot, long yrot, long zrot);
void normalize (mat33f16 dest, mat33f16 src);

/* void printfrac16 (Console *con, frac16 i); */

void drawframe (Item bitmapitem, Object3d *sourceobject);
void drawsolid (Item bitmapitem, Object3d *sourceobject, int dbug);




