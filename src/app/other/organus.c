/* $Id: organus.c,v 1.8 1994/04/09 00:05:48 stan Exp $ */
/***************************************************************\
 10-31-92  -RJ was here to update to Bluebird Portfolio
 08-16-92  Dale was here to update to new version of Opus
 nn-nn-92  RJ WAS HERE IN ORGANUS.C
\***************************************************************/

/* === RJ's Idiosyncracies === */
#define NOT !
#define FOREVER for(;;)
#define SetFlag(v,f) ((v)|=(f))
#define ClearFlag(v,f) ((v)&=~(f))
#define FlagIsSet(v,f) ((bool)(((v)&(f))!=0))
#define FlagIsClear(v,f) ((bool)(((v)&(f))==0))
#define Error(s) kprintf("ERROR:  %s\n",s)
#define Error2(s,arg) kprintf("ERROR:  %s %s\n",s,arg)
#define Warning(s) kprintf("WARNING:  %s\n",s)


#define TEST_SCRIPT_COUNT 1000

#define BACK_MAX_INTENSITY	15
#define ORGA_COUNT (125*5)
#define COLOR_INCREMENT		0x0421
#define RED_MASK			0x7C00
#define GREEN_MASK			0x03E0
#define BLUE_MASK			0x001F
#define LOOP_COUNT			1200

/* #define DISPLAY_WIDTH	320 */
/* #define DISPLAY_HEIGHT	240 */
/*
 * The following are hacks, but better than the above hardcoding
 */
#define DISPLAY_WIDTH	(Bitmaps[0]->bm_Width)
#define DISPLAY_HEIGHT	(Bitmaps[0]->bm_Height)
#define DISPLAY_HALFWIDTH	(DISPLAY_WIDTH /2)
#define DISPLAY_HALFHEIGHT	(DISPLAY_HEIGHT/2)
#define MAX_XDELTA	5
#define MAX_YDELTA	10
#define BOX_WIDTH	40
#define BOX_HEIGHT	120

#define NOT !

#define Version "0.05a"

#define printf kprintf

#define DEBUG(x)	{ kprintf x ; }
#define FULLDEBUG(x) /* { kprintf x ; } */

#include "types.h"

#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"

#include "stdlib.h"		/* get definition of exit() */

#include "filestream.h"
#include "filestreamfunctions.h"

#include "strings.h"

#include "graphics.h"
#include "event.h"

Rect r;
GrafCon grafcon;
int32 UseSPORT = 0;

long xpos[ORGA_COUNT];
long ypos[ORGA_COUNT];
long color[ORGA_COUNT];
long colorinc[ORGA_COUNT];

ulong Red, Green, Blue;



IOInfo ioInfo;
Item DeviceItem, IOReqItem, ReplyPort;
IOReq *ior;
/*???Console console;*/

Item SPORTDevice;
Item MacDev = -1;
Item SPORTIO[2] = {-1};
Item MacIO = -1;
Item ReplyPort = -1;
IOReq *IOReqPtr = NULL;
IOInfo FileInfo;

ulong ScreenPageCount = 0;
long ScreenByteCount = 0;

Item ScreenItems[2];
Item ScreenGroupItem = 0;
Item BitmapItems[2];
Bitmap *Bitmaps[2];

int32 ScreenSelect = 0;

TagArg ScreenTags[] =
{
	/* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
	/* Kludgy code below presumes that SPORTBITS is the first entry */
	CSG_TAG_SPORTBITS,	(void *)0,
	CSG_TAG_SCREENCOUNT,	(void *)1,
	CSG_TAG_DONE,			0
};

/*
 *  The following two arrays are needed as a workaround for the bug in
 *   the Portfolio 1.2.3 graphics folio.
 */
int32 palheights[] = { 288, 288 };
int32 palwidths[] = { 384, 384 };
TagArg PalScreenTags[] =
{
	/* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
	/* Kludgy code below presumes that SPORTBITS is the first entry */
	CSG_TAG_SPORTBITS,			(void *)0,
	CSG_TAG_SCREENCOUNT,		(void *)1,
	CSG_TAG_SCREENHEIGHT,		(void *)288,
	CSG_TAG_DISPLAYHEIGHT,		(void *)288,
	CSG_TAG_DISPLAYTYPE,		(void *)DI_TYPE_PAL2,
/*
 *  The following two lines are needed as a workaround for the bug in
 *   the Portfolio 1.2.3 graphics folio.
 */
	CSG_TAG_BITMAPWIDTH_ARRAY,	(void *)palwidths,
	CSG_TAG_BITMAPHEIGHT_ARRAY,	(void *)palheights,
	CSG_TAG_DONE,			0
};







ulong random ( ulong );
int32 rand ( void );
void CloseEverything( void );
bool Init( void );
void FadeToBlack( int32 frames );
void ClearBitmap( Bitmap *bitmap );
long ReadFile( char *filename, char *buf, long count );
void SPORTClearBitmaps ( Bitmap *bitmap0, Bitmap *bitmap1 );
void SPORTClearBitmap( Bitmap *bitmap );
bool OpenSPORT( void );
void PrintUsage( char *progname );



#define FADE_FRAMECOUNT 45

Screen *ScreenPtr;

void
PrintScreen( Screen* screen )
{
printf("screen=$%lx ", (unsigned long)(screen));
printf("  scr_ScreenGroupPtr=$%lx ", (unsigned long)(screen->scr_ScreenGroupPtr));
printf("scr_VDLPtr=$%lx ", (unsigned long)(screen->scr_VDLPtr));
printf("\n");
printf("  scr_VDLType=%ld ", (unsigned long)(screen->scr_VDLType));
printf("scr_BitmapCount=%ld ", (unsigned long)(screen->scr_BitmapCount));
printf("scr_TempBitmap=$%lx ", (unsigned long)(screen->scr_TempBitmap));
printf("\n");
}


void
SetBackColor( int32 newcolors )
{

	if ( UseSPORT ) 
		{
		SPORTClearBitmap( Bitmaps[0] );
		return;
		}

	if ( newcolors )
		{
		Red = random( BACK_MAX_INTENSITY );
		Green = random( BACK_MAX_INTENSITY );
		Blue = random( BACK_MAX_INTENSITY );
		}
	r.rect_XLeft = 0;
	r.rect_XRight = Bitmaps[0]->bm_Width;
	r.rect_YTop = 0;
	r.rect_YBottom = Bitmaps[0]->bm_Height;
	SetFGPen( &grafcon, MakeRGB15(Red, Green, Blue) );
	FillRect( BitmapItems[0], &grafcon, &r );
}
  
	
int
organus()
{
	long xbase, ybase;
	int i2;
	long thiscolor;
	long loop;
	int32 newjoy;
    Err err;
    ControlPadEventData cp;

    printf("Initializing event utility\n");
    err = InitEventUtility(1, 0, LC_ISFOCUSED);
    if (err < 0) {
    	PrintError(0,"initialize event utility",0,err);
    	return 0;
    }

	SetBackColor( 1 );

	xbase = random( DISPLAY_HALFWIDTH / 3 ) + DISPLAY_HALFWIDTH / 2;
	ybase = random( DISPLAY_HALFHEIGHT / 4 ) + DISPLAY_HALFHEIGHT / 2;
	for ( i2 = 0; i2 < ORGA_COUNT; i2++ )
		{
		DisplayScreen( ScreenItems[0], 0 );

		colorinc[i2] = COLOR_INCREMENT;
		switch( i2 & 3 )
			{
			case 0:
			case 1:
				xpos[i2] = xbase;
				colorinc[i2] &= (RED_MASK | GREEN_MASK);
				break;
			case 2:
			case 3:
				xpos[i2] = DISPLAY_WIDTH - xbase;
				colorinc[i2] &= (BLUE_MASK);
				break;
			}
		switch( i2 & 3 )
			{
			case 0:
			case 2:
				colorinc[i2] &= (RED_MASK | BLUE_MASK);
				break;
			case 1:
			case 3:
				colorinc[i2] &= (GREEN_MASK);
				break;
			}
		switch( i2 & 3 )
			{
			case 0:
			case 1:
				colorinc[i2] &= (RED_MASK | GREEN_MASK);
				break;
			case 2:
			case 3:
				colorinc[i2] &= (BLUE_MASK);
				break;
			}
		switch( i2 % 5 )
			{
			case 0:
				xpos[i2] = DISPLAY_HALFWIDTH;
				ypos[i2] = DISPLAY_HALFHEIGHT;
				break;
			case 1:
				xpos[i2] = xbase;
				ypos[i2] = ybase;
				break;
			case 2:
				xpos[i2] = xbase;
				ypos[i2] = DISPLAY_HEIGHT - ybase;
				break;
			case 3:
				xpos[i2] = DISPLAY_WIDTH - xbase;
				ypos[i2] = ybase;
				break;
			case 4:
				xpos[i2] = DISPLAY_WIDTH - xbase;
				ypos[i2] = DISPLAY_HEIGHT - ybase;
				break;
			}
		color[i2] = random( 0x8000 );
		}

	for ( loop = LOOP_COUNT; loop > 0; loop-- )	
		{
    err = GetControlPad (1, 0, &cp);
	newjoy = cp.cped_ButtonBits;


		if ( newjoy & ControlA ) SetBackColor( 0 );
		if ( newjoy & ControlStart ) { KillEventUtility(); return( 0 ); }
		
		for ( i2 = 0; i2 < ORGA_COUNT; i2++ )
			{
			thiscolor = color[i2];
			SetFGPen (&grafcon, thiscolor );
			WritePixel( BitmapItems[0], &grafcon, xpos[i2], ypos[i2] );
			xpos[i2] += (random( 3 ) - 1);
			ypos[i2] += (random( 3 ) - 1);
			switch ( i2 & 3 )
				{
				case 0:
					if ( (thiscolor & RED_MASK) == RED_MASK ) 
						colorinc[i2] = (-(COLOR_INCREMENT & RED_MASK)) 
								& RED_MASK;
					else if ( (thiscolor & RED_MASK) == 0 ) 
						colorinc[i2] = (COLOR_INCREMENT) & RED_MASK;
					thiscolor = 
							((thiscolor + colorinc[i2]) & RED_MASK)
							| (thiscolor & GREEN_MASK) 
							| (thiscolor & BLUE_MASK);
					break;
				case 1:
					if ( (thiscolor & GREEN_MASK) == GREEN_MASK ) 
						colorinc[i2] = (-(COLOR_INCREMENT & GREEN_MASK)) 
								& GREEN_MASK;
					else if ( (thiscolor & GREEN_MASK) == 0 ) 
						colorinc[i2] = (COLOR_INCREMENT) & GREEN_MASK;
					thiscolor = 
							(thiscolor & RED_MASK) 
							| ((thiscolor + colorinc[i2]) & GREEN_MASK)
							| (thiscolor & BLUE_MASK);
					break;
				case 2:
					if ( (thiscolor & BLUE_MASK) == BLUE_MASK ) 
						colorinc[i2] = (-(COLOR_INCREMENT & BLUE_MASK)) 
								& BLUE_MASK;
					else if ( (thiscolor & BLUE_MASK) == 0 ) 
						colorinc[i2] = (COLOR_INCREMENT) & BLUE_MASK;
					thiscolor = 
							(thiscolor & RED_MASK)
							| (thiscolor & GREEN_MASK) 
							| ((thiscolor + colorinc[i2]) & BLUE_MASK);
					break;
				default:
					break;
				}
			color[i2] = thiscolor;
			}
		}

	KillEventUtility();		
	return( 1 );
}


void
PrintUsage( char *progname )
{
}



int
main( int argc, char *argv[] )
{
	char *progname;
	int32 record;
	char *ptr, c;
		
	record = 0;
		
	progname = *argv++;
	
	printf( "%s %s\n", progname, Version);

	if ( NOT Init() ) exit( 0 );

	for ( ; argc > 1; argc-- )
		{
		ptr = *argv++;
		switch ( c = *ptr++ )
			{
			case 's':
				UseSPORT = 1;
				break;
			case 'h':
			case '?':
				PrintUsage( progname );
				break;
			default:
				kprintf( "ERROR:  unknown command arg %c\n", c );
				break;
			}
		}

	if ( UseSPORT ) 
		{
		if ( NOT OpenSPORT() ) goto DONE;
		}

	SetBackColor( 1 );

	while ( organus() ) ;

DONE:
	FadeToBlack( FADE_FRAMECOUNT );
	ClearBitmap ( Bitmaps[0] );
	CloseEverything();	
	printf( "\n%s sez:  bye!\n", progname );
}



ulong
random (ulong n)
{
  ulong i, j, k;

  i = (ulong)rand()<<1;
  j = i & 0xffff;
  k = i>>16;
  return (((j*n)>>16) + k*n)>>16;
}



void
CloseEverything()
{
}


bool
Init( void )
/* This routine does all the main initializations.  It should be 
 * called once, before the program does much of anything.  
 * Returns non-FALSE if all is well, FALSE if error
 */
{
	bool retvalue;
	long width, height;
	Screen *screen;
	int32 i;
		
	retvalue = FALSE;
	
	OpenGraphicsFolio();

	ScreenTags[0].ta_Arg = (void *)GETBANKBITS( GrafBase->gf_ZeroPage );

/*
 * Use the method published with Portfolio 1.2 for testing to see
 *  whether or not we're on a PAL machine.
 */
	if (GrafBase->gf_VBLFreq == 50)
		ScreenGroupItem = CreateScreenGroup( ScreenItems, PalScreenTags );
	else
		ScreenGroupItem = CreateScreenGroup( ScreenItems, ScreenTags );

	if ( ScreenGroupItem < 0 )
		{
		PrintfSysErr(ScreenGroupItem);
		printf( "Error:  CreateScreen() returned %ld\n", ScreenGroupItem );
		goto DONE;
		}
	AddScreenGroup( ScreenGroupItem, NULL );

	for ( i = 0; i <= 0; i++ )
		{
		screen = (Screen *)LookupItem( ScreenItems[i] );
		if ( screen == 0 ) 
			{
			Error( "Huh?  Couldn't locate screen?" );
			goto DONE;
			}
		BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
		Bitmaps[i] = screen->scr_TempBitmap;
		}

	EnableHAVG( ScreenItems[0] );
	EnableHAVG( ScreenItems[1] );
	EnableVAVG( ScreenItems[0] );
	EnableVAVG( ScreenItems[1] );

	width = Bitmaps[0]->bm_Width;
	height = Bitmaps[0]->bm_Height;

	ScreenPageCount = (width*2*height
			+ GrafBase->gf_VRAMPageSize-1) 
			/ GrafBase->gf_VRAMPageSize;
	ScreenByteCount = ScreenPageCount * GrafBase->gf_VRAMPageSize;

	retvalue = TRUE;

DONE:
	return( retvalue );
}




void
FadeToBlack( int32 frames )
/* This routine presumes that your palette is linear to start with */
{
	int32 i, k;
	ubyte color, index;
	int32 colorEntry;
	Item t;

	t = GetVBLIOReq();
	for ( i = frames - 1; i >= 0; i-- )
		{
		WaitVBL(t, 1);
		k = (i * 255) / (frames - 1 );
		for ( index = 0; index <= 31; index++ )
			{
			color = (ubyte)(k * index / 31);
			colorEntry = MakeCLUTColorEntry(index, color, color, color);
			SetScreenColor( ScreenItems[0], colorEntry );
			SetScreenColor( ScreenItems[1], colorEntry );
			}
		SetScreenColor( ScreenItems[0], MakeCLUTColorEntry(32, 0, 0, 0) );
		SetScreenColor( ScreenItems[1], MakeCLUTColorEntry(32, 0, 0, 0) );
		}
	DeleteItem(t);
}



void
FillDisplay( GrafCon *gcon, long red, long green, long blue )
/* Fills the gcon display buffer by rendering a rectangle using the
 * MakeRGB15 values specified by the args.  Expects the MakeRGB15 values to 
 * be in range (if not, they will overstrike one another's values).  
 */
{
	Rect WorkRect;

	WorkRect.rect_XLeft = 0;
	WorkRect.rect_XRight = Bitmaps[ ScreenSelect ]->bm_Width - 1;
	WorkRect.rect_YTop = 0;
	WorkRect.rect_YBottom = Bitmaps[ ScreenSelect ]->bm_Height - 1;
	SetFGPen( gcon, MakeRGB15(red, green, blue) );
	FillRect( BitmapItems[ ScreenSelect ], gcon, &WorkRect );
}



void
ClearBitmap( Bitmap *bitmap )
/* Uses the SPORT to clear to zero the bitmap */
{
	GrafCon grafcon;
	
	FillDisplay( &grafcon, 0, 0, 0 );
}



long
ReadFile( char *filename, char *buf, long count )
/* Reads count bytes from the filename file into 
 * the specified buffer.  Returns the actual length of 
 * the read, 
 */
{
	Stream *stream;
	char name[256];

	strcpy( name, filename );

	stream = OpenDiskStream( name, 0 );
	if ( stream != NULL )
		{
		if ( ( count = ReadDiskStream( stream, buf, count ) ) < 0 ) 
			Error2( "Couldn't read file:", filename );
		CloseDiskStream( stream );
		}
	else 
		{
		Error2( "file doesn't exist:", filename );
		count = -1;
		}
	return( count );
}



/* *************************************************************************
 * ***                                        ******************************
 * ***  Rendering and SPORT Support Routines  ******************************
 * ***                                        ******************************
 * *************************************************************************
 */



void
SPORTClearBitmaps ( Bitmap *bitmap0, Bitmap *bitmap1 )
/* Uses the SPORT to clear to zero the two bitmaps */
{
	IOInfo inf[2];
	
	inf[0].ioi_Command = SPORTCMD_CLONE;
	inf[0].ioi_Offset = ~0;
	inf[0].ioi_Flags = 0;
	inf[0].ioi_Unit = 0;
	inf[0].ioi_Recv.iob_Len	= (int)ScreenByteCount;
	inf[0].ioi_Recv.iob_Buffer = (void *) ((ulong) bitmap0->bm_Buffer & 
			(-GrafBase->gf_VRAMPageSize));
	inf[0].ioi_Send.iob_Len = (int)(GrafBase->gf_VRAMPageSize);
	inf[0].ioi_Send.iob_Buffer = GrafBase->gf_ZeroPage;

	/* Structure copy */
	inf[1] = inf[0];
	inf[1].ioi_Recv.iob_Buffer = (void *) ((ulong) bitmap1->bm_Buffer &
					       (-GrafBase->gf_VRAMPageSize));

	/*  No error checking.  Eep!  */
	SendIO( SPORTIO[0], &inf[0] );
	DoIO( SPORTIO[1], &inf[1] );
}



void
SPORTClearBitmap( Bitmap *bitmap )
/* Uses the SPORT to clear to zero the bitmap */
{
	IOInfo inf;

	inf.ioi_Command = SPORTCMD_CLONE;
	inf.ioi_Offset = ~0;
	inf.ioi_Flags = 0;
	inf.ioi_Unit = 0;
	inf.ioi_Recv.iob_Len = (int)(ScreenByteCount);
	inf.ioi_Recv.iob_Buffer = (void *) ((ulong) bitmap->bm_Buffer & 
			(-GrafBase->gf_VRAMPageSize));
	inf.ioi_Send.iob_Len = (int)(GrafBase->gf_VRAMPageSize);
	inf.ioi_Send.iob_Buffer = GrafBase->gf_ZeroPage;

	DoIO( SPORTIO[0], &inf );
}



bool
OpenSPORT( void )
/* Open the SPORT device for blasting the background picture and doing 
 * other SPORT operations
 * 
 * NOTE:  this routine will be replaced when we lose the SPORT operations 
 * when we switch to the new RAM architecture
 */
{
	register int32 i;
	bool retvalue;
	TagArg targs[2];
	
	retvalue = FALSE;

	SPORTDevice = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),
										"SPORT"), 0);
	if (SPORTDevice<0) {
    	Error ("Error opening SPORT device\n");
    	goto DONE;
	}

	targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	targs[0].ta_Arg = (void *)SPORTDevice;
	targs[1].ta_Tag = TAG_END;

	for ( i = 1;  i >= 0; i-- )
		if ( (SPORTIO[i] = CreateItem (MKNODEID (KERNELNODE,
				IOREQNODE), targs)) < 0 )
			{
			Error( "Can't create SPORT I/O" );
			goto DONE;
			}

	retvalue = TRUE;
	
DONE:
	return( retvalue );
}




