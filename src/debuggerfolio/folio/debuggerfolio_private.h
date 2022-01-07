/* $Id: debuggerfolio_private.h,v 1.4 1994/11/01 15:49:08 vertex Exp $ */

/*
 * $Log: debuggerfolio_private.h,v $
 * Revision 1.4  1994/11/01  15:49:08  vertex
 * Removed definition of "nil"
 * Changed C++ comments to C comments
 *
 * Revision 1.3  1994/08/02  02:39:30  anup
 * Added Id and Log keywords
 *
 */

/*
	File:		DebuggerFolio.private.h

	Contains:	Private Declarations for Debugger Folio

	Written by:	Anup K Murarka

	Copyright:	© 1993-94 by The 3DO Company, all rights reserved
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

				29.01.94	akm		First Writing

	To Do:
*/

#pragma force_top_level
#pragma include_only_once

#ifndef __DebuggerFolio_Private__
#define __DebuggerFolio_Private__

/* Macros for debugging. */
#define PRT(x)    { printf x ; }
#define SPRT(x)   { Superkprintf x ; }
#define DBUG(x)   PRT(x)
#define SDBUG(x)  SPRT(x)
#define ERR(x)   	SPRT(x)


typedef struct DbgrFolio
{
  Folio  dbgr_Folio;         /* required at the base of each folio     */
  List   dbgr_TaskList;      /* custom data for this folio begins here */
} DbgrFolio;

/* Skeleton folio globals */
extern DbgrFolio *DbgrBase;
extern Item DebuggerFolioNum;


/*
	Error Returns
*/
#define ER_DBGR ((Make6Bit('D')<<6)|Make6Bit('b'))
#define MakeDbErr(svr,class,err) MakeErr(ER_FOLI,ER_DBGR,svr,ER_E_SSTM,class,err)

#define DB_ERR_BADTAG     MakeDbErr(ER_SEVER,ER_C_STND,ER_BadTagArg)
#define DB_ERR_BADTAGVAL  MakeDbErr(ER_SEVER,ER_C_STND,ER_BadTagArgVal)
#define DB_ERR_BADPRIV    MakeDbErr(ER_SEVER,ER_C_STND,ER_NotPrivileged)
#define DB_ERR_BADSUBTYPE MakeDbErr(ER_SEVER,ER_C_STND,ER_BadSubType)
#define DB_ERR_BADITEM    MakeDbErr(ER_SEVER,ER_C_STND,ER_BadItem)
#define DB_ERR_NOMEM      MakeDbErr(ER_SEVER,ER_C_STND,ER_NoMem)
#define DB_ERR_BADPTR     MakeDbErr(ER_SEVER,ER_C_STND,ER_BadPtr)
#define DB_ERR_NOTSUPPORTED MakeDbErr(ER_SEVER,ER_C_STND,ER_NotSupported)


/*
	Following enums define selectors for subroutines
	available for execution directly from the monitor
*/
typedef enum MonitorSelector
{
	mf_GetKernelArea = 0
} MonitorSelector;

enum DebuggerTags
{
	DB_TAG_KBASE = (1),
	DB_TAG_KB_CURRENTTASK,
	DB_TAG_TASK_T,
	DB_TAG_TASK_T_STACKBASE,
	DB_TAG_TASK_T_STACKSIZE,
	DB_TAG_ITEMNODE_N_NAME
};

/*
	This is the KernelArea global the
	debugger queries whenever it needs information
	about a task.
*/
typedef struct KernelArea
{
	void*	KernelBasePtr;
	void*	CurrentTask_Off;
	void*	Task_t_Off;
	void*	StackBase_Off;
	void*	StackSize_Off;
	void*	ItemNode_Name_off;
} KernelArea;

/* Internal DebuggerFolio swi calls */
void	__swi(DebuggerSWI+0x0009) TellDebug(TagArg *TagList);
int32	__swi(DebuggerSWI+0x0010) DebugTaskLaunch( Task* taskControl, void*	startAddr );
int32	__swi(DebuggerSWI+0x0011) DebugTaskKill( Item taskID );


#endif
