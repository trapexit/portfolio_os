/* $Id: graphic_tools.c,v 1.14 1994/05/13 22:13:30 hayes Exp $ */
/*******************************************************************
**
** Simple graphics for demos.
**
** By: Phil Burk
** Copyright 1993 - 3DO Company, Inc. 
**
********************************************************************/


#include "types.h"
#include "kernel.h"
#include "nodes.h"
#include "mem.h"
#include "strings.h"
#include "stdlib.h"
#include "debug.h"
#include "stdio.h"

#include "graphics.h"
#include "graphic_tools.h"

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}
	
#define CHECKPTR(val,name) \
	if (val == 0) \
	{ \
		Result = -1; \
		ERR(("Failure in %s\n", name)); \
		goto cleanup; \
	}

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */


#define RED_MASK      0x7C00
#define GREEN_MASK    0x03E0
#define BLUE_MASK     0x001F
#define RED_SHIFT     10
#define GREEN_SHIFT   5
#define BLUE_SHIFT    0
#define ONE_RED       (1<<REDSHIFT)
#define ONE_GREEN     (1<<GREENSHIFT)
#define ONE_BLUE      (1<<BLUESHIFT)
#define MAX_RED       (RED_MASK>>RED_SHIFT)
#define MAX_GREEN     (GREEN_MASK>>GREEN_SHIFT)
#define MAX_BLUE      (BLUE_MASK>>BLUE_SHIFT)
#define SCREENWIDTH   (320)
#define SCREENHEIGHT  (240)
#define MAX_X_POS (SCREENWIDTH-1)
#define MAX_Y_POS (SCREENHEIGHT-1)

/* Graphics Context contains drawing information */
GrafCon GCon[2];
Item ScreenItems[2];
Item BitmapItems[2];
Bitmap *Bitmaps[2];

Item ScreenGroupItem = 0;
uint32 ScreenSelect = 0;
int32 NumScreens;

/********************************************************************/

char *myitoa(int32 num, char *s, int32 radix)
{
#define MAXITOA 64
	char Pad[MAXITOA];
	int32 nd = 0, a, rem, div, i;
	int32 ifneg = FALSE;
	char *p, c;

	if (num < 0)
	{
		ifneg = TRUE;
		num = -num;
	}
	
#define HOLD(ch) { Pad[nd++] = (ch); }
	a = num;
	do
	{
		div = a/radix;
		rem = a - (div*radix);
		c = (char)rem + '0';
		HOLD(c);
		a = div;
	} while(a > 0);
	
	if (ifneg) HOLD('-');
	
/* Copy string to s. */
	p = s;
	for (i=0; i<nd; i++)
	{
		*p++ = Pad[nd-i-1];
	}
	*p++ = '\0';
	return s;
}

/********************************************************************/
int32 DrawNumber( int32 num )
{
	char Pad[100];

	sprintf(Pad, "%d", num);
	DrawText8( &GCon[0], CURBITMAPITEM, Pad );
	return DrawText8( &GCon[0], CURBITMAPITEM, "    ");
}

/********************************************************************/

void DrawRect( int32 XLeft, int32 YTop, int32 XRight, int32 YBottom )
{
	Rect WorkRect;
	
	WorkRect.rect_XLeft = XLeft;
	WorkRect.rect_XRight = XRight;
	WorkRect.rect_YTop = YTop;
	WorkRect.rect_YBottom = YBottom;
	
	FillRect( CURBITMAPITEM, &GCon[0], &WorkRect );
}

void ToggleScreen ( void )
{
	if (NumScreens < 2) return;
	ScreenSelect = 1 - ScreenSelect;
}
/*******************************************************************/
int32 SwitchScreens ( void )
{
	Item t;
	int32 Result = 0;
	
	t = GetVBLIOReq();
	WaitVBL(t, 1);
	DeleteItem(t);
	
	if (NumScreens < 2) return Result;
	ToggleScreen();
	Result = DisplayScreen( ScreenItems[ ScreenSelect ], 0 );
	CHECKRESULT(Result, "DisplayScreen" );
cleanup:
	return Result;
}

/*******************************************************************/
int32 ClearScreen()
{
	DBUG(("Clear Screen\n"));
	SetFGPen( &GCon[0], MakeRGB15(4,0,6) );
	DrawRect ( 0,0, MAX_X_POS,MAX_Y_POS);
	return 0;
}

/********************************************************************/
int32 ShowScreenLimits( int32 x1, int32 y1, int32 x2, int32 y2 )
{
	int32 red, green, blue;
	int32 xmid,ymid;

	red = MAX_RED;
	green = 0;
	blue = MAX_BLUE;
	SetFGPen( &GCon[0], MakeRGB15(red, green, blue) );

	MoveTo ( &GCon[0], x1,y1 );
/* Draw Box around screen. */
	DRAWTO(x2,y1);
	DRAWTO(x2,y2);
	DRAWTO(x1,y2);
	DRAWTO(x1,y1);
/* Draw Diagonals */
	DRAWTO(x2,y2);
	DRAWTO(x1,y2);
	DRAWTO(x2,y1);
/* Draw Diamond */
	xmid = (x1+x2)/2;
	ymid = (y1+y2)/2;
	MoveTo ( &GCon[0], xmid,y1 );
	DRAWTO(x2,ymid);
	DRAWTO(xmid,y2);
	DRAWTO(x1,ymid);
	DRAWTO(xmid,y1);
	return 0;
}

int32 TermGraphics( void )
{
	int32 Result = 0;
	
	if (ScreenGroupItem) Result = DeleteItem( ScreenGroupItem);
	
/*	ResetSystemGraphics(); */
	return Result;
}

TagArg ScreenTags[] =
{
	CSG_TAG_SCREENCOUNT,	0,
	CSG_TAG_DONE,			0
};


/********************************************************************/


/************************************************************************/
int32 InitGraphics ( int32 NumScr )
{
	int32 Result;
	Screen *screen;
	TagArg ScreenTags[2];
	int32 i;

/* Must be called before making any graphics calls. */
	Result = OpenGraphicsFolio();
	if (Result < 0)
	{
		printf("Could not open graphix folio!, Error = 0x%x\n", Result);
		return (Result);
	}
	
/* Create screen group based on Tag Args. */
	ScreenTags[0].ta_Tag = CSG_TAG_SCREENCOUNT;
	ScreenTags[0].ta_Arg = (void *) NumScr;
	ScreenTags[1].ta_Tag = TAG_END;
	NumScreens = NumScr;
	ScreenGroupItem = CreateScreenGroup( ScreenItems, ScreenTags );
	if (ScreenGroupItem < 0)
	{
		PrintError(0,"create screen group",0,ScreenGroupItem);
		return (ScreenGroupItem);
	}

/* Make Screen Group available for display. */
	AddScreenGroup( ScreenGroupItem, NULL );

/* Get Bitmaps and Save in globals for later use. */
	for ( i = 0; i < NumScr; i++ )
	{
/* Lookup address of Ith screen. */
		screen = (Screen *)LookupItem( ScreenItems[i] );
		if ( screen == 0 )
		{
			printf( "InitGraphics: Bad screen Item!\n" );
			return (-1);
		}
		BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
		Bitmaps[i] = screen->scr_TempBitmap;
	}

	return( Result );
}

