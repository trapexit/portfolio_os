/* $Id: debuggerfolio.h,v 1.3 1994/08/02 02:38:34 anup Exp $ */

/*
 * $Log: debuggerfolio.h,v $
 * Revision 1.3  1994/08/02  02:38:34  anup
 * Added Id and Log keywords
 *
 */

/*
	File:		DebuggerFolio.h

	Contains:	Public Declarations for Debugger Folio

	Written by:	Anup K Murarka

	Copyright:	© 1993-94 by The 3DO Company, all rights reserved
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				13.03.94	akm		Finally back.  Added new swi info
				29.01.94	akm		First Writing

	To Do:
*/

#pragma force_top_level
#pragma include_only_once

#ifndef __DebuggerFolio_h
#define __DebuggerFolio_h


#include "types.h"
#include "stdlib.h"
#include "debug.h"
#include "item.h"
#include "nodes.h"
#include "interrupts.h"
#include "kernel.h"
#include "mem.h"
#include "list.h"
#include "task.h"
#include "folio.h"
#include "kernelnodes.h"
#include "stdarg.h"
#include "string.h"
#include "operror.h"
#include "io.h"
#include "super.h"
#include "semaphore.h"
#include "stdio.h"

#define kDebuggerFolioName "debugger"

typedef struct DebuggerFolioFlags
{
	int	reserved       : 31;
	int	SWITraceEnable :  1;
} DebuggerFolioFlags;


// Debugger Item structure(s)
typedef struct SkelBone
{
  ItemNode 	task_Node;      /* required at the base of each item */
  int32    	task_Start;    /* following data is custom */
  int32    	task_ID;
  List     	task_ConnectList;
} SkelBone;

// Debugger Folio Tags
// Start numbering above last kernels tags
enum debugger_folio_tags
{
	DBGR_TAG_REGISTER = TAG_ITEM_LAST+1   // 10, this will be used to register tasks
};

// Folio ID.  This must be a unique number assigned by Dale Luck at 3DO
#define  DEBUGGERNODE   (0)

/*
	Item type numbers for skeleton folio
	When new types are added, add to SkelNodeData in SkeletonFolio.c ??
	Also add to internalCreateSkelItem()
*/
#define DEBUGGER_TASK_NODE    (1)

enum debugger_task_tags
{
	DB_TAG_LENGTH = (TAG_ITEM_LAST+1),		// 10
	DB_TAG_WEIGHT
};

/*
	Debugger SWIs
	
	All swi's from 0x0000 FFFF to 0x0000 0100 are for Debugger use
	swi's from 0x00000000 to 0x000000FF are reserved for system (ARM)
*/
#define DebuggerSWI (0x0100 + (DEBUGGERNODE<<16))

//@@@@@ these are for testing only, they are going away
int32 __swi(DebuggerSWI+0x000e) SetSkelItemInfo( Item AnyItem,  TagArg *TagList );
int32 __swi(DebuggerSWI+0x000f) ConnectTasks( Item Bone1, Item Bone2);


/**********************************************************************/
/******** Prototypes of User level folio calls ************************/
/**********************************************************************/
// routine numbers for folio user level calls
#define GETSKELITEMINFO      (-1)
#define CREATETASK           (-2)
#define DELETETASK           (-3)

int32 OpenDebuggerFolio(void);
int32 CloseDebuggerFolio(void);

// Conveniance routines for creating and deleting items
Item CreateDbgTask( char *Name, int32 Length, int32 Weight);
int32 DeleteDbgTask(Item Bone);

// Query routine
int32 GetSkelItemInfo( Item AnyItem, TagArg *tp );

#endif

