/* $Id: table_alloc.c,v 1.3 1994/09/08 21:10:30 phil Exp $ */
/*
** Table based allocator Used in Audio Folio
** By:  Phil Burk
** Copyright (c) 1992, 3D0 Company.
** This program is proprietary and confidential.
**
** Derived from handy_tools.c.
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
** 940811 PLB Split from handy_tools.c
** 940812 WJB Commented out dead code (PrintThings()).
**            Removed a ton of extraneous includes and added the one that should be here.
** 940907 PLB Put back includes necessary for proper compilation.
*/

#include "audio_internal.h"
#include "table_alloc.h"

/* #define DEBUG */
#define DBUG(x)   	/* PRT(x) */

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

#if 0       /* @@@ not being used */
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
#endif

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
