#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: semaphore.h,v 1.23 1994/10/07 20:15:44 vertex Exp $
**
**  Kernel semaphore management definitions
**
******************************************************************************/


#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __NODES_H
#include "nodes.h"
#endif

#ifndef __KERNELNODES_H
#include "kernelnodes.h"
#endif

#ifndef __LIST_H
#include "list.h"
#endif

#ifndef __ITEM_H
#include "item.h"
#endif

#ifndef __TASK_H
#include "task.h"
#endif

#ifndef EXTERNAL_RELEASE
typedef struct SemaphoreWaitNode
{
	NamelessNode swn;
	Task *swn_Task;
} SemaphoreWaitNode, *SemaphoreWaitNodeP;
#endif /* EXTERNAL_RELEASE */

typedef struct Semaphore
{
	ItemNode s;
	uint32	sem_bit;	/* use swap on this word */
#ifdef MULTIT
	Task	*sem_Owner;	/* Present owner */
#else
	Item	sem_Owner;	/* Present owner */
#endif
	int32	sem_NestCnt;	/* cnt of times this task has locked */
	List sem_TaskWaitingList;
} Semaphore, *SemaphoreP;


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


int32 __swi(KERNELSWI+7) LockSemaphore(Item s,uint32 flags);
Err __swi(KERNELSWI+6)   UnlockSemaphore(Item s);

extern Item CreateSemaphore(const char *name, uint8 pri);
extern Item CreateUniqueSemaphore(const char *name, uint8 pri);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


#define FindSemaphore(n)     FindNamedItem(MKNODEID(KERNELNODE,SEMAPHORENODE),(n))
#define DeleteSemaphore(semaphore)	DeleteItem(semaphore)


/*****************************************************************************/


#endif /* __SEMAPHORE_H */
