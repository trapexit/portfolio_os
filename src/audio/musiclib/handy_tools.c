/* $Id: handy_tools.c,v 1.32 1994/08/11 20:47:51 phil Exp $ */
/*
** Handy Programming Tools
** By:  Phil Burk
** Copyright (c) 1992, 3D0 Company.
** This program is proprietary and confidential.
**
** This file used to compile 2 ways: one way for audiofolio, another for musiclib.
*/

/*
** 00001 PLB 921203 Fixed NULL return in EZMemAlloc, use negative
**       size to indicate kernel allocation to prevent freeing
**       from wrong space.
** 930303 PLB Improved DumpMemory() format.
** 930504 PLB Return -2 error code if resource not allocated.
** 930612 PLB Added PARANOID memory checks.
** 940316 WJB Added triple bang in header about splitting this module.
** 940511 WJB Replaced internalf.h w/ clib.h.
** 940811 PLB Extracted out Allocation and EZMem for audiofolio
*/

#include "types.h"
#include "stdio.h"		/* for printf() */
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "stdarg.h"
#include "stdlib.h"
#include "strings.h"
#include "operror.h"
#include "clib.h"		/* for isUser() */
#ifndef USERONLY
#include "super.h"
#endif

#ifdef USERONLY
#include "audio.h"
#include "music.h"
#define	PRT(x)	{ printf x; }
#define ERR(x)   	PRT(x)
#else
/* Include this when building the Audio Folio */
#include "audio_internal.h"
#include "music.h"
#endif

/* #define DEBUG */

static void *(*CustomAllocMemVector)(int32 Size, uint32 Type) = NULL;
static void (*CustomFreeMemVector)(void *p, int32 Size) = NULL;

/* #define DEBUG */
#define DBUG(x)   	/* PRT(x) */
#define DBUGMEM(x)  /* PRT(x) */

/*********************************************************/
int32 EZMemSetCustomVectors( void *(*AllocVector)(int32 Size, uint32 Type),
	 void (*FreeVector)(void *p, int32 Size) )
{
/* None or both can be NULL */
	if( (AllocVector == NULL) ^ (FreeVector == NULL) )
	{
		ERR(("EZMemSetCustomVectors: Must set both vectors."));
		return ML_ERR_BADPTR;
	}
	else
	{
		CustomAllocMemVector = AllocVector;
		CustomFreeMemVector = FreeVector;
	}
	return 0;
}


/*********************************************************/
void *zalloc( int32 NumBytes )
{
	uint8 *p;

	p = (uint8 *) malloc( NumBytes );
	if (p) memset( p, 0, NumBytes );

	return p;
}

/*****************************************************************/
 void DumpMemory( void *addr, int32 cnt)
{
	int32 ln, cn, nlines;
	unsigned char *ptr, *cptr, c;

	nlines = (cnt + 15) / 16;

	ptr = (unsigned char *) addr;

	for (ln=0; ln<nlines; ln++)
	{
		PRT(("%8x: ", ptr));
		cptr = ptr;
		for (cn=0; cn<16; cn++)
		{
			PRT(("%02x ", *cptr++));
		}
		PRT(("  "));
		for (cn=0; cn<16; cn++)
		{
			c = *ptr++;
			if ((c < ' ') || (c > '}')) c = '.';
			PRT(("%c", c));
		}
		PRT(("\n"));
	}
}

/*******************************************************************/
/***** Memory Allocation Helpers ***********************************/
/*******************************************************************/
#ifdef PARANOID
typedef struct MemChecker
{
	int32      mc_Size;    /* Make sure the size is correct. */
	List      *mc_List;    /* Make sure we are freeing to the same list. */
	int32      mc_ID;      /* Identifies allocation. */
	int32      mc_Validation;      /* Validation Flag. */
} MemChecker;

#define MC_VALIDATION (0x6789DCBA)

#if 0
/*******************************************************************/
void ReportVRAMUsage( void )
{
	uint32 BytesFree, BytesOwned;
	mySumAvailMem( KernelBase->kb_MemFreeLists,
		MEMTYPE_VRAM, &BytesOwned, &BytesFree);
DBUGMEM(("KVOwned=%8d, KVFree=%8d\n", BytesOwned, BytesFree));
}
#endif

/*******************************************************************/
static void *mc_AllocMemFromMemListsParanoid(List *l,int32 size, uint32 typebits)
{
	char *MemPtr;
	MemChecker *mc;


	mc =  (MemChecker *) AllocMemFromMemLists( l, size+sizeof(MemChecker), typebits);
	if (mc == NULL)
	{
		ERR(("mc_AllocMemFromMemLists: failed, size = %d, type = 0x%x\n",
			size+sizeof(MemChecker), typebits));
		return NULL;
	}
	else
	{
		mc->mc_Size = size;
		mc->mc_List = l;
		if(isUser())
		{
			mc->mc_ID = IncrementGlobalIndex();   /* Use SWI cuz shared. */
		}
#ifndef USERONLY
		else
		{
			mc->mc_ID = AudioBase->af_GlobalIndex++;
		}
#endif

		mc->mc_Validation = MC_VALIDATION;

		MemPtr = (char *)mc;
		MemPtr += sizeof(MemChecker);
DBUGMEM(("MC: %d allocate: 0x%x = ", mc->mc_ID, MemPtr));
DBUGMEM(("( 0x%x, %d, 0x%x)\n", l, size, typebits));

		return (void *) MemPtr;
	}
}

/*******************************************************************/
static void mc_FreeMemToMemListsParanoid(List *l, void *p, int32 size)
{
	char *MemPtr;
	MemChecker *mc;

/* Check memory. */
	MemPtr = (char *) p;
	MemPtr -= sizeof(MemChecker);
	mc = (MemChecker *) MemPtr;

DBUGMEM(("MC: %d free", mc->mc_ID));
DBUGMEM(("( 0x%x, 0x%x, %d )\n", l, p, size));

	if( mc->mc_Validation != MC_VALIDATION )
	{
		ERR(("mc_FreeMemToMemLists: not valid allocated memory!\n"));
		return;
	}

	if( mc->mc_Size != size )
	{
		ERR(("mc_FreeMemToMemLists error: original size = %d, now = %d\n",
				mc->mc_Size, size));
		return;
	}

	if( mc->mc_List != l)
	{
		ERR(("mc_FreeMemToMemLists error: original list = 0x%x, now = 0x%x\n",
				mc->mc_List, l));
		return;
	}

	mc->mc_Validation = 0;
	FreeMemToMemLists( l, (void *) MemPtr, size+sizeof(MemChecker));
}
#endif



/*******************************************************************/
static void *mc_AllocMemFromMemLists(List *l,int32 size, uint32 typebits)
{
	if(CustomAllocMemVector)
	{
		return (*CustomAllocMemVector)( size, typebits );
	}
	else
	{
#ifdef PARANOID
		return mc_AllocMemFromMemListsParanoid( l, size, typebits);
#else
		return AllocMemFromMemLists( l, size, typebits);
#endif
	}
}

/*******************************************************************/
static void mc_FreeMemToMemLists(List *l, void *p, int32 size)
{
	if(CustomFreeMemVector)
	{
		(*CustomFreeMemVector)( p, size );
	}
	else
	{
#ifdef PARANOID
		mc_FreeMemToMemListsParanoid( l, p, size );
#else
		FreeMemToMemLists( l, p, size );
#endif
	}
}

/*******************************************************************/
void SuperMemFree( void *p, int32 size)
{
	if(isUser())
	{
		ERR(("SuperMemFree called from user mode!\n"));
	}
	else
	{
		mc_FreeMemToMemLists(KernelBase->kb_MemFreeLists,p,size);
	}
}

/*******************************************************************/
void UserMemFree( void *p, int32 size)
{
	if(!isUser())
	{
		ERR(("UserMemFree called from supervisor mode!\n"));
	}
	else
	{
		mc_FreeMemToMemLists(KernelBase->kb_CurrentTask->t_FreeMemoryLists,p,size);
	}
}

/*******************************************************************/
void *SuperMemAlloc ( int32 size, int32 type )
{
	if(isUser())
	{
		ERR(("SuperMemAlloc called from user mode!\n"));
		return NULL;
	}
	else
	{
		return mc_AllocMemFromMemLists(KernelBase->kb_MemFreeLists, size, type);
	}
}

/*******************************************************************/
void *UserMemAlloc ( int32 size, int32 type )
{
	if(!isUser())
	{
		ERR(("UserMemAlloc called from supervisor mode!\n"));
		return NULL;
	}
	else
	{
		return mc_AllocMemFromMemLists(KernelBase->kb_CurrentTask->t_FreeMemoryLists, size, type);
	}
}

/*******************************************************************/
void * EZMemAlloc ( int32 size, int32 type )
{
	int32 *MemPtr;

/*	if(size == 0) PRT(("EZMemAlloc 0 bytes!\n")); */
	if( size < 0 ) return NULL;

	if (isUser())
	{
		MemPtr = (int32 *) mc_AllocMemFromMemLists(KernelBase->kb_CurrentTask->t_FreeMemoryLists,
			size + sizeof(int32), type);
	}
	else
	{
		MemPtr = (int32 *) mc_AllocMemFromMemLists(KernelBase->kb_MemFreeLists,
			size + sizeof(int32),type);
		size = -size;
		  /* store negative size if in kernel */
	}
	if (MemPtr == NULL)
	{
		return NULL;
	}
	else
	{
		*MemPtr++ = size;
		return (void *) MemPtr;
	}
}

/*******************************************************************/
void EZMemFree ( void *ptr )
{
	int32 *p, size;

/* Adjust pointer to original */
	p = (int32 *) ptr;
	p--;   /* point to saved size */
	size = *p;

DBUGMEM(("EZMemFree: ( 0x%x ), size = 0x%x\n", ptr, size));

	if ((size > 0) || ((size == 0) && isUser()))
	{
		mc_FreeMemToMemLists(KernelBase->kb_CurrentTask->t_FreeMemoryLists,
			 (void *) p, size + sizeof(int32));
	}
	else if ((size < 0) || ((size == 0) && !isUser()))
	{
/* kernel memory stores negative size */
		size = -size;
		mc_FreeMemToMemLists(KernelBase->kb_MemFreeLists,
			(void *) p, size + sizeof(int32));
	}
}

/*******************************************************************/
int32 EZMemSize ( void *ptr )
{
	int32 *p, size;

/* Adjust pointer to original */
	p = (int32 *) ptr;
	p--;   /* point to saved size */
	size = *p;
	if (size < 0) size = -size;

	return size;
}

/*******************************************************************/
int32 Choose ( int32 range )
{
	int32 val, r16;

	r16 = rand() & 0xFFFF;
	val = (r16*range) >> 16;
	return val;
}

/*******************************************************************/
int32 FreeThings( TableAllocator *tall, int32 Start, int32 Many)
{
	Start -= tall->tall_Offset;
	return MarkThings ( tall, Start, Many, 0);
}

/*******************************************************************/
int32 MarkThings( TableAllocator *tall, int32 StartIndex, int32 Many, int32 Val)
{
	int32 i;
	uchar *p;

	if ( StartIndex < 0) return -1;
	if ( (StartIndex + Many) > tall->tall_Size) return -2;
	p = &tall->tall_Table[StartIndex];
	for(i=0; i<Many; i++)
	{
#ifdef PARANOID
		if( *p == (uchar ) Val )
		{
			ERR(("PARANOIA in MarkThings: *p = %d, Val = %d, StartIndex = 0x%x, Many = 0x%x\n",
				*p, Val, StartIndex, Many));
		}
#endif
		*p++ = (uchar ) Val;
	}

/* Track Total Usage. */
	if (Val)
	{
		tall->tall_Many += Many;  /* Allocate */
	}
	else
	{

		tall->tall_Many -= Many;  /* Free */
	}

	return 0;
}

/*******************************************************************/
int32 ClearThings( TableAllocator *tall)
{
	int32 i;
	uchar *p;

	p = tall->tall_Table;
	if( p == NULL ) return -1;
	for(i=0; i<tall->tall_Size; i++)
	{
		*p++ = (uchar ) 0;
	}
	tall->tall_Many = 0;
	return 0;
}

/*******************************************************************/
int32 PrintThings( TableAllocator *tall)
{
	int32 i;
	uchar *p;

	p = tall->tall_Table;
	for(i=0; i<tall->tall_Size; i++)
	{
		PRT(("%d ", *p++));
	}
	PRT(("\n"));
	return 0;
}

/*******************************************************************/
int32 AllocAlignedThings( TableAllocator *tall, int32 Many, uint32 *Allocated, uint32 PowerOf2)
{
	int32 GotIt;
	int32 InRow;       /* Number of things found in a row. */
	int32 Start;       /* Index of first one found. */
	int32 Size;
	int32 i,j;
	uchar *Table;

	if (Many <= 0) return -1;
	Size = tall->tall_Size;
	if( Size <= 0 ) return -1;
	Table = tall->tall_Table;
	Start=-1;
	InRow=0;
	GotIt=FALSE;

/* Start on an aligned power of 2 boundary. Index must account for offset. */
/*  P2  Offset  i
     1   0x108  0
     1   0x103  0
     8   0x100  0
     8   0x101  7
     8   0x102  6
     8   0x107  1
     8   0x108  0
*/
	i = ( PowerOf2 > 1 ) ? (PowerOf2 - (tall->tall_Offset  & (PowerOf2-1))) & (PowerOf2-1) : 0;

/* Scan for Many contiguous empty slots in Table. */
DBUG(("AllocAlignedThings: tall = $%x, Many = %d, i=%d\n", tall, Many, i));

	while ((i+Many) <= Size)
	{
		if (Table[i] == 0)  /* Found first empty slot on an aligned boundary. */
		{
			Start = i;
			InRow = 1;
			j=i;
			do
			{
				if(InRow >= Many)
				{
					MarkThings ( tall, Start, Many, 1);
/* if( PowerOf2 > 1 ) PRT(("Allocated %d at 0x%x\n", Many, Start + tall->tall_Offset)); */

					*Allocated = Start + tall->tall_Offset;
DBUG(("*Allocated = 0x%x\n",  *Allocated));
					return 0;
				}
				InRow++;
				j++;
			} while (Table[j] == 0);

			i = (j + PowerOf2) & ~(PowerOf2-1);
		}
		else
		{
			i += PowerOf2;
		}
	};

	return -2;
}

/*******************************************************************/
int32 AllocThings( TableAllocator *tall, int32 Many, uint32 *Allocated)
{
	return AllocAlignedThings( tall, Many, Allocated, 1 );
}
