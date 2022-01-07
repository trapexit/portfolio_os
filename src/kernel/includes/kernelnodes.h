#ifndef __KERNELNODES_H
#define __KERNELNODES_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  $Id: kernelnodes.h,v 1.12 1994/09/10 01:35:19 vertex Exp $
**
**  Kernel node and item types
**
******************************************************************************/


#define KERNELNODE	1

#define MEMFREENODE	1
#define LISTNODE	2
#define MEMHDRNODE	3
#define FOLIONODE	4	/* see folio.h */
#define TASKNODE	5	/* see task.h */
#define FIRQNODE	6	/* see interrupts.h */
#define SEMA4NODE	7	/* see semaphores.h */
#define SEMAPHORENODE	SEMA4NODE	/* see semaphores.h */
#define SEMA4WAIT	8

#define MESSAGENODE	9	/* see msgports.h */
#define MSGPORTNODE	10	/* see msgports.h */

#define MEMLISTNODE	11
#define ROMTAGNODE	12

#define DRIVERNODE	13	/* see drivers.h */
#define IOREQNODE	14	/* see io.h */
#define	DEVICENODE	15	/* see devices.h */
#define TIMERNODE	16	/* see timer.h */
#define ERRORTEXTNODE	17	/* see operror.h */

#define	HLINTNODE	18	/* much like a FIRQ node */

/* nodes with 'see xxx.h' are also items */


/*****************************************************************************/


#endif	/* __KERNELNODES_H */
