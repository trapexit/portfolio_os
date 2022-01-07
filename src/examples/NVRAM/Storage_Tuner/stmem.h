#ifndef __STMEM_H
#define __STMEM_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: stmem.h,v 1.2 1994/11/18 18:04:47 vertex Exp $
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __MEM_H
#include "mem.h"
#endif


/*****************************************************************************/


void STInitMem(List *memLists);

void *STAllocMem(uint32 size, uint32 flags);
void STFreeMem(void *p);


/*****************************************************************************/


#endif /* __STMEM_H */
