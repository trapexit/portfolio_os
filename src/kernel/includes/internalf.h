#ifndef __INTERNALF_H
#define __INTERNALF_H

/*****

$Id: internalf.h,v 1.108 1994/11/14 22:37:46 vertex Exp $

$Log: internalf.h,v $
 * Revision 1.108  1994/11/14  22:37:46  vertex
 * Added prototypes for SetSystemState() and GetSystemState()
 *
 * Revision 1.107  1994/11/10  18:42:18  vertex
 * Removed the disctype stuff
 *
 * Revision 1.106  1994/11/09  20:51:55  vertex
 * Added externalSetBootDiscType() and externalGetBootDiscType()
 *
 * Revision 1.105  1994/10/17  19:48:22  limes
 * Function "callPrint3DOHeader()" now obselete. Bye bye!
 *
 * Revision 1.104  1994/10/15  01:24:58  sdas
 * internalSetTaskOwner() prototype added
 *
 * Revision 1.103  1994/10/14  23:13:33  vertex
 * Removed the redundant AllocMemList() and FreeMemList() prototypes
 *
 * Revision 1.102  1994/10/11  22:56:29  limes
 * add callPrint3DOHeader()
 *
 * Revision 1.101  1994/10/06  23:14:40  limes
 * Make "printaif3do" into a kernel call (add "internalPrint3DOHeader").
 *
 * Revision 1.100  1994/09/30  18:41:18  vertex
 * Adapted to changes in the definition of ex/internalWaitIO(),
 * and ex/internalWaitPort()
 *
 * Revision 1.99  1994/09/30  01:49:43  vertex
 * Removed unused prototype for AllocItemSlot()
 * Changed prototype of FindItemSlot() to return a slot number instead of an
 *   Item *
 *
 * Revision 1.98  1994/09/28  01:44:17  sdas
 * AbortCurrentTask() definition added
 *
 * Revision 1.97  1994/09/27  16:56:24  sdas
 * Remove internalCreateMsg() definition
 *
 * Revision 1.96  1994/09/26  22:01:42  sdas
 * iternalSetMsgPortOwner() removed
 *
 * Revision 1.95  1994/09/23  23:44:54  sdas
 * externalCreateMsg, ForceKill, KillSelf and internalSetMsgPortOwner added;
 * added parameter to internalCreateMsg
 *
 * Revision 1.94  1994/09/21  19:43:06  vertex
 * Removed the definition of DumpNode(), since it's being moved to
 * list.h
 *
 * Revision 1.93  1994/09/20  23:37:25  vertex
 * Removed the "reply" parameter to the internalPutMsg() prototype
 *
 * Revision 1.92  1994/09/20  23:16:47  vertex
 * Yanked FindNameTag(), and replaced with TagProcessorNoAlloc()
 *
 * Revision 1.91  1994/09/20  22:56:24  vertex
 * Added IsLegalName() and FindNameTag()
 *
 * Revision 1.90  1994/09/15  20:43:44  vertex
 * Added IsSameTaskContext() prototype
 *
 * Revision 1.89  1994/09/01  18:39:00  vertex
 * Renamed internalReplyMsg() to externalReplyMsg(), in preparation for the
 * addition of a real internalReplyMsg()
 *
 * Revision 1.88  1994/08/31  22:31:04  vertex
 * Added prototype for internalDoIO()
 *
 * Revision 1.87  1994/08/26  23:39:45  vertex
 * Added FindMemList() prototype
 *
 * Revision 1.86  1994/08/17  23:15:27  vertex
 * internalWaitPort() now takes items instead of pointers
 *
 * Revision 1.85  1994/08/16  21:19:30  vertex
 * Added externalWaitIO, externalCheckIO, externalWaitPort,
 * externalDoIO(), and changed the definition of internalWaitPort.
 *
 * Revision 1.84  1994/08/15  17:25:22  vertex
 * Added WhichFolio()
 *
 * Revision 1.83  1994/08/10  22:22:25  vertex
 * Added internalLoadDriver() definition
 *
 * Revision 1.82  1994/08/10  21:47:12  vertex
 * Added internalLoadDevice()
 *
 * Revision 1.81  1994/08/03  16:50:22  vertex
 * Added internalSetSemaphoreOwner()
 *
 * Revision 1.80  1994/07/28  23:35:22  vertex
 * Added internalSetFolioOwner() definition
 *
 * Revision 1.79  1994/07/28  17:13:44  vertex
 * Added internalLoadKernelItem()
 * Added internalLoadFolio()
 *
 * Revision 1.78  1994/07/27  16:37:58  vertex
 * Added NodeToItem()
 *
 * Revision 1.77  1994/07/26  23:59:38  limes
 * DiscOsVersion revisions: fix name conflicts. We now have:
 *  - SuperDiscOsVersion(), which does a folio call to it
 *  - swiDiscOsVersion(), which SWIs to it (bad name! what would be better?)
 *  - DiscOsVersion(), which choses Super or swi as appropriate
 *
 * Revision 1.76  1994/07/09  02:52:15  limes
 * Add a parameter to DiscOsVersion: if nonzero, force DiscOsVersion to
 * the specified value.
 *
 * Revision 1.75  1994/07/08  22:35:20  markn
 * Add DiscOsVersion entrypoint.
 *
 * Revision 1.74  1994/05/11  20:37:04  limes
 * add oldIsRamAddr() prototype for ports.c
 *
 * Revision 1.73  1994/04/22  22:06:57  limes
 * add HLINT internals
 *
 * Revision 1.72  1994/03/25  08:01:19  limes
 * Add prototype for "internalCreateTaskVA", the VarArgs version of
 * internalCreateTask that allows you to put your TagArg array on
 * the stack instead of mucking up the data space. This is just
 * experimental -- doing this may not save space.
 *
 * Revision 1.71  1994/03/15  03:53:43  dale
 * added MayGetChar (serial port hack getchar)
 *
 * Revision 1.70  1994/03/15  03:40:05  limes
 * add internalRegisterPeriodicVBLACS and internalRegisterSingleVBLACS
 *
 * Revision 1.69  1994/03/04  21:42:41  sdas
 * a new superinternalPutMsg() now replaces original internalPutMsg(). The new
 * internalPutMsg() has now one more parameter to indicate send/reply
 *
 * Revision 1.68  1994/02/23  00:42:37  sdas
 * Definition for ValidateSignal() added
 *
 * Revision 1.67  1994/02/03  19:21:47  limes
 * *** empty log message ***
 *
 * Revision 1.66  1994/02/03  18:09:03  limes
 * For ease in generating ACS service routines,
 * change the type to function returning int.
 *
 * Revision 1.65  1994/02/02  22:42:57  limes
 * Turn on ACS declarations.
 *
 * Revision 1.64  1994/01/28  23:20:14  sdas
 * Include Files Self-consistency - includes dependant include files
 *
 * Revision 1.63  1994/01/26  18:51:55  limes
 * Add declarations for internalAllocACS and internalPendACS
 * used when -DPROVIDE_ACS is specified.
 *
 * Revision 1.62  1994/01/21  18:11:24  sdas
 * C++ compatibility - updated
 *
 * Revision 1.61  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.60  1994/01/05  18:27:15  dale
 * Added CompleteIO as a privileged swi function
 *
 * Revision 1.59  1993/12/23  23:58:42  sdas
 * Added declarations for ForceExit() and ExitTask()
 *
 * Revision 1.58  1993/12/06  19:58:37  limes
 * Only provide the "global register" definition of KernelBase to
 * modules with KERNEL defined.
 *
 * Revision 1.57  1993/09/27  22:55:48  dale
 * removed ffs, (went to string.h)
 *
 * Revision 1.56  1993/09/08  02:18:42  dale
 * fixed RSACheck return value
 *
 * Revision 1.55  1993/09/01  21:22:26  andy
 * *** empty log message ***
 *
 * Revision 1.54  1993/08/30  23:39:13  andy
 * RSACheck proto adjust
 *
 * Revision 1.53  1993/08/30  22:02:39  dale
 * reenabled miscfunc.h
 *
 * Revision 1.52  1993/08/28  06:23:29  dale
 * removed include miscfunc.h until I get my act together
 *
 * Revision 1.51  1993/08/27  23:19:36  dale
 * include miscfunc.h to get misc function macros
 *
 * Revision 1.50  1993/08/27  19:26:28  dale
 * removed task.h, after fixing parm problem.
 * Hopefully I fixed the right one.
 *
 * Revision 1.49  1993/08/07  03:52:44  andy
 * added include of task.h to handle param dependency
 *
 * Revision 1.48  1993/08/07  03:49:23  andy
 * changed internalFreeSignal params, added externalFreeSignal call, SystemScavengeMem call
 *
 * Revision 1.47  1993/08/04  22:26:07  dale
 * SetFunction
 *
 * Revision 1.46  1993/08/04  22:10:22  dale
 * SetFunction prototype
 *
 * Revision 1.45  1993/08/01  02:09:02  dale
 * fix another dumb syntax error.
 *
 * Revision 1.44  1993/07/31  10:24:50  dale
 * User Function ItemOpened
 *
 * Revision 1.43  1993/07/31  04:10:01  dale
 * backed out RSACheck changes
 *
 * Revision 1.42  1993/07/31  02:36:24  dale
 * fix RSACheck
 *
 * Revision 1.41  1993/07/31  02:32:12  dale
 * newer RSACheck, removed MD4Check
 *
 * Revision 1.40  1993/07/28  00:26:28  andy
 * added proto for superinternalDeleteItem
 *
 * Revision 1.39  1993/07/20  18:53:52  dale
 * corrected ReadTimer prototype
 *
 * Revision 1.38  1993/07/12  16:43:14  dale
 * fixed double prototype problem for SetOwnerIOReq.
 *
 * Revision 1.37  1993/07/11  21:32:30  dale
 * SetItemOwner misc changes
 *
 * Revision 1.36  1993/06/30  06:45:24  andy
 * added isUser
 *
 * Revision 1.35  1993/06/26  01:03:50  andy
 * added additional prototypes
 *
 * Revision 1.34  1993/06/23  02:32:50  dale
 * RemoveItem mods
 *
 * Revision 1.33  1993/06/18  20:38:58  andy
 * added prototype for TagProcessorSearch
 *
 * Revision 1.32  1993/06/16  23:39:23  dale
 * IsRamAddr now takes uint32 size
 *
 * Revision 1.31  1993/06/13  17:59:44  dale
 * IsRamAddr now takes void *
 *
 * Revision 1.30  1993/05/16  23:56:54  andy
 * changed bits to split madam/clio revs
 *
 * Revision 1.29  1993/04/07  05:16:44  dale
 * New memlist code
 *
 * Revision 1.28  1993/04/01  04:44:43  dale
 * fixed TagProcessor prototype
 * signals screwed down
 *
 * Revision 1.27  1993/03/26  18:27:03  dale
 * TimeStamp(uint32 *p) and Reboot
 *
 * Revision 1.26  1993/03/16  06:50:49  dale
 * api
 *
 * Revision 1.25  1993/03/05  01:36:11  dale
 * ifdef out for proper compilation on the mac
 *
 * Revision 1.24  1993/03/04  22:25:15  dale
 * *** empty log message ***
 *
 * Revision 1.23  1993/02/19  02:12:07  dale
 * new kernel routines, slack timer, TagProcessor
 *
 * Revision 1.22  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.21  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.20  1993/02/09  18:04:15  dale
 * added some kernel routines.
 *
 * Revision 1.19  1993/02/09  00:32:34  dale
 * RED SILICON
 *
 * Revision 1.18  1993/01/13  04:12:49  dale
 * new ErrorText kernelnode
 *
 * Revision 1.17  1992/12/22  21:49:43  dale
 * removed printf, use stdio.h
 *
 * Revision 1.16  1992/12/15  05:17:33  dale
 * new fence routines.
 *
 * Revision 1.15  1992/12/10  12:31:18  dale
 * cleanup interrupt stuff
 *
 * Revision 1.14  1992/12/04  03:58:23  dale
 * Wait changes
 *
 * Revision 1.13  1992/12/02  21:58:20  dale
 * LegalNames
 *
 * Revision 1.12  1992/12/01  22:49:59  dale
 * new OpenItem void * parm
 *
 * Revision 1.11  1992/12/01  04:30:42  dale
 * new FindItem/OpenItem
 *
 * Revision 1.10  1992/11/25  00:01:25  dale
 * new msgs
 *
 * Revision 1.9  1992/11/22  21:28:44  dale
 * fix warnings
 *
 * Revision 1.8  1992/11/18  00:05:42  dale
 * rsa stuff
 *
 * Revision 1.7  1992/10/24  01:37:21  dale
 * fix id
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
#include "nodes.h"
#include "list.h"
#include "aif.h"

#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */

extern int32 getPSR(void);

extern void *AllocateNode(struct Folio *, uint8);
extern void *AllocateSizedNode(struct Folio *, uint8,int32);
extern void FreeNode(struct Folio *,void *);

extern uint32 Disable(void);
extern uint32 Disabled(void);
extern void Enable(uint32);
extern void EnableIrq(void);
extern void DebugTrigger(void);
extern int DebugAbortTrigger(uint32 r0, uint32 r1, uint32 r2, uint32 r3);
extern int MayGetChar(void);	/* get a character from the serial port */

extern uint32 FirqInterruptControl(int32, int32);

#ifdef undef
extern uint32 EnableInterrupt(int32);
extern uint32 DisableInterrupt(int32);
extern uint32 ClearInterrupt(int32);
#endif

extern void ULaunch(struct Task *);
extern void USwap(struct Task *,struct Task *);
extern void Launch(struct Task *);
extern struct Task *ULand(struct Task *);

extern void *CheckItem(Item i, uint8 SubsysType, uint8 Type);
extern void FreeItem(Item);
extern void oldRemoveItem(struct Task *, Item);
extern void RemoveItem(Item);
extern Item GetItem(void *);
extern Item AssignItem(void *, Item);
extern void *LocateItem(Item);
extern void *LoookUpItem(Item);
extern int32 FindItemSlot(struct Task *,Item);
extern Item NodeToItem(Node *n);

extern Err
internalSetFunction(Item folio, int32 num, int32 type, void *func);

extern int32 internalAllocSignal(int32);

extern Err internalFreeSignal(int32,struct Task *);
extern Err externalFreeSignal(int32);
extern Item internalCreateSemaphore(struct Semaphore *,TagArg *);
extern int32 externalLockSemaphore(Item, int32);
extern int32 externalUnlockSemaphore(Item);
extern int32 internalLockSemaphore(struct Semaphore *, int32);
extern int32 internalUnlockSemaphore(struct Semaphore *);
extern Err internalSetSemaphoreOwner(struct Semaphore *, Item newOwner);

extern Item internalCreateSizedItem(int32,void *,int32);
extern int32 internalDeleteItem(Item,struct Task *);
extern int32 superinternalDeleteItem(Item);

extern Item internalOpenItem(Item,void *);
extern int32 internalCloseItem(Item,struct Task *);

extern Err ItemOpened(Item task, Item it);

extern Item internalCreateKernelItem(void *, uint8, void *);

extern int32 internalDeleteKernelItem(Item,struct Task *);
extern Item internalFindKernelItem(int32,TagArg *);
extern Item internalOpenKernelItem(Node *, void *);
extern int32 internalCloseKernelItem(Item,struct Task *);
extern int32 internalSetPriorityKernelItem(ItemNode *,uint8,struct Task *);
extern Err internalSetOwnerKernelItem(ItemNode *,Item,struct Task *);
extern Item internalLoadKernelItem(int32 ntype, TagArg *tags);

extern int32 externalDeleteItem(Item);
extern int32 externalCloseItem(Item);

extern struct Folio *WhichFolio(int32 cntype);
extern Item internalCreateFolio(struct Folio *f,TagArg *);
extern int32 internalDeleteFolio(struct Folio *,struct Task*);
extern Item OpenFolio(struct Folio *, void *);
extern Item internalLoadFolio(char *name);
extern Err internalSetFolioOwner(struct Folio *f, Item newOwner);

extern Item internalCreateFirq(struct FirqNode *,TagArg *);
extern int32 internalDeleteFirq(struct FirqNode *,struct Task*);

extern Item internalCreateHLInt(struct FirqNode *,TagArg *);
extern int32 internalDeleteHLInt(struct FirqNode *,struct Task*);

extern Item	internalAllocACS(char *name, int pri, int32 (*code)());
extern int32	internalPendACS(Item it);

extern int	internalRegisterPeriodicVBLACS(Item it);
extern int	internalRegisterSingleVBLACS(Item it);

extern void internalForbid(void);
extern void internalPermit(void);

extern int32 internalChangeTaskPri(struct Task *t, uint8 newpri);
extern Item internalCreateTask(struct Task *,TagArg *);
extern Item internalCreateTaskVA(struct Task *,uint32 args, ...);
extern int32 AbortCurrentTask(uint32 *);
extern void ForceExit(int32);
extern void ExitTask(int32);
extern void ForceKill(void);
extern void KillSelf(void);
extern void internalSetExitStatus(int32);
extern Err externalSetExitStatus(int32);
extern bool IsSameTaskContext(struct Task *t1, struct Task *t2);

extern int32 internalWait(int32);

extern void Switch(void);	/* give up cpu to next readytask */
extern Err ValidateSignal(int32);
extern Err externalSignal(Item,int32);
extern Err internalSignal(struct Task *,int32);
extern void internalYield(void);
extern int32 internalKill(struct Task *, struct Task *);
extern Err internalSetTaskOwner(struct Task *, Item);
extern int32 externalSetItemPriority(Item i,uint8 pri);
extern int32 externalSetItemOwner(Item itm,Item newOwner);

extern Item internalFindItem(int32,TagArg *);
extern Item externalFindAndOpenItem(int32,TagArg *);

extern int32 internalDeleteSemaphore(struct Semaphore *,struct Task *);

extern char *AllocateString(char *n);
extern char *AllocateName(char *n);
extern void FreeString(char *n);
extern bool IsLegalName (char *name);

extern void *internalAllocMemBlocks(int32,int32);
extern void internalFreeMemBlock(void *,int32);
extern int32 externalFreeMemBlocks(void *,int32);
extern int32 FreePage(struct MemHdr *,struct Task *,uint32 *);
extern int32 FreePages(struct Task *,char *, int32);
extern void *FindML(struct List *,uint32);
extern void *FindMH(void *);
extern struct MemHdr *internalFindMH(void *);
extern uint32 isBitSet(uint32 *bits, int32 n);
extern struct MemList *FindMemList(struct List *l, uint8 *q);

extern void TrashMem(uint32 *p,uint32 val,int32 cnt);

extern void internalInitMemList(struct MemList *,struct MemHdr *,char *);
extern int32 internalControlMem(struct Task *,uint8 *, int32, int32, Item);
extern int32 internalControlSuperMem(char *, int32, int32, Item);
extern int32 externalControlMem(char *, int32, int32, Item);
extern int32 GetPageSize(uint32);
extern int32 ValidateMem(struct Task *,char *p, int32 size);
extern int32 IsRamAddr(void *p,uint32 size);
extern int32 oldIsRamAddr(void *p,uint32 size);
extern int32 externalSystemScavengeMem(void);
extern uint32 internalDiscOsVersion(uint32 set);

/* Routines for messages and msgports */
extern Item internalCreateMsgPort(struct MsgPort *,TagArg *);
extern Item externalCreateMsg(struct Msg *,TagArg *);
extern int32 internalDeleteMsgPort(struct MsgPort *,struct Task*);
extern int32 internalDeleteMsg(struct Msg *,struct Task*);

extern Item internalGetMsg(Item mp);
extern Item internalGetThisMsg(Item msg);
extern int32 internalReplyMsg(struct Msg *, int32 result, void *dataptr, int32 datasize);
extern int32 externalReplyMsg(Item msg, int32 result,
				void *dataptr, int32 datasize);
extern Item internalWaitPort(struct MsgPort *, struct Msg *);
extern Item externalWaitPort(Item,Item);
extern Item oldWaitPort(Item,Item);
extern int32 externalPutMsg(Item mp,Item msg, void *dataptr, int32 datasize);
extern int32 superinternalPutMsg(struct MsgPort *msg,struct Msg *mp,
				void *dataptr, int32 datasize);
extern int32 internalPutMsg(struct MsgPort *msg,struct Msg *mp,
				void *dataptr, int32 datasize);

/* Routines for Devices */
extern Item internalLoadDevice(char *name);
extern Item internalCreateDevice(struct Device *,TagArg *);
extern int32 internalDeleteDevice(struct Device *, struct Task *);
extern Item OpenDevice(struct Device *, void *);
extern int32 CloseDevice(struct Device *, struct Task *);
extern Item internalLoadDriver(char *name);
extern Item internalCreateDriver(struct Driver *,TagArg *);
extern int32 internalDeleteDriver(struct Driver *, struct Task *);
extern Item OpenDriver(struct Driver *, void *);
extern int32 CloseDriver(struct Driver *, struct Task *);
extern Item internalCreateIOReq(struct IOReq *,TagArg *);
extern int32 internalDeleteIOReq(struct IOReq *, struct Task *);
extern Err SetIOReqOwner(struct IOReq *ior, Item newOwner);

extern Item internalCreateTimer(struct Timer *r, TagArg *ta);
extern int32 internalDeleteTimer(struct Timer *r,struct Task *);
extern void ControlTimer(struct Timer *, int32, int32);

extern Item internalCreateErrorText(struct ErrorText *r, TagArg *ta);
extern int32 internalDeleteErrorText(struct ErrorText *et,struct Task *t);
extern int32 GetSysErr(char *ubuff,int32 ubufflen,Item i);
extern int32 SimpleTicksToUsecs(int32 ticks);
extern int32 ReadTimer(int32 n);
extern void LoadTimer(int32 c, int32 v, int32 r);

extern int32 externalSendIO(Item, struct IOInfo *);
extern int32 internalSendIO(struct IOReq *);
extern Err internalDoIO(struct IOReq *);
extern Err externalDoIO(Item iorItem, struct IOInfo *ioInfo);
extern Err internalWaitIO(struct IOReq *);
extern int32 externalCheckIO(Item iorItem);
extern Err externalWaitIO(Item iorItem);

extern void internalPrint3DOHeader(_3DOBinHeader *p3do, char *whatstr, char *copystr);

extern int32 externalAbortIO(Item);
extern void internalAbortIO(struct IOReq *);

extern void externalCompleteIO(struct IOReq *);
extern void internalCompleteIO(struct IOReq *);

extern int32 armswap(uint32 *, uint32);
extern uint8 armswapb(uint8 *,uint8);

extern void TimeStamp(struct timeval *p);

extern uint32 *AllocatePageTable(void);
extern void FreePageTable(struct Task *);
extern void ComputePageTable(struct Task *);
extern void NewPageTable(uint32 *);

extern int32 TagProcessor(void *n, TagArg *tagpt, int32 (*cb)(),void *dataP);
extern int32 TagProcessorNoAlloc(void *n, TagArg *tagpt, int32 (*cb)(),void *dataP);
extern int32 TagProcessorSearch(TagArg *ret, TagArg *tagpt, uint32 tag);

extern Err internalGetSystemState(uint32 tag, void *info, size_t infosize);
extern Err internalSetSystemState(uint32 tag, void *info, size_t infosize);

extern void Pause(void);

extern int32 getPSR(void);
extern int32 getSPSR(void);
extern void InstallArmVector(int32,void (*)());

extern int32 IsSherry(int32);
extern int32 IsRed(void);
extern int32 IsRedWireWrap(void);
extern int32 IsGreen(void);
extern int32 IsBlue(void);
extern int32 IsGreenWW(void);

extern uint32 read3ctrs(uint32 ticks[2]);

extern void timerread(uint32 *);
extern int32 USec2Ticks(int32 usecs);
extern void Ticks2TimeVal(uint32 *ticks, struct timeval *tv);

extern int32 RSACheck(uchar *,int32);

extern void EnableFence(uint32);
extern uint32 DisableFence(void);
extern void LoadFenceBits(struct Task *t);
extern void UpdateFence(struct Task *t);
extern void UpdateMadamBits(void);
extern int32 IsClioGreen(void);
extern int32 IsMadamGreen(void);
extern int32 IsWireWrap(void);

extern uint32 ReadHardwareRandomNumber(void);
extern void Reboot(uint32 *image);

/* new pool mem functions */

extern void FreeMemToMemList(struct MemList *ml, void *q, int32 size);
extern void * AllocMemFromMemList(struct MemList *ml, int32 reqsize, uint32 flags);

/* misc collected functions that will be moved to their proper places */

extern void Guru(int gooroo);
extern int32 init_dev_sem(void);

extern void TailInsertNode(List *l, Node *n);
extern void RemoveNode( Node *n);
extern void GCPutChar(char c);

extern bool isUser(void);

/*#define MEMBUG*/

#ifdef MEMBUG
extern int32 VerifyAllNodes(void);
extern void VerifyAllMem(char *);
#define VERIFYMEM(x)	VerifyAllMem(x)
#else
#define VERIFYMEM(x)
#endif

extern int32 IncreaseResourceTable(struct Task *,int32);

#ifndef MACHOST

#ifdef	KERNEL
/* define KernelBase as a global register*/
typedef struct KernelBase *KernelBaseP;
#pragma -r6
extern KernelBaseP KernelBase;
#pragma -r0
#endif

#endif

#ifdef  __cplusplus
}
#endif  /* __cplusplus */


#include "miscfunc.h"

#endif	/* __INTERNALF_H */
