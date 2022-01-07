/* File : AccessUtility.c */

#include "AccessUtility.h"
#include "mem.h"
#include "string.h"


extern Item vblItem;

void
UtlGetScreenStruct(screen, screenitem)
ScreenDescr	*screen;
Item		screenitem;
{
	Screen *lscreen;

	lscreen = (Screen *) LookupItem(screenitem);
	screen->sd_ScreenItem = screenitem;
	screen->sd_BitmapItem = lscreen->scr_TempBitmap->bm.n_Item;
	screen->sd_Bitmap = lscreen->scr_TempBitmap;
}



void *GetRectAddr(screen, bounds)
ScreenDescr		*screen;
Rect			*bounds;
{
    return GetPixelAddress(screen->sd_ScreenItem, bounds->rect_XLeft - 2,
                                                  bounds->rect_YTop - 2);
}



void
FlashIt(screen, bounds, sptr, times, col)
ScreenDescr		*screen;
Rect			*bounds;
long			*sptr;
int32			times;
Color			col;
{
	int32	count;

	for (count = 0; count < times; count++)
		{
		UnHiliteButton(screen, bounds, sptr);
		WaitABit(1);
		HiliteButton(screen, bounds, sptr, col);
		WaitABit(1);
		}
}



void
DrawBox( GrafCon *gc, Item bitmap, int32 left, int32 top,
		int32 right, int32 bottom )
{
	MoveTo(gc, left, top);
	DrawTo(bitmap, gc, right, top);
	DrawTo(bitmap, gc, right, bottom);
	DrawTo(bitmap, gc, left,  bottom);
	DrawTo(bitmap, gc, left,  top);
}


int32
AccessXORRect( Item bitmapItem, GrafCon *gc, Rect *rect )
/*
 * Fill the rectangle with the specified color.
 * Clipping to bitmap boundaries is performed.
 */
{
	ulong pixeldata[3];
	int32 left, right, top, bottom, i;
	CCB ccb;

	/* This routine needs to be in supervisor mode */

	pixeldata[0] = ((1-1) << PRE0_VCNT_SHIFT) | PRE0_LINEAR | PRE0_BPP_16;
	pixeldata[1] = ((1-1) << PRE1_TLHPCNT_SHIFT)
			| (0 << PRE1_WOFFSET10_SHIFT);

	/* put color into pixeldata */
	pixeldata[2] = 0xFFFF0000;

	ccb.ccb_Flags = CCB_ACW | CCB_ACCW | CCB_LAST | CCB_SPABS
			| CCB_BGND | CCB_NOBLK
			| CCB_LDSIZE | CCB_LDPPMP | CCB_LDPRS | CCB_YOXY | CCB_PXOR;
	ccb.ccb_SourcePtr = (CelData *)(&pixeldata);
	ccb.ccb_Width = 1;
	ccb.ccb_Height = 1;
	ccb.ccb_PIXC = 0x1f801f80;

	left = rect->rect_XLeft;
	right = rect->rect_XRight;
	top = rect->rect_YTop;
	bottom = rect->rect_YBottom;
	if ( left > right ) { i = left; left = right; right = i; }
	if ( top > bottom ) { i = top; top = bottom; bottom = i; }
	right++;
	bottom++;

	ccb.ccb_XPos = left << 16;
	ccb.ccb_YPos = top << 16;
	ccb.ccb_HDX = ((right << 16) - ccb.ccb_XPos) << 4;
	ccb.ccb_HDY = 0;
	ccb.ccb_VDX = 0;
	ccb.ccb_VDY = (bottom << 16) - ccb.ccb_YPos;
	ccb.ccb_HDDX = 0;
	ccb.ccb_HDDY = 0;

	return( DrawCels( bitmapItem, &ccb ) );
}


void
FileHiliteButton(screen, bounds, sptr, col)
ScreenDescr		*screen;
Rect			*bounds;
long			*sptr;
Color			col;
{
	long	*sav;
	long	*dst;
	long	x, y;
	long	hCount;
	long	vCount;
	GrafCon	localGrafCon;

	dst = (long *) GetRectAddr(screen, bounds);
	sav = (long *) sptr;

	vCount = ((long) bounds->rect_YBottom) - ((long) bounds->rect_YTop) + 4L;
	hCount = ((long) bounds->rect_XRight) - ((long) bounds->rect_XLeft) + 4L;

	for (y = 0; y < (vCount/2); y++)
		{
		// do the vertical stuff
		for (x = 0; x < hCount; x++)
			{
			*sav++ = *dst++;
			}
		dst += (screen->sd_Bitmap->bm_Width - hCount);		// Skip to the next line
		}

	WaitVBL(vblItem,1);

	AccessXORRect( screen->sd_BitmapItem, &localGrafCon, bounds );
}



void
HiliteButton(screen, bounds, sptr, col)
ScreenDescr		*screen;
Rect			*bounds;
long			*sptr;
Color			col;
{
	long	*sav;
	long	*dst;
	long	x, y;
	long	hCount;
	long	vCount;
	GrafCon	localGrafCon;

	dst = (long *) GetRectAddr(screen, bounds);
	sav = (long *) sptr;

	vCount = ((long) bounds->rect_YBottom) - ((long) bounds->rect_YTop) + 4L;
	hCount = ((long) bounds->rect_XRight) - ((long) bounds->rect_XLeft) + 4L;

	for (y = 0; y < (vCount/2); y++)
		{
		// do the vertical stuff
		for (x = 0; x < hCount; x++)
			{
			*sav++ = *dst++;
			}
		dst += (screen->sd_Bitmap->bm_Width - hCount);		// Skip to the next line
		}

	WaitVBL(vblItem,1);

	SetFGPen(&localGrafCon, col);
	DrawBox( &localGrafCon, screen->sd_BitmapItem,
			bounds->rect_XLeft - 2, bounds->rect_YTop - 2,
			bounds->rect_XRight, bounds->rect_YBottom );
}



void UnHiliteButton(screen, bounds, sptr)
ScreenDescr		*screen;
Rect			*bounds;
long			*sptr;
{
	long	*sav;
	long	*dst;
	long	x, y;
	long	hCount;
	long	vCount;

	dst = (long *) GetRectAddr(screen, bounds);
	sav = (long *) sptr;

	vCount = ((long) bounds->rect_YBottom) - ((long) bounds->rect_YTop) + 4L;
	hCount = ((long) bounds->rect_XRight) - ((long) bounds->rect_XLeft) + 4L;

	WaitVBL(vblItem,1);

	for (y = 0; y < (vCount/2); y++) {	// do the vertical stuff
		for (x = 0; x < hCount; x++) {
			*dst++ = *sav++;
		}
		dst += (screen->sd_Bitmap->bm_Width - hCount);		// Skip to the next line
	}
}



void	WaitABit(long b)
{
	long	s;
	s = TickCount();
	while (TickCount() < (s + b));
}



long
*GetSaveArea(void)
{
	long	*newPtr;

	newPtr = (long *) malloc(10000);
	return(newPtr);
}



void
FreeSaveArea(ptr)
long	*ptr;
{
	free((char *) ptr);
}



void
DrawDialogBackground(screen, args, bounds)
ScreenDescr		*screen;
AccessArgs		*args;
Rect			*bounds;
{
	GrafCon		localGrafCon;
	int32		i;

	localGrafCon.gc_PenX = bounds->rect_XLeft;
	localGrafCon.gc_PenY = bounds->rect_YTop;

	SetFGPen(&localGrafCon, args->aa_BGPen);
//	SetBGPen(&localGrafCon, args->aa_BGPen);

	FillRect(screen->sd_BitmapItem, &localGrafCon, bounds);

	SetFGPen(&localGrafCon, args->aa_FGPen);
//	SetBGPen(&localGrafCon, args->aa_FGPen);

	DrawBox( &localGrafCon, screen->sd_BitmapItem,
			bounds->rect_XLeft, bounds->rect_YTop,
			bounds->rect_XRight, bounds->rect_YBottom );

	SetFGPen(&localGrafCon, args->aa_ShadowPen);
//	SetBGPen(&localGrafCon, args->aa_ShadowPen);

	for (i = 1; i <= 3; i++)
		{
		MoveTo(&localGrafCon, bounds->rect_XLeft + i, bounds->rect_YBottom + i);
		DrawTo(screen->sd_BitmapItem, &localGrafCon, bounds->rect_XRight + i,
				bounds->rect_YBottom + i);
		DrawTo(screen->sd_BitmapItem, &localGrafCon, bounds->rect_XRight + i,
				bounds->rect_YTop + i);
		}
}



int32
DrawWooText(gc, bits, text)
GrafCon		*gc;
Item		bits;
uint8		*text;
{
	int32	FontPLUT[] = {0x00000000, 0x00000000, 0x00000000, 0x00000000};
	CCB		ccb;
	Font	*font;

	FontPLUT[0] = (((int32) gc->gc_FGPen) & 0xFFFF)
			| ((((int32) gc->gc_BGPen) << 16) & 0xFFFF0000);
	font = GetCurrentFont();
	memcpy(&ccb, font->font_CCB, sizeof(CCB));
	ccb.ccb_PLUTPtr = FontPLUT;
	SetCurrentFontCCB(&ccb);

	return(DrawText8(gc, bits, text));
}



long
AccGetControlPad( int32 continuous )
{
	ControlPadEventData cp;
	long newjoy, returnjoy;
	static long oldjoy = 0;
	Err result;

    result = GetControlPad (1, 0, &cp);

	if ( result < 0 ) ERROR(result);

	// Get state of buttons
	newjoy = cp.cped_ButtonBits;
	// Get all transitions
	returnjoy = newjoy ^ oldjoy;
	// Get only positive transitions
	returnjoy &= newjoy;
	// Also return any continuous-signal switches that are
	// currently depressed
	returnjoy |= (newjoy & continuous);
	oldjoy = newjoy;

	return ( returnjoy );
}


