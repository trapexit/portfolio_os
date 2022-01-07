/* $Id: memdebug.c,v 1.17 1994/12/09 20:35:22 vertex Exp $ */

/* Things to add to this code:
 *
 *   - Check for free pointers that are within a previously allocated block,
 *     and report the original block address
 *
 *   - When creating the memory debugging system, there should be a tag
 *     which lets the application specify that memory should be rationned.
 *     This would randomly return NULL for memory allocations, to test
 *     the robustness of the calling code.
 *
 *   - Add tag to allow user-definable cookie size
 */

#define MEMDEBUG

#include "types.h"
#include "nodes.h"
#include "item.h"
#include "list.h"
#include "semaphore.h"
#include "operror.h"
#include "debug.h"
#include "string.h"
#include "clib.h"
#include "stdio.h"
#include "super.h"
#include "mem.h"


/****************************************************************************/


/* pull in version string for the link library */
#ifdef DEVELOPMENT
extern char *memdebuglib_version;
static void *memdlib_version = &memdebuglib_version;
#endif


/*****************************************************************************/


/* the real kernel entry points, stubs are in allocvectors.s */
void *KernelAllocMemFromMemLists(List *l, int32 size, uint32 typebits);
void KernelFreeMemToMemLists(List *l, void *p, int32 size);
void __swi(KERNELSWI+13) *KernelAllocMemBlocks(int32 size, uint32 typebits);
void Kernelkprintf(const char *fmt,...);
Err KernelLockSemaphore(Item sem, uint32 wait);
Err KernelUnlockSemaphore(Item sem);


/*****************************************************************************/


#define NUM_HASH_ENTRIES 16
#define HASH_MEM(x)      (((((uint32)x) & 0x000000f0) >> 4) & 0xf)
#define COOKIE_SIZE      16
#define ROUND_MEMSIZE(x) ((x + 3) & 0xfffffffc)


/* statistics about memory allocations */
typedef struct AllocStats
{
    uint32 as_CurrentAllocations;
    uint32 as_MaxAllocations;
    uint32 as_TotalAllocations;
    uint32 as_CurrentAllocated;
    uint32 as_MaxAllocated;
} AllocStats;

/* one of these exists for every allocation made (yes, I know it's big) */
typedef struct MemTrack
{
    struct MemTrack *mt_Next;  /* must be at head of structure due to the sneaky way we yank things from the singly-linked list */
    void            *mt_Memory;
    uint32           mt_MemorySize;
    const char      *mt_File;
    uint32           mt_LineNum;
    uint32           mt_CookieValue;
    MemDebugCalls    mt_Call;
    bool             mt_TrackSize;
} MemTrack;

/* one of these exists for every task/thread we know about */
typedef struct TaskTrack
{
    MinNode     tt_Link;
    Item        tt_Task;
    MemTrack   *tt_Memory[NUM_HASH_ENTRIES];        /* list of MemTrack structs */
    AllocStats  tt_DRAMStats;
    AllocStats  tt_VRAMStats;
} TaskTrack;


static List       tasks = INITLIST(tasks,NULL);
static Item       semaphore = -1;
static bool       allocPatterns;
static bool       freePatterns;
static bool       padCookies;
static bool       debugOnErrors;
static bool       allowOwnerSwitch;
static bool       checkAllocFailures;
static bool       keepTaskData;
static uint32     memTypeFlags;
static uint32     cookieValue = 0x11111111;
static AllocStats dramStats;
static AllocStats vramStats;


/*****************************************************************************/


#define OUTPUT(x) xprintf x

static int32 xprintfputcUser(char c, void *dummy)
{
    kprintf("%c",c);
    return c;
}

static int32 xprintfputcSuper(char c, void *dummy)
{
    Kernelkprintf("%c",c);
    return c;
}

extern int __vfprintf(void *, const char *fmt, va_list args, void *, int32 (*putc)(char,void *));


/* custom version of printf() that knows about user-mode vs supervisor mode */
static void xprintf(const char *fmt, ...)
{
va_list args;

    va_start(args, fmt);
    if (isUser())
    {
        __vfprintf(NULL, fmt, args, NULL, xprintfputcUser);
        xprintfputcUser(0,NULL);
    }
    else
    {
        __vfprintf(NULL, fmt, args, NULL, xprintfputcSuper);
        xprintfputcSuper(0,NULL);
    }

    va_end(a);
}


/*****************************************************************************/


/* fill memory with a 32-bit pattern */
static void FillMem(void *mem, uint32 numBytes, uint32 pattern)
{
uint32  extra;
char   *ch;
uint32 *l;

    extra    = numBytes % 4;
    l        = (uint32 *)mem;
    numBytes = numBytes / 4;
    while (numBytes--)
        *l++ = pattern;

    /* the following puts the data in reverse within memory, but who
     * cares... Only use I've ever found for an Intel processor :-)
     */

    ch = (char *)l;
    while (extra--)
    {
        *ch = (char)(pattern % 256);
        pattern /= 256;
    }
}


/*****************************************************************************/


/* check that a memory blocks contains a specific 32-bit pattern */
static bool CheckMem(void *mem, uint32 numBytes, uint32 pattern)
{
uint32 *l;

    l = (uint32 *)mem;
    while (numBytes)
    {
        if (*l++ != pattern)
            return (FALSE);

        numBytes -= 4;
    }

    return (TRUE);
}


/*****************************************************************************/


/* invoked when an error condition is detected. Calls the debugger if needed */
static void MemError(void)
{
    if (debugOnErrors)
        Debug();
}


/*****************************************************************************/


/* lock the module's semaphore */
static Err LockSem(void)
{
    if (isUser())
        return LockSemaphore(semaphore,TRUE);
    else
        return KernelLockSemaphore(semaphore,TRUE);
}


/*****************************************************************************/


/* unlock the module's semaphore */
static void UnlockSem(void)
{
    if (isUser())
        UnlockSemaphore(semaphore);
    else
        KernelUnlockSemaphore(semaphore);
}


/*****************************************************************************/


/* we're about to say something.... */
static void OutputTitle(const char *title)
{
    OUTPUT(("\nMEMDEBUG; %s\n",title));
}


/*****************************************************************************/


/* output information about the given task */
static void OutputTask(const char *header, Item task)
{
Task *t;

    if (task == 0)
        task = CURRENTTASK->t.n_Item;

    OUTPUT(("  %s",header));

    t = (Task *)CheckItem(task,KERNELNODE,TASKNODE);
    if (t)
    {
        OUTPUT(("name %s, addr $%x, item $%x\n",t->t.n_Name,t,task));
    }
    else
    {
        OUTPUT(("<dead task>, item $%x\n",task));
    }
}


/*****************************************************************************/


/* output information about the given allocation and its origins */
static void OutputSrc(const char *header, uint32 memSize,
                      const char *sourceFile, uint32 sourceLine)
{
    OUTPUT(("  %s",header));

    if (sourceFile)
    {
        OUTPUT(("size %d, file '%s', line %d\n",memSize,sourceFile,sourceLine));
    }
    else
    {
        OUTPUT(("size %d, <unknown source file>\n",memSize));
    }
}


/*****************************************************************************/


static void OutputCall(const char *header, MemDebugCalls call)
{
char *str;

    OUTPUT(("  %s",header));

    switch (call)
    {
        case MEMDEBUG_CALL_ALLOCMEMBLOCKS      : str = "AllocMemBlocks"; break;
        case MEMDEBUG_CALL_ALLOCMEM            : str = "AllocMem"; break;
        case MEMDEBUG_CALL_ALLOCMEMFROMMEMLISTS: str = "AllocMem()/AllocMemFromMemLists"; break;
        case MEMDEBUG_CALL_MALLOC              : str = "malloc"; break;
        case MEMDEBUG_CALL_CALLOC              : str = "calloc"; break;
        case MEMDEBUG_CALL_REALLOC             : str = "realloc"; break;
        case MEMDEBUG_CALL_FREEMEM             : str = "FreeMem"; break;
        case MEMDEBUG_CALL_FREEMEMTOMEMLISTS   : str = "FreeMem()/FreeMemToMemLists"; break;
        case MEMDEBUG_CALL_FREE                : str = "free"; break;
        default                                : str = NULL;
    }

    if (str)
        OUTPUT(("%s()\n",str));
}


/*****************************************************************************/


static void OutputAllocator(const TaskTrack *tt, const MemTrack *mt)
{
    OutputTask("Allocation Task/Thread  : ",tt->tt_Task);
    OutputSrc ("Allocation              : ",mt->mt_MemorySize,mt->mt_File,mt->mt_LineNum);
    OutputCall("Allocation Call         : ",mt->mt_Call);
}


/*****************************************************************************/


static void OutputDeallocator(uint32 memSize,
                              const char *sourceFile, uint32 sourceLine,
                              MemDebugCalls call)
{
    OutputTask("Deallocation Task/Thread: ",0);
    OutputSrc ("Deallocation            : ",memSize,sourceFile,sourceLine);
    OutputCall("Deallocation Call       : ",call);
}


/*****************************************************************************/


static void OutputCookies(const MemTrack *mt, void *mem)
{
uint32  i;
uint32 *u;

    OUTPUT(("  Original Cookie         : "));
    for (i = 0; i < COOKIE_SIZE / 4; i++)
    {
        OUTPUT(("$%08x ",mt->mt_CookieValue));
    }
    OUTPUT(("\n"));

    OUTPUT(("  Modified Cookie         : "));
    u = (uint32 *)mem;
    for (i = 0; i < COOKIE_SIZE / 4; i++)
    {
        OUTPUT(("$%08x ",*u++));
    }
    OUTPUT(("\n"));
}


/*****************************************************************************/


static void OutputStats(const char *header, const AllocStats *as)
{
    OUTPUT(("%s",header));
    OUTPUT(("      Current number of allocations            : %d\n",as->as_CurrentAllocations));
    OUTPUT(("      Max number of allocations at any one time: %d\n",as->as_MaxAllocations));
    OUTPUT(("      Total number of allocation calls         : %d\n",as->as_TotalAllocations));
    OUTPUT(("      Current amount allocated                 : %d bytes\n",as->as_CurrentAllocated));
    OUTPUT(("      Max amount allocated at any one time     : %d bytes\n",as->as_MaxAllocated));
}


/*****************************************************************************/


/* debugging version of memory allocator. Tracks the allocation, and calls
 * the real kernel allocator to get memory
 */
void *AllocMemFromMemListsDebug(List *l, uint32 memSize, uint32 memFlags,
                                const char *sourceFile, uint32 sourceLine,
                                MemDebugCalls call)
{
Item        task;
TaskTrack  *tt;
MemTrack   *mt;
bool        found;
void       *mem;
bool        doCookie;
uint32      memType;
AllocStats *taskStats;
AllocStats *globalStats;
uint32      pageSize;

    if (memSize == 0)
    {
        OutputTitle("attempt to allocate a block of 0 bytes in size");
        OutputTask ("Allocation Task/Thread  : ",0);
        OutputSrc  ("Allocation              : ",memSize,sourceFile,sourceLine);
        OutputCall ("Allocation Call         : ",call);
        MemError();

        return NULL;
    }

    if (!l || ((uint32)l & 3))
    {
        OutputTitle("attempt to allocate from a NULL or corrupt memory list");
        OutputTask ("Allocation Task/Thread  : ",0);
        OutputSrc  ("Allocation              : ",memSize,sourceFile,sourceLine);
        OutputCall ("Allocation Call         : ",call);
        MemError();

        return NULL;
    }

    if (LockSem() >= 0)
    {
        /* find the node we have created for this task/thread */

        found = FALSE;
        task  = CURRENTTASK->t.n_Item;
        SCANLIST(&tasks,tt,TaskTrack)
        {
            if (tt->tt_Task == task)
            {
                /* If we found the node, and it's not the first one on the
                 * list, move it to the head. This will make it faster to find
                 * the node the next time through.
                 */
                if (FIRSTNODE(&tasks) != (Node *)tt)
                {
                    RemNode((Node *)tt);
                    AddHead(&tasks,(Node *)tt);
                }
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            /* if we didn't find a task node, we must fabricate one */

            tt = (TaskTrack *)KernelAllocMemFromMemLists(l,sizeof(TaskTrack),MEMTYPE_FILL | memTypeFlags);
            if (tt)
            {
                tt->tt_Task = CURRENTTASK->t.n_Item;
                AddHead(&tasks,(Node *)tt);
            }
        }

        if (tt)
        {
            /* now allocate a memory tracking node */

            mt = (MemTrack *)KernelAllocMemFromMemLists(l,sizeof(MemTrack),memTypeFlags);
            if (mt)
            {
                /* now allocate the memory that the caller wants */

                if (call == MEMDEBUG_CALL_ALLOCMEMBLOCKS)
                {
                    doCookie = FALSE;
                    mem = KernelAllocMemBlocks(memSize,memFlags);
                    if (mem)
                    {
                        if (MEMTYPE_FILL & memFlags)
                        {
                            /* try to figure out how many bytes were really allocated */
                            pageSize = GetPageSize(memFlags);
                            memSize = ((memSize + pageSize - 1) / pageSize) * pageSize;
                        }
                        else
                        {
                            /* get the size from the first longword of the allocation */
                            memSize = *(uint32 *)mem;
                        }
                    }
                }
                else if (((MEMTYPE_STARTPAGE|MEMTYPE_INPAGE|MEMTYPE_SYSTEMPAGESIZE) & memFlags) || !padCookies)
                {
                    /* We don't do cookies if the user doesn't want 'em, or
                     * if any type of alignment is required, since the cookie
                     * would ruin the alignment
                     */

                    doCookie = FALSE;
                    mem = KernelAllocMemFromMemLists(l,memSize,memFlags & (~MEMTYPE_TRACKSIZE));
                }
                else
                {
                    doCookie = TRUE;
                    mem = KernelAllocMemFromMemLists(l,ROUND_MEMSIZE(memSize) + COOKIE_SIZE*2,memFlags & (~MEMTYPE_TRACKSIZE));
                }

                if (mem)
                {
                    if (doCookie)
                    {
                        /* put our cookies in place */

                        FillMem(mem,COOKIE_SIZE,cookieValue);
                        FillMem((void *)((uint32)mem + COOKIE_SIZE + ROUND_MEMSIZE(memSize)), COOKIE_SIZE, cookieValue);
                        mem = (void *)((uint32)mem + COOKIE_SIZE);
                    }

                    /* init the memory tracking node, and add it to the task's
                     * memory tracking list
                     */

                    mt->mt_Next        = tt->tt_Memory[HASH_MEM(mem)];
                    mt->mt_Memory      = mem;
                    mt->mt_MemorySize  = memSize;
                    mt->mt_File        = sourceFile;
                    mt->mt_LineNum     = sourceLine;
                    mt->mt_Call        = call;
                    mt->mt_TrackSize   = (memFlags & MEMTYPE_TRACKSIZE) ? TRUE : FALSE;

                    if (doCookie)
                        mt->mt_CookieValue = cookieValue;
                    else
                        mt->mt_CookieValue = 0;

                    cookieValue += 2;

                    if (allocPatterns)
                    {
                        /* fill the allocated memory with a pattern, but
                         * only if the user didn't ask for it to be filled
                         * by the allocator
                         */
                        if (!(MEMTYPE_FILL & memFlags))
                        {
                            if (memSize > 4)
                            {
                                /* we must skip the first 4 bytes, which contains
                                 * the size of the allocation
                                 */
                                FillMem((void *)((uint32)mem + sizeof(uint32)),memSize - sizeof(uint32),MEMDEBUG_ALLOC_PATTERN);
                            }
                        }
                    }

                    tt->tt_Memory[HASH_MEM(mem)] = mt;

                    memType = GetMemType(mem);
                    if (memType & MEMTYPE_VRAM)
                    {
                        taskStats   = &tt->tt_VRAMStats;
                        globalStats = &vramStats;
                    }
                    else
                    {
                        taskStats   = &tt->tt_DRAMStats;
                        globalStats = &dramStats;
                    }

                    /* update stats for this task/thread */

                    taskStats->as_CurrentAllocated += memSize;
                    if (taskStats->as_CurrentAllocated > taskStats->as_MaxAllocated)
                        taskStats->as_MaxAllocated = taskStats->as_CurrentAllocated;

                    taskStats->as_TotalAllocations++;
                    taskStats->as_CurrentAllocations++;
                    if (taskStats->as_CurrentAllocations > taskStats->as_MaxAllocations)
                        taskStats->as_MaxAllocations = taskStats->as_CurrentAllocations;

                    /* update global stats */

                    globalStats->as_CurrentAllocated += memSize;
                    if (globalStats->as_CurrentAllocated > globalStats->as_MaxAllocated)
                        globalStats->as_MaxAllocated = globalStats->as_CurrentAllocated;

                    globalStats->as_TotalAllocations++;
                    globalStats->as_CurrentAllocations++;
                    if (globalStats->as_CurrentAllocations > globalStats->as_MaxAllocations)
                        globalStats->as_MaxAllocations = globalStats->as_CurrentAllocations;

                    UnlockSem();

                    return (mem);
                }

                KernelFreeMemToMemLists(l,mt,sizeof(MemTrack));
            }

            if (!found)
            {
                RemNode((Node *)tt);
                KernelFreeMemToMemLists(l,tt,sizeof(TaskTrack));
            }
        }

        UnlockSem();
    }
    else
    {
        OUTPUT(("MEMDEBUG: This executable is linked with memdebug.lib,\n"));
        OUTPUT(("          and an attempt was made to allocate memory\n"));
        OUTPUT(("          before CreateMemDebug() was called.\n"));
    }

    if (checkAllocFailures)
    {
        OutputTitle("unable to allocate memory");
        OutputTask ("Allocation Task/Thread  : ",0);
        OutputSrc  ("Allocation              : ",memSize,sourceFile,sourceLine);
        OutputCall ("Allocation Call         : ",call);
        MemError();
    }

    return (NULL);
}


/*****************************************************************************/


static bool ReleaseMemory(List *l, void *mem, int32 memSize, bool checkAllTasks,
                          const char *sourceFile, uint32 sourceLine,
                          MemDebugCalls call)
{
Item       task;
TaskTrack *tt;
MemTrack  *mt;
MemTrack  *prev;
uint32     freeSize;
uint32     memType;

    /* find the node we have created for this task/thread */

    task = CURRENTTASK->t.n_Item;
    SCANLIST(&tasks,tt,TaskTrack)
    {
        if ((tt->tt_Task == task) || checkAllTasks)
        {
            /* now we have a task node, find the memory tracking node that's
             * associated with the memory block
             */

            mt   = tt->tt_Memory[HASH_MEM(mem)];
            prev = (MemTrack *)&tt->tt_Memory[HASH_MEM(mem)];  /* sneaky... */
            while (mt)
            {
                if (mt->mt_Memory == mem)
                {
                    /* We've found a memory tracking node, free it */

                    if ((task != tt->tt_Task) && !allowOwnerSwitch)
                    {
                        OutputTitle("different task for allocation and deallocation");
                        OutputAllocator(tt,mt);
                        OutputDeallocator(memSize,sourceFile,sourceLine,call);
                        MemError();
                    }

                    if (mt->mt_TrackSize)
                    {
                        if (memSize >= 0)
                        {
                            OutputTitle("positive size given for deallocation of MEMTYPE_TRACKSIZE");
                            OutputAllocator(tt,mt);
                            OutputDeallocator(memSize,sourceFile,sourceLine,call);
                            MemError();
                        }
                    }
                    else
                    {
                        if (memSize != mt->mt_MemorySize)
                        {
                            OutputTitle("inconsistent size for allocation and deallocation");
                            OutputAllocator(tt,mt);
                            OutputDeallocator(memSize,sourceFile,sourceLine,call);
                            MemError();
                        }
                    }

                    prev->mt_Next = mt->mt_Next;

                    if (mt->mt_CookieValue)
                    {
                        mem      = (void *)((uint32)mem - COOKIE_SIZE);
                        freeSize = ROUND_MEMSIZE(mt->mt_MemorySize) + COOKIE_SIZE*2;

                        if (!CheckMem(mem,COOKIE_SIZE,mt->mt_CookieValue))
                        {
                            OutputTitle("cookie before allocation was modified");
                            OutputAllocator(tt,mt);
                            OutputDeallocator(memSize,sourceFile,sourceLine,call);
                            OutputCookies(mt,mem);
                            MemError();
                        }

                        if (!CheckMem((void *)((uint32)mem + freeSize - COOKIE_SIZE),COOKIE_SIZE,mt->mt_CookieValue))
                        {
                            OutputTitle("cookie after allocation was modified");
                            OutputAllocator(tt,mt);
                            OutputDeallocator(memSize,sourceFile,sourceLine,call);
                            OutputCookies(mt,(void *)((uint32)mem + freeSize - COOKIE_SIZE));
                            MemError();
                        }
                    }
                    else
                    {
                        freeSize = mt->mt_MemorySize;
                    }

                    /* use true size in case the wrong size was being freed */
                    memSize = mt->mt_MemorySize;

                    if (freePatterns)
                    {
                        FillMem(mem,freeSize,MEMDEBUG_FREE_PATTERN);
                        FillMem(mt,sizeof(MemTrack),MEMDEBUG_FREE_PATTERN);
                    }

                    KernelFreeMemToMemLists(l,mem,freeSize);
                    KernelFreeMemToMemLists(l,mt,sizeof(MemTrack));

                    memType = GetMemType(mem);
                    if (memType & MEMTYPE_VRAM)
                    {
                        tt->tt_VRAMStats.as_CurrentAllocated -= memSize;
                        tt->tt_VRAMStats.as_CurrentAllocations--;

                        vramStats.as_CurrentAllocated -= memSize;
                        vramStats.as_CurrentAllocations--;
                    }
                    else
                    {
                        tt->tt_DRAMStats.as_CurrentAllocated -= memSize;
                        tt->tt_DRAMStats.as_CurrentAllocations--;

                        dramStats.as_CurrentAllocated -= memSize;
                        dramStats.as_CurrentAllocations--;
                    }

                    if (tt->tt_DRAMStats.as_CurrentAllocations + tt->tt_VRAMStats.as_CurrentAllocations == 0)
                    {
                        /* if there are no more allocations attributed to
                         * this task, zap the task node
                         *
                         * if keepTaskData is TRUE, then we should keep the
                         * node around because the caller wants to get some
                         * stats on this task's allocations.
                         */

                        if (!keepTaskData)
                        {
                            RemNode((Node *)tt);
                            KernelFreeMemToMemLists(l,tt,sizeof(TaskTrack));
                        }
                    }

                    return (TRUE);
                }

                prev = mt;
                mt   = mt->mt_Next;
            }

            if (!checkAllTasks)
                break;
        }
    }

    return (FALSE);
}


/*****************************************************************************/


void FreeMemToMemListsDebug(List *l, void *mem, uint32 memSize,
                            const char *sourceFile, uint32 sourceLine,
                            MemDebugCalls call)
{
    if (!mem)
        return;

    if (!l || ((uint32)l & 3))
    {
        OutputTitle("attempt to free to a NULL or corrupt memory list");
        OutputDeallocator(memSize,sourceFile,sourceLine,call);
        MemError();
    }

    if (LockSem() >= 0)
    {
        if (!ReleaseMemory(l,mem,memSize,FALSE,sourceFile,sourceLine,call))
        {
            if (!ReleaseMemory(l,mem,memSize,TRUE,sourceFile,sourceLine,call))
            {
                OutputTitle("memory not allocated by a known task/thread");
                OutputDeallocator(memSize,sourceFile,sourceLine,call);
                MemError();
            }
        }

        UnlockSem();
    }
}


/*****************************************************************************/


void *AllocMemBlocksDebug(int32 memSize, uint32 memFlags,
                          const char *sourceFile, uint32 sourceLine)
{
    return AllocMemFromMemListsDebug(CURRENTTASK->t_FreeMemoryLists, memSize, memFlags,
                                     sourceFile, sourceLine, MEMDEBUG_CALL_ALLOCMEMBLOCKS);
}


/*****************************************************************************/


void *mallocDebug(int32 size, const char *sourceFile, uint32 sourceLine)
{
uint32 *ret;

    if (size == 0)
    {
        AllocMemFromMemListsDebug(CURRENTTASK->t_FreeMemoryLists,
                                  0,MEMTYPE_ANY, sourceFile, sourceLine,
                                  MEMDEBUG_CALL_MALLOC);

        return (NULL);
    }

    ret = (uint32 *)AllocMemFromMemListsDebug(CURRENTTASK->t_FreeMemoryLists,
                  size + sizeof(uint32),MEMTYPE_ANY, sourceFile, sourceLine,
                  MEMDEBUG_CALL_MALLOC);

    if (ret)
    {
        ret[0] = size;
        return (void *)(&ret[1]);
    }

    return (NULL);
}


/*****************************************************************************/


void *callocDebug(size_t nelem, size_t elsize, const char *sourceFile, uint32 sourceLine)
{
uint32  size;
uint32 *ret;

    size = nelem * elsize;

    if (size == 0)
    {
        AllocMemFromMemListsDebug(CURRENTTASK->t_FreeMemoryLists,
                                  0,MEMTYPE_ANY|MEMTYPE_FILL,sourceFile,sourceLine,
                                  MEMDEBUG_CALL_CALLOC);

        return NULL;
    }

    ret = (uint32 *)AllocMemFromMemListsDebug(CURRENTTASK->t_FreeMemoryLists,
        size + sizeof(uint32),MEMTYPE_ANY|MEMTYPE_FILL,sourceFile,sourceLine,
        MEMDEBUG_CALL_CALLOC);

    if (ret)
    {
        ret[0] = size;
        return (void *)(&ret[1]);
    }

    return (NULL);
}


/*****************************************************************************/


void freeDebug(void *mem, const char *sourceFile, uint32 sourceLine)
{
uint32 *p;

    if (mem)
    {
	p = (uint32 *)mem;
	p--;
	FreeMemToMemListsDebug(CURRENTTASK->t_FreeMemoryLists,
                               p,p[0] + sizeof(uint32),sourceFile,sourceLine,
                               MEMDEBUG_CALL_FREE);
    }
}


/*****************************************************************************/


void *reallocDebug(void *oldBlock, size_t newSize, const char *sourceFile, uint32 sourceLine)
{
void   *newBlock;
size_t  oldSize;
uint32 *p;

    oldSize = 0;  /* keep the silly compiler from complaining */
    if (oldBlock)
    {
        oldSize = *(uint32 *)((uint32)oldBlock - sizeof(uint32));
    }
    else if (newSize == 0)
    {
        AllocMemFromMemListsDebug(CURRENTTASK->t_FreeMemoryLists,
                                  0,MEMTYPE_ANY, sourceFile, sourceLine,
                                  MEMDEBUG_CALL_REALLOC);

        return NULL;
    }

    p = (uint32 *)AllocMemFromMemListsDebug(CURRENTTASK->t_FreeMemoryLists,
                    newSize + sizeof(uint32),MEMTYPE_ANY, sourceFile, sourceLine,
                    MEMDEBUG_CALL_REALLOC);

    if (p)
    {
        p[0]     = newSize;
        newBlock = &p[1];
    }
    else
    {
        newBlock = NULL;
    }

    if (newBlock && oldBlock)
        memcpy(newBlock,oldBlock,newSize < oldSize ? newSize : oldSize);

    /* should this be freed when the new allocation failed? Let's assume
     * the answer is yes...
     */
    if (oldBlock)
    {
	p = (uint32 *)oldBlock;
	p--;
	FreeMemToMemListsDebug(CURRENTTASK->t_FreeMemoryLists,
                               p,p[0] + sizeof(uint32),sourceFile,sourceLine,
                               MEMDEBUG_CALL_REALLOC);
    }

    return (newBlock);
}


/*****************************************************************************/


/* stubs to override routines in clib.lib */

#undef AllocMemBlocks
#undef AllocMemFromMemLists
#undef FreeMemToMemLists
#undef malloc
#undef calloc
#undef free
#undef realloc

void *AllocMemFromMemLists(List *l, uint32 memSize, uint32 memFlags)
{
    return AllocMemFromMemListsDebug(l,memSize,memFlags,NULL,0,MEMDEBUG_CALL_ALLOCMEMFROMMEMLISTS);
}

void FreeMemToMemLists(List *l, void *mem, uint32 memSize)
{
    FreeMemToMemListsDebug(l,mem,memSize,NULL,0,MEMDEBUG_CALL_FREEMEMTOMEMLISTS);
}

void *AllocMemBlocks(int32 size, uint32 typebits)
{
    return AllocMemBlocksDebug(size,typebits,NULL,0);
}

void *malloc(int32 size)
{
    return mallocDebug(size,NULL,0);
}

void *calloc(size_t nelem, size_t elsize)
{
    return callocDebug(nelem,elsize,NULL,0);
}

void free(void *mem)
{
    freeDebug(mem,NULL,0);
}

void *realloc(void *oldBlock, size_t newSize)
{
    return reallocDebug(oldBlock,newSize,NULL,0);
}


/*****************************************************************************/


Err CreateMemDebug(uint32 controlFlags, const TagArg *args)
{
    if (controlFlags & ~(MEMDEBUGF_ALLOC_PATTERNS |
                         MEMDEBUGF_FREE_PATTERNS |
                         MEMDEBUGF_PAD_COOKIES |
                         MEMDEBUGF_DEBUG_ON_ERRORS |
                         MEMDEBUGF_ALLOW_OWNER_SWITCH |
                         MEMDEBUGF_CHECK_ALLOC_FAILURES |
                         MEMDEBUGF_KEEP_TASK_DATA |
                         MEMDEBUGF_USE_VRAM))
    {
        /* illegal flags */
        return (MEMDEBUG_ERR_BADFLAGS);
    }

    if (args)
    {
        /* illegal tags, we don't support any yet */
        return (MEMDEBUG_ERR_BADTAG);
    }

    allocPatterns      = FALSE;
    freePatterns       = FALSE;
    padCookies         = FALSE;
    debugOnErrors      = FALSE;
    allowOwnerSwitch   = FALSE;
    checkAllocFailures = FALSE;
    keepTaskData       = FALSE;
    memTypeFlags       = MEMTYPE_ANY;

    if (controlFlags & MEMDEBUGF_ALLOC_PATTERNS)
        allocPatterns = TRUE;

    if (controlFlags & MEMDEBUGF_FREE_PATTERNS)
        freePatterns = TRUE;

    if (controlFlags & MEMDEBUGF_PAD_COOKIES)
        padCookies = TRUE;

    if (controlFlags & MEMDEBUGF_DEBUG_ON_ERRORS)
        debugOnErrors = TRUE;

    if (controlFlags & MEMDEBUGF_ALLOW_OWNER_SWITCH)
        allowOwnerSwitch = TRUE;

    if (controlFlags & MEMDEBUGF_CHECK_ALLOC_FAILURES)
        checkAllocFailures = TRUE;

    if (controlFlags & MEMDEBUGF_KEEP_TASK_DATA)
        keepTaskData = TRUE;

    if (controlFlags & MEMDEBUGF_USE_VRAM)
        memTypeFlags = MEMTYPE_VRAM;

    memset(&dramStats,0,sizeof(dramStats));
    memset(&vramStats,0,sizeof(vramStats));

    semaphore = CreateSemaphore(NULL,0);

    return (semaphore);
}


/*****************************************************************************/


Err DeleteMemDebug(void)
{
uint32     i;
TaskTrack *tt;
MemTrack  *mt;
MemTrack  *next;

    if (LockSem() >= 0)
    {
        while (TRUE)
        {
            tt = (TaskTrack *)RemHead(&tasks);
            if (!tt)
                break;

            for (i = 0; i < NUM_HASH_ENTRIES; i++)
            {
                mt = tt->tt_Memory[i];
                while (mt)
                {
                    next = mt->mt_Next;
                    KernelFreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,mt->mt_Memory,mt->mt_MemorySize);
                    KernelFreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,mt,sizeof(MemTrack));
                    mt = next;
                }
            }

            KernelFreeMemToMemLists(CURRENTTASK->t_FreeMemoryLists,tt,sizeof(TaskTrack));
        }

        UnlockSem();
    }

    return (DeleteSemaphore(semaphore));
}


/*****************************************************************************/


Err DumpMemDebug(const TagArg *args)
{
TaskTrack *tt;
MemTrack  *mt;
uint32     i;
Task      *task;
Err        err;

    if (args)
        return (MEMDEBUG_ERR_BADTAG);

    OutputTitle("DumpMemDebug()");

    err = LockSem();
    if (err >= 0)
    {
        SCANLIST(&tasks,tt,TaskTrack)
        {
            if (FIRSTNODE(&tasks) != (Node *)tt)
            {
                OUTPUT(("\n"));
            }

            task = (Task *)CheckItem(tt->tt_Task,KERNELNODE,TASKNODE);
            if (task)
            {
                OUTPUT(("  Task: name %s, addr $%x, item $%x\n",task->t.n_Name,task,tt->tt_Task));
            }
            else
            {
                OUTPUT(("  Task: <dead task>, item $%x\n",tt->tt_Task));
            }

            OutputStats("    DRAM\n",&tt->tt_DRAMStats);
            OutputStats("    VRAM\n",&tt->tt_VRAMStats);

            OUTPUT(("    Allocations\n"));
            OUTPUT(("      Address     Size  Source Line  Source File\n"));

            for (i = 0; i < NUM_HASH_ENTRIES; i++)
            {
                mt = tt->tt_Memory[i];
                while (mt)
                {
                    if (mt->mt_File)
                    {
                        OUTPUT(("      $%06x  %7d    %5d",mt->mt_Memory,mt->mt_MemorySize,mt->mt_LineNum));
                        OUTPUT(("     '%s'\n",mt->mt_File));
                    }
                    else
                    {
                        OUTPUT(("      $%06x  %7d    <unknown source file>\n",mt->mt_Memory,mt->mt_MemorySize));
                    }
                    mt = mt->mt_Next;
                }
            }
        }

        if (!IsEmptyList(&tasks))
            OUTPUT(("\n"));

        OUTPUT(("  Global History\n"));
        OutputStats("    DRAM\n",&dramStats);
        OutputStats("    VRAM\n",&vramStats);

        UnlockSem();
    }

    return (err);
}


/*****************************************************************************/


Err SanityCheckMemDebug(const TagArg *args)
{
TaskTrack *tt;
MemTrack  *mt;
uint32     i;
Task      *task;
Err        err;
void      *mem;
uint32     memSize;

    if (args)
        return (MEMDEBUG_ERR_BADTAG);

    err = LockSem();
    if (err >= 0)
    {
        SCANLIST(&tasks,tt,TaskTrack)
        {
            task = (Task *)CheckItem(tt->tt_Task,KERNELNODE,TASKNODE);
            if (!task && !allowOwnerSwitch)
            {
                OutputTitle("SanityCheckMemDebug(), dead task still has allocations");
                OutputTask("Allocation Task/Thread: ",tt->tt_Task);
                OUTPUT(("          DRAM Allocations      : %d",tt->tt_DRAMStats.as_CurrentAllocations));
                OUTPUT(("          VRAM Allocations      : %d",tt->tt_VRAMStats.as_CurrentAllocations));
                MemError();
            }

            for (i = 0; i < NUM_HASH_ENTRIES; i++)
            {
                mt = tt->tt_Memory[i];
                while (mt)
                {
                    if (mt->mt_CookieValue)
                    {
                        mem      = mt->mt_Memory;
                        mem      = (void *)((uint32)mem - COOKIE_SIZE);
                        memSize  = ROUND_MEMSIZE(mt->mt_MemorySize) + COOKIE_SIZE*2;

                        if (!CheckMem(mem,COOKIE_SIZE,mt->mt_CookieValue))
                        {
                            OutputTitle("SanityCheckMemDebug(), cookie before allocation was modified");
                            OutputAllocator(tt,mt);
                            OutputCookies(mt,mem);
                            MemError();
                        }

                        if (!CheckMem((void *)((uint32)mem + memSize - COOKIE_SIZE),COOKIE_SIZE,mt->mt_CookieValue))
                        {
                            OutputTitle("SanityCheckMemDebug(), cookie after allocation was modified");
                            OutputAllocator(tt,mt);
                            OutputCookies(mt,(void *)((uint32)mem + memSize - COOKIE_SIZE));
                            MemError();
                        }
                    }
                    mt = mt->mt_Next;
                }
            }
        }

        UnlockSem();
    }

    return (err);
}
