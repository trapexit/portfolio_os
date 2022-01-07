/*-------------------------------------------------------------------------
*start
*
*Name:		FMVUtilities.h
*Creator:	George D. Wilson Jr.
*Date:		2/7/94
*Copyright:	© 1993 by 3DO, Inc., all rights reserved.
*
*Purpose:	
*
*Category:
*File:		FMVUtilities.h
*
*Exports:
*
*Locals:
*
*Detailed:
*
*Note:
*
*History: Change History (most recent first):
*
*	  Date		Programmer					Modification
*	--------	----------		-----------------------------------------
*
*stop
*-----------------------------------------------------------------------*/

#ifndef _FMVUTILITIES_
#define _FMVUTILITIES_

#ifndef	_TYPES_H
#include "types.h"
#endif

#ifndef _ITEM_H
#include "item.h"
#endif

extern int8 FMVFindTagArgIndex(TagArgP theTagArrayPtr, uint32 theTag);
extern int8 FMVSetTagArg(TagArgP theTagArrayPtr, uint32 theTag, void *theArg);
extern int8 FMVGetTagArg(TagArgP theTagArrayPtr, uint32 theTag, void *theArg);

#endif

/*-------------------------------------------------------------------------
*
*							End of Module
*
*-----------------------------------------------------------------------*/
