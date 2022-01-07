/* $Id: example.c,v 1.21 1994/10/28 05:31:50 ewhac Exp $ */

/* *************************************************************************
 *
 * Opera Example Program  -  Cels & Joysticks
 *
 * This example program plays with cels and the joystick.
 *
 * This file works with any tab space from 1 to 8.
 *
 *
 * HISTORY
 * Date   Author           Description
 * ------ ---------------- -------------------------------------------------
 * 930506 JCR              cp'd ex_new.c -> example.c -> example.c.bak
 * 930503 JCR              Created ex_new.c, son of example.c.
 * 921205 -RJ              Added Mouse support
 * 920816 Dale Luck        Updated to new release of Portfolio
 * 920604 -RJ Mical        Created this file!
 *
 * ********************************************************************** */


#define DBUG(x) printf x


#undef SUPER
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
#include "strings.h"
#include "stdio.h"
#include "stdlib.h"

#include "device.h"
#include "driver.h"

#include "graphics.h"

#include "filesystem.h"
#include "filesystemdefs.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"

#include "event.h"




/* *************************************************************************
 * ***  Discussion  ********************************************************
 * *************************************************************************
 *
 * This program uses some techniques that call for discussion.
 *
 * This program does not clean up after itself.  This is Portfolio-approved.
 * All programs *should* exit without bothering to clean up.  This
 * is because Portfolio promises to clean up and deallocate all system
 * resources for you
 *
 * This program, as any good program, never fails (poof) to detect error
 * conditions and abandon execution when an error is encoubtered.  This
 * is accomplished by faithfully following these design rules:
 *    - always prototype every routine
 *    - any routine that might fail to accomplish its stated task should
 *      be implemented as an "error-returnable" routine which is defined
 *      to return a result to the caller that includes an "error" result
 *    - if an error-returnable routine fails it prints an error message
 *      stating meaningful information about the failure and then
 *      returns the error indicator to the caller
 *    - all routines that call "error-returnable" routines:
 *          - must be error-returnable themselves
 *          - must always check the error result of error-returnable routines
 *          - on receiving an error result must immediately exit, returning
 *            an error result to the caller
 * Using this technique, any error encountered immediately percolates
 * to the top and the program exits without executing any other logic
 * (except perhaps an exit routine, which must be careful to not undo
 * something that was never successfully done).
 */

/* *************************************************************************
 * ***                   ***************************************************
 * ***  Our Definitions  ***************************************************
 * ***                   ***************************************************
 * *************************************************************************
 */

#define VERSION "0.04c"

/* The PLAYVDL_GROUPSIZE describes the number of VDLEntry's needed for each
 * VDL group.  In this example, the size is wide enough to accomodate a
 * full palette entry for each line of the display
 * The constant iis defined using these parts:
 *   - 4 entries for the VDL header
 *   - 1 entry for the display control word
 *   - 32 entries for the CLUT
 *   - 1 entry to set the background color
 *   - 2 entries to round the count up to a multiple of 4, a simplifying
 *     mechanism that makes sure that each VDL group will start on a
 *     4-word boundary
 */
#define PLAYVDL_GROUPSIZE (4+1+32+1+2)

#define VDL_PLAY_THICKNESS 16
#define VDL_PLAY_HEIGHT (DISPLAY_HEIGHT + VDL_PLAY_THICKNESS*2)
#define VDL_PLAY_COUNT  8


/*JCR#define printf kprintf*/

#define DISPLAY_WIDTH	320
#define DISPLAY_HEIGHT	240

#define SEL_ENABLE_VAVG   1
#define SEL_ENABLE_HAVG   2
#define SEL_ENABLE_BOTH   3
#define SEL_DISABLE_BOTH  4


#define FADE_FRAMECOUNT 48

#define LITBOX_WIDTH    40
#define LITBOX_HEIGHT   120

#define MAX_VELOCITY    4


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


/* These represent the value one (1) in various number formats.
 * For example, ONE_12_20 is the value of 1 in fixed decimal format
 * of 12 bits integer, 20 bits fraction
 */
#define ONE_12_20  (1<<20)
#define ONE_16_16  (1<<16)


/* The JOYCONTINUOUS definition specifies those joystick flags that we
 * want to know about all the time, whenever the associated switches
 * are depressed, not just when they make the transition from undepressed
 * to depressed (which is the default).
 */
#define JOYCONTINUOUS  (ControlStart | ControlUp | ControlDown | ControlLeft | ControlRight)


/* === Some more of RJ's Idiosyncracies === */
#define Error(s) kprintf("ERROR:  %s\n",s)
#define Error2(s,arg) kprintf("ERROR:  %s %s\n",s,arg)
#define Warning(s) kprintf("WARNING:  %s\n",s)


/* This Opera Cel structure includes both a pointer to a CCB and the
 * fields that allow the cel to be animated
 */
typedef struct OCel
	{
	CCB *CCB;
	long Flags;

	/* For literal cels this is the exact width and height.
	 * For packed cels these are the width and height of the
	 * bounding box.
	 * ( These are duplicates of fields in the graphics folio's
	 *   temporarily-defined CCB structure, but these are derived
	 *   independently and will still be valid when the CCB ones
	 *   go away.  Poof ;-)
	 */
	long Width, Height;

	long XVelocity, YVelocity;
	} OCel;

/* === OCel Flags === */
#define USE_VELOCITIES	0x00000001



/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

void   AddTestOCels( void );
char  *AllocAndLoadFile( char *filename, ulong memtype );
void   AnimateOCels( void );
void   ClearBitmap( Bitmap *bm1 );
void   ClearBitmaps( Bitmap *bm1, Bitmap *bm2 );
void   CloseEverything( void );
void   CloseMacLink( void );
void   CloseSPORT( void );
void   CopyBackPic( Bitmap *bitmap );
bool   DoJoystick( void );
void   FadeToBlack( int32 frames );
long   GetFileLength( char *filename );
void   IncrementPLUT( CCB *cel );
bool   Init( void );
bool   InitBackPic( char *filename );
void   InitLitCel( OCel *ocel );
void   JamPLUTDMode( OCel *ocel, long ppmpvalue );
char  *LoadFile (char *name, void *buffer, ulong buffersize, ulong memtype);
VDLEntry *MakePlayVDL( uint8 *bufptr, int32 special );
void   MoveOCel( OCel *ocel );
void   MoveAllOCels( void );
bool   OpenMacLink( void );
bool   OpenSPORT( void );
void   PrintCCB( CCB *cel );
void   PrintUsage( char *progname );
ulong  Random (ulong);
void   RandomFillDisplay(GrafCon *gcon,long maxred,long maxgreen,long maxblue);
void   RandomizeLiteral16( OCel *ocel );
void   RandomizeOCel( OCel *ocel );
void   RandomizePLUT( OCel *ocel );
long   ReadFile(char *filename, char *buf, long count );
void   TestFont( void );
int32  VDLPlay( void );
int32  WriteMacFile( char *filename, char *buf, int32 count );



/* *************************************************************************
 * ***                            ******************************************
 * ***  GLOBAL Data Declarations  ******************************************
 * ***                            ******************************************
 * *************************************************************************
 */

#define BOOTY 0
/* Below is my VDL booty JCR */
int32 counter;  /*JCR*/
Item bootyItems[4];
int32 booty_tog=0;
VDLEntry *bootyPtr[4];
int32 VDLlen; /* output param from MakePlayVDL() */
int32 FirstPlay = 1;
int32 ColorEntry[VDL_PLAY_THICKNESS][33][8];

int32 PlayLinePos[VDL_PLAY_COUNT];

int32 NextSelect = SEL_ENABLE_VAVG;

int32 PlayWithVDL = 0;

bool freezeframe = 0;
bool nofileread = 0;

long JoyUpCount = 0;
long JoyDownCount = 0;
long JoyLeftCount = 0;
long JoyRightCount = 0;

long RoundTableX[64] =
	{
	20, 20, 20, 19, 18, 18, 17, 15,
	14, 13, 11, 9, 8, 6, 4, 2,
	0, -1, -3, -5, -7, -8, -10, -12,
	-13, -14, -16, -17, -17, -18, -19, -19,
	-19, -19, -19, -18, -17, -17, -16, -14,
	-13, -12, -10, -8, -7, -5, -3, -1,
	0, 2, 4, 6, 8, 9, 11, 13,
	14, 15, 17, 18, 18, 19, 20, 20,
	};
long RoundTableY[64] =
	{
	0, 2, 4, 6, 8, 9, 11, 13,
	14, 15, 17, 18, 18, 19, 20, 20,
	20, 20, 20, 19, 18, 18, 17, 15,
	14, 13, 11, 9, 8, 6, 4, 2,
	0, -1, -3, -5, -7, -8, -10, -12,
	-13, -14, -16, -17, -17, -18, -19, -19,
	-19, -19, -19, -18, -17, -17, -16, -14,
	-13, -12, -10, -8, -7, -5, -3, -1,
	};

Item MacDev = -1;
Item SPORTIO[2] = {-1};
Item MacIO = -1;
Item ReplyPort = -1;
IOReq *IOReqPtr = NULL;
IOInfo FileInfo;

ulong ScreenPageCount = 0;
long ScreenByteCount = 0;
ubyte *BackPic = NULL;
int32 UserVDL = 0;

GrafCon GCon[2];
long ScreenSelect = 0;
CCB *MasterCel = NULL;
CCB *LastMasterCel = NULL;
Rect WorkRect;

CelData Test16LData[] =
  {
  /* Cel first preamble word bits */
  ((14-PRE0_VCNT_PREFETCH)<<PRE0_VCNT_SHIFT)|PRE0_LINEAR|PRE0_BPP_16,

  /* Cel second preamble word bits */
  ((6-PRE1_WOFFSET_PREFETCH)<<PRE1_WOFFSET10_SHIFT)
  |((12-PRE1_TLHPCNT_PREFETCH)<<PRE1_TLHPCNT_SHIFT),

  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  0xFFFF0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000FFFF,
  0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF,
  0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF,
  0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF,
  0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF,
  0xFFFF0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000FFFF,
  0xFFFF0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000FFFF,
  0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF,
  0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF,
  0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF,
  0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF, 0xFFFF0000, 0x0000FFFF,
  0xFFFF0000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x0000FFFF,
  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
  };

struct CCB Test16LCCB =
	{
	/* ulong ccb_Flags; */
	CCB_LAST|CCB_NPABS|CCB_SPABS|CCB_PPABS|CCB_LDSIZE
	  |PMODE_ONE
	  |CCB_LDPRS|CCB_LDPPMP|CCB_YOXY
	  |CCB_ACW|CCB_ACCW|CCB_BGND,

	/* struct CCB *ccb_NextPtr;Cel *ccb_SourcePtr;void *ccb_PLUTPtr; */
	NULL, Test16LData, NULL,

	/* Coord ccb_XPos, ccb_YPos; */
	/* long ccb_hdx, ccb_hdy, ccb_vdx, ccb_vdy, ccb_ddx, ccb_ddy; */
	0,0,
	ONE_12_20 * 4,0, 0,ONE_16_16 * 4, 0,0,

	/* ulong ccb_PIXC; */
	(PPMP_MODE_NORMAL << PPMP_0_SHIFT)|(PPMP_MODE_AVERAGE << PPMP_1_SHIFT),

	/* long ccb_Width, ccb_Height; */
	0, 0
	};

long AnimCounter = 0;

struct OCel Test16L =
	{
	/* CCB *CCB; */
	&Test16LCCB,

	/* long Flags; */
	USE_VELOCITIES,

	/* long Width, Height; */
	0, 0,

	/* long XVelocity, YVelocity; */
	2, 1,
	};

Item ScreenItems[2];
Item ScreenGroupItem = 0;
Item BitmapItems[2];
Bitmap *Bitmaps[2];
VDLEntry *PlayVDL[2] = 0;
Item PlayVDLItems[2] = 0;

VDLEntry *user_ptr [2];
int32 user_len[2];

TagArg ScreenTags[] =
{
	/* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
	/* Kludgy code below presumes that SPORTBITS is the first entry */
	CSG_TAG_SPORTBITS,	(void *)0,
	CSG_TAG_SCREENCOUNT,	(void *)2,
	CSG_TAG_DONE,			0
};

/* JCR: Same, for implicit passing in of user vdl */
TagArg ScreenTagsUserVDL[] =
{
	/* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
	CSG_TAG_SCREENCOUNT, (void *)2,
	CSG_TAG_VDLPTR_ARRAY, user_ptr,
	CSG_TAG_VDLLENGTH_ARRAY, user_len,
	CSG_TAG_SCREENCOUNT, (void *)2,
	CSG_TAG_DONE,        0
};


Item vblioreq;
Item vramioreq;
Item SPORTDevice;


/* *************************************************************************
 * ***                 *****************************************************
 * ***  Main Routines  *****************************************************
 * ***                 *****************************************************
 * *************************************************************************
 */


int
main( int argc, char *argv[] )
/* This program initializes itself, loads a background picture, then in
 * its main loop does double-buffer display/rendering of the background
 * pic and a cel (or cels) while reacting to input from the joystick.
 *
 * The offscreen buffer:
 *   - is initialized by having the background pic SPORT'ed into it
 *   - has an animating cel rendered onto it
 */
{
	char c, *progname, *ptr, *filename;
	int32 retvalue;

	Err err;

	retvalue = 0;

#if (0)
	{
	int32 i,*tmp,*tmpp;
	tmp = (int32 *)USER_ALLOCMEM( 512*4,
			MEMTYPE_VRAM | MEMTYPE_DMA );
	printf("\n\n BUFFER at %8x\n",tmp);
	tmpp = tmp;
	for (i=0; i<20; i++) *tmpp++ = 0x11111111;
	tmpp = tmp;
	for (i=0; i<20; i++)
		{printf("\n BEFORE FWval= %8x",*tmpp);tmpp++;}
	FlashWriteVRAMPages (tmp,(int32) 0xAAAAAAAA,1);
	tmpp = tmp;
	for (i=0; i<20; i++)
		{printf("\n AFTER FWval= %8x",*tmpp);tmpp++;}
	}
#endif

	progname = *argv++;
	printf( "%s %s\n", progname, VERSION );

	counter = 5*60;
/*???counter = 1;*/

if (argc >= 2) { counter = atoi(*argv++); argc--; }
printf("\ncounter= %d \n",counter);

	filename = "opera.l.b";

	DBUG (("Parse arguments\n"));

	for ( ; argc > 1; argc-- )
		{
		ptr = *argv++;
		switch ( c = *ptr++ )
			{
			case 'u':
				UserVDL = 1;
				break;
			case 'v':
				PlayWithVDL = 1;
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

	DBUG (("Call Init\n"));

	if ( NOT Init() ) {
	  printf ("Init failed\n");
	  goto DONE;
	}


	DBUG (("Call InitBackPic\n"));
	if ( NOT InitBackPic( filename ) )
		{
		/* if it doesn't work, it doesn't kill us */
		kprintf( "Warning:  couldn't load the backdrop picture\n" );
		}


	DBUG (("Call InitEventUtility\n"));
	err = InitEventUtility(1, 0, LC_ISFOCUSED);
	if ( err < 0 ) {
		PrintError(0,"init event utility",0,err);
		goto DONE;
	}

	{
		DBUG (("Call TestFont\n"));
		TestFont();
	}
	FOREVER
		{
		/* Seed up a bit */
		Random( 2 );

		if ( NOT DoJoystick( ) ) goto DONE;
		if ( --counter <= 0 ) goto DONE;  /*JCR*/

		/* Reset the Master display cel list */
		MasterCel = NULL;

		/* Add some test cels to the display list */
		AddTestOCels();

		/* Clear the buffer (SPORT the background).
		 * The call to the SPORT routine, which does a DoIO(), will cause us
		 * to sync up to the vertical blank at this point.  After synching to
		 * the vertical blank, we should have as short a path as
		 * possible between here and the call to DisplayScreen(), with,
		 * at best, only one call to DrawCels() standing between.
		 */
		CopyBackPic( Bitmaps[ScreenSelect] );
		/* Start the cel engine */
		if ( MasterCel )
			{
			/* Also, consider using DrawScreenCels() here */
			retvalue = DrawCels(BitmapItems[ ScreenSelect ], &Test16LCCB);
			if ( retvalue < 0 ) {
				PrintError(0,"\\failure in","DrawCels",retvalue);
				goto DONE;
				}
			}

		retvalue = DisplayScreen( ScreenItems[ ScreenSelect ], 0 );
		if ( retvalue < 0 )
			{
			printf( "DisplayScreen() failed, error=%d\n", retvalue );
			goto DONE;
			}

		if ( MasterCel && (NOT freezeframe) )
			{
			AnimateOCels();
			}

			MoveAllOCels();

		if ( PlayWithVDL ) VDLPlay();

		AnimCounter++;
		ScreenSelect = 1 - ScreenSelect;
		}

DONE:
	FadeToBlack( FADE_FRAMECOUNT );
	CloseEverything();
	printf( "\n%s sez:  bye!\n", progname );
	return( (int)retvalue );
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
	Item cur_item;

	retvalue = FALSE;

	DBUG (("Call OpenGraphicsFolio\n"));
	OpenGraphicsFolio();

	/* JCR */
	if (UserVDL)
		{ /* Pass in an implicit vdl via CSG. Use Blank one (a cheat) */
/*		user_len[0] = user_len[1] = 121*8; */
/* 		user_ptr[0] = user_ptr[1] = GrafBase->gf_VDLBlank; */
		ScreenGroupItem = CreateScreenGroup( ScreenItems, ScreenTags );
		}
	else
		ScreenGroupItem = CreateScreenGroup( ScreenItems, ScreenTags );

	if ( ScreenGroupItem < 0 )
		{
		printf( "Error:  CreateScreen() returned %ld\n", ScreenGroupItem );
		goto DONE;
		}
	DBUG (("Call AddScreenGroup\n"));
	AddScreenGroup( ScreenGroupItem, NULL );

	for ( i = 0; i <= 1; i++ )
		{
		screen = (Screen *)LookupItem( ScreenItems[i] );
		if ( screen == 0 )
			{
			Error( "Huh?  Couldn't locate screen?" );
			goto DONE;
			}
		BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
		Bitmaps[i] = screen->scr_TempBitmap;

		if ( PlayWithVDL )
			{
			/* JCR: Set global VDLlen. */
			PlayVDL[i] = MakePlayVDL( Bitmaps[i]->bm_Buffer,0 );
			if ( PlayVDL[i] == 0 ) goto DONE;
			/* Submit, & copy into sys ram */
			PlayVDLItems[i] = SubmitVDL( PlayVDL[i], (int32)VDLlen,VDLTYPE_COLOR );
			/*USER_FREEMEM(PlayVDL[i],(VDLlen*sizeof(VDLEntry)));   JCR */

			if ( PlayVDLItems[i] < 0 )
				{
				PrintError(0,"\\failure in","SubmitVDL",PlayVDLItems[i]);
				goto DONE;
				}

			cur_item = SetVDL( ScreenItems[i], PlayVDLItems[i] );
			if ( cur_item < 0 )
				{
				PrintError(0,"\\failure in","SetVDL",cur_item);
				goto DONE;
				}
			}
		}

	width = Bitmaps[0]->bm_Width;
	height = Bitmaps[0]->bm_Height;

	{
	  uint32 pagesize;
	  pagesize = (ulong)GetPageSize(MEMTYPE_VRAM);
	  ScreenPageCount = (width*2*height + pagesize-1) / pagesize;
	  ScreenByteCount = ScreenPageCount * pagesize;
	}

	DBUG (("Call OpenMacLink\n"));
	if ( NOT OpenMacLink() ) goto DONE;
	DBUG (("Call OpenSPORT\n"));
	if ( NOT OpenSPORT() ) goto DONE;

	InitLitCel( &Test16L );

	DBUG (("Call CreateVBLIOReq\n"));
	if ((vblioreq=CreateVBLIOReq()) < 0) {
	  goto DONE;
	}
	DBUG (("Call CreateVRAMIOReq\n"));
	if ((vramioreq=CreateVRAMIOReq()) < 0) {
	  goto DONE;
	}

	retvalue = TRUE;

DONE:
	return( retvalue );
}



void
PrintUsage( char *progname )
{
	printf( "usage:  %s # m v h?\n", progname );
	printf( "        # (mandatory if any args)    - run for # frames, then close down.\n");
	printf( "            MUST FOLLOW PROG NAME\n");
	printf( "        v (op)      - Submit/set custom VDL's\n" );
	printf( "        u (op)      - Call CreateScreenGroup w/ USER VDL\n" );
	printf( "        h (op) or ? - Print this help page\n" );
}



/* *************************************************************************
 * ***                                  ************************************
 * ***  Miscellaneous Utility Routines  ************************************
 * ***                                  ************************************
 * *************************************************************************
 */

long
GetJoystick( void )
/* Gets the current state of the joystick switches.  The default is
 * to return the switches that have made a transition from off to on,
 * and any of the JOYCONTINUOUS switches that are currently on.
 *
 * NOTE:  routine doesn't debounce yet.  Maybe someone could hack in a
 * little debounce code for me?
 *
 * Something like this routine should end up somewhere in a library,
 * or perhaps system code.
 */
{
	static long oldjoy;
	long newjoy, returnjoy;
	ControlPadEventData cp;
	Err err;

	/* Which joystick switches have changed since last test? */
    err = GetControlPad (1, 0, &cp);
	if ( err < 0 )
		{
		PrintError(0,"read control pad data in","GetJoystick",err);
		}
	newjoy = cp.cped_ButtonBits;

	returnjoy = newjoy ^ oldjoy;

	/* The default is to return only positive transitions */
	returnjoy &= newjoy;

	/* Also return any of the continuous-signal switches that are
	 * currently joy-depressed
	 */
	returnjoy |= (newjoy & JOYCONTINUOUS);
	oldjoy = newjoy;
	return ( returnjoy );
}



bool
DoJoystick( void )
/* Returns non-FALSE if user didn't press START, else returns FALSE to quit */
{
	bool retvalue;
	long joybits;

	retvalue = FALSE;

	joybits = GetJoystick();

	/* If the user has pressed the START button on the joystick
	 * then return FALSE
	 */
	if ( joybits & ControlStart ) goto DONE;

	/* If the select button is pressed advance to the next select state */
	if ( joybits & ControlC )
		{
		switch ( NextSelect )
			{
			case SEL_ENABLE_VAVG:
				kprintf( "Enable vert. averaging, disable horiz. averaging\n" );
				EnableVAVG( ScreenItems[0] );
				EnableVAVG( ScreenItems[1] );
				DisableHAVG( ScreenItems[0] );
				DisableHAVG( ScreenItems[1] );
				NextSelect = SEL_ENABLE_HAVG;
				break;
			case SEL_ENABLE_HAVG:
				kprintf( "Disable vert. averaging, enable horiz. averaging\n" );
				DisableVAVG( ScreenItems[0] );
				DisableVAVG( ScreenItems[1] );
				EnableHAVG( ScreenItems[0] );
				EnableHAVG( ScreenItems[1] );
				NextSelect = SEL_ENABLE_BOTH;
				break;
			case SEL_ENABLE_BOTH:
				kprintf( "Enable vert. averaging, enable horiz. averaging\n" );
				EnableVAVG( ScreenItems[0] );
				EnableVAVG( ScreenItems[1] );
				EnableHAVG( ScreenItems[0] );
				EnableHAVG( ScreenItems[1] );
				NextSelect = SEL_DISABLE_BOTH;
				break;
			case SEL_DISABLE_BOTH:
				kprintf( "Disable vert. averaging, disable horiz. averaging\n" );
				DisableVAVG( ScreenItems[0] );
				DisableVAVG( ScreenItems[1] );
				DisableHAVG( ScreenItems[0] );
				DisableHAVG( ScreenItems[1] );
				NextSelect = SEL_ENABLE_VAVG;
				break;
			}
		}

	/* With FIRE A toggle between freezing and unfreezing the animation */
	if ( joybits & ControlA ) freezeframe = 1 - freezeframe;

	if ( joybits & ControlB )
		{
		static toggle = 0;
		int32 i9, i8;
		toggle = 1 - toggle;
		printf("toggle=%ld ", (unsigned long)(toggle));
		printf("\n");
		if ( toggle )
			{
			for ( i9 = 0; i9 < 2; i9++ )
				{
#define TOGGLE_X_OFFSET  41
				i8 = SetClipWidth( BitmapItems[i9], 320-TOGGLE_X_OFFSET*2 );
				if ( i8 < 0 )
					{
					PrintError(0,"set clip width",0,i8);
					}
				i8 = SetClipHeight( BitmapItems[i9], 240-36*2 );
				if ( i8 < 0 )
					{
					PrintError(0,"set clip height",0,i8);
					}
				i8 = SetClipOrigin( BitmapItems[i9], TOGGLE_X_OFFSET, 36 );
				if ( i8 < 0 )
					{
					PrintError(0,"set clip origin",0,i8);
					}
				}
			}
		else
			{
			for ( i9 = 0; i9 < 2; i9++ )
				{
				i8 = SetClipOrigin( BitmapItems[i9], 0, 0 );
				if ( i8 < 0 )
					{
					PrintError(0,"set clip origin",0,i8);
					}
				i8 = SetClipWidth( BitmapItems[i9], 320 );
				if ( i8 < 0 )
					{
					PrintError(0,"set clip width",0,i8);
					}
				i8 = SetClipHeight( BitmapItems[i9], 240 );
				if ( i8 < 0 )
					{
					PrintError(0,"set clip height",0,i8);
					}
				}
			}
		}

	if ( joybits & (ControlUp|ControlDown|ControlLeft|ControlRight) )
		{
		if ( joybits & ControlUp )
			{
			JoyUpCount++;
			JoyDownCount = 0;
			Test16L.YVelocity--;
			}
		else if ( joybits & ControlDown )
			{
			JoyUpCount = 0;
			JoyDownCount++;
			Test16L.YVelocity++;
			}
		if ( joybits & ControlLeft )
			{
			JoyLeftCount++;
			JoyRightCount = 0;
			Test16L.XVelocity--;
			}
		else if ( joybits & ControlRight )
			{
			JoyLeftCount = 0;
			JoyRightCount++;
			Test16L.XVelocity++;
			}
		}
	else
		{
		JoyUpCount = 0;
		JoyDownCount = 0;
		JoyLeftCount = 0;
		JoyRightCount = 0;
		}

	retvalue = TRUE;

DONE:
	return( retvalue );
}



bool
InDisplay( long x, long y )
/* If the x,y coordinate is in the display this routine returns non-FALSE
 * else it returns FALSE
 */
{
	if (( x >= 0 ) && ( x < DISPLAY_WIDTH )
			&& ( y >= 0 ) && ( y < DISPLAY_HEIGHT ))
		return( TRUE );
	return( FALSE );
}



ulong
Random( ulong n )
/* Return a random number from 0 to n-1
 * The return value has 16 bits of significance, and ought to be unsigned.
 * Is the above true?
 */
{
	ulong i, j, k;

	i = (ulong)rand() << 1;
	j = i & 0x0000FFFF;
	k = i >> 16;
	return ( ( ((j*n) >> 16) + k*n ) >> 16 );
}



void
CloseEverything( void )
{
/*	ResetSystemGraphics(); */
}


CCB *
CloneCCB( CCB *cloneMe )
{
	CCB *cel;
	uchar *ptr, *ptr2;
	long i;

	cel = (CCB *)USER_ALLOCMEM( sizeof(CCB), MEMTYPE_VRAM | MEMTYPE_CEL );
	if ( NOT cel )
		{
		Error( "not enough memory to clone cels" );
		goto DONE;
		}
	ptr = (uchar *)cel;
	ptr2 = (uchar *)cloneMe;
	for ( i = 0; i < sizeof(CCB); i++ ) *ptr++ = *ptr2++;

DONE:
	return( cel );
}



void
NormalFadeToBlack( int32 frames )
/* This routine presumes that your palette is linear to start with */
{
	int32 q,s,i, k;
	ubyte color, index;
	uint32 colorEntries[32];

q=1; /*JCR, for DONE: */
	s= SetScreenColor( ScreenItems[0], MakeCLUTColorEntry(32, 0, 0, 0) );
	if (s<0)
		{
		PrintError(0,"set screen color",0,s);
		goto DONE;
		}

	s= SetScreenColor( ScreenItems[1], MakeCLUTColorEntry(32, 0, 0, 0) );
	if (s<0)
		{
		PrintError(0,"set screen color",0,s);
		goto DONE;
		}
	for ( i = frames - 1; i >= 0; i-- )
		{
		k = (i * 255) / frames;
		for ( index = 0; index <= 31; index++ )
			{
			color = (ubyte)(k * index / 31);
			colorEntries[index] = MakeCLUTColorEntry(index, color, color, color);
			}
		s= SetScreenColors( ScreenItems[0], colorEntries, 32 );
		if (s<0)
			{
			PrintError(0,"set screen color",0,s);
			goto DONE;
			}
		s= SetScreenColors( ScreenItems[1], colorEntries, 32 );
		if (s<0)
			{
			PrintError(0,"set screen color",0,s);
			goto DONE;
			}
		if ( i > 0 )
			{
			WaitVBL (vblioreq, 1);
			}
		}
DONE:
	q++;
}



void
CustomFadeToBlack( int32 frames )
/* This routine is designed to work with the PlayVDL system.
 */
{
	int32 i, line;
	register uint32 k;
	register ubyte color;
	register int32 colorentry;
	uint32 index;
	VDLEntry *vdl, *basevdl;

	for ( i = frames - 1; i >= 0; i-- )
		{
		k = (i * 255) / frames;
		basevdl = PlayVDL[1 - ScreenSelect] + 5;
		for ( index = 0; index <= 32; index++ )
			{
			vdl = basevdl;
			basevdl++;
			if ( index <= 31 )
				{
				color = (ubyte)(k * index / 31);
				/* The following formula keeps your color within NTSC space,
				 * as required by the kludgy Phillips part
				 */
				color = 16 + ( (color * (235 - 16)) / 255 );
				colorentry = MakeCLUTColorEntry(index, color, color, color);
				}
			else colorentry = VDL_DISPCTRL | VDL_BACKGROUND | 0x00101010;
			for ( line = 0; line < DISPLAY_HEIGHT; line++ )
				{
				*vdl = colorentry;
				vdl += PLAYVDL_GROUPSIZE;
				}
			}
		if ( i > 0 )
			{
			WaitVBL( vblioreq, 1 );
			}
		}
}



void
FadeToBlack( int32 frames )
/* This routine presumes that your palette is linear to start with */
{
	if ( PlayWithVDL ) CustomFadeToBlack( frames );
	else NormalFadeToBlack( frames );
}



/* *************************************************************************
 * ***                                        ******************************
 * ***  Rendering and SPORT Support Routines  ******************************
 * ***                                        ******************************
 * *************************************************************************
 */

bool
InitBackPic( char *filename )
/* Allocates the BackPic buffer.  If a filename is specified the routine
 * loads a picture from the Mac for backdrop purposes.  Presumes that
 * the Mac link is already opened.  If no filename is specified, this
 * routine merely clears the BackPic buffer to zero.
 *
 * If all is well returns non-FALSE, else returns FALSE if error.
 */
{
	bool retvalue;
	ubyte *ptr, *ptr2, *ptr3;
	long i, j;
	int32 width;

	retvalue = FALSE;

	BackPic = (ubyte *)ALLOCMEM( (int)(ScreenByteCount),
				    MEMTYPE_STARTPAGE | MEMTYPE_VRAM | MEMTYPE_CEL);

	if ( NOT BackPic)
		{
		Warning ( "unable to allocate BackPic" );
		goto DONE;
		}

	printf ("%lx %lx\n", (uint32)Bitmaps[0]->bm_Buffer, vramioreq);
	SetVRAMPages( vramioreq, Bitmaps[0]->bm_Buffer, 0x42104210, ScreenPageCount, 0xffffffff );
	DisplayScreen (ScreenItems[0], ScreenItems[0]);

	width = Bitmaps[0]->bm_Width * 2;

	if ( filename ) {
	  if ( ReadFile( filename, BackPic, ScreenByteCount ) < 0 ) {
	    goto DONE;
	  }
	  ptr = BackPic;
	  ptr2 = Bitmaps[0]->bm_Buffer;
	  ptr3 = ptr2 + 2;
	  for ( i = 0; i < DISPLAY_HEIGHT; i += 2) {
	    for ( j = 0; j < DISPLAY_WIDTH; j++) {
	      *ptr3++ = *ptr++;
	      *ptr3 = *ptr++;
	      ptr3 += 3;
	      *ptr2++ = *ptr++;
	      *ptr2 = *ptr++;
	      ptr2 += 3;
	    }
	  }
	}
	CopyVRAMPages( vramioreq, BackPic, Bitmaps[0]->bm_Buffer, ScreenPageCount, -1 );

	retvalue = TRUE;

DONE:
	return(retvalue);
}


#define VAL  0x12345678
#define MASK 0x00FFFF00

void
CopyBackPic( Bitmap *bitmap )
{
/* JCR: NOTE: set the if to a 1 to test Flash Writing*/
if (0)
	{
	int32 i;
	int32 *ptr = (int32 *)bitmap->bm_Buffer;
SetVRAMPages (vramioreq, ptr, 0, ScreenPageCount,0xFFFFFFFF);
SetVRAMPages (vramioreq, ptr, VAL, ScreenPageCount,MASK);
	for (i=0; i<60;i++)
		{

		if (*ptr != VAL&MASK)
		 printf("\n Flash ERROR. ptr *ptr = %8x %8x ",ptr,*ptr);
		ptr += 520;
		}
	}
else
	CopyVRAMPages( vramioreq, bitmap->bm_Buffer, BackPic, ScreenPageCount, -1 );
}



void
ClearBitmaps ( Bitmap *bitmap0, Bitmap *bitmap1 )
/* Uses the SPORT to clear to zero the two bitmaps */
{
  SetVRAMPages (vramioreq, bitmap0->bm_Buffer, 0, ScreenPageCount, 0xffffffff);
  SetVRAMPages (vramioreq, bitmap1->bm_Buffer, 0, ScreenPageCount, 0xffffffff);
}



void
ClearBitmap( Bitmap *bitmap )
/* Uses the SPORT to clear to zero the bitmap */
{
  SetVRAMPages (vramioreq, bitmap->bm_Buffer, 0, ScreenPageCount, 0xffffffff);
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

  SPORTDevice = OpenItem(FindNamedItem(MKNODEID(KERNELNODE,DEVICENODE),	"SPORT"), 0);
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



/* *************************************************************************
 * ***                         *********************************************
 * ***  Mac File I/O Routines  *********************************************
 * ***                         *********************************************
 * *************************************************************************
 */

/* To use the Mac file i/o stuff, you first must open the Mac link
 * by calling OpenMacLink().  The CloseMacLink() routine should be called
 * at the end of program run (or whenever you want to explicitly close the
 * connection to the Mac).
 */

long
GetFileLength( char *filename )
/* If successful returns the length of the named Mac file, else if any
 * error returns a negative value.
 */
{
	long retsize;
	int32 j;

	retsize = -1;
	FileInfo.ioi_Command = MACCMD_FILELEN;
	FileInfo.ioi_Recv.iob_Buffer = &retsize;
	FileInfo.ioi_Recv.iob_Len = sizeof (retsize);
	FileInfo.ioi_Send.iob_Buffer = filename;
	FileInfo.ioi_Send.iob_Len = strlen (filename) + 1;
	FileInfo.ioi_Offset = 0;
	if ( (j = DoIO( MacIO, &FileInfo )) < 0 )
		{
		kprintf ("ERROR:  couldn't get length of Mac file %s (%d,%d)\n",
			filename, j, IOReqPtr->io_Error);
		retsize = -1;
		}
	return ( retsize );
}



long
ReadFile( char *filename, char *buf, long count )
/* Reads count bytes from the filename file into
 * the specified buffer.  Returns the actual length of
 * the read,
 */
{
	int32 j;
	long retvalue;

	retvalue = -1;
	FileInfo.ioi_Command = CMD_READ;
	FileInfo.ioi_Recv.iob_Buffer = buf;
	FileInfo.ioi_Recv.iob_Len = (int)( count & ~3 );
	FileInfo.ioi_Send.iob_Buffer = filename;
	FileInfo.ioi_Send.iob_Len = strlen( filename ) + 1;
	FileInfo.ioi_Offset = 0;
	if ( (j = DoIO( MacIO, &FileInfo )) < 0 )
		{
		printf ("ERROR:  couldn't read Mac file %s (%d,%d)\n",
			filename, j, IOReqPtr->io_Error);
		}
	else retvalue = IOReqPtr->io_Actual;

	return ( retvalue );
}



char *
AllocAndLoadFile( char *filename, ulong memtype )
/* This handy little routine allocates a buffer that's large enough for
 * the named file, then loads that file into the buffer, and
 */
{
	long len;
	char *buf;

	buf = NULL;
	len = GetFileLength( filename );
	if ( len )
		{
		len = ( len + 3 ) & ~3L;
		buf = (char *)USER_ALLOCMEM( (int)len, memtype );
		if ( buf )
			{
			if ( ReadFile( filename, buf, len ) < 0 )
				Error2( "Couldn't read file:", filename );
			}
		else Error2( "Out of memory loading file:", filename );
		}
	else Error2( "file doesn't exist or is empty:", filename );
	return( buf );
}



bool
OpenMacLink( void )
/* Open the communications channel between the Opera and the Macintosh */
{
	bool retvalue;
	TagArg targs[3];

	retvalue = FALSE;

	if ( (MacDev = OpenItem( FindNamedItem(MKNODEID (KERNELNODE, DEVICENODE),
				"mac"), 0) ) < 0 )
		{
		kprintf( "Can't open mac device\n" );
		goto DONE;
		}

	targs[0].ta_Tag = TAG_ITEM_NAME;
	targs[0].ta_Arg = (void *)"reply port";
	targs[1].ta_Tag = TAG_END;

	if ( (ReplyPort = CreateItem (MKNODEID (KERNELNODE, MSGPORTNODE),
				     targs)) < 0 )
		{
		kprintf( "Can't create reply port.\n" );
		goto DONE;
		}

	targs[0].ta_Tag = CREATEIOREQ_TAG_DEVICE;
	targs[0].ta_Arg = (void *)MacDev;
	targs[1].ta_Tag = CREATEIOREQ_TAG_REPLYPORT;
	targs[1].ta_Arg = (void *)ReplyPort;
	targs[2].ta_Tag = TAG_END;

	if ( (MacIO = CreateItem( MKNODEID (KERNELNODE, IOREQNODE),
				 targs )) < 0 )
		{
		PrintError(0,"create IOReqPtr node",0,MacIO);
		goto DONE;
		}
	IOReqPtr = (IOReq *)LookupItem( MacIO );

	retvalue = TRUE;

DONE:
	return( retvalue );
}



int32
WriteMacFile( char *filename, char *buf, int32 count )
/* Reads count bytes from the filename file into
 * the specified buffer.  Returns the actual length of
 * the read,
 */
{
	int32 j;
	long retvalue;

	retvalue = -1;
	FileInfo.ioi_Command = CMD_WRITE;
	FileInfo.ioi_Unit = 0;

	FileInfo.ioi_Send.iob_Buffer = buf;
	FileInfo.ioi_Send.iob_Len = (int)( count & ~3 );
	FileInfo.ioi_Recv.iob_Buffer = filename;
	FileInfo.ioi_Recv.iob_Len = strlen( filename ) + 1;

	FileInfo.ioi_Offset = 0;
	if ( (j = DoIO( MacIO, &FileInfo )) < 0 )
		{
		PrintError(0,"write mac file", filename, j);
#if 0
		PrintError(0,"write mac file", filename, IOReqPtr->io_Error);
#endif
		}
	else retvalue = IOReqPtr->io_Actual;

	return ( retvalue );
}



/* *************************************************************************
 * ***                  ****************************************************
 * ***  Debug Routines  ****************************************************
 * ***                  ****************************************************
 * *************************************************************************
 */

void
PrintCCB( CCB *cel )
/* Dump the contents of the specified cel control block */
{
	printf("CCB=$%lx ", (unsigned long)(cel));
	printf("\n");
	printf("  Flags=$%lx ", (unsigned long)(cel->ccb_Flags));
	printf("(%ld) ", (unsigned long)(cel->ccb_Flags));
	printf("NextPtr=$%lx ", (unsigned long)(cel->ccb_NextPtr));
	printf("(%ld) ", (unsigned long)(cel->ccb_NextPtr));
	printf("CelData=$%lx ", (unsigned long)(cel->ccb_SourcePtr));
	printf("(%ld) ", (unsigned long)(cel->ccb_SourcePtr));
	printf("\n");
	printf("  PLUTPtr=$%lx ", (unsigned long)(cel->ccb_PLUTPtr));
	printf("(%ld) ", (unsigned long)(cel->ccb_PLUTPtr));
	printf("X=$%lx ", (unsigned long)(cel->ccb_XPos));
	printf("(%ld) ", (unsigned long)(cel->ccb_XPos));
	printf("Y=$%lx ", (unsigned long)(cel->ccb_YPos));
	printf("(%ld) ", (unsigned long)(cel->ccb_YPos));
	printf("\n");
	printf("  hdx=$%lx ", (unsigned long)(cel->ccb_HDX));
	printf("(%ld) ", (unsigned long)(cel->ccb_HDX));
	printf("hdy=$%lx ", (unsigned long)(cel->ccb_HDY));
	printf("(%ld) ", (unsigned long)(cel->ccb_HDY));
	printf("vdx=$%lx ", (unsigned long)(cel->ccb_VDX));
	printf("(%ld) ", (unsigned long)(cel->ccb_VDX));
	printf("vdy=$%lx ", (unsigned long)(cel->ccb_VDY));
	printf("(%ld) ", (unsigned long)(cel->ccb_VDY));
	printf("\n");
	printf("  ddx=$%lx ", (unsigned long)(cel->ccb_HDDX));
	printf("(%ld) ", (unsigned long)(cel->ccb_HDDX));
	printf("ddy=$%lx ", (unsigned long)(cel->ccb_HDDY));
	printf("(%ld) ", (unsigned long)(cel->ccb_HDDY));
	printf("\n");
	printf("  PPMPC=$%lx ", (unsigned long)(cel->ccb_PIXC));
	printf("(%ld) ", (unsigned long)(cel->ccb_PIXC));
	printf("Width=$%lx ", (unsigned long)(cel->ccb_Width));
	printf("(%ld) ", (unsigned long)(cel->ccb_Width));
	printf("Height=$%lx ", (unsigned long)(cel->ccb_Height));
	printf("(%ld) ", (unsigned long)(cel->ccb_Height));
	printf("\n");
}



/* *************************************************************************
 * ***                    **************************************************
 * ***  OCel Routines  **************************************************
 * ***                    **************************************************
 * *************************************************************************
 */

int32 xx=55;
void
MoveOCel( OCel *ocel )
{
	long x, y;
	CCB *cel;

#define MIN_X  -15
#define MIN_Y  -15

	cel = ocel->CCB;
	x = cel->ccb_XPos >> 16;
	y = cel->ccb_YPos >> 16;
	x += ocel->XVelocity;
	y += ocel->YVelocity;
	if ( x < MIN_X )
		{
		x = MIN_X - ( x - MIN_X );
		ocel->XVelocity = -ocel->XVelocity;
		}
	else if ( x + ocel->Width > DISPLAY_WIDTH )
		{
		x = DISPLAY_WIDTH - ( x - DISPLAY_WIDTH );
		ocel->XVelocity = -ocel->XVelocity;
		}
	if ( y < MIN_Y )
		{
		y = MIN_Y - ( y - MIN_Y );
		ocel->YVelocity = -ocel->YVelocity;
		}
	else if ( y + ocel->Height > DISPLAY_HEIGHT )
		{
		y = DISPLAY_HEIGHT - ( y - DISPLAY_HEIGHT );
		ocel->YVelocity = -ocel->YVelocity;
		}
	cel->ccb_XPos = x << 16;
	cel->ccb_YPos = y << 16;
}



void
MoveAllOCels( void )
{
	MoveOCel( &Test16L );
}



void
RandomizeOCel( OCel *ocel )
{
	ocel->XVelocity = Random( (MAX_VELOCITY * 2) + 1 ) - MAX_VELOCITY;
	ocel->YVelocity = Random( (MAX_VELOCITY * 2) + 1 ) - MAX_VELOCITY;
	RandomizePLUT( ocel );
}



void
RandomizePLUT( OCel *ocel )
{
	CCB *cel;
	long *ptr;

	cel = ocel->CCB;
	ptr = (long *)(cel->ccb_PLUTPtr);
	if ( ptr ) *ptr = ((*ptr)&0xFFFF8000) | (long)Random( 0x08000 );
}



void
RandomizeLiteral16( OCel *ocel )
{
	long *lptr;
	long width, height, count, rgb;
	long newrgb, newinc;
	CCB *cel;

	cel = ocel->CCB;

	/* Have lptr point to the first word of cel data, which is the
	 * first preamble word of the literal cel.  After grabbing the
	 * preambles, lptr will point to the first word of image data
	 */
	lptr = (long *)(cel->ccb_SourcePtr) + 2;
	width = ocel->Width;
	height = ocel->Height;
	newrgb = Random( (RED_MASK|GREEN_MASK|BLUE_MASK)+1 );
	newinc = Random( BLUE_MASK + 1 );
	for ( count = width / 2 * height; count > 0; count-- )
		{
		rgb = *lptr;
		rgb = rgb & ~( (RED_MASK|GREEN_MASK|BLUE_MASK)
		      |((RED_MASK|GREEN_MASK|BLUE_MASK)<<16) );
		rgb |= ( (newrgb & (RED_MASK|GREEN_MASK|BLUE_MASK)) << 16 );
		newrgb += newinc;
		rgb |= newrgb & (RED_MASK|GREEN_MASK|BLUE_MASK);
		newrgb += newinc;
		*lptr++ = rgb;
		}
}



void
AnimateOCels( void )
{
}



void
AddTestOCels( void )
{
	MasterCel = &Test16LCCB;
	Test16LCCB.ccb_NextPtr = NULL;
}



void
InitLitCel( OCel *ocel )
{
	long *lptr;
	long preamble0;
	long preamble1;
	long width, height;
	CCB *cel;

	cel = ocel->CCB;

	/* Have lptr point to the first word of cel data, which is the
	 * first preamble word of the literal cel.  After grabbing the
	 * preambles, lptr will point to the first word of image data
	 */
	lptr = (long *)cel->ccb_SourcePtr;
	preamble0 = *lptr++;
	preamble1 = *lptr++;
	height = ((preamble0 & PRE0_VCNT_MASK) >> PRE0_VCNT_SHIFT)
			+ PRE0_VCNT_PREFETCH;
	width = ((preamble1 & PRE1_TLHPCNT_MASK) >> PRE1_TLHPCNT_SHIFT)
			+ PRE1_TLHPCNT_PREFETCH;
	ocel->Width = cel->ccb_Width = width;
	ocel->Height = cel->ccb_Height = height;
}



/* *************************************************************************
 * ***                        **********************************************
 * ***  VDL Support Routines  **********************************************
 * ***                        **********************************************
 * *************************************************************************
 */

/* JCR: Added special flag. 0= normal (original RJ).
                            1 = reverse pallete
                            2 = XOR of reverse pallette
These 2 new (wierd) palletts are to test custom submit/set vlds */

VDLEntry *
MakePlayVDL( uint8 *bufptr, int32 special )
/* Create a FULL VDL, with a linear ascending CLUT, and have it address
 * the specified buffer.
 * Return length of VDL list, in words, in the global VDLlen. JCR.
 */
{
	int32 size, height, width, row, i, color;
	VDLEntry *vdl, *linkvdl, displayControl, *retvdl, *savevdl;
	uint8 *prevbufptr;

	size = PLAYVDL_GROUPSIZE;
	retvdl = NULL;
	height = DISPLAY_HEIGHT;
	width = DISPLAY_WIDTH;

	VDLlen = size*height;
	vdl = (VDLEntry *)USER_ALLOCMEM( (sizeof(VDLEntry) * VDLlen),
			MEMTYPE_VRAM | MEMTYPE_DMA );

	if ( vdl == NULL )
		{
		/* out of memory */
		kprintf( "Error:  out of memory allocating play VDL\n" );;
		goto DONE;
		}

	savevdl = vdl; /* for return() */

	prevbufptr = bufptr;
	linkvdl = 0;

	for ( row = 0; row < height; row++ )
		{
		/* Assign the address of the first to vdl, else
		 * if we're beyond the first entry, patch the
		 * previous entry to point to this one
		 */
		if ( row > 0 ) *linkvdl = (VDLEntry)vdl;

		/* Build this vdl entry */
		/* First, the VDL control word */
		*vdl++ = VDL_ENVIDDMA | VDL_LDCUR | VDL_LDPREV
				| ( (size-VDL_LEN_PREFETCH) << VDL_LEN_SHIFT )
				| (1 << VDL_LINE_SHIFT );

		/* Set up our vdl to the buf pointers, and then advance the pointers.
		 * Link previous to the data line before this one
		 */
		*vdl++ = (VDLEntry)bufptr;
		*vdl++ = (VDLEntry)prevbufptr;
		prevbufptr = bufptr;
		if (( row & 1 ) == 0 ) bufptr += 2;
		else bufptr = bufptr - 2 + (width * 2 * 2);

		/* Save a pointer to the field to be patched later */
		linkvdl = vdl++;

		/* Now, create the display control word.  In this case, some
		 * developers will have blue silicon and some will have red so
		 * we have to take different actions depending
		 */
		displayControl = DEFAULT_DISPCTRL;
		if ( FlagIsSet( KernelBase->kb_CPUFlags, KB_RED ) )
			{
			displayControl &= ~VDL_DISPMOD_MASK;
			switch ( width )
				{
				case 320:
					displayControl |= VDL_DISPMOD_320;
					break;
				case 384:
					displayControl |= VDL_DISPMOD_384;
					break;
				case 512:
					displayControl |= VDL_DISPMOD_512;
					break;
				case 640:
					displayControl |= VDL_DISPMOD_640;
					break;
				case 1024:
					displayControl |= VDL_DISPMOD_1024;
					break;
				default:
					kprintf( "Error:  goofy display width in MakePlayVDL()\n" );
					goto DONE;
				}
			}
		*vdl++ = displayControl;

		/* Next, create the actual palette entries for this line.
		 * The palette is linear ascending.
		 * Note that the colors are kept within the range of 16-235,
		 * not 0-255.  This is to keep within NTSC limits, as required
		 * by the kludgy Phillips part
		 */
		switch (special)  /* JCR */
			{
			case 0:  /* RJ's original */
			for ( i = 0; i < 32; i++ )
				{
				color = 16 + (( i * (235-16) ) / 31);
				*vdl++ = (VDLEntry)( (i<<24)
						+ (color<<16) + (color<<8) + color);
				}
			break;
			case 1:  /* reverse pallett */
			for ( i = 0; i < 32; i++ )
				{
				color = 16 + (( i * (235-16) ) / 31);
				color |= 0xf0f00f0f;  /* JCR */
				*vdl++ = (VDLEntry)( (i<<24)
						+ (color<<16) + (color<<8) + color);
				}
			case 2:  /* XOR */
			for ( i = 0; i < 32; i++ )
				{
				color = 16 + (( i * (235-16) ) / 31);
				color ^= 0xffffffff;  /* JCR */
				*vdl++ = (VDLEntry)( (i<<24)
						+ (color<<16) + (color<<8) + color);
				}
			break;
			}  /* end of switch special */

		/* Set the background color to minimum black */
		*vdl++ = VDL_DISPCTRL | VDL_BACKGROUND | 0x00101010;

		/* The two pads at the end are due to a current hardware bug
		 * that will go away before final silicon
		 */
		*vdl++ = VDL_NULLVDL;
		*vdl++ = VDL_NULLVDL;
		}

	/* KLUDGE FOR NOW:  point the last vdl to the end vdl entry */
/*	*linkvdl = (VDLEntry)GrafBase->gf_VDLPostDisplay; */

	retvdl = savevdl;

DONE:
	return( retvdl );
}


int32 GRUNT=11;

int32
VDLPlayGrunt( int32 baseline, int32 rgb )
{
	int32 i, i2, line, k, delta, colorentry, fixedi;
	uint32 color, newcolor;
	VDLEntry *vdl, *vdl2;

	if ( FirstPlay )
		{
		FirstPlay = 0;
		for ( i = 0; i <= VDL_PLAY_THICKNESS; i++ )
		 	{
			if ( i <= 0 ) k = 0;
			else k = (i * 255) / VDL_PLAY_THICKNESS;
			for ( i2 = 0; i2 <= 32; i2++ )
				{
				if ( i2 < 32 ) color = (255 * i2) / 31;
				else color = 0;
				delta = 255 - color;
				newcolor = color + (delta * k) / 255;
				/* The following formula keeps your color within NTSC space */
				color = 16 + ( (color * (235 - 16)) / 255 );
				newcolor = 16 + ( (newcolor * (235 - 16)) / 255 );
				color = color & 0xFF;
				newcolor = newcolor & 0xFF;
				if ( i2 < 32 )
					{
					ColorEntry[i][i2][0]
							= MakeCLUTColorEntry(i2, color, color, color);
					ColorEntry[i][i2][1]
							= MakeCLUTColorEntry(i2, color, color, newcolor);
					ColorEntry[i][i2][2]
							= MakeCLUTColorEntry(i2, color, newcolor, color);
					ColorEntry[i][i2][3]
							= MakeCLUTColorEntry(i2, color, newcolor, newcolor);
					ColorEntry[i][i2][4]
							= MakeCLUTColorEntry(i2, newcolor, color, color);
					ColorEntry[i][i2][5]
							= MakeCLUTColorEntry(i2, newcolor, color, newcolor);
					ColorEntry[i][i2][6]
							= MakeCLUTColorEntry(i2, newcolor, newcolor, color);
					ColorEntry[i][i2][7]
							= MakeCLUTColorEntry(i2, newcolor, newcolor, newcolor);
					}
				else
					{
					ColorEntry[i][i2][0] = VDL_DISPCTRL | VDL_BACKGROUND
							| (color << 16) | (color << 8) | color;
					ColorEntry[i][i2][1] = VDL_DISPCTRL | VDL_BACKGROUND
							| (color << 16) | (color << 8) | newcolor;
					ColorEntry[i][i2][2] = VDL_DISPCTRL | VDL_BACKGROUND
							| (color << 16) | (newcolor << 8) | color;
					ColorEntry[i][i2][3] = VDL_DISPCTRL | VDL_BACKGROUND
							| (color << 16) | (newcolor << 8) | newcolor;
					ColorEntry[i][i2][4] = VDL_DISPCTRL | VDL_BACKGROUND
							| (newcolor << 16) | (color << 8) | color;
					ColorEntry[i][i2][5] = VDL_DISPCTRL | VDL_BACKGROUND
							| (newcolor << 16) | (color << 8) | newcolor;
					ColorEntry[i][i2][6] = VDL_DISPCTRL | VDL_BACKGROUND
							| (newcolor << 16) | (newcolor << 8) | color;
					ColorEntry[i][i2][7] = VDL_DISPCTRL | VDL_BACKGROUND
							| (newcolor << 16) | (newcolor << 8) | newcolor;
					}

				}
			}
		}

	/* At the oldest position, fix the CLUT.  At the other positions
	 * ramp up to full
	 */
	for ( i = -1; i <= VDL_PLAY_THICKNESS; i++ )
	 	{
		if ( i < 0 ) fixedi = 0;
		else fixedi = i;
		line = baseline + i;
		if (( line < 0 ) || ( line >= DISPLAY_HEIGHT )) vdl = 0;
		else vdl = PlayVDL[ScreenSelect]
				+ (line * PLAYVDL_GROUPSIZE)
				+ 5;
		line = baseline + VDL_PLAY_THICKNESS * 2 - i;
		if (( line < 0 ) || ( line >= DISPLAY_HEIGHT )) vdl2 = 0;
		else vdl2 = PlayVDL[ScreenSelect]
				+ (line * PLAYVDL_GROUPSIZE)
				+ 5;
if ( (GRUNT > 0) && (vdl||vdl2) )
	{
	GRUNT--;
	/*printf("\nGRUNT modifying: %8x %8x",vdl,vdl2);*/
	}
		for ( i2 = 0; i2 <= 32; i2++ )
			{
			colorentry = ColorEntry[fixedi][i2][rgb];
			if ( vdl ) *vdl++ = colorentry;
			if ( vdl2 ) *vdl2++ = colorentry;
			}
		}

	return( 0 );
}



int32
VDLPlay()
{
	int32 i;

return(0);
	if ( FirstPlay )
		{
		for ( i = 0; i < VDL_PLAY_COUNT; i++ )
			PlayLinePos[i] = -((VDL_PLAY_HEIGHT * ((VDL_PLAY_COUNT - 1) - i))
					/ VDL_PLAY_COUNT);
		}

	/* Bump the pointers, wrapping them as appropriate */
	for ( i = 0; i < VDL_PLAY_COUNT; i++ )
		{
		VDLPlayGrunt( PlayLinePos[i], i );
		PlayLinePos[i]++;
		if ( PlayLinePos[i] >= DISPLAY_HEIGHT + VDL_PLAY_THICKNESS )
			PlayLinePos[i] = -VDL_PLAY_THICKNESS;
		}

	return( 0 );
}




int32 FontPLUT[] =
	{
	0x001F7FE0, 0x63186318, 0x63186318, 0x63186318,
	};

void
TestFont()
{
	GrafCon gcon;
	int32 err;
	CCB ccb;
	Font *font;

	err = ResetCurrentFont();
	printf( "ResetCurrentFont() = %d\n", err );
	if ( err < 0 ) PrintError(0,"reset current font",0,err);

	gcon.gc_PenX = 20;
	gcon.gc_PenY = 60;
	err = DrawText8( &gcon, BitmapItems[0], "Some 8-bit text for you!\n\n" );
	printf( "DrawText8() = %d\n", err );
	if ( err < 0 ) PrintError(0,"draw 8-bit text with","DrawText8",err);
	gcon.gc_PenX = 20;
	gcon.gc_PenY = gcon.gc_PenY + 10;

	font = GetCurrentFont();
	memcpy( &ccb, font->font_CCB, sizeof(CCB) );
	ccb.ccb_PLUTPtr = FontPLUT;
	SetCurrentFontCCB( &ccb );

	err = DrawText8( &gcon, BitmapItems[0], "ABCDEFGHIJKLMNOPQRST" );
	printf( "DrawText8() = %d\n", err );
	if ( err < 0 ) PrintError(0,"draw 8-bit text with","DrawText8",err);
	gcon.gc_PenX = 20;
	gcon.gc_PenY = gcon.gc_PenY + 10;
	err = DrawText8( &gcon, BitmapItems[0], "UVWXYZ!@#$%^&*()_+<>?[]" );
	printf( "DrawText8() = %d\n", err );
	if ( err < 0 ) PrintError(0,"draw 8-bit text with","DrawText8",err);
	DisplayScreen( ScreenItems[ 0 ], 0 );
	WaitVBL(vblioreq, 5*60);
	WaitVBL(vblioreq, 2*60);
	WaitVBL(vblioreq, 60);

#if 0
	{
	  char *ptr;
	  int32 i, i2;
	  uint16 testbuf[256];

	  SetFileFontCacheSize( (sizeof(FontEntry) + 8*3*4) * 5 + 20 );
	  err = OpenFileFont( "test.font" );
	  printf( "OpenFileFont() = %d\n", err );
	  if ( err < 0 ) PrintError(0,"open file font","test.font",err);
	  else {
	    gcon.gc_PenX = 20;
	    gcon.gc_PenY = gcon.gc_PenY + 10;
	    ptr = "ABCDEFGHIJKLMNOPQRSTUVWXYZAAAAAAAABBKJL\0";
	    i = strlen( ptr );
	    for ( i2 = 0; i2 <= i; i2++ ) {
	      testbuf[i2] = *ptr++;
	    }
	    err = DrawText16( &gcon, BitmapItems[0], testbuf );
	    printf( "DrawText16() = %d\n", err );
	    if ( err < 0 ) {
	      PrintError(0,"draw 16-bit text with","DrawText16",err);
	    } else {
#ifdef UNDEF
	      err = CloseFont();
	      printf( "CloseFont() = %d\n", err );
	      if ( err < 0 ) PrintError(0,"close font",0,err);
#endif
	    }
	  }
	  DisplayScreen( ScreenItems[ 0 ], 0 );
	  /*???	WaitVBLCount(5*60);*/
	  WaitVBL(vblioreq, 60);
	}
#endif

#ifdef UNDEF
	err = ResetCurrentFont();
	printf( "ResetCurrentFont() = %d\n", err );
	if ( err < 0 ) PrintError(0,"reset current font",0,err);
	else
		{
		gcon.gc_PenX = 20;
		gcon.gc_PenY = gcon.gc_PenY + 10;
		ptr = "After ResetCurrentFont() - 16 bit\0";
		i = strlen( ptr );
		for ( i2 = 0; i2 <= i; i2++ ) testbuf[i2] = *ptr++;
		err = DrawText16( &gcon, BitmapItems[0], testbuf );
		printf( "DrawText16() = %d\n", err );
		if ( err < 0 ) PrintError(0,"draw 16 bit text with","DrawText16",err);
		}
	DisplayScreen( ScreenItems[ 0 ], 0 );
/*???	WaitVBLCount(5*60);*/
	WaitVBL(vblioreq, 60);

	err = ResetCurrentFont();
	printf( "ResetCurrentFont() = %d\n", err );
	if ( err < 0 ) PrintError(0,"reset current font",0,err);
	else
		{
		gcon.gc_PenX = 20;
		gcon.gc_PenY = gcon.gc_PenY + 10;
		err = DrawText8( &gcon, BitmapItems[0],
				"After ResetCurrentFont() - 8 bit" );
		if ( err < 0 ) PrintError(0,"draw 8 bit text with","DrawText8",err);
		}
	DisplayScreen( ScreenItems[ 0 ], 0 );
/*???	WaitVBLCount(5*60);*/
	WaitVBL(vblioreq, 60);
#endif
}



