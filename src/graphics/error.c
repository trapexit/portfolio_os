/* $Id: error.c,v 1.15 1994/12/10 00:04:03 ewhac Exp $ */

/* *************************************************************************
 *
 * Error routines for the graphix folio
 *
 * Copyright (C) 1993, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with tab stops of 4 in mind
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 930222 -RJ              Created this file!
 *
 * ********************************************************************** */


#undef SUPER
#include "types.h"

#include "nodes.h"
#include "list.h"
#include "kernelnodes.h"
#include "strings.h"
#include "item.h"
#include "folio.h"
#include "kernel.h"

#include "operror.h"
#include "intgraf.h"


static char	nullstr[] = "";

/*
 * First 20 slots were incorrectly assigned to standard system error
 * messages.  They have been "reclaimed."
 */
static char *GF_ErrTable[] = {
  nullstr, nullstr, nullstr, nullstr,
  nullstr, nullstr, nullstr, nullstr,
  nullstr, nullstr, nullstr, nullstr,
  nullstr, nullstr, nullstr, nullstr,
  nullstr, nullstr, nullstr, nullstr,

  "Cel engine timed out",  		/* 20 */
  "Bad clip width/height/origin dimensions",
  "Bad VDL type (must be VDLTYPE_FULL)",
  "Index out of range",
  "Illegal bitmap buffer width",
  "Coordinate out of range",		/* +5 */
  "Illegal VDL display width",
  "Not implemented yet",
  "Mixed screens (must be in same ScreenGroup)",
  "Font file couldn't be opened",
  "Bad Cel engine watch dog value",	/* +10 */
  "This VDL is already being displayed",
  "The sumbitted VDL has failed proofing",
  "No VDL Length array specified",
  "No default font in this graphics folio",
  "Bad display/screen dimensions or screen count",	/* +15 */
  "Bad bitmap count or missing height array",
  "Internal error in graphics folio - please report circumstances",
  "ScreenGroup already in use",
  "ScreenGroup not in use",
  "Graphics folio not opened by current task",	/* +20 */
  "Task does not have write permission to specified memory",
  "Unrecognized display type or display type not available",
  "Cannot mix display types on alternate fields",
  nullstr
};


#define MAX_GF_ERR_SIZE	sizeof("Internal error in graphics folio - please report circumstances")



TagArg ErrTags[] =
{
    TAG_ITEM_NAME,	(void *)"Graphics Folio Error Table",
    ERRTEXT_TAG_OBJID,	(void *)((ER_FOLI<<ERR_IDSIZE)|(ER_GRFX)),
    ERRTEXT_TAG_MAXERR,	(void *)(sizeof(GF_ErrTable)/sizeof(char *)),
    ERRTEXT_TAG_TABLE,	(void *)GF_ErrTable,
    ERRTEXT_TAG_MAXSTR,	(void *)MAX_GF_ERR_SIZE,
    TAG_END,		0
};



/* Called by operator.c */
Item
InitGraphicsErrors( void )
{
	Item ErrorItem;

	ErrorItem = CreateItem( MKNODEID(KERNELNODE,ERRORTEXTNODE), ErrTags );
	if ( ErrorItem  < 0 )
		{
		SDEBUGGRAF(("GraphicsErrorInit: Cannot create error table (0x%lx)\n",
				ErrorItem));
		}
	return( ErrorItem );
}
