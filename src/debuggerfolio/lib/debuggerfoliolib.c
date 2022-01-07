/* $Id: debuggerfoliolib.c,v 1.5 1994/09/21 00:05:37 vertex Exp $ */

/*
 * $Log: debuggerfoliolib.c,v $
 * Revision 1.5  1994/09/21  00:05:37  vertex
 * Added version string reference
 *
 * Revision 1.4  1994/08/04  17:57:51  anup
 * Forgot Id and Log keywords
 *
 */

/****************************************************************
**
** Stub routines to interface to the Skeleton folio
** This file is linked with the users application.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

#include "debuggerfolio.h"
#include "debuggerfolio_private.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *debuggerlib_version;
static void *debulib_version = &debuggerlib_version;
#endif


/****************************************************************************/


#undef	ERR	/* to avoid "redefinition" error */

/* Macros for debugging. */

Item DebuggerFolioNum;
/* Externally referenced pointer to Debugger Folio Base structure. */
DbgrFolio *DbgrBase;

extern void	__main(void);


/**********************************************************************/


int32 OpenDebuggerFolio(void)
{
	int32 Result;

	DebuggerFolioNum = FindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),kDebuggerFolioName);
	if (DebuggerFolioNum < 0)
	{
    ERR(("Error Finding DebuggerFolio = 0x%x\n", DebuggerFolioNum));

		return DebuggerFolioNum;
	}

	Result = OpenItem(DebuggerFolioNum, 0);
	if (Result < 0)
	{
    	ERR (("Error Opening DebuggerFolio (%x)\n", Result));
		return Result;
	}

/*
** Set DbgrBase for values global to the entire Folio.
*/
	DbgrBase = (DbgrFolio *) LookupItem (DebuggerFolioNum);
	DBUG (("DbgrBase located at %lx\n", DbgrBase));

	DebugTaskLaunch(CURRENTTASK, (void*) &__main);		// swi call

	return 0;
}

/**********************************************************************/

int32 CloseDebuggerFolio(void)
{
	int32 result = -1;

	DebugTaskKill(CURRENTTASK->t.n_Item);			// swi call

	DBUG(("CloseDebuggerFolio()\n"));
	if (DebuggerFolioNum != 0)
	{
		result = CloseItem(DebuggerFolioNum);
		DebuggerFolioNum = 0;
	}
	return(result);
}

/* Define user level routines */

/**********************************************************************/
Item CreateDbgTask( char *Name, int32 Length, int32 Weight )
{
    Item it;
    CALLFOLIORET (DbgrBase, CREATETASK, (Name, Length,  Weight), it, (Item));
    return it;
}

/**********************************************************************/
int32 DeleteDbgTask(Item Bone )
{
    int32 Result;
    CALLFOLIORET (DbgrBase, DELETETASK, (Bone), Result, (int32));
    return Result;
}

/**********************************************************************/
int32 GetSkelItemInfo( Item AnyItem, TagArg *tp )
{
    int32 Result;
    CALLFOLIORET (DbgrBase, GETSKELITEMINFO, (AnyItem, tp), Result, (int32));
    return Result;
}
