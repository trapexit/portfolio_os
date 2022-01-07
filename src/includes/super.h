#ifndef __SUPER_H
#define __SUPER_H

/*****

$Id: super.h,v 1.47 1994/09/30 18:40:55 vertex Exp $

$Log: super.h,v $
 * Revision 1.47  1994/09/30  18:40:55  vertex
 * Added SuperInternalWaitPort() and SuperInternalWaitIO()
 *
 * Revision 1.46  1994/08/31  22:16:50  vertex
 * Added definition for SuperInternalDoIO()
 *
 * Revision 1.45  1994/08/30  18:54:22  vertex
 * Fixed prototype for SuperReplyMsg(). It didn't have the two return
 * parameters
 *
 * Revision 1.44  1994/08/29  18:13:38  sdas
 * SuperQuerySysInfo() and SuperSetSysInfo() definitions added
 *
 * Revision 1.43  1994/08/16  21:30:33  vertex
 * Added definition for SuperDoIO()
 *
 * Revision 1.42  1994/08/10  18:52:30  vertex
 * Added prototype for SuperSystemScavengeMem()
 *
 * Revision 1.41  1994/05/03  20:42:18  vertex
 * Sprinkled a few "const" statements around
 *
 * Revision 1.40  1994/04/26  02:36:03  limes
 * Reformat, add SuperSendMsg and SuperSendSmallMsg, fix 2nd parameter
 * type for SuperCreateItem; remove duplicate decl for SuperReplyMsg;
 * remove unused macros SuperinternalControlSuperMem and
 * SuperinternalPutMsg.
 *
 * Revision 1.39  1994/04/25  20:38:13  limes
 * fix declaration of SuperInternalPutMsg
 *
 * Revision 1.38  1994/04/25  19:29:35  vertex
 * Added SuperCreateMsg/BufferedMsg/SmallMsg/MsgPort/Sema4()
 * Added SuperFindMsgPort/Semaphore()
 * Added SuperDeleteMsg/MsgPort/Semaphore()
 * Changed parameter types from (char *) to (void *) for SuperControlMem(),
 *   SuperIsRamAddr(), and SuperValidateMem()
 *
 * Revision 1.37  1994/04/22  22:40:41  limes
 * add entry point SuperCreateHLInt (in kernellib)
 *
 * Revision 1.36  1994/03/30  23:51:01  limes
 * add prototype for "TimeStamp" available from the kernel via a
 * supervisor folio call.
 *
 * Revision 1.35  1994/03/24  21:45:47  dplatt
 * Add SuperSetFunction.
 *
 * Revision 1.34  1994/03/24  21:12:28  dplatt
 * Add SuperReportEvent
 *
 * Revision 1.33  1994/03/23  23:04:14  sdas
 * Now includes types.h
 *
 * Revision 1.32  1994/02/03  19:21:55  limes
 * *** empty log message ***
 *
 * Revision 1.31  1994/01/26  18:48:50  limes
 * Add declarations for AllocACS and PendACS
 *
 * Revision 1.30  1994/01/21  01:14:35  limes
 * RCS files lost. Recovered these changes:
 *
 * +  * Revision 1.31  1994/01/20  05:19:39  phil
 * +  * fixed C++ mods
 * +  *
 * +  * Revision 1.30  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 * +  *
 *
 * Revision 1.31  1994/01/20  05:19:39  phil
 * fixed C++ mods
 *
 * Revision 1.30  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.29  1993/08/30  00:36:27  andy
 * how'd that m get in there ?
 *
 * Revision 1.28  1993/08/27  17:31:47  andy
 * new RSACheck and SuperBCopy swis added
 *
 * Revision 1.27  1993/08/13  02:53:48  dale
 * fixed more prototypes for Signal stuff
 *
 * Revision 1.26  1993/08/13  02:51:03  dale
 * fix SuperInternalFreeSignal prototype
 *
 * Revision 1.25  1993/08/13  00:53:43  dale
 * Superinternal should be SuperInternal
 *
 * Revision 1.24  1993/08/13  00:34:30  dale
 * added SuperinternalFreeSignal
 *
 * Revision 1.23  1993/08/05  02:29:50  dale
 * SuperWait -> SuperWaitSignal
 *
 * Revision 1.22  1993/07/28  02:01:21  andy
 * added SuperInternalDeleteItem
 *
 * Revision 1.21  1993/07/11  21:28:54  dale
 * SetItemOwner returns Err, not int32
 *
 * Revision 1.20  1993/06/12  00:47:56  andy
 * code instead of routine :-)
 *
 * Revision 1.19  1993/06/12  00:45:28  andy
 * Added CallBackSuper SWI
 *
 * Revision 1.18  1993/04/28  01:48:31  dale
 * all internal->Internal, (left some #defines for now)
 * added prototype for SuperSetItemOwner
 *
 * Revision 1.17  1993/04/01  04:44:15  dale
 * fixed TagProcessor prototype
 *
 * Revision 1.16  1993/03/27  09:40:29  dale
 * internal Semaphore stuff
 *
 * Revision 1.15  1993/03/16  07:22:25  dale
 * SuperIsRamAddr, cleanup
 *
 * Revision 1.14  1993/03/10  01:35:41  dale
 * removed extern KernelBase
 *
 * Revision 1.13  1993/02/17  05:23:22  dale
 * SuperCreateIOReq
 *
 * Revision 1.12  1993/02/17  04:43:21  dale
 * added simple library routines for creating Items
 *
 * Revision 1.11  1993/02/09  00:33:52  dale
 * SuperSwitch and new SuperCompleteIO
 *
 * Revision 1.10  1992/12/12  00:21:14  dale
 * fixed goof up in Signal stuff
 *
 * Revision 1.9  1992/12/12  00:12:14  dale
 * added SuperAllocSignal and SuperFreeSignal
 *
 * Revision 1.8  1992/12/01  22:49:59  dale
 * new OpenItem void * parm
 *
 * Revision 1.7  1992/12/01  04:30:42  dale
 * new FindItem/OpenItem
 *
 * Revision 1.6  1992/10/24  01:46:09  dale
 * rcsa
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint32           Disable (void);
extern void             Enable (uint32);

extern Item             SuperCreateItem (int32 ctype, TagArg *tags);
extern Item             SuperDeleteItem (Item);
extern Item             SuperInternalDeleteItem (Item);
extern Item             SuperCloseItem (Item);
extern Item             SuperOpenItem (Item founditem, void *args);
extern Item             SuperFindItem (int32 ctype, TagArg *tp);
extern Item             SuperFindNamedItem (int32 ctype, char *name);
extern Item             SuperCreateSizedItem (int32 ctype, void *p, int32 size);
extern                  Superkprintf (const char *fmt,...);

extern int32            SuperWaitSignal (int32);
extern int32            SuperAllocSignal (int32);
extern Err              SuperFreeSignal (int32);
extern Err              SuperInternalFreeSignal (int32 sigs, struct Task *t);

extern void             SuperSwitch (void);	/* switch tasks */
extern int32            SuperWaitIO (Item IOReq);
extern Err              SuperDoIO (Item IOReq, struct IOInfo *ioi);
extern int32            SuperInternalSignal (struct Task *t, uint32 bits);

extern int32            SuperLockSemaphore (Item s, uint32 wait);
extern int32            SuperUnlockSemaphore (Item s);

extern int32            SuperInternalLockSemaphore (struct Semaphore * s, uint32 wait);
extern int32            SuperInternalUnlockSemaphore (struct Semaphore * s);

extern Err              SuperSendMsg (Item mp, Item msg, void *dataptr, int32 datasize);
extern Err              SuperSendSmallMsg (Item mp, Item msg, uint32 val1, uint32 val2);

extern int32            SuperSetReplyPort (Item msg, Item mp);
extern Item             SuperGetMsg (Item mp);
extern Item             SuperWaitPort (Item mp, Item msg);
extern Item             SuperInternalWaitPort (struct MsgPort *mp, struct Msg *msg);
extern Err              SuperReplyMsg (Item msg, int32 result, void *dataPtr, int32 dataSize);

extern int32            SuperReportEvent (void *eventFrame);

extern int32            SuperInternalPutMsg (struct MsgPort *, struct Message *, void *dataptr, int32 datasize);
extern int32            SuperControlMem (void *p, int32 size, int32 cmd, Item it);
extern int32            SuperInternalControlSuperMem (char *p, int32 size, int32 cmd, Item it);
extern int32            SuperInternalSendIO (struct IOReq *);
extern Err              SuperInternalDoIO (struct IOReq *);
extern Err              SuperInternalWaitIO( struct IOReq *);
extern Item             SuperGetItem (void *);
extern void             SuperCompleteIO (struct IOReq *);
extern void             SuperInternalAbortIO (struct IOReq *);
extern int32            SuperValidateMem (const struct Task *t, const void *p, int32 size);
extern int32            SuperIsRamAddr (const void *p, int32 size);

extern Item             SuperCreateFIRQ (const char *name, uint8 pri, int32 (*code) (), int32 num);
extern Item		SuperCreateHLInt (const char *name, uint8 pri, int32 (*code) (), int32 line);
extern Item             SuperCreateIOReq (const char *name, uint8 pri, Item dev, Item mp);
extern Item             SuperCreateMsgPort (const char *name, uint8 pri, uint32 signal);
extern Item             SuperCreateMsg (const char *name, uint8 pri, Item mp);
extern Item             SuperCreateSmallMsg (const char *name, uint8 pri, Item mp);
extern Item             SuperCreateBufferedMsg (const char *name, uint8 pri, Item mp, uint32 buffsize);
extern Item             SuperCreateSemaphore (const char *name, uint8 pri);

extern Item             AllocACS (char *name, int pri, int32 (*code) (void));
extern int32            PendACS (Item it);

extern void            *SuperSetFunction (Item folio, int32 vnum, int32 vtype, void *newfunc);
extern int32            SuperSystemScavengeMem (void);

extern Err              SuperSetItemOwner (Item i, Item newOwnerTask);

extern uint32		SuperQuerySysInfo(uint32 tag, void *info, size_t infosize);
extern uint32		SuperSetSysInfo(uint32 tag, void *info, size_t infosize);

extern int32            TagProcessor (void *n, TagArg *tagpt, int32 (*cb) (), void *dataP);

extern void             TimeStamp (struct timeval *tvp);

#define SuperinternalSignal SuperInternalSignal
#define SuperinternalAbortIO SuperInternalAbortIO
#define SuperinternalSendIO SuperInternalSendIO
#define SuperinternalSignal SuperInternalSignal

extern Err              __swi (KERNELSWI + 29) CallBackSuper (Err (*code) (), uint32 arg1, uint32 arg2, uint32 arg3);
extern int32            __swi (KERNELSWI + 26) RSACheck (uchar * buff, int32 buffsize);
extern int32            __swi (KERNELSWI + 27) SuperBCopy (int32 *src, int32 *dst, int32 cnt);

#ifdef __cplusplus
}
#endif

#define SuperDeleteMsg(x)	SuperDeleteItem(x)
#define SuperDeleteMsgPort(x)	SuperDeleteItem(x)
#define SuperFindMsgPort(n)     SuperFindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),(n))
#define SuperDeleteSemaphore(s)	SuperDeleteItem(s)
#define SuperFindSemaphore(n)   SuperFindNamedItem(MKNODEID(KERNELNODE,SEMAPHORENODE),(n))

#endif							/* __SUPER_H */
