/***************************************************************\
*								*
* Opera demo program						*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  28-Jun-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company                     *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/

#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) { printf x ; }

#include "types.h"

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
#include "stdlib.h"
#include "stdio.h"
#include "operamath.h"
#ifndef GFILE
#define GFILE "graphics.h"
#endif
#include GFILE
#include "event.h"

#include "loadfile.h"
#include "gettime.h"

GrafCon gc;

#define NBUFFERS 3
#define NUMSPRITES 1

Item ScreenItems[NBUFFERS], ScreenGroupItem, BitmapItems[NBUFFERS];
Bitmap *Bitmaps[NBUFFERS];

CCB *ccbs[NUMSPRITES];
uint32 *pluts[NUMSPRITES];
uint32 *cels[NUMSPRITES];

TagArg ScreenTags[] = {
  CSG_TAG_DISPLAYHEIGHT,(void*)240,
  CSG_TAG_SCREENCOUNT,	(void*)NBUFFERS,
  CSG_TAG_SCREENHEIGHT,	(void*)240,
  CSG_TAG_DONE,		(void*)0,
};

uint32 savebuffer, currentbuffer;

ControlPadEventData cped;


uint32 ScreenPages;


#define NUMVECS 1000
vec3f16 *v1, *v2, *v3;
mat33f16 mat;
vec4f16 *v41, *v42, *v43;
mat44f16 mat4;
int32 numvecs;

Item vramioreq, vblioreq;


uint32 timein[2], timeout[2], overhead[2];

void ClearScreen (void);
void terminate (int32 i);
void usage (void);

void DrawScreen (void);

void adjusttime (void);

void sMulManyVec3Mat33_F16 (vec3f16 *dest, vec3f16 *src, mat33f16 mat, int32 count);



int
main (int argc, char **argv)
{
  int32 i;
  uint32 j, c1, c2;
  uint32 joyold, joynew, joyedge;
  char outstr[80];
  ubyte *x;
  uint32 softtime=0, hardtime=0, hardtime2=0, mdtime=0, it;
  int32 pagesize;

  DBUG (("Matrix hardware demo program\n"));

  numvecs = NUMVECS;
  if (argc>1) {
    numvecs = strtoul (argv[1], 0, 0);
  }

  DBUG (("Running test with %ld vectors\n", numvecs));

  FULLDBUG (("Locate Operamath folio\n"));
  if ((i=OpenMathFolio()) < 0) {
    DBUG (("Error - Unable to open Operamath folio\n"));
    terminate (i);
  }

  FULLDBUG (("Open graphics folio\n"));
  if ((i=OpenGraphicsFolio()) < 0) {
    DBUG (("Error - unable to open graphics folio\n"));
    PrintfSysErr (i);
    terminate (i);
  }

  if ((vramioreq=GetVRAMIOReq()) < 0) {
    DBUG (("Error getting SPORT IOReq\n"));
    terminate (vramioreq);
  }

  if ((vblioreq=GetVBLIOReq()) < 0) {
    DBUG (("Error getting VBL timer IOReq\n"));
    terminate (vblioreq);
  }

#if 1
  FULLDBUG (("Connect to event broker\n"));
  if ((i=InitEventUtility(1,0,LC_ISFOCUSED)) < 0) {
    DBUG (("Error - unable to connect to event broker\n"));
    terminate (i);
  }
#endif

  FULLDBUG (("Create screen group\n"));
  ScreenGroupItem = CreateScreenGroup (ScreenItems, ScreenTags);
  if (ScreenGroupItem<0) {
    DBUG (("Error: CreateScreenGroup() == %lx\n", ScreenGroupItem));
    terminate (ScreenGroupItem);
  }
  FULLDBUG (("Add screen group\n"));
  AddScreenGroup (ScreenGroupItem, NULL);

  for (i=0; i<NBUFFERS; i++) {
    Screen *screen;
    screen = (Screen*)LookupItem (ScreenItems[i]);
    if (screen==0) {
      DBUG (("Error: could not locate screen\n"));
      terminate (1);
    }
    BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
    Bitmaps[i] = screen->scr_TempBitmap;
    FULLDBUG (("Screen[%ld] buffer @ %08lx\n", i, (uint32)Bitmaps[i]->bm_Buffer));
  }

  pagesize = GetPageSize (MEMTYPE_VRAM);
  ScreenPages = (Bitmaps[0]->bm_Width*Bitmaps[0]->bm_Height*2+pagesize-1) / pagesize;

  if (openmacdevice() < 0) {
    terminate (1);
  }

  opentimer ();

  savebuffer = 2;
  currentbuffer = 0;

  DisplayScreen (ScreenItems[savebuffer], ScreenItems[savebuffer]);

  FULLDBUG (("Clear screen\n"));
  SetVRAMPages (vramioreq, Bitmaps[savebuffer]->bm_Buffer,
		MakeRGB15Pair(0,0,0), ScreenPages, 0xffffffff);

  FULLDBUG (("Load in background image\n"));
  x = Bitmaps[1-currentbuffer]->bm_Buffer;
  if (!loadfile ("opera.l.b", x, 153600, 0)) {
    terminate (1);
  }
  for (i=0; i<240; i+=2) {
    for (j=0; j<320; j++) {
      c1 = (uint32)(*x++)<<8;
      c1 += *x++;
      c2 = (uint32)(*x++)<<8;
      c2 += *x++;
      SetFGPen (&gc, c2);
      WritePixel (BitmapItems[savebuffer], &gc, j, i);
      SetFGPen (&gc, c1);
      WritePixel (BitmapItems[savebuffer], &gc, j, i+1);
    }
  }

  overhead[0] = overhead[1] = 0;
  for (i=0; i<1000; i++) {
    gettime (timein);
    gettime (timeout);
    adjusttime ();
    j += timeout[1];
  }
  overhead[1] = j/1000;
  FULLDBUG (("Average overhead = %ld\n", overhead[1]));

  v1 = (vec3f16 *)malloc (numvecs*sizeof(*v1));
  v2 = (vec3f16 *)malloc (numvecs*sizeof(*v2));
  v3 = (vec3f16 *)malloc (numvecs*sizeof(*v3));
  v41 = (vec4f16 *)malloc (numvecs*sizeof(*v41));
  v42 = (vec4f16 *)malloc (numvecs*sizeof(*v42));
  v43 = (vec4f16 *)malloc (numvecs*sizeof(*v43));
  if (!(v1&&v2&&v3&&v41&&v42&&v43)) {
    DBUG (("Unable to allocate memory for vectors\n"));
    terminate (1);
  }

  for (i=0; i<numvecs; i++) {
    v1[i][0] = ((int32)urand())>>3;
    v1[i][1] = ((int32)urand())>>3;
    v1[i][2] = ((int32)urand())>>3;
    v41[i][0] = urand();
    v41[i][1] = urand();
    v41[i][2] = urand();
    v41[i][3] = urand();
  }
  mat[0][0] = ((int32)urand())>>16;
  mat[0][1] = ((int32)urand())>>16;
  mat[0][2] = ((int32)urand())>>16;
  mat[1][0] = ((int32)urand())>>16;
  mat[1][1] = ((int32)urand())>>16;
  mat[1][2] = ((int32)urand())>>16;
  mat[2][0] = ((int32)urand())>>16;
  mat[2][1] = ((int32)urand())>>16;
  mat[2][2] = ((int32)urand())>>16;

  mat4[0][0] = urand();
  mat4[0][1] = urand();
  mat4[0][2] = urand();
  mat4[0][3] = urand();
  mat4[1][0] = urand();
  mat4[1][1] = urand();
  mat4[1][2] = urand();
  mat4[1][3] = urand();
  mat4[2][0] = urand();
  mat4[2][1] = urand();
  mat4[2][2] = urand();
  mat4[2][3] = urand();
  mat4[3][0] = urand();
  mat4[3][1] = urand();
  mat4[3][2] = urand();
  mat4[3][3] = urand();

  joyold = 0;
  for (it=1;;it++) {
    DrawScreen();
#if 1
    GetControlPad (1,0,&cped);
    joynew = cped.cped_ButtonBits;
    joyedge = joynew&~joyold;
#endif

    gettime (timein);
    sMulManyVec3Mat33_F16 (v2, v1, mat, numvecs);
    gettime (timeout);
    adjusttime ();
    MoveTo (&gc, 40, 92);
    sprintf (outstr, "3x3 software xform/sec - usec/call");
    DrawText8 (&gc, BitmapItems[currentbuffer], outstr);
    MoveTo (&gc, 40, 100);
    softtime += timeout[1];
    sprintf (outstr, "%ld (%ld) - %ld (%ld)", (numvecs*1000000)/timeout[1],
	     (numvecs*1000000)/(softtime/it), timeout[1], softtime/it);
    DrawText8 (&gc, BitmapItems[currentbuffer], outstr);

    {
      mmv3m33d s;
      s.dest = v3;
      s.src = v1;
      s.mat = &mat;
      s.count = numvecs;
      s.n = Convert32_F16 (1);
      gettime (timein);
      MulManyVec3Mat33DivZ_F16 (&s);
      gettime (timeout);
      adjusttime ();
      MoveTo (&gc, 40, 140);
      sprintf (outstr, "M+D hardware xform/sec - usec/call");
      DrawText8 (&gc, BitmapItems[currentbuffer], outstr);
      MoveTo (&gc, 40, 148);
      mdtime += timeout[1];
      sprintf (outstr, "%ld (%ld) - %ld (%ld)", (numvecs*1000000)/timeout[1],
	       (numvecs*1000000)/(mdtime/it), timeout[1], mdtime/it);
      DrawText8 (&gc, BitmapItems[currentbuffer], outstr);
    }

    gettime (timein);
    MulManyVec3Mat33_F16 (v3, v1, mat, numvecs);
    gettime (timeout);
    adjusttime ();
    MoveTo (&gc, 40, 116);
    sprintf (outstr, "3x3 hardware xform/sec - usec/call");
    DrawText8 (&gc, BitmapItems[currentbuffer], outstr);
    MoveTo (&gc, 40, 124);
    hardtime += timeout[1];
    sprintf (outstr, "%ld (%ld) - %ld (%ld)", (numvecs*1000000)/timeout[1],
	     (numvecs*1000000)/(hardtime/it), timeout[1], hardtime/it);
    DrawText8 (&gc, BitmapItems[currentbuffer], outstr);

    gettime (timein);
    MulManyVec4Mat44_F16 (v43, v41, mat4, numvecs);
    gettime (timeout);
    adjusttime ();
    MoveTo (&gc, 40, 164);
    sprintf (outstr, "4x4 hardware xform/sec - usec/call");
    DrawText8 (&gc, BitmapItems[currentbuffer], outstr);
    MoveTo (&gc, 40, 172);
    hardtime2 += timeout[1];
    sprintf (outstr, "%ld (%ld) - %ld (%ld)", (numvecs*1000000)/timeout[1],
	     (numvecs*1000000)/(hardtime2/it), timeout[1], hardtime2/it);
    DrawText8 (&gc, BitmapItems[currentbuffer], outstr);

#if 1
    for (i=0; i<numvecs; i++) {
      if ((v3[i][0]-v2[i][0]>2) || (v3[i][1]-v2[i][1]>2) || (v3[i][2]-v2[i][2]>2)) {
	DBUG (("Unmatched values\n"));
	DBUG (("%lx %lx %lx\n", v2[i][0], v2[i][1], v2[i][2]));
	DBUG (("%lx %lx %lx\n", v3[i][0], v3[i][1], v3[i][2]));
	terminate (1);
      }
    }
#endif

    if (joyedge&ControlStart) {
      terminate (0);
    }

    joyold = joynew;
  }
}


void
adjusttime (void)
{
  timeout[0] -= timein[0]+overhead[0];
  timeout[1] -= timein[1]+overhead[1];
  while ((int32)timeout[1]<0) {
    timeout[1]+=1000000;
    timeout[0]--;
  }
}


void
DrawScreen (void)
{

  DisplayScreen (ScreenItems[currentbuffer], ScreenItems[currentbuffer]);
  currentbuffer = 1-currentbuffer;

  CopyVRAMPages (vramioreq, Bitmaps[currentbuffer]->bm_Buffer,
		 Bitmaps[savebuffer]->bm_Buffer, ScreenPages, 0xffffffff);

}


void
exitfade (int32 frames)
{
  int32 j, k, l, m;

  for (j=frames-1; j>=0; j--) {
    WaitVBL (vblioreq, 1);
    k = j*255/(frames-1);
    for (l=0; l<32; l++) {
      m = k*l/31;
      SetScreenColor (ScreenItems[1-currentbuffer], MakeCLUTColorEntry(l,m,m,m));
    }
    SetScreenColor (ScreenItems[1-currentbuffer], MakeCLUTColorEntry(32,0,0,0));
  }
}


void
terminate (int32 i)
{
  if (!i) {
    exitfade (32);
  }
  if (i<0) {
    PrintfSysErr(i);
  }
  KillEventUtility();
  exit ((int)i);
}


