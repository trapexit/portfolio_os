/* $Id: simple.c,v 1.10 1994/10/28 05:31:50 ewhac Exp $ */

/* *************************************************************************
 *
 * Simple Example Program  -  Cels & Joysticks
 *
 * This example program plays with cels and the joystick.
 *
 * This file works with any tab space from 1 to 8.
 *
 *
 * HISTORY
 * Date   Author           Description
 * ------ ---------------- -------------------------------------------------
 * 921022 -RJ Mical        Created this file from test.c
 *
 * ********************************************************************** */


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
#include "stdlib.h"
#include "operror.h"

#include "graphics.h"


#define VERSION "0.01"

#define printf kprintf


/* These represent the value one (1) in various number formats.
 * For example, ONE_12_20 is the value of 1 in fixed decimal format
 * of 12 bits integer, 20 bits fraction
 */
#define ONE_12_20  (1<<20)
#define ONE_16_16  (1<<16)


/* === RJ's Idiosyncracies === */
#define Error(s) kprintf("ERROR:  %s\n",s)
#define Error2(s,arg) kprintf("ERROR:  %s %s\n",s,arg)
#define Warning(s) kprintf("WARNING:  %s\n",s)



/* *************************************************************************
 * ***                       ***********************************************
 * ***  Data Declarations    ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

VDLEntry *user_ptr [2];
int32 user_len[2];
TagArg ScreenTags[] =
{
	/* NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE */
	CSG_TAG_SCREENCOUNT, (void *)2,
	CSG_TAG_DONE,        0,
	CSG_TAG_VDLPTR_ARRAY, user_ptr,
	CSG_TAG_VDLLENGTH_ARRAY, user_len,
	CSG_TAG_SCREENCOUNT, (void *)2,
	CSG_TAG_DONE,        0
};


Item ScreenGroupItem = 0;
Item ScreenItems[2] = {0};
Bitmap *Bitmaps[2];
Item BitmapItems[2];


CelData TestData[] =
  {
  /* Cel first preamble word bits */
  ((14-PRE0_VCNT_PREFETCH)<<PRE0_VCNT_SHIFT)|PRE0_LINEAR|PRE0_BPP_8,

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


struct CCB TestCCB =
	{
	/* ulong ccb_Flags; */
	CCB_LAST|CCB_NPABS|CCB_SPABS|CCB_PPABS|CCB_LDSIZE
	  |PMODE_ONE
	  |CCB_LDPRS|CCB_LDPPMP|CCB_YOXY
	  |CCB_ACW|CCB_ACCW|CCB_BGND,

	/* struct CCB *ccb_NextPtr;Cel *ccb_CelData;void *ccb_PLUTPtr; */
	NULL, TestData, NULL,

	/* Coord ccb_XPos, ccb_YPos; */
	/* long ccb_hdx, ccb_hdy, ccb_vdx, ccb_vdy, ccb_ddx, ccb_ddy; */
	0, 0,
	ONE_12_20 * 10,0, 0,ONE_16_16 * 10, 0,0,

	/* ulong ccb_PIXC; */
	(PPMP_MODE_NORMAL << PPMP_0_SHIFT)|(PPMP_MODE_AVERAGE << PPMP_1_SHIFT),

	/* long ccb_Width, ccb_Height; */
	0, 0
	};


ulong ScreenPageCount = 0;
long ScreenByteCount = 0;

Item vramioreq, vblioreq;


/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

void ClearBitMap( Bitmap *bitmap );
ulong Random( ulong n );


/* *************************************************************************
 * ***                 *****************************************************
 * ***  Main Routines  *****************************************************
 * ***                 *****************************************************
 * *************************************************************************
 */

int
main( int argc, char *argv[] )
{
	char *progname;
	char screenSelect;
	int32 retvalue, i;
	int32 width, height, bigsize;
	Screen *screen;
	int32 x, y;
	int32 counter,xinc, yinc;

	retvalue = 0;
	progname = *argv++;
	printf( "%s %s\n", progname, VERSION );

if (argc != 2) {printf("\nusage: simple num_frames\n");goto DONE;}
	counter= atoi(*argv++); printf("\ncounter= %d",counter); /*JCR*/

	retvalue = 0;

	screenSelect = 0;

	retvalue = OpenGraphicsFolio();
	if ( retvalue < 0 ) {
	  printf( "Error:  OpenGraphicsFolio() returned %ld\n", retvalue );
	  goto DONE;
	}

	vramioreq = CreateVRAMIOReq ();
	if (vramioreq<0) {
	  PrintError(0,"get SPORT IO request",0,vramioreq);
	  exit ((int)vramioreq);
	}

	vblioreq = CreateVBLIOReq ();
	if (vramioreq<0) {
	  PrintError(0,"get VBL timer IO request",0,vblioreq);
	  exit ((int)vblioreq);
	}

/* user_len[0] = user_len[1] = 121*8; */
/* user_ptr[0] = user_ptr[1] = GrafBase->gf_VDLBlank; */

	ScreenGroupItem = CreateScreenGroup( ScreenItems, ScreenTags );
	if ( ScreenGroupItem < 0 ) {
	  printf( "Error:  CreateScreen() returned %ld\n", ScreenGroupItem );
	  goto DONE;
	}
	AddScreenGroup( ScreenGroupItem, NULL );

	for ( i = 0; i <= 1; i++ )
		{
		screen = (Screen *)LookupItem( ScreenItems[i] );
		BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
		Bitmaps[i] = screen->scr_TempBitmap;
		}

	width = screen->scr_TempBitmap->bm_Width;
	height = screen->scr_TempBitmap->bm_Height;
	bigsize = width * height * 2;

	{
	  uint32 pagesize;
	  pagesize = (ulong)GetPageSize(MEMTYPE_VRAM);
	  ScreenPageCount = (bigsize + pagesize-1) / pagesize;
	  ScreenByteCount = ScreenPageCount * pagesize;
	}

	x = y = 0;
	xinc = 2;
	yinc = 1;

	FOREVER
		{
/*???		if ( GrafBase->gf_JoySave & JOYSTART ) goto GOOD_DONE;*/
		if ( --counter <= 0 ) goto GOOD_DONE;

		TestCCB.ccb_XPos = x << 16;
		TestCCB.ccb_YPos = y << 16;
		x += xinc;
		if ( x < 0 )
			{
			x = 0 - (x - 0);
			xinc = Random( 3 ) + 1;
			}
		if ( x > 319 )
			{
			x = 319 - (x - 319);
			xinc = -(Random( 3 ) + 1);
			}
		y += yinc;
		if ( y < 0 )
			{
			y = 0 - (y - 0);
			yinc = Random( 3 ) + 1;
			}
		if ( y > 239 )
			{
			y = 239 - (y - 239);
			yinc = -(Random( 3 ) + 1);
			}

		WaitVBL (vblioreq, 1);
		ClearBitMap( Bitmaps[ screenSelect ] );

		retvalue = DrawCels( BitmapItems[ screenSelect ], &TestCCB );
		if ( retvalue < 0 )
			{
			printf( "DrawCels() failed, error=%d\n", retvalue );
			goto DONE;
			}

		retvalue = DisplayScreen( ScreenItems[ screenSelect ], 0 );
		if ( retvalue < 0 )
			{
			printf( "DisplayScreen() failed, error=%d\n", retvalue );
			goto DONE;
			}

		/* Toggle the screen selecter */
		screenSelect = 1 - screenSelect;
		}

GOOD_DONE:
	retvalue = 0;

DONE:
	printf( "%s sez:  bye!\n", progname );
	return( (int)retvalue );
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
ClearBitMap( Bitmap *bitmap )
/* Uses the SPORT to clear to zero the bitmap */
{
  Err i;
  uint32 ps;
  ps = (ulong)GetPageSize(MEMTYPE_VRAM);
  i = SetVRAMPages (vramioreq, bitmap->bm_Buffer, 0,
		    (bitmap->bm_Width*bitmap->bm_Height*2+ps-1)	/ ps,
		    0xffffffff);
  if (i<0) {
    PrintError(0,"clear bit map using","SetVRAMPages",i);
  }
}


