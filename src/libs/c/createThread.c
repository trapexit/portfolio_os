/* $Id: createThread.c,v 1.17 1994/09/10 02:52:22 vertex Exp $ */
/* file createThread.c */

#include "types.h"
#include "item.h"
#include "kernelnodes.h"
#include "task.h"
#include "mem.h"
#include "operror.h"

/**
|||	AUTODOC PUBLIC spg/kernel/createthread
|||	CreateThread - Create a thread.
|||
|||	  Synopsis
|||
|||	    Item CreateThread( const char *name, uint8 pri, void (*code) (),
|||	                      int32 stacksize )
|||
|||	  Description
|||
|||	    This procedure creates a thread.  The resulting thread belongs to
|||	    the calling task.
|||
|||	  Arguments
|||
|||	    name                        The name of the thread to be created
|||	                                (see `Notes').
|||
|||	    pri                         The priority of the thread.  This can
|||	                                be a value from 11 to 199 (0 to 10 and
|||	                                200 to 255 can only be assigned by
|||	                                system software).  A larger number
|||	                                specifies a higher priority. For all
|||	                                tasks, including threads, the highest
|||	                                priority task -- if there is only one
|||	                                task with that priority -- is always
|||	                                the task that is executed.  If more
|||	                                than one task in the ready queue has
|||	                                the highest priority, the Kernel
|||	                                divides the available CPU time among
|||	                                the tasks by providing each task with
|||	                                a time quantum.
|||
|||	    code                        A pointer to the code that the thread
|||	                                executes.
|||
|||	    stacksize                   The size of the thread's stack, in
|||	                                bytes (see `Notes').
|||
|||	  Return Value
|||
|||	    The procedure returns the item number of the thread or an error
|||	    code if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    task.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    There is no default size for a thread's stack.  To avoid stack
|||	    overflow errors, the stack must be large enough to handle any
|||	    possible uses.  One way to find the proper stack size for a thread
|||	    is to start with a very large stack, reduce its size until a stack
|||	    overflow occurs, and then double its size.
|||
|||	    When you no longer need a thread, use DeleteThread() to delete it.
|||	    Alternatively, the thread can return or call exit().
|||
|||	    You can use FindTask() to find a thread by name.  When naming
|||	    threads, you should assign unique names whenever possible.
|||
|||	  See Also
|||
|||	    DeleteThread(), exit()
|||
**/

Item
CreateThread(const char *name,uint8 pri,void (*code)(),int32 stacksize)
{
	TagArg ta[7];
	uint8 *sb;
	Item ret;

	/* round off to 16 byte size */
	stacksize += 15;
	stacksize &= ~15;
	sb = (uint8 *)ALLOCMEM(stacksize,MEMTYPE_ANY);
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
	if ((int32)ret < 0) FREEMEM(sb, stacksize);
	return ret;
}

/**
|||	AUTODOC PUBLIC spg/kernel/deletethread
|||	DeleteThread - Delete a thread.
|||
|||	  Synopsis
|||
|||	    Err DeleteThread( Item x )
|||
|||	  Description
|||
|||	    This procedure deletes a thread process.
|||
|||	  Arguments
|||
|||	    x                           The item number of the thread to be
|||	                                deleted.
|||
|||	  Return Value
|||
|||	    The procedure returns 0 if the thread was successfully deleted or
|||	    an error code if an error occurs.
|||
|||	  Implementation
|||
|||	    Convenience call implemented in clib.lib V20.
|||
|||	  Associated Files
|||
|||	    task.h                      ANSI C Prototype
|||
|||	    clib.lib                    ARM Link Library
|||
|||	  Notes
|||
|||	    Although threads may be deleted as soon as the parent task has
|||	    finished using it by calling DeleteThread(), threads are
|||	    automatically deleted when they return, call exit() or their
|||	    parent task is deleted or dies.
|||
|||	    Threads share memory with their parent tasks.  This means that:
|||
|||	    *   Memory that was allocated by a thread also belongs to the
|||	        parent task.
|||
|||	    *   Memory that was allocated by a thread is not automatically
|||	        deallocated when the thread is deleted but instead remains the
|||	        property of the parent task.
|||
|||	    If you create a thread using CreateThread(), you must delete it
|||	    using DeleteThread(), which does special cleanup work needed when
|||	    deleted threads.
|||
|||	  See Also
|||
|||	    CreateThread(), exit()
|||
**/

Err
DeleteThread(ti)
Item ti;
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
	FREEMEM(stackbase, stacksize);
    }
    return ret;
}
