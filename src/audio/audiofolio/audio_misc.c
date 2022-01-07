/* $Id: audio_misc.c,v 1.12 1994/12/01 05:42:29 phil Exp $ */
/****************************************************************
**
** Audio Folio Miscellaneous Support routines
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 930617 PLB Force owner before deleting Item.  NO DON'T
** 930904 PLB Call SuperInternalDeleteItem to avoid ownership problem.
** 940608 PLB Disabled calls to mySumAvailMem
** 941128 PLB Added afi_IsRamAddress
****************************************************************/

#include "audio_internal.h"

/* Macros for debugging. */
#define DBUG(x)   /* PRT(x) */


/*****************************************************************/
char *afi_AllocateString( char *s )
{
	int32 len;
	char *news;

	len = strlen(s);
	news = (char *) EZMemAlloc(len+1, 0);
	if ( news == NULL) return NULL;
	strcpy( news, s);
	return news;
}

void afi_FreeString( char *s )
{
	EZMemFree(s);
}

#if 0
/***************************************************************/
int32 afi_ReportMemoryUsage( void )
{
	uint32 BytesFree, BytesOwned;

	mySumAvailMem( KernelBase->kb_CurrentTask->t_FreeMemoryLists,
		MEMTYPE_DRAM, &BytesOwned, &BytesFree);
	PRT(("Memory: UD=%8d", (BytesOwned-BytesFree)));
	
	mySumAvailMem( KernelBase->kb_CurrentTask->t_FreeMemoryLists,
		MEMTYPE_VRAM, &BytesOwned, &BytesFree);
	PRT((", UV=%8d", (BytesOwned-BytesFree)));
	
	mySumAvailMem( KernelBase->kb_MemFreeLists,
		MEMTYPE_DRAM, &BytesOwned, &BytesFree);
	PRT((", SD=%8d", (BytesOwned-BytesFree)));
	
	mySumAvailMem( KernelBase->kb_MemFreeLists,
		MEMTYPE_VRAM, &BytesOwned, &BytesFree);
	PRT((", SV=%8d\n", (BytesOwned-BytesFree)));
		
	return 0;
}
#endif

/*************************************************************/
int32 afi_SuperDeleteItemNode( ItemNode *n )
{
	return afi_SuperDeleteItem( n->n_Item );
}

/*************************************************************/
int32 afi_SuperDeleteItem( Item it )
{
	int32 Result;
	
/* This routine allows non-owners to delete! 930904 */
	Result = SuperInternalDeleteItem( it );
	if(Result < 0)
	{
		ERR(("afi_SuperDeleteItem: delete failed: 0x%x\n", Result));
	}
	
	return Result;
}

/**************************************************************
** Delete Items connected together in a linked list.
**************************************************************/
int32 afi_DeleteLinkedItems( List *ItemList )
{
	ItemNode *n, *next;
	int32 Result = 0;
	
/* Delete Item whose existence depends on owner of list. */
	n = (ItemNode *) FirstNode( ItemList );
	while (ISNODE( ItemList, (Node *) n))
	{
		next = (ItemNode *) NextNode((Node *)n);
		Result = afi_SuperDeleteItemNode( n );
		if( Result < 0 ) return Result;
		n = next;
	}
	
	return Result;
}

/**************************************************************
** Delete all dependant Items that use a reference node.
**************************************************************/
int32 afi_DeleteReferencedItems( List *RefList )
{
	AudioReferenceNode *arnd, *nextarnd;
	int32 Result = 0;
	
/* Assume Item being deleted will remove and delete reference node. %Q really?! */
	arnd = (AudioReferenceNode *)FirstNode(RefList);
	while (ISNODE( RefList, (Node *) arnd))
	{
		nextarnd = (AudioReferenceNode *) NextNode((Node *) arnd);
		Result = afi_SuperDeleteItem( arnd->arnd_RefItem );
		if( Result < 0 ) return Result;
		arnd = nextarnd;
	}
#ifdef PARANOID
	if( !IsEmptyList(RefList) )
	{
		ERR(("afi_DeleteReferencedItems: nodes not getting freed!\n"));
	}
#endif
	return Result;
}

/**************************************************************
** Remove reference to an Item from a list of references.
** Called when a dependant Item is deleted.
**************************************************************/
int32 afi_RemoveReferenceNode( List *RefList, Item RefItem)
{
	AudioReferenceNode *arnd, *nextarnd;

	arnd = (AudioReferenceNode *)FirstNode(RefList);
	while (ISNODE(RefList,arnd))
	{
		nextarnd = (AudioReferenceNode *)NextNode((Node *)arnd);
		if (arnd->arnd_RefItem == RefItem)
		{
			ParanoidRemNode( (Node *) arnd);
			EZMemFree( arnd );
			break;
		}
		arnd = nextarnd;
	}
	return 0;
}


/**************************************************************
** Dummy routine to satisfy TagProcessor.
**************************************************************/
int32 afi_DummyProcessor( void *i, void *p, uint32 tag, uint32 arg)
{
	return 0;
}

/**************************************************************
** Avoid pulling in stdlib
**************************************************************/

/* putc for simple char i/o */
/* and let sprintf work */

FILE *stdout = 0;

int32 putc(char c, FILE *stream)
{
    if (stream == stdout) kprintf("%c",c);
    else
    {
        if (--stream->fcb_numinbuf < 0)
        {
                /* Error! we should never get here */
                /* there is no stream system set up yet */
                return 0;
        }
        *stream->fcb_cp++ = c;
    }
    return c;
}

#ifdef DEBUG_AUDIO
/*****************************************************************/
/***** Trace Support *********************************************/
/*****************************************************************/
int32 TraceIndent( uint32 AndMask, uint32 OrMask, int32 Type)
{
	int32 ilv, doit=0;
/*
DBUG(("TraceIndent(0x%x, 0x%x, %d)\n",  AndMask, OrMask, Type));
DBUG(("TraceIndent: AudioBase = 0x%x\n", AudioBase));
DBUG(("TraceIndent: AudioBase->af_TraceMask = 0x%x\n", AudioBase->af_TraceMask));
DBUG(("TraceIndent: AudioBase->af_IndentLevel = 0x%x\n", AudioBase->af_IndentLevel));
*/
	if (AudioBase == NULL) ERR(("AudioBase == NULL"));
	
	if( ( (AudioBase->af_TraceMask & AndMask) == AndMask) ||  /* All set? */
		(AudioBase->af_TraceMask & OrMask))
	{
		for(ilv=0; ilv<AudioBase->af_IndentLevel; ilv++) PRT(("  "));
		switch (Type)
		{
			case 0:
				PRT((">-"));
/*				AudioBase->af_IndentLevel += 2; */
				break;
				
			case 2:
				PRT(("-<"));
/*				AudioBase->af_IndentLevel -= 2; */
				
		}
		doit = TRUE;
	}
/* DBUG(("TraceIndent returns 0x%x\n", doit)); */
	return doit;
}
#endif


/*****************************************************************/
/***** 16 bit memory access **************************************/
/*****************************************************************/
/* Bulldog does not support 16 bit memory access so we
** need these subroutines.
*/
void Write16( uint16 *Addr, uint16 Data)
{
	uint8 *BytePtr;
	
	BytePtr = (uint8 *) Addr;
	*BytePtr++ = (Data >> 8) & 0xFF;
	*BytePtr = Data & 0xFF;
}

uint32 Read16( uint16 *Addr )
{
	uint8 *BytePtr;
	uint32 Data;
	
	BytePtr = (uint8 *) Addr;
	Data = (*BytePtr++) << 8;
	Data |= *BytePtr;
DBUG(("Read16: 0x%x = *0x%x\n", Data, Addr ));
	return Data;
}


/******************************************************************
** Before this routine, the folio called SuperIsRamAddress
** from many places.  (Warning, the return value for SuperIsRamAddress
** is the opposite of SuperValidateMem.) The folio was expecting a
** negative return code from SuperIsRamAddress which is incorrect.
** SuperIsRamAddress returns FALSE if it is not a RAM address.
** This new routine does return a negative return code for out of RAM,
** and returns zero if in legal RAM. 
******************************************************************/
Err afi_IsRamAddr( const char *p, int32 Size )
{
/*
** We can only do this check for titles after V23.
** This is because titles encrypted with V23 were getting away with
** this and we can't break them now.
*/
	DBUG(("afi_IsRamAddr( 0x%x, %d )\n", p, Size ));
	DBUG(("afi_IsRamAddr: DiscOsVersion = 0x%x )\n", DiscOsVersion(0) ));
	if( DiscOsVersion(0) > MakeDiscOsVersion(24,0) ) /* %Q Is this right for >babs */
	{
		if( !SuperIsRamAddr( p, Size ) )
		{
			ERR(("afi_IsRamAddr: bad addr = 0x%x, Size = %d\n",
				p, Size ));
			return AF_ERR_BADPTR;
		}
	}
	return 0;  /* OK! */
}
