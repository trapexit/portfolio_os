/* *************************************************************************
 *
 * Internal Graphics Include File
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 940531 ewhac            Removed redundant inclusions
 * 940223 ewhac            Added ValidWidth structure definition
 * 920724 -RJ Mical        Start overhaul
 * 920717 Stephen Landrum  Last edits before July handoff
 *
 * ********************************************************************** */



#ifndef __INTGRAF_H
#define __INTGRAF_H


#define _GRAPHICS_INTERNAL


/***************************************************************************
 * Inclusions.
 */
#ifndef	__TYPES_H
#include "types.h"
#endif

#ifndef	__FOLIO_H
#include "folio.h"
#endif

#ifndef	__DRIVER_H
#include "driver.h"
#endif

#ifndef	__DEVICE_H
#include "device.h"
#endif

#ifndef	__IO_H
#include "io.h"
#endif

#ifndef	__SYSINFO_H
#include "sysinfo.h"
#endif

#ifndef __SEMAPHORE_H
#include "semaphore.h"
#endif

#ifndef	__SUPER_H
#include "super.h"
#endif


#ifndef	__GRAPHICS_H
#include "graphics.h"
#endif


#ifndef	__INTHARD_H
#include "inthard.h"
#endif


/***************************************************************************
 * Definitions.
 */

/* This should be defined in inthard.h */
#define SC384EN		0x00100000


/* Private definition of the GrafFolio structure */

typedef struct GrafFolio
{
        Folio gf;

	uint32	gf_Flags;

#ifdef __cplusplus
	uint32 gf_VBLNumber;
#else
	volatile uint32 gf_VBLNumber;
#endif /* __cplusplus */

	void*	gf_ZeroPage;
	void*	gf_VIRSPage;

	uint32	gf_VRAMPageSize;
	int32	gf_DefaultDisplayWidth;
	int32	gf_DefaultDisplayHeight;

	struct Timer*	gf_TimeoutTimer;

	int32	gf_Reserved5;
	int32	gf_Reserved6;
	int32	gf_Reserved7;

	VDLEntry* gf_VDLForcedFirst;
	VDLEntry* gf_VDLPreDisplay;
	VDLEntry* gf_VDLPostDisplay;
	VDLEntry* gf_VDLBlank;
	VDL*	gf_CurrentVDLEven;
	VDL*	gf_CurrentVDLOdd;
	VDLEntry* gf_VDLDisplayLink;

	int32	gf_Reserved1;
	int32	gf_Reserved3;

	Semaphore *gf_CelSemaphore;	/* who has the Cel Engine? */

	int32	gf_VBLTime;		/* number of usec between VBLs */
	int32	gf_VBLFreq;		/* approximate VBL frequency in Hz */

	int32   gf_Reserved2;

	struct Stream*	gf_CurrentFontStream;	/* Much of this block is unused */
	int32   gf_FileFontCacheSize;
	int32   gf_FileFontCacheAlloc;
	ubyte*	gf_FileFontCache;
	FontEntry* gf_FontEntryHead;
	FontEntry* gf_FontEntryButt;
	List    gf_FontLRUList;
	int32   gf_FileFontFlags;
	int32   gf_FontBaseChar;
	int32   gf_FontMaxChar;
	Font*	gf_CurrentFont;
	int32   gf_CharArrayOffset;
	int32   gf_fileFontCacheUsed;

	List	gf_DisplayInfoList;

	uint32	gf_DefaultDisplayType;
	uint32	gf_DisplayTypeMask;
	VDL*	gf_BlankVDL;

/***  Some new stuff.  May get folded upstairs.  ***/

	void	***gf_ForcedFirstPtr;
	void	***gf_DisplayLinkPtr;

} GrafFolio;


#define ItemOpened(t,i) IsItemOpened(t,i)

extern uint32 linewidth;
extern uint32 palflag, pal2flag, _overrideflag, slipstream_dispctrl;

extern int32 _DisplayHeight[], _DisplayWidth[], _DisplayMCTL[], _DisplayHDelay[];
extern int32 _DisplayDISPCTRL[], _DisplayOffset[];


/* Internal switches for compilation mode */
#define _MODE_developer	0
#define _MODE_runtime	1


#if (MODE==_MODE_developer)
void printnotowner (Item it, Item t);
#define PRINTNOTOWNER(x,y) printnotowner(x,y)
#define DEVBUG(x) Superkprintf x
#else
#define PRINTNOTOWNER(x,y) /* printnotowner(x,y) */
#define DEVBUG(x) /* Superkprintf x */
#endif



/* there's 10 altogether */
/*???#define DEBUG(x) { kprintf x; }*/
#define DEBUG(x) {}
/*??? #define SDEBUG(x)   { Superkprintf x; }*/
#define SDEBUG(x)   {}
/*???#define SDEBUGSPORT(x)   { Superkprintf x; }*/
#define SDEBUGSPORT(x)   {}
/*???#define SDEBUGITEM(x)  { Superkprintf x; }*/
#define SDEBUGITEM(x)  {}
/* #define SDEBUGGRAF(x)   { Superkprintf x; } */
#define SDEBUGGRAF(x)   {}
/*???#define DEBUGGRAF(x)   { kprintf x; }*/
#define DEBUGGRAF(x)   {}
/*???#define DEBUGELLIPSE(x)   { kprintf x; }*/
#define DEBUGELLIPSE(x)   {}
/*???#define DEBUGGRAFREGIS(x)  { kprintf x; }*/
#define DEBUGGRAFREGIS(x)  {}
/*???#define DEBUGBLIT(x) { kprintf x; }*/
#define DEBUGBLIT(x) {}
/*???#define SDEBUGVDL(x)  { Superkprintf x; }*/
#define SDEBUGVDL(x)  {}


/* Here are the supervisor mode SWI values that we don't publish to the outside world */

#define _GETVBLATTRS		(GRAFSWI+53)
#define _SETVBLATTRS		(GRAFSWI+52)
#define _MODIFYVDL		(GRAFSWI+51)
#define _CREATESCREENGROUP	(GRAFSWI+50)
/* */
/* #define _SUBMITVDL		(GRAFSWI+48) */
#define _SETVDL			(GRAFSWI+47)
/* */
#define _DISPLAYSCREEN		(GRAFSWI+45)
/* */
/* */
#define _SETCEWATCHDOG		(GRAFSWI+42)
#define _SETCECONTROL		(GRAFSWI+41)
/* */
/* #define _DRAWCELS		(GRAFSWI+39)	published	*/
#define _DRAWTEXT8		(GRAFSWI+38)
#define _GETCURRENTFONT		(GRAFSWI+37)
#define _SETCURRENTFONTCCB	(GRAFSWI+36)
#define _FILLRECT		(GRAFSWI+35)
/* */
#define _DRAWTO			(GRAFSWI+33)
#define _COPYLINE		(GRAFSWI+32)
#define _DRAWCHAR		(GRAFSWI+31)
/* #define _SUPERCLOSEFONT	(GRAFSWI+30) */
#define _DRAWTEXT16		(GRAFSWI+29)
/* */
#define _SUPEROPENRAMFONT	(GRAFSWI+27)
/* #define _SETFILEFONTCACHESIZE	(GRAFSWI+26) */
/* #define _SUPEROPENFILEFONT	(GRAFSWI+25) */
/* */
/* #define _DRAWSCREENCELS		(GRAFSWI+23)	published	*/
/* */
/* */
#define _SETCLIPHEIGHT		(GRAFSWI+20)
#define _SETCLIPWIDTH		(GRAFSWI+19)
#define _REMOVESCREENGROUP	(GRAFSWI+18)
#define _ADDSCREENGROUP		(GRAFSWI+17)
#define _SETBGPEN		(GRAFSWI+16)
#define _SETFGPEN		(GRAFSWI+15)
#define _SUPERRESETCURRENTFONT	(GRAFSWI+14)
#define _SETSCREENCOLORS	(GRAFSWI+13)
/* */
/* #define _RESETSYSTEMGRAPHICS	(GRAFSWI+11) */
#define _RESETSCREENCOLORS	(GRAFSWI+10)
#define _SETSCREENCOLOR		(GRAFSWI+9)
#define _DISABLEHAVG		(GRAFSWI+8)
#define _ENABLEHAVG		(GRAFSWI+7)
#define _DISABLEVAVG		(GRAFSWI+6)
#define _ENABLEVAVG		(GRAFSWI+5)
/* */
#define _SETCLIPORIGIN		(GRAFSWI+3)
#define _RESETREADADDRESS	(GRAFSWI+2)
#define _SETREADADDRESS		(GRAFSWI+1)
/* */




typedef struct SWOFF {
  struct Stream *stream;
  int32 basechar;
  int32 charcount;
  Font *font;
  int32 chararrayoffset;
} SWOFF;


typedef struct SharedListNode {
  MinNode	sl;
  Item		sl_TaskItem;
} SharedListNode;


#define MAX_PLUT_SIZE (32*2)



/*???#define GETPIXELADDRESS -4*/
/*???#define READVDLCOLOR -3*/
/*???#define READPIXEL -2*/
/*???#define QUADMAPSPRITE -1*/



#define BLANKVDL_SIZE (4+34)	/* number of words in the system VDL entry */
#define	BLANKVDL_DMACTRL2 (VDL_ENVIDDMA|VDL_PREVSEL|VDL_LDCUR|VDL_LDPREV\
			   |((32+2)<<VDL_LEN_SHIFT)|240)
#define BLANKVDL_DISPCTRL (VDL_DISPCTRL)
#define	VDL_DMACTRLLAST (VDL_LDCUR|VDL_LDPREV|(2<<VDL_LEN_SHIFT)|0)
#define VDL_DISPCTRLLAST (VDL_DISPCTRL|VDL_HINTEN|VDL_BLSB_NOP|VDL_HSUB_NOP\
			  |VDL_VSUB_NOP|VDL_WINBLSB_NOP|VDL_WINHSUB_NOP\
			  |VDL_WINVSUB_NOP)



/* #define SCREENWIDTH	1024 */
/*???#define DISPLAY_WIDTH      512*/
#define DISPLAY_WIDTH      320
#define DISPLAY_CLIPWIDTH  320
#define	DISPLAY_HEIGHT     240

#define DISPLAY_RAMSIZE      (DISPLAY_WIDTH*DISPLAY_HEIGHT*2)
/*???#define FB_OFFSET	(32*4)*/



#define MAKE_REGCTL1(width,height) (((width-1)<<REG_XCLIP_SHIFT)\
                                     |((height-1)<<REG_YCLIP_SHIFT))



/* routine numbers for folio calls */
/* #define RESETFONT       -11 */
/* #define CLOSEFONT       -10 */
/* #define DRAWTEXT16       -9 */
/* #define OPENFILEFONT     -8 */
/* #define OPENRAMFONT      -7 */
/* #define SETFONTCACHE     -6 */
#define WRITEPIXEL       -5
#define GETPIXELADDRESS  -4
#define READVDLCOLOR     -3
#define READPIXEL        -2
#define MAPSPRITE        -1



/***************************************************************\
* Useful macros							*
\***************************************************************/

#define SWAP(a,b,cast) {cast swp_; swp_=a; a=b; b=swp_;}


#ifdef __SHERRIE
/*???#define MINSPORTVCOUNT	7*/
#define MINSPORTVCOUNT	10
/*???#define MAXSPORTVCOUNT	16*/
#define MAXSPORTVCOUNT	13
#else
#define MINSPORTVCOUNT	7
#define MAXSPORTVCOUNT	14
#endif


/***************************************************************\
* Structures							*
\***************************************************************/



typedef struct CreateScreenArgs	{
  int32 st_DisplayHeight;
  int32 st_ScreenCount;
  int32 st_ScreenHeight;
  int32 st_BitmapCount;
  int32 *st_BitmapWidthArray;
  int32 *st_BitmapHeightArray;
  ubyte **st_BitmapBufArray;
  int32 st_VDLType;
  VDLEntry **st_VDLPtrArray;
  int32 *st_VDLLengthArray;
  int32 st_SPORTBankBits;
  int32 st_bufarrayallocatedflag;
  int32 st_DisplayType;
} CreateScreenArgs;


/* This is stuffed here for the moment just so compiles won't complaing */
#if 0
typedef struct FileFontHeader {
  int32 ffh_Width;
  int32 ffh_ImageSize;
} FileFontHeader;
#endif


/*
 * Structure describing valid widths supported by graphics, and associated
 * HW configuration data.
 */
typedef struct ValidWidth {
	int32	vw_Width;	/*  Width in pixels.			  */
	uint32	vw_CelMods;	/*  Modulo bits for cel engine.		  */
	uint32	vw_DispMods;	/*  Modulo bits for VDL.  Invalid if = ~0 */
} ValidWidth;

#define	_UNDISPLAYABLE	(~0)

/*  These will get publicized at some point.  */
#define	BMF_QUANTIZE	1
#define	BMF_BUMPWIDTH	(1<<1)
#define	BMF_DISPLAYABLE	(1<<2)




/* Additional private tage values for the DisplayInfo structure */

/* Value to be stuffed into HDelay register */
#define DI_HDELAY	(DI_PRIVATE+0)
/* Value to be OR'ed into SC384EN bit in MCTL */
#define DI_MCTL		(DI_PRIVATE+1)
/* Value to be OR'ed into VDL_PALSEL bit in VDL display control words */
#define DI_DISPCTRL	(DI_PRIVATE+2)


#ifdef __cplusplus
extern "C" {
#endif


/***************************************************************\
* Prototypes							*
\***************************************************************/

int InstallGraphicsFolio (int argc, char *argv[]);

/* Routine to initialize SPORT transfer device driver */
/* Item createSPORTDriver(void); */

/* Null routine for placeholder */
Item NULROUTINE(void);

/* Internal routines to manipulate the PSR */
uint32 GDisable (void);
void GEnable (uint32 state);

/* Initialize the graphics folio */
int32 InitGrafBase (GrafFolio *gb);

void SoftCel (GrafCon *gc, CCB *ccb);

void blitrect (uint32 *dest, uint32 *src, uint32 *info);


void graphicRemoveItem(Task *t,Item i);
extern int32 SuperexternalDeleteItem(Item i);

void CalculatePatch(ScreenGroup *sg,VDL *vdl,VDLEntry **PatchPtrPtr,VDLEntry *PatchDMACtrlPtr);
void AddGroupToDisplay(Item sgitem);
void RemoveGroupFromDisplay(Item sgitem);
void MoveGroupInDisplay(Item sgitem,int32 y);

void printgraffolio(void);
void printscreengroup(Item i);
void printscreen(Item i);
void printvdl(Item i);

void resync_displaywidth (void);

Item internalCreateVDL( VDL *vdl, TagArg *args );
Item internalOpenVDL( VDL *vdl, void *args );
Err internalDeleteVDL( VDL *vdl, Task *t );
Err internalCloseVDL( VDL *vdl, Task *t );
Item realCreateScreenGroup( Item *screenItemArray, CreateScreenArgs *stargs );
void realSetVRAMPages(void *dest,int32 val,int32 numpages, int32 mask);/*JCR*/


int32 BuildSystemVDLs( void );

Item internalFindGrafItem (int32 ntype, TagArg *p);
int32 internalDeleteGrafItem (Item it, Task *t);
Item internalCreateGrafItem(void *n, uint8 ntype, void *args);
Item internalOpenGrafItem (Node *n, void *args);
int32 internalCloseGrafItem (Item it, Task *t);

void GrafInit( void );
Item InitGraphicsErrors( void );

VDLEntry *ProofVDLEntry(VDLEntry *VDLDataPtr, int32 length, int32 DisplayType,
			int32 numlines);

void  InitFontEntry( void );

int32 InitFontStuff( void );

Err internalGetVBLAttrs (struct TagArg *);


struct ValidWidth *FindValidWidth (int32 width, uint32 flags);




extern struct KernelBase *KernelBase;

/*??? Get rid of this */
#define PIXELSIZE 1
#define PIXELSHIFT 0
extern VDLEntry *_VDLControlWord;


/* int32  __swi(_SUPERCLOSEFONT)  superCloseFont( void ); */
int32  __swi(_SUPEROPENRAMFONT)  superOpenRAMFont( Font *font );
/* int32  __swi(_SUPEROPENFILEFONT)  superOpenFileFont( SWOFF *swoff ); */
int32  __swi(_SUPERRESETCURRENTFONT)  superResetCurrentFont( void );


/* int32 swiSuperCloseFont( void ); */
/* int32 swiSuperOpenFileFont( SWOFF *swoff ); */
int32 swiSuperOpenRAMFont( Font *font );
int32 swiSuperResetCurrentFont( void );

Err SWIDrawCels (Item bitmapItem, CCB *ccb);
Err SWIDrawScreenCels (Item screenItem, CCB *ccb);

int32 InitDefaultFont( void );
void  InsertFontEntry( FontEntry *newentry );



__swi(_DRAWSCREENCELS) int32 __DrawScreenCels( Item screenItem, CCB *ccb );
__swi(_DRAWCELS) int32 __DrawCels( Item bitmapItem, CCB *ccb );
__swi(_ADDSCREENGROUP) int32 __AddScreenGroup( Item screenGroup, TagArg *targs );
__swi(_DISABLEHAVG) int32 __DisableHAVG( Item screenItem );
__swi(_DISABLEVAVG) int32 __DisableVAVG( Item screenItem );
__swi(_DISPLAYSCREEN) int32 __DisplayScreen( Item screenItem0, Item screenItem1 );
__swi(_DRAWCHAR) int32 __DrawChar( GrafCon *gcon, Item bitmapItem, uint32 character );
__swi(_DRAWTEXT16) int32 __DrawText16( GrafCon *gcon, Item bitmapItem, uint16 *text );
__swi(_DRAWTEXT8) int32 __DrawText8( GrafCon *gcon, Item bitmapItem, uint8 *text );
__swi(_DRAWTO) int32 __DrawTo( Item bitmapItem, GrafCon *grafcon, Coord x, Coord y );
__swi(_ENABLEHAVG) int32 __EnableHAVG( Item screenItem );
__swi(_ENABLEVAVG) int32 __EnableVAVG( Item screenItem );
__swi(_FILLRECT) int32 __FillRect( Item bitmapItem, GrafCon *gc, Rect *r );
__swi(_GETCURRENTFONT) Font *__GetCurrentFont( void );
__swi(_REMOVESCREENGROUP) int32 __RemoveScreenGroup( Item screenGroup );
__swi(_RESETREADADDRESS) int32 __ResetReadAddress( Item bitmapItem );
__swi(_RESETSCREENCOLORS) int32 __ResetScreenColors( Item screenItem );
__swi(_SETCECONTROL) int32 __SetCEControl( Item bitmapItem, int32 controlWord, int32 controlMask );
__swi(_SETCEWATCHDOG) int32 __SetCEWatchDog( Item bitmapItem, int32 db_ctr );
__swi(_SETCLIPHEIGHT) int32 __SetClipHeight( Item bitmapItem, int32 clipHeight );
__swi(_SETCLIPORIGIN) int32 __SetClipOrigin( Item bitmapItem, int32 x, int32 y );
__swi(_SETCLIPWIDTH) int32 __SetClipWidth( Item bitmapItem, int32 clipWidth );
__swi(_SETCURRENTFONTCCB) int32 __SetCurrentFontCCB( CCB *ccb );
/* __swi(_SETFILEFONTCACHESIZE) int32 __SetFileFontCacheSize( int32 size ); */
__swi(_SETREADADDRESS) int32 __SetReadAddress( Item bitmapItem, ubyte *buffer, int32 width );
__swi(_SETSCREENCOLOR) int32 __SetScreenColor( Item screenItem, uint32 colorentry );
__swi(_SETSCREENCOLORS) int32 __SetScreenColors( Item screenItem, uint32 *entries, int32 count );
__swi(_SETVDL) Item  __SetVDL( Item screenItem, Item vdlItem );
/* __swi(_SUBMITVDL) Item  __SubmitVDL( VDLEntry *VDLDataPtr, int32 length, int32 type ); */
__swi(_MODIFYVDL) Err __ModifyVDL (Item vdlItem, TagArg* vdlTags);
__swi(_SETVBLATTRS) Err __SetVBLAttrs (struct TagArg *args);
__swi(_GETVBLATTRS) Err __GetVBLAttrs (struct TagArg *args);


int32 kDrawScreenCels( Item screenItem, CCB *ccb );
int32 kDrawCels( Item bitmapItem, CCB *ccb );
int32 kAddScreenGroup( Item screenGroup, TagArg *targs );
int32 kDisableHAVG( Item screenItem );
int32 kDisableVAVG( Item screenItem );
int32 kDisplayScreen( Item screenItem0, Item screenItem1 );
int32 kDrawChar( GrafCon *gcon, Item bitmapItem, uint32 character );
int32 kDrawText16( GrafCon *gcon, Item bitmapItem, uint16 *text );
int32 kDrawText8( GrafCon *gcon, Item bitmapItem, uint8 *text );
int32 kDrawTo( Item bitmapItem, GrafCon *grafcon, Coord x, Coord y );
int32 kEnableHAVG( Item screenItem );
int32 kEnableVAVG( Item screenItem );
int32 kFillRect( Item bitmapItem, GrafCon *gc, Rect *r );
Font *kGetCurrentFont( void );
int32 kRemoveScreenGroup( Item screenGroup );
int32 kResetReadAddress( Item bitmapItem );
int32 kResetScreenColors( Item screenItem );
int32 kSetCEControl( Item bitmapItem, int32 controlWord, int32 controlMask );
int32 kSetCEWatchDog( Item bitmapItem, int32 db_ctr );
int32 kSetClipHeight( Item bitmapItem, int32 clipHeight );
int32 kSetClipOrigin( Item bitmapItem, int32 x, int32 y );
int32 kSetClipWidth( Item bitmapItem, int32 clipWidth );
int32 kSetCurrentFontCCB( CCB *ccb );
/* int32 kSetFileFontCacheSize( int32 size ); */
int32 kSetReadAddress( Item bitmapItem, ubyte *buffer, int32 width );
int32 kSetScreenColor( Item screenItem, uint32 colorentry );
int32 kSetScreenColors( Item screenItem, uint32 *entries, int32 count );
Item  kSetVDL( Item screenItem, Item vdlItem );
Item  kSubmitVDL( VDLEntry *VDLDataPtr, int32 length, int32 type );

Err kModifyVDL (Item vdlItem, TagArg* vdlTags);
Err kSetVBLAttrs (struct TagArg *args);
Err kGetVBLAttrs (struct TagArg *args);

DisplayInfo* kGetFirstDisplayInfo (void);




#ifdef __cplusplus
}
#endif

#endif /* of #ifndef __INTGRAF_H */
