/* $Id: test3d.c,v 1.24 1994/09/27 08:52:16 phil Exp $ */

/*******************************************************************
**
** Test 3D Sound Spatialization using a RADAR display.
**
** By: Phil Burk
** 
**
*********************************************************************
** 940525 PLB Cleaned up for Alpha Test
********************************************************************/

#if 0
O- get rid of global graphics data, use GraphicsContext *
O- debug jumps
O- improve forward sound
#endif

#include "types.h"
#include "nodes.h"
#include "mem.h"
#include "strings.h"
#include "stdlib.h"
#include "debug.h"
#include "operamath.h"
#include "filefunctions.h"
#include "graphics.h"
#include "audio.h"
#include "sound3d.h"
#include "stdio.h"
#include "event.h"

#define SAMPLENAME "$pbsamples/asgr.aiff"

static int32 IfDebug3D;

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

#define VERSION "0.09"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	{ if (IfDebug3D) PRT(x); }

/* -------------------- Defines */

#define LEFT_VISIBLE_EDGE (20)
#define TOP_VISIBLE_EDGE (12)

#define CURBITMAPITEM BitmapItems[ScreenSelect]
#define DRAWTO(xp,yp)  { DrawTo ( CURBITMAPITEM, &GCon[0], (xp),(yp)); }

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

/* -------------------- Globals */
#define MAX_NUM_SCREENS (4)
typedef struct GraphicsContext
{
	GrafCon		grcn_GCon;
	Item		grcn_ScreenItems[ MAX_NUM_SCREENS ];
	Item		grcn_BitmapItems[ MAX_NUM_SCREENS ];
	Bitmap		*grcn_Bitmaps[ MAX_NUM_SCREENS ];
	Item		grcn_ScreenGroupItem;
	uint32		grcn_ScreenSelect;
	int32		grcn_NumScreens;
} GraphicsContext;

/* Graphics Context contains drawing information */
GrafCon GCon[2];
Item ScreenItems[2];
Item BitmapItems[2];
Bitmap *Bitmaps[2];

Item ScreenGroupItem = 0;
uint32 ScreenSelect = 0;
int32 NumScreens;

/* -------------------- Functions */

int32 ClearScreen( void );
char *myitoa(int32 num, char *s, int32 radix);
int32 InitGraphics( int32 NumScr );
int32 SwitchScreens( void );
int32 TermGraphics( void );
void  DrawRect( int32 XLeft, int32 YTop, int32 XRight, int32 YBottom );
void ToggleScreen( void );

#define DISPLAY_WIDTH	320
#define DISPLAY_HEIGHT	240

/* Screen Layout */
#define RADAR_X_MIN (30)
#define RADAR_X_MAX (220)
#define RADAR_Y_MIN (30)
#define RADAR_Y_MAX (220)

#define RADAR_X_MIDDLE ((RADAR_X_MIN+RADAR_X_MAX)/2)
#define RADAR_Y_MIDDLE ((RADAR_Y_MIN+RADAR_Y_MAX)/2)
#define RADAR_X_RANGE ((RADAR_X_MAX-RADAR_X_MIN)/2)
#define RADAR_Y_RANGE ((RADAR_Y_MAX-RADAR_Y_MIN)/2)

/* Virtual Space Layout */
#define XPERPIXEL (20)
#define YPERPIXEL (20)
#define XTHRUST   (2)
#define YTHRUST   (2)
#define MAX_X_VEL (10*XPERPIXEL)
#define MAX_Y_VEL (10*YPERPIXEL)
#define MAX_RADIUS (RADAR_X_RANGE * XPERPIXEL)

#ifndef ABS
#define ABS(x) (((x)<0) ? -(x) : (x) )
#endif

typedef struct
{
	int32 mt_XPos;
	int32 mt_YPos;
	int32 mt_XMin;
	int32 mt_YMin;
	int32 mt_XMax;
	int32 mt_YMax;
	int32 mt_XVel;
	int32 mt_YVel;
} MovingTarget;

/* *************************************************************************
 * ***                     *************************************************
 * ***  Data Declarations  *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */


/* Audio data */
Item SampleItem = 0, OutputIns = 0;
Sound3D *Snd3D;
PolarPosition4D Start4D=0, End4D=0, Actual4D = 0;

#define NUM_SCREENS 2

#define CURBITMAPITEM BitmapItems[ScreenSelect]

#define MSG_TEXT_X  (RADAR_X_MAX+4)
#define MSG_TEXT_Y  (30)
#define MSG_LINE_HEIGHT (12)

#define MSG_RADIUS_I 0
#define MSG_THETA_I 1
#define MSG_XPOS_I 2
#define MSG_YPOS_I 3
#define MSG_RDSP_I 4
#define MSG_TDSP_I 5

/********************************************************************/
void t3dDrawMsg( int32 PosIndex , char *msg)
{
	int32 x,y;
	
	x = MSG_TEXT_X;
	y = MSG_TEXT_Y + (PosIndex * MSG_LINE_HEIGHT);
	
	MoveTo( &GCon[0], x, y);
	DrawText8( &GCon[0], CURBITMAPITEM, msg );
}

/********************************************************************/
void t3dDrawNum( int32 PosIndex , char *msg, int32 num)
{
	char Pad[100];

	t3dDrawMsg( PosIndex , msg);
	myitoa(num, Pad, 10);
	DrawText8( &GCon[0], CURBITMAPITEM, Pad );
	DrawText8( &GCon[0], CURBITMAPITEM, "    ");
}

/********************************************************************/
int32 ClearRadarScreen( void )
{
	SetFGPen( &GCon[0], MakeRGB15(0, 0, MAX_BLUE) );
	DrawRect(RADAR_X_MIN,RADAR_Y_MIN, RADAR_X_MAX, RADAR_Y_MAX);
	return 0;
}

/********************************************************************/
int32 DrawTestScreen( void )
{
	
	SetFGPen( &GCon[0], MakeRGB15(MAX_RED/4, MAX_GREEN/2, MAX_BLUE/2) );
	DrawRect(0,0, MAX_X_POS, MAX_Y_POS);
	MoveTo( &GCon[0], RADAR_X_MAX+5, RADAR_Y_MIN);
	DrawText8( &GCon[0], CURBITMAPITEM, "3D Sound" );
	
	ClearRadarScreen();
	return 0;
}


/********************************************************************/
int32 InitMovingTarget( MovingTarget *mt)
{
	mt->mt_XPos = 0;
	mt->mt_YPos = 0;
	mt->mt_XVel = 0;
	mt->mt_YVel = 0;
	
	mt->mt_XMin = -( RADAR_X_RANGE * XPERPIXEL );
	mt->mt_XMax = RADAR_X_RANGE * XPERPIXEL;
	mt->mt_YMin = -( RADAR_Y_RANGE * YPERPIXEL );
	mt->mt_YMax = RADAR_Y_RANGE * YPERPIXEL;
	return 0;
}

/********************************************************************/
int32 MoveTarget( MovingTarget *mt)
{
	mt->mt_XPos += mt->mt_XVel;
	if (mt->mt_XPos < mt->mt_XMin)
	{
		mt->mt_XPos = mt->mt_XMin;
		mt->mt_XVel = -mt->mt_XVel; /* Reflect */
	}
	if (mt->mt_XPos > mt->mt_XMax)
	{
		mt->mt_XPos = mt->mt_XMax;
		mt->mt_XVel = -mt->mt_XVel; /* Reflect */
	}
	mt->mt_YPos += mt->mt_YVel;
	if (mt->mt_YPos < mt->mt_YMin)
	{
		mt->mt_YPos = mt->mt_YMin;
		mt->mt_YVel = -mt->mt_YVel; /* Reflect */
	}
	if (mt->mt_YPos > mt->mt_YMax)
	{
		mt->mt_YPos = mt->mt_YMax;
		mt->mt_YVel = -mt->mt_YVel; /* Reflect */
	}
	
/*	DBUG(("mtx,mtyy = %d, %d\n", mt->mt_XPos,mt->mt_YPos)); */
	return 0;
}
/********************************************************************/
int32 DrawRadarVector ( int32 XPos, int32 YPos )
{
	int32 x,y;
	MoveTo( &GCon[0], RADAR_X_MIDDLE, RADAR_Y_MIDDLE );
	x = RADAR_X_MIDDLE + (XPos / XPERPIXEL);
	y = RADAR_Y_MIDDLE - (YPos / YPERPIXEL);
/* Draw radial vector. */
	DRAWTO( x, y );
	return 0;
}

/********************************************************************/
int32 DrawTarget( MovingTarget *mt)
{
	ClearRadarScreen();
	SetFGPen( &GCon[0], MakeRGB15(MAX_RED, MAX_GREEN, MAX_BLUE/2) );
	DrawRadarVector ( mt->mt_XPos, mt->mt_YPos );
	
	return 0;
}

/********************************************************************/
int32 Hypotenuse (int32 a, int32 b )
{
	int32 c;
	c = Sqrt32(a*a + b*b);
	return c;
}
/********************************************************************/
int32 PolarToRectangular( int32 Radius, frac16 Angle, int32 *XPos, int32 *YPos)
{
	*XPos = MulSF16(Radius, SinF16(Angle));
	*YPos = MulSF16(Radius, CosF16(Angle));
	return 0;
}

/********************************************************************/
int32 t3dDisplayStatus( MovingTarget *TargetPtr )
{
	int32 xa,ya;
	BothEarParams BEP;
	int32 Result;

/* Draw RADAR and targets. */
	DrawTarget( TargetPtr );

/* Get current position, Convert Polar to Rectangular. */
	Get3DSoundPos ( Snd3D, &Actual4D );
	PolarToRectangular( Actual4D.pp4d_Radius, Actual4D.pp4d_Theta, &xa, &ya);

/* Draw current possible positions. */
	SetFGPen( &GCon[0], MakeRGB15(0, MAX_GREEN, 0) );
	DrawRadarVector (xa,ya);
/* Angle is ambiguous */
	DrawRadarVector (xa,-ya);

/* Display numbers. */
	Calc3DSoundFilters ( &End4D, Snd3D->s3d_MinRadius, &BEP );
	t3dDrawNum(0,"RT=", End4D.pp4d_Radius);
	t3dDrawNum(1,"TT=", ConvertF16_32(End4D.pp4d_Theta));
	t3dDrawNum(2,"XP=", TargetPtr->mt_XPos);
	t3dDrawNum(3,"YP=", TargetPtr->mt_YPos);
	t3dDrawNum(4,"RA=", Actual4D.pp4d_Radius);
	t3dDrawNum(5,"TA=", ConvertF16_32(Actual4D.pp4d_Theta) );
	t3dDrawNum(6,"AL=", BEP.bep_RightEar.erp_Alpha);
	t3dDrawNum(7,"BT=", BEP.bep_RightEar.erp_Beta);
	t3dDrawNum(8,"VO=", BEP.bep_RightEar.erp_Amplitude);
	
	Result = SwitchScreens();
	CHECKRESULT(Result,"SwitchScreens");
cleanup:
	return Result;
}

/********************************************************************/
int32 t3dMoveTargetSound( MovingTarget *TargetPtr )
{
	int32 Result;

/* Convert position to polar coordinates. */
	End4D.pp4d_Radius = Hypotenuse(TargetPtr->mt_XPos, TargetPtr->mt_YPos);
	End4D.pp4d_Theta = NormalizeAngle(Atan2F16(Convert32_F16(TargetPtr->mt_YPos),
		Convert32_F16(TargetPtr->mt_XPos)));
	End4D.pp4d_Phi = 0;
	End4D.pp4d_Time = (int32) Get3DSoundTime();
	Result = Move3DSound( Snd3D, &Start4D, &End4D );

/* Make new one the old one. */
	Start4D = End4D;
	
	return Result;
}

/********************************************************************/
int32 Drive3D( void )
{
	int32 doit = TRUE;
	uint32 joy;
	MovingTarget Target;
	int32 Result = 0;
	static ControlPadEventData cped;

	
	Start4D.pp4d_Time = (int32) Get3DSoundTime();
	Get3DSoundPos ( Snd3D, &Actual4D );
	
	InitMovingTarget(&Target);
	while(doit)
	{
DBUG(("=================\n"));
		Result = GetControlPad (1, FALSE, &cped);
		CHECKRESULT(Result,"GetControlPad");
		joy = cped.cped_ButtonBits;

		if (joy & ControlX)
		{
			doit = FALSE;
		}
	
/* Pad applies thrust. */
		if (joy & ControlLeft)
		{
			Target.mt_XVel -= XTHRUST;
			if (Target.mt_XVel < -MAX_X_VEL) Target.mt_XVel = -MAX_X_VEL;
		}
		if (joy & ControlRight)
		{
			Target.mt_XVel += XTHRUST;
			if (Target.mt_XVel  > MAX_X_VEL) Target.mt_XVel = MAX_X_VEL;
		}
			
		if (joy & ControlDown)
		{
			Target.mt_YVel -= YTHRUST;
			if (Target.mt_YVel < -MAX_Y_VEL) Target.mt_YVel = -MAX_Y_VEL;
		}
		if (joy & ControlUp)
		{
			Target.mt_YVel += YTHRUST;
			if (Target.mt_YVel > MAX_Y_VEL) Target.mt_YVel = MAX_Y_VEL;
		}
/* A stops target. */
		if (joy& ControlA)
		{
			
			Target.mt_XVel = 0;
			Target.mt_YVel = 0;
			Set3DSoundRates ( Snd3D, Convert32_F16(1), Convert32_F16(1) );
		}
		
		IfDebug3D = (joy & ControlB);
		
		MoveTarget(&Target);

		t3dMoveTargetSound( &Target );
		
		t3dDisplayStatus( &Target );
		
	}
cleanup:
	return Result;
}


/********************************************************************/

int32 TermSound( void )
{
	int32 Result = 0;
	
	StopInstrument( OutputIns, NULL );
	Stop3DSound( Snd3D, NULL );
	Unload3DSound( Snd3D );
	UnloadSample(SampleItem);
	
	return Result;
}

/********************************************************************/
int32 InitSound( char *SampleName )
{
	int32 Result = 0;
	Item Ins;
	
/* Initialize audio, return if error. */ 
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	Snd3D = Open3DSound();
	CHECKPTR(Snd3D, "Open3DSound");
	
	Result = Load3DSound ( Snd3D );
	CHECKRESULT(Result, "Load3DSound");
		
	SampleItem = LoadSample( SampleName );
	CHECKRESULT(SampleItem, "LoadSample");
	Result = Attach3DSoundSample( Snd3D, SampleItem );
	CHECKRESULT(Result, "Attach3DSoundSample");
	
	OutputIns = LoadInstrument( "directout.dsp", 0, 0);
	CHECKRESULT(OutputIns,"LoadInstrument");
	
/* Connect Sampler to Direct Out */
	Ins = Get3DSoundInstrument( Snd3D );
	Result = ConnectInstruments ( Ins, "OutputLeft",
		OutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments ( Ins, "OutputRight",
		OutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");

/* DirectOut must be started */
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	
	Result = Start3DSound ( Snd3D, &Start4D, NULL);
	CHECKRESULT(Result,"Start3DSound");

	return Result;
	
cleanup:
	TermSound();
	return Result;
}

int32 InitDemo( char *SampleName )
/* This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 */
{
	int32 Result;
	
	Result = OpenMathFolio();
	if (Result < 0)
	{
		PrintError(0,"open math folio",0,Result);
		ERR(("Did you run operamath?\n"));
		return Result;
	}

	Result = InitGraphics(2);
	CHECKRESULT(Result, "InitGraphics");

	Result = SwitchScreens();
	CHECKRESULT(Result,"SwitchScreens");
	DrawTestScreen();
	Result = SwitchScreens();
	CHECKRESULT(Result,"SwitchScreens");
	DrawTestScreen();

	Result = InitEventUtility(1, 0, LC_FocusListener);
	CHECKRESULT(Result,"InitEventUtility");

	Result = InitSound( SampleName );
	CHECKRESULT(Result, "InitSound");
	
cleanup:
	return Result;
}

/********************************************************************/
int32 TermDemo( void )
{
	TermGraphics();
	KillEventUtility();
	return TermSound();
}

/********************************************************************/

int
main( int argc, char *argv[] )
{
	char *progname;
	char *SampleName;
	int32 Result;

	progname = argv[0];
	PRT(( "%s - %s\n", progname, VERSION ));

	SampleName = (argc > 1 ) ? argv[1] : SAMPLENAME;
	
	Result = InitDemo( SampleName );
	if ( Result < 0 )
	{
		printf( "InitDemo() failed, error=%d\n", Result );
		goto cleanup;
	}

	Result = DisplayScreen( ScreenItems[0], 0 );
	if ( Result < 0 )
	{
		printf( "DisplayScreen() failed, error=%d\n", Result );
		goto cleanup;
	}
	
	Drive3D();
	
cleanup:
	TermDemo();
	printf( "\n%s sez:  bye!\n", progname );
	return( (int)Result );
}


/******************* GRAPHICS TOOLS **************************/

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
		c = rem + '0';
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

