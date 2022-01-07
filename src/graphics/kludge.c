/* *************************************************************************
 *
 * Graphics routines for the Opera Hardware
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * $Id: kludge.c,v 1.9 1994/08/25 22:55:16 ewhac Exp $
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 930723 SHL              Created this file to sweep the details of the UI
 *                         under the rug of folio calls
 *
 * ********************************************************************** */


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
#include "super.h"

#include "intgraf.h"

#include "stdarg.h"
#include "strings.h"

#include "inthard.h"
#include "clio.h"



int32
kAddScreenGroup( Item screenGroup, TagArg *targs )
{
  return __AddScreenGroup (screenGroup, targs);
}

int32
kDisableHAVG( Item screenItem )
{
  return __DisableHAVG( screenItem );
}

int32
kDisableVAVG( Item screenItem )
{
  return __DisableVAVG( screenItem );
}

int32
kDisplayScreen( Item screenItem0, Item screenItem1 )
{
  return __DisplayScreen( screenItem0, screenItem1 );
}

int32
kDrawChar( GrafCon *gcon, Item bitmapItem, uint32 character )
{
  return __DrawChar( gcon, bitmapItem, character );
}

int32
kDrawText16( GrafCon *gcon, Item bitmapItem, uint16 *text )
{
  return __DrawText16( gcon, bitmapItem, text );
}

int32
kDrawText8( GrafCon *gcon, Item bitmapItem, uint8 *text )
{
  return __DrawText8( gcon, bitmapItem, text );
}

int32
kDrawTo( Item bitmapItem, GrafCon *grafcon, Coord x, Coord y )
{
  return __DrawTo( bitmapItem, grafcon, x, y );
}

int32
kEnableHAVG( Item screenItem )
{
  return __EnableHAVG( screenItem );
}

int32
kEnableVAVG( Item screenItem )
{
  return __EnableVAVG( screenItem );
}

int32
kFillRect( Item bitmapItem, GrafCon *gc, Rect *r )
{
  return __FillRect( bitmapItem, gc, r );
}

Font*
kGetCurrentFont( void )
{
  return __GetCurrentFont( );
}

int32
kRemoveScreenGroup( Item screenGroup )
{
  return __RemoveScreenGroup( screenGroup );
}

int32
kResetReadAddress( Item bitmapItem )
{
  return __ResetReadAddress( bitmapItem );
}

int32
kResetScreenColors( Item screenItem )
{
  return __ResetScreenColors( screenItem );
}

int32
kSetCEControl( Item bitmapItem, int32 controlWord, int32 controlMask )
{
  return __SetCEControl( bitmapItem, controlWord, controlMask );
}

int32
kSetClipHeight( Item bitmapItem, int32 clipHeight )
{
  return __SetClipHeight( bitmapItem, clipHeight );
}

int32
kSetClipOrigin( Item bitmapItem, int32 x, int32 y )
{
  return __SetClipOrigin( bitmapItem, x, y );
}

int32
kSetClipWidth( Item bitmapItem, int32 clipWidth )
{
  return __SetClipWidth( bitmapItem, clipWidth );
}

int32
kSetCurrentFontCCB( CCB *ccb )
{
  return __SetCurrentFontCCB( ccb );
}

#if 0
int32
kSetFileFontCacheSize( int32 size )
{
  return __SetFileFontCacheSize( size );
}
#endif

int32
kSetReadAddress( Item bitmapItem, ubyte *buffer, int32 width )
{
  return __SetReadAddress( bitmapItem, buffer, width );
}

int32
kSetScreenColor( Item screenItem, uint32 colorentry )
{
  return __SetScreenColor( screenItem, colorentry );
}

int32
kSetScreenColors( Item screenItem, uint32 *entries, int32 count )
{
  return __SetScreenColors( screenItem, entries, count );
}

Item
kSetVDL( Item screenItem, Item vdlItem )
{
  return __SetVDL( screenItem, vdlItem );
}

Item
kSubmitVDL( VDLEntry *VDLDataPtr, int32 length, int32 type )
{
  TagArg ta[5];

  ta[0].ta_Tag = CREATEVDL_TAG_DATAPTR;
  ta[0].ta_Arg = (void *)VDLDataPtr;

  ta[1].ta_Tag = CREATEVDL_TAG_LENGTH;
  ta[1].ta_Arg = (void *)length;

  ta[2].ta_Tag = CREATEVDL_TAG_VDLTYPE;
  ta[2].ta_Arg = (void *)type;

  ta[3].ta_Tag = CREATEVDL_TAG_HEIGHT;
  ta[3].ta_Arg = (void *)240;

  ta[4].ta_Tag = CREATEVDL_TAG_DONE;
  ta[4].ta_Arg = (void *)0;

  return CreateItem (MKNODEID(NODE_GRAPHICS,TYPE_VDL), ta);
}

int32
kDrawCels( Item bitmapItem, CCB *ccb )
{
  return __DrawCels( bitmapItem, ccb );
}

int32
kDrawScreenCels( Item screenItem, CCB *ccb )
{
  return __DrawScreenCels( screenItem, ccb );
}

int32
kSetCEWatchDog( Item bitmapItem, int32 db_ctr )
{
  return __SetCEWatchDog( bitmapItem, db_ctr );
}


Err
kModifyVDL (Item vdlItem, TagArg* vdlTags)
{
  return __ModifyVDL (vdlItem, vdlTags);
}

Err
kSetVBLAttrs (struct TagArg *args)
{
	return (__SetVBLAttrs (args));
}

Err
kGetVBLAttrs (struct TagArg *args)
{
	return (__GetVBLAttrs (args));
}
