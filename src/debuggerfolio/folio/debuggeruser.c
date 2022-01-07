/* $Id: debuggeruser.c,v 1.2 1994/08/02 02:37:48 anup Exp $ */

/*
 * $Log: debuggeruser.c,v $
 * Revision 1.2  1994/08/02  02:37:48  anup
 * Added Id and log keywords
 *
 */

/*
** Skeleton Folio User Level Routines
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "debuggerfolio.h"
#include "debuggerfolio_private.h"

/* Macros for debugging. */
#define PRT(x)    { printf x ; }
#define DBUG(x)   PRT(x)


int32 internalGetBoneInfo(SkelBone *bone, TagArg *args);


/*
	Conveniance routine that makes a TagList for CreateItem
*/
Item CreateDbgTask(char *Name, int32 Length, int32 Weight)
{
	int32 Result;
	
	TagArg Tags[] =
	{
		{ TAG_ITEM_NAME, 0},
		{ TAG_NOP, 0},
		{ DB_TAG_LENGTH, 0},
		{ DB_TAG_WEIGHT, 0},
        { 0, 0 }
	};
	
	Tags[0].ta_Arg = (void *) Name;
	Tags[1].ta_Arg = (void *) Length;
	Tags[2].ta_Arg = (void *) Weight;
    Result = CreateItem( MKNODEID(DEBUGGERNODE,DEBUGGER_TASK_NODE), Tags );

	DBUG(("CreateBone returns 0x%x\n", Result));
    return Result;    
}


// Conveniance routine just to balance CreateBone
int32 DeleteDbgTask(Item Bone)
{
	return DeleteItem( Bone );
}



int32 GetSkelItemInfo( Item AnyItem, TagArg *tp )
{
	Node *n;
	int32 Result = DB_ERR_NOTSUPPORTED;
	
	n = (Node *)LookupItem(AnyItem);
	if (n)
	{
		if (n->n_SubsysType != DEBUGGERNODE)
			return DB_ERR_BADITEM;
	}
	else
	{
		return DB_ERR_BADITEM;
	}

	switch (n->n_Type)
	{
	case DEBUGGER_TASK_NODE:
		Result = internalGetBoneInfo((SkelBone *)n, tp);
		break;
    
 	default:
 		Result = DB_ERR_BADITEM;
	}
	
	return Result;
}

int32 internalGetBoneInfo(SkelBone *bone, TagArg *args)
{
/*
** Fill in the TagLIst with values from the Bone
*/
	uint32 tagc, *tagp;
	Item Result;
		
	tagp = (uint32 *)args;

	if (tagp)
	{
		while ((tagc = *tagp++) != 0)
		{
			switch (tagc)
    		{
			case DB_TAG_LENGTH:
				*tagp++ = bone->task_Start;
				break;
			case DB_TAG_WEIGHT:
				*tagp++ = bone->task_ID;
				break;
			default:
				DBUG (("Warning - unrecognized argument to InitSkelBone - 0x%lx: 0x%lx\n",
        	       tagc, *tagp++));
         	  	return DB_ERR_BADTAG;
       			break;
    		}

		}
	}
	return Result;
}
