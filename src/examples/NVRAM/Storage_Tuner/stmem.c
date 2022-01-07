
/******************************************************************************
**
**  $Id: stmem.c,v 1.2 1994/11/18 18:04:47 vertex Exp $
**
******************************************************************************/

#include "types.h"
#include "mem.h"
#include "stmem.h"


/*****************************************************************************/


List *memLists;


/*****************************************************************************/


void STInitMem(List *lists)
{
    if (lists)
        memLists = lists;
    else
        memLists = CURRENTTASK->t_FreeMemoryLists;
}


/*****************************************************************************/


void *STAllocMem(uint32 size, uint32 flags)
{
    return AllocMemFromMemLists(memLists,size,flags | MEMTYPE_TRACKSIZE);
}


/*****************************************************************************/


void STFreeMem(void *p)
{
    FreeMemToMemLists(memLists,p,-1);
}
