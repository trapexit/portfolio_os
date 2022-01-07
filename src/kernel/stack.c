/* $Id: stack.c,v 1.21 1994/07/08 06:20:13 dale Exp $ */

#include "types.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "listmacros.h"
#include "folio.h"
#include "task.h"
#include "msgport.h"

#include "semaphore.h"
#include "interrupts.h"
#include "mem.h"
#include "strings.h"
#include "io.h"
#include "driver.h"
#include "device.h"
#include "timer.h"

#include "kernel.h"
#include "operror.h"
#include "inthard.h"
#include "debug.h"
#include "stdio.h"
#include "internalf.h"

extern void Panic(int halt,char *);

#define DBUG(x)		/* kprintf x */

/* #define DISJOINT */
/* #define DISJOINTTEST */


#ifdef DEVELOPMENT
#define INFO(x) kprintf x
#define SINFO(x) printf x
#else
#define INFO(x)		/* kprintf x */
#endif


#ifdef DISJOINT
extern uint32 *cextenduserstack(int32 size, uint32 limit, uint32 pc, uint32 sp);
extern uint32 *cnewuserstack(int32 size,uint32 limit);
#endif

int csuperstackoverflow(void)
{
#ifdef DEVELOPMENT
	Task *ct = KernelBase->kb_CurrentTask;
#endif

#ifdef DEVELOPMENT
	SINFO(("Error: Super Stack Limit Violation task %lx, hanging task.\n",ct));
	while (1);
#else
	Panic(1,"Super Stack Limit\n");
#endif


#if 0
	if(ct->t_SuperSwapped) {
	    while (1);	/* kill them, they just exploded their second stack */
	}
	if(ct->t_SuperSwapStackBase == NULL) {
	    if(ct->t_SuperSwapStackBase = ALLOCMEM(8192,MEMTYPE_ANY)) {
		ct->t_SuperSwapStackSize = 8192;
		ct->t_SuperStackLimit = ct->t_SuperSwapStackBase + 128;
	    }
	}
	if(ct->t_SuperSwapStackBase) {	/* if we have a stack to swap to */
		ct->t_SuperSwapped = 1;
		return 1;
	}
#endif
	return 0;
}

#ifndef DISJOINT
int32 cextenduserstack(int32 size, uint32 limit, uint32 pc, uint32 sp) 
#else
uint32 *cextenduserstack(int32 size, uint32 limit, uint32 pc, uint32 sp)
#endif
{
Task *ThisTask;
MemList *m;
List *l;
int32 newsize;

    ThisTask = KernelBase->kb_CurrentTask;

    if(size < 0) {	/* we got here with stack already blown */
	size = -size;	/* the 128 byte limit allows us to survive */
    }
    newsize = size - (sp - limit) + 256;

    /*
     * Since this routine dinks with the internals of the memory
     * allocation lists, it has to enforce for itself the requirement
     * that allocation list nodes have sizes that are multiples of 16.
     * "newsize" is the size of the thing we are going to take off the
     * freelist, so we round it up here.
     */
    newsize = (newsize + 15) &~ 15;

    INFO(("\nUser stack limit overflow, routine wants %ld bytes.\n",size));
    INFO(("Attempting to get %ld more bytes for stack.\n",newsize));

    INFO(("PC is %lx, StackBase is %lx\n",pc,ThisTask->t_StackBase));
    INFO(("Stack Limit is %lx, SP is %lx\n",limit,sp));
    INFO(("Did you remember to compile with -zps0 ?\n"));

#ifdef DISJOINTTEST
    return cnewuserstack(size,limit);
#endif

    if(ThisTask && (ThisTask->t_ThreadTask == 0)) {	/* just extend task stacks until thread cleanup problems are finished */
        if(size > 1000000) {
	    INFO(("Are you really sure you remembered to compile with -zps0 ?\n"));
	    INFO(("Error: size request too large, can't extend stack.\n"));
#ifdef DEVELOPMENT
    	    if((KernelBase->kb_CPUFlags & KB_NODBGR)==0)Debug();	/* call debugger SWI */
	    while (1);
#endif
        }
        l=ThisTask->t_FreeMemoryLists;
    	for ( m=(MemList *)FIRSTNODE(l);ISNODE(l,m); m = (MemList *)NEXTNODE(m)) {
	    List *ml = m->meml_l;	/* list of freenodes */
	    Node *n;

	    if (m->meml_Sema4) LockItem(m->meml_Sema4,TRUE);
	    DBUG(("MemList [%ls] at 0x%lx\n",m->meml_n.n_Name,(long)m));

	    /*
	     * scan through the memory nodes and see if the end of the
	     * free list is next to the top of the stack
	     */
	    for (n = FIRSTNODE(ml); ISNODE(ml,n); n = NEXTNODE(n) ) {
		DBUG(("mem node at %lx, end at %lx, size is %ld\n",n,(long)n+n->n_Size,n->n_Size));
		/* 128 is offset set in task.c from StackBase to Stack Limit */
		if( ((ulong)n+n->n_Size) == (ulong)(limit-128)) {
			/* yes, we might be able to extend this stack */
			if(n->n_Size < newsize) {
			    INFO(("Sorry, can't extend stack, hanging task\n"));
		            if (m->meml_Sema4) UnlockItem(m->meml_Sema4);
#ifdef DEVELOPMENT
			    if((KernelBase->kb_CPUFlags & KB_NODBGR)==0)Debug();	/* call debugger SWI */
			    while (1);
#endif

			}
			/* set userstack base and limit to new values */
			INFO(("Extending User Stack by %ld ",newsize));
			if(n->n_Size > newsize+sizeof(Node)) {
				n->n_Size -= newsize;
				INFO(("by shrinking mem node\n"));
			}
			else {
				newsize = n->n_Size;
				REMOVENODE(n);	/* remove the entire node */
				INFO(("by removing mem node for %ld bytes\n",newsize));
			}
			/*
			 * got to be in super mode to change
			 * t_StackSize, t_StackBase, t_regs[10], so we don't
			 * return, where assembler routine will
			 * set new r10 from r0, restore r0-r4, then continue
			 */
		        if (m->meml_Sema4) UnlockItem(m->meml_Sema4);
#ifndef	DISJOINT
			return ThisTask->t_regs[10] - newsize;
#else
			{
			    uint32 *pointer;

			    pointer = (uint32 *)limit-128/sizeof(int32);	/* pass back in old base of stack */
			    *pointer = newsize;
			    DBUG(("pointer is %lx, contains %lx\n",pointer,*pointer));
		 	    return pointer;
			}
#endif
		}
	    }
        if (m->meml_Sema4) UnlockItem(m->meml_Sema4);
        }
    }
    INFO(("\nUser thread stack overflow - can't extend.\n"));
    INFO(("PC is %lx, StackBase is %lx, Limit is %lx\n",pc,ThisTask->t_StackBase,limit));
#ifdef DEVELOPMENT
    if((KernelBase->kb_CPUFlags & KB_NODBGR)==0)Debug();	/* call debugger SWI */
    while(1);
#endif
    return -1;
}



#ifdef DISJOINT

uint32 *cnewuserstack(int32 size,uint32 limit) {

uint32 *newstack;
Task *ThisTask = KernelBase->kb_CurrentTask;

	size += 512;
	INFO(("Allocating disjoint stack for task %lx\n",ThisTask));
   	newstack = (uint32 *)USER_ALLOCMEM(size+4,MEMTYPE_ANY);
	INFO(("allocated memory\n"));
	if(newstack == NULL)while(1); /* kill the task...no memory */
	newstack++;

	*newstack = 0;
	*(newstack+1) = size;
	INFO(("new stack is at %lx, size %ld\n",newstack,size));
	return newstack;
}


#endif




