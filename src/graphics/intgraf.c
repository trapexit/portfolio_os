/* *************************************************************************
 *
 * Graphics routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * $Id: intgraf.c,v 1.67 1994/12/13 08:19:02 ewhac Exp $
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 940223 ewhac            Tore out CreateBitmap and put it into bitmap.c
 * 930830 SHL              Split CreateBitmap out of CreateScreenGroup
 * 930706 SHL              Commented out all pre-red support
 * 930617 SHL              changed all GEnable/GDisable to Enable/Disable
 * 930301 -RJ              Moved MSYSBits bit setting to MCTL
 * 920909 -RJ              Incorporate recent changes from Dale & Stephen
 * 920724 -RJ Mical        Start overhaul
 * 920717 Stephen Landrum  Last edits before July handoff
 *
 * ********************************************************************** */


#define SSSDBUG(x) /* Superkprintf x */
#define DBGX(x)  /* Superkprintf x */


/***************************************************************\
* Header files
\***************************************************************/


#include "types.h"

#include "mem.h"

#include "debug.h"
#include "item.h"
#include "nodes.h"
#include "interrupts.h"
#include "kernel.h"
#include "list.h"
#include "task.h"
#include "folio.h"
#include "kernelnodes.h"
#include "semaphore.h"
#include "super.h"
#include "timer.h"

#include "intgraf.h"

#include "stdarg.h"
#include "strings.h"
#include "stdio.h"

#include "inthard.h"
#include "clio.h"

#include "overlay.h"
#include "vbl.h"


/***************************************************************\
* Data & necessary structures
\***************************************************************/



void *(*GrafSWIFuncs[])() = {
  (void *(*)())GetVBLAttrs,	/* 53 */
  (void *(*)())SetVBLAttrs,	/* 52 */
  (void *(*)())ModifyVDL,	/* 51 */
  (void *(*)())realCreateScreenGroup,	/* 50 */
  (void *(*)())NULROUTINE,	/* 49 */
/*  (void *(*)())SubmitVDL, */	/* 48 */
  (void *(*)())NULROUTINE,	/* 48 */
  (void *(*)())SetVDL,		/* 47 */
/*  (void *(*)())DeleteVDL, */	/* 46 */
  (void *(*)())NULROUTINE,	/* 46 */
  (void *(*)())DisplayScreen,	/* 45 */
/*  (void *(*)())DeleteScreenGroup, */	/* 44 */
  (void *(*)())NULROUTINE,	/* 44 */
  (void *(*)())NULROUTINE,	/* 43 */
/*  (void *(*)())CopyRect, */	/* 42 */
  (void *(*)())SetCEWatchDog,	/* 42 */
  (void *(*)())SetCEControl,	/* 41 */
  (void *(*)())NULROUTINE,	/* 40 */
  (void *(*)())SWIDrawCels,	/* 39 */
  (void *(*)())DrawText8,	/* 38 */
  (void *(*)())GetCurrentFont,	/* 37 */
  (void *(*)())SetCurrentFontCCB,	/* 36 */
  (void *(*)())FillRect,	/* 35 */
  (void *(*)())NULROUTINE,	/* 34 */
  (void *(*)())DrawTo,		/* 33 */
  (void *(*)())NULROUTINE,	/* 32 */
  (void *(*)())DrawChar,	/* 31 */

/*  (void *(*)())swiSuperCloseFont, */	/* 30 */
  (void *(*)())NULROUTINE,	/* 30 */
  (void *(*)())DrawText16,	/* 29 */

/*???   (void *(*)())OpenFileFont, */	/* 28 */
  (void *(*)())NULROUTINE,	/* 28 */

  (void *(*)())swiSuperOpenRAMFont,	/* 27 */
/*  (void *(*)())SetFileFontCacheSize, */	/* 26 */
  (void *(*)())NULROUTINE,	/* 26 */

/*  (void *(*)())swiSuperOpenFileFont, */	/* 25 */
  (void *(*)())NULROUTINE,	/* 25 */
/*  (void *(*)())FillEllipse, */	/* 24 */
  (void *(*)())NULROUTINE,	/* 24 */
  (void *(*)())SWIDrawScreenCels,	/* 23 */
  (void *(*)())NULROUTINE,	/* 22 */
  (void *(*)())NULROUTINE,		/* 21 */
  (void *(*)())SetClipHeight,	/* 20 */
  (void *(*)())SetClipWidth,	/* 19 */
  (void *(*)())RemoveScreenGroup,	/* 18 */
  (void *(*)())AddScreenGroup,	/* 17 */
  (void *(*)())NULROUTINE,	/* 16 */
  (void *(*)())NULROUTINE,	/* 15 */
  (void *(*)())swiSuperResetCurrentFont,	/* 14 */
  (void *(*)())SetScreenColors,	/* 13 */
  (void *(*)())NULROUTINE,	/* 12 */
/*  (void *(*)())ResetSystemGraphics, */	/* 11 */
  (void *(*)())NULROUTINE,	/* 11 */
  (void *(*)())ResetScreenColors,	/* 10 */
  (void *(*)())SetScreenColor,	/* 9 */
  (void *(*)())DisableHAVG,	/* 8 */
  (void *(*)())EnableHAVG,	/* 7 */
  (void *(*)())DisableVAVG,	/* 6 */
  (void *(*)())EnableVAVG,	/* 5 */
/*  (void *(*)())WaitForLine, */	/* 4 DEFUNCT. 5-10-93 JCR */
  (void *(*)())NULROUTINE,	/* 4 */
  (void *(*)())SetClipOrigin,	/* 3 */
  (void *(*)())ResetReadAddress,	/* 2 */
  (void *(*)())SetReadAddress,	/* 1 */
/*  (void *(*)())GrafInit, */	/* 0 */
  (void *(*)())NULROUTINE,	/* 0 */
};



void *(*GrafUserFuncs[])() = {
  (void *(*)())internalGetVBLAttrs,	/* -1 */

  /* Front end patches for kludgy routines that should be rewritten */
  (void *(*)())NULROUTINE,	/* -53 */
  (void *(*)())kGetVBLAttrs,	/* -52 */
  (void *(*)())kSetVBLAttrs,	/* -51 */
  (void *(*)())DisplayOverlay,	/* -50 */
  (void *(*)())QueryGraphicsList,	/* -49 */
  (void *(*)())QueryGraphics,	/* -48 */
  (void *(*)())kModifyVDL,	/* -47 */
  (void *(*)())kGetFirstDisplayInfo,	/* -46 */
  (void *(*)())kSetCEWatchDog,	/* -45 */
  (void *(*)())kDrawScreenCels,	/* -44 */
  (void *(*)())kDrawCels,	/* -43 */
  (void *(*)())kSubmitVDL,	/* -42 */
  (void *(*)())kSetVDL,	/* -41 */
  (void *(*)())kDisplayScreen,	/* -40 */
  (void *(*)())NULROUTINE,	/* -39 */
  (void *(*)())kSetCEControl,	/* -38 */
  (void *(*)())kDrawText8,	/* -37 */
  (void *(*)())kGetCurrentFont,	/* -36 */
  (void *(*)())kSetCurrentFontCCB,	/* -35 */
  (void *(*)())kFillRect,	/* -34 */
  (void *(*)())kDrawTo,	/* -33 */
  (void *(*)())kDrawChar,	/* -32 */
  (void *(*)())NULROUTINE,	/* -31 */
  (void *(*)())MoveTo,	/* -30 */
  (void *(*)())kSetClipHeight,	/* -29 */
  (void *(*)())kSetClipWidth,	/* -28 */
  (void *(*)())kRemoveScreenGroup,	/* -27 */
  (void *(*)())kAddScreenGroup,	/* -26 */
  (void *(*)())SetBGPen,	/* -25 */
  (void *(*)())SetFGPen,	/* -24 */
  (void *(*)())DeleteScreenGroup,	/* -23 */
  (void *(*)())kSetScreenColors,	/* -22 */
  (void *(*)())kResetScreenColors,	/* -21 */
  (void *(*)())kSetScreenColor,	/* -20 */
  (void *(*)())kDisableHAVG,	/* -19 */
  (void *(*)())kEnableHAVG,	/* -18 */
  (void *(*)())kDisableVAVG,	/* -17 */
  (void *(*)())kEnableVAVG,	/* -16 */
  (void *(*)())kSetClipOrigin,	/* -15 */
  (void *(*)())kResetReadAddress,	/* -14 */
  (void *(*)())kSetReadAddress,	/* -13 */

  /* User functions for graphics folio */
  (void *(*)())CreateScreenGroup,	/* -12 */
  (void *(*)())ResetCurrentFont,	/* -11 */
/*  (void *(*)())CloseFont, */	/* -10 */
  (void *(*)())NULROUTINE,	/* -10 */

  (void *(*)())kDrawText16,	/* -9 */

/*  (void *(*)())OpenFileFont, */	/* -8 */
  (void *(*)())NULROUTINE,	/* -8 */
/*  (void *(*)())OpenRAMFont, */	/* -7 */
  (void *(*)())NULROUTINE,	/* -7 */

/*  (void *(*)())kSetFileFontCacheSize, */	/* -6 */
  (void *(*)())NULROUTINE,	/* -6 */

  (void *(*)())WritePixel,	/* -5 */
  (void *(*)())GetPixelAddress,	/* -4 */
  (void *(*)())ReadCLUTColor,	/* -3 */
  (void *(*)())ReadPixel,	/* -2 */
  (void *(*)())MapCel,	/* -1 */
};

#define NUM_GRAFSWIFUNCS (sizeof(GrafSWIFuncs)/sizeof(void *))
#define NUM_GRAFUSERFUNCS (sizeof(GrafUserFuncs)/sizeof(void *))

struct NodeData GrafNodeData[] = {
	{ 0, 0 },
	{ sizeof(ScreenGroup),	NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(Screen),	NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(Bitmap),	NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(VDL),		NODE_ITEMVALID | NODE_SIZELOCKED },
	{ sizeof(DisplayInfo),	0 },
	{ sizeof(Overlay),	NODE_ITEMVALID | NODE_SIZELOCKED },
};

#define GRAFNODECOUNT (sizeof(GrafNodeData)/sizeof(NodeData))


TagArg GrafFolioTags[] = {
	/* size of graphics folio */
	{ CREATEFOLIO_TAG_DATASIZE,	(void *) sizeof (GrafFolio)	},
	/* number of SWI functions */
	{ CREATEFOLIO_TAG_NSWIS,	(void *) NUM_GRAFSWIFUNCS	},
	/* number of user functions */
	{ CREATEFOLIO_TAG_NUSERVECS,	(void *) NUM_GRAFUSERFUNCS	},
	/* list of swi functions */
	{ CREATEFOLIO_TAG_SWIS,		(void *) GrafSWIFuncs		},
	/* list of user functions */
	{ CREATEFOLIO_TAG_USERFUNCS,	(void *) GrafUserFuncs		},
	/* name of graphics folio */
	{ TAG_ITEM_NAME,		(void *) "Graphics"		},
	/* initialization code */
	{ CREATEFOLIO_TAG_INIT,		(void *) ((long) InitGrafBase)	},
	/* we have to be item #1 */
	{ CREATEFOLIO_TAG_ITEM,		(void *) NST_GRAPHICS		},
	/* for lack of a better value */
	{ TAG_ITEM_PRI,			(void *) 0			},
	/* Graphics node database */
	{ CREATEFOLIO_TAG_NODEDATABASE,	(void *) GrafNodeData		},
	/* number of nodes */
	{ CREATEFOLIO_TAG_MAXNODETYPE,	(void *) GRAFNODECOUNT		},
	/* end of tag list */
	{ 0,				(void *) 0			},
};

extern uint32	graphicsFirq (void);
extern Item	internalCreateBitmap (struct Bitmap *, struct TagArg *);

extern Item	internalCreateOverlay (struct Overlay *, struct TagArg *);
extern Item	internalOpenOverlay (struct Overlay *, struct TagArg *);
extern Err	internalCloseOverlay (struct Overlay *, struct Task *);
extern Err	internalDeleteOverlay (struct Overlay *, struct Task *);
extern void	TakedownOverlay (struct Overlay *);
extern void	setundercolor (struct Overlay *, struct VDL *);

Err		internalSetOwnerGrafItem (ItemNode *it, Item newOwner,
					  Task *oldOwner);

static struct DisplayInfo *allocDInode (TagArg *ditags, int32 size);


TagArg CelSemaphoreTags[] =
{
	TAG_ITEM_NAME,	(void *)"Cel Engine",
	TAG_ITEM_END,	(void *)0,
};

TagArg TimeoutTimerTags[] = {
	CREATETIMER_TAG_NUM,	(void *)2,
	0, 0,
};


List ScreenGroupList, ScreenList;


extern bool isUser(void);


TagArg _displayInfoNTSC[] = {
	{ DI_TYPE,	(void*)DI_TYPE_NTSC },
	{ DI_WIDTH,	(void*)320 },
	{ DI_HEIGHT,	(void*)240 },
	{ DI_FIELDTIME, (void*)16684 },
	{ DI_FIELDFREQ, (void*)60 },
	{ DI_ASPECT,	(void*)0x00010000 },
	{ DI_ASPECTW,	(void*)1 },
	{ DI_ASPECTH,	(void*)1 },
	{ DI_NAME,	(void*)"NTSC" },
	{ DI_HDELAY,	(void*)0x000000c6 },
	{ DI_MCTL,	(void*)0 },
	{ DI_DISPCTRL,	(void*)0 },
	{ DI_END,	0 },
};

TagArg _displayInfoPAL1[] = {
	{ DI_TYPE,	(void*)DI_TYPE_PAL1 },
	{ DI_WIDTH,	(void*)320 },
	{ DI_HEIGHT,	(void*)288 },
	{ DI_FIELDTIME, (void*)20000 },
	{ DI_FIELDFREQ, (void*)50 },
	{ DI_ASPECT,	(void*)0x00010000 },
	{ DI_ASPECTW,	(void*)1 },
	{ DI_ASPECTH,	(void*)1 },
	{ DI_NAME,	(void*)"PAL (Narrow)" },
	{ DI_HDELAY,	(void*)0x000000182 },
	{ DI_MCTL,	(void*)0 },
	{ DI_DISPCTRL,	(void*)0 },
	{ DI_END,	0 },
};

TagArg _displayInfoPAL2[] = {
	{ DI_TYPE,	(void*)DI_TYPE_PAL2 },
	{ DI_WIDTH,	(void*)384 },
	{ DI_HEIGHT,	(void*)288 },
	{ DI_FIELDTIME, (void*)20000 },
	{ DI_FIELDFREQ, (void*)50 },
	{ DI_ASPECT,	(void*)0x00010000 },
	{ DI_ASPECTW,	(void*)1 },
	{ DI_ASPECTH,	(void*)1 },
	{ DI_NAME,	(void*)"PAL" },
	{ DI_HDELAY,	(void*)0x000000102 },
	{ DI_MCTL,	(void*)SC384EN },
	{ DI_DISPCTRL,	(void*)VDL_PALSEL },
	{ DI_END,	0 },
};


int32 _DisplayHeight[] = {
  0,	/* Error condition, 0 is not a valid display type */
  240,	/* NTSC */
  288,	/* PAL1 */
  288,	/* PAL2 */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
};


int32 _DisplayWidth[] = {
  0,	/* Error condition, 0 is not a valid display type */
  320,	/* NTSC */
  320,	/* PAL1 */
  384,	/* PAL2 */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
};


int32 _DisplayMCTL[] = {
  0,	/* Error condition, 0 is not a valid display type */
  0,	/* NTSC */
  0,	/* PAL1 */
  SC384EN,	/* PAL2 */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
};


int32 _DisplayHDelay[] = {
  0,	/* Error condition, 0 is not a valid display type */
  0x0cc,	/* NTSC */
  0x182,	/* PAL1 */
  0x102,	/* PAL2 */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
};


int32 _DisplayDISPCTRL[] = {
  0,	/* Error condition, 0 is not a valid display type */
  0,	/* NTSC */
  0,	/* PAL1 */
  VDL_PALSEL,	/* PAL2 */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
};


int32 _DisplayOffset[] = {
  0,	/* Error condition, 0 is not a valid display type */
  0,	/* NTSC */
  1,	/* PAL1 */
  1,	/* PAL2 */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
  0,	/* Display types to be defined in the future... */
};


extern uint32 _tripaddress, _HDelayOverride;
extern uint32 *_PatchDISPCTRL1, *_PatchDISPCTRL2, *_PatchDISPCTRL3;
extern uint32 _suppress_MCTL;
extern uint32 SEEDv;
extern GrafFolio *GrafBase;
extern uint32 _firqstate;


/***************************************************************\
* Code
\***************************************************************/


#if (MODE==_MODE_developer)
void
printnotowner(Item it, Item t)
{
  if (isUser()) {
    printf ("Task %lx does not own item %lx\n", t, it);
  } else {
    Superkprintf ("Task %lx does not own item %lx\n", t, it);
  }
}
#endif

Item
NULROUTINE (void)
{
	return GRAFERR_NOTYET;
}


long
InitGrafBase (GrafFolio *gb)
{
  long retvalue;
  int32 i;

  SDEBUGGRAF (("Initializing Graphics folio\n"));

  GrafBase = gb;					/* Where am I located? */

  SDEBUGGRAF (("GrafBase = %lx\n", GrafBase));

  GrafBase->gf.f_ItemRoutines->ir_Delete = (internalDeleteGrafItem);
  GrafBase->gf.f_ItemRoutines->ir_Find = (internalFindGrafItem);
  GrafBase->gf.f_ItemRoutines->ir_Open = (internalOpenGrafItem);
  GrafBase->gf.f_ItemRoutines->ir_Close = (internalCloseGrafItem);
  GrafBase->gf.f_ItemRoutines->ir_Create = (internalCreateGrafItem);
  GrafBase->gf.f_ItemRoutines->ir_SetOwner = (internalSetOwnerGrafItem);

  GrafBase->gf_VBLNumber = 0;
  GrafBase->gf_VRAMPageSize = (ulong) GetPageSize (MEMTYPE_VRAM);

  /*???	GrafBase->gf_DefaultDisplayWidth = DISPLAY_WIDTH;*/
  GrafBase->gf_DefaultDisplayWidth = linewidth;
  GrafBase->gf_DefaultDisplayHeight = DISPLAY_HEIGHT;

  InitList (&GrafBase->gf_DisplayInfoList, "DisplayInfo list");

  SDEBUGGRAF (("Determine if we are in PAL mode\n"));
  i = SuperQuerySysInfo (SYSINFO_TAG_GRAPHDISPSUPP, NULL, 0);
  if (i & SYSINFO_PAL_DFLT) {
    Superkprintf ("PAL system detected\n");
    if (!_overrideflag) {
      palflag = true;
    }
  } else if (i & SYSINFO_NTSC_DFLT) {
    Superkprintf ("NTSC system detected\n");
    if (!_overrideflag) {
      palflag = false;
      pal2flag = false;
    }
  } else {
    Superkprintf ("Error - SuperQuerySysInfo returned %lx\n", i);
    retvalue = (-1);
    goto DONE;
  }

  SDEBUGGRAF (("Create DisplayInfo structure(s)\n"));
  if (palflag) {
    DisplayInfo *di;

    SDEBUGGRAF (("PAL\n"));
    if (!(di = allocDInode (_displayInfoPAL1, sizeof (_displayInfoPAL1)))) {
badDI:
      Superkprintf ("Error creating DisplayInfo structures\n");
      retvalue = -1;
      goto DONE;	/*  Look down.  */
    }

    AddTail (&GrafBase->gf_DisplayInfoList, (Node *) di);

    if (!(di = allocDInode (_displayInfoPAL2, sizeof (_displayInfoPAL2))))
      goto badDI;	/*  Look up.  */

    AddTail (&GrafBase->gf_DisplayInfoList, (Node *)di);

    if (pal2flag) {
      GrafBase->gf_DefaultDisplayType = DI_TYPE_PAL2;
    } else {
      GrafBase->gf_DefaultDisplayType = DI_TYPE_PAL1;
    }
    GrafBase->gf_DisplayTypeMask = (1<<DI_TYPE_PAL1) | (1<<DI_TYPE_PAL2);

  } else {
    DisplayInfo *di;

    SDEBUGGRAF (("NTSC\n"));
    if (!(di = allocDInode (_displayInfoNTSC, sizeof (_displayInfoNTSC))))
      goto badDI;	/*  Look up.  */

    AddTail (&GrafBase->gf_DisplayInfoList, (Node *)di);

    GrafBase->gf_DefaultDisplayType = DI_TYPE_NTSC;
    GrafBase->gf_DisplayTypeMask = (1<<DI_TYPE_NTSC);
  }

  if (palflag) {
    GrafBase->gf_VBLTime = 20000;	/* number of usec between VBLs */
  } else {
    GrafBase->gf_VBLTime = 16684;	/* number of usec between VBLs */
  }
  /* calculate approximate frequency of VBL in Hz */
  GrafBase->gf_VBLFreq = (1000000+GrafBase->gf_VBLTime/2)/GrafBase->gf_VBLTime;

  /*
   * Set HDELAY (once and for smegging all).
   * ### NOTE: This section should, but does not, update the HDELAY values
   * ### in the DisplayInfo structures.
   */
  if (SuperQuerySysInfo (SYSINFO_TAG_GRAPH_HDELAY, &i, sizeof (int32)) !=
      SYSINFO_HDELAY_SUPPORTED)
  {
    Superkprintf ("Whoopsie!  SysInfo not reporting HDELAY; using default.\n");
    _DisplayHDelay[DI_TYPE_PAL1] = _DisplayHDelay[DI_TYPE_NTSC] = *HDELAY;
  } else
    _DisplayHDelay[DI_TYPE_PAL1] = _DisplayHDelay[DI_TYPE_NTSC] = i;

  _DisplayHDelay[DI_TYPE_PAL2] = _DisplayHDelay[DI_TYPE_PAL1] - 0x80;


#if 0
  if (palflag) {	/* sequence to initialize the BT855 video encoder */
    *SLOWBUS = 0x00;
    *(SLOWBUS+6) = 0x00;
    *(SLOWBUS+6) = 0x0c;
    *(SLOWBUS+6) = 0xf0;
    *(SLOWBUS+6) = 0x82;
    *(SLOWBUS+6) = 0x68;
    *SLOWBUS = 0x06;
    *(SLOWBUS+6) = 0x67;
    *(SLOWBUS+6) = 0x02;
    *(SLOWBUS+6) = 0xcd;
    *(SLOWBUS+6) = 0x08;
    *SLOWBUS = 0x0c;
    *(SLOWBUS+6) = 0xb0;
    *(SLOWBUS+6) = 0x03;
    *HDELAY = 0x102;
  }
#endif


  /* Dale added some stuff here */
    {
      /* create a semaphore for access to the cel engine */
      retvalue = SuperCreateItem (MKNODEID (KERNELNODE, SEMA4NODE), CelSemaphoreTags);
      if (retvalue < 0)
	{
	  SDEBUG(("Unable to create Semaphore for Cel Engine\n"));
	  goto DONE;
	}
      GrafBase->gf_CelSemaphore = (Semaphore *)LookupItem(retvalue);

    }

  GrafBase->gf_Flags = 0;

  SDEBUGGRAF(("Alloc memory for VIRS and zero pages.\n"));
  GrafBase->gf_VIRSPage = SUPER_ALLOCMEM (2 * GrafBase->gf_VRAMPageSize,
					  MEMTYPE_VRAM | MEMTYPE_STARTPAGE);
  if (!GrafBase->gf_VIRSPage) {
    SDEBUG (("Unable to allocate VIRS/Zero pages.\n"));
    retvalue = GRAFERR_NOMEM;
    goto DONE;
  }
  memset (GrafBase->gf_VIRSPage, 0, 2 * GrafBase->gf_VRAMPageSize);

  {
    /*
     * Initialize VIRS page.
     */
    uint32 *p;
    p = (uint32 *) GrafBase->gf_VIRSPage + 17;	/* 35 */
    for (i = 148;  --i >= 0; )			/* 295 */
      *p++ = MakeRGB15Pair (1, 1, 1);

    for (i = 73;  --i >= 0; )			/* 147 */
      *p++ = MakeRGB15Pair (2, 2, 2);

  }

  GrafBase->gf_ZeroPage = (void *) ((int32) GrafBase->gf_VIRSPage +
  				    GrafBase->gf_VRAMPageSize);

  retvalue = BuildSystemVDLs ();
  if (retvalue < 0) goto DONE;

/*???	InitList( &ScreenGroupList, "ScreenGroupList" );*/
/*???	GrafBase->gf_ScreenGroupListPtr = &ScreenGroupList;*/
/*???	InitList( &ScreenList, "ScreenList" );*/
/*???	GrafBase->gf_ScreenListPtr = &ScreenList;*/

  {
    /*
     * Install graphics's VDLs into hardware.  Try not to kill it while
     * doing so.
     */
    uint32 intbits, line;

    DBGX (("MCTL = %08lx\n", *MCTL));

    intbits = Disable ();

#if 0
    /*  Deprecated.  */
    do {
      line = *VCNT & 0x7ff;
    } while (line > 15  ||  line < 7);	/* Wait for a safe line to kill display */
#endif

/*    *MCTL &= ~CLUTXEN | VSCTXEN; */	/* Disable VDL and Video DMA transfers */

    /*
     * Point hardware at *last* VDL entry so that hardware will expire it
     * naturally, rather than potentially overrunning the bottom of the
     * display.
     */
    *((volatile void **) CLUTMIDctl) = GrafBase->gf_VDLPostDisplay;

    *MCTL |= CLUTXEN | VSCTXEN;	/* re-renable VDL and Video DMA transfers */

    _firqstate++;

    _suppress_MCTL = 8;
    Enable (intbits);
  }

  {
    Item t;
    t = SuperCreateItem (MKNODEID (KERNELNODE, TIMERNODE), TimeoutTimerTags);
    if (t < 0) {
      retvalue = t;
      goto DONE;
    }
    GrafBase->gf_TimeoutTimer = (Timer *) LookupItem (t);
  }

  retvalue = InitFontStuff ();
  if (retvalue < 0) goto DONE;

  SDEBUGGRAF (("Returning from InitGrafBase\n"));

  retvalue = 0;

DONE:
  return( retvalue );
}


static struct DisplayInfo *
allocDInode (ditags, size)
TagArg	*ditags;
int32	size;
{
	DisplayInfo	*di;

	if (di = SUPER_ALLOCMEM (size + sizeof (Node), MEMTYPE_ANY))
		memcpy (di->di_Tags, ditags, size);

	return (di);
}


DisplayInfo *
kGetFirstDisplayInfo (void)
{
  return (DisplayInfo *)FIRSTNODE(&GrafBase->gf_DisplayInfoList);
}



int32
SetCEControl( Item bitmapItem, int32 controlWord, int32 controlMask )
{
  Bitmap *bitmap;

  SDEBUG(("SetCEControl( "));
  SDEBUG(("bitmapItem=$%lx ", (unsigned long)(bitmapItem)));
  SDEBUG(("controlWord=$%lx ", (unsigned long)(controlWord)));
  SDEBUG(("controlMask=$%lx ", (unsigned long)(controlMask)));
  SDEBUG((" )\n"));

  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }
  if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,bitmapItem)<0) {
      PRINTNOTOWNER (bitmap->bm.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

  bitmap->bm_CEControl = (bitmap->bm_CEControl & ~controlMask) | (controlWord & controlMask);

  return 0;
}



void *
GetPixelAddress (Item scrbitItem, Coord x, Coord y)
/*
 * Return the address of the specified pixel in the Bitmap.
 * A read outside the bitmap boundaries returns a value of NULL.
 */
{
  void			*retvalue;
  register Bitmap	*bitmap;

  retvalue = NULL;

  if (!(bitmap = (Bitmap *) CheckItem
  			     (scrbitItem, NODE_GRAPHICS, TYPE_BITMAP)))
  {
    register Screen	*screen;

    if (!(screen = (Screen *) CheckItem
			       (scrbitItem, NODE_GRAPHICS, TYPE_SCREEN)))
      goto DONE;	/*  Look down.  */

    bitmap = screen->scr_TempBitmap;
  }

  if (x < 0  ||  x >= (bitmap->bm_ClipWidth)  ||
      y < 0  ||  y >= (bitmap->bm_ClipHeight))
    goto DONE;	/*  Look down.  */

  retvalue = (void *) (bitmap->bm_Buffer +
		       (((y >> 1) * bitmap->bm_Width) << 2) +
		       ((y & 1) << 1) +
		       (x << 2));

DONE:
  return (retvalue);
}



int32
SWIDrawScreenCels (Item screenItem, CCB *ccb)
/*
 * Draw cels into the display, following the CCB chain
 */
{
  Screen *screen;

  SDEBUG(("DrawScreenCels( "));
  SDEBUG(("screenItem=$%lx ", (unsigned long)(screenItem)));
  SDEBUG(("ccb=$%lx ", (unsigned long)(ccb)));
  SDEBUG((" )\n"));

  screen = (Screen *)CheckItem( screenItem, NODE_GRAPHICS, TYPE_SCREEN );
  if ( !screen ) {
    return GRAFERR_BADITEM;
  }
  if (screen->scr.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,screenItem)<0) {
      PRINTNOTOWNER (screen->scr.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

/*???*/
  return SWIDrawCels (screen->scr_TempBitmap->bm.n_Item, ccb);
}


/*
 * ewhac 9404.28:  Pulled all the bigpad?() functions out.  They were
 * #ifdef'ed out, anyway.
 */



/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING */
/*                                                                 */
/* The cel engine hardware has several serious bugs in it.         */
/* Do not attempt mess with the DrawCels routine.                  */
/*                                                                 */
/* WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING */

int32
SWIDrawCels (Item bitmapItem, CCB *ccb)
/*
 * Draw cels into the display, following the CCB chain
 */
{
  int32 retvalue;
  Bitmap *bitmap;
  uint32 t1, tid, state;
  Timer *timer;
  /* This routine should be in supervisor mode */

  SDEBUG(("DrawCels( "));
  SDEBUG(("bitmapItem=$%lx ", (unsigned long)(bitmapItem)));
  SDEBUG(("ccb=$%lx ", (unsigned long)(ccb)));
  SDEBUG((" )\n"));

  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }
  if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,bitmapItem)<0) {
      PRINTNOTOWNER (bitmap->bm.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

  SuperInternalLockSemaphore (GrafBase->gf_CelSemaphore, SEM_WAIT);

  t1 = bitmap->bm_WatchDogCtr;
  timer = GrafBase->gf_TimeoutTimer;
  tid = timer->tm_ID;
  (*timer->tm_Control) (timer, 0, TIMER_ALLBITS);
  (*timer->tm_Load) (tid-1, t1&0xffff, 0xffff);
  (*timer->tm_Load) (tid, (t1>>16)+1, 0x0000);
  (*timer->tm_Control) (timer, TIMER_CASCADE|TIMER_DECREMENT|TIMER_RELOAD, 0);

  *CCBCTL0 = bitmap->bm_CEControl;
  *REGCTL0 = bitmap->bm_REGCTL0;
  *REGCTL1 = bitmap->bm_REGCTL1;
  *REGCTL2 = bitmap->bm_REGCTL2;
  *REGCTL3 = bitmap->bm_REGCTL3;
  *NEXTPTR = (ulong)ccb;
  *SPRSTRT = 0; /* GO */

  retvalue = 0;

  state = 1;

  while( *STATBits & SPRON ) {
    /* Here because of an interrupt
     * and the Cel engine has more to do.
     * Is there a higher priority task waiting?
     * If so, go, return here when we are highest priority
     */
    if (*STATBits & SPRPAU) {
      if ( (KernelBase->kb_PleaseReschedule) ) {
	/* Stop timer during task switch */
	(*timer->tm_Control) (timer, 0, TIMER_DECREMENT);
	SuperSwitch();
	/* Restart timer upon return */
	(*timer->tm_Control) (timer, TIMER_DECREMENT, 0);
      }

      if (state) {
/*	if ((*timer->tm_Read)(tid)==0x0000) { */
	if (((HardTimer*)Timer0)[tid].ht_cnt==0x0000) {
	  *SPRPAUS = 0;		/* If timed out, issue pause request */
	  state--;
	}
      } else {
	retvalue = GRAFERR_CELTIMEOUT;	/* Timeout value underflowed */
	*SPRSTOP = 0;
	break;
      }
    }
    *SPRCNTU = 0;
  }

  SuperInternalUnlockSemaphore(GrafBase->gf_CelSemaphore);
  return retvalue;
}



void
MapCel( CCB *ccb, Point *quad )
/* Take a cel and a point and create position and delta values to map
 * its corners onto the specified quadrilateral.
 */
{

/* This routine should be in user mode */

	ccb->ccb_XPos = ((quad[0].pt_X<<16) & 0xffff0000) + 0x8000;
	ccb->ccb_YPos = ((quad[0].pt_Y<<16) & 0xffff0000) + 0x8000;
	ccb->ccb_HDX = ((quad[1].pt_X-quad[0].pt_X)<<20) / ccb->ccb_Width;
	ccb->ccb_HDY = ((quad[1].pt_Y-quad[0].pt_Y)<<20) / ccb->ccb_Width;
	ccb->ccb_VDX = ((quad[3].pt_X-quad[0].pt_X)<<16) / ccb->ccb_Height;
	ccb->ccb_VDY = ((quad[3].pt_Y-quad[0].pt_Y)<<16) / ccb->ccb_Height;
	ccb->ccb_HDDX = ((quad[2].pt_X-quad[3].pt_X-quad[1].pt_X+quad[0].pt_X)
			<< 20) / (ccb->ccb_Width*ccb->ccb_Height);
	ccb->ccb_HDDY = ((quad[2].pt_Y-quad[3].pt_Y-quad[1].pt_Y+quad[0].pt_Y)
			<< 20) / (ccb->ccb_Width*ccb->ccb_Height);
}



/* === ================================================================== */
/* === ================================================================== */
/* === ================================================================== */

void
zAddScreenGroup( Item sgitem, TagArg *targs )
{
/*???	List *list;*/
/*???	ScreenGroup *sg,*thissg;*/
/*???	int32 show,make;*/
/*???*/
/*???	show = GrafBase->gf_VDLSwitch;*/
/*???	make = 1 - show;*/
/*???*/
/*???	thissg = (ScreenGroup *)LocateItem( sgitem );*/
/*???	if ( (int32)thissg->sg_NextDisplay[show] != -1 ) return;*/
/*???*/
/*???	list = GrafBase->gf_ScreenGroupListPtr;*/
/*???	for( sg = (ScreenGroup *)FIRSTNODE( list ); ISNODE( list, sg ); */
/*???			sg = (ScreenGroup *)NEXTNODE( sg ) )*/
/*???		{*/
/*???		sg->sg_NextDisplay[make] = sg->sg_NextDisplay[show];*/
/*???		sg->sg_VDLPtr[make][0] = sg->sg_VDLPtr[show][0];*/
/*???		sg->sg_VDLPtr[make][1] = sg->sg_VDLPtr[show][1];*/
/*???		}*/
/*???	thissg->sg_NextDisplay[make] = GrafBase->gf_FirstDisplay[show];*/
/*???	GrafBase->gf_FirstDisplay[make] = thissg; */
}



void
zRemoveScreenGroup( Item sgitem )
{
/*???	List *list;*/
/*???	ScreenGroup *sg,*thissg;*/
/*???	int32 show,make;*/
/*???*/
/*???	show = GrafBase->gf_VDLSwitch;*/
/*???	make = 1 - show;*/
/*???*/
/*???	thissg = (ScreenGroup *)LocateItem( sgitem );*/
/*???	if( (int32)thissg->sg_NextDisplay[show] == -1 ) return;*/
/*???*/
/*???	list = GrafBase->gf_ScreenGroupListPtr;*/
/*???	for( sg = (ScreenGroup *)FIRSTNODE( list ); ISNODE( list, sg );*/
/*???			sg = (ScreenGroup *)NEXTNODE( sg ))*/
/*???		{*/
/*???		sg->sg_NextDisplay[make] = sg->sg_NextDisplay[show];*/
/*???		sg->sg_VDLPtr[make][0] = sg->sg_VDLPtr[show][0];*/
/*???		sg->sg_VDLPtr[make][1] = sg->sg_VDLPtr[show][1];*/
/*???		}*/
/*???*/
/*???	sg = GrafBase->gf_FirstDisplay[make] = GrafBase->gf_FirstDisplay[show];*/
/*???	if( sg == thissg )*/
/*???		GrafBase->gf_FirstDisplay[make] = thissg->sg_NextDisplay[make];*/
/*???	else*/
/*???		for( ; sg; sg = sg->sg_NextDisplay[make] )*/
/*???			{*/
/*???			if( sg->sg_NextDisplay[make] == thissg )*/
/*???				{*/
/*???				sg->sg_NextDisplay[make] = thissg->sg_NextDisplay[make];*/
/*???				break;*/
/*???				}*/
/*???			}*/
/*???*/
/*???	thissg->sg_NextDisplay[make] = (ScreenGroup *)-1;*/
}



/* === ================================================================== */
/* === ================================================================== */
/* === ================================================================== */

Item
internalCreateScreenGroup (ScreenGroup *sg, TagArg *args)
{
  SDEBUGVDL(("internalCreateScreenGroup( ScreenGroup=0x%x TagArg=0x%x\n",sg,args));
  sg->sg_Y = -1;		/* code for "screen location not yet allocated" */
  InitList (&sg->sg_SharedList, "ScreenGroup shared access list\n");
  return sg->sg.n_Item;
}



Item
internalCreateScreen (Screen *scr, void *args)
{
/*???	AddTail(GrafBase->gf_ScreenListPtr,(Node *)scr);*/
  InitList (&scr->scr_SharedList, "Screen shared access list\n");
  return scr->scr.n_Item;
}



Item
internalOpenScreenGroup (ScreenGroup *sg, void *args)
{
  /* For now, we require the args field to be NULL */
  if (args) {
    return GRAFERR_BADPTR;
  }
  return GRAFERR_NOTYET;
}


Item
internalOpenScreen (Screen *s, void *args)
{
  Err e;
  SharedListNode *sl;

  /* For now, we require the args field to be NULL */
  if (args) {
    return GRAFERR_BADPTR;
  }
  e = SuperOpenItem (s->scr_TempBitmap->bm.n_Item, 0);
  if (e<0) {
    return e;
  }
  sl = (SharedListNode*) SUPER_ALLOCMEM (sizeof(SharedListNode), MEMTYPE_ANY);
  if (!sl) {
    return GRAFERR_NOMEM;
  }
  sl->sl_TaskItem = CURRENTTASK->t.n_Item;
  AddTail (&s->scr_SharedList, (Node *)sl);
  return s->scr.n_Item;
}


Item
internalOpenBitmap (Bitmap *b, void *args)
{
  Err e;
  SharedListNode *sl;

  /* For now, we require the args field to be NULL */
  if (args) {
    return GRAFERR_BADPTR;
  }
  e = SuperValidateMem (CURRENTTASK, (uint8*)b->bm_Buffer, b->bm_Width*b->bm_Height*2);
  if (e<0) {
    return GRAFERR_NOWRITEACCESS;
  }
  sl = (SharedListNode*) SUPER_ALLOCMEM (sizeof(SharedListNode), MEMTYPE_ANY);
  if (!sl) {
    return GRAFERR_NOMEM;
  }
  sl->sl_TaskItem = CURRENTTASK->t.n_Item;
  AddTail (&b->bm_SharedList, (Node *)sl);
  return b->bm.n_Item;
}



Err
internalCloseScreenGroup (ScreenGroup *sg, Task *t)
{
  Node *n;
  for (n=FIRSTNODE(&sg->sg_SharedList); ISNODE(&sg->sg_SharedList,n); n=NEXTNODE(n)) {
    if (((SharedListNode *)n)->sl_TaskItem == t->t.n_Item) {
      RemNode(n);
      SUPER_FREEMEM (n, sizeof(SharedListNode));
      return 0;
    }
  }
  DEVBUG (("CloseScreenGroup failed\n"));
  DEVBUG (("Unable to find task item 0x%lx in shared list for item 0x%lx\n",
	   t->t.n_Item, sg->sg.n_Item));
  return GRAFERR_INTERNALERROR;
}


int32
internalDeleteScreenGroup (ScreenGroup *sg, Task *t)
{
	Item sgi;

	sgi = sg->sg.n_Item;

 	RemNode( (Node *)sg );  /* Unhook from other applications' groups */

	/* Delete any Screen items that refer to this Screen Group */
/*???	list = GrafBase->gf_ScreenListPtr;*/
/*???	scr = (Screen *)FIRSTNODE(list);*/
/*???	while(ISNODE(list,scr))*/
/*???		{*/
/*???		nextscr = (Screen *)NEXTNODE(scr);*/
/*???		if(scr->scr_ScreenGroupPtr == sg)*/
/*???			SuperexternalDeleteItem(scr->scr.n_Item);*/
/*???		scr = nextscr;*/
/*???		}*/

	return 0;						/* Error free return */
}


Err
internalCloseScreen (Screen *scr, Task *t)
{
  Node *n;

  SuperCloseItem (scr->scr_TempBitmap->bm.n_Item);		/* Close bitmap */

  for (n=FIRSTNODE(&scr->scr_SharedList); ISNODE(&scr->scr_SharedList,n); n=NEXTNODE(n)) {
    if (((SharedListNode *)n)->sl_TaskItem == t->t.n_Item) {
      RemNode(n);
      SUPER_FREEMEM (n, sizeof(SharedListNode));
      return 0;
    }
  }
  DEVBUG (("CloseScreen failed\n"));
  DEVBUG (("Unable to find task item 0x%lx in shared list for item 0x%lx\n",
	   t->t.n_Item, scr->scr.n_Item));
  return GRAFERR_INTERNALERROR;
}


int32
internalDeleteScreen (Screen *scr, Task *t)
{
  extern Overlay	*ActiveOverlay;
  Node			*n;

  SDEBUGVDL(("internalDeleteScreen called with screen ptr 0x%x\n",scr));

  while ( n=FIRSTNODE(&scr->scr_SharedList),ISNODE(&scr->scr_SharedList,n) ) {
    RemNode (n);
    SUPER_FREEMEM (n, sizeof(SharedListNode));
  }

  /* JCR */
  if (( GrafBase->gf_CurrentVDLEven == scr->scr_VDLPtr )
      || ( GrafBase->gf_CurrentVDLOdd == scr->scr_VDLPtr )) {
    if (GrafBase->gf_CurrentVDLEven->vdl_DisplayType == DI_TYPE_PAL2) {
      resync_displaywidth();
    }
    GrafBase->gf_CurrentVDLEven = GrafBase->gf_BlankVDL;
    GrafBase->gf_CurrentVDLOdd = GrafBase->gf_BlankVDL;

    if (ActiveOverlay)
      TakedownOverlay (ActiveOverlay);
  }

  RemNode((Node *)scr);			/* Unhook from list of screens */
  return 0;
}


Err
internalCloseBitmap (Bitmap *bm, Task *t)
{
  Node *n;
  for (n=FIRSTNODE(&bm->bm_SharedList); ISNODE(&bm->bm_SharedList,n); n=NEXTNODE(n)) {
    if (((SharedListNode *)n)->sl_TaskItem == t->t.n_Item) {
      RemNode(n);
      SUPER_FREEMEM (n, sizeof(SharedListNode));
      return 0;
    }
  }
  DEVBUG (("CloseBitmap failed\n"));
  DEVBUG (("Unable to find task item 0x%lx in shared list for item 0x%lx\n",
	   t->t.n_Item, bm->bm.n_Item));
  return GRAFERR_INTERNALERROR;
}


int32
internalDeleteBitmap (Bitmap *bm, Task *t)
{
  Node *n;
  while ( n=FIRSTNODE(&bm->bm_SharedList),ISNODE(&bm->bm_SharedList,n) ) {
    RemNode (n);
    SUPER_FREEMEM (n, sizeof(SharedListNode));
  }
  RemNode ((Node*)bm);
  return 0;
}



Item
internalCreateGrafItem( void *n, uint8 ntype, void *args )
{
  SDEBUGITEM (("CreateGrafItem (0x%lx, %d, 0x%lx)\n", n, ntype, args));

  switch (ntype) {
  case TYPE_SCREENGROUP:
    return internalCreateScreenGroup( (ScreenGroup *)n, (TagArg *)args );
  case TYPE_SCREEN:
    return internalCreateScreen( (Screen *)n, (TagArg *)args );
  case TYPE_BITMAP:
    return internalCreateBitmap( (Bitmap *)n, (TagArg *)args );
  case TYPE_VDL:
    return internalCreateVDL( (VDL *)n, (TagArg *)args );
  case TYPE_OVERLAY:
    return (internalCreateOverlay ((Overlay *) n, (TagArg *) args));
  }

  return( GRAFERR_BADSUBTYPE );
}




int32
internalDeleteGrafItem( Item it, Task *t )
{
	Node *n;

	SDEBUGITEM (("DeleteGrafItem (0x%lx, 0x%lx)\n", it, t));

	n = (Node *)LookupItem( it );

	switch (n->n_Type) {
	case TYPE_SCREENGROUP:
		return internalDeleteScreenGroup ((ScreenGroup *)n, t);
	case TYPE_SCREEN:
		return internalDeleteScreen ((Screen *)n, t);
	case TYPE_BITMAP:
		return internalDeleteBitmap ((Bitmap *)n, t);
	case TYPE_VDL:
		return internalDeleteVDL ((VDL *)n, t);
	case TYPE_OVERLAY:
		return (internalDeleteOverlay ((Overlay *) n, t));
	}

	return( GRAFERR_INTERNALERROR );
}



Item
internalFindGrafItem (int32 ntype, TagArg *p)
{
	SDEBUGITEM (("FindGrafItem (%d, %s)\n", ntype, p));

	return( GRAFERR_NOTYET );
}



Item
internalOpenGrafItem (Node *n, void *args)
{
  SDEBUGITEM (("OpenGrafItem (%d, 0x%lx)\n", node, args));

  switch (n->n_Type) {
  case TYPE_SCREENGROUP:
    return internalOpenScreenGroup ((ScreenGroup *)n, args);
  case TYPE_SCREEN:
    return internalOpenScreen ((Screen *)n, args);
  case TYPE_BITMAP:
    return internalOpenBitmap ((Bitmap *)n, args);
  case TYPE_VDL:
    return internalOpenVDL ((VDL *)n, args);
  case TYPE_OVERLAY:
    return (internalOpenOverlay ((Overlay *) n, (TagArg *) args));
  default:
    return( GRAFERR_INTERNALERROR );
  }
}


Item
internalCloseGrafItem (Item it, Task *t)
{
  Node *n;

  SDEBUGITEM (("CloseGrafItem (%d, 0x%lx)\n", node, args));

  n = (Node *)LookupItem (it);

  switch (n->n_Type) {
  case TYPE_SCREENGROUP:
    return internalCloseScreenGroup ((ScreenGroup *)n, t);
  case TYPE_SCREEN:
    return internalCloseScreen ((Screen *)n, t);
  case TYPE_BITMAP:
    return internalCloseBitmap ((Bitmap *)n, t);
  case TYPE_VDL:
    return internalCloseVDL ((VDL *)n, t);
  case TYPE_OVERLAY:
    return (internalCloseOverlay ((Overlay *) n, t));
  default:
    return GRAFERR_INTERNALERROR;
  }
}


Err
internalSetOwnerGrafItem (ItemNode *it, Item newOwner, Task *oldOwner)
{
	switch (it->n_Type) {
	case TYPE_SCREEN:
	case TYPE_BITMAP:
	case TYPE_VDL:
	case TYPE_OVERLAY:
		return (0);

	default:
		/*
		 * No ScreenGroups or DisplayInfos.
		 */
		return (GRAFERR_INTERNALERROR);
	}
}


/* === ================================================================== */
/* === ================================================================== */
/* === ================================================================== */

int32
SetClipWidth( Item bitmapItem, int32 clipwidth )
/*
 * Set the bitmap
 */
{
  Bitmap *bitmap;

/* This routine needs to be in supervisor mode and needs to do serious */
/* validity checking */

  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }
  if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,bitmapItem)<0) {
      PRINTNOTOWNER (bitmap->bm.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

  if (( clipwidth <= 0 ) || ( clipwidth + bitmap->bm_ClipX > bitmap->bm_Width )) {
    return GRAFERR_BADCLIP;
  }

  bitmap->bm_ClipWidth = clipwidth;
  bitmap->bm_REGCTL1 = MAKE_REGCTL1( bitmap->bm_ClipWidth, bitmap->bm_ClipHeight );

  return 0;
}



int32
SetClipHeight( Item bitmapItem, int32 clipheight )
/*
 * Set the bitmap
 */
{
  Bitmap *bitmap;

/* This routine needs to be in supervisor mode and needs to do serious */
/* validity checking */

  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }
  if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,bitmapItem)<0) {
      PRINTNOTOWNER (bitmap->bm.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

  if (( clipheight <= 0 ) || ( clipheight + bitmap->bm_ClipY > bitmap->bm_Height )) {
    return GRAFERR_BADCLIP;
  }

  bitmap->bm_ClipHeight = clipheight;
  bitmap->bm_REGCTL1 = MAKE_REGCTL1( bitmap->bm_ClipWidth, bitmap->bm_ClipHeight );

  return 0;
}



int32
SetClipOrigin( Item bitmapItem, int32 x, int32 y )
/*
 * Set the bitmap
 */
{
  int32 i;
  Bitmap *bitmap;

SDEBUG(("SetClipOrigin( "));
SDEBUG(("bitmapItem=$%lx ", (unsigned long)(bitmapItem)));
SDEBUG(("x=%ld ", (unsigned long)(x)));
SDEBUG(("y=%ld ", (unsigned long)(y)));
SDEBUG((" )\n"));

/* This routine needs to be in supervisor mode and needs to do serious */
/* validity checking */

  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }
  if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,bitmapItem)<0) {
      PRINTNOTOWNER (bitmap->bm.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

/*???*/
  if ( y & 1 ) {
    DEVBUG (("warning:  SetClipOrigin odd Y\n"));
    y = y & -2;
  }

  if (( x < 0 ) || ( x + bitmap->bm_ClipWidth > bitmap->bm_Width )
      || ( y < 0 ) || ( y + bitmap->bm_ClipHeight > bitmap->bm_Height )) {
    return GRAFERR_BADCLIP;
  }

  i = (y * bitmap->bm_Width + x * 2) * 2;
  i += (int32)(bitmap->bm_Buffer);
  if ( bitmap->bm_REGCTL2 == bitmap->bm_REGCTL3 ) {
    bitmap->bm_REGCTL2 = i;
  }
  bitmap->bm_REGCTL3 = i;
  bitmap->bm_ClipX = x;
  bitmap->bm_ClipY = y;

  return 0;
}



int32
SetScreenColor( Item screenItem, uint32 colorEntry )
{
	return( SetScreenColors( screenItem, &colorEntry, 1 ) );
}


int32
SetScreenColors( Item screenItem, uint32 *colorEntries, int32 count )
{
  extern Overlay *ActiveOverlay;
  uint32 i;
  ubyte index, red, green, blue;
  Screen *screen;
  uint32 colorEntry;

  /* This routine needs to be in supervisor mode */

  screen = (Screen *)CheckItem( screenItem, NODE_GRAPHICS, TYPE_SCREEN );
  if ( NOT screen ) {
    return GRAFERR_BADITEM;
  }
  if (screen->scr.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,screenItem)<0) {
      PRINTNOTOWNER (screenItem, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }
  if ( screen->scr_VDLType != VDLTYPE_SIMPLE ) {
    return GRAFERR_BADVDLTYPE;
  }

  for ( ; count > 0; count-- ) {
    colorEntry = *colorEntries++;
    index = (ubyte)(colorEntry >> 24);
    if (index == ((VDL_DISPCTRL | VDL_BACKGROUND) >> 24))
    	index = 32;	/*  Cheesy hack for background color entries.  */
    if ( index <= 32 ) {
      red = (ubyte)(colorEntry >> 16);
      green = (ubyte)(colorEntry >> 8);
      blue = (ubyte)(colorEntry >> 0);

      if (index==32) {
	i = MakeCLUTBackgroundEntry (red, green, blue);
      } else {
	i = MakeCLUTColorEntry (index, red, green, blue);
      }

      *(screen->scr_VDLPtr->vdl_DataPtr + 5 + index) = i;
    } else {
      return GRAFERR_INDEXRANGE;
    }
  }
  if (ActiveOverlay)
    setundercolor (ActiveOverlay, screen->scr_VDLPtr);

  return 0;
}


RGB888
ReadCLUTColor (ulong index)
{
/*???	RGB888 c;*/
/*???*/
/*???	if ( index > 32 ) return 0xffffffff;*/
/*???	c = GrafBase->gf_VDL[index+GrafBase->gf_LineHeaderSize] & 0x00ffffff;*/
/*???#ifdef __SHERRIE*/
/*???	{*/
/*???		ulong r,g,b;*/
/*???		r = (c>>16)&0xff;*/
/*???		g = (c>>8)&0xff;*/
/*???		b = c&0xff;*/
/*???		r = ((r-16)*255)/(235-16);*/
/*???		g = ((g-16)*255)/(235-16);*/
/*???		b = ((b-16)*255)/(235-16);*/
/*???		c = (r<<16) + (g<<8) + b;*/
/*???	}*/
/*???#endif*/
/*???	return c;*/

return 0;
}


int32
ResetScreenColors( Item screenItem )
{
  ulong i;
  ubyte color;
  int32 colorEntry;
  int32 retvalue;

  /*??? This routine would be faster (and fatter) if we calculated
   * the screen address once and poked the values directly
   */
  for ( i = 0; i < 32; i++ ) {
    color = (ubyte)(( i * 255 ) / 31);
    colorEntry = MakeCLUTColorEntry(i, color, color, color );
    retvalue = SetScreenColor( screenItem, colorEntry );
    if ( retvalue < 0 ) goto DONE;
  }

  retvalue = 0;

DONE:
  return( retvalue );
}


int32 ResetReadAddress( Item bitmapItem )
{
  Bitmap *bitmap;
  int32 i3;

  bitmap = (Bitmap *)CheckItem( bitmapItem, NODE_GRAPHICS, TYPE_BITMAP );
  if ( !bitmap ) {
    return GRAFERR_BADITEM;
  }
  if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
    if (ItemOpened(CURRENTTASK->t.n_Item,bitmapItem)<0) {
      PRINTNOTOWNER (bitmap->bm.n_Item, CURRENTTASK->t.n_Item);
      return GRAFERR_NOTOWNER;
    }
  }

  i3 = (bitmap->bm_REGCTL0 & WMOD_MASK);
  bitmap->bm_REGCTL0 = i3 | ( ((i3 >> WMOD_SHIFT) << RMOD_SHIFT) & RMOD_MASK );
  bitmap->bm_REGCTL2 = bitmap->bm_REGCTL3;

  return 0;
}



Err
SetReadAddress
(
Item	bitmapItem,
ubyte	*buffer,
int32	width
)
{
	ValidWidth	*vw;
	Bitmap		*bitmap;
	int32		rc0, size;

	if ((bitmap = (Bitmap *) CheckItem (bitmapItem,
					    NODE_GRAPHICS,
					    TYPE_BITMAP)) == 0)
		return (GRAFERR_BADITEM);

	if (bitmap->bm.n_Owner != CURRENTTASK->t.n_Item) {
		if (ItemOpened (CURRENTTASK->t.n_Item,bitmapItem) < 0) {
			PRINTNOTOWNER
			 (bitmap->bm.n_Item, CURRENTTASK->t.n_Item);
			return (GRAFERR_NOTOWNER);
		}
	}

	if ((vw = FindValidWidth (width, 0)) == 0)
		return (GRAFERR_BUFWIDTH);
	rc0 = vw->vw_CelMods & RMOD_MASK;

	/*
	 * This size calculation will eventually be performed by an external
	 * routine.
	 */
	size = (bitmap->bm_ClipHeight + 1) >> 1;  /*  Quantized height.  */
	size = (width * (size - 1) + bitmap->bm_ClipWidth) * sizeof (uint32);

	if (SuperIsRamAddr (buffer, size) == 0)
		return (GRAFERR_BADPTR);

	bitmap->bm_REGCTL0 = (bitmap->bm_REGCTL0 & (~RMOD_MASK)) | rc0;
	bitmap->bm_REGCTL2 = (int32) buffer;

	return (0);
}
