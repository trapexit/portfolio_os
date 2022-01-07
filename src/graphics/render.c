/* $Id: render.c,v 1.8 1994/09/22 19:55:57 ewhac Exp $ */

/* *************************************************************************
 *
 * Rendering primitives for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 921010 -RJ Mical        Created this file!
 *
 * ********************************************************************** */


/***************************************************************\
* Header files                                                 *
\***************************************************************/


#define INTERNALDEBUG
#define NOERASE

#include "types.h"

/*???#include "stdlib.h"*/

#include "debug.h"
#include "item.h"
#include "nodes.h"
#include "interrupts.h"
#include "kernel.h"
#define SUPER
#include "mem.h"
#undef SUPER
#include "list.h"
#include "task.h"
#include "folio.h"
#include "kernelnodes.h"
#include "super.h"

#include "intgraf.h"

#include "stdarg.h"
#include "strings.h"



void
MoveTo (GrafCon *gc, Coord x, Coord y)
{

	/* This routine doesn't need to be in supervisor mode */
	gc->gc_PenX = x;
	gc->gc_PenY = y;
}


void
SetFGPen (GrafCon *gc, Color c)
{
	gc->gc_FGPen = c;
}


void
SetBGPen (GrafCon *gc, Color c)
{
	gc->gc_BGPen = c;
}


int32
WritePixel( Item bitmapItem, GrafCon *gc, Coord x, Coord y )
/*
 * Set the current pen coordinates and write a pixel in the
 * foreground color to the current screen.
 */
{
  ubyte *i;
  Bitmap *bitmap;

/* This routine needs to be in supervisor mode and needs to do serious */
/* validity checking */

  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }
  if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,bitmapItem)<0) {
      return GRAFERR_NOTOWNER;
    }
  }

  if ( x < 0 || x >= (bitmap->bm_ClipWidth*PIXELSIZE)
      || y < 0 || y >= (bitmap->bm_ClipHeight*PIXELSIZE) ) {
    return GRAFERR_COORDRANGE;
  }

  gc->gc_PenX = x;
  gc->gc_PenY = y;

  x = x/PIXELSIZE;
  y = y/PIXELSIZE;

  i = bitmap->bm_Buffer + (((y>>1)*bitmap->bm_Width)<<2) + ((y&1)<<1) + (x<<2);
  *i++ = (ubyte)(gc->gc_FGPen>>8);
  *i = (ubyte)gc->gc_FGPen;

  return 0;
}


Color
ReadPixel( Item bitmapItem, GrafCon *gc, Coord x, Coord y )
/*
 * Read a pixel from the graphics context and return its value.
 * A read outside the bitmap boundaries returns a value of < 0.
 */
{
  ubyte *i;
  Bitmap *bitmap;

/* This routine can be in user mode */

  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }

  if ( x < 0 || x >= (bitmap->bm_ClipWidth*PIXELSIZE)
      || y < 0 || y >= (bitmap->bm_ClipHeight*PIXELSIZE) ) {
    return GRAFERR_COORDRANGE;
  }

  x = x/PIXELSIZE;
  y = y/PIXELSIZE;

  i = (ubyte *)(bitmap->bm_Buffer + (((y>>1)*bitmap->bm_Width)<<2) + ((y&1)<<1) + (x<<2));
  return ((Color)i[0]<<8) + (Color)i[1];
}



#if 0
void
zVLine (GrafCon *gc, Coord x, Coord y1, Coord y2)
/*
 * Draw a vertical line from y1 to y2 on column x.
 * The bottommost pixel of the line is not drawn.
 * Clipping to bitmap coordinates is performed.
 */
{

/*??? This routine should be replaced with a sprite call */

/*???	if ( x < 0 || x >= (gc->gc_ClipWidth*PIXELSIZE) */
/*???			|| (y1 < 0 && y2 < 0)*/
/*???			|| (y1 > (gc->gc_ClipHeight*PIXELSIZE) */
/*???			&& y2 > (gc->gc_ClipHeight*PIXELSIZE)) ) */
/*???		return;*/
/*???*/
/*???	if (y1 > y2) { SWAP(y1,y2,Coord) }*/
/*???	if (y1 < 0) { y1 = 0; }*/
/*???	if (y2 < 0) { y2 = 0; }*/
/*???	if (y1 > (gc->gc_ClipHeight*PIXELSIZE)) */
/*???		{ y1 = (gc->gc_ClipHeight*PIXELSIZE); }*/
/*???	if (y2 > (gc->gc_ClipHeight*PIXELSIZE)) */
/*???		{ y2 = (gc->gc_ClipHeight*PIXELSIZE); }*/
/*???*/
/*???	while ( y1 < y2 ) */
/*???		{*/
/*???		WritePixel (gc, x, y1);*/
/*???		y1 += PIXELSIZE;*/
/*???		}*/
}
#endif



/*???	CCB ccb;*/
/*???int32 seenccb = 0;*/

int32
FillRect( Item bitmapItem, GrafCon *gc, Rect *rect )
/*
 * Fill the rectangle with the specified color.
 * Clipping to bitmap boundaries is performed.
 */
{
	ulong pixeldata[3];
	int32 left, right, top, bottom, i;
	CCB ccb;

/*???if ( seenccb == 0 )*/
/*???	{*/
/*???	Superkprintf("\n\nJAVIER:  ccb=$%lx\n\n", (unsigned long)(&ccb));*/
/*???	seenccb = 1;*/
/*???	}*/

	/* This routine needs to be in supervisor mode */

	pixeldata[0] = ((1-1) << PRE0_VCNT_SHIFT) | PRE0_LINEAR | PRE0_BPP_16;
/*???	pixeldata[1] = PRE1_LRFORM | ((1-1) << PRE1_TLHPCNT_SHIFT)*/
	pixeldata[1] = ((1-1) << PRE1_TLHPCNT_SHIFT)
			| (0 << PRE1_WOFFSET10_SHIFT);

	/* put color into pixeldata */
	pixeldata[2] = gc->gc_FGPen << 16;

	ccb.ccb_Flags = CCB_ACW | CCB_ACCW | CCB_LAST | CCB_SPABS
			| CCB_BGND | CCB_NOBLK
			| CCB_LDSIZE | CCB_LDPPMP | CCB_LDPRS | CCB_YOXY;
	ccb.ccb_SourcePtr = (CelData *)(&pixeldata);
	ccb.ccb_Width = 1;
	ccb.ccb_Height = 1;
	ccb.ccb_PIXC = 0x1f401f40;

	left = rect->rect_XLeft;
	right = rect->rect_XRight;
	top = rect->rect_YTop;
	bottom = rect->rect_YBottom;
	if ( left > right ) { i = left; left = right; right = i; }
	if ( top > bottom ) { i = top; top = bottom; bottom = i; }
	right++;
	bottom++;
/*???Superkprintf("left=%ld ", (unsigned long)(left));*/
/*???Superkprintf("right=%ld ", (unsigned long)(right));*/
/*???Superkprintf("top=%ld ", (unsigned long)(top));*/
/*???Superkprintf("bottom=%ld ", (unsigned long)(bottom));*/
/*???Superkprintf("\n");*/

	ccb.ccb_XPos = left << 16;
	ccb.ccb_YPos = top << 16;
	ccb.ccb_HDX = ((right << 16) - ccb.ccb_XPos) << 4;
	ccb.ccb_HDY = 0;
	ccb.ccb_VDX = 0;
	ccb.ccb_VDY = (bottom << 16) - ccb.ccb_YPos;
	ccb.ccb_HDDX = 0;
	ccb.ccb_HDDY = 0;

	return (SWIDrawCels (bitmapItem, &ccb));
}


#if 0
void
CopyRect (GrafCon *destgc, GrafCon *srcgc, Rect *r)
/*
 * Copy a rectangle from the source screen to the destination screen.
 * Use the routine
 * MoveTo to set the desired destination of the rectangle in the destination
 * screen.
 * Clipping to both the source and destination screen bounds will be performed.
 * The rightmost and bottommost lines will not be copied.
 * Vertical positions must be given as even multiples of two (currently).
 */
{
/*???	Coord xsrc, ysrc, xdest, ydest, width, height;*/
/*???	ulong info[4];*/

/*??? This routine should be replaced with a sprite call */

/*???	xsrc = r->rect_XLeft;*/
/*???	ysrc = r->rect_YTop&(-(1L<<PIXELSIZE));*/
/*???	width = r->rect_XRight-xsrc;*/
/*???	height = (r->rect_YBot&(-(1L<<PIXELSIZE)))-ysrc;*/
/*???	xdest = destgc->gc_PenX;*/
/*???	ydest = destgc->gc_PenY&(-(1L<<PIXELSIZE));*/
/*???*/
/*???	if (xsrc>srcgc->gc_ClipWidth || ysrc>srcgc->gc_ClipHeight */
/*???			|| xdest>destgc->gc_ClipWidth*/
/*???			|| ydest>destgc->gc_ClipHeight) */
/*???		return;*/
/*???*/
/*???	if (xsrc<0) */
/*???		{*/
/*???		width += xsrc;*/
/*???		xdest -= xsrc;*/
/*???		xsrc = 0;*/
/*???		}*/
/*???	if (xdest<0) */
/*???		{*/
/*???		width += xdest;*/
/*???		xsrc -= xdest;*/
/*???		xdest = 0;*/
/*???		}*/
/*???*/
/*???	if (ysrc<0) */
/*???		{*/
/*???		height += ysrc;*/
/*???		ydest -= ysrc;*/
/*???		ysrc = 0;*/
/*???		}*/
/*???	if (ydest<0) */
/*???		{*/
/*???		height += ydest;*/
/*???		ysrc -= ydest;*/
/*???		ydest = 0;*/
/*???		}*/
/*???*/
/*???	if (xsrc+width > srcgc->gc_ClipWidth) */
/*???		width -= xsrc+width - srcgc->gc_ClipWidth;*/
/*???	if (ysrc+height > srcgc->gc_ClipHeight) */
/*???		height -= ysrc+height - srcgc->gc_ClipHeight;*/
/*???	if (xdest+width > destgc->gc_ClipWidth) */
/*???		width -= xdest+width - destgc->gc_ClipWidth;*/
/*???	if (ydest+height > destgc->gc_ClipHeight) */
/*???		height -= ydest+height - destgc->gc_ClipHeight;*/
/*???*/
/*???	if (width<=0 || height<=0) return;*/
/*???*/
/*???	info[0] = width;*/
/*???	info[1] = srcgc->gc_BitMap.bm_Width-width;*/
/*???	info[2] = destgc->gc_BitMap.bm_Width-width;*/
/*???	info[3] = height/2;*/
/*???*/
/*???	blitrect( ((ulong *)destgc->gc_BitMap.bm_PixMap)*/
/*???			+ xdest + (ydest/2) * destgc->gc_BitMap.bm_Width,*/
/*???			((ulong *)srcgc->gc_BitMap.bm_PixMap)*/
/*???			+ xsrc + (ysrc/2) * srcgc->gc_BitMap.bm_Width, info );*/
/*???*/
}
#endif

#if 0
void
FillEllipse (GrafCon *gc, Rect *r)
/*
 * Fill the ellipse with the specified color.
 * Unlike the RectFill routine, the ellipse will extend to all four bounds of the rectangle.
 * Clipping to bitmap boundaries is performed.
 * Ellipses much larger than screen size may cause internal overflows in calculations, causing
 * unpredictable shapes to be output.
 */
{
/*???	Coord x1, x2, y1, y2;*/
/*???	Coord xc, yc, dx, dy, xoff, yoff, error, test1, test2;*/
/*???	Coord cx, cx2, cy, cy2;*/

/* This routine will probably need to be in supervisor mode because it */
/* wants to directly call supervisor mode routines */

/*???	x1 = r->rect_XLeft/PIXELSIZE;*/
/*???	x2 = r->rect_XRight/PIXELSIZE;*/
/*???	y1 = r->rect_YTop/PIXELSIZE;*/
/*???	y2 = r->rect_YBot/PIXELSIZE;*/
/*???*/
/*???	if (x1>x2) { SWAP(x1,x2,Coord) }*/
/*???	if (y1>y2) { SWAP(y1,y2,Coord) }*/
/*???*/
/*???	if ( x2 < 0 || x1 >= (gc->gc_ClipWidth*PIXELSIZE) */
/*???			|| y2 < 0 || y1 >= (gc->gc_ClipHeight*PIXELSIZE) )*/
/*???		return;*/
/*???*/
/*???	xc = x1 + x2;*/
/*???	yc = y1 + y2;*/
/*???	dx = x2 - x1;*/
/*???	dy = y2 - y1;*/
/*???*/
/*???	DEBUGELLIPSE (("x1, x2 = %d, %d\n", x1, x2));*/
/*???	DEBUGELLIPSE (("y1, y2 = %d, %d\n", y1, y2));*/
/*???	DEBUGELLIPSE (("xc,yc = %d, %d\n", xc, yc));*/
/*???	DEBUGELLIPSE (("dx,dy = %d, %d\n", dx, dy));*/
/*???*/
/*???	cx = (dx/2);*/
/*???	cx2 = cx*cx;*/
/*???	cy = (dy/2+1);*/
/*???	cy2 = cy*cy;*/
/*???*/
/*???	xoff = dx & 1;*/
/*???	test1 = cy * cx2 * 2;*/
/*???	test2 = cy2;*/
/*???	error = (test1+test2)/2;*/
/*???	DEBUGELLIPSE (("test1,error = %d,%d\n", test1, error));*/
/*???	for (yoff = dy; yoff>=0; yoff-=2*PIXELSIZE) */
/*???		{*/
/*???		while (error < test1) */
/*???			{*/
/*???			error += test2 + cy2;*/
/*???			test2 += cy2 * 2;*/
/*???			xoff += 2 * PIXELSIZE;*/
/*???			DEBUGELLIPSE (("test2,error,xoff = %d,%d,", test2, error));*/
/*???			DEBUGELLIPSE (("%d\n", xoff));*/
/*???			}*/
/*???		error -= test1;*/
/*???		test1 -= cx2 * 2;*/
/*???		HLine (gc,(xc-xoff)/(2/PIXELSIZE),(xc+xoff)/(2/PIXELSIZE)+PIXELSIZE,(yc-yoff)/(2/PIXELSIZE));*/
/*???		HLine (gc,(xc-xoff)/(2/PIXELSIZE),(xc+xoff)/(2/PIXELSIZE)+PIXELSIZE,(yc+yoff)/(2/PIXELSIZE));*/
/*???		DEBUGELLIPSE (("test1,error = %d,%d\n", test1, error));*/
/*???		}*/
/*???*/
/*???	DEBUGELLIPSE (("xmin, xmax = %d,%d\n", (xc-xoff)/2, (xc+xoff)/2));*/
/*???*/
/*???#ifdef INTERNALDEBUG*/
/*???	if ( (xc-xoff)/2 != x1 ) */
/*???		{*/
/*???		DEBUG (("Ellipse Error\n"));*/
/*???		DEBUG (("x1,x2	= %d,%d\n", x1, x2));*/
/*???		DEBUG (("y1,y2	= %d,%d\n", y1, y2));*/
/*???		DEBUG (("xmin,xmax	= %d,%d\n", (xc-xoff)/2, (xc+xoff)/2));*/
/*???		}*/
/*???#endif*/
}
#endif


int32
DrawTo( Item bitmapItem, GrafCon *gc, Coord x, Coord y )
{
	CCB ccb;
	ulong pixeldata[3];
	Coord adx;

	/* This routine needs to be in supervisor mode */

	pixeldata[0] = ((1-1) << PRE0_VCNT_SHIFT) | PRE0_LINEAR | PRE0_BPP_16;
/*???	pixeldata[1] = PRE1_LRFORM | ((1-1) << PRE1_TLHPCNT_SHIFT)*/
	pixeldata[1] = ((1-1) << PRE1_TLHPCNT_SHIFT)
			| (0 << PRE1_WOFFSET10_SHIFT);

	/* put color into pixeldata */
	pixeldata[2] = gc->gc_FGPen << 16;

	ccb.ccb_Flags = CCB_ACW | CCB_ACCW | CCB_LAST | CCB_SPABS
			| CCB_BGND | CCB_NOBLK
			| CCB_LDSIZE | CCB_LDPPMP | CCB_LDPRS | CCB_YOXY;
	ccb.ccb_SourcePtr = (CelData *)(&pixeldata);
	ccb.ccb_Width = 1;
	ccb.ccb_Height = 1;
	ccb.ccb_PIXC = 0x1f401f40;

	ccb.ccb_HDDX = 0;
	ccb.ccb_HDDY = 0;

	if ( y >= gc->gc_PenY )
		{
		ccb.ccb_XPos = gc->gc_PenX << 16;
		ccb.ccb_VDX = (x << 16) - ccb.ccb_XPos;
		ccb.ccb_YPos = gc->gc_PenY << 16;
		ccb.ccb_VDY = (y << 16) - ccb.ccb_YPos;
		}
	else
		{
		ccb.ccb_XPos = x << 16;
		ccb.ccb_VDX = (gc->gc_PenX << 16) - ccb.ccb_XPos;
		ccb.ccb_YPos = y << 16;
		ccb.ccb_VDY = (gc->gc_PenY << 16) - ccb.ccb_YPos;
		}

	adx = ccb.ccb_VDX;
	if ( adx < 0 ) adx = -adx;

	if ( adx >= ccb.ccb_VDY )
		{
		ccb.ccb_HDX = 0;
		ccb.ccb_HDY = (-1) << 20;
		ccb.ccb_VDY += (1 << 16);
		if (ccb.ccb_VDX >= 0) ccb.ccb_VDX += (1 << 16);
		else
			{
			ccb.ccb_VDX -= (1 << 16);
			ccb.ccb_XPos += (1 << 16);
			}
		}
	else
		{
		ccb.ccb_HDX = (1 << 20);
		ccb.ccb_HDY = 0;
		if (ccb.ccb_VDX >= 0)
			{
			ccb.ccb_VDX += (1 << 16);
			}
		else
			{
			ccb.ccb_VDX -= (1 << 16);
			}
		ccb.ccb_VDY += (1 << 16);
		}

/*???	if (y>=gc->gc_PenY) */
/*???		{*/
/*???		ccb.ccb_XPos = (gc->gc_PenX & (-PIXELSIZE)) << (16-PIXELSHIFT);*/
/*???		ccb.ccb_VDX = ((x & (-PIXELSIZE)) << (16-PIXELSHIFT)) - ccb.ccb_XPos;*/
/*???		ccb.ccb_YPos = (gc->gc_PenY & (-PIXELSIZE)) << (16-PIXELSHIFT);*/
/*???		ccb.ccb_VDY = ((y & (-PIXELSIZE)) << (16-PIXELSHIFT)) - ccb.ccb_YPos;*/
/*???		} */
/*???	else */
/*???		{*/
/*???		ccb.ccb_XPos = (x & (-PIXELSIZE)) << (16-PIXELSHIFT);*/
/*???		ccb.ccb_VDX = ((gc->gc_PenX & (-PIXELSIZE)) << (16-PIXELSHIFT)) */
/*???				- ccb.ccb_XPos;*/
/*???		ccb.ccb_YPos = (y & (-PIXELSIZE)) << (16-PIXELSHIFT);*/
/*???		ccb.ccb_VDY = ((gc->gc_PenY & (-PIXELSIZE)) << (16-PIXELSHIFT)) */
/*???				- ccb.ccb_YPos;*/
/*???		}*/
/*???*/
/*???	adx = ccb.ccb_VDX>=0 ? ccb.ccb_VDX : -ccb.ccb_VDX;*/
/*???*/
/*???	if (adx >= ccb.ccb_VDY) */
/*???		{*/
/*???		ccb.ccb_HDX = 0;*/
/*???		ccb.ccb_HDY = (-1)<<20;*/
/*???		ccb.ccb_VDY += 1<<16;*/
/*???		if (ccb.ccb_VDX >= 0) */
/*???			{*/
/*???			ccb.ccb_VDX += 1<<16;*/
/*???			} */
/*???		else */
/*???			{*/
/*???			ccb.ccb_VDX -= 1<<16;*/
/*???			ccb.ccb_XPos += 1<<16;*/
/*???			}*/
/*???		} */
/*???	else */
/*???		{*/
/*???		ccb.ccb_HDX = 1<<20;*/
/*???		ccb.ccb_HDY = 0;*/
/*???		if (ccb.ccb_VDX >= 0) */
/*???			{*/
/*???			ccb.ccb_VDX += 1<<16;*/
/*???			} */
/*???		else */
/*???			{*/
/*???			ccb.ccb_VDX -= 1<<16;*/
/*???			}*/
/*???		ccb.ccb_VDY += 1<<16;*/
/*???		}*/

/*???Superkprintf("gc->gc_PenX=%ld ", (unsigned long)(gc->gc_PenX));*/
/*???Superkprintf("gc->gc_PenY=%ld ", (unsigned long)(gc->gc_PenY));*/
/*???Superkprintf("x=%ld ", (unsigned long)(x));*/
/*???Superkprintf("y=%ld ", (unsigned long)(y));*/
/*???Superkprintf("\n");*/
/*???Superkprintf("  X=$%08lx ", (unsigned long)(ccb.ccb_XPos));*/
/*???Superkprintf("  Y=$%08lx ", (unsigned long)(ccb.ccb_YPos));*/
/*???Superkprintf("\n");*/
/*???Superkprintf("HDX=$%08lx ", (unsigned long)(ccb.ccb_HDX));*/
/*???Superkprintf("HDY=$%08lx ", (unsigned long)(ccb.ccb_HDY));*/
/*???Superkprintf("\n");*/
/*???Superkprintf("VDX=$%08lx ", (unsigned long)(ccb.ccb_VDX));*/
/*???Superkprintf("VDY=$%08lx ", (unsigned long)(ccb.ccb_VDY));*/
/*???Superkprintf("\n");*/
/*???Superkprintf("DDX=$%08lx ", (unsigned long)(ccb.ccb_HDDX));*/
/*???Superkprintf("DDY=$%08lx ", (unsigned long)(ccb.ccb_HDDY));*/
/*???Superkprintf("\n");*/


	gc->gc_PenX = x;
	gc->gc_PenY = y;

	return (SWIDrawCels (bitmapItem, &ccb));
}
