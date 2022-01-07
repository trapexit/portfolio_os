/***************************************************************\
*								*
* Opera demo program						*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  4-Aug-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company, Inc.               *
*								*
* This program is proprietary and confidential			*
*								*
\***************************************************************/

#define DBUG(x)	{ printf x ; }
#define FULLDBUG(x) /* { printf x ; } */

#define SOLIDFLAG (KernelBase->kb_CpuFlags&KB_SHERRIE)
/* #define SOLIDFLAG TRUE */

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
/* #include "rom.h" */

#include "strings.h"
#include "stdlib.h"
#include "stdio.h"

#ifndef GFILE
#define GFILE "graphics.h"
#endif
#include GFILE
#include "loadfile.h"
#include "ThreeD.h"
#include "event.h"
#include "operror.h"


#define MINVBLDELAY	2


#define SPRWIDTH	64
#define SPRHEIGHT	64

#define EYEOFFSET	50
#define EYECAMOFFSET	5
#define SPRINGZ		15

Point3d CubeCorners[] = {
  { Convert32_F16(-10), Convert32_F16(-10), Convert32_F16(-10), },
  { Convert32_F16(-10), Convert32_F16(10),  Convert32_F16(-10), },
  { Convert32_F16(10),  Convert32_F16(10),  Convert32_F16(-10), },
  { Convert32_F16(10),  Convert32_F16(-10), Convert32_F16(-10), },
  { Convert32_F16(-10), Convert32_F16(-10), Convert32_F16(10),  },
  { Convert32_F16(-10), Convert32_F16(10),  Convert32_F16(10),  },
  { Convert32_F16(10),  Convert32_F16(10),  Convert32_F16(10),  },
  { Convert32_F16(10),  Convert32_F16(-10), Convert32_F16(10),  },
};

Face3d CubeFaces[] = {
  { { 1, 2, 3, 0, }, NULL, 0, },
  { { 2, 6, 7, 3, }, NULL, 0, },
  { { 6, 5, 4, 7, }, NULL, 0, },
  { { 5, 1, 0, 4, }, NULL, 0, },
  { { 5, 6, 2, 1, }, NULL, 0, },
  { { 0, 3, 7, 4, }, NULL, 0, },
};


Object3d Cube = {
  sizeof(CubeCorners)/sizeof(*CubeCorners),	/* number of corners */
  sizeof(CubeFaces)/sizeof(*CubeFaces),		/* number of faces */
  { Convert32_F16(0), Convert32_F16(0), Convert32_F16(0), },	/* location of cube */
  { Convert32_F16(1), Convert32_F16(0), Convert32_F16(0),
    Convert32_F16(0), Convert32_F16(1), Convert32_F16(0),
    Convert32_F16(0), Convert32_F16(0), Convert32_F16(1), },	/* orientation of cube */
  &CubeCorners,	/* pointer to corner list */
  &CubeFaces,	/* pointer to face list */
  NULL,		/* scratch pointer */
  NULL,		/* scratch pointer */
  { 0, 0, 0, },	/* scratch vector */
  { 0, 0, 0, },	/* scratch vector */
  0,		/* flags */
  "Cube",	/* name */
};

Object3d Cube1 = {
  sizeof(CubeCorners)/sizeof(*CubeCorners),	/* number of corners */
  sizeof(CubeFaces)/sizeof(*CubeFaces),		/* number of faces */
  { Convert32_F16(35), Convert32_F16(0), Convert32_F16(0), },	/* location of cube */
  { Convert32_F16(1), Convert32_F16(0), Convert32_F16(0),
    Convert32_F16(0), Convert32_F16(1), Convert32_F16(0),
    Convert32_F16(0), Convert32_F16(0), Convert32_F16(1), },	/* orientation of cube */
  &CubeCorners,	/* pointer to corner list */
  &CubeFaces,	/* pointer to face list */
  NULL,		/* scratch pointer */
  NULL,		/* scratch pointer */
  { 0, 0, 0, },	/* scratch vector */
  { 0, 0, 0, },	/* scratch vector */
  0,		/* flags */
  "Cube1",	/* name */
};

Object3d Cube2 = {
  sizeof(CubeCorners)/sizeof(*CubeCorners),	/* number of corners */
  sizeof(CubeFaces)/sizeof(*CubeFaces),		/* number of faces */
  { Convert32_F16(-35), Convert32_F16(0), Convert32_F16(0), },	/* location of cube */
  { Convert32_F16(1), Convert32_F16(0), Convert32_F16(0),
    Convert32_F16(0), Convert32_F16(1), Convert32_F16(0),
    Convert32_F16(0), Convert32_F16(0), Convert32_F16(1), },	/* orientation of cube */
  &CubeCorners,	/* pointer to corner list */
  &CubeFaces,	/* pointer to face list */
  NULL,		/* scratch pointer */
  NULL,		/* scratch pointer */
  { 0, 0, 0, },	/* scratch vector */
  { 0, 0, 0, },	/* scratch vector */
  0,		/* flags */
  "Cube2",	/* name */
};

extern Object3d Camera;


Item ScreenItems[3], ScreenGroupItem, BitmapItems[3];
Bitmap *Bitmaps[3];

TagArg ScreenTags[] = {
  CSG_TAG_DISPLAYHEIGHT,(void*)240,
  CSG_TAG_SCREENCOUNT,	(void*)3,
  CSG_TAG_SCREENHEIGHT,	(void*)240,
  CSG_TAG_DONE,		(void*)0,
};

CCB *ccb;
uint32 *cel;
uint32 *plut;
uint32 ScreenPages;

ControlPadEventData cped;

GrafCon gc;

ubyte *savebuffer, *backdrop;

int runflag;



void DrawScreen (void);
void terminate (int32 i);
void cleartensions (void);
void dotensions (void);
void dofriction (void);

uint32 sqrt (uint32 num);


Item vramioreq;
Item vblioreq;

uint32 ItemOpened (Item, Item);


int
main (int argc, char **argv)
{
  uint32 joyold, joynew, joyedge;
  int32 dtx, dty, dtz;
  int32 tx, ty, tz;
  int32 i, j;
  int32 pagesize;

  if ((i=OpenMathFolio()) < 0) {
    DBUG (("Error opening math folio\n"));
    terminate (i);
  }

  DBUG (("ItemOpened == %lx\n", (uint32)ItemOpened)); 
  i = ItemOpened (KernelBase->kb_CurrentTask->t.n_Item, 2);
  DBUG (("ItemOpened -> %lx\n", i));

  if ((i=OpenGraphicsFolio()) < 0) {
    DBUG (("Error opening graphics folio\n"));
    terminate (i);
  }

  i = ItemOpened (KernelBase->kb_CurrentTask->t.n_Item, 2);
  DBUG (("ItemOpened -> %lx\n", i));

  QueryGraphics (QUERYGRAF_TAG_FIELDTIME, &i);
  DBUG (("VBLTime -> %ld\n", i));
  QueryGraphics (QUERYGRAF_TAG_FIELDFREQ, &i);
  DBUG (("VBLFreq -> %ld\n", i));

  if ((vramioreq=GetVRAMIOReq()) < 0) {
    DBUG (("Error getting SPORT IOReq\n"));
    terminate (vramioreq);
  }

  if ((vblioreq=GetVBLIOReq()) < 0) {
    DBUG (("Error getting VBL timer IOReq\n"));
    terminate (vblioreq);
  }

  FULLDBUG (("Connect to event broker\n"));
  if ((i=InitEventUtility(1,0,LC_ISFOCUSED)) < 0) {
    DBUG (("Error - unable to connect to event broker\n"));
    terminate (i);
  }

  if ((i=openmacdevice())<0) {
    DBUG (("Error opening Mac device\n"));
    terminate (i);
  }

  ccb = (CCB *)ALLOCMEM (sizeof(CCB), MEMTYPE_CEL|MEMTYPE_DMA);
  if (!ccb) {
    DBUG (("Unable to allocate CCB\n"));
  }

  ScreenGroupItem = CreateScreenGroup (ScreenItems, ScreenTags);
  if (ScreenGroupItem<0) {
    DBUG (("Error: CreateScreenGroup() == %lx\n", ScreenGroupItem));
    terminate (ScreenGroupItem);
  }
  AddScreenGroup (ScreenGroupItem, NULL);

  for (i=0; i<3; i++) {
    Screen *screen;
    screen = (Screen*)LookupItem (ScreenItems[i]);
    if (screen==0) {
      DBUG (("Error: could not locate screen\n"));
      terminate (1);
    }
    BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
    Bitmaps[i] = screen->scr_TempBitmap;
    FULLDBUG (("BitmapItems[%ld] = %ld\n", i, BitmapItems[i]));
  }

  pagesize = GetPageSize(MEMTYPE_VRAM);
  ScreenPages = (Bitmaps[0]->bm_Width*Bitmaps[0]->bm_Height*2+pagesize-1) / pagesize;

  for (i=0; i<Cube.numfaces; i++) {
    CubeFaces[i].ccb = ccb;
  }

  zoom = Convert32_F16(300);
  wincx = Bitmaps[0]->bm_ClipWidth/2;
  wincy = Bitmaps[0]->bm_ClipHeight/2;
  dtx = dty = dtz = tx = ty = tz = 0;

#if 0
  savebuffer = (ubyte *)ALLOCMEM ((int)(ScreenPages*pagesize),
				  MEMTYPE_STARTPAGE|MEMTYPE_VRAM|MEMTYPE_CEL);
#else
  savebuffer = (ubyte *)Bitmaps[0]->bm_Buffer;
#endif
  if (!savebuffer) {
    DBUG (("Unable to allocate savebuffer\n"));
    terminate (1);
  }

  backdrop = Bitmaps[2]->bm_Buffer;

  if (!(loadfile ("Opera.l.b", savebuffer, ScreenPages*pagesize, 0))) {
    DBUG (("Error loading background picture\n"));
  }

  {
    char *x;
    Color c1, c2;

    DisplayScreen (ScreenItems[2], ScreenItems[2]);
    x = (char *)savebuffer;
    for (i=0; i<240; i+=2) {
      for (j=0; j<320; j++) {
	c1 = (uint32)(*x++)<<8;
	c1 += *x++;
	c2 = (uint32)(*x++)<<8;
	c2 += *x++;
	SetFGPen (&gc, c2);
	WritePixel (BitmapItems[2], &gc, j, i);
	SetFGPen (&gc, c1);
	WritePixel (BitmapItems[2], &gc, j, i+1);
      }
    }
  }


  {
    cel = (uint32 *)loadfile ("sw4a.spr", 0, 0, MEMTYPE_VRAM|MEMTYPE_CEL);
    if (!cel) {
      DBUG (("Unable to load cel file\n"));
      terminate (1);
    }
    DBUG (("cel @ $%lx\n", (uint32)cel));
    ccb->ccb_Flags = CCB_LAST|CCB_NPABS|CCB_SPABS|CCB_PPABS|CCB_LDSIZE|CCB_LDPRS|CCB_LDPPMP
      |CCB_YOXY|CCB_ACW|PMODE_ZERO|CCB_BGND;
    if (!(cel[0]&PRE0_LINEAR)) {
      plut = (uint32 *)loadfile ("sw4a.plut", 0, 0, MEMTYPE_VRAM|MEMTYPE_DMA);
      if (!plut) {
	DBUG (("Unable to load plut\n"));
      }
      ccb->ccb_Flags |= CCB_LDPLUT;
    }
    if (!(cel[0]&PRE0_LITERAL)) {
      ccb->ccb_Flags |= CCB_PACKED;
    }
    if ((cel[0]&PRE0_LITERAL) && (cel[1]&PRE1_LRFORM)) { 
      ccb->ccb_Flags |= CCB_LCE|CCB_ACE;
    }
    ccb->ccb_NextPtr = ccb;
    ccb->ccb_SourcePtr = (CelData *)cel;
    ccb->ccb_PLUTPtr = plut;
    ccb->ccb_PIXC = 0x1f001f00;
    ccb->ccb_Width = SPRWIDTH;
    ccb->ccb_Height = SPRHEIGHT;
  }

  joyold = 0;
  DrawScreen();
  for (;;) {
    int cubeflag;

    GetControlPad (1,0,&cped);
    joynew = cped.cped_ButtonBits;
    joyedge = joynew&~joyold;
    cubeflag = false;
    if (joyedge&ControlStart) {
      terminate (0);
    }
    if (joynew&ControlLeft) {
      dty -= 0x04000;
      /* rotatematrix (&Cube.orientation, &Cube.orientation, 0, -1, 0); */
      cubeflag = true;
    }
    if (joynew&ControlRight) {
      dty += 0x04000;
      /* rotatematrix (&Cube.orientation, &Cube.orientation, 0, 1, 0); */
      cubeflag = true;
    }
    if (joynew&ControlUp) {
      dtx -= 0x04000;
      /* rotatematrix (&Cube.orientation, &Cube.orientation, -1, 0, 0); */
      cubeflag = true;
    }
    if (joynew&ControlDown) {
      dtx += 0x04000;
      /* rotatematrix (&Cube.orientation, &Cube.orientation, 1, 0, 0); */
      cubeflag = true;
    }
    if (joynew&ControlA) {
      dtz -= 0x04000;
    }
    if (joynew&ControlB) {
      dtz += 0x04000;
    }
#if 1
    tx = (tx+dtx);
    ty = (ty+dty);
    tz = (tz+dtz);
    if (joyedge&ControlC) {
      tx = 0; ty = 0; tz = 0;
      dtx = 0; dty = 0; dtz = 0;
    }
    makerotmatrix (Cube.orientation, tx, ty, tz);
#else
    rotatematrix (Cube.orientation, Cube.orientation, dtx, dty, dtz);
    normalize (Cube.orientation, Cube.orientation);
    if (joyedge&ControlC) {
      dtx = 0; dty = 0; dtz = 0;
      makerotmatrix (Cube.orientation, dtx, dty, dtz);
    }
#endif
    rotatematrix (Cube1.orientation, Cube1.orientation, 0, 0x04000, 0);
    normalize (Cube1.orientation, Cube1.orientation);
    rotatematrix (Cube2.orientation, Cube2.orientation, 0x04000, 0, 0);
    normalize (Cube2.orientation, Cube2.orientation);
    DrawScreen ();
    joyold = joynew;
  }
}


void
DrawScreen (void)
{
  static int32 b1=0;

  DisplayScreen (ScreenItems[b1], ScreenItems[b1]);
  b1 = 1-b1;
  CopyVRAMPages (vramioreq, Bitmaps[b1]->bm_Buffer, backdrop, ScreenPages, 0xffffffff);
  drawsolid (BitmapItems[b1], &Cube, 0);
  drawsolid (BitmapItems[b1], &Cube1, 0);
  drawsolid (BitmapItems[b1], &Cube2, 0);
  WaitIO (vblioreq);
  WaitVBLDefer (vblioreq, MINVBLDELAY);
}


void
exitfade (int32 frames)
{
  int32 j, k, l, m;
  Rect r;

  DisplayScreen (ScreenItems[0], ScreenItems[0]);

  r.rect_XLeft = 0;
  r.rect_XRight = Bitmaps[0]->bm_ClipWidth;
  r.rect_YTop = 0;
  r.rect_YBottom = Bitmaps[0]->bm_ClipHeight;

  for (j=frames-1; j>=0; j--) {
    WaitVBL (vblioreq, 1);
    k = j*255/(frames-1);
    for (l=0; l<32; l++) {
      m = k*l/31;
      SetScreenColor (ScreenItems[0], MakeCLUTColorEntry(l,m,m,m));
    }
    SetScreenColor (ScreenItems[0], MakeCLUTColorEntry(32,0,0,0));
  }

  SetFGPen (&gc, MakeRGB15(0,0,0));
  FillRect (BitmapItems[0], &gc, &r);
}


void
terminate (int32 i)
{
  if (i) {
    if (i<0) {
      PrintfSysErr (i);
    }
  } else {
    exitfade (32);
  }
  exit ((int)i);
}


uint32
sqrt (uint32 num)
{
  uint32 numh, sqrt, tmp;
  int i;

  numh = 0;
  sqrt = 0;
  for (i=0; i<16; i++) {
    numh = (numh<<2) + (num>>30);
    num <<= 2;
    sqrt <<= 1;
    tmp = sqrt+sqrt+1;
    if (numh >= tmp) {
      numh -= tmp;
      sqrt++;
    }
  }
  return sqrt;
}


#if 0
void
cleartensions (void)
{
  int i;

  for (i=0; i<NUMPTS; i++) {
    ddob[i].x = 0;
    ddob[i].y = 0;
  }
}


void
dotensions (void)
{
  int i;
  int32 j, k, l, m;
  int32 tdx, tdy;
  int32 dist;

  i = 0;
  while (i<NUMCONNECTS*4) {
    j = connects[i++];
    k = connects[i++];
    l = connects[i++];
    m = connects[i++];

    tdx = ob[j].x - ob[k].x;
    tdy = ob[j].y - ob[k].y;
    dist = sqrt((tdx/UNIT)*(tdx/UNIT)+(tdy/UNIT)*(tdy/UNIT));
    if (dist) {
      tdx /= dist;
      tdy /= dist;
    } else {
      tdx = UNIT;
      tdy = 0;
    }
    tdx = (tdx*(l-dist))/m;
    tdy = (tdy*(l-dist))/m;
    ddob[j].x += tdx;
    ddob[j].y += tdy;
    ddob[k].x -= tdx;
    ddob[k].y -= tdy;
  }
}


void
dofriction (void)
{
  int i;

  for (i=0; i<NUMPTS; i++) {
    if (dob[i].x<MINFRICTION && dob[i].x>-MINFRICTION) dob[i].x = 0;
    dob[i].x -= dob[i].x/INVFRICTION+(MINFRICTION*((dob[i].x>0)-(dob[i].x<0)));
    if (dob[i].y<MINFRICTION && dob[i].y>-MINFRICTION) dob[i].y = 0;
    dob[i].y -= dob[i].y/INVFRICTION+(MINFRICTION*((dob[i].y>0)-(dob[i].y<0)));
  }
}
#endif
