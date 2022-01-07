
/******************************************************************************
**
**  $Id: pd_faders.c,v 1.17 1994/10/06 19:20:01 peabody Exp $
**
**  patchdemo.arm gui module.  See patchdemo.c for documentation.
**
******************************************************************************/

    /* local */
#include "patchdemo.h"

    /* audiodemo */
#include "audiodemo.h"      /* macros */
#include "faders.h"
#include "graphic_tools.h"  /* rendering */

    /* portfolio */
#include <stdio.h>          /* sprintf() */
#include <event.h>          /* Init/KillEventUtility(), GetControlPadEvent() */


/* -------------------- Debugging */

#define DEBUG_Entry     0       /* entry/exit */
#define DEBUG_Events    0       /* FaderHandler() */


/* -------------------- Conditional */

#define PAGE_IncludeEmptyPages  0       /* when enabled, includes empty pages in the patch display */


/* -------------------- Local functions */

static int32 FaderHandler (int32 ifader, int32 newvalue, FaderBlock *);


/* -------------------- New/DeletePatchPageList() */

/*
    Create PatchPageList for Patch.

    Can return w/ an empty List.
    Currently doesn't add pages w/o any faders.  Thus can return fewer pages than instruments.
*/
PatchPageList *NewPatchPageList (Patch *patch, Err *errbuf)
{
    PatchPageList *pagelist = NULL;
    Err errcode = 0;

  #if DEBUG_Entry
    printf ("NewPatchPageList() patch=$%p ", patch); printavail(); printf ("\n");
  #endif

        /* Allocate and initialize PatchPageList */
    if ((pagelist = (PatchPageList *)myalloc (sizeof *pagelist)) == NULL) {
        errcode = -1;       /* !!! real error code */
        goto clean;
    }
    pagelist->ppage_Patch = patch;
    InitList (&pagelist->ppage_PageList, "Instrument Pages");

        /* Create a PatchPage for every PatchInstrument in Patch, and add them to PatchPageList */
    {
        PatchInstrument *inst;

        for (inst = (PatchInstrument *)FirstNode (&patch->patch_InstrumentList); IsNode(&patch->patch_InstrumentList,inst); inst = (PatchInstrument *)NextNode (inst)) {
            PatchPage *page;

            if ((page = NewPatchPage (inst, &errcode)) == NULL) goto clean;

                /* add page to pagelist if there are some knobs on it, otherwise, delete page
                   (depending on PAGE_IncludeEmptyPages setting) */
          #if !PAGE_IncludeEmptyPages
            if (!page->ppage_FaderBlock.fdbl_NumFaders)
                DeletePatchPage (page);
            else
          #endif
                AddTail (&pagelist->ppage_PageList, (Node *)page);
        }
    }

clean:
    if (errcode < 0) {
        CloseRes (DeletePatchPageList, pagelist);
        if (errbuf) *errbuf = errcode;
    }

  #if DEBUG_Entry
    printf ("  pagelist=$%p err=%ld ", pagelist, errcode); printavail(); printf ("\n");
  #endif

    return pagelist;
}

/*
    Delete PatchPageList created by NewPatchPageList()
*/
Err DeletePatchPageList (PatchPageList *pagelist)
{
  #if DEBUG_Entry
    printf ("DeletePatchPageList() pagelist=$%p ", pagelist); printavail(); printf ("\n");
  #endif

    if (pagelist) {

            /* delete attached PatchPages */
        {
            struct PatchPage *page;

            while ((page = (PatchPage *)RemTail (&pagelist->ppage_PageList)) != NULL) DeletePatchPage (page);
        }

            /* free PatchPageList */
        myfree (pagelist);
    }

  #if DEBUG_Entry
    printf ("  "); printavail(); printf ("\n");
  #endif

    return 0;
}


/* -------------------- New/DeletePatchPage() */

/*
    Create a new PatchPage for PatchInstrument.

    Sets fdr_UserItem of each Fader to the Knob item # of the associated knob.
*/
PatchPage *NewPatchPage (PatchInstrument *inst, Err *errbuf)
{
    PatchPage *page = NULL;
    Err errcode = 0;

  #if DEBUG_Entry
    printf ("NewPatchPage() inst=\"%s\" ", inst->pinst_Symbol.psym_Node.n_Name); printavail(); printf ("\n");
  #endif

        /* Allocate and initialize PatchPage and Fader array */
    {
        const int32 nfaders = GetNGrabbedPatchKnobs (inst);

      #if DEBUG_Entry
        printf ("  nfaders=%ld\n", nfaders);
      #endif
        if ((page = (PatchPage *)myalloc (sizeof *page + nfaders * sizeof (Fader))) == NULL) {
            errcode = -1;       /* !!! real error code */
            goto clean;
        }
        page->ppage_Node.n_Size = sizeof *page;
        page->ppage_Instrument = inst;
        InitFaderBlock (&page->ppage_FaderBlock, (Fader *)(page + 1), nfaders, FaderHandler);
    }

        /* Bind faders to knobs */
    {
        PatchKnob *knob = inst->pinst_KnobTable;
        int32 nknobs    = inst->pinst_NKnobs;
        Fader *fader    = page->ppage_FaderBlock.fdbl_Faders;

        for (; nknobs--; knob++) if (IsPatchKnobGrabbed (knob)) {
            TagArg getknobtags[] = {    /* @@@ order is assumed below */
                { AF_TAG_MIN },
                { AF_TAG_MAX },
                { AF_TAG_CURRENT },
                { TAG_END }
            };

                /* get knob attributes */
            if ((errcode = GetAudioItemInfo (knob->pknob_Knob, getknobtags)) < 0) goto clean;

                /* fill out Fader from PatchKnob */
            fader->fdr_Name      = knob->pknob_Name;
            fader->fdr_UserItem  = knob->pknob_Knob;
            fader->fdr_VMin      = (int32) getknobtags[0].ta_Arg;
            fader->fdr_VMax      = (int32) getknobtags[1].ta_Arg;
            fader->fdr_Value     = (int32) getknobtags[2].ta_Arg;
            fader->fdr_Increment = MAX (uceil (fader->fdr_VMax - fader->fdr_VMin, 100), 1);

                /* recompute fader position (!!! inherited from ta_faders.c - do this more gracefully) */
            fader->fdr_YMin = FADER_YMIN + ((int32)(fader - page->ppage_FaderBlock.fdbl_Faders) * FADER_SPACING) + 15;
            fader->fdr_YMax = fader->fdr_YMin + FADER_HEIGHT;

                /* increment fader pointer */
            fader++;
        }
    }

clean:
    if (errcode < 0) {
        CloseRes (DeletePatchPage, page);
        if (errbuf) *errbuf = errcode;
    }

  #if DEBUG_Entry
    printf ("  page=$%p err=%ld ", page, errcode); printavail(); printf ("\n");
  #endif

    return page;
}


/*
    Delete PatchPage created by NewPatchPage().
*/
Err DeletePatchPage (PatchPage *page)
{
  #if DEBUG_Entry
    printf ("DeletePatchPage() page=$%p ", page); printavail(); printf ("\n");
  #endif

    myfree (page);

  #if DEBUG_Entry
    printf ("  "); printavail(); printf ("\n");
  #endif

    return 0;
}


/* -------------------- ShowPatchPage() */

/*
    Display PatchPage on screen.
*/
Err ShowPatchPage (PatchPage *page)
{
    int32 nscreens = NumScreens;
    char b[64];

  #if DEBUG_Entry
    printf ("ShowPatchPage() page=$%p inst=\"%s\"\n", page, page->ppage_Instrument->pinst_Symbol.psym_Node.n_Name);
    printf ("  NumScreens=%ld\n", NumScreens);
  #endif

        /* draw everything in each screen buffer */
    while (nscreens--) {

            /* clear screen */
        ClearScreen();

            /* draw text (!!! make this better) */
        MoveTo( &GCon[0], 20, TOP_VISIBLE_EDGE + 5 );
        DrawText8( &GCon[0], CURBITMAPITEM, "PatchDemo" );

        sprintf (b, "%s %s", page->ppage_Instrument->pinst_Symbol.psym_Node.n_Name, page->ppage_Instrument->pinst_TemplateName);
        MoveTo( &GCon[0], 20, TOP_VISIBLE_EDGE + 20 );
        DrawText8( &GCon[0], CURBITMAPITEM, b);

            /* !!! more */

            /* draw FaderBlock */
        DrawFaderBlock (&page->ppage_FaderBlock);

        SwitchScreens();
    }

  #if DEBUG_Entry
    printf ("  ScreenSelect=%ld\n", ScreenSelect);
  #endif

    return DisplayScreen (ScreenItems[ScreenSelect], 0);    
/* !!! should SwitchScreens() even if there's no real switch to be done? */
}


/* -------------------- FaderEventHandler() */

/*
    Tweak knob associated with Fader on value changes.
*/
static int32 FaderHandler (int32 ifader, int32 newvalue, FaderBlock *fdbl)
{
    const Fader *fader = &fdbl->fdbl_Faders[ifader];

  #if DEBUG_Events
    printf ("tweak \"%s\" %ld\n", fader->fdr_Name, newvalue);
  #endif

    return TweakRawKnob (fader->fdr_UserItem, newvalue);
}

/********************************************************************/
/******** Support for scope.   Move to new file.  !!!!!  ************/
/********************************************************************/


/***************************************************************
** support for capture of digital signals from DSPP
** Collect signals in a delay line for analysis or graphics display.
**************************************************************/
/**************************************************************/
Err DeleteScopeProbe( ScopeProbe *scpr )
{
	if( scpr == NULL ) return -1;

	DetachSample( scpr->scpr_Attachment );
	DeleteDelayLine( scpr->scpr_DelayLine );
	UnloadInstrument( scpr->scpr_Probe );
	FreeMem( scpr, sizeof(ScopeProbe) );
	return 0;
}

/**************************************************************/
ScopeProbe *CreateScopeProbe( int32 NumBytes )
{
	ScopeProbe *scpr;
	int32 Result;
	TagArg Tags[2];
	int32 Time0, Time1;

	scpr = AllocMem( sizeof(ScopeProbe), MEMTYPE_FILL );
	if( scpr == NULL ) return NULL;

/* Create Probe Instrument */
/* Use low priority to get final output. */
	scpr->scpr_Probe = LoadInstrument( "delaymono.dsp", 0, 100 );
	CHECKRESULT(scpr->scpr_Probe,"LoadInstrument delaymono");

/* Create DelayLine */
printf("Create probe %d bytes long\n", NumBytes );
	scpr->scpr_DelayLine = CreateDelayLine( NumBytes, 1, FALSE );
	CHECKRESULT(scpr->scpr_DelayLine,"CreateDelayLine");
	scpr->scpr_Size = NumBytes;
        Tags[0].ta_Tag = AF_TAG_ADDRESS;
        Tags[0].ta_Arg = NULL;
        Tags[1].ta_Tag = TAG_END;
        Result = GetAudioItemInfo(scpr->scpr_DelayLine, Tags);
        CHECKRESULT(Result,"GetAudioItemInfo");
        scpr->scpr_Data = (char *) Tags[0].ta_Arg;  

/* Attach delay line. */
	scpr->scpr_Attachment = AttachSample(scpr->scpr_Probe,
		scpr->scpr_DelayLine, NULL );
	CHECKRESULT(scpr->scpr_Attachment,"AttachSample");
	printf("scpr->scpr_Attachment = 0x%x\n",
		scpr->scpr_Attachment );

/* Make CUE for monitoring delay line. */
	scpr->scpr_Cue = CreateCue( NULL );
	CHECKRESULT(scpr->scpr_Cue,"CreateCue");
	scpr->scpr_Signal = GetCueSignal( scpr->scpr_Cue );

/* MonitorAttachment() for Delays is broken until Portfolio 2.2 */
// #define USE_MONITOR_ATTACHMENT
#ifdef USE_MONITOR_ATTACHMENT
/* Why does this cause the delay line to stop prematurely? */
/* Because of a bug in audiofolio that was fixed on 940810 and
** released in Portfolio 2.2 */
	Result = MonitorAttachment( scpr->scpr_Attachment, scpr->scpr_Cue, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");
#endif

	Result = StartInstrument( scpr->scpr_Probe, NULL );
	CHECKRESULT(Result,"StartInstrument");

#ifdef USE_MONITOR_ATTACHMENT
/* Wait for signal to be returned. */
	WaitSignal( scpr->scpr_Signal );
#endif

	return scpr;
cleanup:
	DeleteScopeProbe( scpr );
	return NULL;
}


/**************************************************************/
Err CaptureScopeBuffer( ScopeProbe *scpr, int32 XShift)
{
	int32 Result;

/* Start Instrument now. */
	Result = StartAttachment( scpr->scpr_Attachment, NULL );
	CHECKRESULT(Result,"StartInstrument");

#ifdef USE_MONITOR_ATTACHMENT
/* Wait for signal to be returned. */
	WaitSignal( scpr->scpr_Signal );
#else
	SleepAudioTicks( (SCOPE_MAX_SHOW << XShift) / 180 );
#endif

cleanup:
	return Result;
}


/**************************************************************/
#define SCOPE_LEFT_EDGE  (30)
#define SCOPE_Y_AXIS  (120)
Err DisplayScopeBuffer( ScopeProbe *scpr, int32 XOffset, int32 XShift, int32 YShift )
{
	int32 i, NumPnts;
	int16 *Data16Ptr;
	int32 Data32;
	int32 x,y;
	int32 si, j, SamplesPerPixel;
	int32 MinVal, MaxVal;

	ToggleScreen();
	ClearScreen();

	NumPnts =  256;

	SetFGPen( &GCon[0], MakeRGB15(31, 31, 0) );

	MoveTo( &GCon[0], SCOPE_LEFT_EDGE, SCOPE_Y_AXIS );
	Data16Ptr = scpr->scpr_Data;

	SamplesPerPixel = 1<<XShift;

/* Display samples on screen. */
	for( i=0; i<NumPnts; i++ )
	{
		x = i + SCOPE_LEFT_EDGE;
		si = i<<XShift;
		if( XShift > 0 )
		{
/* Scan for extreme values.  Always span or include zero. */
			MinVal = 0;
			MaxVal = 0;
			for( j = 0; j<SamplesPerPixel; j++ )
			{
				Data32 = Data16Ptr[j+si+XOffset];
				if( Data32 < MinVal ) MinVal = Data32;
				if( Data32 > MaxVal ) MaxVal = Data32;
			}
/* Scale Y to fit on screen. */
			y = SCOPE_Y_AXIS - (MinVal>>YShift);
			MoveTo( &GCon[0], x, y );
			y = SCOPE_Y_AXIS - (MaxVal>>YShift);
			DrawTo( CURBITMAPITEM,  &GCon[0], x, y );
		}
		else
		{
/* Draw line between each sample. */
			Data32 = Data16Ptr[si+XOffset];
			y = SCOPE_Y_AXIS - (Data32>>YShift);
			DrawTo( CURBITMAPITEM,  &GCon[0], x, y );
		}
	}

    	return DisplayScreen (ScreenItems[ScreenSelect], 0);    
}

/**************************************************************/
static Err ConnectScopePage( ScopeProbe *scpr, PatchPage *Page )
{
	int32 Result;
	Item SrcInsItem, DstInsItem;

	SrcInsItem = Page->ppage_Instrument->pinst_Instrument;
	DstInsItem = scpr->scpr_Probe;

/* Try several connections. */
	Result = ConnectInstruments( SrcInsItem, "Output",
			DstInsItem, "Input" );
	if( Result == 0 )return Result;

	Result = ConnectInstruments( SrcInsItem, "LeftOutput",
			DstInsItem, "Input" );

	return Result;
}

/**************************************************************/
/* Enter digital scope interactive mode.
**
**   Up/Down = change Y scale
**   Left/Right = pan left and right
**   Shift Left/Right = change X scale
**   A = acquire
**   C = return to faders.
*/
Err DoScope( ScopeProbe *scpr, PatchPage *Page )
{
	int32 Result;
	int32 XShift = 0, YShift = 9, XOffset = 0;
	int32 DoIt;
	ControlPadEventData cped;
 	uint32 CurButtons, LastButtons = 0;
	int32 IfWaitButton = TRUE;

	Result = ConnectScopePage( scpr, Page );
	CHECKRESULT( Result, "ConnectScopePage" );

	Result = CaptureScopeBuffer( scpr, XShift );
	CHECKRESULT( Result, "CaptureScopeBuffer" );

	DoIt = TRUE;
	do
	{

		Result = DisplayScopeBuffer( scpr, XOffset, XShift, YShift );
		CHECKRESULT( Result, "DisplayScopeBuffer" );
	
		if ((Result = GetControlPad (1, IfWaitButton, &cped)) < 0) goto cleanup;
		CurButtons = cped.cped_ButtonBits;

		IfWaitButton = TRUE;

		if( ControlC & CurButtons & ~LastButtons )
		{
			DoIt = FALSE;
			break;
		}
		if( ControlX & CurButtons & ~LastButtons )
		{
			DoIt = FALSE;
			break;
		}

/* Set Vertical Gain. */
		if( ControlDown & CurButtons & ~LastButtons )
		{
			YShift++;
			if(YShift > 15) YShift = 15;
		}
		if( ControlUp & CurButtons & ~LastButtons )
		{
			YShift--;
			if(YShift < 1) YShift = 1;
		}
/* Set Time Axis. */
		if( ControlRightShift & CurButtons & ~LastButtons )
		{
			XShift--;
			if(XShift < 0) XShift = 0;
		}
		if( ControlLeftShift & CurButtons & ~LastButtons )
		{
			XShift++;
			if(XShift >  SCOPE_MAX_SHIFT ) XShift =  SCOPE_MAX_SHIFT;
		}

/* Pan left or right continuously. */
		if( ControlRight & CurButtons )
		{
			XOffset += 8 << XShift;
			if(XOffset > (SCOPE_MAX_SAMPLES - (SCOPE_MAX_SHOW<<XShift)))
				XOffset = SCOPE_MAX_SAMPLES - (SCOPE_MAX_SHOW<<XShift);
			IfWaitButton = FALSE;
		}
		if( ControlLeft & CurButtons )
		{
			XOffset -= 8 << XShift;
			if(XOffset < 0) XOffset = 0;
			IfWaitButton = FALSE;
		}

/* Acquire and display while A held down. */
		if( ControlA & CurButtons & ~LastButtons )
		{
			
			do
			{
				XOffset = 0;
				Result = CaptureScopeBuffer( scpr, XShift );
				CHECKRESULT( Result, "CaptureScopeBuffer" );

				Result = DisplayScopeBuffer( scpr, XOffset, XShift, YShift );
				CHECKRESULT( Result, "DisplayScopeBuffer" );

				if ((Result = GetControlPad (1, FALSE, &cped)) < 0) goto cleanup;
			}
			while( cped.cped_ButtonBits & ControlA );
		}

		LastButtons = cped.cped_ButtonBits;

	} while( DoIt );
				
/* Clear buttons by waiting for all buttons up. */
	while( cped.cped_ButtonBits )
	{
		if ((Result = GetControlPad (1, TRUE, &cped)) < 0) goto cleanup;
	}

cleanup:
	return Result;
}
