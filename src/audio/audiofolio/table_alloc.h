/* $Id: table_alloc.h,v 1.2 1994/08/12 23:35:46 peabody Exp $ */
#pragma include_only_once
#ifndef _table_alloc_h
#define _table_alloc_h

#include <types.h>

/****************************************************
** Includes for Table Allocation for audio folio
** By:  Phil Burk
**
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
******************************************************
** 940811 PLB Stripped from handy_tools.c
** 940812 WJB Added types.h
******************************************************/

/* Structures */
typedef struct TableAllocator
{
	int32  tall_Size;
	int32  tall_Offset;
	uchar *tall_Table;
	int32  tall_Many;   /* How many have been allocated. */
} TableAllocator;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Prototypes */
int32 AllocThings( TableAllocator *tall, int32 Many, uint32 *Allocated);
int32 Choose ( int32 range );
int32 ClearThings( TableAllocator *tall);
int32 FreeThings( TableAllocator *tall, int32 Start, int32 Many);
int32 MarkThings( TableAllocator *tall, int32 StartIndex, int32 Many, int32 Val);
int32 PrintThings( TableAllocator *tall);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
