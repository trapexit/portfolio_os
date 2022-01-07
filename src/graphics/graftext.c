/* $Id: graftext.c,v 1.7 1994/09/22 19:55:57 ewhac Exp $ */

/* *************************************************************************
 *
 * Text routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 930629 SHL              Changed AllocMem() reference to SUPER_ALLOCMEM()
 * 921212 -RJ Mical        Created this file!
 *
 * ********************************************************************** */


#define SSSS(x) Superkprintf x


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
#include "super.h"

#include "intgraf.h"

#include "stdarg.h"
#include "strings.h"

#include "filesystem.h"
#include "filesystemdefs.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"




/*???Some of the GrafBase Font fields are redundant (eg gf_FileFontFlags)*/


#define FF_FILEBASED 0x00000001

#define FONTENTRY_FULLSIZE ((sizeof(FontEntry)+3)&~3)

#define FONT_CHAR_COUNT 49
extern FontChar FontArray[FONT_CHAR_COUNT + 1];
extern CelData Image_WhatTheHell[];

void PrintTreeFont( FontEntry *entry );


FontEntry *DefaultFontEntryFont = 0;


int32 GuestFontPLUT[MAX_PLUT_SIZE];
CCB   GuestFontCCB;
Font  GuestFont;


FontEntry FontEntryHeadStruct;
FontEntry FontEntryButtStruct;



int32 FontPLUT[] =
	{
	0x18C66318, 0x63186318, 0x63186318, 0x63186318,
	};

CCB FontCCB =
	{
	/* ulong ccb_Flags; */
	CCB_LAST | CCB_NPABS | CCB_SPABS | CCB_PPABS | CCB_LDSIZE | CCB_LDPRS
			| CCB_LDPPMP | CCB_LDPLUT | CCB_YOXY | CCB_ACW | CCB_ACCW
			| PMODE_ZERO,

	/* struct CCB *ccb_NextPtr; CelData *ccb_SourcePtr; void *ccb_PLUTPtr; */
	&FontCCB, NULL, FontPLUT,

	/* Coord ccb_XPos; Coord ccb_YPos; */
	/* long ccb_HDX, ccb_HDY, ccb_VDX, ccb_VDY, ccb_DDX, ccb_DDY; */
	0,0,
	ONE_12_20,0, 0,ONE_16_16, 0,0,

	/* ulong ccb_PPMPC; */
	(PPMP_MODE_NORMAL << PPMP_0_SHIFT)|(PPMP_MODE_AVERAGE << PPMP_1_SHIFT),
	};

Font DefaultFont =
	{
	/* uint8     font_Height; */
	8,
	/* uint8     font_Flags; */
	FONT_ASCII | FONT_ASCII_UPPERCASE,
	/* CCB      *font_CCB; */
	&FontCCB,
	/* FontEntry *font_FontEntries; */
	NULL,
	};




void
ResetFontLinks()
{
	GrafBase->gf_FontEntryButt->ft_LesserBranch = GrafBase->gf_FontEntryButt;
	GrafBase->gf_FontEntryButt->ft_GreaterBranch = GrafBase->gf_FontEntryButt;
	GrafBase->gf_FontEntryHead->ft_GreaterBranch = GrafBase->gf_FontEntryButt;
	GrafBase->gf_FontEntryHead->ft_CharValue = -1;

	InitList( &GrafBase->gf_FontLRUList, "FontLeastRecentlyUsedList" );
}



void
SetGuestFontCCB( CCB *ccb, int32 *plut )
{
	memcpy( &GuestFontCCB, ccb, sizeof(CCB) );

	/* After overstriking the GuestFontCCB struct, patch appropriate fields */
	GuestFontCCB.ccb_PLUTPtr = GuestFontPLUT;
	GuestFontCCB.ccb_NextPtr = &GuestFontCCB;
	if ( plut != 0 ) memcpy( &GuestFontPLUT, plut, MAX_PLUT_SIZE );
}



void
SetGuestFont( Font *font )
{
	memcpy( &GuestFont, font, sizeof(Font) );
	/* After overstriking the GuestFont struct, patch the appropriate fields */
	GuestFont.font_CCB = &GuestFontCCB;
	SetGuestFontCCB( font->font_CCB, (int32 *)(font->font_CCB->ccb_PLUTPtr) );
}



void
SetGuestAsCurrentFont()
{
	GrafBase->gf_CurrentFont = &GuestFont;
}



int32
swiSuperResetCurrentFont()
{
	GrafBase->gf_CurrentFontStream = 0;
	ResetFontLinks();
	GrafBase->gf_CurrentFont = &DefaultFont;
	ClearFlag( GrafBase->gf_FileFontFlags, FF_FILEBASED );
	return( 0 );
}



int32
ResetCurrentFont()
{
	superResetCurrentFont();
	if ( DefaultFontEntryFont )
		return( superOpenRAMFont( &DefaultFont ) );
	else return( GRAFERR_NO_FONT );
}



int32
DrawText8( GrafCon *gcon, Item bitmapItem, uint8 *text )
{
	uint8 character;
	int32 retvalue;

	retvalue = 0;
	while ( (character = *text++) && (retvalue == 0) )
		retvalue = DrawChar( gcon, bitmapItem, character );
	return( retvalue );
}



int32
SetCurrentFontCCB( CCB *ccb )
{
	SetGuestFontCCB( ccb, (int32 *)(ccb->ccb_PLUTPtr) );
	return( 0 );
}



Font *
GetCurrentFont( void )
{
	return( GrafBase->gf_CurrentFont );
}



void
ButtTheTree( FontEntry *font )
{
	if ( font != GrafBase->gf_FontEntryButt )
		{
		if ( font->ft_LesserBranch == 0 )
			font->ft_LesserBranch = GrafBase->gf_FontEntryButt;
		else ButtTheTree( font->ft_LesserBranch );
		if ( font->ft_GreaterBranch == 0 )
			font->ft_GreaterBranch = GrafBase->gf_FontEntryButt;
		else ButtTheTree( font->ft_GreaterBranch );
		}
}



void
DeButtTheTree( FontEntry *font )
{
	if ( font != 0 )
		{
		if ( font->ft_LesserBranch == GrafBase->gf_FontEntryButt )
			font->ft_LesserBranch = 0;
		else DeButtTheTree( font->ft_LesserBranch );
		if ( font->ft_GreaterBranch == GrafBase->gf_FontEntryButt )
			font->ft_GreaterBranch = 0;
		else DeButtTheTree( font->ft_GreaterBranch );
		}
}



int32
swiSuperCloseFont()
{
  int32 retvalue;

/*??? Don't do this for now, until Dale implements the user-level
 * memory management routines
 */
/*???	if ( FileFontCache )*/
/*???		{*/
/*???		FreeMem( FileFontCache, FileFontCacheAlloc );*/
/*???		FileFontCache = 0;*/
/*???		}*/

  /* Here we presume that the user-level routine closed any stream file */
  GrafBase->gf_CurrentFontStream = 0;

  DeButtTheTree( GrafBase->gf_FontEntryHead->ft_GreaterBranch );
  ResetFontLinks();
  retvalue = 0;

/*???DONE:*/
  return( retvalue );
}


#if 0
int32
CloseFont()
{
	int32 retvalue;

	if ( GrafBase->gf_CurrentFontStream )
		{
		CloseDiskStream( GrafBase->gf_CurrentFontStream );
		}

	retvalue = superCloseFont( );
	if ( retvalue >= 0 )
		{
		retvalue = ResetCurrentFont();
		}

	return( retvalue );
}
#endif



int32
swiSuperOpenFileFont( SWOFF *swoff  )
{
	int32 retvalue;

	Stream *stream;
	int32 basechar;
	int32 charcount;
	Font *font;
	int32 chararrayoffset;

	stream = swoff->stream;
	basechar = swoff->basechar;
	charcount = swoff->charcount;
	font = swoff->font;
	chararrayoffset = swoff->chararrayoffset;

	SDEBUG(("superOpenFileFont( "));
	SDEBUG(("stream=$%lx ", (unsigned long)(stream)));
	SDEBUG(("basechar=%ld ", (unsigned long)(basechar)));
	SDEBUG(("charcount=%ld ", (unsigned long)(charcount)));
	SDEBUG(("font=$%lx ", (unsigned long)(font)));
	SDEBUG((" )\n"));

	GrafBase->gf_CharArrayOffset = chararrayoffset;
	GrafBase->gf_CurrentFontStream = stream;
	GrafBase->gf_FontBaseChar = basechar;
	GrafBase->gf_FontMaxChar = GrafBase->gf_FontBaseChar + charcount - 1;

/*??? Don't do this for now, until Dale implements the user-level
 * memory management routines
 */
/*???	FileFontCache = AllocMem( FileFontCacheSize, MEMTYPE_CEL );*/
/*???	if ( FileFontCache == 0 )*/
/*???		{*/
/*???		retvalue = GRAFERR_NOMEM;*/
/*???		goto DONE;*/
/*???		}*/

	GrafBase->gf_FileFontCacheAlloc = GrafBase->gf_FileFontCacheSize;

/*??? For now, use this fakey size kludge until Dale implements the new
 * user-level management stuff
 */
GrafBase->gf_fileFontCacheUsed = 0;

	ResetFontLinks();
	SetFlag( GrafBase->gf_FileFontFlags, FF_FILEBASED );

	SetGuestFont( font );
	SetGuestAsCurrentFont();
	SetFlag( GuestFont.font_Flags, FONT_FILEBASED );

	retvalue = 0;

/*??? DONE: */
	return( retvalue );
}


#if 0
int32
OpenFileFont( char *filename )
{
	int32 charcount, basechar, retvalue;
	Stream *stream;
	CCB ccb;
	Font font;
	uint8 size;
	int32 plut[MAX_PLUT_SIZE];
	int32 chararrayoffset;
	SWOFF swoff;

	DEBUG(("OpenFileFont( "));
	DEBUG(("filename=\"%s\" ", filename));
	DEBUG((" )\n"));

	/* For safety, close any open font */
	CloseFont();

	stream = OpenDiskStream( filename, 0 );
	if ( stream == 0 )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}

	if ( ReadDiskStream( stream, (char *)&font.font_Height, 1 ) != 1 )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}
	if ( ReadDiskStream( stream, (char *)&font.font_Flags, 1 ) != 1 )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}

	if ( ReadDiskStream( stream, (char *)&size, 1 ) != 1 )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}
	if ( size > sizeof(CCB) )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}
	if ( ReadDiskStream( stream, (char *)&ccb, size ) != size )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}

	if ( ReadDiskStream( stream, (char *)&size, 1 ) != 1 )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}
	if ( size > MAX_PLUT_SIZE )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}
	if ( ReadDiskStream( stream, (char *)plut, size ) != size )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}

	if ( ReadDiskStream( stream, (char *)&basechar, 4 ) != 4 )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}
	if ( ReadDiskStream( stream, (char *)&charcount, 4 ) != 4 )
		{
		retvalue = GRAFERR_BADFONTFILE;
		goto DONE;
		}

	chararrayoffset = SeekDiskStream( stream, 0, SEEK_CUR );

	ccb.ccb_PLUTPtr = plut;
	font.font_CCB = &ccb;
	font.font_FontEntries = 0;

	swoff.stream = stream;
	swoff.basechar = basechar;
	swoff.charcount = charcount;
	swoff.font = &font;
	swoff.chararrayoffset = chararrayoffset;

	retvalue = superOpenFileFont( &swoff );

	if ( retvalue < 0 ) goto DONE;

DONE:
	if ( retvalue < 0 ) CloseFont();
	return( retvalue );
}
#endif


int32
SetFileFontCacheSize( int32 size )
{
	int32 retvalue;

	SDEBUG(("SetFileFontCacheSize( "));
	SDEBUG(("size=%ld ", (unsigned long)(size)));
	SDEBUG((" )\n"));

	/*??? Must do something special if file font is already opened */
	retvalue = GrafBase->gf_FileFontCacheSize;
	GrafBase->gf_FileFontCacheSize = size;
	return( retvalue );
}



int32
swiSuperOpenRAMFont( Font *font )
{
	int32 retvalue;

	SDEBUG(("swiSuperOpenRAMFont( "));
	SDEBUG(("font=$%lx ", (unsigned long)(font)));
	SDEBUG((" )\n"));

	/* For safety, user-level caller should close any open font */

	ResetFontLinks();
	/* After resetting the links, attach this font's entries tree to the
	 * root of the graphics folio's font tree
	 */
	GrafBase->gf_FontEntryHead->ft_GreaterBranch = font->font_FontEntries;
	ButtTheTree( font->font_FontEntries );

	ClearFlag( GrafBase->gf_FileFontFlags, FF_FILEBASED );

	SetGuestFont( font );
	SetGuestAsCurrentFont();
	ClearFlag( GuestFont.font_Flags, FONT_FILEBASED );

	retvalue = 0;

/*???DONE:*/
	return( retvalue );
}


#if 0
int32
OpenRAMFont( Font *font )
{
	/* For safety, close any open font */
	CloseFont();

	return( superOpenRAMFont( font ) );
}
#endif


void
InsertFontEntry( FontEntry *newentry )
{
	FontEntry *preventry, *thisentry;
	int32 character;

	character = newentry->ft_CharValue;

	preventry = GrafBase->gf_FontEntryHead;
	thisentry = GrafBase->gf_FontEntryHead->ft_GreaterBranch;
	while ( thisentry != GrafBase->gf_FontEntryButt )
		{
		preventry = thisentry;
		if ( character < thisentry->ft_CharValue )
			thisentry = thisentry->ft_LesserBranch;
		else thisentry = thisentry->ft_GreaterBranch;
		}
	newentry->ft_LesserBranch = GrafBase->gf_FontEntryButt;
	newentry->ft_GreaterBranch = GrafBase->gf_FontEntryButt;
	if ( character < preventry->ft_CharValue )
		preventry->ft_LesserBranch = newentry;
	else preventry->ft_GreaterBranch = newentry;
}



void
UnlinkFontEntry( FontEntry *newentry )
{
	FontEntry *preventry, *thisentry, *workentry, *unlinkme;
	int32 character;

	character = newentry->ft_CharValue;

	GrafBase->gf_FontEntryButt->ft_CharValue = newentry->ft_CharValue;
	preventry = GrafBase->gf_FontEntryHead;
	thisentry = GrafBase->gf_FontEntryHead->ft_GreaterBranch;

	while ( thisentry->ft_CharValue != character )
		{
		preventry = thisentry;
		if ( character < thisentry->ft_CharValue )
			thisentry = thisentry->ft_LesserBranch;
		else thisentry = thisentry->ft_GreaterBranch;
		}
	unlinkme = thisentry;
	if ( unlinkme->ft_GreaterBranch == GrafBase->gf_FontEntryButt )
		thisentry = thisentry->ft_LesserBranch;
	else if ( unlinkme->ft_GreaterBranch->ft_LesserBranch == GrafBase->gf_FontEntryButt )
		{
		thisentry = thisentry->ft_GreaterBranch;
		thisentry->ft_LesserBranch = unlinkme->ft_LesserBranch;
		}
	else
		{
		workentry = thisentry->ft_GreaterBranch;
		while ( workentry->ft_LesserBranch->ft_LesserBranch
				!= GrafBase->gf_FontEntryButt )
			workentry = workentry->ft_LesserBranch;
		thisentry = workentry->ft_LesserBranch;
		workentry->ft_LesserBranch = thisentry->ft_GreaterBranch;
		thisentry->ft_LesserBranch = unlinkme->ft_LesserBranch;
		thisentry->ft_GreaterBranch = unlinkme->ft_GreaterBranch;
		}
	if ( character < preventry->ft_CharValue )
		preventry->ft_LesserBranch = thisentry;
	else preventry->ft_GreaterBranch = thisentry;
}


#if 0
/*??? Kill this */
void
PrintTreeFont( FontEntry *entry )
{
	if ( entry != GrafBase->gf_FontEntryButt )
		{
		PrintTreeFont( entry->ft_LesserBranch );
		Superkprintf( ":%d", entry->ft_CharValue );
		PrintTreeFont( entry->ft_GreaterBranch );
		}
}


void
PrintLRUList()
{
	Node *node;
	FontEntry *fontchar;
	int32 i;

	i = 0;
	for ( node = FirstNode( &GrafBase->gf_FontLRUList );
			IsNode( &GrafBase->gf_FontLRUList, node );
			node = NextNode( node ) )
		{
		fontchar = (FontEntry *)node;
		if ( i ) Superkprintf( "->" );
		i++;
		Superkprintf("%ld", (unsigned long)(fontchar->ft_CharValue));
		}
	Superkprintf("\n");
}
#endif


#if 0
FontEntry *
LoadFontChar( int32 character )
{
  FontEntry *fontchar, *workentry;
  FileFontHeader header;
  int32 i, i2, size, offset;
  ubyte *ptr;

  SSSS (("In\n"));

  SDEBUG(("LoadFontChar( "));
  SDEBUG(("character=%ld ", (unsigned long)(character)));
  SDEBUG((" )\n"));

  if ( character < GrafBase->gf_FontBaseChar ) {
    return 0;
  }
  if ( character > GrafBase->gf_FontMaxChar ) {
    return 0;
  }

  /* Seek to the header of this character:
   *   - Seek to the position of the offset
   *   - Read the offset of the character
   *   - Seek to the character's data
   */
  offset = GrafBase->gf_CharArrayOffset + ((character - GrafBase->gf_FontBaseChar) * 4);
  i = SeekDiskStream( GrafBase->gf_CurrentFontStream, offset, SEEK_SET );
  if ( i < 0 ) {
    return 0;
  }
  if ( (i2 = ReadDiskStream( GrafBase->gf_CurrentFontStream, (char *)&i, 4 )) != 4 ) {

/* RETRY??? */
/* RETRY??? */
/* RETRY??? */
    offset = GrafBase->gf_CharArrayOffset + ((character - GrafBase->gf_FontBaseChar) * 4);
    i = SeekDiskStream( GrafBase->gf_CurrentFontStream, offset, SEEK_SET );
    if ( i < 0 ) {
      return 0;
    }
    if ( (i2 = ReadDiskStream( GrafBase->gf_CurrentFontStream, (char *)&i, 4 )) != 4 ) {
      return 0;
    }
/* RETRY??? */
/* RETRY??? */
/* RETRY??? */

/*???		return( 0 );*/

  }

  i = SeekDiskStream( GrafBase->gf_CurrentFontStream, i, SEEK_SET );
  if ( i < 0 ) {
    return 0;
  }

  if ( ReadDiskStream(GrafBase->gf_CurrentFontStream,(char *)&header,sizeof(FileFontHeader))
      != sizeof(FileFontHeader) ) {
    return 0;
  }

  size = FONTENTRY_FULLSIZE + header.ffh_ImageSize;

	/* Get fontchar to point to an available space, or return zero */
/*??? For now, use this fakey size kludge until Dale implements the new
 * user-level management stuff
 */

  while ( GrafBase->gf_fileFontCacheUsed + size >= GrafBase->gf_FileFontCacheAlloc ) {
    workentry = (FontEntry *)RemHead( &GrafBase->gf_FontLRUList );
    if ( workentry == 0 ) {
      return 0;
    }
    /* Disconnect workentry from the current character entry */
    UnlinkFontEntry( workentry );

    i = workentry->ft_ImageByteCount + FONTENTRY_FULLSIZE;
    FreeMem( (void *)workentry, i );
    GrafBase->gf_fileFontCacheUsed -= i;
  }
  fontchar = (FontEntry *)SUPER_ALLOCMEM( size, MEMTYPE_CEL );
  if ( fontchar == 0 ) {
    return 0;
  }
  GrafBase->gf_fileFontCacheUsed += size;

  /* fontchar points to a valid place.  load it and link it */
  ptr = (ubyte *)((long)fontchar + FONTENTRY_FULLSIZE);
  if ( ReadDiskStream( GrafBase->gf_CurrentFontStream, ptr, header.ffh_ImageSize )
      != header.ffh_ImageSize ) {
    return 0;
  }

  fontchar->ft_CharValue = character;
  fontchar->ft_Width = header.ffh_Width;
  fontchar->ft_Image = (CelData *)(ptr);
  fontchar->ft_ImageByteCount = header.ffh_ImageSize;

  /* Insert this character into the entry, and add it to the end of the
   * Least Recently Used list
   */
  InsertFontEntry( fontchar );
  AddTail( &GrafBase->gf_FontLRUList, &fontchar->ft );

/*???PrintTreeFont( FontEntryHead->ft_GreaterBranch );*/
/*???Superkprintf("\n");*/
  return fontchar;
  SSSS (("Out\n"));
}
#endif



int32
DrawChar( GrafCon *gcon, Item bitmapItem, uint32 character )
{
  FontEntry *fontchar;
  CCB *ccb;

  ccb = GrafBase->gf_CurrentFont->font_CCB;

  if (FlagIsSet( GrafBase->gf_CurrentFont->font_Flags, FONT_ASCII )) {
    if (FlagIsSet( GrafBase->gf_CurrentFont->font_Flags, FONT_ASCII_UPPERCASE )) {
      if ( ( character >= 'a' ) && ( character <= 'z' ) ) {
	character = character - ('a' - 'A');
      }
    }
  }

  /* Try to match this character in the current entry */
  fontchar = GrafBase->gf_FontEntryHead->ft_GreaterBranch;
  while ( fontchar != GrafBase->gf_FontEntryButt ) {
    if ( character == fontchar->ft_CharValue ) {
      /* Modify LRU list by moving node to end of list */
      if ( FlagIsSet( GrafBase->gf_FileFontFlags, FF_FILEBASED ) ) {
	RemNode( &fontchar->ft );
	AddTail( &GrafBase->gf_FontLRUList, &fontchar->ft );
      }
      goto THIS_CHARACTER;
    }
    else if ( character < fontchar->ft_CharValue ) {
      fontchar = fontchar->ft_LesserBranch;
    } else {
      fontchar = fontchar->ft_GreaterBranch;
    }
  }

#if 0
  /* Character not matched, try to load it from disk */
  if ( FlagIsSet( GrafBase->gf_FileFontFlags, FF_FILEBASED ) ) {
    fontchar = LoadFontChar( character );
    if ( fontchar ) {
      goto THIS_CHARACTER;
    }
  }
#endif

  /* ... else set fontchar to
   * point at the WhatTheHell char
   */
  fontchar = 0;

THIS_CHARACTER:
  ccb->ccb_XPos = gcon->gc_PenX << 16;
  ccb->ccb_YPos = gcon->gc_PenY << 16;
  if ( fontchar )  {
    ccb->ccb_SourcePtr = fontchar->ft_Image;
    if (FlagIsSet( GrafBase->gf_CurrentFont->font_Flags, FONT_VERTICAL )) {
      gcon->gc_PenY += GrafBase->gf_CurrentFont->font_Height;
    } else {
      gcon->gc_PenX += fontchar->ft_Width;
    }
  } else {
    if (FlagIsSet( GrafBase->gf_CurrentFont->font_Flags, FONT_VERTICAL )) {
      gcon->gc_PenY += GrafBase->gf_CurrentFont->font_Height;
    } else {
      gcon->gc_PenX += 8;
    }
    return 0;
  }

  return (SWIDrawCels (bitmapItem, ccb));
}



int32
DrawText16( GrafCon *gcon, Item bitmapItem, uint16 *text )
{
	uint16 character;
	int32 retvalue;

	retvalue = 0;
	while ( (character = *text++) && (retvalue == 0) )
		retvalue = DrawChar( gcon, bitmapItem, character );
	return( retvalue );
}


int32
InitFontStuff()
{
	GrafBase->gf_CurrentFontStream = 0;
	GrafBase->gf_FileFontCacheSize = 16384;
	GrafBase->gf_FileFontCacheAlloc = 0;
	GrafBase->gf_fileFontCacheUsed = 0;
	GrafBase->gf_FileFontCache = 0;
	GrafBase->gf_FontEntryHead = &FontEntryHeadStruct;
	GrafBase->gf_FontEntryButt = &FontEntryButtStruct;
	GrafBase->gf_FileFontFlags = 0;
	GrafBase->gf_CurrentFont = &DefaultFont;

	ResetFontLinks();

	return( InitDefaultFont() );
}


