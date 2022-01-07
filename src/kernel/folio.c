/* $Id: folio.c,v 1.167 1994/11/09 22:37:18 vertex Exp $ */
/*file: folio.c */

#define DBUG(x)	/*printf x*/
#define DBUGOF(x)	/* printf x*/
#define DBUGCF(x)	/* printf x*/

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
#include "stdio.h"
#include "sysinfo.h"
#include "super.h"

#include "aif.h"
#include "internalf.h"
#include "usermodeservices.h"
#include "tags.h"

/* hard assigned folio numbers */
/* Kernel = 1
   Graphics = 2
   FileSystem = 3
   Audio = 4
   JetStream = 5
   Network = 6
*/

extern uint32 *RamDiskAddr;
extern int32 RamDiskSize;

extern void TailInsertNode(List *,Node *);
extern void InsertNodeFromHead(List *,Node *);

extern int clock_freq;
extern char serial_ok;

/*
 * MISC_CODE is a macro that expands to "0" if we
 * are using an external module for misc code, or
 * to its single parameter if we are linking the
 * misc code into the kernel. Only the kernel that
 * boots from within the ROM for use with the ROM
 * based applications normally has the misc code
 * linked into it, but this is sometimes overridden
 * (either way) for debugging.
 */
#ifdef	EXT_MISC
#define MISC_CODE(a)	0	/* eval to "0" if misc code separate */
#endif

#ifdef	INT_MISC
#define MISC_CODE(a)	a	/* eval to "a" if misc code linked in */
#endif

#ifndef	MISC_CODE
please chose either EXT_MISC or INT_MISC
#endif


#define NUM_FOLIOS_CACHED 16
static Folio *folioCache[NUM_FOLIOS_CACHED];


#ifdef MASTERDBUG
AllocateDebugTable(f,nnvecs)
Folio *f;
int nnvecs;
{
	f->f_DebugTable = AllocMem(sizeof((nnvecs+63)/32),MEMTYPE_FILL);
	printf("DebugTable at: %lx\n",f->f_DebugTable);
	if (!f->f_DebugTable)
	{
		Panic("Could not Allocate DebugTable\n");
	}
}
#endif

void
InitFolio(folio,name,size,pri)
Folio *folio;
char *name;
int32 size;
uint8 pri;
{
	/* initialize standard Node fields */
	folio->fn.n_Type = FOLIONODE;
	folio->fn.n_SubsysType = KERNELNODE;
	folio->fn.n_Priority = pri;
	folio->fn.n_Flags = NODE_ITEMVALID|NODE_NAMEVALID;
	folio->fn.n_Name = name;
	folio->fn.n_Size = size;
	folio->fn.n_ItemFlags |= ITEMNODE_UNIQUE_NAME;

	folio->f_OpenCount = 0;
	/*TailInsertNode(KernelBase->kb_FolioList,(Node *)folio);*/
	AddTail(KernelBase->kb_FolioList,(Node *)folio);
}

int
CheckDebug(f,i)
Folio *f;
int i;
{
    /* check the debug bit in the folio DebugTable */
    uint32 *tbl = f->f_DebugTable;
    if (!tbl)	return 0;

    i += f->f_MaxUserFunctions;
    /*printf("CheckDebug: i=%ld\n",i);*/
    if (isBitSet(tbl,i))	return 1;
    return 0;
}

int32
internalDeleteFolio(f,t)
Folio *f;
Task *t;
{
Folio  *kludgedf;
uint32  nvecs;
int32   ret = 0;

    if (f->f_OpenCount)
    {
        /* We require that the folio has its open count dropped to 0 before it
         * can be deleted. This is because folks cache pointers to folios, and
         * jump through the vectors tables preceeding these pointers.
         * This includes supervisor code...
         */

	return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemStillOpened);
    }

	/* Have to force everyone out */
	/* close down folio and release memory */
	DBUG(("DeleteFolio: enter\n"));

	if (f->f_DeleteFolio)
		ret = (*f->f_DeleteFolio)(f);
	if (ret < 0)	return ret;

	REMOVENODE((Node *)f);

	/* free off ItemFunction table */
	FreeMem(f->f_ItemRoutines,sizeof(ItemRoutines));

    /* compensate for the nasty item mangling that occurs in CreateFolio() */

    if (NODETOSUBSYS(f) < NUM_FOLIOS_CACHED)
        folioCache[NODETOSUBSYS(f)] = NULL;

    nvecs = (f->f_MaxUserFunctions + f->f_MaxSwiFunctions + 3) & ~3;
    kludgedf = (Folio *)((uint32)f - 4*nvecs);
    memcpy(kludgedf,f,sizeof(Folio));
    AssignItem(kludgedf,kludgedf->fn.n_Item);

    return 0;
}

/*
   Initial event-report routine, returns a "not implemented" result.
   The Control Port driver will do a SetFunction call to grab the
   SuperReportEvent() subroutine hook, replacing this function with its
   own.
*/

static int32 internalReportEvent(void *event)
{
  return MakeKErr(ER_SEVERE,ER_C_STND,ER_NotSupported);
}

/*
   Code which manages filesystem cache and directory-block memory
   allocation.  This is present here so that a single filesystem image
   can be used in multiple environments, based on decisions made in
   the Sherry compile.  Environments supported:

   -  Bootable-CD.  Supports filesystem cache, and has directory buffers
      large enough for a CD-ROM filesystem.  Multiple buffers can be
      allocated on demand, thus permitting simultaneous Open operations
      to take place.

   -  Development - like bootable-CD.

   -  ROM-applications only.  Does not support filesystem cache (not needed
      for ROM-based filesystems, since they're pretty fast) and has a small
      directory buffer (thus saving space for the ROMapps, which are
      notorious pigs).  Only one buffer is allocated, at system startup
      time.  This means that simultaneous open-file operations can collide,
      and implies that the only filesystems it's safe to open are those on
      devices whose drivers can complete all read operations without
      relinquishing the processor.  All existing RAM/ROM device units are
      of this nature.  CD-ROM is not!  Beware!
*/

#ifdef ROMBUILD
static const int32 bufsize = 512; /* This is the upper limit for ROM fs */
static void *buffer = NULL;

void InitDirectoryBuffer(void)
{
  buffer = AllocMem(bufsize, MEMTYPE_ANY);
}
#endif

static void *internalGetDirectoryCache(int32 *size)
{
#ifdef ROMBUILD
  *size = 0;
  return NULL;
#else
  void *buffer;
  buffer = AllocMem(*size, MEMTYPE_ANY);
  if (!buffer) {
    *size = 0;
  }
  return buffer;
#endif
}

static void *internalGetDirectoryBuffer(int32 *size)
{
#ifdef ROMBUILD
  *size = bufsize;
  return buffer;
#else
  void *buffer;
  DBUG(("Buffer allocator, want %d bytes\n", *size));
  buffer = AllocMem(*size, MEMTYPE_ANY);
  if (!buffer) {
    *size = 0;
  }
  DBUG(("Got %d bytes at 0x%x\n", *size, buffer));
  return buffer;
#endif
}

static void internalReleaseDirectoryBuffer(void *buffer, int32 size)
{
#ifndef ROMBUILD
  FreeMem(buffer, size);
#endif
}

void
ChangeTaskDataBlock(t)
Task *t;
{
    int32 newsize = KernelBase->kb_FolioTaskDataSize;
    int32 oldsize = newsize-4;
    void **newtbl;
    if (t)
    {
	newtbl = (void **)ALLOCMEM(newsize,MEMTYPE_FILL);
	if (!newtbl)
	{
#ifdef DEVELOPMENT
		printf("Panic in ChangeTaskDataBlock, could not get mem\n");
#endif
		while (1);
	}
	memcpy(newtbl,t->t_FolioData,oldsize*4);
	FREEMEM(t->t_FolioData,oldsize*4);
	t->t_FolioData = newtbl;
    }
}

struct FolioInfo
{
	int32 nswis;
	int32 datasize;
	int32 taskData;
	int32 (*finit)();
	void *fs;
	void *f;
	bool specificItem;
	bool needprivilege;
};

static int32
icf_c(f, p, tag, arg)
Folio *f;
struct FolioInfo *p;
uint32 tag;
uint32 arg;
{
	switch(tag)
	{
		case CREATEFOLIO_TAG_DATASIZE :
				p->datasize = (int32)arg;
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_NUSERVECS:
				f->f_MaxUserFunctions = (uint8)arg;
				break;
		case CREATEFOLIO_TAG_USERFUNCS : p->f = (void *)arg;
				break;
		case CREATEFOLIO_TAG_INIT:
				p->finit = Make_Func(int32,arg);
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_NODEDATABASE:
				f->f_NodeDB = (NodeData *)arg;
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_MAXNODETYPE:
				f->f_MaxNodeType = (uint8)arg;
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_ITEM:	/* needs to be */
				f->fn.n_Item = (Item)arg;
				p->specificItem = TRUE;
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_OPENF:
				f->f_OpenFolio = Make_Func(int32,arg);
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_CLOSEF:
				f->f_CloseFolio = Make_Func(void,arg);
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_DELETEF:
				f->f_DeleteFolio = Make_Func(int32,arg);
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_NSWIS :
				p->nswis = (int32)arg;
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_SWIS : p->fs = (void *)arg;
				p->needprivilege = TRUE;
				break;
		case CREATEFOLIO_TAG_TASKDATA :
				p->taskData = (uint32)arg;
				p->needprivilege = TRUE;
				break;
		default:
			return BADTAG;
	}
	return 0;
}

Item
internalCreateFolio(fdummy,tagpt)
Folio *fdummy;
TagArg *tagpt;
{
	int32 nvecs = 0;
	Folio *folio = 0;
	int32 *t = 0;
	int32 size = 0;
	Item ret;
	uint32 oldints;
	Folio localfolio;
	Folio *f;
	void  *allocAddr = NULL;

	struct FolioInfo finfo;

	int32 nnvecs;	/* nvecs rounded up to *4 */

	DBUG(("internalCreateFolio(%lx,%lx)\n",(uint32)fdummy,(uint32)tagpt));

	/* collect all the info in a local folio header */
	f = &localfolio;

	memset(&finfo, 0, sizeof(finfo));

	finfo.datasize = sizeof(Folio);	/* set default datasize */

	memset(f, 0, sizeof(*f));

	f->fn.n_Type = FOLIONODE;
	f->fn.n_SubsysType = KERNELNODE;
	f->fn.n_Item = -1;

	DBUG(("Calling TagProcessor\n"));
	ret = TagProcessor(f,tagpt,icf_c,&finfo);

	if (ret < 0) return ret;

	/* check for proper privilege */
	if ( finfo.needprivilege )
	{
	    if ((KernelBase->kb_CurrentTask->t.n_Flags & TASK_SUPER) == 0)
	    {
		ret = BADPRIV;
		goto done;
	    }
	}

	if (!f->fn.n_Name)	return BADTAGVAL;	/* We require a name */
					/* should also make it unique! */
	if ( (finfo.nswis < 0)
	   ||(finfo.datasize < sizeof(Folio))
	   )
	{
		ret = BADTAGVAL;
		goto done;
	}

	nvecs = finfo.nswis+f->f_MaxUserFunctions;
	DBUG(("nvecs=%ld\n",nvecs));
 	nnvecs = (nvecs + 3) & ~3;
	/* new style folio tables */
	size = 4*nnvecs+finfo.datasize;

	t = (int32 *)ALLOCMEM(size,MEMTYPE_FILL);
	if (!t)
	{
		ret = NOMEM;
		goto done;
	}

        allocAddr = t;

	DBUG(("/* first copy the swi vectors */\n"));
	/* first copy the swi vectors */
	t += nnvecs - nvecs;	/* align to 16 bytes for node */

	if (!IsRamAddr(finfo.fs,finfo.nswis*4))
	{
	    ret = BADPTR;
	    goto done;
	}

	memcpy(t,finfo.fs,finfo.nswis*4);
	t += finfo.nswis;

	if (!IsRamAddr(finfo.f,((size_t)f->f_MaxUserFunctions)*4))
	{
	    ret = BADPTR;
	    goto done;
	}
	memcpy(t,finfo.f,((size_t)f->f_MaxUserFunctions)*4);

	DBUG(("/* allocate ItemFunction table */\n"));
	/* allocate ItemFunction table */

	f->f_ItemRoutines =
	    (ItemRoutines *)ALLOCMEM(sizeof(ItemRoutines),MEMTYPE_FILL);
	if (!f->f_ItemRoutines)
	{
		ret = NOMEM;
		goto done;
	}

	/* must be done before calling GetItem */

	DBUG(("/* transfer the header data */\n"));

	/* transfer the header data */
	folio = (Folio *)(t+f->f_MaxUserFunctions);
	*folio = *f;

	/* now must use the real node */

	DBUG(("item requested=%d\n",folio->fn.n_Item));
	if (folio->fn.n_Item >= 0)
			folio->fn.n_Item = AssignItem(folio,folio->fn.n_Item);
	else		folio->fn.n_Item = GetItem(folio);
	DBUG(("f_Item=%d\n",folio->fn.n_Item));
	if (folio->fn.n_Item < 0)
	{
		ret = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemTableFull);
		goto done;
	}

	if (finfo.taskData)
	{
	    folio->f_TaskDataIndex = KernelBase->kb_FolioTaskDataCnt;
	    KernelBase->kb_DataFolios[folio->f_TaskDataIndex] = folio;
	    KernelBase->kb_FolioTaskDataCnt++;
	    if (KernelBase->kb_FolioTaskDataCnt >
			KernelBase->kb_FolioTaskDataSize)
	    {
		Task *t;
		/* Need to call the folio CreateTask routine */
		/* for all tasks that already exist! */
		/* bug here, fix later */
		KernelBase->kb_FolioTaskDataSize += 4;
		/* go through all tasks and reallocate the taskdata blocks */
		ChangeTaskDataBlock(KernelBase->kb_CurrentTask);
		oldints = Disable();
		for (t = (Task *)FIRSTNODE(KernelBase->kb_TaskWaitQ);
		    ISNODE(KernelBase->kb_TaskWaitQ,t);
			t = (Task *)NEXTNODE(t) )
			    ChangeTaskDataBlock(t);
		for (t = (Task *)FIRSTNODE(KernelBase->kb_TaskReadyQ);
		    ISNODE(KernelBase->kb_TaskReadyQ,t);
			t = (Task *)NEXTNODE(t) )
			    ChangeTaskDataBlock(t);
		Enable(oldints);
	    }
	    /* Note we do not check for success here, we should panic! */
	}

#ifdef MASTERDBUG
	AllocateMasterDebug(folio,nnvecs);
#endif

	folio->f_MaxSwiFunctions = (uint8)(nvecs - folio->f_MaxUserFunctions);

	InitFolio(folio,folio->fn.n_Name,size,folio->fn.n_Priority);

	DBUG(("finfo.finit = %lx\n",finfo.finit));
	if (finfo.finit)
	{
	    DBUG(("calling finit routine\n"));
	    ret = (*finfo.finit)(folio);
	    DBUG(("after calling finit routine ret=%lx\n",ret));
	    if ( ret < 0)
	    {	/* error */
		REMOVENODE((Node *)folio);
		goto done;
	    }
	}

        if (NODETOSUBSYS(folio) < NUM_FOLIOS_CACHED)
        {
            folioCache[NODETOSUBSYS(folio)] = folio;
        }
#ifdef DEVELOPMENT
        else if (finfo.specificItem)
        {
            printf("INFO: Folio subsystem number exceeds folio cache capacity. You\n");
            printf("      might want to increase the cache size in folio.c to keep\n");
            printf("      performance higher.\n");
        }
#endif

	return folio->fn.n_Item;
done:
    DBUG(("done:\n"));

    if (ret < 0)
    {
	if (folio && (folio->fn.n_Item > 0)) FreeItem(folio->fn.n_Item);
	FREEMEM(f->f_ItemRoutines,sizeof(ItemRoutines));
	FreeString(f->fn.n_Name);
	FREEMEM(allocAddr,size);
    }
    return ret;
}

/* get the internal kernel routines */

int32
wcopy(src,dst,cnt)
int32 *src,*dst;
int32 cnt;
{
	Task *ct = KernelBase->kb_CurrentTask;
	if ( (ct->t.n_Flags & TASK_SUPER) == 0) return BADPRIV;
	do
	{
		*dst++ = *src++;
		cnt -= 4;
	} while (cnt>0);
	return 0;
}


/* call priv task back in super mode */
Err callbacksuper(Err (*code)(),uint32 arg1, uint32 arg2, uint32 arg3)
{
	Task *ct = KernelBase->kb_CurrentTask;
	if ( (ct->t.n_Flags & TASK_SUPER) == 0) return BADPRIV;
	return (*code)(arg1,arg2,arg3);
}

void
illegal(void)
{ }

/*
 * void here only to make compiler quiet
 * to use __vfprintf in table.  If vfprintf
 * were called in this file, we would have to
 * do a lot more to keep the compiler quiet
*/
extern int __vfprintf(void);

extern void cstartup(void );	/* from cstartup */
extern void stackoverflow(void); /* from startup.s */

void *(*KernelFunctions[])() =
{
	/* swi handlers start here. */
	/* add new handlers at the top. */
	(void *(*)())internalPrint3DOHeader,		/* 42 */
	(void *(*)())externalWaitIO,			/* 41 */
	(void *(*)())externalWaitPort,			/* 40 */
	(void *(*)())externalSetExitStatus,		/* 39 */
	(void *(*)())SampleSystemTime,			/* 38 */
	(void *(*)())externalDoIO,                      /* 37 */
	(void *(*)())externalFindAndOpenItem,           /* 36 */
	(void *(*)())internalDiscOsVersion,		/* 35 */
	(void *(*)())externalCompleteIO,		/* 34 */
	(void *(*)())externalSystemScavengeMem,		/* 33 */
	(void *(*)())illegal,				/* 32 */
	(void *(*)())illegal,				/* 31 */
	(void *(*)())MayGetChar,			/* 30 */
	(void *(*)())callbacksuper,			/* 29 */
	(void *(*)())externalSetItemOwner,		/* 28 */
	(void *(*)())wcopy,				/* 27 */
	(void *(*)())RSACheck,				/* 26 */
	(void *(*)())externalAbortIO,			/* 25 */
	(void *(*)())externalSendIO,			/* 24 */
	(void *(*)())internalSetFunction,		/* 23 */
	(void *(*)())externalFreeSignal,		/* 22 */
	(void *(*)())internalAllocSignal,		/* 21 */
	(void *(*)())externalControlMem,		/* 20 */
	(void *(*)())internalGetMsg,			/* 19 */
	(void *(*)())externalReplyMsg,			/* 18 */
	(void *(*)())ReadHardwareRandomNumber,		/* 17 */
	(void *(*)())externalPutMsg,			/* 16 */
	(void *(*)())internalGetThisMsg,		/* 15 */
	(void *(*)())printf,				/* 14: kprintf */
	(void *(*)())internalAllocMemBlocks,		/* 13 */
#ifdef FORBID
	(void *(*)())internalPermit,			/* 12 */
	(void *(*)())internalForbid,			/* 11 */
#else
	(void *(*)())illegal,
	(void *(*)())illegal,
#endif
	(void *(*)())externalSetItemPriority,		/* 10 */
	(void *(*)())internalYield,			/* 9 */
	(void *(*)())externalCloseItem,			/* 8 */
	(void *(*)())externalLockSemaphore,		/* 7 */
	(void *(*)())externalUnlockSemaphore,		/* 6 */
	(void *(*)())internalOpenItem,			/* 5 */
	(void *(*)())internalFindItem,			/* 4 */
	(void *(*)())externalDeleteItem,		/* 3 */
	(void *(*)())externalSignal,			/* 2 */
	(void *(*)())internalWait,			/* 1 */
	(void *(*)())internalCreateSizedItem,		/* 0 */

	/* Supervisor mode routines will now start here */
	/* you must already be in supervisor mode to use these! */

	(void *(*)())superinternalPutMsg,		/* -1 */
	(void *(*)())Disable,				/* -2 */
	(void *(*)())Enable,				/* -3 */
	(void *(*)())DebugTrigger,			/* -4 */
	(void *(*)())FirqInterruptControl,		/* -5 */
	(void *(*)())internalSendIO,			/* -6 */
	(void *(*)())GetItem,				/* -7 */
	(void *(*)())internalCompleteIO,		/* -8 */
	(void *(*)())internalAbortIO,			/* -9 */
	(void *(*)())internalControlSuperMem,		/* -10 */
	(void *(*)())ValidateMem,			/* -11 */
	(void *(*)())internalSignal,			/* -12 */
	(void *(*)())IsRamAddr,				/* -13 */
	(void *(*)())Switch,				/* -14 */
	(void *(*)())TagProcessor,			/* -15 */
	(void *(*)())TimeStamp,				/* -16 */
	(void *(*)())internalUnlockSemaphore,		/* -17 */
	(void *(*)())internalLockSemaphore,		/* -18 */
	(void *(*)())superinternalDeleteItem,		/* -19 */
	(void *(*)())internalFreeSignal,		/* -20 */
	(void *(*)())internalAllocACS,			/* -21 */
	(void *(*)())internalPendACS,			/* -22 */
	(void *(*)())internalRegisterPeriodicVBLACS,	/* -23 */
	(void *(*)())internalRegisterSingleVBLACS,	/* -24 */
	(void *(*)())internalReportEvent,		/* -25 */
	(void *(*)())SuperQuerySysInfo,			/* -26 */
	(void *(*)())SuperSetSysInfo,			/* -27 */
	(void *(*)())SectorECC,				/* -28 */
	(void *(*)())internalGetDirectoryCache,		/* -29 */
	(void *(*)())internalGetDirectoryBuffer,	/* -30 */
	(void *(*)())internalReleaseDirectoryBuffer,	/* -31 */
	(void *(*)())internalWaitPort,			/* -32 */
	(void *(*)())internalWaitIO,			/* -33 */
	(void *(*)())internalDoIO,			/* -34 */

	/* add new supervisor mode calls here at the bottom */
	/* Remember to adjust UserFuncCount!!!! */

	/* User mode routines will now start here */
	/* add new user mode calls here at the top */
	/* Remember to adjust UserFuncCount!!!! */

        (void *(*)())NextTagArg,			/* 45 */
        (void *(*)())FindTagArg,			/* 44 */
        (void *(*)())IsMemWritable,			/* 43 */
	(void *(*)())IsRamAddr,				/* 42 */
        (void *(*)())externalCheckIO,			/* 41 */
	(void *(*)())GetMemTrackSize,			/* 40 */
	(void *(*)())GetMemAllocAlignment,		/* 39 */
	(void *(*)())FindImage,				/* 38 */
	(void *(*)())ExitTask,				/* 37 */
	(void *(*)())0,					/* 36 __rt_udiv10 */
	(void *(*)())0,					/* 35 __rt_sdiv10 */
	(void *(*)())0,					/* 34 __rt_udiv */
	(void *(*)())0,					/* 33 __rt_sdiv */
	(void *(*)())ItemOpened,			/* 32 */
	(void *(*)())stackoverflow,			/* 31 */
	(void *(*)())cstartup,				/* 30 */
	(void *(*)())FreeMemList,			/* 29 */
	(void *(*)())AllocMemList,			/* 28 */
	(void *(*)())FreeMemToMemList,			/* 27 */
	(void *(*)())AllocMemFromMemList,		/* 26 */
	(void *(*)())internalFindMH,			/* 25 */
	(void *(*)())oldWaitPort,			/* 24 */
	(void *(*)())illegal,				/* 23 */
	(void *(*)())GetSysErr,				/* 22 */
	(void *(*)())__vfprintf,			/* 21 */
	(void *(*)())MISC_CODE(InsertNodeFromHead),	/* 20 */
	(void *(*)())SetNodePri,			/* 19 */
	(void *(*)())USec2Ticks,			/* 18 */
	(void *(*)())UniversalInsertNode,		/* 17 */
	(void *(*)())CheckItem,				/* 16 */
	(void *(*)())GetPageSize,			/* 15 */
	(void *(*)())MISC_CODE(memcpy),			/* 14 */
	(void *(*)())MISC_CODE(memset),			/* 13 */
	(void *(*)())LookupItem,			/* 12 */
	(void *(*)())ScavengeMem,			/* 11 */
	(void *(*)())FindNamedNode,			/* 10 */
	(void *(*)())MISC_CODE(InitList),		/* 9 */
	(void *(*)())FreeMemToMemLists,			/* 8 */
	(void *(*)())AllocMemFromMemLists,		/* 7 */
	(void *(*)())MISC_CODE(RemNode),		/* 6 */
	(void *(*)())MISC_CODE(TailInsertNode),		/* 5 */
	(void *(*)())MISC_CODE(AddTail),		/* 4 */
	(void *(*)())MISC_CODE(RemTail),		/* 3 */
	(void *(*)())MISC_CODE(AddHead),		/* 2 */
	(void *(*)())MISC_CODE(RemHead),		/* 1 */
};

/* Common folio routines     */
/* FindItem(char *)    */
/* CreateItem(NODETYPE,args) */
/* DestroyItem(Item)         */

#define UserFuncCount 79 /* total count of non-swi functions in table */

#define funccount (sizeof(KernelFunctions)/sizeof(void *))

extern void setKernelBase(struct KernelBase *);

extern uint32 *screenstart;

/* make sure we can align the actual node on a 16byte boundary */
uint32 lclKnlNode[(funccount+3)+((sizeof(struct KernelBase)+3)>>2)];

#ifdef MASTERDEBUG
uint32 DebugTable[(funccount+31)/32];
#endif

List MemHdrList;
List MemFreeLists;
List FolioList,TaskWaitQ,TaskReadyQ;
List Semaphores;

List MsgPorts;
List RomTags;
List Drivers;
List Devices;
List Tasks;

/* Table of Node or List ptrs */
Node *InterruptHandlers[INT_MAX];

Folio *PerTaskDataFolio[32];

struct NodeData NodeDB[] =
{
	{ 0, 0 },	/* no node here */
	{ sizeof(NamelessNode), 0 },
	{ sizeof(List), NODE_NAMEVALID },
	{ sizeof(MemHdr), NODE_NAMEVALID },
	{ 0, NODE_NAMEVALID },	/* Folio */
	{ sizeof(Task), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(FirqNode), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(Semaphore), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(SemaphoreWaitNode), NODE_SIZELOCKED },
	{ 0, NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED }, /* Msg */
	{ sizeof(MsgPort), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(MemList), NODE_NAMEVALID },
	{ /*sizeof(RomTag)*/ 0, NODE_NAMEVALID|NODE_ITEMVALID },
	{ sizeof(Driver), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ 0, NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },	/* IOReq */
	{ sizeof(Device), NODE_NAMEVALID|NODE_ITEMVALID },
	{ sizeof(Timer), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
	{ sizeof(ErrorText), NODE_NAMEVALID|NODE_ITEMVALID },
};

#define NODECOUNT (sizeof(NodeDB)/sizeof(NodeData))

extern uint32 * PtrMacPkt;

extern uint32 CPUFlags;
extern uint32 Arm600;


#if 0
Folio *DebugBase;

#ifdef DEVELOPMENT
Folio *InitDebug(void)
{
	DebugBase = (Folio *)ALLOCMEM(sizeof(Folio),MEMTYPE_FILL);
	if (!DebugBase)
	{
		printf("Panic: no mem for debug lib?\n");
		while (1);
	}
	InitFolio(DebugBase,"debug",(int32)sizeof(Folio),(uint8)0);
	return DebugBase;
}
#endif
#endif

ItemRoutines kb_ItemRoutines =
{
	internalFindKernelItem,			/* ir_Find	  */
	internalCreateKernelItem,		/* ir_Create	  */
	internalDeleteKernelItem,		/* ir_Delete	  */
	internalOpenKernelItem,			/* ir_Open	  */
	internalCloseKernelItem,		/* ir_Close	  */
	internalSetPriorityKernelItem,		/* ir_SetPriority */
	internalSetOwnerKernelItem,		/* ir_SetOwner	  */
	internalLoadKernelItem                  /* ir_Load        */
};

#ifdef	EXT_MISC
/* pointer to array of functions returning ptr to void */
extern void FixKernelFolio(void *(*ftbl[])());
#endif

void
InitKB(_3DOBinHeader *thdo)
{
	/* Initialize Kernal data structures */
	uint8 *t;
	t = (uint8 *)lclKnlNode;
	DBUG(("InitKB: Disabled=%lx\n",Disabled()));
	bzero(t,sizeof(lclKnlNode));
	{
		/* align KernelBase to 16 byte boundary */
		int32 q = (int32)t;
		q += (funccount+3)*4;
		q &= ~15;
		q -= funccount*4;
		t = (char *)q;
	}
	memcpy(t,(uint8 *)KernelFunctions,funccount*4);
	DBUG(("InitKB, after memcpy: Disabled=%lx\n",Disabled()));
	KernelBase = (struct KernelBase *)(t+funccount*4);
	setKernelBase(KernelBase);
	DBUG(("InitKB, after setkernelBase: Disabled=%lx\n",Disabled()));

#ifdef	EXT_MISC
	FixKernelFolio((void *(**)())KernelBase); /* bring in the Misc functions */
#endif

	KernelBase->kb_CPUFlags |= CPUFlags;

	KernelBase->kb_denomticks = 128*1000; /* keep denom to 16bits */

	/* set up for video dma on, clut dma on, 512word rows */
	/* set up for 2 4Mbyte DRAM, total of 8M DRAM */
	/*KernelBase->kb_MSysBits |= DRAMSETZ_4MEG|DRAMSIZE_SET0SET1;*/
	KernelBase->kb_CPUFlags |= KB_SHERRY;

	/* Base RED system */
	KernelBase->kb_CPUFlags |= KB_RED;
	KernelBase->kb_numticks = clock_freq/10;
	KernelBase->kb_MadamRev = 0;
	KernelBase->kb_ClioRev = 0;

#ifdef	undef
	if(IsWireWrap()) {	/* it is at least a red wire wrap ? */
		KernelBase->kb_CPUFlags |= KB_REDWW;
		/*KernelBase->kb_numticks = REDWWMHZx1000;*/
		KernelBase->kb_CPUFlags |= KB_WIREWRAP;
	}

	if (IsClioGreen())
	{
		KernelBase->kb_CPUFlags |= KB_REDWW;	/* green has everything the red wirewrap has */
		KernelBase->kb_ClioRev = 1;

		KernelBase->kb_CPUFlags |= KB_GREEN;	/* temporary compatibility */
	}

	if (IsMadamGreen())
	{
		KernelBase->kb_CPUFlags |= KB_REDWW;	/* green has everything the red wirewrap has */
		KernelBase->kb_MadamRev = 1;
		KernelBase->kb_CPUFlags |= KB_GREEN;	/* temporary compatibility */
	}
#else	/* undef */
	/* Base GREEN system */
	/* Anyone who needs to depend on the following */
	/* values should instead use SysInfo queries */
	/* These definitions are left below only for compatibility */

	KernelBase->kb_CPUFlags |= (KB_REDWW|KB_GREEN);
	KernelBase->kb_MadamRev = 1;			/* atleast GREEN */
	KernelBase->kb_ClioRev = 1;			/* atleast GREEN */
#endif	/* undef */

	KernelBase->kb_CPUFlags |= KB_BROOKTREE;

	if (Arm600)	KernelBase->kb_CPUFlags |= KB_ARM600;

	if (serial_ok)	KernelBase->kb_CPUFlags |= KB_SERIALPORT;

	KernelBase->kb_MacPkt = PtrMacPkt;

	/* Must be done before calling InitFolio */
	KernelBase->kb_FolioList = &FolioList;
	InitList(KernelBase->kb_FolioList,"Folio");

	DBUG(("FolioList=%lx\n",&FolioList));
	DBUG(("Hand construct KernelFolioBase Disabled=%lx\n",Disabled()));
	/* Must hand construct the first */
	InitFolio((Folio*)KernelBase,"kernel",(int32)sizeof(struct KernelBase),254);
	KernelBase->kb.f_MaxSwiFunctions = funccount - UserFuncCount;
	KernelBase->kb.f_MaxUserFunctions = UserFuncCount;
	KernelBase->kb.f_MaxNodeType = NODECOUNT;
	KernelBase->kb.f_NodeDB = NodeDB;
	folioCache[KERNELNODE] = (Folio *)KernelBase;

	DBUG(("MaxSwiFunctions=%d\n",(int32)KernelBase->kb.f_MaxSwiFunctions));
	DBUG(("MaxUserFunctions=%d\n",(int32)KernelBase->kb.f_MaxUserFunctions));

	KernelBase->kb_MemFreeLists = (List *)&MemFreeLists;

	DBUG(("Init some lists\n"));
	InitList(KernelBase->kb_MemFreeLists,"MemFreeLists");

	KernelBase->kb_MemHdrList = &MemHdrList;
	InitList(KernelBase->kb_MemHdrList,"MemHdr");

	KernelBase->kb_TaskWaitQ = &TaskWaitQ;
	InitList(KernelBase->kb_TaskWaitQ,"TaskWaitQ");

	KernelBase->kb_Semaphores = &Semaphores;
	InitList(KernelBase->kb_Semaphores,"Semaphores");

	KernelBase->kb_TaskReadyQ = &TaskReadyQ;
	InitList(KernelBase->kb_TaskReadyQ,"TaskReadyQ");

	KernelBase->kb_RomTags = &RomTags;
	InitList(KernelBase->kb_RomTags,"RomTags");

	KernelBase->kb_InterruptHandlers = InterruptHandlers;
	/* In case it needs to be increased in the future by some */
	/* hack */
	KernelBase->kb_MaxInterrupts = INT_MAX;

#ifdef undef
	KernelBase->kb_FIRQ = &FIRQ;
	InitList(KernelBase->kb_FIRQ,"FIRQ");
#endif

	KernelBase->kb_MsgPorts = &MsgPorts;
	InitList(KernelBase->kb_MsgPorts,"Msg Ports");

	KernelBase->kb_Drivers = &Drivers;
	InitList(KernelBase->kb_Drivers,"Drivers");

	KernelBase->kb_Devices = &Devices;
	InitList(KernelBase->kb_Devices,"Devices");

	KernelBase->kb_Tasks = &Tasks;
	InitList(KernelBase->kb_Tasks,"Tasks");

	KernelBase->kb.f_ItemRoutines = &kb_ItemRoutines;

	KernelBase->kb_TimerBits = 0x00003fff;	/* 14 available */
				/* top 2 are reserved for dipir */

	KernelBase->kb_CurrentTask = 0;
	KernelBase->kb_Forbid = 0;
	KernelBase->kb_PleaseReschedule = 0;
	KernelBase->kb_VRAMHack = screenstart;

	KernelBase->kb_DataFolios = PerTaskDataFolio;
	KernelBase->kb_RamDiskAddr = RamDiskAddr;
	KernelBase->kb_RamDiskSize = RamDiskSize;

	KernelBase->kb.fn.n_Version = thdo->_3DO_Item.n_Version;
	KernelBase->kb.fn.n_Revision = thdo->_3DO_Item.n_Revision;

#ifdef MASTERDEBUG
	KernelBase->kb.f_DebugTable = DebugTable;

	printf("DebugTable=%lx\n",DebugTable);
#endif

	DBUG(("InitKB now returning\n"));
	/*while  (1);*/
}


/*****************************************************************************/


#if 0
/* done in-line in item.c SetOwnerKernelItem() */
Err internalSetFolioOwner(Folio *f, Item newOwner)
{
Task *owner;

    owner = (Task *)LookupItem(newOwner);
    if (owner->t_ThreadTask == CURRENTTASK->t_ThreadTask)
        return 0;

    return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);
}
#endif


/*****************************************************************************/


/* Load a folio from external storage */
Item internalLoadFolio(char *name)
{
void  *context;
Item   result;
Folio *f;

    result = LoadModule("folio",name,MKNODEID(KERNELNODE,FOLIONODE),&context);

    f = (Folio *)LookupItem(result);
    if (f)
        f->f_DemandLoad = context;

    return result;
}


/*****************************************************************************/


/* Unload a folio from memory */
Err internalUnloadFolio(Folio *f)
{
    return UnloadModule(&f->fn,f->f_DemandLoad);
}


/*****************************************************************************/


Item
OpenFolio(Folio *f, void *a)
{
int32 ret = 0;

    /* ignore version number for now */

    f->f_OpenCount++;

    DBUGOF(("OpenFolio: entering, folio '%s', fopen = $%lx\n",f->fn.n_Name,f->f_OpenFolio));

    if (f->f_OpenFolio)
    {
        ret = (*f->f_OpenFolio)(f);
        if (ret < 0)
        {
            f->f_OpenCount--;
            DBUGOF(("OpenFolio: returns %lx\n",ret));

            /* if the folio doesn't want to open up, nuke it... */
            internalUnloadFolio(f);

            return ret;
        }
    }

    DBUGOF(("OpenFolio: exiting with $%lx, new open count is %d\n",f->fn.n_Item,f->f_OpenCount));

    return f->fn.n_Item;
}


/*****************************************************************************/


static void ReleaseFolioItems(Task *task, Folio *f)
{
Item     *ip;
int32     i;
Item      it;
uint32    cnt;
uint32    folioSubSys;
ItemNode *in;

    if (task->t_Flags & TASK_EXITING)
    {
        /* don't bother if the task is going away */
        return;
    }

    /* first determine if the current task has more than one open on the
     * given folio
     */

    ip  = task->t_ResourceTable;
    ip += task->t_ResourceCnt; /* go to last entry */
    cnt = 0;

    for (i = 0; i < task->t_ResourceCnt; i++)
    {
        it = *--ip;
        if (it == (f->fn.n_Item | ITEM_WAS_OPENED))
            cnt++;
    }

    /* If there's only one open, it means the task is in the process of
     * closing the folio for the last time. We must therefore throw away
     * any outstanding items for that folio that this task might still have.
     */

    if (cnt == 1)
    {
        ip           = task->t_ResourceTable;
        ip          += task->t_ResourceCnt; /* go to last entry */
        folioSubSys  = NODETOSUBSYS(f);

        for (i = 0; i < task->t_ResourceCnt; i++)
        {
            it = *--ip;
            if ((it >= 0) && (it != (f->fn.n_Item | ITEM_WAS_OPENED)))
            {
                in = (ItemNode *)LookupItem(it & (~ITEM_WAS_OPENED));
                if (in && (in->n_SubsysType == folioSubSys))
                {
#ifdef DEVELOPMENT
                    if (task->t_ThreadTask)
                        printf("WARNING: thread");
                    else
                        printf("WARNING: task");

                    printf(" '%s' is closing the '%s' folio w/o having %s item $%06x\n         (type %d",task->t.n_Name,f->fn.n_Name,((it & ITEM_WAS_OPENED) ? "closed" : "deleted"),it,in->n_Type);
                    if ((in->n_Flags & NODE_NAMEVALID) && (in->n_Name))
                        printf(", name '%s')\n",in->n_Name);
                    else
                        printf(")\n");
#endif

                    if (it & ITEM_WAS_OPENED)
                    {
                        internalCloseItem(it & (~ITEM_WAS_OPENED),task);
                    }
                    else
                    {
                        internalDeleteItem(it,task);
                    }
                }
            }
        }
    }
}


/*****************************************************************************/


int
CloseFolio(Folio *f, Task *ct)
{
Err   ret;

    /* If we get this far we know we opened it up.
     * Decrement use count. If the use count drops to 0, and the folio
     * was demand-loaded, remove it from memory
     */

    DBUGCF(("CloseFolio: entering, folio = $%lx ('%s'), current open count = %d\n",f,f->fn.n_Name,f->f_OpenCount));
    if (f->f_CloseFolio)
    {
        DBUGCF(("CloseFolio: calling folio-specific close vector\n"));
        (*f->f_CloseFolio)(f);
        DBUGCF(("CloseFolio: folio specific close vector returned\n"));
    }
    f->f_OpenCount--;

    /* We need to check if the current task will no longer have any opens
     * on the folio once this call returns. If this is the case, we must
     * make sure to nuke any item in the resource table that were created
     * by the folio being closed.
     */

    ReleaseFolioItems(ct,f);

    ret = 0;

    if (!f->f_OpenCount)
    {
        DBUGCF(("CloseFolio: calling UnloadFolio\n"));
        ret = internalUnloadFolio(f);
    }

    DBUGCF(("CloseFolio: exiting with %d\n",ret));

    return (int) ret;
}


Err
internalSetFunction(Item folio, int32 num, int32 type, void *func)
{
    /* redirect vector in folio jump table */
    void *oldfunc;
    Folio *fp;
    uint32 *vp;
    if ( (KernelBase->kb_CurrentTask->t.n_Flags & TASK_SUPER) == 0)
	return BADPRIV;
    fp = (Folio *)CheckItem(folio, KERNELNODE, FOLIONODE);
    if (!fp)	return BADITEM;
    vp = (uint32 *)fp;
    switch (type)
    {
	case VTYPE_SWI :
		/* num is positive */
		if (fp->f_MaxSwiFunctions < num) return -1;
		vp -= fp->f_MaxUserFunctions;
		vp -= num+1;
		break;
	case VTYPE_SUPER:
		/* num is negative */
		if (fp->f_MaxUserFunctions < -num) return -1;
		vp -= fp->f_MaxUserFunctions;
		vp -= num+1;
		break;
	case VTYPE_USER:
		/* num is positive */
		if (fp->f_MaxUserFunctions < num) return -1;
		vp -= num;
		break;
	default : return -1;
    }
    oldfunc = (void *)(*vp);
    *vp = (uint32)func;
    return (Err)oldfunc;
}


/*****************************************************************************/


Folio *WhichFolio(int32 cntype)
{
uint32  subsys;
Folio  *f;

    subsys = SUBSYSPART(cntype);

    /* first look in the cache */
    if (subsys < NUM_FOLIOS_CACHED)
    {
        f = folioCache[subsys];
        if (f)
            return f;
    }

    /* if it's not in the cache, use the long route... */
    SCANLIST(KernelBase->kb_FolioList,f,Folio)
    {
        if (NODETOSUBSYS(f) == subsys)
            return f;
    }

    return NULL;
}
