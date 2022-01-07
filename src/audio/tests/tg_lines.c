/* $Id: tg_lines.c,v 1.3 1994/02/09 01:22:45 limes Exp $ */

/* *************************************************************************
 *
 * tg_boxes  -  Simple Random Rectangles
 *
 * HISTORY
 * Date   Author           Description
 * ------ ---------------- -------------------------------------------------
 * 921201 Phil Burk        Convert to simple lines demo
 * 920816 Dale Luck        Updated to new release of Portfolio
 * 920604 -RJ Mical        Created this file!
 *
 * ********************************************************************** */


#include "types.h"

#include "kernel.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "mem.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "debug.h"

#include "graphics.h"

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

#define VERSION "0.04"

#define printf kprintf

#define DISPLAY_WIDTH	320
#define DISPLAY_HEIGHT	240

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

/* === Some more of RJ's Idiosyncracies === */
#define Error(s) kprintf("ERROR:  %s\n",s)
#define Error2(s,arg) kprintf("ERROR:  %s %s\n",s,arg)
#define Warning(s) kprintf("WARNING:  %s\n",s)



/* *************************************************************************
 * ***                       ***********************************************
 * ***  Function Prototypes  ***********************************************
 * ***                       ***********************************************
 * *************************************************************************
 */

int32 InitDemo( void );
void RandomLines ( void );
ulong Random (ulong);

/* *************************************************************************
 * ***                     *************************************************
 * ***  Data Declarations  *************************************************
 * ***                     *************************************************
 * *************************************************************************
 */


/* Graphics Context contains drawing information */
GrafCon GCon[2];

Item ScreenItems[2];
Item ScreenGroupItem = 0;
Item BitmapItems[2];
Bitmap *Bitmaps[2];

#define NUM_SCREENS 1

TagArg ScreenTags[] =
{
	CSG_TAG_SCREENCOUNT,	(void *)NUM_SCREENS,
	CSG_TAG_DONE,			0
};


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
	int32 retvalue;

	progname = *argv++;
	printf( "%s %s\n", progname, VERSION );

	if ( (retvalue = InitDemo()) != 0 ) goto DONE;

	retvalue = DisplayScreen( ScreenItems[0], 0 );
	if ( retvalue < 0 )
	{
		printf( "DisplayScreen() failed, error=%d\n", retvalue );
		goto DONE;
	}
	
	RandomLines();
	
DONE:
/*	ResetSystemGraphics(); */
	printf( "\n%s sez:  bye!\n", progname );
	return( (int)retvalue );
}


int32 InitDemo( void )
/* This routine does all the main initializations.  It should be
 * called once, before the program does much of anything.
 * Returns non-FALSE if all is well, FALSE if error
 */
{
	int32 retvalue;
	Screen *screen;
	int32 i;

	retvalue = -1;

	OpenGraphicsFolio();

	ScreenGroupItem = CreateScreenGroup( ScreenItems, ScreenTags );
	if ( ScreenGroupItem < 0 )
		{
		printf( "Error:  CreateScreen() returned %ld\n", ScreenGroupItem );
		goto DONE;
		}
	AddScreenGroup( ScreenGroupItem, NULL );

	for ( i = 0; i < NUM_SCREENS; i++ )
		{
		screen = (Screen *)LookupItem( ScreenItems[i] );
		if ( screen == 0 )
			{
			Error( "Huh?  Couldn't lookup screen?" );
			goto DONE;
			}
		BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
printf("BitmapItems[i]=%ld ", (unsigned long)(BitmapItems[i]));
		Bitmaps[i] = screen->scr_TempBitmap;
printf("Bitmaps[i]=$%lx ", (unsigned long)(Bitmaps[i]));
printf("\n");
		}

	retvalue = FALSE;

DONE:
	return( retvalue );
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

void RandomLines ( void )
{
	int32 x1,y1;
	int32 red, green, blue;
	int32 linecount=0;
	
	MoveTo ( &GCon[0], 20, 20 );
	
	while(1)
	{
		red = Random(MAX_RED);
		green = Random(MAX_GREEN);
		blue = Random(MAX_BLUE);
		SetFGPen( &GCon[0], MakeRGB15(red, green, blue) );
		
		x1 = Random(DISPLAY_WIDTH);
		y1 = Random(DISPLAY_HEIGHT);
/*
kprintf("x1 = %d, y1 = %d, ", x1,y1);
kprintf("x2 = %d, y2 = %d\n", x2,y2);
*/
		DrawTo ( BitmapItems[0], &GCon[0], x1,y1);
		if((linecount++ & 0x7FFF) == 0)
		{
			printf("%d lines drawn\n", linecount);
		}
	}
}

