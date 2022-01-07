/* $Id: createthread.c,v 1.1 1994/12/17 02:16:31 vertex Exp $ */

#include "types.h"
#include "task.h"
#include "item.h"
#include "kernel.h"
#include "mem.h"
#include "operror.h"


/*****************************************************************************/


void *KernelAllocMemFromMemLists(List *l, int32 size, uint32 typebits);
void KernelFreeMemToMemLists(List *l, void *p, int32 size);


/*****************************************************************************/


Item CreateThread(const char *name, uint8 pri, void (*code)(), int32 stacksize)
{
	TagArg ta[7];
	uint8 *sb;
	Item ret;

	/* round off to 16 byte size */
	stacksize += 15;
	stacksize &= ~15;
	sb = (uint8 *)KernelAllocMemFromMemLists(CURRENTTASK->t_FreeMemoryLists,
                                                 stacksize,MEMTYPE_ANY);
	if (!sb)	return NOMEM;

	ta[0].ta_Tag = TAG_ITEM_PRI;
	ta[0].ta_Arg = (void *)pri;
	if (name)
	{
	    ta[1].ta_Tag = TAG_ITEM_NAME;
	    ta[1].ta_Arg = (void *)name;
	}
	else ta[1].ta_Tag = TAG_NOP;

	/* Item dependent options */
	ta[2].ta_Tag = CREATETASK_TAG_PC;
	ta[2].ta_Arg = (void *)((long)code);

	ta[3].ta_Tag = CREATETASK_TAG_STACKSIZE;
	ta[3].ta_Arg = (void *)stacksize;

	ta[4].ta_Tag = CREATETASK_TAG_SP;
	ta[4].ta_Arg = (void *)(sb + stacksize);

	ta[5].ta_Tag = CREATETASK_TAG_ALLOCDTHREADSP;

	ta[6].ta_Tag = TAG_END;
	ret = CreateItem(MKNODEID(KERNELNODE,TASKNODE),ta);
	if ((int32)ret < 0) KernelFreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists, sb, stacksize);
	return ret;
}


/*****************************************************************************/


Err DeleteThread(Item ti)
{
    Err ret;
    int32 stacksize;
    uint32 *stackbase;
    Task *t = (Task *)LookupItem(ti);

    if (!t) return BADITEM;
    stacksize = t->t_StackSize;
    stackbase = t->t_StackBase;

    ret = DeleteItem(ti);
    if (ret == 0)
    {
	KernelFreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,stackbase, stacksize);
    }
    return ret;
}
