/* $Id: faders.c,v 1.16 1994/03/01 01:15:35 peabody Exp $ */
/***************************************************************
**
** Horizontal faders.
**
** By: Phil Burk
** Copyright 1993 - 3DO Company, Inc.
**
****************************************************************
**  940225 WJB  Added DrawFaderBlock().  Added const to DrawFader().
**  940228 WJB  Fixed functions to tolerate fdbl_NumFaders == 0.  Cleaned up includes.
**  940228 WJB  Changed fader fine adjustment to use smaller increments near 0.
**  940228 WJB  Now only calls fdbl_Function() when fader value actually changes.
**  940228 WJB  Added string.h.
****************************************************************/

#include <event.h>          /* Control Pad stuff */
#include <stdio.h>
#include <string.h>         /* memset() */

#include "audiodemo.h"


/* -------------------- Debugging */

#define DEBUG_JoyToFader    0

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
#define	DBUG(x)	PRT(x)


/* -------------------- Fader functions */

/********************************************************************/
int32 InitFader ( Fader *fdr , int32 Index)
{
    memset (fdr, 0, sizeof *fdr);

	fdr->fdr_XMin = 120;
	fdr->fdr_XMax = 250;
	fdr->fdr_YMin = FADER_YMIN + ( Index * FADER_SPACING );
	fdr->fdr_YMax = fdr->fdr_YMin + FADER_HEIGHT;
	fdr->fdr_VMin = 0;   /* Value min and max. */
	fdr->fdr_VMax = MAX_FADER_VALUE;
	fdr->fdr_Value = MAX_FADER_VALUE/2;
	fdr->fdr_Increment = 20;

	return 0;
}

/********************************************************************/
Err DrawFader ( const Fader *fdr )
{
	int32 xpos, t;

	if (fdr->fdr_Name )
	{
		MoveTo( &GCon[0], LEFT_VISIBLE_EDGE+4, fdr->fdr_YMin+2 );
		DrawText8( &GCon[0], CURBITMAPITEM, fdr->fdr_Name );
	}

	t = (fdr->fdr_Value - fdr->fdr_VMin) * ( fdr->fdr_XMax - fdr->fdr_XMin );
	xpos = fdr->fdr_XMin +
		((t - fdr->fdr_VMin) / ( fdr->fdr_VMax - fdr->fdr_VMin ));

	if (fdr->fdr_Highlight)
	{
		SetFGPen( &GCon[0], MakeRGB15(MAX_RED, MAX_GREEN, 0) );
	}
	else
	{
		SetFGPen( &GCon[0], MakeRGB15( 0, MAX_GREEN, 0) );
	}
	DrawRect(fdr->fdr_XMin, fdr->fdr_YMin, xpos, fdr->fdr_YMax);

	if (fdr->fdr_Highlight)
	{
		SetFGPen( &GCon[0], MakeRGB15( 0, MAX_GREEN, MAX_BLUE) );
	}
	else
	{
		SetFGPen( &GCon[0], MakeRGB15( 0, 0, MAX_BLUE) );
	}
	DrawRect(xpos, fdr->fdr_YMin, fdr->fdr_XMax, fdr->fdr_YMax);

	MoveTo ( &GCon[0], fdr->fdr_XMax + 4, fdr->fdr_YMin );
	DrawNumber ( fdr->fdr_Value );

	return 0;
}

/********************************************************************/
int32 UpdateFader ( Fader *fdr , int32 val )
{
	return 0;
}

/********************************************************************/
int32 JoyToFader( Fader *fdr, uint32 joy )
{
	const int32 Incr = joy & ControlLeftShift
	                                                /* LShift - fine: use 5% of normal increment or range (whichever is smaller), but no smaller than 1 */
                     ? MAX (MIN (fdr->fdr_Increment, ABS (fdr->fdr_Value)) / 20, 1)
                     : fdr->fdr_Increment;          /* normal - coarse: normal increment */
    int32 newvalue = fdr->fdr_Value;

  #if DEBUG_JoyToFader
	printf ("%s: incr=%ld val=%ld->", fdr->fdr_Name, Incr, fdr->fdr_Value);
  #endif

    if (joy & ControlLeft)  newvalue -= Incr;
    if (joy & ControlRight) newvalue += Incr;

    fdr->fdr_Value = CLIPRANGE (newvalue, fdr->fdr_VMin, fdr->fdr_VMax);

  #if DEBUG_JoyToFader
	printf ("%ld\n", fdr->fdr_Value);
  #endif

	return 0;
}


/* -------------------- FaderBlock */

/********************************************************************/
int32 DriveFaders ( FaderBlock *fdbl, uint32 joy )
{
	int32 Result = 0;

    if (fdbl->fdbl_NumFaders) {
        int32 NewFader = 0;

        if (joy & (ControlRight|ControlLeft))
        {
            Fader * const fader = &fdbl->fdbl_Faders [ fdbl->fdbl_Current ];
            const int32 oldval = fader->fdr_Value;

            JoyToFader (fader, joy);
            if (fader->fdr_Value != oldval && fdbl->fdbl_Function)
                (*fdbl->fdbl_Function) (fdbl->fdbl_Current, fader->fdr_Value, fdbl);
        }
        else if (joy & ControlDown)
        {
            if (fdbl->fdbl_OneShot == 0)
            {
                if (fdbl->fdbl_Current < fdbl->fdbl_NumFaders-1 )
                {
                    fdbl->fdbl_Current += 1;
                    NewFader = 1;
                }
                fdbl->fdbl_OneShot = 1;
            }
        }
        else if (joy & ControlUp)
        {
            if (fdbl->fdbl_OneShot == 0)
            {
                if (fdbl->fdbl_Current > 0 )
                {
                    fdbl->fdbl_Current -= 1;
                    NewFader = 1;
                }
                fdbl->fdbl_OneShot = 1;
            }
        }

        if (joy& JOYFIREA)
        {
        }

        if (joy == 0)
        {
            fdbl->fdbl_OneShot = 0;
        }

        if (NewFader)
        {
            fdbl->fdbl_Faders[fdbl->fdbl_Previous].fdr_Highlight = 0;
            DrawFader ( &fdbl->fdbl_Faders[fdbl->fdbl_Previous] );
            ToggleScreen();
            DrawFader ( &fdbl->fdbl_Faders[fdbl->fdbl_Previous] );
            ToggleScreen();
            fdbl->fdbl_Faders[fdbl->fdbl_Current].fdr_Highlight = 1;
            fdbl->fdbl_Previous = fdbl->fdbl_Current;
        }

        Result = DrawFader ( &fdbl->fdbl_Faders[fdbl->fdbl_Current] );
    }

	return Result;
}

/********************************************************************/
/*
    Init FaderBlock and Faders.  Harmless if NumFaders == 0.
*/
int32 InitFaderBlock ( FaderBlock *fdbl, Fader *Faders, int32 NumFaders, FaderEventFunctionP eventfn )
{
	memset (fdbl, 0, sizeof *fdbl);
	fdbl->fdbl_NumFaders = NumFaders;
	fdbl->fdbl_Function  = eventfn;

        /* only set this stuff if NumFaders != 0 */
    if (NumFaders) {
        int32 i;

            /* link in faders array */
        fdbl->fdbl_Faders = Faders;

            /* init faders array */
        for (i=0; i<NumFaders; i++) InitFader ( &Faders[i], i );
        fdbl->fdbl_Faders[fdbl->fdbl_Current].fdr_Highlight = 1;
    }

	return 0;
}

/********************************************************************/
/*
    Draw all Faders in a FaderBlock
*/
Err DrawFaderBlock (const FaderBlock *faderblock)
{
    const Fader *fader = faderblock->fdbl_Faders;
    int32 nfaders      = faderblock->fdbl_NumFaders;

    while (nfaders--) DrawFader (fader++);

    return 0;
}
