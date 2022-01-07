/* $Id: fence.c,v 1.18 1994/12/14 22:28:58 vertex Exp $ */
/* file: fence.c */

#include "types.h"
#include "mem.h"
#include "kernel.h"
#include "task.h"
#include "inthard.h"
#include "internalf.h"

extern int dram_size;
extern bool two_vram_banks;

#ifdef	undef

#define ISREDWW(i)	(i == MADAM_REDBLUE)
#define ISGRNWW(i)	(i == MADAM_GREENWW)

int32
IsWireWrap(void) {
    int32 i = IsSherry(0);
    return ((i&1) != 0);
}

#define CHIP_COLOR_MASK	0x00ff0000

/* The following two functions determine base greenness */

int32
IsMadamGreen(void)
{
    int32 i = IsSherry(0);
    return ((i&CHIP_COLOR_MASK) >= (MADAM_GREEN&CHIP_COLOR_MASK))
	|| ISGRNWW(i);
}


int32
IsClioGreen(void)
{
    int32 i = IsSherry(1);
    return ((i&CHIP_COLOR_MASK) >= (CLIO_GREEN&CHIP_COLOR_MASK));
}

#endif	/* undef */

void
EnableFence(uint32 fenceflag)
{
    uint32 oldints;
    oldints = Disable();
    *MCTL |= FENCEEN;
    Enable(oldints);
}

void
LoadFenceBits(Task *t)
{
uint32  *fence;
MemList *ml;
uint32  *p;

    /* This routine is only called when the cel engine is stopped,
     * so it is ok to bang the fence bits.
     */

#ifdef ARM600
    NewPageTable(t->t_PageTable);
#endif

    fence = (uint32 *)(0x3300200);

    /* load DRAM bits */

    ml = (MemList *)FIRSTNODE(t->t_FreeMemoryLists);
    p  = (uint32 *)(ml->meml_WriteBits);
    fence[6] = (*p++);
    fence[7] = *p;

    /* and VRAM bits */

    ml = (MemList *)NEXTNODE(ml);
    p  = (uint32 *)(ml->meml_WriteBits);
    fence[14] = *p++;

    if (two_vram_banks)
    {
        ml = (MemList *)NEXTNODE(ml);
        p  = (uint32 *)(ml->meml_WriteBits);
    }

    fence[15] = *p;
}

void UpdateFence(Task *t)
{
    /* Update the fence bits if the memory list of the task/thread is
     * the one who's bits are currently loaded...
     */

    if (t->t_FreeMemoryLists == KernelBase->kb_CurrentTask->t_FreeMemoryLists)
    {
	LoadFenceBits(t);
    }
}

extern int GetRowCnt(void);

void
WaitVSafe(void)
{
    int row;
waitagain:
    row = GetRowCnt();
    if (row < 10)  goto waitagain;
    if (row > 12)  goto waitagain;
    return;
}

void
EnableVDLVideo(void)
{
    uint32 oldints;
    oldints = Disable();
    WaitVSafe();
    *MCTL |= CLUTXEN;
    while (GetRowCnt() < 14) ;
    WaitVSafe();
    *MCTL |= VSCTXEN;
    Enable(oldints);
}
