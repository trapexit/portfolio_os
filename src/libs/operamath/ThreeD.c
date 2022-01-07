/***************************************************************\
*								*
* Graphics routines and math support for 3d simulation		*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  14-Jun-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company, Inc.               *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/


#define DBUG(x) { printf x; }
#define NDBUG(x) /* { printf x; } */

#include "ThreeD.h"
#include "list.h"
#include "task.h"
#include "strings.h"
#include "stdio.h"

#include "operamath.h"



long tx, ty, tz;
long wincx, wincy;
frac16 zoom;


/* Console *DBUGWindow; */
#define CDBUG(x) /* { ConsolePrintf x; } */


Object3d Camera = {
  0,
  0,
  { Convert32_F16(0), Convert32_F16(0), Convert32_F16(-100) },
  { Convert32_F16(1), Convert32_F16(0), Convert32_F16(0),
    Convert32_F16(0), Convert32_F16(1), Convert32_F16(0),
    Convert32_F16(0), Convert32_F16(0), Convert32_F16(1) },
  NULL,
  NULL,
  NULL,
  NULL,
  {0, 0, 0},
  {0, 0, 0},
  0,
  "Camera",
};









#if 0
frac16
sin1616 (frac16 x)
{
  x = x%411774;
  if (x<0) { x=x+411774; }
  return (sintable[((ufrac16)x)*(ufrac16)1024/(ufrac16)411774]);
}


frac16
cos1616 (frac16 x)
{
  x = x%411774;
  if (x<0) { x=x+411774; }
  return (sintable[((ufrac16)x)*(ufrac16)1024/(ufrac16)411774+256]);
}
#endif


#if 0
void
printfrac16 (Console *con, frac16 i)
{
  ConsolePrintD (con, i/65536);
  ConsolePutChar (con, '.');
  while (i -= ((i/65536)*65536)) {
    ConsolePrintD (con, i/65536);
    i *= 10;
  }
}
#endif



void
matrixmul (mat33f16 dest, mat33f16 m1, mat33f16 m2)
{
  mat33f16 d;

  d[0][0] = MulSF16((m1)[0][0],(m2)[0][0]) + MulSF16((m1)[0][1],(m2)[1][0])
		+ MulSF16((m1)[0][2],(m2)[2][0]);
  d[0][1] = MulSF16((m1)[0][0],(m2)[0][1]) + MulSF16((m1)[0][1],(m2)[1][1])
		+ MulSF16((m1)[0][2],(m2)[2][1]);
  d[0][2] = MulSF16((m1)[0][0],(m2)[0][2]) + MulSF16((m1)[0][1],(m2)[1][2])
		+ MulSF16((m1)[0][2],(m2)[2][2]);
  d[1][0] = MulSF16((m1)[1][0],(m2)[0][0]) + MulSF16((m1)[1][1],(m2)[1][0])
		+ MulSF16((m1)[1][2],(m2)[2][0]);
  d[1][1] = MulSF16((m1)[1][0],(m2)[0][1]) + MulSF16((m1)[1][1],(m2)[1][1])
		+ MulSF16((m1)[1][2],(m2)[2][1]);
  d[1][2] = MulSF16((m1)[1][0],(m2)[0][2]) + MulSF16((m1)[1][1],(m2)[1][2])
		+ MulSF16((m1)[1][2],(m2)[2][2]);
  d[2][0] = MulSF16((m1)[2][0],(m2)[0][0]) + MulSF16((m1)[2][1],(m2)[1][0])
		+ MulSF16((m1)[2][2],(m2)[2][0]);
  d[2][1] = MulSF16((m1)[2][0],(m2)[0][1]) + MulSF16((m1)[2][1],(m2)[1][1])
		+ MulSF16((m1)[2][2],(m2)[2][1]);
  d[2][2] = MulSF16((m1)[2][0],(m2)[0][2]) + MulSF16((m1)[2][1],(m2)[1][2])
		+ MulSF16((m1)[2][2],(m2)[2][2]);
  memcpy (dest, &d, sizeof(mat33f16));
}


void
transposemul (mat33f16 dest, mat33f16 m1, mat33f16 m2)
{
  mat33f16 d;

  d[0][0] = MulSF16((m1)[0][0],(m2)[0][0]) + MulSF16((m1)[1][0],(m2)[1][0])
		+ MulSF16((m1)[2][0],(m2)[2][0]);
  d[0][1] = MulSF16((m1)[0][0],(m2)[0][1]) + MulSF16((m1)[1][0],(m2)[1][1])
		+ MulSF16((m1)[2][0],(m2)[2][1]);
  d[0][2] = MulSF16((m1)[0][0],(m2)[0][2]) + MulSF16((m1)[1][0],(m2)[1][2])
		+ MulSF16((m1)[2][0],(m2)[2][2]);
  d[1][0] = MulSF16((m1)[0][1],(m2)[0][0]) + MulSF16((m1)[1][1],(m2)[1][0])
		+ MulSF16((m1)[2][1],(m2)[2][0]);
  d[1][1] = MulSF16((m1)[0][1],(m2)[0][1]) + MulSF16((m1)[1][1],(m2)[1][1])
		+ MulSF16((m1)[2][1],(m2)[2][1]);
  d[1][2] = MulSF16((m1)[0][1],(m2)[0][2]) + MulSF16((m1)[1][1],(m2)[1][2])
		+ MulSF16((m1)[2][1],(m2)[2][2]);
  d[2][0] = MulSF16((m1)[0][2],(m2)[0][0]) + MulSF16((m1)[1][2],(m2)[1][0])
		+ MulSF16((m1)[2][2],(m2)[2][0]);
  d[2][1] = MulSF16((m1)[0][2],(m2)[0][1]) + MulSF16((m1)[1][2],(m2)[1][1])
		+ MulSF16((m1)[2][2],(m2)[2][1]);
  d[2][2] = MulSF16((m1)[0][2],(m2)[0][2]) + MulSF16((m1)[1][2],(m2)[1][2])
		+ MulSF16((m1)[2][2],(m2)[2][2]);
  memcpy (dest, &d, sizeof(mat33f16));
}


void
matrixvectormul (vec3f16 dest, mat33f16 m, vec3f16 v)
{
  vec3f16 d;

  d[0] = MulSF16((m)[0][0],(v)[0]) + MulSF16((m)[0][1],(v)[1]) + MulSF16((m)[0][2],(v)[2]);
  d[1] = MulSF16((m)[1][0],(v)[0]) + MulSF16((m)[1][1],(v)[1]) + MulSF16((m)[1][2],(v)[2]);
  d[2] = MulSF16((m)[2][0],(v)[0]) + MulSF16((m)[2][1],(v)[1]) + MulSF16((m)[2][2],(v)[2]);
  memcpy (dest, &d, sizeof(vec3f16));
}


void
transposevectormul (vec3f16 dest, mat33f16 m, vec3f16 v)
{
  vec3f16 d;

  d[0] = MulSF16((m)[0][0],(v)[0]) + MulSF16((m)[1][0],(v)[1]) + MulSF16((m)[2][0],(v)[2]);
  d[1] = MulSF16((m)[0][1],(v)[0]) + MulSF16((m)[1][1],(v)[1]) + MulSF16((m)[2][1],(v)[2]);
  d[2] = MulSF16((m)[0][2],(v)[0]) + MulSF16((m)[1][2],(v)[1]) + MulSF16((m)[2][2],(v)[2]);
  memcpy (dest, &d, sizeof(vec3f16));
}


frac16
dotproduct (vec3f16 v1, vec3f16 v2)
{
  return MulSF16((v1)[0],(v2)[0]) + MulSF16((v1)[1],(v2)[1]) + MulSF16((v1)[2],(v2)[2]);
}


void
transpose (mat33f16 dest, mat33f16 m)
{
  (dest)[0][0] = (m)[0][0];
  (dest)[0][1] = (m)[1][0];
  (dest)[0][2] = (m)[2][0];
  (dest)[1][0] = (m)[0][1];
  (dest)[1][1] = (m)[1][1];
  (dest)[1][2] = (m)[2][1];
  (dest)[2][0] = (m)[0][2];
  (dest)[2][1] = (m)[1][2];
  (dest)[2][2] = (m)[2][2];
}


void
makerotmatrix (mat33f16 dest, long xrot, long yrot, long zrot)
{
  frac16 sx, cx, sy, cy, sz, cz;

  sx = SinF16(xrot);
  cx = CosF16(xrot);
  sy = SinF16(yrot);
  cy = CosF16(yrot);
  sz = SinF16(zrot);
  cz = CosF16(zrot);

  (dest)[0][0] = MulSF16(cz,cy) - MulSF16(sz,MulSF16(sy,sx));
  (dest)[0][1] = MulSF16(sz,cy) + MulSF16(cz,MulSF16(sy,sx));
  (dest)[0][2] = -MulSF16(sy,cx);

  (dest)[1][0] = -MulSF16(sz,cx);
  (dest)[1][1] = MulSF16(cz,cx);
  (dest)[1][2] = sx;

  (dest)[2][0] = MulSF16(cz,sy) + MulSF16(sz,MulSF16(cy,sx));
  (dest)[2][1] = MulSF16(sz,sy) - MulSF16(cz,MulSF16(cy,sx));
  (dest)[2][2] = MulSF16(cy,cx);
}


void
rotatematrix (mat33f16 dest, mat33f16 m, long xrot, long yrot, long zrot)
{
  mat33f16 rot;

  makerotmatrix (rot, xrot, yrot, zrot);
  /* matrixmul (dest, rot, m); */
  MulMat33Mat33_F16 (dest, m, rot);
}


void
drawframe (Item bitmapitem, Object3d *sourceobject)
{
  long i;
  /* char outstr[100]; */
  Object3d tempobject;
  vec3f16 temppoints[20];
  GrafCon gc;

  CDBUG ((DBUGWindow, "Entering drawframe\n"));

  makerotmatrix (Camera.orientation, tx, ty, tz);

  /* CDBUG ((DBUGWindow, "%d, %d\n", sourceobject->numcorners, sourceobject->numfaces)); */
		/* copy source object into temp object */
  tempobject.numcorners = sourceobject->numcorners;
  tempobject.numfaces = sourceobject->numfaces;
  tempobject.corners = &temppoints;
  tempobject.faces = sourceobject->faces;
		/* translate position based on camera */
  tempobject.location[0] = sourceobject->location[0] - Camera.location[0];
  tempobject.location[1] = sourceobject->location[1] - Camera.location[1];
  tempobject.location[2] = sourceobject->location[2] - Camera.location[2];
  CDBUG ((DBUGWindow, "%d %d %d\n", tempobject.location[0], tempobject.location[1],
	 tempobject.location[2]));
		/* rotate relative to the camera */
  transposemul (tempobject.orientation, Camera.orientation, sourceobject->orientation);
  transposevectormul (tempobject.location, Camera.orientation, tempobject.location);
  CDBUG ((DBUGWindow, "%d %d %d\n", tempobject.location[0], tempobject.location[1],
	 tempobject.location[2]));
		/* rotate, translate, and tranSForm corner points to screen coordinates */
  for (i=0; i<tempobject.numcorners; i++) {
    matrixvectormul (temppoints[i], tempobject.orientation, (*sourceobject->corners)[i]);
    CDBUG ((DBUGWindow, "%d, %d\n",
	   (temppoints[i])[2], tempobject.location[2]));
    (temppoints[i])[0] += tempobject.location[0];
    (temppoints[i])[1] += tempobject.location[1];
    (temppoints[i])[2] += tempobject.location[2];

    CDBUG ((DBUGWindow, "%d\n", (temppoints[i])[2]));
    /* CDBUG ((DBUGWindow, "%d %d %d\n", (temppoints[i])[0], (temppoints[i])[2],
	   DivSF16((temppoints[i])[0],(temppoints[i])[2]))); */
    (temppoints[i])[0] =
      ConvertF16_32(MulSF16(zoom,DivSF16((temppoints[i])[0],(temppoints[i])[2])));
    (temppoints[i])[1] =
      ConvertF16_32(MulSF16(zoom,DivSF16((temppoints[i])[1],(temppoints[i])[2])));
		/* check for overflow */
    /* CDBUG ((DBUGWindow, "%d, %d, %d\n",
	   (temppoints[i])[0], (temppoints[i])[1], (temppoints[i])[2])); */
    if ((temppoints[i])[0] > 32766 || (temppoints[i])[0] > 32766 ||
	(temppoints[i])[1] < -32766 || (temppoints[i])[1] < -32766) {
      (temppoints[i])[2] = 0;
    }
						/* normalize to screen coordinates */
    /* CDBUG ((DBUGWindow, "%d, %d, %d\n",
	   (temppoints[i])[0], (temppoints[i])[1], (temppoints[i])[2])); */
    (temppoints[i])[0] = wincx + (temppoints[i])[0];
    (temppoints[i])[1] = wincy - (temppoints[i])[1];
  }

  /* SetPort (imagewindow); */		/* Set up pointers to the image window */
  /* ForeColor ( (sourceobject==objectlist[curObject]) ? redColor : blackColor ); */
  SetFGPen (&gc, MakeRGB15(0,0,0));	/* black */

  for (i=0; i<tempobject.numfaces; i++) {
		/* check to see if face penetrates camera plane */
    if ((temppoints[(*tempobject.faces)[i].corners[0]])[2]<=0 ||
	(temppoints[(*tempobject.faces)[i].corners[1]])[2]<=0 ||
	(temppoints[(*tempobject.faces)[i].corners[2]])[2]<=0 ||
	(temppoints[(*tempobject.faces)[i].corners[3]])[2]<=0)	continue;

    MoveTo (&gc, (temppoints[(*tempobject.faces)[i].corners[0]])[0],
	    (temppoints[(*tempobject.faces)[i].corners[0]])[1]);
    DrawTo (bitmapitem, &gc, (temppoints[(*tempobject.faces)[i].corners[1]])[0],
	    (temppoints[(*tempobject.faces)[i].corners[1]])[1]);
    DrawTo (bitmapitem, &gc, (temppoints[(*tempobject.faces)[i].corners[2]])[0],
	    (temppoints[(*tempobject.faces)[i].corners[2]])[1]);
    DrawTo (bitmapitem, &gc, (temppoints[(*tempobject.faces)[i].corners[3]])[0],
	    (temppoints[(*tempobject.faces)[i].corners[3]])[1]);
    DrawTo (bitmapitem, &gc, (temppoints[(*tempobject.faces)[i].corners[0]])[0],
	    (temppoints[(*tempobject.faces)[i].corners[0]])[1]);
  }

#if 0
  if (textFlag) {
    SetPort (textwindow);
    EraseRect (&textwinrect);
    ForeColor (blackColor);

    sprintf (outstr, "Camera x, y, z = %Lg, %Lg, %Lg,   Zoom = %Lg",
	     mtoe(Camera.location.x), mtoe(Camera.location.y), mtoe(Camera.location.z),
	     mtoe(zoom));
    MoveTo (10, 10);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "tx, ty, tz = %d, %d, %d", tx, ty, tz);
    MoveTo (10, 25);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "Object = ");
    MoveTo (10, 55);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "%Lg, %Lg, %Lg", mtoe(sourceobject->orientation.xx),
	     mtoe(sourceobject->orientation.xy), mtoe(sourceobject->orientation.xz));
    MoveTo (65, 40);
    DrawText (outstr, 0, strlen(outstr));
    sprintf (outstr, "%Lg, %Lg, %Lg", mtoe(sourceobject->orientation.yx),
	     mtoe(sourceobject->orientation.yy), mtoe(sourceobject->orientation.yz));
    MoveTo (65, 55);
    DrawText (outstr, 0, strlen(outstr));
    sprintf (outstr, "%Lg, %Lg, %Lg", mtoe(sourceobject->orientation.zx),
	     mtoe(sourceobject->orientation.zy), mtoe(sourceobject->orientation.zz));
    MoveTo (65, 70);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "Delta x, y, z = %Lg, %Lg, %Lg", mtoe(tempobject.location.x),
	     mtoe(tempobject.location.y), mtoe(tempobject.location.z));
    MoveTo (10, 85);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "step, count = %ld, %ld", turnstep, turncount);
    MoveTo (10, 100);
    DrawText (outstr, 0, strlen(outstr));
  }
#endif
}


void
drawsolid (Item bitmapitem, Object3d *sourceobject, int cdbug)
{
  long i;
  /* char outstr[100]; */
  Object3d tempobject;
  vec3f16 temppoints[20];
  Point q[4];

  CDBUG ((DBUGWindow, "Entering drawsolid\n"));

  makerotmatrix (Camera.orientation, tx, ty, tz);

  /* CDBUG ((DBUGWindow, "%d, %d\n", sourceobject->numcorners, sourceobject->numfaces)); */
		/* copy source object into temp object */
  tempobject.numcorners = sourceobject->numcorners;
  tempobject.numfaces = sourceobject->numfaces;
  tempobject.corners = &temppoints;
  tempobject.faces = sourceobject->faces;
		/* translate position based on camera */
  tempobject.location[0] = sourceobject->location[0] - Camera.location[0];
  tempobject.location[1] = sourceobject->location[1] - Camera.location[1];
  tempobject.location[2] = sourceobject->location[2] - Camera.location[2];
  CDBUG ((DBUGWindow, "%d %d %d\n", tempobject.location[0], tempobject.location[1],
	 tempobject.location[2]));
		/* rotate relative to the camera */
  transposemul (tempobject.orientation, Camera.orientation, sourceobject->orientation);
  transposevectormul (tempobject.location, Camera.orientation, tempobject.location);
  CDBUG ((DBUGWindow, "%d %d %d\n", tempobject.location[0], tempobject.location[1],
	 tempobject.location[2]));
		/* rotate, translate, and tranSForm corner points to screen coordinates */
  for (i=0; i<tempobject.numcorners; i++) {
    matrixvectormul (temppoints[i], tempobject.orientation, (*sourceobject->corners)[i]);
    CDBUG ((DBUGWindow, "%d, %d\n",
	   (temppoints[i])[2], tempobject.location[2]));
    (temppoints[i])[0] += tempobject.location[0];
    (temppoints[i])[1] += tempobject.location[1];
    (temppoints[i])[2] += tempobject.location[2];

    CDBUG ((DBUGWindow, "%d\n", (temppoints[i])[2]));
    /* CDBUG ((DBUGWindow, "%d %d %d\n", (temppoints[i])[0], (temppoints[i])[2],
	   DivSF16((temppoints[i])[0],(temppoints[i])[2]))); */
    (temppoints[i])[0] =
      ConvertF16_32(MulSF16(zoom,DivSF16((temppoints[i])[0],(temppoints[i])[2])));
    (temppoints[i])[1] =
      ConvertF16_32(MulSF16(zoom,DivSF16((temppoints[i])[1],(temppoints[i])[2])));
		/* check for overflow */
    /* CDBUG ((DBUGWindow, "%d, %d, %d\n",
	   (temppoints[i])[0], (temppoints[i])[1], (temppoints[i])[2])); */
    if ((temppoints[i])[0] > 32766 || (temppoints[i])[0] > 32766 ||
	(temppoints[i])[1] < -32766 || (temppoints[i])[1] < -32766) {
      (temppoints[i])[2] = 0;
    }
						/* normalize to screen coordinates */
    /* CDBUG ((DBUGWindow, "%d, %d, %d\n",
	   (temppoints[i])[0], (temppoints[i])[1], (temppoints[i])[2])); */
    (temppoints[i])[0] = wincx + (temppoints[i])[0];
    (temppoints[i])[1] = wincy - (temppoints[i])[1];
  }

  /* SetPort (imagewindow); */		/* Set up pointers to the image window */
  /* ForeColor ( (sourceobject==objectlist[curObject]) ? redColor : blackColor ); */

  for (i=0; i<tempobject.numfaces; i++) {
		/* check to see if face penetrates camera plane */
    if ((temppoints[(*tempobject.faces)[i].corners[0]])[2]<=0 ||
	(temppoints[(*tempobject.faces)[i].corners[1]])[2]<=0 ||
	(temppoints[(*tempobject.faces)[i].corners[2]])[2]<=0 ||
	(temppoints[(*tempobject.faces)[i].corners[3]])[2]<=0)	continue;

    q[0].pt_X = (temppoints[(*tempobject.faces)[i].corners[0]])[0];
    q[0].pt_Y = (temppoints[(*tempobject.faces)[i].corners[0]])[1];
    q[1].pt_X = (temppoints[(*tempobject.faces)[i].corners[1]])[0];
    q[1].pt_Y = (temppoints[(*tempobject.faces)[i].corners[1]])[1];
    q[2].pt_X = (temppoints[(*tempobject.faces)[i].corners[2]])[0];
    q[2].pt_Y = (temppoints[(*tempobject.faces)[i].corners[2]])[1];
    q[3].pt_X = (temppoints[(*tempobject.faces)[i].corners[3]])[0];
    q[3].pt_Y = (temppoints[(*tempobject.faces)[i].corners[3]])[1];

    if (cdbug) {
#if 0
      ConsolePrintf (DBUGWindow, "\n%d,", q[0].pt_X);
      ConsolePrintf (DBUGWindow, "%d  ", q[0].pt_Y);
      ConsolePrintf (DBUGWindow, "%d,", q[1].pt_X);
      ConsolePrintf (DBUGWindow, "%d  ", q[1].pt_Y);
      ConsolePrintf (DBUGWindow, "%d,", q[2].pt_X);
      ConsolePrintf (DBUGWindow, "%d  ", q[2].pt_Y);
      ConsolePrintf (DBUGWindow, "%d,", q[3].pt_X);
      ConsolePrintf (DBUGWindow, "%d  ", q[3].pt_Y);
#else
# if 0
      ConsolePrintf (DBUGWindow, "\n%d,%d  %d,%d  %d,%d  %d,%d  ",
		     q[0].pt_X, q[0].pt_Y, q[1].pt_X, q[1].pt_Y,
		     q[2].pt_X, q[2].pt_Y, q[3].pt_X, q[3].pt_Y);
# endif
#endif
    }

    {
      int64 tmp1, tmp2, tmp3;
      CCB *ccb = (*tempobject.faces)[i].ccb;
      long ii;

      MapCel (ccb, q);
      
      if (ccb->ccb_Flags&(CCB_ACW|CCB_ACCW) != CCB_ACW|CCB_ACCW) {
	MulS32_64 (&tmp1, ccb->ccb_HDX, ccb->ccb_VDY);
	MulS32_64 (&tmp2, ccb->ccb_VDX, ccb->ccb_HDY);
	Sub64 (&tmp3, &tmp1, &tmp2);
	ii = tmp3.hi;
	if (!ii) {
	  ii = (tmp3.lo?1:0);
	  if (!ii) {
	    MulS32_64 (&tmp1, ccb->ccb_HDX+(ccb->ccb_VDX<<4)+ccb->ccb_HDDX, ccb->ccb_VDY);
	    MulS32_64 (&tmp1, ccb->ccb_VDX, ccb->ccb_HDY+(ccb->ccb_VDY<<4)+ccb->ccb_HDDY);
	    Sub64 (&tmp3, &tmp1, &tmp2);
	    ii = tmp3.hi;
	    if (!ii) {
	      ii = (tmp3.lo?1:0);
	      if (!ii) {
		MulS32_64 (&tmp1, ccb->ccb_HDX, ccb->ccb_HDY+(ccb->ccb_VDY<<4)+ccb->ccb_HDDY);
		MulS32_64 (&tmp2, ccb->ccb_HDX+(ccb->ccb_VDX<<4)+ccb->ccb_HDDX, ccb->ccb_HDY);
		Sub64 (&tmp3, &tmp1, &tmp2);
		ii = tmp3.hi;
		if (!ii) {
		  ii = (tmp3.lo?1:0);
		}
	      }
	    }
	  }
	}
	if ( ((ii<0)&&(ccb->ccb_Flags&CCB_ACCW)) || ((ii>0)&&(ccb->ccb_Flags&CCB_ACW)) || !ii) {
	  DrawCels (bitmapitem, ccb);
	}
      }
    }
  }

#if 0
  if (textFlag) {
    SetPort (textwindow);
    EraseRect (&textwinrect);
    ForeColor (blackColor);

    sprintf (outstr, "Camera x, y, z = %Lg, %Lg, %Lg,   Zoom = %Lg",
	     mtoe(Camera.location.x), mtoe(Camera.location.y), mtoe(Camera.location.z),
	     mtoe(zoom));
    MoveTo (10, 10);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "tx, ty, tz = %d, %d, %d", tx, ty, tz);
    MoveTo (10, 25);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "Object = ");
    MoveTo (10, 55);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "%Lg, %Lg, %Lg", mtoe(sourceobject->orientation.xx),
	     mtoe(sourceobject->orientation.xy), mtoe(sourceobject->orientation.xz));
    MoveTo (65, 40);
    DrawText (outstr, 0, strlen(outstr));
    sprintf (outstr, "%Lg, %Lg, %Lg", mtoe(sourceobject->orientation.yx),
	     mtoe(sourceobject->orientation.yy), mtoe(sourceobject->orientation.yz));
    MoveTo (65, 55);
    DrawText (outstr, 0, strlen(outstr));
    sprintf (outstr, "%Lg, %Lg, %Lg", mtoe(sourceobject->orientation.zx),
	     mtoe(sourceobject->orientation.zy), mtoe(sourceobject->orientation.zz));
    MoveTo (65, 70);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "Delta x, y, z = %Lg, %Lg, %Lg", mtoe(tempobject.location.x),
	     mtoe(tempobject.location.y), mtoe(tempobject.location.z));
    MoveTo (10, 85);
    DrawText (outstr, 0, strlen(outstr));

    sprintf (outstr, "step, count = %ld, %ld", turnstep, turncount);
    MoveTo (10, 100);
    DrawText (outstr, 0, strlen(outstr));
  }
#endif
}


void
normalize (mat33f16 dest, mat33f16 src)
{
  frac16 x;

  x = MulSF16((src)[0][0],(src)[0][0]) + MulSF16((src)[0][1],(src)[0][1])
    + MulSF16((src)[0][2],(src)[0][2]);
  x = RecipUF16(SqrtF16(x));
  (dest)[0][0] = MulSF16(x,(src)[0][0]);
  (dest)[0][1] = MulSF16(x,(src)[0][1]);
  (dest)[0][2] = MulSF16(x,(src)[0][2]);
  x = MulSF16((src)[1][0],(src)[1][0]) + MulSF16((src)[1][1],(src)[1][1])
    + MulSF16((src)[1][2],(src)[1][2]);
  x = RecipUF16(SqrtF16(x));
  (dest)[1][0] = MulSF16(x,(src)[1][0]);
  (dest)[1][1] = MulSF16(x,(src)[1][1]);
  (dest)[1][2] = MulSF16(x,(src)[1][2]);
  x = MulSF16((src)[2][0],(src)[2][0]) + MulSF16((src)[2][1],(src)[2][1])
    + MulSF16((src)[2][2],(src)[2][2]);
  x = RecipUF16(SqrtF16(x));
  (dest)[2][0] = MulSF16(x,(src)[2][0]);
  (dest)[2][1] = MulSF16(x,(src)[2][1]);
  (dest)[2][2] = MulSF16(x,(src)[2][2]);
}
